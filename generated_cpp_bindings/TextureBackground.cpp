// TextureBackground.cpp
#include "TextureBackground.hpp"

std::string TextureBackground::nodeTypeName() const {
  return "TextureBackground";
}

std::string TextureBackground::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TextureBackground::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "backTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "backTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).getBackTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).setBackTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bindTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBindableNode::getBindTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DBindableNode::emitBindTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bottomTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "bottomTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).getBottomTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).setBottomTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frontTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "frontTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).getFrontTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).setFrontTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "groundAngle", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBackgroundNode::getGroundAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n)
              .X3DBackgroundNode::setGroundAngleUnchecked(
                  std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "groundColor", X3DFieldType::MFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBackgroundNode::getGroundColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n)
              .X3DBackgroundNode::setGroundColorUnchecked(
                  std::any_cast<MFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isBound", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBindableNode::getIsBound());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DBindableNode::emitIsBound(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "leftTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "leftTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).getLeftTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).setLeftTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rightTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "rightTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).getRightTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).setRightTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_bind", X3DFieldType::SFBool, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DBindableNode::onSet_bind(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skyAngle", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBackgroundNode::getSkyAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n)
              .X3DBackgroundNode::setSkyAngleUnchecked(
                  std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skyColor", X3DFieldType::MFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBackgroundNode::getSkyColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n)
              .X3DBackgroundNode::setSkyColorUnchecked(
                  std::any_cast<MFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "topTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "topTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).getTopTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).setTopTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureBackground &>(n)
                              .X3DBackgroundNode::getTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n)
              .X3DBackgroundNode::setTransparencyUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureBackground &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureBackground &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TextureBackground::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void TextureBackground::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  X3DBackgroundNode::checkRangesGroundAngle(X3DBackgroundNode::getGroundAngle(),
                                            nodeTypeName(), "", out);

  X3DBackgroundNode::checkRangesGroundColor(X3DBackgroundNode::getGroundColor(),
                                            nodeTypeName(), "", out);

  X3DBackgroundNode::checkRangesSkyAngle(X3DBackgroundNode::getSkyAngle(),
                                         nodeTypeName(), "", out);

  X3DBackgroundNode::checkRangesSkyColor(X3DBackgroundNode::getSkyColor(),
                                         nodeTypeName(), "", out);

  X3DBackgroundNode::checkRangesTransparency(
      X3DBackgroundNode::getTransparency(), nodeTypeName(), "", out);
}
