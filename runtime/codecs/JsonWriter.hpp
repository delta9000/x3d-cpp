// X3D-JSON writer: value fields use @ keys; node fields use - keys and arrays.
// Numeric fields retain their scalar or array shape.

#ifndef X3D_JSON_WRITER_HPP
#define X3D_JSON_WRITER_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::nodes {
class X3DNode;
} // namespace x3d::nodes

namespace x3d::runtime {
struct ExternProtoDeclaration;
struct Head;
struct IsConnection;
struct ProtoDeclaration;
struct ProtoBody;
struct ProtoField;
class ProtoInstance;
class Scene;
class DynamicFieldStore;
class X3DDocument;
} // namespace x3d::runtime

namespace x3d::core {
struct FieldInfo;
} // namespace x3d::core

namespace x3d::codec {
using namespace x3d::core;
using x3d::nodes::X3DNode;

class JsonWriter {
public:
  std::string writeDocument(const runtime::X3DDocument &doc);

private:
  std::unordered_set<const X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup. Null for body-internal
  // fresh writers so proto expansion redirect is off inside proto bodies.
  const runtime::Scene *scene_ = nullptr;
  const runtime::DynamicFieldStore *authorFields_ = nullptr;
  // Borrowed ProtoBody connections; null outside body serialization.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;
  // Borrowed nested placements; null outside body serialization.
  const std::vector<runtime::ProtoInstance> *bodyNested_ = nullptr;
  const runtime::ProtoBody *bodyOrder_ = nullptr;

  X3DNode *defaultFor(const std::string &typeName);

  static void pad(std::ostringstream &os, int depth);

  static std::string jstr(const std::string &s);

  void writeHead(std::ostringstream &os, const runtime::Head &head, int depth);

  /// Emit all scene -children: PROTO declarations first, then root nodes.
  void writeSceneChildren(std::ostringstream &os, const runtime::Scene &s,
                          int depth);

  /// Emit ROUTE/IMPORT/EXPORT scene-scope statements as members of the "Scene"
  /// object (each an array of '@'-prefixed-attribute objects), matching the
  /// JsonReader's expectations so they survive a JSON round-trip. Each emitted
  /// member is preceded by ",\n" so it chains after the "-children" array.
  void writeSceneStatements(std::ostringstream &os, const runtime::Scene &scene,
                            int depth);

  /// Re-emit a captured <ProtoInstance> as a JSON object.
  void writeJsonProtoInstance(std::ostringstream &os,
                              const runtime::ProtoInstance &src, int depth);

  /// Emit one proto interface `field` as a JSON object.
  std::string jsonProtoField(const runtime::ProtoField &f);

  /// Emit a <ProtoDeclare> as a JSON object entry in the -children array.
  void writeJsonProtoDeclare(std::ostringstream &os,
                             const runtime::ProtoDeclaration &d, int depth);

  /// Emit an <ExternProtoDeclare> as a JSON object entry in the -children
  /// array.
  void writeJsonExternProtoDeclare(std::ostringstream &os,
                                   const runtime::ExternProtoDeclaration &d,
                                   int depth);

  void writeNode(std::ostringstream &os, const std::shared_ptr<X3DNode> &node,
                 int depth);

  /// emit the "IS" member for `node` if bodyIsc_ holds any IsConnection
  /// bound to it (by identity). Returns "" when there is none. Shape:
  ///   "IS": { "connect": [ { "@nodeField":..., "@protoField":... }, ... ] }
  std::string jsonIsBlock(const std::shared_ptr<X3DNode> &node);

  /// build "-<slot>": [ <ProtoInstance> ] member strings for every
  /// Case-A nested instance whose parent is `node`, grouped by parentField slot
  /// so the JsonReader recovers parent == node and the same slot. Empty when
  /// there are none (or no body nested list threaded in). `depth` is the slot's
  /// own indent level; the array contents sit at depth+1.
  std::vector<std::string> jsonNestedFor(const std::shared_ptr<X3DNode> &node,
                                         int depth);

  /// Scene-level nested ProtoInstances: build "-<slot>": [ <ProtoInstance> ]
  /// member strings for every un-expanded instance in scene.protoInstances
  /// whose parent is `node`, grouped by parentField slot. Mirrors jsonNestedFor
  /// but reads from scene_.protoInstances instead of bodyNested_. Empty when
  /// there are none or scene_ is null. `depth` is the slot's own indent level.
  std::vector<std::string>
  jsonSceneNestedFor(const std::shared_ptr<X3DNode> &node, int depth);

  /// Format a value field as a JSON value (scalar, string, or array). Returns
  /// "" for an unsupported type; empty multi-fields produce [].
  std::string jsonValue(const X3DNode &node, const FieldInfo &f);

  /// Emit one node child field as a "-field": [ children ] member, or "" if the
  /// field holds no node.
  std::string jsonNodeField(const std::shared_ptr<X3DNode> &node,
                            const FieldInfo &f, int depth);

  /// Emit author fields as declarations with scalar or structural defaults
  /// (ISO/IEC 19775-1, 29.4.1 and Clause 31). Return "" when none are stored.
  std::string jsonAuthorFields(const X3DNode &node);

  /// re-emit a Script's inline source (Script.sourceCode) as
  /// the canonical X3D-JSON "#sourceText" array, one element per source line so
  /// the JsonReader recovers it. Returns "" when the Script has no inline body.
  std::string jsonScriptSource(const X3DNode &node);
};

} // namespace x3d::codec

#endif // X3D_JSON_WRITER_HPP
