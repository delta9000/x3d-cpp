// ParticleSystem.cpp
#include "x3d/nodes/ParticleSystem.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ParticleSystem::nodeTypeName() const { return "ParticleSystem"; }

std::string ParticleSystem::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ParticleSystem::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "appearance", X3DFieldType::SFNode, AccessType::InputOutput,
        "appearance",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n)
                              .X3DShapeNode::getAppearance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DShapeNode::setAppearance(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n)
                              .X3DShapeNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n)
              .X3DShapeNode::setBboxCenterUnchecked(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n)
                              .X3DShapeNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DShapeNode::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n)
                              .X3DShapeNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DShapeNode::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "castShadow", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n)
                              .X3DShapeNode::getCastShadow());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DShapeNode::setCastShadow(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFNode, AccessType::InitializeOnly, "color",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setColorUnchecked(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "colorKey", X3DFieldType::MFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getColorKey());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setColorKeyUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "createParticles", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getCreateParticles());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setCreateParticles(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emitter", X3DFieldType::SFNode, AccessType::InitializeOnly, "emitter",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n).getEmitter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setEmitterUnchecked(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometry", X3DFieldType::SFNode, AccessType::InputOutput, "geometry",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setGeometry(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometryType", X3DFieldType::SFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getGeometryType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setGeometryTypeUnchecked(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ParticleSystem &>(n).getIsActive());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ParticleSystem &>(n).emitIsActive(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "lifetimeVariation", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getLifetimeVariation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setLifetimeVariationUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxParticles", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getMaxParticles());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setMaxParticlesUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "particleLifetime", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getParticleLifetime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setParticleLifetimeUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "particleSize", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getParticleSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setParticleSizeUnchecked(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "physics", X3DFieldType::MFNode, AccessType::InitializeOnly, "physics",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ParticleSystem &>(n).getPhysics());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setPhysicsUnchecked(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"texCoord", X3DFieldType::SFNode, AccessType::InitializeOnly,
                  "texCoord",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ParticleSystem &>(n).getTexCoord());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ParticleSystem &>(n).setTexCoordUnchecked(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "texCoordKey", X3DFieldType::MFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).getTexCoordKey());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).setTexCoordKeyUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ParticleSystem &>(n)
                                        .X3DShapeNode::getVisible());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ParticleSystem &>(n).X3DShapeNode::setVisible(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ParticleSystem &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ParticleSystem &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ParticleSystem::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ParticleSystem::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesLifetimeVariation(getLifetimeVariation(), nodeTypeName(), "", out);

  checkRangesMaxParticles(getMaxParticles(), nodeTypeName(), "", out);

  checkRangesParticleLifetime(getParticleLifetime(), nodeTypeName(), "", out);

  checkRangesParticleSize(getParticleSize(), nodeTypeName(), "", out);
}

void ParticleSystem::checkRangesLifetimeVariation(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "lifetimeVariation",
                                  "lifetimeVariation below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "lifetimeVariation",
                                  "lifetimeVariation above maximum of 1"});
}

void ParticleSystem::checkRangesMaxParticles(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "maxParticles",
                                  "maxParticles below minimum of 0"});
}

void ParticleSystem::checkRangesParticleLifetime(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "particleLifetime",
                                  "particleLifetime below minimum of 0"});
}

void ParticleSystem::checkRangesParticleSize(
    const SFVec2f &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.x < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "particleSize",
                                  "particleSize.x below minimum of 0"});

  if (value.y < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "particleSize",
                                  "particleSize.y below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createParticleSystem() {
  return std::make_shared<ParticleSystem>();
}
} // namespace factory_detail

} // namespace x3d::nodes
