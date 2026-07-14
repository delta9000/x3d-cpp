// GeoMetadata.cpp
#include "x3d/nodes/GeoMetadata.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string GeoMetadata::nodeTypeName() const { return "GeoMetadata"; }

std::string GeoMetadata::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &GeoMetadata::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoRefresh", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoMetadata &>(n)
                              .X3DUrlObject::getAutoRefresh());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DUrlObject::setAutoRefreshUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"autoRefreshTimeLimit", X3DFieldType::SFTime,
                          AccessType::InputOutput, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const GeoMetadata &>(n)
                                    .X3DUrlObject::getAutoRefreshTimeLimit());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<GeoMetadata &>(n)
                                .X3DUrlObject::setAutoRefreshTimeLimitUnchecked(
                                    std::any_cast<SFTime>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "data", X3DFieldType::MFNode, AccessType::InputOutput, "data",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoMetadata &>(n).getData());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).setData(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoMetadata &>(n)
                              .X3DUrlObject::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DUrlObject::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const GeoMetadata &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<GeoMetadata &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "load", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoMetadata &>(n).X3DUrlObject::getLoad());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DUrlObject::setLoad(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoMetadata &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "summary", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoMetadata &>(n).getSummary());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).setSummary(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "url", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoMetadata &>(n).X3DUrlObject::getUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DUrlObject::setUrl(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const GeoMetadata &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<GeoMetadata &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const GeoMetadata &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<GeoMetadata &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoMetadata &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const GeoMetadata &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<GeoMetadata &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoMetadata &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoMetadata &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void GeoMetadata::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void GeoMetadata::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DUrlObject::checkRangesAutoRefresh(X3DUrlObject::getAutoRefresh(),
                                       nodeTypeName(), "", out);

  X3DUrlObject::checkRangesAutoRefreshTimeLimit(
      X3DUrlObject::getAutoRefreshTimeLimit(), nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createGeoMetadata() {
  return std::make_shared<GeoMetadata>();
}
} // namespace factory_detail

} // namespace x3d::nodes
