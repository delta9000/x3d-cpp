// Shared reflection-based node construction and field application for encoding readers.
// Unknown types and fields are skipped without throwing.

#ifndef X3D_PARSE_NODE_BUILDER_HPP
#define X3D_PARSE_NODE_BUILDER_HPP

#include "x3d/core/X3DReflection.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::nodes {
class X3DNode;
}

namespace x3d::runtime {
class Scene;
}

namespace x3d::codec {
class VrmlTokenizer;
}

namespace x3d::codec::build {
using namespace x3d::core;
/// Find a field by its X3D name, or null if the node has no such field.
const FieldInfo *findField(const FieldTable &table, std::string_view name);

/// True if `name` names an SFNode/MFNode field on this node (so a reader can
/// disambiguate `fieldName Type{…}` child syntax from a value field).
bool isNodeField(const FieldTable &table, std::string_view name);
// Node construction.

/// Instantiate a node by type name. Returns null for an unknown type so the
/// caller can skip it (consistent with XmlReader).
std::shared_ptr<x3d::nodes::X3DNode> beginNode(std::string_view typeName);
/// Set one value field on a node from its wire string, via the FieldInfo
/// thunks. Enum fields route through setEnumString; everything else through
/// parseValue + set. Fields without a setter and unknown names are silently
/// skipped; initializeOnly storage fields may have a setter. Returns true
/// only when a value was applied, so readers can record authored presence.
/// DEF is handled here too (it is a normal SFString field on every node).
bool applyField(x3d::nodes::X3DNode &node, std::string_view x3dName,
                const std::string &wire);
/// Attach a child node into the correct SF/MFNode field of `parent`.
/// Resolution order (same as XmlReader):
///   1) the node field whose reflected containerField == `slot`;
///   2) failing that, the node field whose x3dName == `slot`;
///   3) failing that, the "children" node field if present;
///   4) failing that, the first writable node field (best-effort).
/// `slot` is the explicit containerField/field-name the syntax supplied (may be
/// empty). SFNode sets; MFNode appends. When `scene` is supplied, the resolved
/// field is recorded in authored order so round-trip writers preserve it.
void attachChild(x3d::nodes::X3DNode &parent, std::string_view slot,
                 const std::shared_ptr<x3d::nodes::X3DNode> &child,
                 runtime::Scene *scene = nullptr);

/// Node-child fields of `node`, ordered by the authored child-field order
/// recorded in `scene` (when present), then any remaining node-child fields in
/// static declaration order. Round-trip writers iterate this instead of the raw
/// field table so a node shared across fields keeps its authored DEF placement
/// (e.g. HAnimHumanoid.skeleton holds the hierarchy, joints holds the USE).
/// With no scene / no recorded order this is exactly declaration order.
std::vector<const FieldInfo *>
orderedChildFields(const x3d::nodes::X3DNode &node,
                   const runtime::Scene *scene);
// DEF/USE.

/// Register a node under a DEF name so a later USE shares its identity. Must be
/// called before recursing into the node's children, so a USE nested inside the
/// subtree resolves to this same shared_ptr.
void defineDef(runtime::Scene &scene, std::string_view def,
               const std::shared_ptr<x3d::nodes::X3DNode> &node);

/// Resolve a USE name to the shared node registered under that DEF,
/// or null if unknown (tolerated; the caller skips).
std::shared_ptr<x3d::nodes::X3DNode> resolveUse(runtime::Scene &scene,
                                                std::string_view name);
// collectFieldValue: gather the right run of VRML tokens for a field.

/// How many whitespace-separated scalar components one SF value of `type` has.
/// (SFString/enum are handled specially by the caller; MF* is bracketed.)
int sfComponentCount(X3DFieldType type);

/// How many whitespace-separated scalar components one element of an MF value
/// of `type` has (e.g. MFVec3f -> 3, MFRotation -> 4, MFInt32 -> 1). Used to
/// size a bracket-less single-element MF run.
int mfElementComponents(X3DFieldType type);

bool isMF(X3DFieldType type);

bool isStringType(X3DFieldType type);

/// Re-quote a string token for handing back into FieldValueIO::parseMFString,
/// escaping embedded quote/backslash (the inverse of the tokenizer's unescape).
std::string requote(const std::string &s);

/// Consume the run of tokens that make up one field's value from a tokenizer
/// and return the joined wire string that FieldValueIO::parseValue (or, for
/// MFString, parseMFString within parseValue) expects.
///
///   * SF scalar / struct: `sfComponentCount(type)` plain tokens, space-joined.
///   * SFString: one quoted string token (re-quoted for parseValue).
///   * MFString: a `[ "a" "b" ]` run (or a single bare string), each element
///     re-quoted and space-joined so quotes are preserved.
///   * other MF*: a bracketed `[ … ]` run, or a single bare SF-element run
///     (VRML permits `field 0 0 0` as a one-element MF) — every inner token
///     space-joined.
///
/// Enum fields are not routed here; the caller reads one token and calls
/// setEnumString directly (per the spec).
std::string collectFieldValue(VrmlTokenizer &tok, X3DFieldType type);

} // namespace x3d::codec::build

#endif // X3D_PARSE_NODE_BUILDER_HPP
