#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SignalPdu {
  static constexpr std::string_view x3d_name = "SignalPdu";
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
};

} // namespace x3d::sai::experimental::bindings
