#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DNetworkSensorNode.hpp"

#include "x3d/nodes/X3DBoundedObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TransmitterPdu
 * @brief TransmitterPdu is a networked Protocol Data Unit (PDU) information
 * node that provides detailed information about a radio transmitter modeled in
 * a simulation.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html#TransmitterPdu
 */
class TransmitterPdu : public virtual X3DNetworkSensorNode,
                       public virtual X3DBoundedObject {
public:
  TransmitterPdu() = default;

  ~TransmitterPdu() = default;

  static SFString getDefaultAddress() { return "localhost"; }

  static SFVec3f getDefaultAntennaLocation() { return SFVec3f{0, 0, 0}; }

  static SFInt32 getDefaultAntennaPatternLength() { return 0; }

  static SFInt32 getDefaultAntennaPatternType() { return 0; }

  static SFInt32 getDefaultApplicationID() { return 0; }

  static SFInt32 getDefaultCryptoKeyID() { return 0; }

  static SFInt32 getDefaultCryptoSystem() { return 0; }

  static SFInt32 getDefaultEntityID() { return 0; }

  static SFInt32 getDefaultFrequency() { return 0; }

  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFInt32 getDefaultInputSource() { return 0; }

  static SFInt32 getDefaultLengthOfModulationParameters() { return 0; }

  static SFInt32 getDefaultModulationTypeDetail() { return 0; }

  static SFInt32 getDefaultModulationTypeMajor() { return 0; }

  static SFInt32 getDefaultModulationTypeSpreadSpectrum() { return 0; }

  static SFInt32 getDefaultModulationTypeSystem() { return 0; }

  static SFInt32 getDefaultMulticastRelayPort() { return 0; }

  static NetworkModeChoices getDefaultNetworkMode() {
    return NetworkModeChoices::STANDALONE;
  }

  static SFInt32 getDefaultPort() { return 0; }

  static SFFloat getDefaultPower() { return 0.0; }

  static SFInt32 getDefaultRadioEntityTypeCategory() { return 0; }

  static SFInt32 getDefaultRadioEntityTypeCountry() { return 0; }

  static SFInt32 getDefaultRadioEntityTypeDomain() { return 0; }

  static SFInt32 getDefaultRadioEntityTypeKind() { return 0; }

  static SFInt32 getDefaultRadioEntityTypeNomenclature() { return 0; }

  static SFInt32 getDefaultRadioEntityTypeNomenclatureVersion() { return 0; }

  static SFInt32 getDefaultRadioID() { return 0; }

  static SFTime getDefaultReadInterval() { return 0.1; }

  static SFVec3f getDefaultRelativeAntennaLocation() {
    return SFVec3f{0, 0, 0};
  }

  static SFBool getDefaultRtpHeaderExpected() { return false; }

  static SFInt32 getDefaultSiteID() { return 0; }

  static SFFloat getDefaultTransmitFrequencyBandwidth() { return 0; }

  static SFInt32 getDefaultTransmitState() { return 0; }

  static SFInt32 getDefaultWhichGeometry() { return 1; }

  static SFTime getDefaultWriteInterval() { return 1.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "DIS"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Multicast network address, or else 'localhost'.
   */
  const SFString &getAddress() const { return _address; }

  /**
   * Access: inputOutput.
   * @details Multicast network address, or else 'localhost'.
   */
  void setAddress(const SFString &value) { _address = value; }

  void setAddress(SFString &&value) { _address = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details World coordinates for antenna location.
   */
  const SFVec3f &getAntennaLocation() const { return _antennaLocation; }

  /**
   * Access: inputOutput.
   * @details World coordinates for antenna location.
   */
  void setAntennaLocation(const SFVec3f &value) { _antennaLocation = value; }

  void setAntennaLocation(SFVec3f &&value) {

    _antennaLocation = std::move(value);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details .
   */
  const SFInt32 &getAntennaPatternLength() const {
    return _antennaPatternLength;
  }

  /**
   * Access: inputOutput.
   * @details .
   */
  void setAntennaPatternLength(const SFInt32 &value) {

    _antennaPatternLength = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Antenna shape pattern: 0 for omnidirectional, 1 for beam, 2 for
   * spherical harmonic (deprecated), or optional higher value.
   */
  const SFInt32 &getAntennaPatternType() const { return _antennaPatternType; }

  /**
   * Access: inputOutput.
   * @details Antenna shape pattern: 0 for omnidirectional, 1 for beam, 2 for
   * spherical harmonic (deprecated), or optional higher value.
   */
  void setAntennaPatternType(const SFInt32 &value) {

    _antennaPatternType = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   */
  const SFInt32 &getApplicationID() const { return _applicationID; }

  /**
   * Access: inputOutput.
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   */
  void setApplicationID(const SFInt32 &value) { _applicationID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Nonzero value corresponding to the simulated cryptographic key.
   */
  const SFInt32 &getCryptoKeyID() const { return _cryptoKeyID; }

  /**
   * Access: inputOutput.
   * @details Nonzero value corresponding to the simulated cryptographic key.
   */
  void setCryptoKeyID(const SFInt32 &value) { _cryptoKeyID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Indicates type of crypto system being used, even if the encryption
   * equipment is not keyed.
   */
  const SFInt32 &getCryptoSystem() const { return _cryptoSystem; }

  /**
   * Access: inputOutput.
   * @details Indicates type of crypto system being used, even if the encryption
   * equipment is not keyed.
   */
  void setCryptoSystem(const SFInt32 &value) { _cryptoSystem = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details EntityID unique ID for entity within that application.
   */
  const SFInt32 &getEntityID() const { return _entityID; }

  /**
   * Access: inputOutput.
   * @details EntityID unique ID for entity within that application.
   */
  void setEntityID(const SFInt32 &value) { _entityID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Transmission frequency in Hz.
   */
  const SFInt32 &getFrequency() const { return _frequency; }

  /**
   * Access: inputOutput.
   * @details Transmission frequency in Hz.
   */
  void setFrequency(const SFInt32 &value) {

    validateFrequency(value);

    _frequency = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFrequency() for range validation.
   */
  void setFrequencyUnchecked(const SFInt32 &value) { _frequency = value; }

  /**
   * SFVec3d; access: inputOutput. Borrows the stored value.
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   */
  const SFVec3d &getGeoCoords() const { return _geoCoords; }

  /**
   * Access: inputOutput.
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   */
  void setGeoCoords(const SFVec3d &value) { _geoCoords = value; }

  void setGeoCoords(SFVec3d &&value) { _geoCoords = std::move(value); }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details Identifies spatial reference frame: Geodetic (GD), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   */
  const MFString &getGeoSystem() const { return _geoSystem; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Source of transmission input.
   */
  const SFInt32 &getInputSource() const { return _inputSource; }

  /**
   * Access: inputOutput.
   * @details Source of transmission input.
   */
  void setInputSource(const SFInt32 &value) { _inputSource = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity).
   */
  const SFBool &getIsNetworkReader() const { return _isNetworkReader; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity).
   */
  void emitIsNetworkReader(const SFBool &value) { _isNetworkReader = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval).
   */
  const SFBool &getIsNetworkWriter() const { return _isNetworkWriter; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval).
   */
  void emitIsNetworkWriter(const SFBool &value) { _isNetworkWriter = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether incoming DIS packets have an RTP header prepended.
   */
  const SFBool &getIsRtpHeaderHeard() const { return _isRtpHeaderHeard; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether incoming DIS packets have an RTP header prepended.
   */
  void emitIsRtpHeaderHeard(const SFBool &value) { _isRtpHeaderHeard = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events).
   */
  const SFBool &getIsStandAlone() const { return _isStandAlone; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events).
   */
  void emitIsStandAlone(const SFBool &value) { _isStandAlone = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details .
   */
  const SFInt32 &getLengthOfModulationParameters() const {
    return _lengthOfModulationParameters;
  }

  /**
   * Access: inputOutput.
   * @details .
   */
  void setLengthOfModulationParameters(const SFInt32 &value) {

    _lengthOfModulationParameters = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumeration containing detailed information depending on
   * the major modulation type.
   */
  const SFInt32 &getModulationTypeDetail() const {
    return _modulationTypeDetail;
  }

  /**
   * Access: inputOutput.
   * @details Integer enumeration containing detailed information depending on
   * the major modulation type.
   */
  void setModulationTypeDetail(const SFInt32 &value) {

    _modulationTypeDetail = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumeration containing major classification of the
   * modulation type.
   */
  const SFInt32 &getModulationTypeMajor() const { return _modulationTypeMajor; }

  /**
   * Access: inputOutput.
   * @details Integer enumeration containing major classification of the
   * modulation type.
   */
  void setModulationTypeMajor(const SFInt32 &value) {

    _modulationTypeMajor = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Indicates the spread spectrum technique or combination of spread
   * spectrum techniques in use.
   */
  const SFInt32 &getModulationTypeSpreadSpectrum() const {
    return _modulationTypeSpreadSpectrum;
  }

  /**
   * Access: inputOutput.
   * @details Indicates the spread spectrum technique or combination of spread
   * spectrum techniques in use.
   */
  void setModulationTypeSpreadSpectrum(const SFInt32 &value) {

    _modulationTypeSpreadSpectrum = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Specifies radio system associated with this Transmitter PDU and
   * used to interpret other fields whose values depend on a specific radio
   * system.
   */
  const SFInt32 &getModulationTypeSystem() const {
    return _modulationTypeSystem;
  }

  /**
   * Access: inputOutput.
   * @details Specifies radio system associated with this Transmitter PDU and
   * used to interpret other fields whose values depend on a specific radio
   * system.
   */
  void setModulationTypeSystem(const SFInt32 &value) {

    _modulationTypeSystem = value;
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Fallback server address if multicast not available locally.
   */
  const SFString &getMulticastRelayHost() const { return _multicastRelayHost; }

  /**
   * Access: inputOutput.
   * @details Fallback server address if multicast not available locally.
   */
  void setMulticastRelayHost(const SFString &value) {

    _multicastRelayHost = value;
  }

  void setMulticastRelayHost(SFString &&value) {

    _multicastRelayHost = std::move(value);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Fallback server port if multicast not available locally.
   */
  const SFInt32 &getMulticastRelayPort() const { return _multicastRelayPort; }

  /**
   * Access: inputOutput.
   * @details Fallback server port if multicast not available locally.
   */
  void setMulticastRelayPort(const SFInt32 &value) {

    _multicastRelayPort = value;
  }

  /**
   * NetworkModeChoices; access: inputOutput. Borrows the stored value.
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   */
  const NetworkModeChoices &getNetworkMode() const { return _networkMode; }

  /**
   * Access: inputOutput.
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   */
  void setNetworkMode(const NetworkModeChoices &value) { _networkMode = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Multicast network port, for example: 3000.
   */
  const SFInt32 &getPort() const { return _port; }

  /**
   * Access: inputOutput.
   * @details Multicast network port, for example: 3000.
   */
  void setPort(const SFInt32 &value) { _port = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Power that radio would be capable of outputting if on and
   * transmitting, independent of actual transmit state of the radio.
   */
  const SFFloat &getPower() const { return _power; }

  /**
   * Access: inputOutput.
   * @details Power that radio would be capable of outputting if on and
   * transmitting, independent of actual transmit state of the radio.
   */
  void setPower(const SFFloat &value) { _power = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumeration containing EntityType of transmitter radio.
   */
  const SFInt32 &getRadioEntityTypeCategory() const {
    return _radioEntityTypeCategory;
  }

  /**
   * Access: inputOutput.
   * @details Integer enumeration containing EntityType of transmitter radio.
   */
  void setRadioEntityTypeCategory(const SFInt32 &value) {

    _radioEntityTypeCategory = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   */
  const SFInt32 &getRadioEntityTypeCountry() const {
    return _radioEntityTypeCountry;
  }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   */
  void setRadioEntityTypeCountry(const SFInt32 &value) {

    _radioEntityTypeCountry = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   */
  const SFInt32 &getRadioEntityTypeDomain() const {
    return _radioEntityTypeDomain;
  }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   */
  void setRadioEntityTypeDomain(const SFInt32 &value) {

    _radioEntityTypeDomain = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   */
  const SFInt32 &getRadioEntityTypeKind() const { return _radioEntityTypeKind; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   */
  void setRadioEntityTypeKind(const SFInt32 &value) {

    _radioEntityTypeKind = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value indicating nomenclature (name) for a
   * particular emitter.
   */
  const SFInt32 &getRadioEntityTypeNomenclature() const {
    return _radioEntityTypeNomenclature;
  }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value indicating nomenclature (name) for a
   * particular emitter.
   */
  void setRadioEntityTypeNomenclature(const SFInt32 &value) {

    _radioEntityTypeNomenclature = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Named equipment version number.
   */
  const SFInt32 &getRadioEntityTypeNomenclatureVersion() const {
    return _radioEntityTypeNomenclatureVersion;
  }

  /**
   * Access: inputOutput.
   * @details Named equipment version number.
   */
  void setRadioEntityTypeNomenclatureVersion(const SFInt32 &value) {

    _radioEntityTypeNomenclatureVersion = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Identifies a particular radio within a given entity.
   */
  const SFInt32 &getRadioID() const { return _radioID; }

  /**
   * Access: inputOutput.
   * @details Identifies a particular radio within a given entity.
   */
  void setRadioID(const SFInt32 &value) { _radioID = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details Seconds between read updates, 0 means no reading.
   */
  const SFTime &getReadInterval() const { return _readInterval; }

  /**
   * Access: inputOutput.
   * @details Seconds between read updates, 0 means no reading.
   */
  void setReadInterval(const SFTime &value) {

    validateReadInterval(value);

    _readInterval = value;
  }

  void setReadInterval(SFTime &&value) {

    validateReadInterval(value);

    _readInterval = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setReadInterval() for range validation.
   */
  void setReadIntervalUnchecked(const SFTime &value) { _readInterval = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Relative coordinates for antenna location.
   */
  const SFVec3f &getRelativeAntennaLocation() const {
    return _relativeAntennaLocation;
  }

  /**
   * Access: inputOutput.
   * @details Relative coordinates for antenna location.
   */
  void setRelativeAntennaLocation(const SFVec3f &value) {

    _relativeAntennaLocation = value;
  }

  void setRelativeAntennaLocation(SFVec3f &&value) {

    _relativeAntennaLocation = std::move(value);
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether RTP headers are prepended to DIS PDUs.
   */
  const SFBool &getRtpHeaderExpected() const { return _rtpHeaderExpected; }

  /**
   * Access: inputOutput.
   * @details Whether RTP headers are prepended to DIS PDUs.
   */
  void setRtpHeaderExpected(const SFBool &value) { _rtpHeaderExpected = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  const SFInt32 &getSiteID() const { return _siteID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  void setSiteID(const SFInt32 &value) { _siteID = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details DIS timestamp in X3D units (value 0.
   */
  const SFTime &getTimestamp() const { return _timestamp; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details DIS timestamp in X3D units (value 0.
   */
  void emitTimestamp(const SFTime &value) { _timestamp = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Bandwidth of the particular transmitter measured between the
   * half-power (-3 dB) points (this value represents total bandwidth, not the
   * deviation from the center frequency).
   */
  const SFFloat &getTransmitFrequencyBandwidth() const {
    return _transmitFrequencyBandwidth;
  }

  /**
   * Access: inputOutput.
   * @details Bandwidth of the particular transmitter measured between the
   * half-power (-3 dB) points (this value represents total bandwidth, not the
   * deviation from the center frequency).
   */
  void setTransmitFrequencyBandwidth(const SFFloat &value) {

    _transmitFrequencyBandwidth = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Specify radio transmission state where enumerations value 0 is for
   * off, value 1 for powered but not transmitting, or value 1 is for powered
   * and transmitting,.
   */
  const SFInt32 &getTransmitState() const { return _transmitState; }

  /**
   * Access: inputOutput.
   * @details Specify radio transmission state where enumerations value 0 is for
   * off, value 1 for powered but not transmitting, or value 1 is for powered
   * and transmitting,.
   */
  void setTransmitState(const SFInt32 &value) { _transmitState = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Select geometry to render: -1 for no geometry, 0 for text trace, 1
   * for default geometry, (optional) higher values to render different states.
   */
  const SFInt32 &getWhichGeometry() const { return _whichGeometry; }

  /**
   * Access: inputOutput.
   * @details Select geometry to render: -1 for no geometry, 0 for text trace, 1
   * for default geometry, (optional) higher values to render different states.
   */
  void setWhichGeometry(const SFInt32 &value) { _whichGeometry = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details Seconds between write updates, 0 means no writing (sending).
   */
  const SFTime &getWriteInterval() const { return _writeInterval; }

  /**
   * Access: inputOutput.
   * @details Seconds between write updates, 0 means no writing (sending).
   */
  void setWriteInterval(const SFTime &value) {

    validateWriteInterval(value);

    _writeInterval = value;
  }

  void setWriteInterval(SFTime &&value) {

    validateWriteInterval(value);

    _writeInterval = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setWriteInterval() for range validation.
   */
  void setWriteIntervalUnchecked(const SFTime &value) {
    _writeInterval = value;
  }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFrequency(const SFInt32 &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesReadInterval(const SFTime &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesWriteInterval(const SFTime &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateFrequency(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("frequency below minimum of 0");
  }

  static void validateReadInterval(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("readInterval below minimum of 0");
  }

  static void validateWriteInterval(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("writeInterval below minimum of 0");
  }

  SFString _address{"localhost"};

  SFVec3f _antennaLocation{SFVec3f{0, 0, 0}};

  SFInt32 _antennaPatternLength{0};

  SFInt32 _antennaPatternType{0};

  SFInt32 _applicationID{0};

  SFInt32 _cryptoKeyID{0};

  SFInt32 _cryptoSystem{0};

  SFInt32 _entityID{0};

  SFInt32 _frequency{0};

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFInt32 _inputSource{0};

  SFBool _isNetworkReader{};

  SFBool _isNetworkWriter{};

  SFBool _isRtpHeaderHeard{};

  SFBool _isStandAlone{};

  SFInt32 _lengthOfModulationParameters{0};

  SFInt32 _modulationTypeDetail{0};

  SFInt32 _modulationTypeMajor{0};

  SFInt32 _modulationTypeSpreadSpectrum{0};

  SFInt32 _modulationTypeSystem{0};

  SFString _multicastRelayHost{};

  SFInt32 _multicastRelayPort{0};

  NetworkModeChoices _networkMode{NetworkModeChoices::STANDALONE};

  SFInt32 _port{0};

  SFFloat _power{0.0};

  SFInt32 _radioEntityTypeCategory{0};

  SFInt32 _radioEntityTypeCountry{0};

  SFInt32 _radioEntityTypeDomain{0};

  SFInt32 _radioEntityTypeKind{0};

  SFInt32 _radioEntityTypeNomenclature{0};

  SFInt32 _radioEntityTypeNomenclatureVersion{0};

  SFInt32 _radioID{0};

  SFTime _readInterval{0.1};

  SFVec3f _relativeAntennaLocation{SFVec3f{0, 0, 0}};

  SFBool _rtpHeaderExpected{false};

  SFInt32 _siteID{0};

  SFTime _timestamp{};

  SFFloat _transmitFrequencyBandwidth{0};

  SFInt32 _transmitState{0};

  SFInt32 _whichGeometry{1};

  SFTime _writeInterval{1.0};
};

} // namespace x3d::nodes
