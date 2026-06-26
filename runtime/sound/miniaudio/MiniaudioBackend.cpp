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

enum class MaNodeKind { Oscillator, Biquad, Gain, Panner, Destination };

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

  // Panner: splitterNode (above) is used as the mono pass-through in the graph;
  // spatializer + listener are used by renderStereo.
  ma_spatializer         spatializer{};
  ma_spatializer_listener spatializerListener{};
  bool                   spatializerInited   = false;
  bool                   listenerInited      = false;

  // Pointer to the ma_node* interface (set during flush).
  ma_node* node = nullptr;

  bool initialized = false;

  // Prevent accidental copy/move (the structs are non-trivially copyable
  // anyway, but be explicit).
  MaNode(const MaNode&) = delete;
  MaNode& operator=(const MaNode&) = delete;
  MaNode(MaNode&&) = delete;
  MaNode& operator=(MaNode&&) = delete;
  MaNode() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// Impl
// ─────────────────────────────────────────────────────────────────────────────

struct MiniaudioBackend::Impl {
  struct PendingNode {
    NodeKind  kind;
    NodeParams params;
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
          mn->node = &mn->lpfNode;
        } else if (pn.params.filterType == FilterType::Highpass) {
          ma_hpf_node_config hcfg =
              ma_hpf_node_config_init(1, sr, cutoff, order);
          if (ma_hpf_node_init(&graph, &hcfg, nullptr, &mn->hpfNode) !=
              MA_SUCCESS) {
            std::fprintf(stderr, "[MiniaudioBackend] ma_hpf_node_init failed\n");
            continue;
          }
          mn->node = &mn->hpfNode;
        } else {
          ma_bpf_node_config bcfg =
              ma_bpf_node_config_init(1, sr, cutoff, order);
          if (ma_bpf_node_init(&graph, &bcfg, nullptr, &mn->bpfNode) !=
              MA_SUCCESS) {
            std::fprintf(stderr, "[MiniaudioBackend] ma_bpf_node_init failed\n");
            continue;
          }
          mn->node = &mn->bpfNode;
        }
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
        spatCfg.attenuationModel = toMaAttenuation(pn.params.distanceModel);
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
      ma_node* srcNode = sit->second->node;
      if (!dstNode || !srcNode) continue;
      ma_result r = ma_node_attach_output_bus(srcNode, 0, dstNode, 0);
      if (r != MA_SUCCESS)
        std::fprintf(stderr,
                     "[MiniaudioBackend] ma_node_attach_output_bus failed (%d)\n",
                     r);
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
  ma_node* srcNode = sit->second->node;
  if (!dstNode || !srcNode) return;
  ma_node_attach_output_bus(srcNode, 0, dstNode, 0);
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
      case Param::PositionX:  it->second.params.sourcePosition[0] = value; break;
      case Param::PositionY:  it->second.params.sourcePosition[1] = value; break;
      case Param::PositionZ:  it->second.params.sourcePosition[2] = value; break;
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
      ma_waveform_set_amplitude(&mn.waveform, static_cast<double>(value));
      mn.params.gain = value;
    }
    break;

  case MaNodeKind::Biquad: {
    if (param == Param::Frequency) {
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
      ma_node_set_output_bus_volume(mn.node, 0, value);
      mn.params.gain = value;
    }
    break;

  case MaNodeKind::Panner:
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

  case MaNodeKind::Destination:
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
