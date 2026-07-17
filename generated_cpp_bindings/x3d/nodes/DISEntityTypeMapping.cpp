// DISEntityTypeMapping.cpp
#include "x3d/nodes/DISEntityTypeMapping.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string DISEntityTypeMapping::nodeTypeName() const {
  return "DISEntityTypeMapping";
}

std::string DISEntityTypeMapping::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &DISEntityTypeMapping::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoRefresh", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DUrlObject::getAutoRefresh());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n)
              .X3DUrlObject::setAutoRefreshUnchecked(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"autoRefreshTimeLimit", X3DFieldType::SFTime,
                          AccessType::InputOutput, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const DISEntityTypeMapping &>(n)
                                    .X3DUrlObject::getAutoRefreshTimeLimit());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<DISEntityTypeMapping &>(n)
                                .X3DUrlObject::setAutoRefreshTimeLimitUnchecked(
                                    std::any_cast<SFTime>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "category", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getCategory());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setCategoryUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "country", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getCountry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setCountryUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DUrlObject::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DUrlObject::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "domain", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getDomain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setDomainUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "extra", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getExtra());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setExtraUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "kind", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getKind());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setKindUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "load", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DUrlObject::getLoad());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DUrlObject::setLoad(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "specific", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getSpecific());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setSpecificUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "subcategory", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).getSubcategory());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).setSubcategoryUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "url", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DUrlObject::getUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DUrlObject::setUrl(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DISEntityTypeMapping &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DISEntityTypeMapping &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DISEntityTypeMapping &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void DISEntityTypeMapping::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void DISEntityTypeMapping::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  X3DUrlObject::checkRangesAutoRefresh(X3DUrlObject::getAutoRefresh(),
                                       nodeTypeName(), "", out);

  X3DUrlObject::checkRangesAutoRefreshTimeLimit(
      X3DUrlObject::getAutoRefreshTimeLimit(), nodeTypeName(), "", out);

  checkRangesCategory(getCategory(), nodeTypeName(), "", out);

  checkRangesCountry(getCountry(), nodeTypeName(), "", out);

  checkRangesDomain(getDomain(), nodeTypeName(), "", out);

  checkRangesExtra(getExtra(), nodeTypeName(), "", out);

  checkRangesKind(getKind(), nodeTypeName(), "", out);

  checkRangesSpecific(getSpecific(), nodeTypeName(), "", out);

  checkRangesSubcategory(getSubcategory(), nodeTypeName(), "", out);
}

void DISEntityTypeMapping::checkRangesCategory(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "category",
                                  "category below minimum of 0"});
  if (value > 255)
    out.push_back(RangeDiagnostic{nodeType, defName, "category",
                                  "category above maximum of 255"});
}

void DISEntityTypeMapping::checkRangesCountry(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "country",
                                  "country below minimum of 0"});
  if (value > 65535)
    out.push_back(RangeDiagnostic{nodeType, defName, "country",
                                  "country above maximum of 65535"});
}

void DISEntityTypeMapping::checkRangesDomain(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "domain",
                                  "domain below minimum of 0"});
  if (value > 255)
    out.push_back(RangeDiagnostic{nodeType, defName, "domain",
                                  "domain above maximum of 255"});
}

void DISEntityTypeMapping::checkRangesExtra(const SFInt32 &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "extra",
                                  "extra below minimum of 0"});
  if (value > 255)
    out.push_back(RangeDiagnostic{nodeType, defName, "extra",
                                  "extra above maximum of 255"});
}

void DISEntityTypeMapping::checkRangesKind(const SFInt32 &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "kind", "kind below minimum of 0"});
  if (value > 255)
    out.push_back(RangeDiagnostic{nodeType, defName, "kind",
                                  "kind above maximum of 255"});
}

void DISEntityTypeMapping::checkRangesSpecific(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "specific",
                                  "specific below minimum of 0"});
  if (value > 255)
    out.push_back(RangeDiagnostic{nodeType, defName, "specific",
                                  "specific above maximum of 255"});
}

void DISEntityTypeMapping::checkRangesSubcategory(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "subcategory",
                                  "subcategory below minimum of 0"});
  if (value > 255)
    out.push_back(RangeDiagnostic{nodeType, defName, "subcategory",
                                  "subcategory above maximum of 255"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createDISEntityTypeMapping() {
  return std::make_shared<DISEntityTypeMapping>();
}
} // namespace factory_detail

} // namespace x3d::nodes
