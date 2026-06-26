// Vrml97Dialect.hpp
// The thin VRML97 (ISO/IEC 14772-1) -> X3D dialect layer. VRML97 and X3D
// ClassicVRML share ~90% of their concrete syntax (brace-delimited
// `Type { field value }` nodes, DEF/USE, ROUTE, PROTO/EXTERNPROTO, `#`
// comments, identical SF*/MF* value lexis), so the tokenizer/parser is built
// ONCE in ClassicVrmlReader and the VRML97-ness lives entirely here: a static
// name-remap table applied during node/field lookup.
//
// VRML97 was deliberately forward-compatible, so the table is tiny — almost
// every name is identical and passes through unchanged. It is deny-by-omission-
// SAFE: any token not in the table is returned verbatim and resolved against
// the generated reflection layer. This keeps the table to the handful of true
// renames rather than enumerating all 54 VRML97 nodes.
//
// Known deltas (per the VRML97 reader spec):
//   * access keywords  eventIn/eventOut/field/exposedField -> inputOnly/
//     outputOnly/initializeOnly/inputOutput  (already handled in
//     ClassicVrmlReader::accessTypeFromString; listed here for completeness).
//   * field renames    LOD.level   -> children   (X3D renamed level ->
//   children)
//                      Switch.choice -> children  (X3D renamed choice ->
//                      children)
//   * boolean literals TRUE/FALSE -> true/false   (already normalized in the
//     value layer by FieldValueIO::parseValue; no action needed here).
//   * nodes            (none) — all 54 VRML97 nodes exist verbatim in X3D 4.0.
//
// Header-only, namespace x3d::codec.
#ifndef X3D_PARSE_VRML97_DIALECT_HPP
#define X3D_PARSE_VRML97_DIALECT_HPP

#include <string>

namespace x3d::codec::vrml97 {

/// Map a VRML97 node type token to its X3D type name. All 54 VRML97 nodes keep
/// their names in X3D 4.0, so this is currently the identity; it exists so a
/// future delta has a single home and so the reader's call site reads
/// uniformly.
inline std::string mapNodeName(const std::string &token) { return token; }

/// Map a VRML97 field-name token (on a node of `nodeType`) to its X3D field
/// name. Only two node-field renames matter for the corpus, both collapsing the
/// VRML97 child-holder onto X3D's unified `children`:
///   LOD.level  -> children
///   Switch.choice -> children
/// Everything else passes through unchanged (deny-by-omission-safe).
inline std::string mapFieldName(const std::string &nodeType,
                                const std::string &token) {
  if (nodeType == "LOD" && token == "level")
    return "children";
  if (nodeType == "Switch" && token == "choice")
    return "children";
  return token;
}

} // namespace x3d::codec::vrml97

#endif // X3D_PARSE_VRML97_DIALECT_HPP
