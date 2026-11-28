// X3DVersionFit.hpp — which X3D version a scene's authored content needs.
//
// The node bindings are generated from the newest UOM and hold every node and
// field of every earlier version (ADR-0077). Each node type reports the version
// that introduced it (X3DNode::introducedIn()) and each reflected field carries
// its own (FieldInfo::since). This pass compares the authored content with the
// version a document declares, so a 4.0 file that uses EnvironmentLight or
// IndexedFaceSet.tangent is reported instead of passing silently.
#ifndef X3D_RUNTIME_VERSION_FIT_HPP
#define X3D_RUNTIME_VERSION_FIT_HPP

#include "FieldRead.hpp"
#include "X3DDocument.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimMotion.hpp"
#include "x3d/nodes/TextureProjector.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <algorithm>
#include <charconv>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

using core::X3DVersion;

/// Parse an X3D version token ("4.0", "3.3"). nullopt if it isn't MAJOR.MINOR.
inline std::optional<X3DVersion> parseX3DVersion(const std::string &token) {
  const auto dot = token.find('.');
  if (dot == std::string::npos || dot == 0 || dot + 1 >= token.size())
    return std::nullopt;
  int major = 0, minor = 0;
  const char *b = token.data();
  const char *e = b + token.size();
  auto r1 = std::from_chars(b, b + dot, major);
  if (r1.ec != std::errc{} || r1.ptr != b + dot) return std::nullopt;
  auto r2 = std::from_chars(b + dot + 1, e, minor);
  if (r2.ec != std::errc{} || r2.ptr != e) return std::nullopt;
  return X3DVersion{major, minor};
}

/// One authored node type or field that needs a version newer than some floor.
struct VersionUse {
  std::string node;  // node type name
  std::string field; // empty for the node type itself
  X3DVersion since;

  std::string label() const { return field.empty() ? node : node + "." + field; }
  friend bool operator<(const VersionUse &a, const VersionUse &b) {
    return std::tie(a.node, a.field) < std::tie(b.node, b.field);
  }
};

namespace version_fit_detail {

/// The reflected field a ROUTE or IS endpoint names, with the set_ / _changed
/// aliases of an inputOutput field resolved. Null when the node has no such field.
inline const core::FieldInfo *endpointField(const nodes::X3DNode &node,
                                            const std::string &name) {
  const auto &fields = node.fields();
  for (const auto &f : fields)
    if (f.x3dName == name) return &f;
  std::string base;
  if (name.rfind("set_", 0) == 0)
    base = name.substr(4);
  else if (name.size() > 8 && name.compare(name.size() - 8, 8, "_changed") == 0)
    base = name.substr(0, name.size() - 8);
  if (base.empty()) return nullptr;
  for (const auto &f : fields)
    if (f.x3dName == base) return &f;
  return nullptr;
}

struct Collector {
  X3DVersion floor;
  std::set<VersionUse> uses;
  std::unordered_set<const nodes::X3DNode *> seen;

  /// A field use, listed only when it is newer than both the floor and the
  /// node type (the node's own entry covers fields as old as the type).
  void field(const nodes::X3DNode &node, const core::FieldInfo &f) {
    if (floor < f.since && node.introducedIn() < f.since)
      uses.insert({node.nodeTypeName(), f.x3dName, f.since});
  }

  void endpoint(const std::shared_ptr<nodes::X3DNode> &node,
                const std::string &name) {
    if (!node) return;
    if (const auto *f = endpointField(*node, name)) field(*node, *f);
  }

  void node(const std::shared_ptr<nodes::X3DNode> &n,
            const AuthoredScalarFields &authored) {
    if (!n || !seen.insert(n.get()).second) return;
    if (floor < n->introducedIn())
      uses.insert({n->nodeTypeName(), {}, n->introducedIn()});
    for (const auto &f : n->fields())
      if (!f.isNode() && authored.contains(n, f.x3dName)) field(*n, f);
    // Values whose allowed range grew in a later version.
    if (floor < X3DVersion{4, 1}) {
      int loa = -1;
      if (const auto *h = dynamic_cast<const nodes::HAnimHumanoid *>(n.get()))
        loa = h->getLoa();
      else if (const auto *m = dynamic_cast<const nodes::HAnimMotion *>(n.get()))
        loa = m->getLoa();
      if (loa > 4) uses.insert({n->nodeTypeName(), "loa " + std::to_string(loa),
                                X3DVersion{4, 1}});
    }
    forEachChildNode(*n, [&](const core::FieldInfo &f,
                             const std::shared_ptr<nodes::X3DNode> &child) {
      if (child) field(*n, f); // a node-valued field counts once it holds a node
      node(child, authored);
    });
  }
};

/// The node a DEF name labels among `roots` and their descendants (PROTO
/// bodies have their own DEF scope, with no table of their own).
inline std::shared_ptr<nodes::X3DNode>
findDef(const std::vector<std::shared_ptr<nodes::X3DNode>> &roots,
        const std::string &def) {
  std::shared_ptr<nodes::X3DNode> found;
  std::unordered_set<const nodes::X3DNode *> seen;
  auto visit = [&](auto &&self, const std::shared_ptr<nodes::X3DNode> &n) -> void {
    if (!n || found || !seen.insert(n.get()).second) return;
    if (n->getDEF() == def) { found = n; return; }
    forEachChildNode(*n, [&](const core::FieldInfo &,
                             const std::shared_ptr<nodes::X3DNode> &c) { self(self, c); });
  };
  for (const auto &r : roots) visit(visit, r);
  return found;
}

} // namespace version_fit_detail

/// Authored node types, fields and values newer than `floor`, one entry per
/// distinct use, sorted by name. A field is listed only when it is newer than
/// its node type too (IndexedFaceSet.tangent, not EnvironmentLight.intensity).
/// Walks the scene graph and every PROTO declaration body. A field counts when
/// a reader recorded it (Scene::authoredScalarFields, or the declaration's own
/// set for a PROTO body), when a node-valued field holds a node, or when a
/// ROUTE or IS connection names it; generated defaults never count.
inline std::vector<VersionUse> versionUsesAbove(const Scene &scene,
                                                X3DVersion floor) {
  version_fit_detail::Collector c{floor, {}, {}};
  for (const auto &root : scene.rootNodes) c.node(root, scene.authoredScalarFields);
  for (const auto &r : scene.routes) {
    auto from = r.from.lock(), to = r.to.lock();
    if (!from) from = scene.resolve(r.fromNode);
    if (!to) to = scene.resolve(r.toNode);
    c.endpoint(from, r.fromField);
    c.endpoint(to, r.toField);
  }
  for (const auto &decl : scene.protoDeclarations) {
    if (!decl) continue;
    for (const auto &n : decl->body.nodes) c.node(n, decl->authoredScalarFields);
    for (const auto &r : decl->body.routes) {
      c.endpoint(version_fit_detail::findDef(decl->body.nodes, r.fromNode), r.fromField);
      c.endpoint(version_fit_detail::findDef(decl->body.nodes, r.toNode), r.toField);
    }
    for (const auto &is : decl->body.isConnections) c.endpoint(is.node, is.nodeField);
  }
  return {c.uses.begin(), c.uses.end()};
}

/// Give fields whose default changed between versions the default of the
/// version a document declares, when the document leaves them out. Run by
/// the parse front door before PROTO expansion. Today one field qualifies:
/// TextureProjector.upVector was 0 0 1 before X3D 4.1 and is 0 1 0 since,
/// so a 4.0 projector whose direction is not along Z keeps its 4.0 frame.
/// An undeclared version reads as the 3.0 floor, like the rest of the read.
inline void applyVersionDefaults(X3DDocument &doc) {
  const auto declared = parseX3DVersion(doc.version);
  if (!declared || !(*declared < X3DVersion{4, 1})) return;
  std::unordered_set<const nodes::X3DNode *> seen;
  auto visit = [&](auto &&self, const std::shared_ptr<nodes::X3DNode> &n,
                   const AuthoredScalarFields &authored) -> void {
    if (!n || !seen.insert(n.get()).second) return;
    if (auto *p = dynamic_cast<nodes::TextureProjector *>(n.get()))
      if (!authored.contains(n, "upVector")) p->setUpVector(core::SFVec3f{0, 0, 1});
    forEachChildNode(*n, [&](const core::FieldInfo &,
                             const std::shared_ptr<nodes::X3DNode> &c) {
      self(self, c, authored);
    });
  };
  for (const auto &root : doc.scene.rootNodes)
    visit(visit, root, doc.scene.authoredScalarFields);
  for (const auto &decl : doc.scene.protoDeclarations)
    if (decl)
      for (const auto &n : decl->body.nodes) visit(visit, n, decl->authoredScalarFields);
}

/// The lowest X3D version whose node and field set covers the authored content
/// (never below 3.0, the earliest X3D version).
inline X3DVersion requiredVersion(const Scene &scene) {
  X3DVersion need{3, 0};
  for (const auto &u : versionUsesAbove(scene, need))
    need = std::max(need, u.since);
  return need;
}

/// Add a ReaderWarning{Version} for each authored node type or field newer than
/// the version `doc` declares. Silent for an undeclared version (the readers
/// then read as 3.0, which says nothing about the author's intent) or one that
/// is not MAJOR.MINOR.
inline void collectVersionWarnings(X3DDocument &doc) {
  if (!doc.versionDeclared) return;
  const auto declared = parseX3DVersion(doc.version);
  if (!declared) return;
  for (const auto &u : versionUsesAbove(doc.scene, *declared))
    doc.readerWarnings.push_back(
        {ReaderWarning::Kind::Version,
         u.label() + " needs X3D " + u.since.str() +
             " but the document declares " + doc.version});
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_VERSION_FIT_HPP
