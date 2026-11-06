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
  SignalPdu() = default;

  ~SignalPdu() = default;

  static SFString getDefaultAddress() { return "localhost"; }

  static SFInt32 getDefaultApplicationID() { return 0; }

  static SFInt32 getDefaultDataLength() { return 0; }

  static SFInt32 getDefaultEncodingScheme() { return 0; }

  static SFInt32 getDefaultEntityID() { return 0; }

  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFInt32 getDefaultMulticastRelayPort() { return 0; }

  static NetworkModeChoices getDefaultNetworkMode() {
    return NetworkModeChoices::STANDALONE;
  }

  static SFInt32 getDefaultPort() { return 0; }

  static SFInt32 getDefaultRadioID() { return 0; }

  static SFTime getDefaultReadInterval() { return 0.1; }

  static SFBool getDefaultRtpHeaderExpected() { return false; }

  static SFInt32 getDefaultSampleRate() { return 0; }

  static SFInt32 getDefaultSamples() { return 0; }

  static SFInt32 getDefaultSiteID() { return 0; }

  static SFInt32 getDefaultTdlType() { return 0; }

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
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Holds audio or digital data conveyed by the radio transmission.
   */
  const MFInt32 &getData() const { return _data; }

  /**
   * Access: inputOutput.
   * @details Holds audio or digital data conveyed by the radio transmission.
   */
  void setData(const MFInt32 &value) { _data = value; }

  void setData(MFInt32 &&value) { _data = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details number of bits of digital voice audio or digital data being sent
   * in the Signal PDU.
   */
  const SFInt32 &getDataLength() const { return _dataLength; }

  /**
   * Access: inputOutput.
   * @details number of bits of digital voice audio or digital data being sent
   * in the Signal PDU.
   */
  void setDataLength(const SFInt32 &value) { _dataLength = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details designates both Encoding Class and Encoding Type.
   */
  const SFInt32 &getEncodingScheme() const { return _encodingScheme; }

  /**
   * Access: inputOutput.
   * @details designates both Encoding Class and Encoding Type.
   */
  void setEncodingScheme(const SFInt32 &value) { _encodingScheme = value; }

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
   * @details sampleRate gives either (1) sample rate in samples per second if
   * Encoding Class is encoded audio, or (2) data rate in bits per second for
   * data transmissions.
   */
  const SFInt32 &getSampleRate() const { return _sampleRate; }

  /**
   * Access: inputOutput.
   * @details sampleRate gives either (1) sample rate in samples per second if
   * Encoding Class is encoded audio, or (2) data rate in bits per second for
   * data transmissions.
   */
  void setSampleRate(const SFInt32 &value) { _sampleRate = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Number of samples in the PDU if the Encoding Class is encoded
   * voice, otherwise the field is set to zero.
   */
  const SFInt32 &getSamples() const { return _samples; }

  /**
   * Access: inputOutput.
   * @details Number of samples in the PDU if the Encoding Class is encoded
   * voice, otherwise the field is set to zero.
   */
  void setSamples(const SFInt32 &value) { _samples = value; }

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
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Tactical Data Link (TDL) type as an enumerated value when the
   * Encoding Class is voice, raw binary, application-specific, or database
   * index representation of a TDL message.
   */
  const SFInt32 &getTdlType() const { return _tdlType; }

  /**
   * Access: inputOutput.
   * @details Tactical Data Link (TDL) type as an enumerated value when the
   * Encoding Class is voice, raw binary, application-specific, or database
   * index representation of a TDL message.
   */
  void setTdlType(const SFInt32 &value) { _tdlType = value; }

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

  SFString _address{"localhost"};

  SFInt32 _applicationID{0};

  MFInt32 _data{};

  SFInt32 _dataLength{0};

  SFInt32 _encodingScheme{0};

  SFInt32 _entityID{0};

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFBool _isNetworkReader{};

  SFBool _isNetworkWriter{};

  SFBool _isRtpHeaderHeard{};

  SFBool _isStandAlone{};

  SFString _multicastRelayHost{};

  SFInt32 _multicastRelayPort{0};

  NetworkModeChoices _networkMode{NetworkModeChoices::STANDALONE};

  SFInt32 _port{0};

  SFInt32 _radioID{0};

  SFTime _readInterval{0.1};

  SFBool _rtpHeaderExpected{false};

  SFInt32 _sampleRate{0};

  SFInt32 _samples{0};

  SFInt32 _siteID{0};

  SFInt32 _tdlType{0};

  SFTime _timestamp{};

  SFInt32 _whichGeometry{1};

  SFTime _writeInterval{1.0};
};

} // namespace x3d::nodes
