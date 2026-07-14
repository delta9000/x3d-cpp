// XmlWriter.hpp
// Node-agnostic X3D-XML serializer: turns an x3d::runtime::X3DDocument (or a
// single node / Scene) into the X3D-XML encoding.
//
// Strategy (all driven by reflection, no per-node code):
//   * A node element is named by node->nodeTypeName().
//   * Each reflected value field (SF*/MF* that is not a node) becomes an XML
//     attribute, formatted via FieldValueIO. Enum fields use the FieldInfo
//     string thunk. DEF/USE are emitted from the node's DEF/USE fields.
//   * Fields equal to their type's default are omitted (clean output). The
//     default is discovered by formatting the same field on a fresh factory
//     instance of the node's type.
//   * SFNode/MFNode fields become child elements, placed under the child's
//     containerField (the field's own name unless the child overrides it).
//   * DEF/USE: the first time a shared node (by identity) is written it gets a
//     DEF; subsequent writes emit a <Type USE='...'/> reference instead.
//   * The document wraps <X3D><head>...</head><Scene>...</Scene></X3D>; ROUTEs
//     and IMPORT/EXPORT are emitted as Scene children.
//
// Public declaration, namespace x3d::codec. The implementation is compiled in
// x3d_cpp_authoring_runtime.
#ifndef X3D_XML_WRITER_HPP
#define X3D_XML_WRITER_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::nodes {
class Script;
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
struct FieldInfo;
} // namespace x3d::core

namespace x3d::xml {
struct Element;
} // namespace x3d::xml

namespace x3d::codec {
using namespace x3d::core;
using x3d::nodes::Script;
using x3d::nodes::X3DNode;

/// Serializes the runtime document model to X3D-XML.
class XmlWriter {
public:
  /// Serialize a full document to an X3D-XML string (with <?xml?> prolog).
  std::string writeDocument(const runtime::X3DDocument &doc);

  /// Serialize just a Scene's children/routes (no <X3D> wrapper).
  std::string writeScene(const runtime::Scene &scene);

  /// Serialize a single node subtree (no Scene/X3D wrapper). Useful for tests.
  std::string writeNode(const std::shared_ptr<X3DNode> &node);

private:
  // Identity set of nodes already written with a DEF (for USE on revisits).
  std::unordered_set<const X3DNode *> seen_;
  // Cache of fresh "default" instances per node type, for default elision.
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  // Scene being written, for expandedSources lookup at node-emit sites. Null
  // when serializing a bare node (writeNode) — expansion round-trip is then
  // off.
  const runtime::Scene *scene_ = nullptr;
  // PRF-2: while re-emitting a ProtoBody, the body's IsConnection list so that
  // writeNodeElement can attach an <IS> block to EVERY emitted body node at any
  // depth (not just the top body node). Null outside ProtoBody re-emit.
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;

  void writeSceneInto(xml::Element *scene, const runtime::Scene &s);

  /// Return the fresh "all-defaults" instance for a node type (cached).
  X3DNode *defaultFor(const std::string &typeName);

  /// Build the XML element for one node, recursing into node fields.
  /// `containerOverride` is the containerField the parent placed this child in;
  /// if it differs from the node's default container, emit a containerField
  /// attr.
  std::unique_ptr<xml::Element>
  writeNodeElement(const std::shared_ptr<X3DNode> &node,
                   const std::string &containerOverride);

  /// Emit a Script's captured author fields as <field name accessType type
  /// [value]/> children, and its inline source as the element's CDATA text. The
  /// author FieldInfos come from the DynamicFieldStore (effectiveFields minus
  /// the node's static table); each readable inputOutput/initializeOnly field
  /// emits its current boxed value, mirroring writeProtoFieldElement. An empty
  /// sourceCode leaves the element bodyless (a url-only Script is unchanged).
  void writeScriptAuthorFields(xml::Element &el, const Script &script);

  /// Recurse an SFNode (single child) or MFNode (list) field into child els.
  /// `f.containerField` is the parent's slot name for this child. We always
  /// stamp the child with that slot as its containerField attribute so the
  /// reader can place it back unambiguously by matching attribute -> field,
  /// purely from reflection (no per-node default-container table needed).
  void writeNodeField(xml::Element &parent,
                      const std::shared_ptr<X3DNode> &owner,
                      const FieldInfo &f);

  /// Re-emit a captured <ProtoInstance> for an expanded primary node. Scalar
  /// fieldValue overrides become `<fieldValue name=... value=.../>`;
  /// node-valued overrides nest the child node(s). The scalar value's wire type
  /// is taken from the (extern)declaration interface (a ProtoFieldValue carries
  /// only a type-erased std::any); a value whose type is unknown is emitted
  /// nameless of value rather than mis-formatted.
  std::unique_ptr<xml::Element>
  writeProtoInstanceElement(const runtime::ProtoInstance &src,
                            const std::string &containerOverride);

  /// Emit a <field name type accessType [value]> element; node-typed defaults
  /// nest their default child node(s).
  std::unique_ptr<xml::Element>
  writeProtoFieldElement(const runtime::ProtoField &f);

  std::unique_ptr<xml::Element>
  writeProtoDeclareElement(const runtime::ProtoDeclaration &d);

  std::unique_ptr<xml::Element>
  writeExternProtoDeclareElement(const runtime::ExternProtoDeclaration &d);

  /// Append <IS><connect nodeField protoField/></IS> for every IsConnection
  /// whose body node matches `node` (by identity) onto its emitted element.
  static void attachIsBlocks(xml::Element &nodeEl,
                             const std::shared_ptr<X3DNode> &node,
                             const std::vector<runtime::IsConnection> &isc);

  /// Look up a ProtoInstance's interface field by name in its resolved
  /// (extern)declaration, for the field's wire type. Null if not found.
  static const runtime::ProtoField *
  interfaceField(const runtime::ProtoInstance &src, const std::string &name);

  // ---- rendering ----
  static void indent(std::ostringstream &os, int depth);

  void render(const xml::Element &el, std::ostringstream &os, int depth);
};

} // namespace x3d::codec

#endif // X3D_XML_WRITER_HPP
