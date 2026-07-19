#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TransmitterPdu {
  static constexpr std::string_view x3d_name = "TransmitterPdu";
  inline static constexpr field_key<TransmitterPdu, std::string> address{
      "address", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::vec3f>
      antennaLocation{"antennaLocation", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      antennaPatternLength{"antennaPatternLength", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      antennaPatternType{"antennaPatternType", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> applicationID{
      "applicationID", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<TransmitterPdu, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> cryptoKeyID{
      "cryptoKeyID", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> cryptoSystem{
      "cryptoSystem", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> entityID{
      "entityID", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> frequency{
      "frequency", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::vec3d>
      geoCoords{"geoCoords", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> inputSource{
      "inputSource", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<TransmitterPdu, bool> isNetworkReader{
      "isNetworkReader", access_type::output_only};
  inline static constexpr field_key<TransmitterPdu, bool> isNetworkWriter{
      "isNetworkWriter", access_type::output_only};
  inline static constexpr field_key<TransmitterPdu, bool> isRtpHeaderHeard{
      "isRtpHeaderHeard", access_type::output_only};
  inline static constexpr field_key<TransmitterPdu, bool> isStandAlone{
      "isStandAlone", access_type::output_only};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      lengthOfModulationParameters{"lengthOfModulationParameters",
                                   access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      modulationTypeDetail{"modulationTypeDetail", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      modulationTypeMajor{"modulationTypeMajor", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      modulationTypeSpreadSpectrum{"modulationTypeSpreadSpectrum",
                                   access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      modulationTypeSystem{"modulationTypeSystem", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string>
      multicastRelayHost{"multicastRelayHost", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      multicastRelayPort{"multicastRelayPort", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::enum_value>
      networkMode{"networkMode", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> port{
      "port", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, float> power{
      "power", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      radioEntityTypeCategory{"radioEntityTypeCategory",
                              access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      radioEntityTypeCountry{"radioEntityTypeCountry",
                             access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      radioEntityTypeDomain{"radioEntityTypeDomain", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      radioEntityTypeKind{"radioEntityTypeKind", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      radioEntityTypeNomenclature{"radioEntityTypeNomenclature",
                                  access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t>
      radioEntityTypeNomenclatureVersion{"radioEntityTypeNomenclatureVersion",
                                         access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> radioID{
      "radioID", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::time_value>
      readInterval{"readInterval", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::vec3f>
      relativeAntennaLocation{"relativeAntennaLocation",
                              access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, bool> rtpHeaderExpected{
      "rtpHeaderExpected", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> siteID{
      "siteID", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::time_value>
      timestamp{"timestamp", access_type::output_only};
  inline static constexpr field_key<TransmitterPdu, float>
      transmitFrequencyBandwidth{"transmitFrequencyBandwidth",
                                 access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> transmitState{
      "transmitState", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::int32_t> whichGeometry{
      "whichGeometry", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu,
                                    ::x3d::sai::experimental::time_value>
      writeInterval{"writeInterval", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TransmitterPdu, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
