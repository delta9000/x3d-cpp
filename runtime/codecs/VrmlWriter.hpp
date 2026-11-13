// ClassicVRML writer. Multi-valued fields retain brackets, including empty lists.

#ifndef X3D_VRML_WRITER_HPP
#define X3D_VRML_WRITER_HPP

#include <any>
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
struct ProtoBody;
struct ProtoField;
class ProtoInstance;
class Scene;
class AuthoredScalarFields;
class DynamicFieldStore;
class X3DDocument;
} // namespace x3d::runtime

namespace x3d::core {
enum class X3DFieldType;
struct FieldInfo;
} // namespace x3d::core

namespace x3d::codec {
using namespace x3d::core;
using x3d::nodes::X3DNode;

class VrmlWriter {
public:
  std::string writeDocument(const runtime::X3DDocument &doc);

private:
  // ClassicVRML (ISO 19776-2) writes booleans as TRUE/FALSE, not the XML/JSON
  // lowercase tokens. formatValue() yields lowercase; uppercase each token here
  // for SFBool and the space-separated MFBool list, leaving everything else as
  // emitted by the shared formatter.
  static std::string vrmlBoolCase(X3DFieldType type, const std::string &in);

  // ClassicVRML / VRML97
  // grammar (ISO/IEC 19776-2, VRML97 Annex A) requires an MF value to be
  // bracketed
  //   mfValue ::= '[' ']' | '[' sfValue+ ']' | sfValue
  // A bare run of >1 sfValue is not a valid mfValue (it truncates to the first
  // element on reparse), and an empty MF must be written '[ ]'. We always
  // bracket MF values (single/empty included) — always grammatical, never
  // lossy.
  static void writeInterfaceValue(std::ostringstream &os, X3DFieldType type,
                                  const std::any &value);

  // ClassicVRML SFString literals are double-quoted; embedded backslash and
  // double-quote must be escaped (\\ and \"). MFString self-escapes via
  // fmtMFString; this is only for the bare SFString branches.
  static std::string vrmlEscapeString(const std::string &in);

  std::unordered_set<const X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup. Null for body writers.
  const runtime::Scene *scene_ = nullptr;
  const runtime::AuthoredScalarFields *unitDefaults_ = nullptr;
  const runtime::DynamicFieldStore *authorFields_ = nullptr;
  // Borrowed ProtoBody connections; null outside body serialization.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;
  // Borrowed nested placements; null outside body serialization.
  const std::vector<runtime::ProtoInstance> *bodyNested_ = nullptr;
  const runtime::ProtoBody *bodyOrder_ = nullptr;

  static void pad(std::ostringstream &os, int depth);

  X3DNode *defaultFor(const std::string &typeName);

  /// Emit a proto instance: `[DEF d ]Name {\n  field value\n}\n`
  void writeVrmlProtoInstance(std::ostringstream &os,
                              const runtime::ProtoInstance &src, int depth);

  /// emit `slot [ <ProtoInstance> ... ]` blocks for every Case-A nested
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

  // emit author field interface declarations from the author-field store as
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
