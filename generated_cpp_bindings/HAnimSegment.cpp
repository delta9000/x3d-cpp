// HAnimSegment.cpp
#include "HAnimSegment.hpp"

std::string HAnimSegment::nodeTypeName() const { return "HAnimSegment"; }

std::string HAnimSegment::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &HAnimSegment::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "addChildren", X3DFieldType::MFNode, AccessType::InputOnly,
        "addChildren",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DGroupingNode::onAddChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n)
                              .X3DGroupingNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n)
              .X3DGroupingNode::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n)
                              .X3DGroupingNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DGroupingNode::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n)
                              .X3DGroupingNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DGroupingNode::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "centerOfMass", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).getCenterOfMass());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).setCenterOfMass(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n)
                              .X3DGroupingNode::getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DGroupingNode::setChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coord", X3DFieldType::SFNode, AccessType::InputOutput, "coord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n).getCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).setCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"displacers", X3DFieldType::MFNode, AccessType::InputOutput,
                  "displacers",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimSegment &>(n).getDisplacers());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimSegment &>(n).setDisplacers(
                        std::any_cast<MFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimSegment &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimSegment &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "mass", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n).getMass());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).setMassUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "momentsOfInertia", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).getMomentsOfInertia());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).setMomentsOfInertiaUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "name", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimSegment &>(n).getName());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).setName(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "removeChildren", X3DFieldType::MFNode, AccessType::InputOnly,
        "removeChildren",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DGroupingNode::onRemoveChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const HAnimSegment &>(n)
                                        .X3DGroupingNode::getVisible());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimSegment &>(n).X3DGroupingNode::setVisible(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimSegment &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimSegment &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimSegment &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimSegment &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void HAnimSegment::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void HAnimSegment::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesMass(getMass(), nodeTypeName(), "", out);

  checkRangesMomentsOfInertia(getMomentsOfInertia(), nodeTypeName(), "", out);
}

void HAnimSegment::checkRangesMass(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "mass", "mass below minimum of 0"});
}

void HAnimSegment::checkRangesMomentsOfInertia(
    const MFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "momentsOfInertia",
                                    "momentsOfInertia below minimum of 0"});
  }
}
