// ConeEmitter.cpp
#include "ConeEmitter.hpp"

std::string ConeEmitter::nodeTypeName() const { return "ConeEmitter"; }

std::string ConeEmitter::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ConeEmitter::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "angle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ConeEmitter &>(n).getAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).setAngleUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "direction", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ConeEmitter &>(n).getDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).setDirectionUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ConeEmitter &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ConeEmitter &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"mass", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ConeEmitter &>(n)
                                        .X3DParticleEmitterNode::getMass());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ConeEmitter &>(n)
                        .X3DParticleEmitterNode::setMassUnchecked(
                            std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ConeEmitter &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ConeEmitter &>(n)
                              .X3DParticleEmitterNode::getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).X3DParticleEmitterNode::setOn(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "position", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ConeEmitter &>(n).getPosition());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).setPosition(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"speed", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ConeEmitter &>(n)
                                        .X3DParticleEmitterNode::getSpeed());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ConeEmitter &>(n)
                        .X3DParticleEmitterNode::setSpeedUnchecked(
                            std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "surfaceArea", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ConeEmitter &>(n)
                              .X3DParticleEmitterNode::getSurfaceArea());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n)
              .X3DParticleEmitterNode::setSurfaceAreaUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "variation", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ConeEmitter &>(n)
                              .X3DParticleEmitterNode::getVariation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n)
              .X3DParticleEmitterNode::setVariationUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ConeEmitter &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ConeEmitter &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ConeEmitter &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ConeEmitter &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ConeEmitter &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ConeEmitter &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ConeEmitter &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ConeEmitter &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ConeEmitter &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ConeEmitter::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ConeEmitter::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAngle(getAngle(), nodeTypeName(), "", out);

  checkRangesDirection(getDirection(), nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesMass(X3DParticleEmitterNode::getMass(),
                                          nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesSpeed(X3DParticleEmitterNode::getSpeed(),
                                           nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesSurfaceArea(
      X3DParticleEmitterNode::getSurfaceArea(), nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesVariation(
      X3DParticleEmitterNode::getVariation(), nodeTypeName(), "", out);
}

void ConeEmitter::checkRangesAngle(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "angle",
                                  "angle below minimum of 0"});
  if (value > 3.1416)
    out.push_back(RangeDiagnostic{nodeType, defName, "angle",
                                  "angle above maximum of 3.1416"});
}

void ConeEmitter::checkRangesDirection(const SFVec3f &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out) {
  if (value.x < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "direction",
                                  "direction.x below minimum of -1"});
  if (value.x > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "direction",
                                  "direction.x above maximum of 1"});

  if (value.y < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "direction",
                                  "direction.y below minimum of -1"});
  if (value.y > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "direction",
                                  "direction.y above maximum of 1"});

  if (value.z < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "direction",
                                  "direction.z below minimum of -1"});
  if (value.z > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "direction",
                                  "direction.z above maximum of 1"});
}
