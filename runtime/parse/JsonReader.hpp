// JsonReader.hpp
// Node-agnostic X3D-JSON parser: turns an X3D-JSON string into the runtime
// document model (x3d::runtime::X3DDocument). It is the inverse of JsonWriter
// and completes read/write symmetry for the .json encoding.
//
// It walks exactly the shape JsonWriter emits (Web3D X3D-JSON conventions):
//
//   { "X3D": { "@profile": "...", "@version": "...",
//              "head": { "component": [ {"@name","@level"} ],
//                        "meta":      [ {"@name","@content"} ],
//                        "unit":      [
//                        {"@category","@name","@conversionFactor"} ] },
//              "Scene": { "-children": [ <node>, ... ] } } }
//
// where each node object is  { "TypeName": { <members> } }  and members are:
//   * "@DEF" / "@USE"        — DEF/USE identity (USE shares the prior
//   shared_ptr);
//   * "@field": value        — a value field (scalar / array / string), mapped
//                              back to the X3D wire string parseValue()
//                              expects;
//   * "-containerField": [..] — an SF/MFNode child slot holding child node
//                              objects (the de-prefixed key is the
//                              containerField).
//   * "ROUTE": {...} or [..] — scene-scope event routes (under "Scene").
//
// Strategy (all driven by reflection + the node factory, no per-node code):
//   * Instantiate a node by its single object key via X3DNodeFactory::create.
//   * For each "@field" member, find the matching FieldInfo and set the value
//     via build::applyField (enum -> setEnumString, else parseValue + set),
//     after converting the JSON value to the encoding-independent wire string.
//   * "-childField" members route into the parent's SF/MFNode field via
//     build::attachChild using the de-prefixed key as the containerField slot.
//
// JSON value -> wire string conversion bypasses collectFieldValue (that is the
// VRML-token path); here JSON already gives structured values, so numbers are
// space-joined from their original lexemes, bools become true/false, SFString
// is taken verbatim, and MFString elements are re-quoted so parseMFString sees
// them.
//
// Mirrors XmlReader's tolerance: unknown node types / fields are skipped, never
// throwing (malformed JSON itself does throw, via JsonLite).
//
// Public declaration, namespace x3d::codec. The implementation is compiled in
// x3d_cpp_runtime.
#ifndef X3D_PARSE_JSON_READER_HPP
#define X3D_PARSE_JSON_READER_HPP

#include "DynamicField.hpp" // AuthorFieldDecl + dynamicFieldStore() (S1)
#include "FieldAliases.hpp"
#include "FieldValueIO.hpp" // parseInt/parseDouble (x3d::codec)
#include "JsonLite.hpp"     // x3d::json::parse, Value
#include "NodeBuilder.hpp"  // build::beginNode/applyField/attachChild/etc.
#include "X3DReader.hpp"
#include "X3DRuntime.hpp"
#include "x3d/nodes/Script.hpp" // Script::setSourceCode (inline source capture)

#include <memory>
#include <string>

namespace x3d::codec {
using namespace x3d::core;

/// Parses X3D-JSON into the runtime document model. The inverse of JsonWriter.
class JsonReader : public X3DReader {
public:
  Encoding encoding() const override;

  /// Parse a full X3D-JSON document string. Throws std::runtime_error on
  /// malformed JSON (via JsonLite). Unknown node/field names are skipped.
  runtime::X3DDocument readDocument(const std::string &text) override;

private:
  // -------------------------------------------------------------------------
  // Head.
  // -------------------------------------------------------------------------
  void readHead(const json::Value &head, runtime::Head &out);

  // -------------------------------------------------------------------------
  // Scene.
  // -------------------------------------------------------------------------
  void readScene(const json::Value &scene, runtime::Scene &out);

  /// Fill a Route vector from a "ROUTE" member (single object or array).
  void readRoutesInto(const json::Value *v,
                      std::vector<runtime::Route> &routes);

  void readRoutes(const json::Value *v, runtime::Scene &out);

  void readImports(const json::Value *v, runtime::Scene &out);

  void readExports(const json::Value *v, runtime::Scene &out);

  // -------------------------------------------------------------------------
  // Node.
  // -------------------------------------------------------------------------
  /// Build a node (and subtree) from a `{ "TypeName": { ... } }` wrapper
  /// object. `currentProtoBody` is non-null when reading inside a ProtoBody,
  /// and is threaded through to intercept nested PROTO statements.
  std::shared_ptr<X3DNode>
  readNode(const json::Value &wrapper, runtime::Scene &scene,
           runtime::ProtoBody *currentProtoBody = nullptr);

  /// Attach the child node objects under one "-slot" member into `node`.
  /// `parentNode` and `currentProtoBody` allow PROTO statement interception.
  void attachChildren(X3DNode &node, const std::string &slot,
                      const json::Value &val, runtime::Scene &scene,
                      const std::shared_ptr<X3DNode> &parentNode,
                      runtime::ProtoBody *currentProtoBody);

  // -------------------------------------------------------------------------
  // PROTO capture (parity with XmlReader).
  // All capture is LENIENT — a malformed or partial PROTO element captures what
  // it can and skips the rest, never throwing.
  // -------------------------------------------------------------------------

  /// Returns true and captures if `wrapper` is a PROTO statement object
  /// ({ "ProtoDeclare"|"ExternProtoDeclare"|"ProtoInstance": {...} }).
  /// `parent`/`slot` give a nested instance's placement; `body` (non-null
  /// inside a ProtoBody) routes a captured instance into that body's
  /// nestedInstances.
  bool tryReadProtoStatement(const json::Value &wrapper, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot, runtime::ProtoBody *body);

  /// Read every "field" entry of `iface` into `out` (shared by ProtoDeclare
  /// and ExternProtoDeclare). `field` may be a single object or an array.
  void readJsonInterfaceFields(const json::Value &iface,
                               std::vector<runtime::ProtoField> &out,
                               runtime::Scene &scene);

  void readJsonProtoDeclare(const json::Value &obj, runtime::Scene &scene);

  void readJsonExternProtoDeclare(const json::Value &obj,
                                  runtime::Scene &scene);

  /// Read a ProtoBody object: iterate "-children" (PROTO statements go into
  /// `out`, ordinary nodes into `out.nodes`); collect IS connections; collect
  /// any inline ROUTE statements into `out.routes`.
  void readJsonProtoBody(const json::Value &obj, runtime::Scene &scene,
                         runtime::ProtoBody &out);

  /// Record { "IS": { "connect": [ { "@nodeField", "@protoField" } ] } }
  /// found inside a body-node wrapper object.
  void collectJsonIsConnections(const json::Value &wrapper,
                                const std::shared_ptr<X3DNode> &node,
                                runtime::ProtoBody &out);

  // -------------------------------------------------------------------------
  // Script author interface (SCR-SAI-DYN S1).
  // -------------------------------------------------------------------------
  /// Capture a Script object's author <field> declarations + inline source.
  /// `node` is the freshly-built Script (its identity keys the
  /// DynamicFieldStore). Author decls live under the un-prefixed "field" member
  /// (mirroring ProtoInterface's "field"); inline source under "#sourceText"
  /// (canonical X3D-JSON CDATA, an array of lines) or — failing that — an @url
  /// inline scheme. Lenient: a malformed entry is skipped, never thrown.
  void captureScriptInterface(const json::Value &body,
                              const std::shared_ptr<X3DNode> &node,
                              runtime::Scene & /*scene*/);

  /// Return the body of the first ecmascript:/javascript:/vrmlscript: entry in
  /// `url`, or empty if none is an inline script. Mirrors
  /// ScriptSystem::decodeInlineSource so the reader can pre-stage sourceCode.
  static std::string decodeInlineUrl(const MFString &url);

  /// Capture a ProtoInstance: name, DEF/USE, fieldValues (single-or-array).
  /// `parent`/`slot` give placement; `body` routes into body->nestedInstances.
  void readJsonProtoInstance(const json::Value &obj, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot, runtime::ProtoBody *body);

  /// Map an X3D field-type string (e.g. "SFVec3f") to X3DFieldType.
  static X3DFieldType mapProtoFieldType(const std::string &w);

  /// Map an X3D accessType string to AccessType (X3D and VRML spellings).
  static AccessType mapProtoAccessType(const std::string &w);

  // -------------------------------------------------------------------------
  // Field value application.
  // -------------------------------------------------------------------------
  /// Apply one "@field" member: find the FieldInfo, convert the JSON value to
  /// the X3D wire string, and set it via build::applyField. Unknown fields are
  /// skipped. DEF (a normal SFString field on every node) flows through here.
  void applyJsonField(X3DNode &node, const std::string &x3dName,
                      const json::Value &val);

  /// Convert a parsed JSON value to the space-/quote-delimited X3D wire string
  /// that FieldValueIO::parseValue expects for the given field type.
  static std::string jsonToWire(const json::Value &val, X3DFieldType type);

  /// One JSON scalar -> its wire token. Numbers use their original lexeme so an
  /// integer stays integral; bools become true/false; strings pass through (an
  /// enum token emitted by JsonWriter as a JSON string lands here).
  static std::string scalarToken(const json::Value &v);

  /// Re-quote a string for parseMFString, escaping embedded quote/backslash.
  static std::string requoteString(const std::string &s);

  // -------------------------------------------------------------------------
  // Small member accessors.
  // -------------------------------------------------------------------------
  static std::string strMemberOr(const json::Value &obj, const std::string &key,
                                 const std::string &fallback);

  static std::string strMember(const json::Value &obj, const std::string &key);

  static int intMember(const json::Value &obj, const std::string &key,
                       int fallback);

  static double doubleMember(const json::Value &obj, const std::string &key,
                             double fallback);
};

} // namespace x3d::codec

#endif // X3D_PARSE_JSON_READER_HPP
