// X3DScene.hpp
// Hand-written runtime model for the X3D <Scene> statement.
#ifndef X3D_RUNTIME_SCENE_HPP
#define X3D_RUNTIME_SCENE_HPP

#include "X3DImportExport.hpp"
#include "X3DAuthoredScalarFields.hpp"
#include "X3DProto.hpp"
#include "X3DRoute.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {

/**
 * @brief The root <Scene> of an X3D document.
 * @details Holds the scene graph (root nodes), the DEF symbol table that gives
 *          nodes their identity, the list of ROUTEs, the PROTO/ExternPROTO
 *          declarations visible at scene scope, and IMPORT/EXPORT statements.
 *
 *          DEF / USE semantics: a node introduced with a DEF is registered in
 *          `defs` keyed by its DEF name. A USE reference resolves to the SAME
 *          std::shared_ptr<X3DNode> via `resolve()`, so the graph shares node
 *          identity rather than cloning. Codecs emit a `DEF` attribute the
 *          first time a shared node is written and a `USE` attribute thereafter.
 */
class Scene {
public:
  // Root-level children of the scene (MFNode).
  std::vector<std::shared_ptr<X3DNode>> rootNodes;

  // DEF symbol table: DEF name -> the shared node it labels.
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defs;

  // Event ROUTEs at scene scope.
  std::vector<Route> routes;

  // Prototype declarations at scene scope.
  std::vector<std::shared_ptr<ProtoDeclaration>> protoDeclarations;
  std::vector<std::shared_ptr<ExternProtoDeclaration>> externProtoDeclarations;

  // ProtoInstances encountered at scene scope, retained as structural data
  // for expansion and round-tripping. Reader-authored instances also have
  // template nodes in their exact scene graph slots.
  std::vector<ProtoInstance> protoInstances;

  // An authored placement template is a node slot; the matching structural
  // record remains the source for name, DEF, and fieldValues before expansion.
  const ProtoInstance *instanceAtPlacement(const X3DNode *node) const {
    for (const auto &instance : protoInstances)
      if (auto placed = instance.placementTemplate.lock();
          placed && placed.get() == node)
        return &instance;
    return nullptr;
  }

  // Body-internal ROUTEs of expanded instances, pre-resolved to concrete
  // endpoints (proto-local DEF scope; NOT registered in `defs`).
  std::vector<ResolvedProtoRoute> resolvedProtoRoutes;

  // Non-rendered peers of expanded PROTO bodies stay live (§4.4.4.3).
  std::vector<std::shared_ptr<X3DNode>> protoPeerNodes;

  // Inline expansion (Tier 1): synthetic Group (key) -> the original Inline node
  // it replaced. Writers consult this to re-emit <Inline url=.../> rather than
  // the spliced content (AUD-B redirect pattern; mirrors expandedSources).
  std::unordered_map<X3DNode *, std::shared_ptr<X3DNode>> expandedInlines;

  // Internal ROUTEs of expanded Inlines, pre-resolved against the CHILD scene's
  // own DEF scope (never registered in this scene's `defs`). The bridge
  // registers these directly. Reuses ResolvedProtoRoute (same endpoint shape).
  std::vector<ResolvedProtoRoute> resolvedInlineRoutes;

  // The child Scene each Inline expanded to, keyed by the ORIGINAL Inline node.
  // Retained so an <IMPORT inlineDEF=... importedDEF=...> can resolve the
  // imported name against the child's exported/DEF'd node (§9.2 cross-Inline
  // escape hatch) without leaking the child's whole DEF table.
  std::unordered_map<const X3DNode *, std::shared_ptr<Scene>> expandedInlineScenes;

  // Exposed interface event fields -> IS-mapped body endpoints, keyed by the
  // expanded primary node pointer then the interface field name. The bridge
  // consults this to redirect external routes that target an instance.
  std::unordered_map<X3DNode *,
                     std::unordered_map<std::string, std::vector<ProtoRedirect>>>
      protoRedirects;

  // Expanded primary node -> its source ProtoInstance, for <ProtoInstance>
  // round-trip on write. Scene-side so the generated X3DNode is untouched.
  std::unordered_map<X3DNode *, ProtoInstance> expandedSources;

  // Authored child-field order (round-trip fidelity): per parent node, the order
  // its node-child fields were first populated during read. Round-trip writers
  // replay this so a node shared across fields (e.g. HAnimHumanoid skeleton vs
  // joints) keeps its authored DEF placement instead of falling back to static
  // field-declaration order. Absent/empty => writers use declaration order.
  // Scene-side so the generated X3DNode is untouched (mirrors expandedSources).
  std::unordered_map<const X3DNode *, std::vector<std::string>> childFieldOrder;

  // Record `field` as authored on `parent` (first-touch order, de-duplicated).
  void recordChildField(const X3DNode *parent, const std::string &field) {
    if (!parent || field.empty()) return;
    auto &order = childFieldOrder[parent];
    for (const std::string &f : order)
      if (f == field) return;
    order.push_back(field);
  }

  // IMPORT / EXPORT statements.
  std::vector<Import> imports;
  std::vector<Export> exports;

  // Snapshot of UNIT declarations in this Scene's source document. Empty means
  // no UNIT was authored there. Writers continue to serialize X3DDocument::head.
  std::vector<Unit> sourceUnits;

  // Scalar fields explicitly accepted by a reader, including values equal to
  // generated defaults. Node identity is weak so marks cannot follow a reused
  // raw address or retain a source scene.
  AuthoredScalarFields authoredScalarFields;

  // Runtime normalization is per field and follows shared node identity. Marks
  // survive repeated builds and separate execution contexts without owning nodes.
  AuthoredScalarFields normalizedUnitFields;

  // PROTO body clones retain the declaration's units. IS scalar targets carry
  // the supplied value's source units; dimensions belong to the concrete target.
  std::map<std::weak_ptr<X3DNode>,
           std::unordered_map<std::string, std::vector<Unit>>,
           std::owner_less<std::weak_ptr<X3DNode>>> unitFieldSources;

  /**
   * @brief Register a node under a DEF name (overwrites any prior binding).
   * @return The same shared_ptr, for convenience.
   */
  std::shared_ptr<X3DNode> define(const std::string &name,
                                  std::shared_ptr<X3DNode> node) {
    defs[name] = node;
    return node;
  }

  /**
   * @brief Resolve a USE reference to the shared node bound to a DEF name.
   * @return The shared node, or nullptr if the DEF name is unknown.
   */
  std::shared_ptr<X3DNode> resolve(const std::string &defName) const {
    auto it = defs.find(defName);
    return it == defs.end() ? nullptr : it->second;
  }

  /**
   * @brief Add a root node and, if it carries a DEF, register it.
   */
  void addRootNode(std::shared_ptr<X3DNode> node);

  /**
   * @brief Look up a prototype declaration by name (local first, then extern).
   * @return nullptr if no declaration with that name exists in this scene.
   */
  std::shared_ptr<ProtoDeclaration> findProto(const std::string &name) const {
    for (const auto &p : protoDeclarations) {
      if (p && p->name == name) {
        return p;
      }
    }
    return nullptr;
  }

  // Register a reader-authored declaration. A nested declaration replaces an
  // inherited name in this Scene's copied tables, including across PROTO and
  // EXTERNPROTO kinds. The enclosing Scene and instances already bound to a
  // declaration keep their shared_ptrs.
  void declareProto(std::shared_ptr<ProtoDeclaration> decl) {
    if (!decl) return;
    const std::string name = decl->name;
    std::erase_if(protoDeclarations, [&](const auto &p) {
      return p && p->name == name;
    });
    std::erase_if(externProtoDeclarations, [&](const auto &p) {
      return p && p->name == name;
    });
    protoDeclarations.push_back(std::move(decl));
  }

  void declareExternProto(std::shared_ptr<ExternProtoDeclaration> decl) {
    if (!decl) return;
    const std::string name = decl->name;
    std::erase_if(protoDeclarations, [&](const auto &p) {
      return p && p->name == name;
    });
    std::erase_if(externProtoDeclarations, [&](const auto &p) {
      return p && p->name == name;
    });
    externProtoDeclarations.push_back(std::move(decl));
  }

  /**
   * @brief Populate each Route's resolved from/to weak_ptrs from the DEF table.
   * @details Routes that reference unknown (or IMPORTed/external) DEF names are
   *          left with empty weak_ptrs; their DEF-name strings remain valid for
   *          serialization.
   */
  void resolveRoutes() {
    for (auto &r : routes) {
      r.from = resolve(r.fromNode);
      r.to = resolve(r.toNode);
    }
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCENE_HPP
