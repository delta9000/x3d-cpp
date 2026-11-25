#include "x3d/nodes/Tangent.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Tangent::nodeTypeName() const { return "Tangent"; }

std::string Tangent::defaultContainerField() const {
  return getDefaultContainerField();
}

X3DVersion Tangent::introducedIn() const { return X3DVersion{4, 1}; }

const FieldTable &Tangent::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Tangent &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Tangent &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getMetadata(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "vector", X3DFieldType::MFVec4f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Tangent &>(n).getVector());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).setVectorUnchecked(
              std::any_cast<MFVec4f>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).getVector(),
                  &typeid(MFVec4f)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Tangent &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Tangent &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Tangent &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getClass_(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Tangent &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Tangent &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Tangent &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const Tangent &>(n).X3DNode::getStyle(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    return t;
  }();
  return table;
}

void Tangent::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Tangent::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesVector(getVector(), nodeTypeName(), "", out);
}

void Tangent::checkRangesVector(const MFVec4f &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v.x < -1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.x below minimum of -1"});
    if (v.x > 1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.x above maximum of 1"});

    if (v.y < -1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.y below minimum of -1"});
    if (v.y > 1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.y above maximum of 1"});

    if (v.z < -1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.z below minimum of -1"});
    if (v.z > 1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.z above maximum of 1"});

    if (v.w < -1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.w below minimum of -1"});
    if (v.w > 1.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "vector",
                                    "vector.w above maximum of 1"});
  }
}

namespace factory_detail {
std::shared_ptr<X3DNode> createTangent() { return std::make_shared<Tangent>(); }
} // namespace factory_detail

} // namespace x3d::nodes
