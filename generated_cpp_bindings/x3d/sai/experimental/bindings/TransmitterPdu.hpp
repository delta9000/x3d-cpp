#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TransmitterPdu {
  static constexpr std::string_view x3d_name = "TransmitterPdu";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 56> field_keys{{
      {address.name(), address.kind, address.access()},
      {antennaLocation.name(), antennaLocation.kind, antennaLocation.access()},
      {antennaPatternLength.name(), antennaPatternLength.kind,
       antennaPatternLength.access()},
      {antennaPatternType.name(), antennaPatternType.kind,
       antennaPatternType.access()},
      {applicationID.name(), applicationID.kind, applicationID.access()},
      {bboxCenter.name(), bboxCenter.kind, bboxCenter.access()},
      {bboxDisplay.name(), bboxDisplay.kind, bboxDisplay.access()},
      {bboxSize.name(), bboxSize.kind, bboxSize.access()},
      {cryptoKeyID.name(), cryptoKeyID.kind, cryptoKeyID.access()},
      {cryptoSystem.name(), cryptoSystem.kind, cryptoSystem.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {entityID.name(), entityID.kind, entityID.access()},
      {frequency.name(), frequency.kind, frequency.access()},
      {geoCoords.name(), geoCoords.kind, geoCoords.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {inputSource.name(), inputSource.kind, inputSource.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isNetworkReader.name(), isNetworkReader.kind, isNetworkReader.access()},
      {isNetworkWriter.name(), isNetworkWriter.kind, isNetworkWriter.access()},
      {isRtpHeaderHeard.name(), isRtpHeaderHeard.kind,
       isRtpHeaderHeard.access()},
      {isStandAlone.name(), isStandAlone.kind, isStandAlone.access()},
      {lengthOfModulationParameters.name(), lengthOfModulationParameters.kind,
       lengthOfModulationParameters.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {modulationTypeDetail.name(), modulationTypeDetail.kind,
       modulationTypeDetail.access()},
      {modulationTypeMajor.name(), modulationTypeMajor.kind,
       modulationTypeMajor.access()},
      {modulationTypeSpreadSpectrum.name(), modulationTypeSpreadSpectrum.kind,
       modulationTypeSpreadSpectrum.access()},
      {modulationTypeSystem.name(), modulationTypeSystem.kind,
       modulationTypeSystem.access()},
      {multicastRelayHost.name(), multicastRelayHost.kind,
       multicastRelayHost.access()},
      {multicastRelayPort.name(), multicastRelayPort.kind,
       multicastRelayPort.access()},
      {networkMode.name(), networkMode.kind, networkMode.access()},
      {port.name(), port.kind, port.access()},
      {power.name(), power.kind, power.access()},
      {radioEntityTypeCategory.name(), radioEntityTypeCategory.kind,
       radioEntityTypeCategory.access()},
      {radioEntityTypeCountry.name(), radioEntityTypeCountry.kind,
       radioEntityTypeCountry.access()},
      {radioEntityTypeDomain.name(), radioEntityTypeDomain.kind,
       radioEntityTypeDomain.access()},
      {radioEntityTypeKind.name(), radioEntityTypeKind.kind,
       radioEntityTypeKind.access()},
      {radioEntityTypeNomenclature.name(), radioEntityTypeNomenclature.kind,
       radioEntityTypeNomenclature.access()},
      {radioEntityTypeNomenclatureVersion.name(),
       radioEntityTypeNomenclatureVersion.kind,
       radioEntityTypeNomenclatureVersion.access()},
      {radioID.name(), radioID.kind, radioID.access()},
      {readInterval.name(), readInterval.kind, readInterval.access()},
      {relativeAntennaLocation.name(), relativeAntennaLocation.kind,
       relativeAntennaLocation.access()},
      {rtpHeaderExpected.name(), rtpHeaderExpected.kind,
       rtpHeaderExpected.access()},
      {siteID.name(), siteID.kind, siteID.access()},
      {timestamp.name(), timestamp.kind, timestamp.access()},
      {transmitFrequencyBandwidth.name(), transmitFrequencyBandwidth.kind,
       transmitFrequencyBandwidth.access()},
      {transmitState.name(), transmitState.kind, transmitState.access()},
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
