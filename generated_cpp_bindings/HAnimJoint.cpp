// HAnimJoint.cpp
#include "HAnimJoint.hpp"

std::string HAnimJoint::nodeTypeName() const { return "HAnimJoint"; }

std::string HAnimJoint::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &HAnimJoint::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{"addChildren", X3DFieldType::MFNode,
                          AccessType::InputOnly, "addChildren",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<HAnimJoint &>(n).onAddChildren(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n)
              .X3DBoundedObject::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DBoundedObject::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "center", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setCenter(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "displacers", X3DFieldType::MFNode, AccessType::InputOutput,
        "displacers",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getDisplacers());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setDisplacers(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "limitOrientation", X3DFieldType::SFRotation, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).getLimitOrientation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setLimitOrientation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "llimit", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getLlimit());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setLlimit(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "name", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getName());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setName(
              std::any_cast<HanimJointNameValues>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const HAnimJoint &>(n).getName());
        },

        [](X3DNode &n, const std::string &s) {
          HanimJointNameValues ev;
          if (from_string(s, ev))
            dynamic_cast<HAnimJoint &>(n).setName(ev);
        }

    });

    t.push_back(FieldInfo{"removeChildren", X3DFieldType::MFNode,
                          AccessType::InputOnly, "removeChildren",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<HAnimJoint &>(n).onRemoveChildren(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rotation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setRotation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scale", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setScale(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scaleOrientation", X3DFieldType::SFRotation, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).getScaleOrientation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setScaleOrientation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skinCoordIndex", X3DFieldType::MFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).getSkinCoordIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setSkinCoordIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skinCoordWeight", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).getSkinCoordWeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setSkinCoordWeight(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stiffness", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getStiffness());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setStiffnessUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "translation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getTranslation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setTranslation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "ulimit", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).getUlimit());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).setUlimit(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const HAnimJoint &>(n)
                                        .X3DBoundedObject::getVisible());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimJoint &>(n).X3DBoundedObject::setVisible(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimJoint &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimJoint &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimJoint &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimJoint &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimJoint &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimJoint &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimJoint &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void HAnimJoint::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void HAnimJoint::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesSkinCoordIndex(getSkinCoordIndex(), nodeTypeName(), "", out);

  checkRangesStiffness(getStiffness(), nodeTypeName(), "", out);
}

void HAnimJoint::checkRangesSkinCoordIndex(const MFInt32 &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "skinCoordIndex",
                                    "skinCoordIndex below minimum of 0"});
  }
}

void HAnimJoint::checkRangesStiffness(const MFFloat &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "stiffness",
                                    "stiffness below minimum of 0"});
    if (v > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "stiffness",
                                    "stiffness above maximum of 1"});
  }
}
