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

/// Authored node types and fields newer than `floor`, one entry per distinct
/// type or field, sorted by name. A field is listed only when it is newer than
/// its node type too (IndexedFaceSet.tangent, not EnvironmentLight.intensity). Walks the scene graph and every PROTO
/// declaration body. A field counts as authored when a reader recorded it
/// (Scene::authoredScalarFields, Scene::childFieldOrder); generated defaults
/// never count.
inline std::vector<VersionUse> versionUsesAbove(const Scene &scene,
                                                X3DVersion floor) {
  std::set<VersionUse> uses;
  std::unordered_set<const nodes::X3DNode *> seen;
  auto visit = [&](auto &&self,
                   const std::shared_ptr<nodes::X3DNode> &node) -> void {
    if (!node || !seen.insert(node.get()).second) return;
    const std::string type = node->nodeTypeName();
    if (floor < node->introducedIn())
      uses.insert({type, {}, node->introducedIn()});
    const auto order = scene.childFieldOrder.find(node.get());
    for (const auto &f : node->fields()) {
      // A field as old as its node type is covered by the node's own entry.
      if (!(floor < f.since) || !(node->introducedIn() < f.since)) continue;
      bool authored = scene.authoredScalarFields.contains(node, f.x3dName);
      if (!authored && order != scene.childFieldOrder.end())
        authored = std::find(order->second.begin(), order->second.end(),
                             f.x3dName) != order->second.end();
      if (authored) uses.insert({type, f.x3dName, f.since});
    }
    forEachChildNode(*node, [&](const core::FieldInfo &,
                                const std::shared_ptr<nodes::X3DNode> &child) {
      self(self, child);
    });
  };
  for (const auto &root : scene.rootNodes) visit(visit, root);
  for (const auto &decl : scene.protoDeclarations)
    if (decl)
      for (const auto &n : decl->body.nodes) visit(visit, n);
  return {uses.begin(), uses.end()};
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
