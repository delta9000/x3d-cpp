// ReceiverPdu.cpp
#include "x3d/nodes/ReceiverPdu.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ReceiverPdu::nodeTypeName() const { return "ReceiverPdu"; }

std::string ReceiverPdu::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ReceiverPdu::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "address", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getAddress());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setAddress(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "applicationID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getApplicationID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setApplicationID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n)
              .X3DBoundedObject::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DBoundedObject::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "entityID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setEntityID(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoCoords", X3DFieldType::SFVec3d, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getGeoCoords());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setGeoCoords(
              std::any_cast<SFVec3d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoSystem", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getGeoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setGeoSystemUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ReceiverPdu &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ReceiverPdu &>(n)
                                        .X3DSensorNode::getIsActive());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).X3DSensorNode::emitIsActive(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "isNetworkReader", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getIsNetworkReader());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).emitIsNetworkReader(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isNetworkWriter", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getIsNetworkWriter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).emitIsNetworkWriter(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isRtpHeaderHeard", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getIsRtpHeaderHeard());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).emitIsRtpHeaderHeard(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isStandAlone", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getIsStandAlone());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).emitIsStandAlone(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayHost", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getMulticastRelayHost());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setMulticastRelayHost(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "multicastRelayPort", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getMulticastRelayPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setMulticastRelayPort(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "networkMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setNetworkMode(
              std::any_cast<NetworkModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const ReceiverPdu &>(n).getNetworkMode());
        },

        [](X3DNode &n, const std::string &s) {
          NetworkModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<ReceiverPdu &>(n).setNetworkMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "port", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getPort());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setPort(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radioID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getRadioID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setRadioID(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "readInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getReadInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setReadIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "receivedPower", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getReceivedPower());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setReceivedPower(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "receiverState", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getReceiverState());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setReceiverState(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rtpHeaderExpected", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getRtpHeaderExpected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setRtpHeaderExpected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "siteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setSiteID(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "timestamp", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ReceiverPdu &>(n).getTimestamp());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).emitTimestamp(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"transmitterApplicationID", X3DFieldType::SFInt32,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ReceiverPdu &>(n)
                                        .getTransmitterApplicationID());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).setTransmitterApplicationID(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "transmitterEntityID", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getTransmitterEntityID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setTransmitterEntityID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transmitterRadioID", X3DFieldType::SFInt32, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getTransmitterRadioID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setTransmitterRadioID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transmitterSiteID", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getTransmitterSiteID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setTransmitterSiteID(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ReceiverPdu &>(n)
                                        .X3DBoundedObject::getVisible());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).X3DBoundedObject::setVisible(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "whichGeometry", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getWhichGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setWhichGeometry(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "writeInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).getWriteInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).setWriteIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ReceiverPdu &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ReceiverPdu &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ReceiverPdu &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ReceiverPdu &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ReceiverPdu &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ReceiverPdu &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ReceiverPdu::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ReceiverPdu::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesReadInterval(getReadInterval(), nodeTypeName(), "", out);

  checkRangesWriteInterval(getWriteInterval(), nodeTypeName(), "", out);
}

void ReceiverPdu::checkRangesReadInterval(const SFTime &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "readInterval",
                                  "readInterval below minimum of 0"});
}

void ReceiverPdu::checkRangesWriteInterval(const SFTime &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "writeInterval",
                                  "writeInterval below minimum of 0"});
}

} // namespace x3d::nodes
