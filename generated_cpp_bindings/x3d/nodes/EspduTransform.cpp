// EspduTransform.cpp
#include "x3d/nodes/EspduTransform.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string EspduTransform::nodeTypeName() const { return "EspduTransform"; }

std::string EspduTransform::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &EspduTransform::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "addChildren", X3DFieldType::MFNode, AccessType::InputOnly,
        "addChildren",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DGroupingNode::onAddChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "address", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getAddress());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setAddress(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "applicationID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getApplicationID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setApplicationID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "articulationParameterArray", X3DFieldType::MFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .getArticulationParameterArray());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setArticulationParameterArray(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "articulationParameterChangeIndicatorArray", X3DFieldType::MFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .getArticulationParameterChangeIndicatorArray());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n)
              .setArticulationParameterChangeIndicatorArray(
                  std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "articulationParameterCount", X3DFieldType::SFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .getArticulationParameterCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setArticulationParameterCount(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterDesignatorArray",
                          X3DFieldType::MFInt32, AccessType::InputOutput, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterDesignatorArray());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .setArticulationParameterDesignatorArray(
                                    std::any_cast<MFInt32>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "articulationParameterIdPartAttachedToArray", X3DFieldType::MFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .getArticulationParameterIdPartAttachedToArray());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n)
              .setArticulationParameterIdPartAttachedToArray(
                  std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "articulationParameterTypeArray", X3DFieldType::MFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .getArticulationParameterTypeArray());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setArticulationParameterTypeArray(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue0_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue0_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue0_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue1_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue1_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue1_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue2_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue2_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue2_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue3_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue3_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue3_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue4_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue4_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue4_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue5_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue5_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue5_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue6_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue6_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue6_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"articulationParameterValue7_changed",
                          X3DFieldType::SFFloat, AccessType::OutputOnly, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const EspduTransform &>(n)
                                    .getArticulationParameterValue7_changed());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<EspduTransform &>(n)
                                .emitArticulationParameterValue7_changed(
                                    std::any_cast<SFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .X3DGroupingNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n)
              .X3DGroupingNode::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .X3DGroupingNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DGroupingNode::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .X3DGroupingNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n)
              .X3DGroupingNode::setBboxSizeUnchecked(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "center", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setCenter(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .X3DGroupingNode::getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DGroupingNode::setChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "collideTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getCollideTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitCollideTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "collisionType", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getCollisionType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setCollisionType(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "deadReckoning", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getDeadReckoning());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setDeadReckoning(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "detonateTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getDetonateTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitDetonateTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "detonationLocation", X3DFieldType::SFVec3f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getDetonationLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setDetonationLocation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "detonationRelativeLocation", X3DFieldType::SFVec3f,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .getDetonationRelativeLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setDetonationRelativeLocation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "detonationResult", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getDetonationResult());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setDetonationResult(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityCategory", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntityCategory());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntityCategory(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityCountry", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntityCountry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntityCountry(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityDomain", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntityDomain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntityDomain(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityExtra", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntityExtra());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntityExtra(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntityID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityKind", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntityKind());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntityKind(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entitySpecific", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntitySpecific());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntitySpecific(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entitySubcategory", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEntitySubcategory());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEntitySubcategory(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "eventApplicationID", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEventApplicationID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEventApplicationID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "eventEntityID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEventEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEventEntityID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "eventNumber", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEventNumber());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEventNumber(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "eventSiteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getEventSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setEventSiteID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fired1", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getFired1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setFired1(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fired2", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getFired2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setFired2(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"firedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const EspduTransform &>(n).getFiredTime());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<EspduTransform &>(n).emitFiredTime(
                        std::any_cast<SFTime>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "fireMissionIndex", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getFireMissionIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setFireMissionIndex(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "firingRange", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getFiringRange());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setFiringRange(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "firingRate", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getFiringRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setFiringRate(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forceID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getForceID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setForceID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fuse", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getFuse());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setFuse(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoCoords", X3DFieldType::SFVec3d, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getGeoCoords());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setGeoCoords(
              std::any_cast<SFVec3d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoSystem", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getGeoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setGeoSystemUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const EspduTransform &>(n).getIsActive());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<EspduTransform &>(n).emitIsActive(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "isCollided", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getIsCollided());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitIsCollided(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isDetonated", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getIsDetonated());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitIsDetonated(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkReader", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getIsNetworkReader());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitIsNetworkReader(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkWriter", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getIsNetworkWriter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitIsNetworkWriter(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isRtpHeaderHeard", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getIsRtpHeaderHeard());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitIsRtpHeaderHeard(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isStandAlone", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getIsStandAlone());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).emitIsStandAlone(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "linearAcceleration", X3DFieldType::SFVec3f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getLinearAcceleration());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setLinearAcceleration(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "linearVelocity", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getLinearVelocity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setLinearVelocity(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "marking", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getMarking());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMarking(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayHost", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMulticastRelayHost());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMulticastRelayHost(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayPort", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMulticastRelayPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMulticastRelayPort(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"munitionApplicationID", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const EspduTransform &>(n)
                                        .getMunitionApplicationID());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<EspduTransform &>(n).setMunitionApplicationID(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "munitionEndPoint", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMunitionEndPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMunitionEndPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "munitionEntityID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMunitionEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMunitionEntityID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "munitionQuantity", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMunitionQuantity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMunitionQuantity(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "munitionSiteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMunitionSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMunitionSiteID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "munitionStartPoint", X3DFieldType::SFVec3f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getMunitionStartPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setMunitionStartPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "networkMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setNetworkMode(
              std::any_cast<NetworkModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const EspduTransform &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::string &s) {
          NetworkModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<EspduTransform &>(n).setNetworkMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "port", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setPort(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "readInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getReadInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setReadIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "removeChildren", X3DFieldType::MFNode, AccessType::InputOnly,
        "removeChildren",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DGroupingNode::onRemoveChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rotation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setRotation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rtpHeaderExpected", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getRtpHeaderExpected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setRtpHeaderExpected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scale", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setScale(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scaleOrientation", X3DFieldType::SFRotation, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getScaleOrientation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setScaleOrientation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue0", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue0(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue1", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue1(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue2", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue2(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue3", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue3(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue4", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue4(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue5", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue5(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue6", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue6(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_articulationParameterValue7", X3DFieldType::SFFloat,
        AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).onSet_articulationParameterValue7(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "siteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setSiteID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"timestamp", X3DFieldType::SFTime, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const EspduTransform &>(n).getTimestamp());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<EspduTransform &>(n).emitTimestamp(
                        std::any_cast<SFTime>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "translation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getTranslation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setTranslation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n)
                              .X3DGroupingNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DGroupingNode::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "warhead", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EspduTransform &>(n).getWarhead());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setWarhead(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "writeInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).getWriteInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).setWriteIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EspduTransform &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EspduTransform &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void EspduTransform::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void EspduTransform::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesReadInterval(getReadInterval(), nodeTypeName(), "", out);

  checkRangesWriteInterval(getWriteInterval(), nodeTypeName(), "", out);
}

void EspduTransform::checkRangesReadInterval(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "readInterval",
                                  "readInterval below minimum of 0"});
}

void EspduTransform::checkRangesWriteInterval(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "writeInterval",
                                  "writeInterval below minimum of 0"});
}

} // namespace x3d::nodes
