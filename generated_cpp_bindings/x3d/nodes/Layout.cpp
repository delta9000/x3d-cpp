// Layout.cpp
#include "x3d/nodes/Layout.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Layout::nodeTypeName() const { return "Layout"; }

std::string Layout::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Layout::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "align", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).getAlign());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).setAlign(
              std::any_cast<std::vector<LayoutAlignChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec = dynamic_cast<const Layout &>(n).getAlign();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<LayoutAlignChoices> vec;
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
              LayoutAlignChoices ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<Layout &>(n).setAlign(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Layout &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "offset", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).getOffset());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).setOffset(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "offsetUnits", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).getOffsetUnits());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).setOffsetUnits(
              std::any_cast<std::vector<LayoutUnitsChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec = dynamic_cast<const Layout &>(n).getOffsetUnits();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<LayoutUnitsChoices> vec;
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
              LayoutUnitsChoices ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<Layout &>(n).setOffsetUnits(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "scaleMode", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).getScaleMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).setScaleMode(
              std::any_cast<std::vector<LayoutScaleModeChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec = dynamic_cast<const Layout &>(n).getScaleMode();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<LayoutScaleModeChoices> vec;
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
              LayoutScaleModeChoices ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<Layout &>(n).setScaleMode(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "size", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).getSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).setSize(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sizeUnits", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).getSizeUnits());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).setSizeUnits(
              std::any_cast<std::vector<LayoutUnitsChoices>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec = dynamic_cast<const Layout &>(n).getSizeUnits();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<LayoutUnitsChoices> vec;
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
              LayoutUnitsChoices ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<Layout &>(n).setSizeUnits(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Layout &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Layout &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Layout::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
