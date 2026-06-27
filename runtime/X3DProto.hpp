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
#include "X3DRoute.hpp"

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class X3DNode;

namespace x3d::runtime {

class ProtoInstance; // defined below; ProtoBody holds a vector of these

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
    RecursionLimit, UnknownField
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
  X3DFieldType type = X3DFieldType::SFString;
  AccessType access = AccessType::InputOutput;

  // Default value for SF*/MF* scalar fields (empty if none / event field).
  std::any value;
  // Default child node(s) for SFNode/MFNode fields.
  std::vector<std::shared_ptr<X3DNode>> nodeDefault;

  bool hasValue() const { return value.has_value() || !nodeDefault.empty(); }
};

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

  // Placement: where this instance sits in the graph so expansion can splice
  // the primary node back in. Empty `parent` => the instance is a Scene root.
  std::weak_ptr<X3DNode> parent;
  std::string parentField;          // containerField slot on `parent`

  // Resolved declaration this instance refers to (optional; set by the model
  // after PROTO declarations are collected). Either a local or extern decl.
  std::shared_ptr<ProtoDeclaration> declaration;
  std::shared_ptr<ExternProtoDeclaration> externDeclaration;

  // Set by expandScene when this instance was successfully expanded + spliced
  // into the graph (its primary node is then re-emitted via Scene::expandedSources).
  // FALSE means expansion failed/was skipped (e.g. unresolvable EXTERNPROTO in
  // headless mode); such instances are not in the node graph, so the writers must
  // re-emit them directly from scene.protoInstances or they are lost (AUD-B).
  bool expanded = false;

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

} // namespace x3d::runtime

#endif // X3D_RUNTIME_PROTO_HPP
