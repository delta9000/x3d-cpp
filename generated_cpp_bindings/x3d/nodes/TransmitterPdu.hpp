// TransmitterPdu.hpp
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
  /**
   * @brief Default constructor for TransmitterPdu
   */
  TransmitterPdu() = default;

  /**
   * @brief Destructor for TransmitterPdu
   */
  ~TransmitterPdu() = default;

  /**
   * @brief Get the default value for address
   * @return SFString The default value
   */
  static SFString getDefaultAddress() { return "localhost"; }

  /**
   * @brief Get the default value for antennaLocation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAntennaLocation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for antennaPatternLength
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultAntennaPatternLength() { return 0; }

  /**
   * @brief Get the default value for antennaPatternType
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultAntennaPatternType() { return 0; }

  /**
   * @brief Get the default value for applicationID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultApplicationID() { return 0; }

  /**
   * @brief Get the default value for cryptoKeyID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultCryptoKeyID() { return 0; }

  /**
   * @brief Get the default value for cryptoSystem
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultCryptoSystem() { return 0; }

  /**
   * @brief Get the default value for entityID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityID() { return 0; }

  /**
   * @brief Get the default value for frequency
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFrequency() { return 0; }

  /**
   * @brief Get the default value for geoCoords
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  /**
   * @brief Get the default value for geoSystem
   * @return MFString The default value
   */
  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  /**
   * @brief Get the default value for inputSource
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultInputSource() { return 0; }

  /**
   * @brief Get the default value for lengthOfModulationParameters
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultLengthOfModulationParameters() { return 0; }

  /**
   * @brief Get the default value for modulationTypeDetail
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultModulationTypeDetail() { return 0; }

  /**
   * @brief Get the default value for modulationTypeMajor
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultModulationTypeMajor() { return 0; }

  /**
   * @brief Get the default value for modulationTypeSpreadSpectrum
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultModulationTypeSpreadSpectrum() { return 0; }

  /**
   * @brief Get the default value for modulationTypeSystem
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultModulationTypeSystem() { return 0; }

  /**
   * @brief Get the default value for multicastRelayPort
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMulticastRelayPort() { return 0; }

  /**
   * @brief Get the default value for networkMode
   * @return NetworkModeChoices The default value
   */
  static NetworkModeChoices getDefaultNetworkMode() {
    return NetworkModeChoices::STANDALONE;
  }

  /**
   * @brief Get the default value for port
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultPort() { return 0; }

  /**
   * @brief Get the default value for power
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPower() { return 0.0; }

  /**
   * @brief Get the default value for radioEntityTypeCategory
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioEntityTypeCategory() { return 0; }

  /**
   * @brief Get the default value for radioEntityTypeCountry
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioEntityTypeCountry() { return 0; }

  /**
   * @brief Get the default value for radioEntityTypeDomain
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioEntityTypeDomain() { return 0; }

  /**
   * @brief Get the default value for radioEntityTypeKind
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioEntityTypeKind() { return 0; }

  /**
   * @brief Get the default value for radioEntityTypeNomenclature
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioEntityTypeNomenclature() { return 0; }

  /**
   * @brief Get the default value for radioEntityTypeNomenclatureVersion
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioEntityTypeNomenclatureVersion() { return 0; }

  /**
   * @brief Get the default value for radioID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultRadioID() { return 0; }

  /**
   * @brief Get the default value for readInterval
   * @return SFTime The default value
   */
  static SFTime getDefaultReadInterval() { return 0.1; }

  /**
   * @brief Get the default value for relativeAntennaLocation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultRelativeAntennaLocation() {
    return SFVec3f{0, 0, 0};
  }

  /**
   * @brief Get the default value for rtpHeaderExpected
   * @return SFBool The default value
   */
  static SFBool getDefaultRtpHeaderExpected() { return false; }

  /**
   * @brief Get the default value for siteID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSiteID() { return 0; }

  /**
   * @brief Get the default value for transmitFrequencyBandwidth
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTransmitFrequencyBandwidth() { return 0; }

  /**
   * @brief Get the default value for transmitState
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultTransmitState() { return 0; }

  /**
   * @brief Get the default value for whichGeometry
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultWhichGeometry() { return 1; }

  /**
   * @brief Get the default value for writeInterval
   * @return SFTime The default value
   */
  static SFTime getDefaultWriteInterval() { return 1.0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "DIS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of address. AccessType: inputOutput
   * @details Multicast network address, or else 'localhost'.
   * @return SFString The current value of address.
   */
  SFString getAddress() const { return _address; }

  /**
   * @brief Sets the value of address. AccessType: inputOutput
   * @details Multicast network address, or else 'localhost'.
   * @param value The new value for address.
   */
  void setAddress(const SFString &value) { _address = value; }

  void setAddress(SFString &&value) { _address = std::move(value); }

  /**
   * @brief Gets the value of antennaLocation. AccessType: inputOutput
   * @details World coordinates for antenna location.
   * @return SFVec3f The current value of antennaLocation.
   */
  SFVec3f getAntennaLocation() const { return _antennaLocation; }

  /**
   * @brief Sets the value of antennaLocation. AccessType: inputOutput
   * @details World coordinates for antenna location.
   * @param value The new value for antennaLocation.
   */
  void setAntennaLocation(const SFVec3f &value) { _antennaLocation = value; }

  void setAntennaLocation(SFVec3f &&value) {

    _antennaLocation = std::move(value);
  }

  /**
   * @brief Gets the value of antennaPatternLength. AccessType: inputOutput
   * @details .
   * @return SFInt32 The current value of antennaPatternLength.
   */
  SFInt32 getAntennaPatternLength() const { return _antennaPatternLength; }

  /**
   * @brief Sets the value of antennaPatternLength. AccessType: inputOutput
   * @details .
   * @param value The new value for antennaPatternLength.
   */
  void setAntennaPatternLength(const SFInt32 &value) {

    _antennaPatternLength = value;
  }

  /**
   * @brief Gets the value of antennaPatternType. AccessType: inputOutput
   * @details Antenna shape pattern: 0 for omnidirectional, 1 for beam, 2 for
   * spherical harmonic (deprecated), or optional higher value.
   * @return SFInt32 The current value of antennaPatternType.
   */
  SFInt32 getAntennaPatternType() const { return _antennaPatternType; }

  /**
   * @brief Sets the value of antennaPatternType. AccessType: inputOutput
   * @details Antenna shape pattern: 0 for omnidirectional, 1 for beam, 2 for
   * spherical harmonic (deprecated), or optional higher value.
   * @param value The new value for antennaPatternType.
   */
  void setAntennaPatternType(const SFInt32 &value) {

    _antennaPatternType = value;
  }

  /**
   * @brief Gets the value of applicationID. AccessType: inputOutput
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   * @return SFInt32 The current value of applicationID.
   */
  SFInt32 getApplicationID() const { return _applicationID; }

  /**
   * @brief Sets the value of applicationID. AccessType: inputOutput
   * @details Each simulation application that can respond to simulation
   * management PDUs needs to have a unique applicationID.
   * @param value The new value for applicationID.
   */
  void setApplicationID(const SFInt32 &value) { _applicationID = value; }

  /**
   * @brief Gets the value of cryptoKeyID. AccessType: inputOutput
   * @details Nonzero value corresponding to the simulated cryptographic key.
   * @return SFInt32 The current value of cryptoKeyID.
   */
  SFInt32 getCryptoKeyID() const { return _cryptoKeyID; }

  /**
   * @brief Sets the value of cryptoKeyID. AccessType: inputOutput
   * @details Nonzero value corresponding to the simulated cryptographic key.
   * @param value The new value for cryptoKeyID.
   */
  void setCryptoKeyID(const SFInt32 &value) { _cryptoKeyID = value; }

  /**
   * @brief Gets the value of cryptoSystem. AccessType: inputOutput
   * @details Indicates type of crypto system being used, even if the encryption
   * equipment is not keyed.
   * @return SFInt32 The current value of cryptoSystem.
   */
  SFInt32 getCryptoSystem() const { return _cryptoSystem; }

  /**
   * @brief Sets the value of cryptoSystem. AccessType: inputOutput
   * @details Indicates type of crypto system being used, even if the encryption
   * equipment is not keyed.
   * @param value The new value for cryptoSystem.
   */
  void setCryptoSystem(const SFInt32 &value) { _cryptoSystem = value; }

  /**
   * @brief Gets the value of entityID. AccessType: inputOutput
   * @details EntityID unique ID for entity within that application.
   * @return SFInt32 The current value of entityID.
   */
  SFInt32 getEntityID() const { return _entityID; }

  /**
   * @brief Sets the value of entityID. AccessType: inputOutput
   * @details EntityID unique ID for entity within that application.
   * @param value The new value for entityID.
   */
  void setEntityID(const SFInt32 &value) { _entityID = value; }

  /**
   * @brief Gets the value of frequency. AccessType: inputOutput
   * @details Transmission frequency in Hz.
   * @return SFInt32 The current value of frequency.
   */
  SFInt32 getFrequency() const { return _frequency; }

  /**
   * @brief Sets the value of frequency. AccessType: inputOutput
   * @details Transmission frequency in Hz.
   * @param value The new value for frequency.
   */
  void setFrequency(const SFInt32 &value) {

    validateFrequency(value);

    _frequency = value;
  }

  /**
   * @brief Non-validating write of frequency (runtime/reader ingest path).
   * @details Assigns without the range check setFrequency() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setFrequency() stays the
   *          enforcement point for programmatic callers.
   */
  void setFrequencyUnchecked(const SFInt32 &value) { _frequency = value; }

  /**
   * @brief Gets the value of geoCoords. AccessType: inputOutput
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   * @return SFVec3d The current value of geoCoords.
   */
  SFVec3d getGeoCoords() const { return _geoCoords; }

  /**
   * @brief Sets the value of geoCoords. AccessType: inputOutput
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   * @param value The new value for geoCoords.
   */
  void setGeoCoords(const SFVec3d &value) { _geoCoords = value; }

  void setGeoCoords(SFVec3d &&value) { _geoCoords = std::move(value); }

  /**
   * @brief Gets the value of geoSystem. AccessType: initializeOnly
   * @details Identifies spatial reference frame: Geodetic (GD), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   * @return MFString The current value of geoSystem.
   */
  MFString getGeoSystem() const { return _geoSystem; }
  /**
   * @brief Data-layer write of geoSystem (reader/init ingest path).
   * @details geoSystem is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeoSystem().
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * @brief Gets the value of inputSource. AccessType: inputOutput
   * @details Source of transmission input.
   * @return SFInt32 The current value of inputSource.
   */
  SFInt32 getInputSource() const { return _inputSource; }

  /**
   * @brief Sets the value of inputSource. AccessType: inputOutput
   * @details Source of transmission input.
   * @param value The new value for inputSource.
   */
  void setInputSource(const SFInt32 &value) { _inputSource = value; }

  /**
   * @brief Gets the value of isNetworkReader. AccessType: outputOnly
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity).
   * @return SFBool The current value of isNetworkReader.
   */
  SFBool getIsNetworkReader() const { return _isNetworkReader; }

  /**
   * @brief Emit an output value on isNetworkReader. AccessType: outputOnly
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity). outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsNetworkReader(const SFBool &value) { _isNetworkReader = value; }

  /**
   * @brief Gets the value of isNetworkWriter. AccessType: outputOnly
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval).
   * @return SFBool The current value of isNetworkWriter.
   */
  SFBool getIsNetworkWriter() const { return _isNetworkWriter; }

  /**
   * @brief Emit an output value on isNetworkWriter. AccessType: outputOnly
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval). outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsNetworkWriter(const SFBool &value) { _isNetworkWriter = value; }

  /**
   * @brief Gets the value of isRtpHeaderHeard. AccessType: outputOnly
   * @details Whether incoming DIS packets have an RTP header prepended.
   * @return SFBool The current value of isRtpHeaderHeard.
   */
  SFBool getIsRtpHeaderHeard() const { return _isRtpHeaderHeard; }

  /**
   * @brief Emit an output value on isRtpHeaderHeard. AccessType: outputOnly
   * @details Whether incoming DIS packets have an RTP header prepended.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsRtpHeaderHeard(const SFBool &value) { _isRtpHeaderHeard = value; }

  /**
   * @brief Gets the value of isStandAlone. AccessType: outputOnly
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events).
   * @return SFBool The current value of isStandAlone.
   */
  SFBool getIsStandAlone() const { return _isStandAlone; }

  /**
   * @brief Emit an output value on isStandAlone. AccessType: outputOnly
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events). outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsStandAlone(const SFBool &value) { _isStandAlone = value; }

  /**
   * @brief Gets the value of lengthOfModulationParameters. AccessType:
   * inputOutput
   * @details .
   * @return SFInt32 The current value of lengthOfModulationParameters.
   */
  SFInt32 getLengthOfModulationParameters() const {
    return _lengthOfModulationParameters;
  }

  /**
   * @brief Sets the value of lengthOfModulationParameters. AccessType:
   * inputOutput
   * @details .
   * @param value The new value for lengthOfModulationParameters.
   */
  void setLengthOfModulationParameters(const SFInt32 &value) {

    _lengthOfModulationParameters = value;
  }

  /**
   * @brief Gets the value of modulationTypeDetail. AccessType: inputOutput
   * @details Integer enumeration containing detailed information depending on
   * the major modulation type.
   * @return SFInt32 The current value of modulationTypeDetail.
   */
  SFInt32 getModulationTypeDetail() const { return _modulationTypeDetail; }

  /**
   * @brief Sets the value of modulationTypeDetail. AccessType: inputOutput
   * @details Integer enumeration containing detailed information depending on
   * the major modulation type.
   * @param value The new value for modulationTypeDetail.
   */
  void setModulationTypeDetail(const SFInt32 &value) {

    _modulationTypeDetail = value;
  }

  /**
   * @brief Gets the value of modulationTypeMajor. AccessType: inputOutput
   * @details Integer enumeration containing major classification of the
   * modulation type.
   * @return SFInt32 The current value of modulationTypeMajor.
   */
  SFInt32 getModulationTypeMajor() const { return _modulationTypeMajor; }

  /**
   * @brief Sets the value of modulationTypeMajor. AccessType: inputOutput
   * @details Integer enumeration containing major classification of the
   * modulation type.
   * @param value The new value for modulationTypeMajor.
   */
  void setModulationTypeMajor(const SFInt32 &value) {

    _modulationTypeMajor = value;
  }

  /**
   * @brief Gets the value of modulationTypeSpreadSpectrum. AccessType:
   * inputOutput
   * @details Indicates the spread spectrum technique or combination of spread
   * spectrum techniques in use.
   * @return SFInt32 The current value of modulationTypeSpreadSpectrum.
   */
  SFInt32 getModulationTypeSpreadSpectrum() const {
    return _modulationTypeSpreadSpectrum;
  }

  /**
   * @brief Sets the value of modulationTypeSpreadSpectrum. AccessType:
   * inputOutput
   * @details Indicates the spread spectrum technique or combination of spread
   * spectrum techniques in use.
   * @param value The new value for modulationTypeSpreadSpectrum.
   */
  void setModulationTypeSpreadSpectrum(const SFInt32 &value) {

    _modulationTypeSpreadSpectrum = value;
  }

  /**
   * @brief Gets the value of modulationTypeSystem. AccessType: inputOutput
   * @details Specifies radio system associated with this Transmitter PDU and
   * used to interpret other fields whose values depend on a specific radio
   * system.
   * @return SFInt32 The current value of modulationTypeSystem.
   */
  SFInt32 getModulationTypeSystem() const { return _modulationTypeSystem; }

  /**
   * @brief Sets the value of modulationTypeSystem. AccessType: inputOutput
   * @details Specifies radio system associated with this Transmitter PDU and
   * used to interpret other fields whose values depend on a specific radio
   * system.
   * @param value The new value for modulationTypeSystem.
   */
  void setModulationTypeSystem(const SFInt32 &value) {

    _modulationTypeSystem = value;
  }

  /**
   * @brief Gets the value of multicastRelayHost. AccessType: inputOutput
   * @details Fallback server address if multicast not available locally.
   * @return SFString The current value of multicastRelayHost.
   */
  SFString getMulticastRelayHost() const { return _multicastRelayHost; }

  /**
   * @brief Sets the value of multicastRelayHost. AccessType: inputOutput
   * @details Fallback server address if multicast not available locally.
   * @param value The new value for multicastRelayHost.
   */
  void setMulticastRelayHost(const SFString &value) {

    _multicastRelayHost = value;
  }

  void setMulticastRelayHost(SFString &&value) {

    _multicastRelayHost = std::move(value);
  }

  /**
   * @brief Gets the value of multicastRelayPort. AccessType: inputOutput
   * @details Fallback server port if multicast not available locally.
   * @return SFInt32 The current value of multicastRelayPort.
   */
  SFInt32 getMulticastRelayPort() const { return _multicastRelayPort; }

  /**
   * @brief Sets the value of multicastRelayPort. AccessType: inputOutput
   * @details Fallback server port if multicast not available locally.
   * @param value The new value for multicastRelayPort.
   */
  void setMulticastRelayPort(const SFInt32 &value) {

    _multicastRelayPort = value;
  }

  /**
   * @brief Gets the value of networkMode. AccessType: inputOutput
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   * @return NetworkModeChoices The current value of networkMode.
   */
  NetworkModeChoices getNetworkMode() const { return _networkMode; }

  /**
   * @brief Sets the value of networkMode. AccessType: inputOutput
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   * @param value The new value for networkMode.
   */
  void setNetworkMode(const NetworkModeChoices &value) { _networkMode = value; }

  /**
   * @brief Gets the value of port. AccessType: inputOutput
   * @details Multicast network port, for example: 3000.
   * @return SFInt32 The current value of port.
   */
  SFInt32 getPort() const { return _port; }

  /**
   * @brief Sets the value of port. AccessType: inputOutput
   * @details Multicast network port, for example: 3000.
   * @param value The new value for port.
   */
  void setPort(const SFInt32 &value) { _port = value; }

  /**
   * @brief Gets the value of power. AccessType: inputOutput
   * @details Power that radio would be capable of outputting if on and
   * transmitting, independent of actual transmit state of the radio.
   * @return SFFloat The current value of power.
   */
  SFFloat getPower() const { return _power; }

  /**
   * @brief Sets the value of power. AccessType: inputOutput
   * @details Power that radio would be capable of outputting if on and
   * transmitting, independent of actual transmit state of the radio.
   * @param value The new value for power.
   */
  void setPower(const SFFloat &value) { _power = value; }

  /**
   * @brief Gets the value of radioEntityTypeCategory. AccessType: inputOutput
   * @details Integer enumeration containing EntityType of transmitter radio.
   * @return SFInt32 The current value of radioEntityTypeCategory.
   */
  SFInt32 getRadioEntityTypeCategory() const {
    return _radioEntityTypeCategory;
  }

  /**
   * @brief Sets the value of radioEntityTypeCategory. AccessType: inputOutput
   * @details Integer enumeration containing EntityType of transmitter radio.
   * @param value The new value for radioEntityTypeCategory.
   */
  void setRadioEntityTypeCategory(const SFInt32 &value) {

    _radioEntityTypeCategory = value;
  }

  /**
   * @brief Gets the value of radioEntityTypeCountry. AccessType: inputOutput
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   * @return SFInt32 The current value of radioEntityTypeCountry.
   */
  SFInt32 getRadioEntityTypeCountry() const { return _radioEntityTypeCountry; }

  /**
   * @brief Sets the value of radioEntityTypeCountry. AccessType: inputOutput
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   * @param value The new value for radioEntityTypeCountry.
   */
  void setRadioEntityTypeCountry(const SFInt32 &value) {

    _radioEntityTypeCountry = value;
  }

  /**
   * @brief Gets the value of radioEntityTypeDomain. AccessType: inputOutput
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   * @return SFInt32 The current value of radioEntityTypeDomain.
   */
  SFInt32 getRadioEntityTypeDomain() const { return _radioEntityTypeDomain; }

  /**
   * @brief Sets the value of radioEntityTypeDomain. AccessType: inputOutput
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   * @param value The new value for radioEntityTypeDomain.
   */
  void setRadioEntityTypeDomain(const SFInt32 &value) {

    _radioEntityTypeDomain = value;
  }

  /**
   * @brief Gets the value of radioEntityTypeKind. AccessType: inputOutput
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   * @return SFInt32 The current value of radioEntityTypeKind.
   */
  SFInt32 getRadioEntityTypeKind() const { return _radioEntityTypeKind; }

  /**
   * @brief Sets the value of radioEntityTypeKind. AccessType: inputOutput
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   * @param value The new value for radioEntityTypeKind.
   */
  void setRadioEntityTypeKind(const SFInt32 &value) {

    _radioEntityTypeKind = value;
  }

  /**
   * @brief Gets the value of radioEntityTypeNomenclature. AccessType:
   * inputOutput
   * @details Integer enumerations value indicating nomenclature (name) for a
   * particular emitter.
   * @return SFInt32 The current value of radioEntityTypeNomenclature.
   */
  SFInt32 getRadioEntityTypeNomenclature() const {
    return _radioEntityTypeNomenclature;
  }

  /**
   * @brief Sets the value of radioEntityTypeNomenclature. AccessType:
   * inputOutput
   * @details Integer enumerations value indicating nomenclature (name) for a
   * particular emitter.
   * @param value The new value for radioEntityTypeNomenclature.
   */
  void setRadioEntityTypeNomenclature(const SFInt32 &value) {

    _radioEntityTypeNomenclature = value;
  }

  /**
   * @brief Gets the value of radioEntityTypeNomenclatureVersion. AccessType:
   * inputOutput
   * @details Named equipment version number.
   * @return SFInt32 The current value of radioEntityTypeNomenclatureVersion.
   */
  SFInt32 getRadioEntityTypeNomenclatureVersion() const {
    return _radioEntityTypeNomenclatureVersion;
  }

  /**
   * @brief Sets the value of radioEntityTypeNomenclatureVersion. AccessType:
   * inputOutput
   * @details Named equipment version number.
   * @param value The new value for radioEntityTypeNomenclatureVersion.
   */
  void setRadioEntityTypeNomenclatureVersion(const SFInt32 &value) {

    _radioEntityTypeNomenclatureVersion = value;
  }

  /**
   * @brief Gets the value of radioID. AccessType: inputOutput
   * @details Identifies a particular radio within a given entity.
   * @return SFInt32 The current value of radioID.
   */
  SFInt32 getRadioID() const { return _radioID; }

  /**
   * @brief Sets the value of radioID. AccessType: inputOutput
   * @details Identifies a particular radio within a given entity.
   * @param value The new value for radioID.
   */
  void setRadioID(const SFInt32 &value) { _radioID = value; }

  /**
   * @brief Gets the value of readInterval. AccessType: inputOutput
   * @details Seconds between read updates, 0 means no reading.
   * @return SFTime The current value of readInterval.
   */
  SFTime getReadInterval() const { return _readInterval; }

  /**
   * @brief Sets the value of readInterval. AccessType: inputOutput
   * @details Seconds between read updates, 0 means no reading.
   * @param value The new value for readInterval.
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
   * @brief Non-validating write of readInterval (runtime/reader ingest path).
   * @details Assigns without the range check setReadInterval() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setReadInterval() stays the
   *          enforcement point for programmatic callers.
   */
  void setReadIntervalUnchecked(const SFTime &value) { _readInterval = value; }

  /**
   * @brief Gets the value of relativeAntennaLocation. AccessType: inputOutput
   * @details Relative coordinates for antenna location.
   * @return SFVec3f The current value of relativeAntennaLocation.
   */
  SFVec3f getRelativeAntennaLocation() const {
    return _relativeAntennaLocation;
  }

  /**
   * @brief Sets the value of relativeAntennaLocation. AccessType: inputOutput
   * @details Relative coordinates for antenna location.
   * @param value The new value for relativeAntennaLocation.
   */
  void setRelativeAntennaLocation(const SFVec3f &value) {

    _relativeAntennaLocation = value;
  }

  void setRelativeAntennaLocation(SFVec3f &&value) {

    _relativeAntennaLocation = std::move(value);
  }

  /**
   * @brief Gets the value of rtpHeaderExpected. AccessType: inputOutput
   * @details Whether RTP headers are prepended to DIS PDUs.
   * @return SFBool The current value of rtpHeaderExpected.
   */
  SFBool getRtpHeaderExpected() const { return _rtpHeaderExpected; }

  /**
   * @brief Sets the value of rtpHeaderExpected. AccessType: inputOutput
   * @details Whether RTP headers are prepended to DIS PDUs.
   * @param value The new value for rtpHeaderExpected.
   */
  void setRtpHeaderExpected(const SFBool &value) { _rtpHeaderExpected = value; }

  /**
   * @brief Gets the value of siteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @return SFInt32 The current value of siteID.
   */
  SFInt32 getSiteID() const { return _siteID; }

  /**
   * @brief Sets the value of siteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @param value The new value for siteID.
   */
  void setSiteID(const SFInt32 &value) { _siteID = value; }

  /**
   * @brief Gets the value of timestamp. AccessType: outputOnly
   * @details DIS timestamp in X3D units (value 0.
   * @return SFTime The current value of timestamp.
   */
  SFTime getTimestamp() const { return _timestamp; }

  /**
   * @brief Emit an output value on timestamp. AccessType: outputOnly
   * @details DIS timestamp in X3D units (value 0.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTimestamp(const SFTime &value) { _timestamp = value; }

  /**
   * @brief Gets the value of transmitFrequencyBandwidth. AccessType:
   * inputOutput
   * @details Bandwidth of the particular transmitter measured between the
   * half-power (-3 dB) points (this value represents total bandwidth, not the
   * deviation from the center frequency).
   * @return SFFloat The current value of transmitFrequencyBandwidth.
   */
  SFFloat getTransmitFrequencyBandwidth() const {
    return _transmitFrequencyBandwidth;
  }

  /**
   * @brief Sets the value of transmitFrequencyBandwidth. AccessType:
   * inputOutput
   * @details Bandwidth of the particular transmitter measured between the
   * half-power (-3 dB) points (this value represents total bandwidth, not the
   * deviation from the center frequency).
   * @param value The new value for transmitFrequencyBandwidth.
   */
  void setTransmitFrequencyBandwidth(const SFFloat &value) {

    _transmitFrequencyBandwidth = value;
  }

  /**
   * @brief Gets the value of transmitState. AccessType: inputOutput
   * @details Specify radio transmission state where enumerations value 0 is for
   * off, value 1 for powered but not transmitting, or value 1 is for powered
   * and transmitting,.
   * @return SFInt32 The current value of transmitState.
   */
  SFInt32 getTransmitState() const { return _transmitState; }

  /**
   * @brief Sets the value of transmitState. AccessType: inputOutput
   * @details Specify radio transmission state where enumerations value 0 is for
   * off, value 1 for powered but not transmitting, or value 1 is for powered
   * and transmitting,.
   * @param value The new value for transmitState.
   */
  void setTransmitState(const SFInt32 &value) { _transmitState = value; }

  /**
   * @brief Gets the value of whichGeometry. AccessType: inputOutput
   * @details Select geometry to render: -1 for no geometry, 0 for text trace, 1
   * for default geometry, (optional) higher values to render different states.
   * @return SFInt32 The current value of whichGeometry.
   */
  SFInt32 getWhichGeometry() const { return _whichGeometry; }

  /**
   * @brief Sets the value of whichGeometry. AccessType: inputOutput
   * @details Select geometry to render: -1 for no geometry, 0 for text trace, 1
   * for default geometry, (optional) higher values to render different states.
   * @param value The new value for whichGeometry.
   */
  void setWhichGeometry(const SFInt32 &value) { _whichGeometry = value; }

  /**
   * @brief Gets the value of writeInterval. AccessType: inputOutput
   * @details Seconds between write updates, 0 means no writing (sending).
   * @return SFTime The current value of writeInterval.
   */
  SFTime getWriteInterval() const { return _writeInterval; }

  /**
   * @brief Sets the value of writeInterval. AccessType: inputOutput
   * @details Seconds between write updates, 0 means no writing (sending).
   * @param value The new value for writeInterval.
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
   * @brief Non-validating write of writeInterval (runtime/reader ingest path).
   * @details Assigns without the range check setWriteInterval() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setWriteInterval() stays the
   *          enforcement point for programmatic callers.
   */
  void setWriteIntervalUnchecked(const SFTime &value) {
    _writeInterval = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "TransmitterPdu").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
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

  /**
   * @brief Member variable for address.
   */

  SFString _address{"localhost"};

  /**
   * @brief Member variable for antennaLocation.
   */

  SFVec3f _antennaLocation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for antennaPatternLength.
   */

  SFInt32 _antennaPatternLength{0};

  /**
   * @brief Member variable for antennaPatternType.
   */

  SFInt32 _antennaPatternType{0};

  /**
   * @brief Member variable for applicationID.
   */

  SFInt32 _applicationID{0};

  /**
   * @brief Member variable for cryptoKeyID.
   */

  SFInt32 _cryptoKeyID{0};

  /**
   * @brief Member variable for cryptoSystem.
   */

  SFInt32 _cryptoSystem{0};

  /**
   * @brief Member variable for entityID.
   */

  SFInt32 _entityID{0};

  /**
   * @brief Member variable for frequency.
   */

  SFInt32 _frequency{0};

  /**
   * @brief Member variable for geoCoords.
   */

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for inputSource.
   */

  SFInt32 _inputSource{0};

  /**
   * @brief Member variable for isNetworkReader.
   */

  SFBool _isNetworkReader{};

  /**
   * @brief Member variable for isNetworkWriter.
   */

  SFBool _isNetworkWriter{};

  /**
   * @brief Member variable for isRtpHeaderHeard.
   */

  SFBool _isRtpHeaderHeard{};

  /**
   * @brief Member variable for isStandAlone.
   */

  SFBool _isStandAlone{};

  /**
   * @brief Member variable for lengthOfModulationParameters.
   */

  SFInt32 _lengthOfModulationParameters{0};

  /**
   * @brief Member variable for modulationTypeDetail.
   */

  SFInt32 _modulationTypeDetail{0};

  /**
   * @brief Member variable for modulationTypeMajor.
   */

  SFInt32 _modulationTypeMajor{0};

  /**
   * @brief Member variable for modulationTypeSpreadSpectrum.
   */

  SFInt32 _modulationTypeSpreadSpectrum{0};

  /**
   * @brief Member variable for modulationTypeSystem.
   */

  SFInt32 _modulationTypeSystem{0};

  /**
   * @brief Member variable for multicastRelayHost.
   */

  SFString _multicastRelayHost{};

  /**
   * @brief Member variable for multicastRelayPort.
   */

  SFInt32 _multicastRelayPort{0};

  /**
   * @brief Member variable for networkMode.
   */

  NetworkModeChoices _networkMode{NetworkModeChoices::STANDALONE};

  /**
   * @brief Member variable for port.
   */

  SFInt32 _port{0};

  /**
   * @brief Member variable for power.
   */

  SFFloat _power{0.0};

  /**
   * @brief Member variable for radioEntityTypeCategory.
   */

  SFInt32 _radioEntityTypeCategory{0};

  /**
   * @brief Member variable for radioEntityTypeCountry.
   */

  SFInt32 _radioEntityTypeCountry{0};

  /**
   * @brief Member variable for radioEntityTypeDomain.
   */

  SFInt32 _radioEntityTypeDomain{0};

  /**
   * @brief Member variable for radioEntityTypeKind.
   */

  SFInt32 _radioEntityTypeKind{0};

  /**
   * @brief Member variable for radioEntityTypeNomenclature.
   */

  SFInt32 _radioEntityTypeNomenclature{0};

  /**
   * @brief Member variable for radioEntityTypeNomenclatureVersion.
   */

  SFInt32 _radioEntityTypeNomenclatureVersion{0};

  /**
   * @brief Member variable for radioID.
   */

  SFInt32 _radioID{0};

  /**
   * @brief Member variable for readInterval.
   */

  SFTime _readInterval{0.1};

  /**
   * @brief Member variable for relativeAntennaLocation.
   */

  SFVec3f _relativeAntennaLocation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for rtpHeaderExpected.
   */

  SFBool _rtpHeaderExpected{false};

  /**
   * @brief Member variable for siteID.
   */

  SFInt32 _siteID{0};

  /**
   * @brief Member variable for timestamp.
   */

  SFTime _timestamp{};

  /**
   * @brief Member variable for transmitFrequencyBandwidth.
   */

  SFFloat _transmitFrequencyBandwidth{0};

  /**
   * @brief Member variable for transmitState.
   */

  SFInt32 _transmitState{0};

  /**
   * @brief Member variable for whichGeometry.
   */

  SFInt32 _whichGeometry{1};

  /**
   * @brief Member variable for writeInterval.
   */

  SFTime _writeInterval{1.0};
};

} // namespace x3d::nodes
