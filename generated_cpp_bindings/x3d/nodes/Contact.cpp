// Contact.cpp
#include "x3d/nodes/Contact.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Contact::nodeTypeName() const { return "Contact"; }

std::string Contact::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Contact::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "appliedParameters", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).getAppliedParameters());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setAppliedParameters(
              std::any_cast<std::vector<AppliedParametersChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec =
              dynamic_cast<const Contact &>(n).getAppliedParameters();
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
          std::size_t i = 0;
          while (i < s.size()) {
            while (i < s.size() &&
                   (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' ||
                    s[i] == '\r' || s[i] == ','))
              ++i;
            std::size_t j = i;
            while (j < s.size() && s[j] != ' ' && s[j] != '\t' &&
                   s[j] != '\n' && s[j] != '\r' && s[j] != ',')
              ++j;
            if (j > i) {
              AppliedParametersChoices ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<Contact &>(n).setAppliedParameters(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "body1", X3DFieldType::SFNode, AccessType::InputOutput, "body1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getBody1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setBody1(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2", X3DFieldType::SFNode, AccessType::InputOutput, "body2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getBody2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setBody2(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getBounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setBounceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "contactNormal", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getContactNormal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setContactNormal(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "depth", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getDepth());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setDepth(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frictionCoefficients", X3DFieldType::SFVec2f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).getFrictionCoefficients());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setFrictionCoefficientsUnchecked(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frictionDirection", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).getFrictionDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setFrictionDirection(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometry1", X3DFieldType::SFNode, AccessType::InputOutput, "geometry1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getGeometry1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setGeometry1(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometry2", X3DFieldType::SFNode, AccessType::InputOutput, "geometry2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getGeometry2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setGeometry2(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minBounceSpeed", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getMinBounceSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setMinBounceSpeedUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "position", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getPosition());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setPosition(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "slipCoefficients", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).getSlipCoefficients());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setSlipCoefficients(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "softnessConstantForceMix", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).getSoftnessConstantForceMix());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setSoftnessConstantForceMixUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "softnessErrorCorrection", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Contact &>(n).getSoftnessErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setSoftnessErrorCorrectionUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceSpeed", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).getSurfaceSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).setSurfaceSpeed(std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Contact &>(n).X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Contact &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Contact &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Contact &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Contact::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Contact::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesBounce(getBounce(), nodeTypeName(), "", out);

  checkRangesFrictionCoefficients(getFrictionCoefficients(), nodeTypeName(), "",
                                  out);

  checkRangesMinBounceSpeed(getMinBounceSpeed(), nodeTypeName(), "", out);

  checkRangesSoftnessConstantForceMix(getSoftnessConstantForceMix(),
                                      nodeTypeName(), "", out);

  checkRangesSoftnessErrorCorrection(getSoftnessErrorCorrection(),
                                     nodeTypeName(), "", out);
}

void Contact::checkRangesBounce(const SFFloat &value,
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

void Contact::checkRangesFrictionCoefficients(
    const SFVec2f &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.x < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frictionCoefficients",
                                  "frictionCoefficients.x below minimum of 0"});

  if (value.y < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frictionCoefficients",
                                  "frictionCoefficients.y below minimum of 0"});
}

void Contact::checkRangesMinBounceSpeed(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "minBounceSpeed",
                                  "minBounceSpeed below minimum of 0"});
}

void Contact::checkRangesSoftnessConstantForceMix(
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

void Contact::checkRangesSoftnessErrorCorrection(
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
std::shared_ptr<X3DNode> createContact() { return std::make_shared<Contact>(); }
} // namespace factory_detail

} // namespace x3d::nodes
