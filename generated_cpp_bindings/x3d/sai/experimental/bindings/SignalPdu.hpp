#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SignalPdu {
  static constexpr std::string_view x3d_name = "SignalPdu";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SignalPdu, std::string> address{
      "address", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> applicationID{
      "applicationID", access_type::input_output};
  inline static constexpr field_key<SignalPdu, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<SignalPdu, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<SignalPdu, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::int32_list>
      data{"data", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> dataLength{
      "dataLength", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<SignalPdu, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> encodingScheme{
      "encodingScheme", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> entityID{
      "entityID", access_type::input_output};
  inline static constexpr field_key<SignalPdu, ::x3d::sai::experimental::vec3d>
      geoCoords{"geoCoords", access_type::input_output};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SignalPdu, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<SignalPdu, bool> isNetworkReader{
      "isNetworkReader", access_type::output_only};
  inline static constexpr field_key<SignalPdu, bool> isNetworkWriter{
      "isNetworkWriter", access_type::output_only};
  inline static constexpr field_key<SignalPdu, bool> isRtpHeaderHeard{
      "isRtpHeaderHeard", access_type::output_only};
  inline static constexpr field_key<SignalPdu, bool> isStandAlone{
      "isStandAlone", access_type::output_only};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> multicastRelayHost{
      "multicastRelayHost", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> multicastRelayPort{
      "multicastRelayPort", access_type::input_output};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::enum_value>
      networkMode{"networkMode", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> port{
      "port", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> radioID{
      "radioID", access_type::input_output};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::time_value>
      readInterval{"readInterval", access_type::input_output};
  inline static constexpr field_key<SignalPdu, bool> rtpHeaderExpected{
      "rtpHeaderExpected", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> sampleRate{
      "sampleRate", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> samples{
      "samples", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> siteID{
      "siteID", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> tdlType{
      "tdlType", access_type::input_output};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::time_value>
      timestamp{"timestamp", access_type::output_only};
  inline static constexpr field_key<SignalPdu, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::int32_t> whichGeometry{
      "whichGeometry", access_type::input_output};
  inline static constexpr field_key<SignalPdu,
                                    ::x3d::sai::experimental::time_value>
      writeInterval{"writeInterval", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SignalPdu, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 40> field_keys{{
      {address.name(), address.kind, address.access()},
      {applicationID.name(), applicationID.kind, applicationID.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {data.name(), data.kind, data.access()},
      {dataLength.name(), dataLength.kind, dataLength.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {encodingScheme.name(), encodingScheme.kind, encodingScheme.access()},
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
      {rtpHeaderExpected.name(), rtpHeaderExpected.kind,
       rtpHeaderExpected.access()},
      {sampleRate.name(), sampleRate.kind, sampleRate.access()},
      {samples.name(), samples.kind, samples.access()},
      {siteID.name(), siteID.kind, siteID.access()},
      {tdlType.name(), tdlType.kind, tdlType.access()},
      {timestamp.name(), timestamp.kind, timestamp.access()},
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
