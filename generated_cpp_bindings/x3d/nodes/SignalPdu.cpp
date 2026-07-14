// SignalPdu.cpp
#include "x3d/nodes/SignalPdu.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string SignalPdu::nodeTypeName() const { return "SignalPdu"; }

std::string SignalPdu::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &SignalPdu::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "address", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getAddress());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setAddress(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "applicationID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getApplicationID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setApplicationID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DBoundedObject::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DBoundedObject::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "data", X3DFieldType::MFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getData());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setData(std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "dataLength", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getDataLength());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setDataLength(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "encodingScheme", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getEncodingScheme());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setEncodingScheme(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setEntityID(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoCoords", X3DFieldType::SFVec3d, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getGeoCoords());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setGeoCoords(std::any_cast<SFVec3d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoSystem", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getGeoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setGeoSystemUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).X3DSensorNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DSensorNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkReader", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getIsNetworkReader());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).emitIsNetworkReader(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkWriter", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getIsNetworkWriter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).emitIsNetworkWriter(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isRtpHeaderHeard", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getIsRtpHeaderHeard());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).emitIsRtpHeaderHeard(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isStandAlone", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getIsStandAlone());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).emitIsStandAlone(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayHost", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getMulticastRelayHost());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setMulticastRelayHost(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayPort", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getMulticastRelayPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setMulticastRelayPort(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "networkMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setNetworkMode(
              std::any_cast<NetworkModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const SignalPdu &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::string &s) {
          NetworkModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<SignalPdu &>(n).setNetworkMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "port", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setPort(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radioID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getRadioID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setRadioID(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "readInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getReadInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setReadIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rtpHeaderExpected", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getRtpHeaderExpected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setRtpHeaderExpected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sampleRate", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getSampleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setSampleRate(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "samples", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getSamples());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setSamples(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "siteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setSiteID(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "tdlType", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getTdlType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setTdlType(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "timestamp", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).getTimestamp());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).emitTimestamp(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const SignalPdu &>(n)
                                        .X3DBoundedObject::getVisible());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SignalPdu &>(n).X3DBoundedObject::setVisible(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "whichGeometry", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getWhichGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setWhichGeometry(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "writeInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).getWriteInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).setWriteIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SignalPdu &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SignalPdu &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SignalPdu &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SignalPdu &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SignalPdu &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void SignalPdu::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void SignalPdu::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesReadInterval(getReadInterval(), nodeTypeName(), "", out);

  checkRangesWriteInterval(getWriteInterval(), nodeTypeName(), "", out);
}

void SignalPdu::checkRangesReadInterval(const SFTime &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "readInterval",
                                  "readInterval below minimum of 0"});
}

void SignalPdu::checkRangesWriteInterval(const SFTime &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "writeInterval",
                                  "writeInterval below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createSignalPdu() {
  return std::make_shared<SignalPdu>();
}
} // namespace factory_detail

} // namespace x3d::nodes
