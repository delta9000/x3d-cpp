// PointEmitter.cpp
#include "x3d/nodes/PointEmitter.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PointEmitter::nodeTypeName() const { return "PointEmitter"; }

std::string PointEmitter::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PointEmitter::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "direction", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointEmitter &>(n).getDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).setDirection(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PointEmitter &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PointEmitter &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"mass", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const PointEmitter &>(n)
                                        .X3DParticleEmitterNode::getMass());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PointEmitter &>(n)
                        .X3DParticleEmitterNode::setMassUnchecked(
                            std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointEmitter &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointEmitter &>(n)
                              .X3DParticleEmitterNode::getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).X3DParticleEmitterNode::setOn(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "position", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointEmitter &>(n).getPosition());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).setPosition(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"speed", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const PointEmitter &>(n)
                                        .X3DParticleEmitterNode::getSpeed());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PointEmitter &>(n)
                        .X3DParticleEmitterNode::setSpeedUnchecked(
                            std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "surfaceArea", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointEmitter &>(n)
                              .X3DParticleEmitterNode::getSurfaceArea());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n)
              .X3DParticleEmitterNode::setSurfaceAreaUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "variation", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointEmitter &>(n)
                              .X3DParticleEmitterNode::getVariation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n)
              .X3DParticleEmitterNode::setVariationUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointEmitter &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointEmitter &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointEmitter &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PointEmitter &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PointEmitter &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointEmitter &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointEmitter &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PointEmitter::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void PointEmitter::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DParticleEmitterNode::checkRangesMass(X3DParticleEmitterNode::getMass(),
                                          nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesSpeed(X3DParticleEmitterNode::getSpeed(),
                                           nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesSurfaceArea(
      X3DParticleEmitterNode::getSurfaceArea(), nodeTypeName(), "", out);

  X3DParticleEmitterNode::checkRangesVariation(
      X3DParticleEmitterNode::getVariation(), nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPointEmitter() {
  return std::make_shared<PointEmitter>();
}
} // namespace factory_detail

} // namespace x3d::nodes
