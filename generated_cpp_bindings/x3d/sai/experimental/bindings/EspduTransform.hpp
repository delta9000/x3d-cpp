#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct EspduTransform {
  static constexpr std::string_view x3d_name = "EspduTransform";
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
};

} // namespace x3d::sai::experimental::bindings
