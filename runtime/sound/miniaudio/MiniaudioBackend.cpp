// MiniaudioBackend.cpp — the ONLY translation unit where miniaudio meets the
// AudioBackend seam.  MINIAUDIO_IMPLEMENTATION is defined here so the ~100k-
// line header body compiles into exactly ONE object file and never leaks to
// consumers.  MiniaudioBackend.hpp is miniaudio-free (pImpl).
//
// Design: LAZY INIT.  The AudioBackend seam delivers sampleRate at render()
// time, but miniaudio nodes need it at construction.  Solution: createNode()
// stores a PendingNode; the first render() call initializes the ma_node_graph
// (with the known sampleRate), flushes all PendingNodes to real ma_* nodes,
// then replays deferred connect() calls.  Subsequent render() calls find the
// graph already initialized and just call ma_node_graph_read_pcm_frames.
//
// PINNED NODES: ma_data_source_node stores an internal pointer back to the
// ma_waveform it wraps. Any move/copy of the owning struct invalidates that
// pointer. Therefore MaNode is heap-allocated (std::unique_ptr<MaNode>) and
// never moved after creation. The map stores unique_ptr<MaNode> so nodes are
// stable at their original heap addresses throughout the backend's lifetime.
//
// Node mapping (synthesis chain, v1):
//   NodeKind::Oscillator  → ma_waveform + ma_data_source_node
//   NodeKind::Biquad      → ma_lpf_node / ma_hpf_node / ma_bpf_node
//                           wrapped by a ma_splitter_node: bus 0 feeds the
//                           filter (wet), bus 1 bypasses it (dry); the enabled
//                           flag mixes the two so a disabled filter passes its
//                           input through unchanged.
//   NodeKind::Gain        → ma_splitter_node (1-in 2-out, only bus 0 used)
//                           + ma_node_set_output_bus_volume
//   NodeKind::Panner      → ma_splitter_node (unity pass-through in mono graph)
//                           + ma_spatializer (applied in renderStereo)
//                           + ma_spatializer_listener (owned by Panner node)
//   NodeKind::Destination → ma_node_graph_get_endpoint (the graph's sink)
//
// connect(dst, src) seam contract: src feeds INTO dst.
// miniaudio contract: ma_node_attach_output_bus(src, 0, dst, 0).
//
// renderStereo for a Panner graph:
//   The mono node graph renders the signal through the Panner (as a unity
//   splitter), delivering it to the Destination endpoint.  renderStereo then
//   finds the first Panner node, renders the mono graph output, and feeds that
//   buffer through ma_spatializer_process_pcm_frames to produce interleaved
//   stereo.  If no Panner node exists, it falls back to duplicating the mono
//   signal into both channels (the pre-Task-4 behaviour).

#define MINIAUDIO_IMPLEMENTATION
// Suppress device/OS backend code — we use the node graph headlessly.
// MA_NO_DEVICE_IO trims the device I/O code entirely (no ALSA/PulseAudio/etc).
// Do NOT define MA_NO_THREADING: the node graph uses spinlocks internally for
// thread-safe output-bus attach/detach. The spinlocks use C11 atomics which
// require proper linking; MA_NO_THREADING would break the atomic path.
#define MA_NO_DEVICE_IO
#include "miniaudio.h"

#include "MiniaudioBackend.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>

namespace x3d::runtime::miniaudio {

// ─────────────────────────────────────────────────────────────────────────────
// Internal node representation (heap-pinned — never moved after init)
// ─────────────────────────────────────────────────────────────────────────────

enum class MaNodeKind { Oscillator, Biquad, Gain, Panner, Destination, Buffer };

// ─────────────────────────────────────────────────────────────────────────────
// Buffer source (NodeKind::Buffer, ADR-0050): decoded PCM played through a
// custom ma_data_source — read cursor at rate * srcRate/outRate, linear
// interpolation, wrapping at the end; stopped/paused read silence.
// ─────────────────────────────────────────────────────────────────────────────
struct BufferSource {
  ma_data_source_base base{};  // must be first: miniaudio casts to it
  const std::vector<float> *samples = nullptr;
  double srcRate = 44100.0, outRate = 44100.0, rate = 1.0, cursor = 0.0, gain = 1.0;
  int state = 0;               // 0 stopped, 1 playing, 2 paused
};

static ma_result bufRead(ma_data_source *ds, void *out, ma_uint64 frames, ma_uint64 *read) {
  auto *b = static_cast<BufferSource *>(ds);
  auto *dst = static_cast<float *>(out);
  const std::size_t len = b->samples ? b->samples->size() : 0;
  for (ma_uint64 i = 0; i < frames; ++i) {
    if (b->state != 1 || len == 0) { dst[i] = 0.0f; continue; }
    const std::size_t i0 = static_cast<std::size_t>(b->cursor) % len;
    const std::size_t i1 = (i0 + 1) % len;
    const double frac = b->cursor - std::floor(b->cursor);
    dst[i] = static_cast<float>(b->gain * ((*b->samples)[i0] + ((*b->samples)[i1] - (*b->samples)[i0]) * frac));
    b->cursor += b->rate * b->srcRate / b->outRate;
    if (b->cursor >= double(len)) b->cursor = std::fmod(b->cursor, double(len));
    if (b->cursor < 0.0) b->cursor = 0.0;
  }
  if (read) *read = frames;
  return MA_SUCCESS;
}
static ma_result bufSeek(ma_data_source *, ma_uint64) { return MA_SUCCESS; }
static ma_result bufFormat(ma_data_source *ds, ma_format *fmt, ma_uint32 *ch, ma_uint32 *sr,
                           ma_channel *map, size_t cap) {
  auto *b = static_cast<BufferSource *>(ds);
  if (fmt) *fmt = ma_format_f32;
  if (ch) *ch = 1;
  if (sr) *sr = static_cast<ma_uint32>(b->outRate);
  if (map) ma_channel_map_init_standard(ma_standard_channel_map_default, map, cap, 1);
  return MA_SUCCESS;
}
static ma_result bufCursor(ma_data_source *, ma_uint64 *c) { if (c) *c = 0; return MA_SUCCESS; }
static ma_result bufLength(ma_data_source *, ma_uint64 *l) { if (l) *l = 0; return MA_NOT_IMPLEMENTED; }
static ma_data_source_vtable g_bufferVtable = {bufRead, bufSeek, bufFormat, bufCursor, bufLength,
                                               nullptr, 0};

// §16.4.17 Sound attenuation (DistanceModel::Ellipsoid), computed here from the
// geometry that crossed the seam: two focus-anchored ellipsoids along
// `direction` reach 2FB / ((F+B) - (F-B)cos theta) towards the listener; full
// level inside the inner, linear in dB to -20 dB at the outer, silence beyond.
static double ellipsoidGain(const NodeParams &np) {
  const double vx = double(np.listenerPosition[0]) - np.sourcePosition[0];
  const double vy = double(np.listenerPosition[1]) - np.sourcePosition[1];
  const double vz = double(np.listenerPosition[2]) - np.sourcePosition[2];
  const double d = std::sqrt(vx * vx + vy * vy + vz * vz);
  const double dl = std::sqrt(double(np.direction[0]) * np.direction[0] +
                              double(np.direction[1]) * np.direction[1] +
                              double(np.direction[2]) * np.direction[2]);
  const double cosT = (d > 1e-12 && dl > 1e-12)
      ? (vx * np.direction[0] + vy * np.direction[1] + vz * np.direction[2]) / (d * dl) : 1.0;
  auto reach = [cosT](double f, double b) {
    const double den = (f + b) - (f - b) * cosT;
    return den > 1e-12 ? 2.0 * f * b / den : 0.0;
  };
  const double rMin = reach(np.minFront, np.minBack), rMax = reach(np.maxFront, np.maxBack);
  const double g = d <= rMin ? 1.0
                 : (d >= rMax || rMax <= rMin) ? 0.0
                 : std::pow(10.0, -(d - rMin) / (rMax - rMin));
  return g * np.intensity;
}

struct MaNode {
  MaNodeKind kind;
  NodeParams params;

  // Oscillator: waveform MUST NOT MOVE after ma_data_source_node_init because
  // dsNode.pDataSource points back into it.
  ma_waveform            waveform{};
  ma_data_source_node    dsNode{};

  // Biquad (only one valid at a time)
  ma_lpf_node            lpfNode{};
  ma_hpf_node            hpfNode{};
  ma_bpf_node            bpfNode{};

  // Gain
  ma_splitter_node       splitterNode{};

  // Buffer: the custom data source (pinned; dsNode above points back into it).
  BufferSource           bufferSrc{};

  // Panner: splitterNode (above) is used as the mono pass-through in the graph;
  // spatializer + listener are used by renderStereo.
  ma_spatializer         spatializer{};
  ma_spatializer_listener spatializerListener{};
  bool                   spatializerInited   = false;
  bool                   listenerInited      = false;

  // Pointer to the ma_node* interface (set during flush). For a Biquad this is
  // the bypass splitter; `filterNode` is the actual filter it feeds.
  ma_node* node = nullptr;
  ma_node* filterNode = nullptr;

  bool initialized = false;
  bool timeActive = true;

  // Prevent accidental copy/move (the structs are non-trivially copyable
  // anyway, but be explicit).
  MaNode(const MaNode&) = delete;
  MaNode& operator=(const MaNode&) = delete;
  MaNode(MaNode&&) = delete;
  MaNode& operator=(MaNode&&) = delete;
  MaNode() = default;
};

// Biquad bypass mix. The bypass splitter (`mn.node`) duplicates its input onto
// bus 0 (the wet path, into the filter) and bus 1 (the dry path, straight to the
// downstream node). Enabled opens the wet path and closes the dry; disabled does
// the reverse; inactive (PlaybackState 0) closes both. This is the live
// equivalent of BuiltinDspBackend's pass-through when !enabled.
static void applyBiquadMix(MaNode &mn) {
  const bool wet = mn.timeActive && mn.params.enabled;
  const bool dry = mn.timeActive && !mn.params.enabled;
  if (mn.filterNode) ma_node_set_output_bus_volume(mn.filterNode, 0, wet ? 1.0f : 0.0f);
  ma_node_set_output_bus_volume(mn.node, 1, dry ? 1.0f : 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Impl
// ─────────────────────────────────────────────────────────────────────────────

struct MiniaudioBackend::Impl {
  struct PendingNode {
    NodeKind  kind;
    NodeParams params;
    int    playState = 0;   // Buffer: state/rate set before the lazy flush
    double rate      = 1.0;
  };
  struct PendingConnect {
    NodeHandle dst;
    NodeHandle src;
  };

  std::unordered_map<NodeHandle, PendingNode>         pending;
  std::vector<PendingConnect>                         deferredConnects;

  ma_node_graph                                       graph{};
  bool                                                graphInited = false;
  float                                               initSampleRate = 0.0f;

  // Heap-pinned nodes: unique_ptr ensures the MaNode never moves.
  std::unordered_map<NodeHandle, std::unique_ptr<MaNode>> nodes;

  NodeHandle destHandle = 0;
  NodeHandle nextHandle = 1;

  ~Impl() {
    if (graphInited) {
      for (auto &[h, pn] : nodes) {
        if (!pn || !pn->initialized) continue;
        switch (pn->kind) {
        case MaNodeKind::Oscillator:
          ma_data_source_node_uninit(&pn->dsNode, nullptr);
          ma_waveform_uninit(&pn->waveform);
          break;
        case MaNodeKind::Biquad:
          ma_splitter_node_uninit(&pn->splitterNode, nullptr);
          if (pn->params.filterType == FilterType::Lowpass)
            ma_lpf_node_uninit(&pn->lpfNode, nullptr);
          else if (pn->params.filterType == FilterType::Highpass)
            ma_hpf_node_uninit(&pn->hpfNode, nullptr);
          else
            ma_bpf_node_uninit(&pn->bpfNode, nullptr);
          break;
        case MaNodeKind::Gain:
          ma_splitter_node_uninit(&pn->splitterNode, nullptr);
          break;
        case MaNodeKind::Panner:
          // Uninit the mono pass-through splitter.
          ma_splitter_node_uninit(&pn->splitterNode, nullptr);
          // Uninit the spatializer objects if they were successfully inited.
          if (pn->spatializerInited)
            ma_spatializer_uninit(&pn->spatializer, nullptr);
          if (pn->listenerInited)
            ma_spatializer_listener_uninit(&pn->spatializerListener, nullptr);
          break;
        case MaNodeKind::Destination:
          break; // owned by graph
        case MaNodeKind::Buffer:
          ma_data_source_node_uninit(&pn->dsNode, nullptr);
          ma_data_source_uninit(&pn->bufferSrc.base);
          break;
        }
      }
      ma_node_graph_uninit(&graph, nullptr);
    }
  }

  // ── helper: map seam DistanceModel → miniaudio ma_attenuation_model ─────────
  static ma_attenuation_model toMaAttenuation(DistanceModel dm) {
    switch (dm) {
    case DistanceModel::Linear:      return ma_attenuation_model_linear;
    case DistanceModel::Exponential: return ma_attenuation_model_exponential;
    case DistanceModel::Inverse:     // fall through (default)
    default:                         return ma_attenuation_model_inverse;
    }
  }

  // ── Wire src's output into dstNode's first input bus ────────────────────────
  // A Biquad fans out through its bypass splitter: the filtered (wet) output and
  // the dry splitter output both feed dstNode, which sums them per the seam's
  // "many inputs sum" contract, so `enabled` can mix them live.
  void attachOutput(MaNode &src, ma_node *dstNode) {
    auto attach = [&](ma_node *node, ma_uint32 bus) {
      ma_result r = ma_node_attach_output_bus(node, bus, dstNode, 0);
      if (r != MA_SUCCESS)
        std::fprintf(stderr,
                     "[MiniaudioBackend] ma_node_attach_output_bus failed (%d)\n",
                     r);
    };
    if (src.kind == MaNodeKind::Biquad) {
      if (src.filterNode) attach(src.filterNode, 0);
      attach(src.node, 1);
    } else {
      attach(src.node, 0);
    }
  }

  // ── flush: lazy-init the graph + all pending nodes ──────────────────────────
  bool flush(float sampleRate) {
    if (graphInited) return true;
    initSampleRate = sampleRate;
    auto sr = static_cast<ma_uint32>(sampleRate);

    // Init the node graph (mono, 1 channel).
    ma_node_graph_config cfg = ma_node_graph_config_init(/*channels=*/1);
    if (ma_node_graph_init(&cfg, nullptr, &graph) != MA_SUCCESS) {
      std::fprintf(stderr, "[MiniaudioBackend] ma_node_graph_init failed\n");
      return false;
    }
    graphInited = true;

    // Flush pending nodes — allocate each on the heap first so they're stable.
    for (auto &[handle, pn] : pending) {
      auto mn = std::make_unique<MaNode>();
      mn->params = pn.params;

      if (pn.kind == NodeKind::Destination) {
        mn->kind = MaNodeKind::Destination;
        mn->node = ma_node_graph_get_endpoint(&graph);
        mn->initialized = true;
        destHandle = handle;
        nodes[handle] = std::move(mn);
        continue;
      }

      if (pn.kind == NodeKind::Oscillator) {
        mn->kind = MaNodeKind::Oscillator;
        auto wtype = ma_waveform_type_sine;
        switch (pn.params.waveform) {
        case Waveform::Sine:     wtype = ma_waveform_type_sine;     break;
        case Waveform::Square:   wtype = ma_waveform_type_square;   break;
        case Waveform::Sawtooth: wtype = ma_waveform_type_sawtooth; break;
        case Waveform::Triangle: wtype = ma_waveform_type_triangle; break;
        }
        double freq = static_cast<double>(pn.params.frequency);
        if (pn.params.detune != 0.0f)
          freq *= std::pow(2.0, pn.params.detune / 1200.0);

        ma_waveform_config wcfg = ma_waveform_config_init(
            ma_format_f32, /*channels=*/1, sr, wtype,
            static_cast<double>(pn.params.gain), freq);
        if (ma_waveform_init(&wcfg, &mn->waveform) != MA_SUCCESS) {
          std::fprintf(stderr, "[MiniaudioBackend] ma_waveform_init failed\n");
          continue;
        }
        // pDataSource stores &mn->waveform — mn must not move after this call.
        ma_data_source_node_config dscfg =
            ma_data_source_node_config_init(&mn->waveform);
        if (ma_data_source_node_init(&graph, &dscfg, nullptr, &mn->dsNode) !=
            MA_SUCCESS) {
          std::fprintf(stderr,
                       "[MiniaudioBackend] ma_data_source_node_init failed\n");
          ma_waveform_uninit(&mn->waveform);
          continue;
        }
        mn->node = &mn->dsNode;
        mn->initialized = true;
        nodes[handle] = std::move(mn);
        continue;
      }

      if (pn.kind == NodeKind::Buffer) {
        mn->kind = MaNodeKind::Buffer;
        mn->bufferSrc.samples = &mn->params.samples;  // mn is pinned
        mn->bufferSrc.srcRate = pn.params.sampleRate > 0.0f ? pn.params.sampleRate : 44100.0;
        mn->bufferSrc.outRate = sampleRate;
        mn->bufferSrc.state = pn.playState;
        mn->bufferSrc.rate = pn.rate;
        mn->bufferSrc.gain = pn.params.gain;
        ma_data_source_config dcfg = ma_data_source_config_init();
        dcfg.vtable = &g_bufferVtable;
        if (ma_data_source_init(&dcfg, &mn->bufferSrc.base) != MA_SUCCESS) {
          std::fprintf(stderr, "[MiniaudioBackend] buffer data source init failed\n");
          continue;
        }
        ma_data_source_node_config dscfg = ma_data_source_node_config_init(&mn->bufferSrc);
        if (ma_data_source_node_init(&graph, &dscfg, nullptr, &mn->dsNode) != MA_SUCCESS) {
          std::fprintf(stderr, "[MiniaudioBackend] buffer source node init failed\n");
          ma_data_source_uninit(&mn->bufferSrc.base);
          continue;
        }
        mn->node = &mn->dsNode;
        mn->initialized = true;
        nodes[handle] = std::move(mn);
        continue;
      }

      if (pn.kind == NodeKind::Biquad) {
        mn->kind = MaNodeKind::Biquad;
        double cutoff = static_cast<double>(pn.params.frequency);
        ma_uint32 order = 2;
        if (pn.params.filterType == FilterType::Lowpass) {
          ma_lpf_node_config lcfg =
              ma_lpf_node_config_init(1, sr, cutoff, order);
          if (ma_lpf_node_init(&graph, &lcfg, nullptr, &mn->lpfNode) !=
              MA_SUCCESS) {
            std::fprintf(stderr, "[MiniaudioBackend] ma_lpf_node_init failed\n");
            continue;
          }
          mn->filterNode = &mn->lpfNode;
        } else if (pn.params.filterType == FilterType::Highpass) {
          ma_hpf_node_config hcfg =
              ma_hpf_node_config_init(1, sr, cutoff, order);
          if (ma_hpf_node_init(&graph, &hcfg, nullptr, &mn->hpfNode) !=
              MA_SUCCESS) {
            std::fprintf(stderr, "[MiniaudioBackend] ma_hpf_node_init failed\n");
            continue;
          }
          mn->filterNode = &mn->hpfNode;
        } else {
          ma_bpf_node_config bcfg =
              ma_bpf_node_config_init(1, sr, cutoff, order);
          if (ma_bpf_node_init(&graph, &bcfg, nullptr, &mn->bpfNode) !=
              MA_SUCCESS) {
            std::fprintf(stderr, "[MiniaudioBackend] ma_bpf_node_init failed\n");
            continue;
          }
          mn->filterNode = &mn->bpfNode;
        }
        // Bypass splitter: bus 0 feeds the filter (wet), bus 1 is the dry path.
        ma_splitter_node_config scfg = ma_splitter_node_config_init(1);
        if (ma_splitter_node_init(&graph, &scfg, nullptr, &mn->splitterNode) !=
            MA_SUCCESS) {
          std::fprintf(stderr, "[MiniaudioBackend] Biquad splitter init failed\n");
          continue;
        }
        mn->node = &mn->splitterNode;
        ma_node_attach_output_bus(mn->node, 0, mn->filterNode, 0);
        applyBiquadMix(*mn);
        mn->initialized = true;
        nodes[handle] = std::move(mn);
        continue;
      }

      if (pn.kind == NodeKind::Gain) {
        mn->kind = MaNodeKind::Gain;
        ma_splitter_node_config scfg = ma_splitter_node_config_init(1);
        if (ma_splitter_node_init(&graph, &scfg, nullptr, &mn->splitterNode) !=
            MA_SUCCESS) {
          std::fprintf(stderr,
                       "[MiniaudioBackend] ma_splitter_node_init failed\n");
          continue;
        }
        mn->node = &mn->splitterNode;
        mn->initialized = true;
        ma_node_set_output_bus_volume(mn->node, 0, pn.params.gain);
        nodes[handle] = std::move(mn);
        continue;
      }

      if (pn.kind == NodeKind::Panner) {
        // The Panner node is a unity splitter in the mono graph (so it passes
        // audio through unchanged for render() consumers).  Its spatial
        // processing is applied by renderStereo via ma_spatializer.
        mn->kind = MaNodeKind::Panner;
        ma_splitter_node_config scfg = ma_splitter_node_config_init(1);
        if (ma_splitter_node_init(&graph, &scfg, nullptr, &mn->splitterNode) !=
            MA_SUCCESS) {
          std::fprintf(stderr,
                       "[MiniaudioBackend] Panner splitter init failed\n");
          continue;
        }
        mn->node = &mn->splitterNode;
        ma_node_set_output_bus_volume(mn->node, 0, 1.0f);  // unity gain

        // ── Init the spatializer (mono → stereo) ───────────────────────────
        ma_spatializer_config spatCfg =
            ma_spatializer_config_init(/*channelsIn=*/1, /*channelsOut=*/2);
        // Ellipsoid (§16.4.17): the spatializer only pans; renderStereo applies
        // the ellipsoid gain computed from the same geometry.
        spatCfg.attenuationModel = pn.params.distanceModel == DistanceModel::Ellipsoid
                                       ? ma_attenuation_model_none
                                       : toMaAttenuation(pn.params.distanceModel);
        // Defensive: guard referenceDistance <= 0 to avoid NaN in the
        // inverse/exponential formulae (minDistance must be > 0).
        float refDist = pn.params.referenceDistance;
        if (refDist <= 0.0f) refDist = 1e-6f;
        spatCfg.minDistance = refDist;
        spatCfg.maxDistance = pn.params.maxDistance;
        spatCfg.rolloff     = pn.params.rolloffFactor;
        spatCfg.dopplerFactor = 0.0f;  // no doppler

        if (ma_spatializer_init(&spatCfg, nullptr, &mn->spatializer) !=
            MA_SUCCESS) {
          std::fprintf(stderr,
                       "[MiniaudioBackend] ma_spatializer_init failed\n");
          ma_splitter_node_uninit(&mn->splitterNode, nullptr);
          continue;
        }
        mn->spatializerInited = true;

        // Set initial source position.
        ma_spatializer_set_position(&mn->spatializer,
                                    pn.params.sourcePosition[0],
                                    pn.params.sourcePosition[1],
                                    pn.params.sourcePosition[2]);

        // ── Init the listener (at listenerPosition, facing listenerForward) ─
        ma_spatializer_listener_config listCfg =
            ma_spatializer_listener_config_init(/*channelsOut=*/2);
        if (ma_spatializer_listener_init(&listCfg, nullptr,
                                         &mn->spatializerListener) !=
            MA_SUCCESS) {
          std::fprintf(stderr,
                       "[MiniaudioBackend] ma_spatializer_listener_init failed\n");
          ma_spatializer_uninit(&mn->spatializer, nullptr);
          mn->spatializerInited = false;
          ma_splitter_node_uninit(&mn->splitterNode, nullptr);
          continue;
        }
        mn->listenerInited = true;

        ma_spatializer_listener_set_position(&mn->spatializerListener,
                                             pn.params.listenerPosition[0],
                                             pn.params.listenerPosition[1],
                                             pn.params.listenerPosition[2]);
        ma_spatializer_listener_set_direction(&mn->spatializerListener,
                                              pn.params.listenerForward[0],
                                              pn.params.listenerForward[1],
                                              pn.params.listenerForward[2]);
        ma_spatializer_listener_set_world_up(&mn->spatializerListener,
                                             pn.params.listenerUp[0],
                                             pn.params.listenerUp[1],
                                             pn.params.listenerUp[2]);

        mn->initialized = true;
        nodes[handle] = std::move(mn);
        continue;
      }
    }
    pending.clear();

    // Replay deferred connects.
    for (auto &dc : deferredConnects) {
      auto dit = nodes.find(dc.dst);
      auto sit = nodes.find(dc.src);
      if (dit == nodes.end() || sit == nodes.end()) continue;
      if (!dit->second || !sit->second) continue;
      ma_node* dstNode = dit->second->node;
      if (!dstNode) continue;
      attachOutput(*sit->second, dstNode);
    }
    deferredConnects.clear();
    return true;
  }

  // ── findFirstPanner: return the first initialized Panner node, or nullptr ──
  MaNode* findFirstPanner() {
    for (auto &[h, pn] : nodes) {
      if (pn && pn->initialized && pn->kind == MaNodeKind::Panner)
        return pn.get();
    }
    return nullptr;
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// MiniaudioBackend methods
// ─────────────────────────────────────────────────────────────────────────────

MiniaudioBackend::MiniaudioBackend() : impl_(std::make_unique<Impl>()) {}
MiniaudioBackend::~MiniaudioBackend() = default;

NodeHandle MiniaudioBackend::createNode(NodeKind kind,
                                        const NodeParams &params) {
  NodeHandle h = impl_->nextHandle++;
  impl_->pending[h] = {kind, params};
  return h;
}

void MiniaudioBackend::connect(NodeHandle dst, NodeHandle src) {
  if (!impl_->graphInited) {
    impl_->deferredConnects.push_back({dst, src});
    return;
  }
  auto dit = impl_->nodes.find(dst);
  auto sit = impl_->nodes.find(src);
  if (dit == impl_->nodes.end() || sit == impl_->nodes.end()) return;
  if (!dit->second || !sit->second) return;
  ma_node* dstNode = dit->second->node;
  if (!dstNode) return;
  impl_->attachOutput(*sit->second, dstNode);
}

void MiniaudioBackend::setParam(NodeHandle node, Param param, float value) {
  if (!impl_->graphInited) {
    auto it = impl_->pending.find(node);
    if (it != impl_->pending.end()) {
      switch (param) {
      case Param::Frequency:  it->second.params.frequency         = value; break;
      case Param::Detune:     it->second.params.detune            = value; break;
      case Param::Q:          it->second.params.q                 = value; break;
      case Param::Gain:       it->second.params.gain              = value; break;
      case Param::Enabled:    it->second.params.enabled           = value != 0.0f; break;
      case Param::PositionX:  it->second.params.sourcePosition[0] = value; break;
      case Param::PositionY:  it->second.params.sourcePosition[1] = value; break;
      case Param::PositionZ:  it->second.params.sourcePosition[2] = value; break;
      case Param::ListenerPositionX: it->second.params.listenerPosition[0] = value; break;
      case Param::ListenerPositionY: it->second.params.listenerPosition[1] = value; break;
      case Param::ListenerPositionZ: it->second.params.listenerPosition[2] = value; break;
      case Param::ListenerForwardX:  it->second.params.listenerForward[0]  = value; break;
      case Param::ListenerForwardY:  it->second.params.listenerForward[1]  = value; break;
      case Param::ListenerForwardZ:  it->second.params.listenerForward[2]  = value; break;
      case Param::ListenerUpX:       it->second.params.listenerUp[0]       = value; break;
      case Param::ListenerUpY:       it->second.params.listenerUp[1]       = value; break;
      case Param::ListenerUpZ:       it->second.params.listenerUp[2]       = value; break;
      case Param::DirectionX:        it->second.params.direction[0]        = value; break;
      case Param::DirectionY:        it->second.params.direction[1]        = value; break;
      case Param::DirectionZ:        it->second.params.direction[2]        = value; break;
      case Param::Intensity:         it->second.params.intensity           = value; break;
      case Param::PlaybackState:     it->second.playState = static_cast<int>(value); break;
      case Param::PlaybackRate:      it->second.rate = value; break;
      case Param::DelayTime:         it->second.params.delayTime = value; break;
      case Param::MaxDelayTime:      it->second.params.maxDelayTime = value; break;
      }
    }
    return;
  }

  auto it = impl_->nodes.find(node);
  if (it == impl_->nodes.end() || !it->second || !it->second->initialized)
    return;
  MaNode &mn = *it->second;

  switch (mn.kind) {
  case MaNodeKind::Oscillator:
    if (param == Param::Frequency) {
      double f = static_cast<double>(value);
      if (mn.params.detune != 0.0f)
        f *= std::pow(2.0, mn.params.detune / 1200.0);
      ma_waveform_set_frequency(&mn.waveform, f);
      mn.params.frequency = value;
    } else if (param == Param::Detune) {
      mn.params.detune = value;
      double f = static_cast<double>(mn.params.frequency);
      if (value != 0.0f) f *= std::pow(2.0, value / 1200.0);
      ma_waveform_set_frequency(&mn.waveform, f);
    } else if (param == Param::Gain) {
      ma_waveform_set_amplitude(&mn.waveform,
          mn.params.enabled && mn.timeActive ? static_cast<double>(value) : 0.0);
      mn.params.gain = value;
    } else if (param == Param::Enabled) {
      mn.params.enabled = value != 0.0f;
      ma_waveform_set_amplitude(&mn.waveform,
          mn.params.enabled && mn.timeActive ? static_cast<double>(mn.params.gain) : 0.0);
    } else if (param == Param::PlaybackState) {
      mn.timeActive = static_cast<int>(value) == 1;
      ma_waveform_set_amplitude(&mn.waveform,
          mn.timeActive && mn.params.enabled ? mn.params.gain : 0.0);
    }
    break;

  case MaNodeKind::Biquad: {
    if (param == Param::Enabled) {
      mn.params.enabled = value != 0.0f;
      applyBiquadMix(mn);
    } else if (param == Param::PlaybackState) {
      mn.timeActive = static_cast<int>(value) == 1;
      applyBiquadMix(mn);
    } else if (param == Param::Frequency) {
      mn.params.frequency = value;
      double cutoff = static_cast<double>(value);
      auto sr = static_cast<ma_uint32>(impl_->initSampleRate);
      ma_uint32 order = 2;
      if (mn.params.filterType == FilterType::Lowpass) {
        ma_lpf_config lcfg = ma_lpf_config_init(ma_format_f32, 1, sr, cutoff, order);
        ma_lpf_node_reinit(&lcfg, &mn.lpfNode);
      } else if (mn.params.filterType == FilterType::Highpass) {
        ma_hpf_config hcfg = ma_hpf_config_init(ma_format_f32, 1, sr, cutoff, order);
        ma_hpf_node_reinit(&hcfg, &mn.hpfNode);
      } else {
        ma_bpf_config bcfg = ma_bpf_config_init(ma_format_f32, 1, sr, cutoff, order);
        ma_bpf_node_reinit(&bcfg, &mn.bpfNode);
      }
    }
    break;
  }

  case MaNodeKind::Gain:
    if (param == Param::Gain) {
      ma_node_set_output_bus_volume(mn.node, 0, mn.timeActive ? value : 0.0f);
      mn.params.gain = value;
    } else if (param == Param::Enabled) {
      mn.params.enabled = value != 0.0f;
      ma_node_set_output_bus_volume(mn.node, 0, mn.timeActive
          ? (mn.params.enabled ? mn.params.gain : 1.0f) : 0.0f);
    } else if (param == Param::PlaybackState) {
      mn.timeActive = static_cast<int>(value) == 1;
      ma_node_set_output_bus_volume(mn.node, 0,
          mn.timeActive ? (mn.params.enabled ? mn.params.gain : 1.0f) : 0.0f);
    }
    break;

  case MaNodeKind::Destination:
    if (param == Param::Gain) {
      ma_node_set_output_bus_volume(mn.node, 0, value);
      mn.params.gain = value;
    }
    break;

  case MaNodeKind::Panner:
    switch (param) {
    case Param::ListenerPositionX: case Param::ListenerPositionY: case Param::ListenerPositionZ: {
      const int i = param == Param::ListenerPositionX ? 0 : param == Param::ListenerPositionY ? 1 : 2;
      mn.params.listenerPosition[i] = value;
      ma_spatializer_listener_set_position(&mn.spatializerListener, mn.params.listenerPosition[0],
                                           mn.params.listenerPosition[1], mn.params.listenerPosition[2]);
      return;
    }
    case Param::ListenerForwardX: case Param::ListenerForwardY: case Param::ListenerForwardZ: {
      const int i = param == Param::ListenerForwardX ? 0 : param == Param::ListenerForwardY ? 1 : 2;
      mn.params.listenerForward[i] = value;
      ma_spatializer_listener_set_direction(&mn.spatializerListener, mn.params.listenerForward[0],
                                            mn.params.listenerForward[1], mn.params.listenerForward[2]);
      return;
    }
    case Param::ListenerUpX: case Param::ListenerUpY: case Param::ListenerUpZ: {
      const int i = param == Param::ListenerUpX ? 0 : param == Param::ListenerUpY ? 1 : 2;
      mn.params.listenerUp[i] = value;
      ma_spatializer_listener_set_world_up(&mn.spatializerListener, mn.params.listenerUp[0],
                                           mn.params.listenerUp[1], mn.params.listenerUp[2]);
      return;
    }
    case Param::DirectionX: mn.params.direction[0] = value; return;
    case Param::DirectionY: mn.params.direction[1] = value; return;
    case Param::DirectionZ: mn.params.direction[2] = value; return;
    case Param::Intensity:  mn.params.intensity = value; return;
    default: break;
    }
    // Update source position on the spatializer.
    if (param == Param::PositionX) {
      mn.params.sourcePosition[0] = value;
      ma_spatializer_set_position(&mn.spatializer,
                                  mn.params.sourcePosition[0],
                                  mn.params.sourcePosition[1],
                                  mn.params.sourcePosition[2]);
    } else if (param == Param::PositionY) {
      mn.params.sourcePosition[1] = value;
      ma_spatializer_set_position(&mn.spatializer,
                                  mn.params.sourcePosition[0],
                                  mn.params.sourcePosition[1],
                                  mn.params.sourcePosition[2]);
    } else if (param == Param::PositionZ) {
      mn.params.sourcePosition[2] = value;
      ma_spatializer_set_position(&mn.spatializer,
                                  mn.params.sourcePosition[0],
                                  mn.params.sourcePosition[1],
                                  mn.params.sourcePosition[2]);
    }
    break;

  case MaNodeKind::Buffer:
    if (param == Param::PlaybackState) {
      const int st = static_cast<int>(value);
      if (st == 0) mn.bufferSrc.cursor = 0.0;  // stop rewinds; pause holds
      mn.bufferSrc.state = st;
    } else if (param == Param::PlaybackRate) {
      mn.bufferSrc.rate = value;
    } else if (param == Param::Gain) {
      mn.bufferSrc.gain = value;
    }
    break;
  }
}

void MiniaudioBackend::render(NodeHandle /*destination*/, int frames,
                              float sampleRate, std::vector<float> &out) {
  if (!impl_->flush(sampleRate)) {
    out.assign(static_cast<std::size_t>(frames), 0.0f);
    return;
  }
  out.resize(static_cast<std::size_t>(frames));
  ma_uint64 framesRead = 0;
  ma_node_graph_read_pcm_frames(&impl_->graph, out.data(),
                                static_cast<ma_uint64>(frames), &framesRead);
  if (static_cast<int>(framesRead) < frames) {
    for (int i = static_cast<int>(framesRead); i < frames; ++i)
      out[static_cast<std::size_t>(i)] = 0.0f;
  }
}

void MiniaudioBackend::renderStereo(NodeHandle destination, int frames,
                                     float sampleRate,
                                     std::vector<float> &outLR) {
  if (!impl_->flush(sampleRate)) {
    outLR.assign(static_cast<std::size_t>(frames) * 2, 0.0f);
    return;
  }

  // Find the first Panner node in the graph.
  MaNode* panner = impl_->findFirstPanner();

  if (panner == nullptr || !panner->spatializerInited || !panner->listenerInited) {
    // No Panner: render mono and duplicate into L+R.
    std::vector<float> mono;
    render(destination, frames, sampleRate, mono);
    outLR.resize(static_cast<std::size_t>(frames) * 2);
    for (int i = 0; i < frames; ++i) {
      float s = mono[static_cast<std::size_t>(i)];
      outLR[static_cast<std::size_t>(i) * 2]     = s;  // L
      outLR[static_cast<std::size_t>(i) * 2 + 1] = s;  // R
    }
    return;
  }

  // Panner present: render mono through the graph (the Panner is a unity
  // splitter in the node graph, so the mono result IS the pre-spatializer
  // signal).  Then apply ma_spatializer to produce interleaved stereo.
  std::vector<float> mono;
  render(destination, frames, sampleRate, mono);

  outLR.resize(static_cast<std::size_t>(frames) * 2);
  if (panner->params.distanceModel == DistanceModel::Ellipsoid) {
    // §16.4.17: the ellipsoid gain from the geometry; pan only when spatialized
    // (centred equal-power otherwise, as the built-in backend does).
    const float g = static_cast<float>(ellipsoidGain(panner->params));
    if (!panner->params.spatialize) {
      const float c = g * 0.70710678f;
      for (int i = 0; i < frames; ++i) {
        const float s = mono[static_cast<std::size_t>(i)] * c;
        outLR[static_cast<std::size_t>(i) * 2] = s;
        outLR[static_cast<std::size_t>(i) * 2 + 1] = s;
      }
      return;
    }
    ma_spatializer_process_pcm_frames(&panner->spatializer, &panner->spatializerListener,
                                      outLR.data(), mono.data(), static_cast<ma_uint64>(frames));
    for (float &s : outLR) s *= g;
    return;
  }
  ma_result r = ma_spatializer_process_pcm_frames(
      &panner->spatializer, &panner->spatializerListener,
      outLR.data(), mono.data(),
      static_cast<ma_uint64>(frames));
  if (r != MA_SUCCESS) {
    std::fprintf(stderr,
                 "[MiniaudioBackend] ma_spatializer_process_pcm_frames "
                 "failed (%d); falling back to mono duplicate\n", r);
    for (int i = 0; i < frames; ++i) {
      float s = mono[static_cast<std::size_t>(i)];
      outLR[static_cast<std::size_t>(i) * 2]     = s;
      outLR[static_cast<std::size_t>(i) * 2 + 1] = s;
    }
  }
}

} // namespace x3d::runtime::miniaudio
