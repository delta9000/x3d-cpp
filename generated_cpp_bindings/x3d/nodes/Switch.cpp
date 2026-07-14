// Switch.cpp
#include "x3d/nodes/Switch.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Switch::nodeTypeName() const { return "Switch"; }

std::string Switch::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Switch::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"addChildren", X3DFieldType::MFNode, AccessType::InputOnly,
                  "addChildren",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Switch &>(n).X3DGroupingNode::onAddChildren(
                        std::any_cast<MFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Switch &>(n).X3DGroupingNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DGroupingNode::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n)
                              .X3DGroupingNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DGroupingNode::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Switch &>(n).X3DGroupingNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DGroupingNode::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Switch &>(n).X3DGroupingNode::getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DGroupingNode::setChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Switch &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"removeChildren", X3DFieldType::MFNode, AccessType::InputOnly,
                  "removeChildren",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Switch &>(n).X3DGroupingNode::onRemoveChildren(
                        std::any_cast<MFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Switch &>(n).X3DGroupingNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DGroupingNode::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "whichChoice", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).getWhichChoice());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).setWhichChoiceUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Switch &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Switch &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Switch::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Switch::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesWhichChoice(getWhichChoice(), nodeTypeName(), "", out);
}

void Switch::checkRangesWhichChoice(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "whichChoice",
                                  "whichChoice below minimum of -1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createSwitch() { return std::make_shared<Switch>(); }
} // namespace factory_detail

} // namespace x3d::nodes
