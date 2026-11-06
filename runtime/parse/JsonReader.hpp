// X3D-JSON reader. Unknown node and field names are skipped; malformed JSON throws.
// Numeric wire values preserve original JSON lexemes.

#ifndef X3D_PARSE_JSON_READER_HPP
#define X3D_PARSE_JSON_READER_HPP

#include "X3DReader.hpp"
#include "X3DRuntime.hpp"

#include <memory>
#include <string>
#include <vector>

namespace x3d::json {
struct Value;
}

namespace x3d::codec {
using namespace x3d::core;

class JsonReader : public X3DReader {
public:
  Encoding encoding() const override;

  /// Parse a full X3D-JSON document string. Throws std::runtime_error on
  /// malformed JSON (via JsonLite). Unknown node/field names are skipped.
  runtime::X3DDocument readDocument(const std::string &text) override;

private:
  // Reader-recovery diagnostics accumulated during the current readDocument()
  // call, moved into X3DDocument.readerWarnings on return.
  std::vector<runtime::ReaderWarning> readerWarnings_;
  bool capturingProtoFieldValue_ = false;
  void readHead(const json::Value &head, runtime::Head &out);
  void readScene(const json::Value &scene, runtime::Scene &out);

  /// Fill a Route vector from a "ROUTE" member (single object or array).
  void readRoutesInto(const json::Value *v,
                      std::vector<runtime::Route> &routes);

  void readRoutes(const json::Value *v, runtime::Scene &out);

  void readImports(const json::Value *v, runtime::Scene &out);

  void readExports(const json::Value *v, runtime::Scene &out);
  // Node.
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
  // PROTO capture (parity with XmlReader).
  // All capture is lenient — a malformed or partial PROTO element captures what
  // it can and skips the rest, never throwing.

  /// Capture statement wrappers in child arrays (ISO/IEC 19776-5 Annex A).
  /// `parent`/`slot` give a nested instance's placement; `body` (non-null
  /// inside a ProtoBody) routes a captured instance into that body's
  /// nestedInstances.
  bool tryReadStatement(const json::Value &wrapper, runtime::Scene &scene,
                        const std::shared_ptr<X3DNode> &parent,
                        const std::string &slot, runtime::ProtoBody *body);

  /// Read every "field" entry of `iface` into `out` (shared by ProtoDeclare
  /// and ExternProtoDeclare). `field` may be a single object or an array.
  void readJsonInterfaceFields(const json::Value &iface,
                               std::vector<runtime::ProtoField> &out,
                               runtime::Scene &scene, runtime::ProtoBody *body = nullptr);

  std::shared_ptr<runtime::ProtoDeclaration>
  readJsonProtoDeclare(const json::Value &obj, runtime::Scene &scene);

  std::shared_ptr<runtime::ExternProtoDeclaration>
  readJsonExternProtoDeclare(const json::Value &obj,
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
  // Script author interface .
  /// Capture author field declarations and Script inline source
  /// (ISO/IEC 19776-5 Annex A). The node's identity keys the
  /// DynamicFieldStore. Author decls live under the un-prefixed "field" member
  /// (mirroring ProtoInterface's "field"); inline source under "#sourceText"
  /// (canonical X3D-JSON CDATA, an array of lines) or — failing that — an @url
  /// inline scheme. Lenient: a malformed entry is skipped, never thrown.
  void captureAuthorInterface(const json::Value &body,
                               const std::shared_ptr<X3DNode> &node,
                               runtime::Scene &scene,
                               runtime::ProtoBody *currentProtoBody);

  /// Return the body of the first ecmascript:/javascript:/vrmlscript: entry in
  /// `url`, or empty if none is an inline script. Mirrors
  /// ScriptSystem::decodeInlineSource so the reader can pre-stage sourceCode.
  static std::string decodeInlineUrl(const MFString &url);

  /// Capture a ProtoInstance: name, DEF/USE, fieldValues (single-or-array).
  /// `parent`/`slot` give placement; `body` routes into body->nestedInstances.
  void readJsonProtoInstance(const json::Value &obj, runtime::Scene &scene,
                             const std::shared_ptr<X3DNode> &parent,
                             const std::string &slot, runtime::ProtoBody *body);

  std::shared_ptr<X3DNode> readJsonInstanceTemplate(
      const json::Value &obj, runtime::Scene &scene);

  /// Map an X3D field-type string (e.g. "SFVec3f") to X3DFieldType.
  static X3DFieldType mapProtoFieldType(const std::string &w);

  /// Map an X3D accessType string to AccessType (X3D and VRML spellings).
  static AccessType mapProtoAccessType(const std::string &w);
  // Field value application.
  /// Apply one "@field" member: find the FieldInfo, convert the JSON value to
  /// the X3D wire string, and set it via build::applyField. Unknown fields are
  /// skipped. DEF (a normal SFString field on every node) flows through here.
  bool applyJsonField(X3DNode &node, const std::string &x3dName,
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
  // Small member accessors.
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
