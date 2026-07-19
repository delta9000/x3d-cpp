#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct EspduTransform {
  static constexpr std::string_view x3d_name = "EspduTransform";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<EspduTransform, std::string> address{
      "address", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> applicationID{
      "applicationID", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::float_list>
      articulationParameterArray{"articulationParameterArray",
                                 access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::int32_list>
      articulationParameterChangeIndicatorArray{
          "articulationParameterChangeIndicatorArray",
          access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      articulationParameterCount{"articulationParameterCount",
                                 access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::int32_list>
      articulationParameterDesignatorArray{
          "articulationParameterDesignatorArray", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::int32_list>
      articulationParameterIdPartAttachedToArray{
          "articulationParameterIdPartAttachedToArray",
          access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::int32_list>
      articulationParameterTypeArray{"articulationParameterTypeArray",
                                     access_type::input_output};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue0_changed{"articulationParameterValue0_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue1_changed{"articulationParameterValue1_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue2_changed{"articulationParameterValue2_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue3_changed{"articulationParameterValue3_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue4_changed{"articulationParameterValue4_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue5_changed{"articulationParameterValue5_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue6_changed{"articulationParameterValue6_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform, float>
      articulationParameterValue7_changed{"articulationParameterValue7_changed",
                                          access_type::output_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<EspduTransform, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::time_value>
      collideTime{"collideTime", access_type::output_only};
  inline static constexpr field_key<EspduTransform, std::int32_t> collisionType{
      "collisionType", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> deadReckoning{
      "deadReckoning", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::time_value>
      detonateTime{"detonateTime", access_type::output_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      detonationLocation{"detonationLocation", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      detonationRelativeLocation{"detonationRelativeLocation",
                                 access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      detonationResult{"detonationResult", access_type::input_output};
  inline static constexpr field_key<EspduTransform, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      entityCategory{"entityCategory", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> entityCountry{
      "entityCountry", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> entityDomain{
      "entityDomain", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> entityExtra{
      "entityExtra", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> entityID{
      "entityID", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> entityKind{
      "entityKind", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      entitySpecific{"entitySpecific", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      entitySubcategory{"entitySubcategory", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      eventApplicationID{"eventApplicationID", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> eventEntityID{
      "eventEntityID", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> eventNumber{
      "eventNumber", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> eventSiteID{
      "eventSiteID", access_type::input_output};
  inline static constexpr field_key<EspduTransform, bool> fired1{
      "fired1", access_type::input_output};
  inline static constexpr field_key<EspduTransform, bool> fired2{
      "fired2", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::time_value>
      firedTime{"firedTime", access_type::output_only};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      fireMissionIndex{"fireMissionIndex", access_type::input_output};
  inline static constexpr field_key<EspduTransform, float> firingRange{
      "firingRange", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> firingRate{
      "firingRate", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> forceID{
      "forceID", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> fuse{
      "fuse", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3d>
      geoCoords{"geoCoords", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<EspduTransform, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<EspduTransform, bool> isCollided{
      "isCollided", access_type::output_only};
  inline static constexpr field_key<EspduTransform, bool> isDetonated{
      "isDetonated", access_type::output_only};
  inline static constexpr field_key<EspduTransform, bool> isNetworkReader{
      "isNetworkReader", access_type::output_only};
  inline static constexpr field_key<EspduTransform, bool> isNetworkWriter{
      "isNetworkWriter", access_type::output_only};
  inline static constexpr field_key<EspduTransform, bool> isRtpHeaderHeard{
      "isRtpHeaderHeard", access_type::output_only};
  inline static constexpr field_key<EspduTransform, bool> isStandAlone{
      "isStandAlone", access_type::output_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      linearAcceleration{"linearAcceleration", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      linearVelocity{"linearVelocity", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> marking{
      "marking", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string>
      multicastRelayHost{"multicastRelayHost", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      multicastRelayPort{"multicastRelayPort", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      munitionApplicationID{"munitionApplicationID", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      munitionEndPoint{"munitionEndPoint", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      munitionEntityID{"munitionEntityID", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      munitionQuantity{"munitionQuantity", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t>
      munitionSiteID{"munitionSiteID", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      munitionStartPoint{"munitionStartPoint", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::enum_value>
      networkMode{"networkMode", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> port{
      "port", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::time_value>
      readInterval{"readInterval", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::rotation>
      rotation{"rotation", access_type::input_output};
  inline static constexpr field_key<EspduTransform, bool> rtpHeaderExpected{
      "rtpHeaderExpected", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      scale{"scale", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::rotation>
      scaleOrientation{"scaleOrientation", access_type::input_output};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue0{"set_articulationParameterValue0",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue1{"set_articulationParameterValue1",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue2{"set_articulationParameterValue2",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue3{"set_articulationParameterValue3",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue4{"set_articulationParameterValue4",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue5{"set_articulationParameterValue5",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue6{"set_articulationParameterValue6",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, float>
      set_articulationParameterValue7{"set_articulationParameterValue7",
                                      access_type::input_only};
  inline static constexpr field_key<EspduTransform, std::int32_t> siteID{
      "siteID", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::time_value>
      timestamp{"timestamp", access_type::output_only};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::vec3f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<EspduTransform, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::int32_t> warhead{
      "warhead", access_type::input_output};
  inline static constexpr field_key<EspduTransform,
                                    ::x3d::sai::experimental::time_value>
      writeInterval{"writeInterval", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<EspduTransform, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 100> field_keys{{
      {addChildren.name(), addChildren.kind, addChildren.access()},
      {address.name(), address.kind, address.access()},
      {applicationID.name(), applicationID.kind, applicationID.access()},
      {articulationParameterArray.name(), articulationParameterArray.kind,
       articulationParameterArray.access()},
      {articulationParameterChangeIndicatorArray.name(),
       articulationParameterChangeIndicatorArray.kind,
       articulationParameterChangeIndicatorArray.access()},
      {articulationParameterCount.name(), articulationParameterCount.kind,
       articulationParameterCount.access()},
      {articulationParameterDesignatorArray.name(),
       articulationParameterDesignatorArray.kind,
       articulationParameterDesignatorArray.access()},
      {articulationParameterIdPartAttachedToArray.name(),
       articulationParameterIdPartAttachedToArray.kind,
       articulationParameterIdPartAttachedToArray.access()},
      {articulationParameterTypeArray.name(),
       articulationParameterTypeArray.kind,
       articulationParameterTypeArray.access()},
      {articulationParameterValue0_changed.name(),
       articulationParameterValue0_changed.kind,
       articulationParameterValue0_changed.access()},
      {articulationParameterValue1_changed.name(),
       articulationParameterValue1_changed.kind,
       articulationParameterValue1_changed.access()},
      {articulationParameterValue2_changed.name(),
       articulationParameterValue2_changed.kind,
       articulationParameterValue2_changed.access()},
      {articulationParameterValue3_changed.name(),
       articulationParameterValue3_changed.kind,
       articulationParameterValue3_changed.access()},
      {articulationParameterValue4_changed.name(),
       articulationParameterValue4_changed.kind,
       articulationParameterValue4_changed.access()},
      {articulationParameterValue5_changed.name(),
       articulationParameterValue5_changed.kind,
       articulationParameterValue5_changed.access()},
      {articulationParameterValue6_changed.name(),
       articulationParameterValue6_changed.kind,
       articulationParameterValue6_changed.access()},
      {articulationParameterValue7_changed.name(),
       articulationParameterValue7_changed.kind,
       articulationParameterValue7_changed.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {center.name(), center.kind, center.access()},
      {children.name(), children.kind, children.access()},
      {collideTime.name(), collideTime.kind, collideTime.access()},
      {collisionType.name(), collisionType.kind, collisionType.access()},
      {deadReckoning.name(), deadReckoning.kind, deadReckoning.access()},
      {description.name(), description.kind, description.access()},
      {detonateTime.name(), detonateTime.kind, detonateTime.access()},
      {detonationLocation.name(), detonationLocation.kind,
       detonationLocation.access()},
      {detonationRelativeLocation.name(), detonationRelativeLocation.kind,
       detonationRelativeLocation.access()},
      {detonationResult.name(), detonationResult.kind,
       detonationResult.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {entityCategory.name(), entityCategory.kind, entityCategory.access()},
      {entityCountry.name(), entityCountry.kind, entityCountry.access()},
      {entityDomain.name(), entityDomain.kind, entityDomain.access()},
      {entityExtra.name(), entityExtra.kind, entityExtra.access()},
      {entityID.name(), entityID.kind, entityID.access()},
      {entityKind.name(), entityKind.kind, entityKind.access()},
      {entitySpecific.name(), entitySpecific.kind, entitySpecific.access()},
      {entitySubcategory.name(), entitySubcategory.kind,
       entitySubcategory.access()},
      {eventApplicationID.name(), eventApplicationID.kind,
       eventApplicationID.access()},
      {eventEntityID.name(), eventEntityID.kind, eventEntityID.access()},
      {eventNumber.name(), eventNumber.kind, eventNumber.access()},
      {eventSiteID.name(), eventSiteID.kind, eventSiteID.access()},
      {fired1.name(), fired1.kind, fired1.access()},
      {fired2.name(), fired2.kind, fired2.access()},
      {firedTime.name(), firedTime.kind, firedTime.access()},
      {fireMissionIndex.name(), fireMissionIndex.kind,
       fireMissionIndex.access()},
      {firingRange.name(), firingRange.kind, firingRange.access()},
      {firingRate.name(), firingRate.kind, firingRate.access()},
      {forceID.name(), forceID.kind, forceID.access()},
      {fuse.name(), fuse.kind, fuse.access()},
      {geoCoords.name(), geoCoords.kind, geoCoords.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isCollided.name(), isCollided.kind, isCollided.access()},
      {isDetonated.name(), isDetonated.kind, isDetonated.access()},
      {isNetworkReader.name(), isNetworkReader.kind, isNetworkReader.access()},
      {isNetworkWriter.name(), isNetworkWriter.kind, isNetworkWriter.access()},
      {isRtpHeaderHeard.name(), isRtpHeaderHeard.kind,
       isRtpHeaderHeard.access()},
      {isStandAlone.name(), isStandAlone.kind, isStandAlone.access()},
      {linearAcceleration.name(), linearAcceleration.kind,
       linearAcceleration.access()},
      {linearVelocity.name(), linearVelocity.kind, linearVelocity.access()},
      {marking.name(), marking.kind, marking.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {multicastRelayHost.name(), multicastRelayHost.kind,
       multicastRelayHost.access()},
      {multicastRelayPort.name(), multicastRelayPort.kind,
       multicastRelayPort.access()},
      {munitionApplicationID.name(), munitionApplicationID.kind,
       munitionApplicationID.access()},
      {munitionEndPoint.name(), munitionEndPoint.kind,
       munitionEndPoint.access()},
      {munitionEntityID.name(), munitionEntityID.kind,
       munitionEntityID.access()},
      {munitionQuantity.name(), munitionQuantity.kind,
       munitionQuantity.access()},
      {munitionSiteID.name(), munitionSiteID.kind, munitionSiteID.access()},
      {munitionStartPoint.name(), munitionStartPoint.kind,
       munitionStartPoint.access()},
      {networkMode.name(), networkMode.kind, networkMode.access()},
      {port.name(), port.kind, port.access()},
      {readInterval.name(), readInterval.kind, readInterval.access()},
      {removeChildren.name(), removeChildren.kind, removeChildren.access()},
      {rotation.name(), rotation.kind, rotation.access()},
      {rtpHeaderExpected.name(), rtpHeaderExpected.kind,
       rtpHeaderExpected.access()},
      {scale.name(), scale.kind, scale.access()},
      {scaleOrientation.name(), scaleOrientation.kind,
       scaleOrientation.access()},
      {set_articulationParameterValue0.name(),
       set_articulationParameterValue0.kind,
       set_articulationParameterValue0.access()},
      {set_articulationParameterValue1.name(),
       set_articulationParameterValue1.kind,
       set_articulationParameterValue1.access()},
      {set_articulationParameterValue2.name(),
       set_articulationParameterValue2.kind,
       set_articulationParameterValue2.access()},
      {set_articulationParameterValue3.name(),
       set_articulationParameterValue3.kind,
       set_articulationParameterValue3.access()},
      {set_articulationParameterValue4.name(),
       set_articulationParameterValue4.kind,
       set_articulationParameterValue4.access()},
      {set_articulationParameterValue5.name(),
       set_articulationParameterValue5.kind,
       set_articulationParameterValue5.access()},
      {set_articulationParameterValue6.name(),
       set_articulationParameterValue6.kind,
       set_articulationParameterValue6.access()},
      {set_articulationParameterValue7.name(),
       set_articulationParameterValue7.kind,
       set_articulationParameterValue7.access()},
      {siteID.name(), siteID.kind, siteID.access()},
      {timestamp.name(), timestamp.kind, timestamp.access()},
      {translation.name(), translation.kind, translation.access()},
      {visible.name(), visible.kind, visible.access()},
      {warhead.name(), warhead.kind, warhead.access()},
      {writeInterval.name(), writeInterval.kind, writeInterval.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
