// X3DPickSensorNode.cpp
#include "X3DPickSensorNode.hpp"

std::string X3DPickSensorNode::nodeTypeName() const {
  return "X3DPickSensorNode";
}

std::string X3DPickSensorNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DPickSensorNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DPickSensorNode &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DPickSensorNode &>(n)
                              .X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "intersectionType", X3DFieldType::SFEnum, AccessType::InitializeOnly,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getIntersectionType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).setIntersectionTypeUnchecked(
              std::any_cast<IntersectionTypeValues>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const X3DPickSensorNode &>(n).getIntersectionType());
        },

        [](X3DNode &n, const std::string &s) {
          IntersectionTypeValues ev;
          if (from_string(s, ev))
            dynamic_cast<X3DPickSensorNode &>(n).setIntersectionTypeUnchecked(
                ev);
        }

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DPickSensorNode &>(n)
                              .X3DSensorNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DSensorNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "matchCriterion", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getMatchCriterion());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).setMatchCriterion(
              std::any_cast<PickSensorMatchCriterionChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const X3DPickSensorNode &>(n).getMatchCriterion());
        },

        [](X3DNode &n, const std::string &s) {
          PickSensorMatchCriterionChoices ev;
          if (from_string(s, ev))
            dynamic_cast<X3DPickSensorNode &>(n).setMatchCriterion(ev);
        }

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DPickSensorNode &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "objectType", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getObjectType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).setObjectType(
              std::any_cast<std::vector<PickableObjectTypeValues>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec =
              dynamic_cast<const X3DPickSensorNode &>(n).getObjectType();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<PickableObjectTypeValues> vec;
          std::size_t i = 0;
          while (i < s.size()) {
            while (i < s.size() &&
                   (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' ||
                    s[i] == '\r' || s[i] == ','))
              ++i;
            std::size_t j = i;
            while (j < s.size() && s[j] != ' ' && s[j] != '\t' &&
                   s[j] != '\n' && s[j] != '\r' && s[j] != ',')
              ++j;
            if (j > i) {
              PickableObjectTypeValues ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<X3DPickSensorNode &>(n).setObjectType(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "pickedGeometry", X3DFieldType::MFNode, AccessType::OutputOnly,
        "pickedGeometry",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getPickedGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).emitPickedGeometry(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pickingGeometry", X3DFieldType::SFNode, AccessType::InputOutput,
        "pickingGeometry",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getPickingGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).setPickingGeometry(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pickTarget", X3DFieldType::MFNode, AccessType::InputOutput,
        "pickTarget",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getPickTarget());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).setPickTarget(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sortOrder", X3DFieldType::SFEnum, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).getSortOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).setSortOrderUnchecked(
              std::any_cast<PickSensorSortOrderValues>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const X3DPickSensorNode &>(n).getSortOrder());
        },

        [](X3DNode &n, const std::string &s) {
          PickSensorSortOrderValues ev;
          if (from_string(s, ev))
            dynamic_cast<X3DPickSensorNode &>(n).setSortOrderUnchecked(ev);
        }

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPickSensorNode &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPickSensorNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DPickSensorNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}
