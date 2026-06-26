// SignalPdu.hpp
#ifndef SIGNALPDU_HPP
#define SIGNALPDU_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DNetworkSensorNode.hpp"

#include "X3DBoundedObject.hpp"

/**
 * @class SignalPdu
 * @brief SignalPdu is a networked Protocol Data Unit (PDU) information node
 * that communicates the transmission of voice, audio or other data modeled in a
 * simulation.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html#SignalPdu
 */
class SignalPdu : public virtual X3DNetworkSensorNode,
                  public virtual X3DBoundedObject {
public:
  /**
   * @brief Default constructor for SignalPdu
   */
  SignalPdu() = default;

  /**
   * @brief Destructor for SignalPdu
   */
  ~SignalPdu() = default;

  /**
   * @brief Get the default value for address
   * @return SFString The default value
   */
  static SFString getDefaultAddress() { return "localhost"; }

  /**
   * @brief Get the default value for applicationID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultApplicationID() { return 0; }

  /**
   * @brief Get the default value for dataLength
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultDataLength() { return 0; }

  /**
   * @brief Get the default value for encodingScheme
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEncodingScheme() { return 0; }

  /**
   * @brief Get the default value for entityID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityID() { return 0; }

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
   * @brief Get the default value for rtpHeaderExpected
   * @return SFBool The default value
   */
  static SFBool getDefaultRtpHeaderExpected() { return false; }

  /**
   * @brief Get the default value for sampleRate
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSampleRate() { return 0; }

  /**
   * @brief Get the default value for samples
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSamples() { return 0; }

  /**
   * @brief Get the default value for siteID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSiteID() { return 0; }

  /**
   * @brief Get the default value for tdlType
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultTdlType() { return 0; }

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
   * @brief Gets the value of data. AccessType: inputOutput
   * @details Holds audio or digital data conveyed by the radio transmission.
   * @return MFInt32 The current value of data.
   */
  MFInt32 getData() const { return _data; }

  /**
   * @brief Sets the value of data. AccessType: inputOutput
   * @details Holds audio or digital data conveyed by the radio transmission.
   * @param value The new value for data.
   */
  void setData(const MFInt32 &value) { _data = value; }

  void setData(MFInt32 &&value) { _data = std::move(value); }

  /**
   * @brief Gets the value of dataLength. AccessType: inputOutput
   * @details number of bits of digital voice audio or digital data being sent
   * in the Signal PDU.
   * @return SFInt32 The current value of dataLength.
   */
  SFInt32 getDataLength() const { return _dataLength; }

  /**
   * @brief Sets the value of dataLength. AccessType: inputOutput
   * @details number of bits of digital voice audio or digital data being sent
   * in the Signal PDU.
   * @param value The new value for dataLength.
   */
  void setDataLength(const SFInt32 &value) { _dataLength = value; }

  /**
   * @brief Gets the value of encodingScheme. AccessType: inputOutput
   * @details designates both Encoding Class and Encoding Type.
   * @return SFInt32 The current value of encodingScheme.
   */
  SFInt32 getEncodingScheme() const { return _encodingScheme; }

  /**
   * @brief Sets the value of encodingScheme. AccessType: inputOutput
   * @details designates both Encoding Class and Encoding Type.
   * @param value The new value for encodingScheme.
   */
  void setEncodingScheme(const SFInt32 &value) { _encodingScheme = value; }

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
   * @brief Gets the value of sampleRate. AccessType: inputOutput
   * @details sampleRate gives either (1) sample rate in samples per second if
   * Encoding Class is encoded audio, or (2) data rate in bits per second for
   * data transmissions.
   * @return SFInt32 The current value of sampleRate.
   */
  SFInt32 getSampleRate() const { return _sampleRate; }

  /**
   * @brief Sets the value of sampleRate. AccessType: inputOutput
   * @details sampleRate gives either (1) sample rate in samples per second if
   * Encoding Class is encoded audio, or (2) data rate in bits per second for
   * data transmissions.
   * @param value The new value for sampleRate.
   */
  void setSampleRate(const SFInt32 &value) { _sampleRate = value; }

  /**
   * @brief Gets the value of samples. AccessType: inputOutput
   * @details Number of samples in the PDU if the Encoding Class is encoded
   * voice, otherwise the field is set to zero.
   * @return SFInt32 The current value of samples.
   */
  SFInt32 getSamples() const { return _samples; }

  /**
   * @brief Sets the value of samples. AccessType: inputOutput
   * @details Number of samples in the PDU if the Encoding Class is encoded
   * voice, otherwise the field is set to zero.
   * @param value The new value for samples.
   */
  void setSamples(const SFInt32 &value) { _samples = value; }

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
   * @brief Gets the value of tdlType. AccessType: inputOutput
   * @details Tactical Data Link (TDL) type as an enumerated value when the
   * Encoding Class is voice, raw binary, application-specific, or database
   * index representation of a TDL message.
   * @return SFInt32 The current value of tdlType.
   */
  SFInt32 getTdlType() const { return _tdlType; }

  /**
   * @brief Sets the value of tdlType. AccessType: inputOutput
   * @details Tactical Data Link (TDL) type as an enumerated value when the
   * Encoding Class is voice, raw binary, application-specific, or database
   * index representation of a TDL message.
   * @param value The new value for tdlType.
   */
  void setTdlType(const SFInt32 &value) { _tdlType = value; }

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
   * @brief The X3D type name of this node (e.g. "SignalPdu").
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
   * @brief Member variable for applicationID.
   */

  SFInt32 _applicationID{0};

  /**
   * @brief Member variable for data.
   */

  MFInt32 _data{};

  /**
   * @brief Member variable for dataLength.
   */

  SFInt32 _dataLength{0};

  /**
   * @brief Member variable for encodingScheme.
   */

  SFInt32 _encodingScheme{0};

  /**
   * @brief Member variable for entityID.
   */

  SFInt32 _entityID{0};

  /**
   * @brief Member variable for geoCoords.
   */

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

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
   * @brief Member variable for radioID.
   */

  SFInt32 _radioID{0};

  /**
   * @brief Member variable for readInterval.
   */

  SFTime _readInterval{0.1};

  /**
   * @brief Member variable for rtpHeaderExpected.
   */

  SFBool _rtpHeaderExpected{false};

  /**
   * @brief Member variable for sampleRate.
   */

  SFInt32 _sampleRate{0};

  /**
   * @brief Member variable for samples.
   */

  SFInt32 _samples{0};

  /**
   * @brief Member variable for siteID.
   */

  SFInt32 _siteID{0};

  /**
   * @brief Member variable for tdlType.
   */

  SFInt32 _tdlType{0};

  /**
   * @brief Member variable for timestamp.
   */

  SFTime _timestamp{};

  /**
   * @brief Member variable for whichGeometry.
   */

  SFInt32 _whichGeometry{1};

  /**
   * @brief Member variable for writeInterval.
   */

  SFTime _writeInterval{1.0};
};

#endif // SIGNALPDU_HPP