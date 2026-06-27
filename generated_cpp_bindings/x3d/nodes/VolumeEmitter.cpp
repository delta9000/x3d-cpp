// VolumeEmitter.cpp
#include "x3d/nodes/VolumeEmitter.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string VolumeEmitter::nodeTypeName() const { return "VolumeEmitter"; }

std::string VolumeEmitter::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &VolumeEmitter::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "coord", X3DFieldType::SFNode, AccessType::InputOutput, "coord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const VolumeEmitter &>(n).getCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).setCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coordIndex", X3DFieldType::MFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).getCoordIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).setCoordIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "direction", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).getDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).setDirectionUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "internal", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const VolumeEmitter &>(n).getInternal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).setInternalUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"mass", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const VolumeEmitter &>(n)
                                        .X3DParticleEmitterNode::getMass());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<VolumeEmitter &>(n)
                        .X3DParticleEmitterNode::setMassUnchecked(
                            std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const VolumeEmitter &>(n)
                              .X3DParticleEmitterNode::getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DParticleEmitterNode::setOn(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_coordIndex", X3DFieldType::MFInt32,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<VolumeEmitter &>(n).onSet_coordIndex(
                                std::any_cast<MFInt32>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"speed", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const VolumeEmitter &>(n)
                                        .X3DParticleEmitterNode::getSpeed());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<VolumeEmitter &>(n)
                        .X3DParticleEmitterNode::setSpeedUnchecked(
                            std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "surfaceArea", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const VolumeEmitter &>(n)
                              .X3DParticleEmitterNode::getSurfaceArea());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n)
              .X3DParticleEmitterNode::setSurfaceAreaUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "variation", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const VolumeEmitter &>(n)
                              .X3DParticleEmitterNode::getVariation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n)
              .X3DParticleEmitterNode::setVariationUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const VolumeEmitter &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<VolumeEmitter &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void VolumeEmitter::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void VolumeEmitter::validateRanges(std::vector<RangeDiagnostic> &out) const {

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

void VolumeEmitter::checkRangesDirection(const SFVec3f &value,
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

} // namespace x3d::nodes
