// TransmitterPdu.cpp
#include "TransmitterPdu.hpp"

std::string TransmitterPdu::nodeTypeName() const { return "TransmitterPdu"; }

std::string TransmitterPdu::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TransmitterPdu::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "address", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n).getAddress());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setAddress(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "antennaLocation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getAntennaLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setAntennaLocation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"antennaPatternLength", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransmitterPdu &>(n)
                                        .getAntennaPatternLength());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).setAntennaPatternLength(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "antennaPatternType", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getAntennaPatternType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setAntennaPatternType(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "applicationID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getApplicationID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setApplicationID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n)
              .X3DBoundedObject::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n)
              .X3DBoundedObject::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "cryptoKeyID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getCryptoKeyID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setCryptoKeyID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "cryptoSystem", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getCryptoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setCryptoSystem(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransmitterPdu &>(n)
                                        .X3DSensorNode::getEnabled());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).X3DSensorNode::setEnabled(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "entityID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setEntityID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frequency", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getFrequency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setFrequencyUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoCoords", X3DFieldType::SFVec3d, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getGeoCoords());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setGeoCoords(
              std::any_cast<SFVec3d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoSystem", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getGeoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setGeoSystemUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "inputSource", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getInputSource());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setInputSource(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .X3DSensorNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DSensorNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkReader", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getIsNetworkReader());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).emitIsNetworkReader(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkWriter", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getIsNetworkWriter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).emitIsNetworkWriter(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isRtpHeaderHeard", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getIsRtpHeaderHeard());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).emitIsRtpHeaderHeard(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isStandAlone", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getIsStandAlone());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).emitIsStandAlone(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "lengthOfModulationParameters", X3DFieldType::SFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getLengthOfModulationParameters());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setLengthOfModulationParameters(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"modulationTypeDetail", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransmitterPdu &>(n)
                                        .getModulationTypeDetail());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).setModulationTypeDetail(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "modulationTypeMajor", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getModulationTypeMajor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setModulationTypeMajor(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "modulationTypeSpreadSpectrum", X3DFieldType::SFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getModulationTypeSpreadSpectrum());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setModulationTypeSpreadSpectrum(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"modulationTypeSystem", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransmitterPdu &>(n)
                                        .getModulationTypeSystem());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).setModulationTypeSystem(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "multicastRelayHost", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getMulticastRelayHost());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setMulticastRelayHost(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayPort", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getMulticastRelayPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setMulticastRelayPort(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "networkMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setNetworkMode(
              std::any_cast<NetworkModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const TransmitterPdu &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::string &s) {
          NetworkModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TransmitterPdu &>(n).setNetworkMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "port", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n).getPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setPort(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "power", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n).getPower());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setPower(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radioEntityTypeCategory", X3DFieldType::SFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getRadioEntityTypeCategory());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setRadioEntityTypeCategory(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"radioEntityTypeCountry", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransmitterPdu &>(n)
                                        .getRadioEntityTypeCountry());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).setRadioEntityTypeCountry(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"radioEntityTypeDomain", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransmitterPdu &>(n)
                                        .getRadioEntityTypeDomain());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).setRadioEntityTypeDomain(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "radioEntityTypeKind", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getRadioEntityTypeKind());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setRadioEntityTypeKind(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radioEntityTypeNomenclature", X3DFieldType::SFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getRadioEntityTypeNomenclature());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setRadioEntityTypeNomenclature(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radioEntityTypeNomenclatureVersion", X3DFieldType::SFInt32,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getRadioEntityTypeNomenclatureVersion());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n)
              .setRadioEntityTypeNomenclatureVersion(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radioID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n).getRadioID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setRadioID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "readInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getReadInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setReadIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "relativeAntennaLocation", X3DFieldType::SFVec3f,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getRelativeAntennaLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setRelativeAntennaLocation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rtpHeaderExpected", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getRtpHeaderExpected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setRtpHeaderExpected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "siteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n).getSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setSiteID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"timestamp", X3DFieldType::SFTime, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const TransmitterPdu &>(n).getTimestamp());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransmitterPdu &>(n).emitTimestamp(
                        std::any_cast<SFTime>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "transmitFrequencyBandwidth", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .getTransmitFrequencyBandwidth());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setTransmitFrequencyBandwidth(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transmitState", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getTransmitState());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setTransmitState(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransmitterPdu &>(n)
                              .X3DBoundedObject::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DBoundedObject::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "whichGeometry", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getWhichGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setWhichGeometry(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "writeInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).getWriteInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).setWriteIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransmitterPdu &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransmitterPdu &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TransmitterPdu::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void TransmitterPdu::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesFrequency(getFrequency(), nodeTypeName(), "", out);

  checkRangesReadInterval(getReadInterval(), nodeTypeName(), "", out);

  checkRangesWriteInterval(getWriteInterval(), nodeTypeName(), "", out);
}

void TransmitterPdu::checkRangesFrequency(const SFInt32 &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frequency",
                                  "frequency below minimum of 0"});
}

void TransmitterPdu::checkRangesReadInterval(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "readInterval",
                                  "readInterval below minimum of 0"});
}

void TransmitterPdu::checkRangesWriteInterval(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "writeInterval",
                                  "writeInterval below minimum of 0"});
}
