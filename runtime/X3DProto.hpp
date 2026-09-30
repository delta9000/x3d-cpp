// X3DProto.hpp
// Hand-written runtime model for X3D prototypes:
//   <ProtoDeclare> / <ExternProtoDeclare> (declarations)
//   <ProtoInterface> + <field> (interface)
//   <ProtoBody> (implementation body)
//   <ProtoInstance> + <fieldValue> (instantiation)
//
// NOTE: This models the *structure* of prototypes so a document can carry and
//       round-trip their declarations and instances. Full PROTO expansion
//       (instantiating the body with field substitution / IS routing) is a
//       documented stub: see ProtoInstance::expand().
#ifndef X3D_RUNTIME_PROTO_HPP
#define X3D_RUNTIME_PROTO_HPP

#include "x3d/core/X3DReflection.hpp" // AccessType, X3DFieldType
#include "X3DAuthoredScalarFields.hpp"
#include "X3DHeader.hpp"
#include "X3DRoute.hpp"
#include "x3d/nodes/X3DNode.hpp"

#include <any>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {
using namespace x3d::core;

class ProtoInstance; // defined below; ProtoBody holds a vector of these
struct ProtoDeclaration;
struct ExternProtoDeclaration;

// Direct children of a ProtoBody in authored order. Existing body vectors
// remain the authority for nodes and instances; declarations are owned here.
struct ProtoBodyStatement {
  enum class Kind { Node, Instance, Proto, ExternProto };
  Kind kind;
  std::shared_ptr<X3DNode> node;
  std::size_t instanceIndex = 0;
  std::shared_ptr<ProtoDeclaration> proto;
  std::shared_ptr<ExternProtoDeclaration> externProto;
  // Field containing a child node/instance when this statement belongs to a
  // body node. Empty for declarations and direct ProtoBody statements.
  std::string field{};
};

/// One `field IS protoField` / <connect> mapping captured from a PROTO body.
struct IsConnection {
  std::shared_ptr<X3DNode> node;   // the body node carrying the field
  std::string nodeField;           // its field name
  std::string protoField;          // the interface field it maps to
};

/// A body-internal ROUTE pre-resolved to concrete cloned endpoints. Holds
/// shared_ptr (not raw FieldAddress) so auxiliary body nodes stay alive.
struct ResolvedProtoRoute {
  std::shared_ptr<X3DNode> from;
  std::string fromField;
  std::shared_ptr<X3DNode> to;
  std::string toField;
};

/// One redirect target for an exposed interface event field.
struct ProtoRedirect {
  std::shared_ptr<X3DNode> targetNode;  // cloned body node (keep-alive)
  std::string targetField;
};

/// Non-fatal PROTO diagnostic, collected into X3DDocument.protoWarnings.
struct ProtoWarning {
  enum class Kind {
    UnresolvedExtern, MissingDeclaration, InterfaceMismatch,
    RecursionLimit, UnknownField, BuiltinShadow
  };
  Kind kind;
  std::string instanceName;
  std::string detail;
};

/// Non-fatal Inline diagnostic, collected into X3DDocument.inlineWarnings.
struct InlineWarning {
  enum class Kind { UnresolvedUrl, LoadError };
  Kind kind;
  std::string inlineDEF; // DEF of the Inline node, or "" if anonymous
  std::string detail;    // e.g. the url that failed to resolve
};

/// Non-fatal reader diagnostic, collected into X3DDocument.readerWarnings.
/// Emitted when a reader recovers from authored input that is not legal X3D
/// (an unknown node element is discarded, a profile token is coerced), so the
/// recovery stays visible to `x3d validate` instead of being silent.
struct ReaderWarning {
  enum class Kind { UnknownNode, ProfileCoerced };
  Kind kind;
  std::string detail;
};

/**
 * @brief One <field> declaration in a ProtoInterface / ExternProtoDeclare.
 * @details Mirrors the X3D <field name type accessType value/> statement.
 *          For initializeOnly / inputOutput fields a default `value` may be
 *          present (boxed in std::any, concrete type per `type`); for SFNode /
 *          MFNode default-valued fields, child nodes are carried in
 *          `nodeDefault`. inputOnly / outputOnly fields carry no value.
 */
struct ProtoField {
  std::string name;
  std::string appinfo;
  std::string documentation;
  X3DFieldType type = X3DFieldType::SFString;
  AccessType access = AccessType::InputOutput;

  // Default value for SF*/MF* scalar fields (empty if none / event field).
  std::any value;
  // Default child node(s) for SFNode/MFNode fields.
  std::vector<std::shared_ptr<X3DNode>> nodeDefault;

  bool hasValue() const { return value.has_value() || !nodeDefault.empty(); }
};

// Match an authored PROTO interface ROUTE endpoint. Exact names win;
// inputOutput aliases are valid only on their corresponding event side.
inline const ProtoField *findProtoRouteField(
    const std::vector<ProtoField> &fields, const std::string &name,
    bool asSource) {
  for (const auto &field : fields)
    if (field.name == name) return &field;
  std::string base;
  if (!asSource && name.rfind("set_", 0) == 0)
    base = name.substr(4);
  else if (asSource && name.size() > 8 &&
           name.compare(name.size() - 8, 8, "_changed") == 0)
    base = name.substr(0, name.size() - 8);
  if (base.empty()) return nullptr;
  for (const auto &field : fields)
    if (field.name == base && field.access == AccessType::InputOutput)
      return &field;
  return nullptr;
}

/**
 * @brief The body of a PROTO: the nodes (and nested ROUTEs) it instantiates.
 * @details The first child node of a ProtoBody is the prototype's primary
 *          type; remaining children are auxiliary. IS/connect mappings inside
 *          the body are represented on the generated nodes' `IS` field, so they
 *          are not duplicated here.
 */
struct ProtoBody {
  std::vector<std::shared_ptr<X3DNode>> nodes;
  std::vector<Route> routes;
  std::vector<IsConnection> isConnections;
  // ProtoInstances that appear INSIDE this body (template). Each carries its
  // placement (parent = an original body node, or empty for a direct body-root
  // child) so the expansion engine can splice the expanded primary into the
  // per-instantiation CLONE of that parent. Kept here, not in scene.protoInstances,
  // so a body-nested instance is expanded once per outer instantiation rather
  // than once globally / mis-attached to the un-cloned template.
  std::vector<ProtoInstance> nestedInstances;
  // Entries referring to nestedInstances use indices. Editing that vector's
  // order requires updating these entries too. Unrecorded programmatic nodes
  // and direct instances are appended by orderedStatements().
  std::vector<ProtoBodyStatement> statements;
  // Authored children of each body node, including declarations that have no
  // reflected field value. Weak keys avoid retaining removed template nodes.
  std::map<std::weak_ptr<X3DNode>, std::vector<ProtoBodyStatement>,
           std::owner_less<std::weak_ptr<X3DNode>>> nodeStatements;

  void recordChildNode(const std::shared_ptr<X3DNode> &parent,
                       const std::string &field,
                       const std::shared_ptr<X3DNode> &child) {
    nodeStatements[parent].push_back(
        {ProtoBodyStatement::Kind::Node, child, 0, {}, {}, field});
  }
  void recordChildInstance(const std::shared_ptr<X3DNode> &parent,
                           const std::string &field, std::size_t index) {
    nodeStatements[parent].push_back(
        {ProtoBodyStatement::Kind::Instance, {}, index, {}, {}, field});
  }
  void recordChildProto(const std::shared_ptr<X3DNode> &parent,
                        const std::shared_ptr<ProtoDeclaration> &decl) {
    nodeStatements[parent].push_back(
        {ProtoBodyStatement::Kind::Proto, {}, 0, decl, {}});
  }
  void recordChildExternProto(
      const std::shared_ptr<X3DNode> &parent,
      const std::shared_ptr<ExternProtoDeclaration> &decl) {
    nodeStatements[parent].push_back(
        {ProtoBodyStatement::Kind::ExternProto, {}, 0, {}, decl});
  }

  void recordNode(const std::shared_ptr<X3DNode> &node) {
    statements.push_back({ProtoBodyStatement::Kind::Node, node, 0, {}, {}});
  }
  void recordInstance(std::size_t index) {
    statements.push_back({ProtoBodyStatement::Kind::Instance, {}, index, {}, {}});
  }
  void recordProto(const std::shared_ptr<ProtoDeclaration> &decl) {
    statements.push_back({ProtoBodyStatement::Kind::Proto, {}, 0, decl, {}});
  }
  void recordExternProto(const std::shared_ptr<ExternProtoDeclaration> &decl) {
    statements.push_back({ProtoBodyStatement::Kind::ExternProto, {}, 0, {}, decl});
  }
  std::vector<ProtoBodyStatement> orderedStatements() const;
};

/**
 * @brief A <ProtoDeclare> statement: a named, locally-defined prototype.
 */
struct ProtoDeclaration {
  std::string name;
  std::vector<ProtoField> interface; // <ProtoInterface> fields
  ProtoBody body;                    // <ProtoBody>
  std::string appinfo;               // optional documentation metadata
  std::string documentation;         // optional documentation URL
  // Snapshot of UNIT declarations in the document that authored this PROTO.
  // Empty means that source declared no UNIT; Head remains the serializable
  // authority for its document.
  std::vector<Unit> sourceUnits;
  // Body nodes may outlive their reader's temporary Scene (including an
  // EXTERN resolver's document). Carry their field-presence marks here.
  AuthoredScalarFields authoredScalarFields;
};

/**
 * @brief An <ExternProtoDeclare> statement: a prototype declared elsewhere.
 * @details Carries the interface (so instances can be validated/round-tripped)
 *          plus the `url` list pointing at the external definition. It has no
 *          body in this document.
 */
struct ExternProtoDeclaration {
  std::string name;
  std::vector<ProtoField> interface;
  std::vector<std::string> url;
  std::string appinfo;
  std::string documentation;
};

/**
 * @brief One <fieldValue> in a ProtoInstance: a value for a named proto field.
 */
struct ProtoFieldValue {
  std::string name;
  std::any value;                                     // scalar override
  std::vector<std::shared_ptr<X3DNode>> nodeValue;    // SFNode/MFNode override
  // Source of an authored/forwarded scalar, independent of its eventual IS
  // target. nullopt supports programmatically assembled values in caller units.
  std::optional<std::vector<Unit>> sourceUnits = std::nullopt;
};

/// One `nodeField IS protoField` mapping attached to a ProtoInstance.
struct ProtoInstanceIsConnection {
  std::string nodeField;   // nested instance interface field
  std::string protoField;  // enclosing proto interface field
};

/**
 * @brief A <ProtoInstance>: an instantiation of a (Extern)ProtoDeclaration.
 * @details An instance is itself usable wherever a node is (it carries a
 *          containerField). Full expansion is a documented stub.
 */
class ProtoInstance {
public:
  std::string name;                       // the prototype being instantiated
  std::vector<ProtoFieldValue> fieldValues;
  std::vector<ProtoInstanceIsConnection> isConnections;
  std::string DEF;                        // optional DEF
  std::string USE;                        // optional USE
  std::string containerField = "children";

  // Exact authored node slot for a scene instance, either a Scene root or a
  // child of an ordinary node. A weak pointer keeps deletion from that slot
  // authoritative; DEF names separately keep the template reachable.
  std::weak_ptr<X3DNode> placementTemplate;

  // A weak pointer retains its owner identity after its target expires. This
  // distinguishes a deleted authored node slot from a programmatic instance
  // that never had a node slot, without another persistent state flag.
  bool hasPlacementTemplate() const {
    const std::weak_ptr<X3DNode> empty;
    return placementTemplate.owner_before(empty) ||
           empty.owner_before(placementTemplate);
  }

  // Placement: where this instance sits in the graph so expansion can splice
  // the primary node back in. Empty `parent` => the instance is a Scene root.
  std::weak_ptr<X3DNode> parent;
  std::string parentField;          // containerField slot on `parent`

  // Resolved declaration this instance refers to (optional; set by the model
  // after PROTO declarations are collected). Either a local or extern decl.
  std::shared_ptr<ProtoDeclaration> declaration;
  std::shared_ptr<ExternProtoDeclaration> externDeclaration;

  // Set by expandScene when this instance was successfully expanded into the
  // graph (its primary node is then re-emitted via Scene::expandedSources).
  // A failed reader-authored instance keeps its template in the graph; an
  // unlinked programmatic instance is emitted from scene.protoInstances.
  bool expanded = false;

  // ADR-0033: this instance named a PROTO/EXTERNPROTO that was quarantined for
  // shadowing a built-in node type. Expansion creates the built-in instead and
  // applies the fieldValues to its fields by name.
  bool builtinFallback = false;

  /**
   * @brief Expand this instance into a concrete node tree. STUB.
   * @details Full PROTO expansion (cloning the ProtoBody, substituting field
   *          values, and wiring IS/connect routes) is intentionally not
   *          implemented in this stage. The model retains all information
   *          needed to perform it later. Returns nullptr to signal "not
   *          expanded".
   */
  std::shared_ptr<X3DNode> expand() const { return nullptr; }
};

// An authored ProtoInstance occupies a node slot among concrete nodes in a
// PROTO interface default, instance fieldValue, or scene graph. This preserves
// order and DEF/USE pointer identity until expansion materializes its graph.
class ProtoInstanceTemplate final : public X3DNode {
public:
  ProtoInstance instance;

  explicit ProtoInstanceTemplate(ProtoInstance source)
      : instance(std::move(source)) {
    setDEF(instance.DEF);
  }

  std::string nodeTypeName() const override { return "ProtoInstance"; }
  std::string defaultContainerField() const override {
    return instance.containerField;
  }
};

inline std::vector<ProtoBodyStatement> ProtoBody::orderedStatements() const {
  std::vector<ProtoBodyStatement> result;
  std::unordered_map<const X3DNode *, std::size_t> availableNodes;
  std::unordered_map<const X3DNode *, std::size_t> consumedNodes;
  std::unordered_set<std::size_t> seenInstances;
  for (const auto &node : nodes)
    if (node) ++availableNodes[node.get()];
  for (const auto &entry : statements) {
    if (entry.kind == ProtoBodyStatement::Kind::Node) {
      if (!entry.node) continue;
      if (consumedNodes[entry.node.get()] >= availableNodes[entry.node.get()])
        continue;
      ++consumedNodes[entry.node.get()];
    }
    if (entry.kind == ProtoBodyStatement::Kind::Instance) {
      if (entry.instanceIndex >= nestedInstances.size() ||
          nestedInstances[entry.instanceIndex].parent.lock())
        continue;
      seenInstances.insert(entry.instanceIndex);
    }
    if (entry.kind == ProtoBodyStatement::Kind::Proto && !entry.proto)
      continue;
    if (entry.kind == ProtoBodyStatement::Kind::ExternProto && !entry.externProto)
      continue;
    result.push_back(entry);
  }
  for (const auto &node : nodes) {
    if (!node) continue;
    if (consumedNodes[node.get()] > 0) {
      --consumedNodes[node.get()];
      continue;
    }
    result.push_back({ProtoBodyStatement::Kind::Node, node, 0, {}, {}});
  }
  for (std::size_t i = 0; i < nestedInstances.size(); ++i)
    if (!nestedInstances[i].parent.lock() && !seenInstances.contains(i))
      result.push_back({ProtoBodyStatement::Kind::Instance, {}, i, {}, {}});
  return result;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_HPP
