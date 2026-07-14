// Sound.cpp
#include "x3d/nodes/Sound.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Sound::nodeTypeName() const { return "Sound"; }

std::string Sound::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Sound::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Sound &>(n).X3DSoundNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DSoundNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "direction", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setDirection(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Sound &>(n).X3DSoundNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DSoundNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "intensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "location", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setLocation(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxBack", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getMaxBack());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setMaxBackUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxFront", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getMaxFront());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setMaxFrontUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Sound &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minBack", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getMinBack());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setMinBackUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minFront", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getMinFront());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setMinFrontUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "priority", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getPriority());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setPriorityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "source", X3DFieldType::SFNode, AccessType::InputOutput, "source",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getSource());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setSource(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "spatialize", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).getSpatialize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).setSpatializeUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Sound &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Sound &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Sound::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Sound::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesIntensity(getIntensity(), nodeTypeName(), "", out);

  checkRangesMaxBack(getMaxBack(), nodeTypeName(), "", out);

  checkRangesMaxFront(getMaxFront(), nodeTypeName(), "", out);

  checkRangesMinBack(getMinBack(), nodeTypeName(), "", out);

  checkRangesMinFront(getMinFront(), nodeTypeName(), "", out);

  checkRangesPriority(getPriority(), nodeTypeName(), "", out);
}

void Sound::checkRangesIntensity(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "intensity",
                                  "intensity below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "intensity",
                                  "intensity above maximum of 1"});
}

void Sound::checkRangesMaxBack(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "maxBack",
                                  "maxBack below minimum of 0"});
}

void Sound::checkRangesMaxFront(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "maxFront",
                                  "maxFront below minimum of 0"});
}

void Sound::checkRangesMinBack(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "minBack",
                                  "minBack below minimum of 0"});
}

void Sound::checkRangesMinFront(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "minFront",
                                  "minFront below minimum of 0"});
}

void Sound::checkRangesPriority(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "priority",
                                  "priority below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "priority",
                                  "priority above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createSound() { return std::make_shared<Sound>(); }
} // namespace factory_detail

} // namespace x3d::nodes
