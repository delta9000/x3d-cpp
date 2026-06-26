// HAnimHumanoid.cpp
#include "HAnimHumanoid.hpp"

std::string HAnimHumanoid::nodeTypeName() const { return "HAnimHumanoid"; }

std::string HAnimHumanoid::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &HAnimHumanoid::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n)
              .X3DBoundedObject::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n)
              .X3DBoundedObject::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "center", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setCenter(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "info", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getInfo());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setInfo(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"jointBindingPositions", X3DFieldType::MFVec3f,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                                        .getJointBindingPositions());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimHumanoid &>(n).setJointBindingPositions(
                        std::any_cast<MFVec3f>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"jointBindingRotations", X3DFieldType::MFRotation,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                                        .getJointBindingRotations());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimHumanoid &>(n).setJointBindingRotations(
                        std::any_cast<MFRotation>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "jointBindingScales", X3DFieldType::MFVec3f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getJointBindingScales());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setJointBindingScales(
              std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "joints", X3DFieldType::MFNode, AccessType::InputOutput, "joints",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getJoints());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setJoints(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "loa", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getLoa());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setLoaUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motions", X3DFieldType::MFNode, AccessType::InputOutput, "motions",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getMotions());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setMotions(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motionsEnabled", X3DFieldType::MFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getMotionsEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setMotionsEnabled(
              std::any_cast<MFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "name", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getName());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setName(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rotation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setRotation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scale", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setScale(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scaleOrientation", X3DFieldType::SFRotation, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getScaleOrientation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setScaleOrientation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "segments", X3DFieldType::MFNode, AccessType::InputOutput, "segments",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getSegments());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSegments(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sites", X3DFieldType::MFNode, AccessType::InputOutput, "sites",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getSites());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSites(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"skeletalConfiguration", X3DFieldType::SFString,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                                        .getSkeletalConfiguration());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimHumanoid &>(n).setSkeletalConfiguration(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "skeleton", X3DFieldType::MFNode, AccessType::InputOutput, "skeleton",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getSkeleton());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSkeleton(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skin", X3DFieldType::MFNode, AccessType::InputOutput, "skin",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getSkin());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSkin(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skinBindingCoords", X3DFieldType::SFNode, AccessType::InputOutput,
        "skinBindingCoords",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getSkinBindingCoords());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSkinBindingCoords(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skinBindingNormals", X3DFieldType::SFNode, AccessType::InputOutput,
        "skinBindingNormals",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getSkinBindingNormals());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSkinBindingNormals(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skinCoord", X3DFieldType::SFNode, AccessType::InputOutput, "skinCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getSkinCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setSkinCoord(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"skinNormal", X3DFieldType::SFNode, AccessType::InputOutput,
                  "skinNormal",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimHumanoid &>(n).getSkinNormal());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimHumanoid &>(n).setSkinNormal(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "translation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).getTranslation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setTranslation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "version", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n).getVersion());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).setVersion(
              std::any_cast<HanimVersionChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const HAnimHumanoid &>(n).getVersion());
        },

        [](X3DNode &n, const std::string &s) {
          HanimVersionChoices ev;
          if (from_string(s, ev))
            dynamic_cast<HAnimHumanoid &>(n).setVersion(ev);
        }

    });

    t.push_back(
        FieldInfo{"viewpoints", X3DFieldType::MFNode, AccessType::InputOutput,
                  "viewpoints",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimHumanoid &>(n).getViewpoints());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimHumanoid &>(n).setViewpoints(
                        std::any_cast<MFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimHumanoid &>(n)
                              .X3DBoundedObject::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DBoundedObject::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimHumanoid &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimHumanoid &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void HAnimHumanoid::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void HAnimHumanoid::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesLoa(getLoa(), nodeTypeName(), "", out);
}

void HAnimHumanoid::checkRangesLoa(const SFInt32 &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out) {
  if (value < -1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "loa", "loa below minimum of -1"});
  if (value > 4)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "loa", "loa above maximum of 4"});
}
