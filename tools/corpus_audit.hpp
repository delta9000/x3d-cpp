// tools/corpus_audit.hpp
// ─────────────────────────────────────────────────────────────────────────────
// DIFFERENTIAL AUDIT engine (Phase 0). Reusable, header-only core shared by the
// `x3d_corpus_audit` CLI binary and the `x3d_corpus_audit_selftest` unit test.
//
// It produces an ORACLE-FREE conformance signal over real X3D scenes via two
// families of check, run per file that PARSES OK:
//
//   A. INVARIANTS — bounds/transform finiteness, tick determinism on a static
//      scene, and re-extract stability. These hold for ANY correct extraction
//      regardless of the scene's authored intent (no golden needed).
//
//   B. ROUND-TRIP FIDELITY — the strong oracle-free signal. Extract a SEMANTIC,
//      order-independent fingerprint F0 from the parsed doc; for each of the
//      three writable encodings {XML, JSON, ClassicVRML}, serialize -> reparse
//      -> re-extract F1, and compare F0 vs F1 with float epsilons. A semantic
//      drift across a round-trip is a fidelity DEFECT. Formatting, default-
//      dropping and float-precision differences are EXPECTED and never flagged
//      (the fingerprint is semantic, NOT a byte diff).
//
// Output is a machine-clusterable list of Finding records (one per failure),
// each carrying a stable `signature` dedup key so a downstream phase can
// cluster identical defects. The engine itself does NO file IO beyond what the
// SDK parse path performs; the CLI owns directory walking + JSONL writing.
//
// Everything goes through the x3d::sdk façade — the exact surface an embedder
// uses — so a regression in the public API surfaces here too.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef X3D_TOOLS_CORPUS_AUDIT_HPP
#define X3D_TOOLS_CORPUS_AUDIT_HPP

#include "x3d/sdk.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::audit {

namespace sdk = x3d::sdk;
using namespace x3d::core;    // SFVec3f, X3DFieldType, SFNode, ...
using x3d::nodes::X3DNode;   // complete node base; shadows ::X3DNode fwd decl in ScriptEngine.hpp

// ───────────────────────────────────────────────────────────────────────────
// Finding — one audit failure. `signature` is a STABLE dedup key (check +
// encoding + dominant nodeType + a normalized detail bucket) so a clustering
// phase can group identical defects without re-deriving structure from `detail`.
// ───────────────────────────────────────────────────────────────────────────
struct Finding {
  std::string file;      // path relative to the swept root (or a synthetic id).
  std::string check;     // "bounds-finite" / "roundtrip" / "reparse-throw" / ...
  std::string encoding;  // "xml"/"json"/"classicvrml" for round-trip; else "".
  std::string nodeType;  // dominant node type implicated, when known; else "".
  std::string detail;    // human-readable specifics (NOT used for clustering).
  std::string signature; // stable cluster key.
};

// ───────────────────────────────────────────────────────────────────────────
// Semantic fingerprint. ORDER-INDEPENDENT and float-tolerant by construction:
//   * itemCount                       — number of emitted RenderItems.
//   * items (a MULTISET, sorted)      — per-item canonical tuple string:
//       topology | positions.size | indices.size |
//       baseColor RGB (rounded) | worldTransform translation (rounded) |
//       worldTransform scale magnitudes (rounded) | det (rounded).
//     Sorting the per-item strings makes the comparison independent of the
//     emission/serialization order (a writer may reorder DEF/USE or children).
//   * structural counts on the MODEL  — nodeCount, defCount, useCount,
//     routeCount, protoCount. These catch DEF/USE/ROUTE/PROTO loss that the
//     render-item multiset alone could miss (e.g. an inert sensor subtree).
//
// Rounding (not exact compare) is the whole point: formatting + default-
// dropping + float-precision drift across encodings is EXPECTED and must NOT be
// flagged. Two fingerprints are equal iff every rounded quantity matches.
// ───────────────────────────────────────────────────────────────────────────
struct Fingerprint {
  std::size_t itemCount = 0;
  std::vector<std::string> items;   // sorted multiset of per-item canon tuples.
  long long nodeCount = 0;
  long long defCount = 0;
  long long useCount = 0;           // shared-ref placements beyond first sighting.
  long long routeCount = 0;
  long long protoCount = 0;         // PROTO + EXTERNPROTO + ProtoInstance decls.

  bool operator==(const Fingerprint &o) const {
    return itemCount == o.itemCount && items == o.items &&
           nodeCount == o.nodeCount && defCount == o.defCount &&
           useCount == o.useCount && routeCount == o.routeCount &&
           protoCount == o.protoCount;
  }

  // Compact one-line dump for diagnostics / the structural round-trip seam.
  std::string dump() const {
    std::ostringstream os;
    os << "items=" << itemCount << " nodes=" << nodeCount << " def=" << defCount
       << " use=" << useCount << " routes=" << routeCount
       << " proto=" << protoCount;
    return os.str();
  }
};

// ── rounding helpers (epsilon == one bucket of the rounding grid) ────────────
inline double roundq(double v, double q) {
  if (!std::isfinite(v)) return v; // NaN/Inf compare structurally as-is.
  return std::round(v / q) * q;
}
inline std::string fmtq(double v, double q) {
  std::ostringstream os;
  os.setf(std::ios::fixed);
  os.precision(3);
  os << roundq(static_cast<double>(v), q);
  return os.str();
}

// ── finiteness predicates ────────────────────────────────────────────────────
inline bool finite3(const SFVec3f &v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
inline bool finiteAabb(const sdk::Aabb &b) {
  return finite3(b.min) && finite3(b.max);
}
inline bool finiteMat(const sdk::Mat4 &m) {
  for (float e : m.m)
    if (!std::isfinite(e)) return false;
  return true;
}
// 3x3 upper-left determinant of a column-major Mat4 (the scale/rotation block).
inline float det3(const sdk::Mat4 &m) {
  const auto &a = m.m;
  return a[0] * (a[5] * a[10] - a[9] * a[6]) -
         a[4] * (a[1] * a[10] - a[9] * a[2]) +
         a[8] * (a[1] * a[6] - a[5] * a[2]);
}

// ───────────────────────────────────────────────────────────────────────────
// Structural model counts — walk the scene graph + scene tables ONCE.
//   nodeCount : DISTINCT node instances reachable from root (shared nodes once).
//   useCount  : reachable placements MINUS distinct nodes = shared re-references.
//   defCount  : entries in the DEF symbol table.
//   routeCount: scene ROUTEs.
//   protoCount: PROTO + EXTERNPROTO declarations + recorded ProtoInstances.
// ───────────────────────────────────────────────────────────────────────────
struct StructCounts {
  long long nodes = 0, uses = 0;
};

inline void countWalk(const X3DNode *n, std::unordered_set<const X3DNode *> &seen,
                      StructCounts &sc) {
  if (!n) return;
  if (!seen.insert(n).second) {
    ++sc.uses; // a re-reachable node == a shared (USE) placement.
    return;
  }
  ++sc.nodes;
  for (const auto &f : n->fields()) {
    if (!f.get) continue;
    if (f.type == X3DFieldType::SFNode) {
      auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
      if (c) countWalk(c.get(), seen, sc);
    } else if (f.type == X3DFieldType::MFNode) {
      for (const auto &c :
           std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
        if (c) countWalk(c.get(), seen, sc);
    }
  }
}

inline void fillStructCounts(const sdk::Scene &scene, Fingerprint &fp) {
  StructCounts sc;
  std::unordered_set<const X3DNode *> seen;
  for (const auto &root : scene.rootNodes)
    if (root) countWalk(root.get(), seen, sc);
  fp.nodeCount = sc.nodes;
  fp.useCount = sc.uses;
  fp.defCount = static_cast<long long>(scene.defs.size());
  fp.routeCount = static_cast<long long>(scene.routes.size());
  fp.protoCount = static_cast<long long>(scene.protoDeclarations.size()) +
                  static_cast<long long>(scene.externProtoDeclarations.size()) +
                  static_cast<long long>(scene.protoInstances.size());
}

// ───────────────────────────────────────────────────────────────────────────
// makeFingerprint — the entry point. Builds a context, snapshots once, and
// derives the semantic fingerprint + structural counts. Self-contained so both
// the invariant pass and the round-trip pass can reuse it.
//
// posEps / colEps / trEps are the rounding grids (== effective epsilons). They
// are intentionally LOOSE relative to single-precision serialization round-trip
// error so that legitimate precision drift never trips a false positive.
// ───────────────────────────────────────────────────────────────────────────
inline Fingerprint makeFingerprint(sdk::X3DDocument &doc,
                                   double posEps = 1e-3,
                                   double colEps = 1.0 / 256.0,
                                   double trEps = 1e-3) {
  Fingerprint fp;

  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  sdk::MeshBuildOptions opts;
  sdk::SceneExtractor ex(ctx, doc.scene, opts);
  sdk::RenderDelta snap = ex.fullSnapshot();

  fp.itemCount = ex.itemCount();
  fp.items.reserve(fp.itemCount);
  for (sdk::RenderItemId id = 0; id < ex.itemCount(); ++id) {
    const sdk::RenderItem &it = ex.item(id);
    const sdk::Mat4 &w = it.worldTransform;
    // Translation column (col 3) of a column-major matrix.
    const double tx = w.m[12], ty = w.m[13], tz = w.m[14];
    // Per-axis scale magnitudes = column vector lengths (rotation-invariant).
    auto colLen = [&](int c) {
      return std::sqrt(double(w.m[c * 4 + 0]) * w.m[c * 4 + 0] +
                       double(w.m[c * 4 + 1]) * w.m[c * 4 + 1] +
                       double(w.m[c * 4 + 2]) * w.m[c * 4 + 2]);
    };
    const double sx = colLen(0), sy = colLen(1), sz = colLen(2);
    const sdk::MaterialDesc &mat = it.material;

    std::ostringstream os;
    os << static_cast<int>(it.mesh->topology) << '|' << it.mesh->positions.size()
       << '|' << it.mesh->indices.size() << '|' << fmtq(mat.toRGBA().r, colEps)
       << ',' << fmtq(mat.toRGBA().g, colEps) << ','
       << fmtq(mat.toRGBA().b, colEps) << '|' << fmtq(tx, trEps) << ','
       << fmtq(ty, trEps) << ',' << fmtq(tz, trEps) << '|' << fmtq(sx, posEps)
       << ',' << fmtq(sy, posEps) << ',' << fmtq(sz, posEps) << '|'
       << fmtq(det3(w), posEps);
    fp.items.push_back(os.str());
  }
  std::sort(fp.items.begin(), fp.items.end()); // order-independent multiset.

  fillStructCounts(doc.scene, fp);
  (void)snap;
  return fp;
}

// Human-readable first-mismatch reason between two fingerprints (for `detail`).
inline std::string explainMismatch(const Fingerprint &a, const Fingerprint &b) {
  std::ostringstream os;
  if (a.itemCount != b.itemCount)
    os << "itemCount " << a.itemCount << "->" << b.itemCount << "; ";
  if (a.nodeCount != b.nodeCount)
    os << "nodeCount " << a.nodeCount << "->" << b.nodeCount << "; ";
  if (a.defCount != b.defCount)
    os << "defCount " << a.defCount << "->" << b.defCount << "; ";
  if (a.useCount != b.useCount)
    os << "useCount " << a.useCount << "->" << b.useCount << "; ";
  if (a.routeCount != b.routeCount)
    os << "routeCount " << a.routeCount << "->" << b.routeCount << "; ";
  if (a.protoCount != b.protoCount)
    os << "protoCount " << a.protoCount << "->" << b.protoCount << "; ";
  if (a.items != b.items) {
    // Count multiset symmetric difference for a compact, stable description.
    std::map<std::string, int> m;
    for (const auto &s : a.items) ++m[s];
    for (const auto &s : b.items) --m[s];
    int onlyA = 0, onlyB = 0;
    for (const auto &kv : m) {
      if (kv.second > 0) onlyA += kv.second;
      if (kv.second < 0) onlyB += -kv.second;
    }
    os << "itemTuples differ (+" << onlyA << " only-in-orig / +" << onlyB
       << " only-in-roundtrip); ";
  }
  std::string s = os.str();
  if (s.empty()) s = "(no structural difference detected)";
  return s;
}

// ── signature bucketing ──────────────────────────────────────────────────────
// A normalized detail bucket: collapse numbers to '#' so distinct magnitudes of
// the same defect class share a signature. Keeps the cluster key stable while
// the human `detail` stays specific.
inline std::string detailBucket(const std::string &detail) {
  std::string out;
  bool inNum = false;
  for (char c : detail) {
    const bool d = (c >= '0' && c <= '9');
    if (d) {
      if (!inNum) out += '#';
      inNum = true;
    } else {
      inNum = false;
      out += c;
    }
  }
  // Trim to the leading clause so per-file specifics don't fragment the key.
  auto semi = out.find(';');
  if (semi != std::string::npos) out.resize(semi);
  return out;
}

inline std::string makeSignature(const std::string &check,
                                 const std::string &encoding,
                                 const std::string &nodeType,
                                 const std::string &detail) {
  std::ostringstream os;
  os << check << '|' << encoding << '|' << nodeType << '|'
     << detailBucket(detail);
  return os.str();
}

inline Finding makeFinding(const std::string &file, const std::string &check,
                           const std::string &encoding,
                           const std::string &nodeType,
                           const std::string &detail) {
  Finding f;
  f.file = file;
  f.check = check;
  f.encoding = encoding;
  f.nodeType = nodeType;
  f.detail = detail;
  f.signature = makeSignature(check, encoding, nodeType, detail);
  return f;
}

// ── time-driver detection (conservative tick-determinism guard) ──────────────
// Returns true if the scene contains ANY node that could legitimately drive a
// non-empty delta at t>0: a *Sensor, *Interpolator, Script, or TimeSensor.
// Deliberately broad (presence, not enabled-state) to avoid false positives on
// the tick-determinism invariant — we only flag spontaneous churn when we are
// confident NOTHING could have driven it.
inline bool isTimeDriverType(const std::string &t) {
  auto ends = [&](const char *suf) {
    const std::string s(suf);
    return t.size() >= s.size() &&
           t.compare(t.size() - s.size(), s.size(), s) == 0;
  };
  return ends("Sensor") || ends("Interpolator") || t == "Script" ||
         t == "TimeSensor";
}
inline bool hasTimeDriverWalk(const X3DNode *n,
                              std::unordered_set<const X3DNode *> &seen) {
  if (!n || !seen.insert(n).second) return false;
  if (isTimeDriverType(n->nodeTypeName())) return true;
  for (const auto &f : n->fields()) {
    if (!f.get) continue;
    if (f.type == X3DFieldType::SFNode) {
      auto c = std::any_cast<std::shared_ptr<X3DNode>>(f.get(*n));
      if (c && hasTimeDriverWalk(c.get(), seen)) return true;
    } else if (f.type == X3DFieldType::MFNode) {
      for (const auto &c :
           std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*n)))
        if (c && hasTimeDriverWalk(c.get(), seen)) return true;
    }
  }
  return false;
}
inline bool sceneHasTimeDriver(const sdk::Scene &scene) {
  std::unordered_set<const X3DNode *> seen;
  for (const auto &root : scene.rootNodes)
    if (root && hasTimeDriverWalk(root.get(), seen)) return true;
  return false;
}

// ───────────────────────────────────────────────────────────────────────────
// auditInvariants — run check-family A on ONE parsed document. Appends Findings.
// `file` is the label used in each Finding (relative path or synthetic id).
// ───────────────────────────────────────────────────────────────────────────
inline void auditInvariants(sdk::X3DDocument &doc, const std::string &file,
                            std::vector<Finding> &out) {
  sdk::X3DExecutionContext ctx;
  ctx.buildSceneGraph(doc.scene);
  ctx.buildFrom(doc.scene);
  sdk::MeshBuildOptions opts;
  sdk::SceneExtractor ex(ctx, doc.scene, opts);
  sdk::RenderDelta snap = ex.fullSnapshot();
  const std::size_t n0 = ex.itemCount();

  // --- bounds finite (scene + per-item) -------------------------------------
  const sdk::Aabb sb = ex.sceneWorldBounds();
  if (!finiteAabb(sb))
    out.push_back(makeFinding(file, "bounds-finite", "", "",
                              "sceneWorldBounds has non-finite component"));
  if (n0 > 0 && sb.empty)
    out.push_back(makeFinding(file, "bounds-nonempty", "", "",
                              "sceneWorldBounds empty with items present"));

  for (sdk::RenderItemId id = 0; id < n0; ++id) {
    const sdk::RenderItem &it = ex.item(id);
    // mesh-derived local bound.
    sdk::Aabb local;
    bool meshFinite = true;
    for (const SFVec3f &p : it.mesh->positions) {
      if (!finite3(p)) { meshFinite = false; break; }
      local.expand(p);
    }
    if (!meshFinite) {
      out.push_back(makeFinding(file, "bounds-finite", "",
                                it.geometry.node ? it.geometry.node->nodeTypeName()
                                                 : "",
                                "RenderItem mesh has non-finite position"));
      continue;
    }
    if (!local.empty && !finiteAabb(local.transformed(it.worldTransform)))
      out.push_back(makeFinding(file, "bounds-finite", "",
                                it.geometry.node ? it.geometry.node->nodeTypeName()
                                                 : "",
                                "RenderItem world bound non-finite"));

    // --- transform finite + finite determinant ------------------------------
    if (!finiteMat(it.worldTransform))
      out.push_back(makeFinding(file, "transform-finite", "",
                                it.geometry.node ? it.geometry.node->nodeTypeName()
                                                 : "",
                                "worldTransform has non-finite entry"));
    else if (!std::isfinite(det3(it.worldTransform)))
      out.push_back(makeFinding(file, "transform-finite", "",
                                it.geometry.node ? it.geometry.node->nodeTypeName()
                                                 : "",
                                "worldTransform determinant non-finite"));
  }

  // --- tick determinism on a static scene -----------------------------------
  // fullSnapshot() seeded lastDeltaNow_ = ctx.now() (== 0). Advance the clock by
  // a small dt so delta()'s one-delta-per-tick contract is satisfied, then a
  // static scene (no time-driver) MUST produce an empty item-level delta.
  ctx.tick(1.0 / 60.0);
  sdk::RenderDelta d = ex.delta();
  const bool itemDelta = !d.added.empty() || !d.removed.empty() ||
                         !d.updatedTransform.empty() ||
                         !d.updatedGeometry.empty() ||
                         !d.updatedMaterial.empty();
  if (itemDelta && !sceneHasTimeDriver(doc.scene)) {
    std::ostringstream os;
    os << "static scene churned at t>0 (added=" << d.added.size()
       << " removed=" << d.removed.size() << " xf=" << d.updatedTransform.size()
       << " geo=" << d.updatedGeometry.size()
       << " mat=" << d.updatedMaterial.size() << ")";
    out.push_back(makeFinding(file, "tick-determinism", "", "", os.str()));
  }

  // --- re-extract stability (item count stable across a 2nd full snapshot) ---
  sdk::RenderDelta snap2 = ex.fullSnapshot();
  if (ex.itemCount() != n0) {
    std::ostringstream os;
    os << "itemCount changed on re-extract " << n0 << "->" << ex.itemCount();
    out.push_back(makeFinding(file, "reextract-stable", "", "", os.str()));
  }
  (void)snap;
  (void)snap2;
}

// ── round-trip encoding descriptor ───────────────────────────────────────────
struct EncDef {
  const char *key;          // signature/JSONL encoding tag.
  sdk::Encoding hint;       // reparse hint.
};

// classifyThrow — bucket a reparse exception so a clustering phase can separate
// EXPECTED unresolved-external throws (Inline / EXTERNPROTO url) from genuine
// "we emitted unparseable output" defects.
inline bool isExpectedExternalThrow(const std::string &what) {
  auto has = [&](const char *s) { return what.find(s) != std::string::npos; };
  return has("EXTERNPROTO") || has("ExternProto") || has("externproto") ||
         has("Inline") || has("inline") || has("resolve") || has("url") ||
         has("URL") || has("cannot open") || has("not found") || has("http");
}

// ───────────────────────────────────────────────────────────────────────────
// auditRoundTrip — run check-family B on ONE parsed document. For each of the
// three writable encodings: write -> reparse -> re-fingerprint -> compare.
//   * a reparse throw classified EXPECTED-external is recorded as informational
//     "reparse-throw-external" (Phase 3 separates it); an UNEXPECTED throw is a
//     real "reparse-throw" defect (we emitted unparseable output).
//   * a semantic fingerprint mismatch is a "roundtrip" fidelity defect.
// ───────────────────────────────────────────────────────────────────────────
inline void auditRoundTrip(sdk::X3DDocument &doc, const std::string &file,
                           const Fingerprint &f0, std::vector<Finding> &out) {
  const EncDef encs[3] = {
      {"xml", sdk::Encoding::XML},
      {"json", sdk::Encoding::JSON},
      {"classicvrml", sdk::Encoding::ClassicVRML},
  };

  for (const EncDef &e : encs) {
    std::string text;
    try {
      if (e.hint == sdk::Encoding::XML)
        text = sdk::XmlWriter().writeDocument(doc);
      else if (e.hint == sdk::Encoding::JSON)
        text = sdk::JsonWriter().writeDocument(doc);
      else
        text = sdk::VrmlWriter().writeDocument(doc);
    } catch (const std::exception &ex) {
      out.push_back(makeFinding(file, "write-throw", e.key, "",
                                std::string("writeDocument threw: ") + ex.what()));
      continue;
    }

    sdk::X3DDocument doc1;
    try {
      doc1 = sdk::parseDocument(text, e.hint);
    } catch (const std::exception &ex) {
      const std::string what = ex.what();
      const bool expected = isExpectedExternalThrow(what);
      out.push_back(makeFinding(
          file, expected ? "reparse-throw-external" : "reparse-throw", e.key, "",
          std::string("parseDocument threw: ") + what));
      continue;
    }

    Fingerprint f1;
    try {
      f1 = makeFingerprint(doc1);
    } catch (const std::exception &ex) {
      out.push_back(makeFinding(file, "reextract-throw", e.key, "",
                                std::string("re-extract threw: ") + ex.what()));
      continue;
    }

    if (!(f0 == f1)) {
      out.push_back(makeFinding(file, "roundtrip", e.key, "",
                                explainMismatch(f0, f1)));
    }
  }
}

// ── per-file driver ──────────────────────────────────────────────────────────
struct AuditOptions {
  bool invariantsOnly = false; // skip round-trip family B.
  bool doRoundTrip = true;     // gated by --roundtrip-limit at the call site.
};

// auditDocument — run the requested check families on an already-parsed doc.
inline void auditDocument(sdk::X3DDocument &doc, const std::string &file,
                          const AuditOptions &ao, std::vector<Finding> &out) {
  try {
    auditInvariants(doc, file, out);
  } catch (const std::exception &ex) {
    out.push_back(makeFinding(file, "invariant-throw", "", "",
                              std::string("invariant pass threw: ") + ex.what()));
  }
  if (ao.invariantsOnly || !ao.doRoundTrip) return;

  Fingerprint f0;
  try {
    f0 = makeFingerprint(doc);
  } catch (const std::exception &ex) {
    out.push_back(makeFinding(file, "fingerprint-throw", "", "",
                              std::string("base fingerprint threw: ") + ex.what()));
    return;
  }
  auditRoundTrip(doc, file, f0, out);
}

} // namespace x3d::audit

#endif // X3D_TOOLS_CORPUS_AUDIT_HPP
