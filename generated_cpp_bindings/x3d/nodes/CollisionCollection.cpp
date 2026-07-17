// CollisionCollection.cpp
#include "x3d/nodes/CollisionCollection.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string CollisionCollection::nodeTypeName() const {
  return "CollisionCollection";
}

std::string CollisionCollection::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &CollisionCollection::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "appliedParameters", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .getAppliedParameters());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setAppliedParameters(
              std::any_cast<std::vector<AppliedParametersChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec = dynamic_cast<const CollisionCollection &>(n)
                                .getAppliedParameters();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<AppliedParametersChoices> vec;
          for (const auto &tok : parseEnumTokens(s)) {
            AppliedParametersChoices ev;
            if (from_string(tok, ev))
              vec.push_back(ev);
          }
          dynamic_cast<CollisionCollection &>(n).setAppliedParameters(
              std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n)
              .X3DBoundedObject::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n)
              .X3DBoundedObject::setBboxDisplay(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n)
              .X3DBoundedObject::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getBounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setBounceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "collidables", X3DFieldType::MFNode, AccessType::InputOutput,
        "collidables",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getCollidables());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setCollidables(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frictionCoefficients", X3DFieldType::SFVec2f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .getFrictionCoefficients());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n)
              .setFrictionCoefficientsUnchecked(std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minBounceSpeed", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getMinBounceSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setMinBounceSpeedUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "slipFactors", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getSlipFactors());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setSlipFactors(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "softnessConstantForceMix", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .getSoftnessConstantForceMix());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n)
              .setSoftnessConstantForceMixUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "softnessErrorCorrection", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .getSoftnessErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n)
              .setSoftnessErrorCorrectionUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceSpeed", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).getSurfaceSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).setSurfaceSpeed(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionCollection &>(n)
                              .X3DBoundedObject::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DBoundedObject::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const CollisionCollection &>(n)
                                        .X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<CollisionCollection &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionCollection &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionCollection &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void CollisionCollection::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void CollisionCollection::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesBounce(getBounce(), nodeTypeName(), "", out);

  checkRangesFrictionCoefficients(getFrictionCoefficients(), nodeTypeName(), "",
                                  out);

  checkRangesMinBounceSpeed(getMinBounceSpeed(), nodeTypeName(), "", out);

  checkRangesSoftnessConstantForceMix(getSoftnessConstantForceMix(),
                                      nodeTypeName(), "", out);

  checkRangesSoftnessErrorCorrection(getSoftnessErrorCorrection(),
                                     nodeTypeName(), "", out);
}

void CollisionCollection::checkRangesBounce(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "bounce",
                                  "bounce below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "bounce",
                                  "bounce above maximum of 1"});
}

void CollisionCollection::checkRangesFrictionCoefficients(
    const SFVec2f &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.x < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frictionCoefficients",
                                  "frictionCoefficients.x below minimum of 0"});

  if (value.y < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frictionCoefficients",
                                  "frictionCoefficients.y below minimum of 0"});
}

void CollisionCollection::checkRangesMinBounceSpeed(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "minBounceSpeed",
                                  "minBounceSpeed below minimum of 0"});
}

void CollisionCollection::checkRangesSoftnessConstantForceMix(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "softnessConstantForceMix",
                        "softnessConstantForceMix below minimum of 0"});
  if (value > 1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "softnessConstantForceMix",
                        "softnessConstantForceMix above maximum of 1"});
}

void CollisionCollection::checkRangesSoftnessErrorCorrection(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "softnessErrorCorrection",
                        "softnessErrorCorrection below minimum of 0"});
  if (value > 1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "softnessErrorCorrection",
                        "softnessErrorCorrection above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createCollisionCollection() {
  return std::make_shared<CollisionCollection>();
}
} // namespace factory_detail

} // namespace x3d::nodes
