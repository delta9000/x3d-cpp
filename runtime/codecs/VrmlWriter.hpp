// VrmlWriter.hpp
// Node-agnostic ClassicVRML (X3D ClassicVRML / .x3dv) serializer (writer only).
//
// Emits the ClassicVRML encoding:
//   #X3D V4.0 utf8
//   PROFILE Interchange
//   <statements...>
//   <root nodes...>
// where each node is:
//   [DEF Name] TypeName { field value  field value  childField Node { ... } }
// USE references emit `USE Name`. SF/MF value fields emit `name value`; node
// fields emit `name <node>` (SFNode) or `name [ <node> ... ]` (MFNode). MF/SF
// structured values reuse the X3D wire formatting (space-separated). MFString
// keeps its quoted-list form. Enum fields emit their token via the FieldInfo
// string thunk. ROUTEs emit `ROUTE A.f TO B.g`.
//
// Public declaration, namespace x3d::codec. The implementation is compiled in
// x3d_cpp_authoring_runtime.
#ifndef X3D_VRML_WRITER_HPP
#define X3D_VRML_WRITER_HPP

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
struct IsConnection;
struct ProtoDeclaration;
struct ProtoField;
class ProtoInstance;
class Scene;
class X3DDocument;
} // namespace x3d::runtime

namespace x3d::core {
enum class X3DFieldType;
struct FieldInfo;
} // namespace x3d::core

namespace x3d::codec {
using namespace x3d::core;
using x3d::nodes::X3DNode;

/// Serializes the runtime document model to ClassicVRML.
class VrmlWriter {
public:
  std::string writeDocument(const runtime::X3DDocument &doc);

private:
  /// X3D has no version below 3.0; floor a sub-3.0/legacy token to "3.0" so the
  /// emitted header is always valid (VP-2 §8). >= 3.0 (incl. future) passes
  /// through.
  static std::string headerVersion(const std::string &v);

  // ClassicVRML (ISO 19776-2) writes booleans as TRUE/FALSE, not the XML/JSON
  // lowercase tokens. formatValue() yields lowercase; uppercase each token here
  // for SFBool and the space-separated MFBool list, leaving everything else as
  // emitted by the shared formatter.
  static std::string vrmlBoolCase(X3DFieldType type, const std::string &in);

  // True for every multiple-value (MF*) field type. ClassicVRML / VRML97
  // grammar (ISO/IEC 19776-2, VRML97 Annex A) requires an MF value to be
  // bracketed
  //   mfValue ::= '[' ']' | '[' sfValue+ ']' | sfValue
  // A bare run of >1 sfValue is NOT a valid mfValue (it truncates to the first
  // element on reparse), and an empty MF must be written '[ ]'. We always
  // bracket MF values (single/empty included) — always grammatical, never
  // lossy.
  static bool isMultiField(X3DFieldType t);

  // ClassicVRML SFString literals are double-quoted; embedded backslash and
  // double-quote must be escaped (\\ and \"). MFString self-escapes via
  // fmtMFString; this is only for the bare SFString branches.
  static std::string vrmlEscapeString(const std::string &in);

  std::unordered_set<const X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup. Null for body writers.
  const runtime::Scene *scene_ = nullptr;
  // PRF-1: while re-emitting a ProtoBody, the body's IsConnection list so that
  // writeNode can emit `nodeField IS protoField` lines inside EVERY emitted
  // body node at any depth (mirrors the XML writer's bodyIsc_). Null outside a
  // body.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;
  // PRF-3: while re-emitting a ProtoBody, the body's nestedInstances list so
  // writeNode can inject each Case-A instance (parent == the node being
  // written) INSIDE that node's body, in its parentField slot — mirroring
  // XmlWriter pushing them onto the parent element's children — rather than
  // appending them after the parent node. Null outside a body.
  const std::vector<runtime::ProtoInstance> *bodyNested_ = nullptr;

  static void pad(std::ostringstream &os, int depth);

  X3DNode *defaultFor(const std::string &typeName);

  // ---- PROTO emit helpers ----

  /// Look up a ProtoInstance's interface field by name (for type information).
  static const runtime::ProtoField *
  interfaceField(const runtime::ProtoInstance &src, const std::string &name);

  /// Emit a proto instance: `[DEF d ]Name {\n  field value\n}\n`
  void writeVrmlProtoInstance(std::ostringstream &os,
                              const runtime::ProtoInstance &src, int depth);

  /// PRF-3: emit `slot [ <ProtoInstance> ... ]` blocks for every Case-A nested
  /// instance whose parent is `node`, grouped by parentField slot, so the
  /// ClassicVrmlReader recovers parent == node and the same slot. No-op when no
  /// body nested list is threaded in. `depth` is the slot line's indent level.
  void writeVrmlNestedFor(std::ostringstream &os,
                          const std::shared_ptr<X3DNode> &node, int depth);

  /// Emit a PROTO declaration: `PROTO Name [ interface ] {\n body \n}\n`
  void writeVrmlProtoDeclare(std::ostringstream &os,
                             const runtime::ProtoDeclaration &d);

  /// Emit an ExternProto declaration.
  void writeVrmlExternProtoDeclare(std::ostringstream &os,
                                   const runtime::ExternProtoDeclaration &d);

  void writeNode(std::ostringstream &os, const std::shared_ptr<X3DNode> &node,
                 int depth);

  // Task B: emit author field interface declarations from the S1 store as
  //   accessType FieldType name [default]
  // (the inverse of ClassicVrmlReader::consumeInterfaceDeclaration). A readable
  // (initializeOnly/inputOutput) field whose boxed value is present emits its
  // default; event fields (inputOnly/outputOnly) and value-less fields emit no
  // default. SFString defaults are quoted, booleans cased TRUE/FALSE — matching
  // the rest of the writer.
  void writeAuthorFields(std::ostringstream &os, const X3DNode &node,
                         int depth);

  void writeNodeField(std::ostringstream &os,
                      const std::shared_ptr<X3DNode> &node, const FieldInfo &f,
                      int depth);
};

} // namespace x3d::codec

#endif // X3D_VRML_WRITER_HPP
