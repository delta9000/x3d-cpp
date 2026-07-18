// SpatialSound.cpp
#include "x3d/nodes/SpatialSound.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string SpatialSound::nodeTypeName() const { return "SpatialSound"; }

std::string SpatialSound::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &SpatialSound::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coneInnerAngle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getConeInnerAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setConeInnerAngleUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coneOuterAngle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getConeOuterAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setConeOuterAngleUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coneOuterGain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getConeOuterGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setConeOuterGain(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n)
                              .X3DSoundNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DSoundNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "direction", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n).getDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setDirection(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "distanceModel", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getDistanceModel());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setDistanceModel(
              std::any_cast<DistanceModelChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const SpatialSound &>(n).getDistanceModel());
        },

        [](X3DNode &n, const std::string &s) {
          DistanceModelChoices ev;
          if (from_string(s, ev))
            dynamic_cast<SpatialSound &>(n).setDistanceModel(ev);
        }

    });

    t.push_back(FieldInfo{
        "dopplerEnabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getDopplerEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setDopplerEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).X3DSoundNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DSoundNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enableHRTF", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getEnableHRTF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setEnableHRTF(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n).getGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setGain(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "intensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n).getIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SpatialSound &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SpatialSound &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "location", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n).getLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setLocation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxDistance", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getMaxDistance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setMaxDistanceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "priority", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SpatialSound &>(n).getPriority());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setPriorityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "referenceDistance", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getReferenceDistance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setReferenceDistanceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rolloffFactor", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getRolloffFactor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setRolloffFactorUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "spatialize", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).getSpatialize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).setSpatializeUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SpatialSound &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SpatialSound &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SpatialSound &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SpatialSound &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void SpatialSound::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void SpatialSound::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesConeInnerAngle(getConeInnerAngle(), nodeTypeName(), "", out);

  checkRangesConeOuterAngle(getConeOuterAngle(), nodeTypeName(), "", out);

  checkRangesIntensity(getIntensity(), nodeTypeName(), "", out);

  checkRangesMaxDistance(getMaxDistance(), nodeTypeName(), "", out);

  checkRangesPriority(getPriority(), nodeTypeName(), "", out);

  checkRangesReferenceDistance(getReferenceDistance(), nodeTypeName(), "", out);

  checkRangesRolloffFactor(getRolloffFactor(), nodeTypeName(), "", out);
}

void SpatialSound::checkRangesConeInnerAngle(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "coneInnerAngle",
                                  "coneInnerAngle below minimum of 0"});
  if (value > 6.2832f)
    out.push_back(RangeDiagnostic{nodeType, defName, "coneInnerAngle",
                                  "coneInnerAngle above maximum of 6.2832"});
}

void SpatialSound::checkRangesConeOuterAngle(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "coneOuterAngle",
                                  "coneOuterAngle below minimum of 0"});
  if (value > 6.2832f)
    out.push_back(RangeDiagnostic{nodeType, defName, "coneOuterAngle",
                                  "coneOuterAngle above maximum of 6.2832"});
}

void SpatialSound::checkRangesIntensity(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "intensity",
                                  "intensity below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "intensity",
                                  "intensity above maximum of 1"});
}

void SpatialSound::checkRangesMaxDistance(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "maxDistance",
                                  "maxDistance below minimum of 0"});
}

void SpatialSound::checkRangesPriority(const SFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "priority",
                                  "priority below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "priority",
                                  "priority above maximum of 1"});
}

void SpatialSound::checkRangesReferenceDistance(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "referenceDistance",
                                  "referenceDistance below minimum of 0"});
}

void SpatialSound::checkRangesRolloffFactor(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "rolloffFactor",
                                  "rolloffFactor below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createSpatialSound() {
  return std::make_shared<SpatialSound>();
}
} // namespace factory_detail

} // namespace x3d::nodes
