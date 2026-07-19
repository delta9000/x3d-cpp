#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ReceiverPdu {
  static constexpr std::string_view x3d_name = "ReceiverPdu";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ReceiverPdu, std::string> address{
      "address", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> applicationID{
      "applicationID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<ReceiverPdu, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<ReceiverPdu, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> entityID{
      "entityID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::vec3d>
      geoCoords{"geoCoords", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<ReceiverPdu, bool> isNetworkReader{
      "isNetworkReader", access_type::output_only};
  inline static constexpr field_key<ReceiverPdu, bool> isNetworkWriter{
      "isNetworkWriter", access_type::output_only};
  inline static constexpr field_key<ReceiverPdu, bool> isRtpHeaderHeard{
      "isRtpHeaderHeard", access_type::output_only};
  inline static constexpr field_key<ReceiverPdu, bool> isStandAlone{
      "isStandAlone", access_type::output_only};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::string>
      multicastRelayHost{"multicastRelayHost", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t>
      multicastRelayPort{"multicastRelayPort", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::enum_value>
      networkMode{"networkMode", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> port{
      "port", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> radioID{
      "radioID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::time_value>
      readInterval{"readInterval", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, float> receivedPower{
      "receivedPower", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> receiverState{
      "receiverState", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, bool> rtpHeaderExpected{
      "rtpHeaderExpected", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> siteID{
      "siteID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::time_value>
      timestamp{"timestamp", access_type::output_only};
  inline static constexpr field_key<ReceiverPdu, std::int32_t>
      transmitterApplicationID{"transmitterApplicationID",
                               access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t>
      transmitterEntityID{"transmitterEntityID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t>
      transmitterRadioID{"transmitterRadioID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t>
      transmitterSiteID{"transmitterSiteID", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::int32_t> whichGeometry{
      "whichGeometry", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu,
                                    ::x3d::sai::experimental::time_value>
      writeInterval{"writeInterval", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ReceiverPdu, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 40> field_keys{{
      {address.name(), address.kind, address.access()},
      {applicationID.name(), applicationID.kind, applicationID.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {entityID.name(), entityID.kind, entityID.access()},
      {geoCoords.name(), geoCoords.kind, geoCoords.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isNetworkReader.name(), isNetworkReader.kind, isNetworkReader.access()},
      {isNetworkWriter.name(), isNetworkWriter.kind, isNetworkWriter.access()},
      {isRtpHeaderHeard.name(), isRtpHeaderHeard.kind,
       isRtpHeaderHeard.access()},
      {isStandAlone.name(), isStandAlone.kind, isStandAlone.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {multicastRelayHost.name(), multicastRelayHost.kind,
       multicastRelayHost.access()},
      {multicastRelayPort.name(), multicastRelayPort.kind,
       multicastRelayPort.access()},
      {networkMode.name(), networkMode.kind, networkMode.access()},
      {port.name(), port.kind, port.access()},
      {radioID.name(), radioID.kind, radioID.access()},
      {readInterval.name(), readInterval.kind, readInterval.access()},
      {receivedPower.name(), receivedPower.kind, receivedPower.access()},
      {receiverState.name(), receiverState.kind, receiverState.access()},
      {rtpHeaderExpected.name(), rtpHeaderExpected.kind,
       rtpHeaderExpected.access()},
      {siteID.name(), siteID.kind, siteID.access()},
      {timestamp.name(), timestamp.kind, timestamp.access()},
      {transmitterApplicationID.name(), transmitterApplicationID.kind,
       transmitterApplicationID.access()},
      {transmitterEntityID.name(), transmitterEntityID.kind,
       transmitterEntityID.access()},
      {transmitterRadioID.name(), transmitterRadioID.kind,
       transmitterRadioID.access()},
      {transmitterSiteID.name(), transmitterSiteID.kind,
       transmitterSiteID.access()},
      {visible.name(), visible.kind, visible.access()},
      {whichGeometry.name(), whichGeometry.kind, whichGeometry.access()},
      {writeInterval.name(), writeInterval.kind, writeInterval.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
