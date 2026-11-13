// Shared scene-graph construction for the ordinary and canonical XML renderers.
#ifndef X3D_XML_GRAPH_BUILDER_HPP
#define X3D_XML_GRAPH_BUILDER_HPP

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace x3d::nodes {
class X3DNode;
}
namespace x3d::core {
enum class X3DFieldType;
struct FieldInfo;
} // namespace x3d::core
namespace x3d::runtime {
class DynamicFieldStore;
struct ExternProtoDeclaration;
struct IsConnection;
struct ProtoBody;
struct ProtoDeclaration;
struct ProtoField;
struct Unit;
class ProtoInstance;
class Scene;
class AuthoredScalarFields;
} // namespace x3d::runtime
namespace x3d::xml {
struct Element;
}

namespace x3d::codec {

struct XmlGraphPolicy {
  // The same formatter compares factory defaults and emits retained values.
  std::string (*formatValue)(core::X3DFieldType, const std::any &);
  bool authoredChildOrder;
  bool sortAttributes;
};

class XmlGraphBuilder {
public:
  explicit XmlGraphBuilder(
      XmlGraphPolicy policy,
      const runtime::DynamicFieldStore *authorFields = nullptr)
      : policy_(policy), authorFields_(authorFields) {}

  void writeSceneInto(xml::Element *element, const runtime::Scene &scene,
                      const std::vector<runtime::Unit> *units = nullptr);
  std::unique_ptr<xml::Element>
  writeNodeElement(const std::shared_ptr<nodes::X3DNode> &node,
                   const std::string &containerOverride);
  void orderAttributes(xml::Element &element) const;

private:
  XmlGraphPolicy policy_;
  std::unordered_set<const nodes::X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<nodes::X3DNode>> defaults_;
  const runtime::Scene *scene_ = nullptr;
  const runtime::AuthoredScalarFields *unitDefaults_ = nullptr;
  const runtime::DynamicFieldStore *authorFields_ = nullptr;
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;
  const runtime::ProtoBody *bodyOrder_ = nullptr;

  nodes::X3DNode *defaultFor(const std::string &typeName);
  void writeAuthorFields(xml::Element &element, const nodes::X3DNode &node);
  void writeNodeField(xml::Element &parent,
                      const std::shared_ptr<nodes::X3DNode> &owner,
                      const core::FieldInfo &field);
  std::unique_ptr<xml::Element>
  writeProtoInstanceElement(const runtime::ProtoInstance &source,
                            const std::string &containerOverride);
  std::unique_ptr<xml::Element>
  writeProtoFieldElement(const runtime::ProtoField &field);
  std::unique_ptr<xml::Element>
  writeProtoDeclareElement(const runtime::ProtoDeclaration &declaration);
  std::unique_ptr<xml::Element> writeExternProtoDeclareElement(
      const runtime::ExternProtoDeclaration &declaration);
  static void
  attachIsBlocks(xml::Element &element,
                 const std::shared_ptr<nodes::X3DNode> &node,
                 const std::vector<runtime::IsConnection> &connections);
};

} // namespace x3d::codec
#endif // X3D_XML_GRAPH_BUILDER_HPP
