// ScreenFontStyle.cpp
#include "x3d/nodes/ScreenFontStyle.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ScreenFontStyle::nodeTypeName() const { return "ScreenFontStyle"; }

std::string ScreenFontStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ScreenFontStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ScreenFontStyle &>(n)
                              .X3DFontStyleNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).X3DFontStyleNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "family", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ScreenFontStyle &>(n).getFamily());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setFamily(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "horizontal", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).getHorizontal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setHorizontal(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ScreenFontStyle &>(n)
                                        .X3DFontStyleNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ScreenFontStyle &>(n).X3DFontStyleNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ScreenFontStyle &>(n)
                                        .X3DFontStyleNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ScreenFontStyle &>(n).X3DFontStyleNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "justify", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).getJustify());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setJustify(
              std::any_cast<std::vector<JustifyChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec =
              dynamic_cast<const ScreenFontStyle &>(n).getJustify();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<JustifyChoices> vec;
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
              JustifyChoices ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<ScreenFontStyle &>(n).setJustify(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "language", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).getLanguage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setLanguage(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "leftToRight", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).getLeftToRight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setLeftToRight(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ScreenFontStyle &>(n)
                              .X3DFontStyleNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).X3DFontStyleNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pointSize", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).getPointSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setPointSize(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"spacing", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ScreenFontStyle &>(n).getSpacing());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ScreenFontStyle &>(n).setSpacingUnchecked(
                        std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ScreenFontStyle &>(n).getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setStyle(
              std::any_cast<FontStyleChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const ScreenFontStyle &>(n).getStyle());
        },

        [](X3DNode &n, const std::string &s) {
          FontStyleChoices ev;
          if (from_string(s, ev))
            dynamic_cast<ScreenFontStyle &>(n).setStyle(ev);
        }

    });

    t.push_back(FieldInfo{
        "topToBottom", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).getTopToBottom());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).setTopToBottom(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ScreenFontStyle &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ScreenFontStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ScreenFontStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ScreenFontStyle::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesSpacing(getSpacing(), nodeTypeName(), "", out);
}

void ScreenFontStyle::checkRangesSpacing(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "spacing",
                                  "spacing below minimum of 0"});
}

} // namespace x3d::nodes
