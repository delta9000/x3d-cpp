#include "x3d/nodes/RenderedTexture.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string RenderedTexture::nodeTypeName() const { return "RenderedTexture"; }

std::string RenderedTexture::defaultContainerField() const {
  return getDefaultContainerField();
}

X3DVersion RenderedTexture::introducedIn() const { return X3DVersion{4, 1}; }

const FieldTable &RenderedTexture::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "colorDepth", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).getColorDepth());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setColorDepth(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getColorDepth(),
                  &typeid(SFInt32)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "depthMap", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).getDepthMap());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setDepthMap(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getDepthMap(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n)
                              .X3DTextureNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DTextureNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n)
                       .X3DTextureNode::getDescription(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "height", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n).getHeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setHeightUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getHeight(),
                  &typeid(SFInt32)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "maxNumberFrames", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).getMaxNumberFrames());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setMaxNumberFrames(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const RenderedTexture &>(n).getMaxNumberFrames(),
              &typeid(SFInt32)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const RenderedTexture &>(n).X3DNode::getMetadata(),
              &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "repeatS", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n)
                              .X3DTexture2DNode::getRepeatS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n)
              .X3DTexture2DNode::setRepeatSUnchecked(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n)
                       .X3DTexture2DNode::getRepeatS(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "repeatT", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n)
                              .X3DTexture2DNode::getRepeatT());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n)
              .X3DTexture2DNode::setRepeatTUnchecked(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n)
                       .X3DTexture2DNode::getRepeatT(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "replaceImage", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).getReplaceImage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setReplaceImage(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getReplaceImage(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "singleFrame", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).getSingleFrame());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setSingleFrame(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getSingleFrame(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "textureProperties", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "textureProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n)
                              .X3DTexture2DNode::getTextureProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n)
              .X3DTexture2DNode::setTexturePropertiesUnchecked(
                  std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n)
                       .X3DTexture2DNode::getTextureProperties(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "update", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n).getUpdate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setUpdate(
              std::any_cast<GeneratedCubeMapTextureUpdateChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const RenderedTexture &>(n).getUpdate());
        },

        [](X3DNode &n, const std::string &s) {
          GeneratedCubeMapTextureUpdateChoices ev;
          if (from_string(s, ev))
            dynamic_cast<RenderedTexture &>(n).setUpdate(ev);
        }

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getUpdate(),
                  &typeid(GeneratedCubeMapTextureUpdateChoices)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "updateInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).getUpdateInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setUpdateInterval(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getUpdateInterval(),
                  &typeid(SFTime)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "url", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n)
                              .X3DUrlOutputObject::getUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DUrlOutputObject::setUrl(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n)
                       .X3DUrlOutputObject::getUrl(),
                  &typeid(MFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "width", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RenderedTexture &>(n).getWidth());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).setWidth(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).getWidth(),
                  &typeid(SFInt32)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const RenderedTexture &>(n).X3DNode::getClass_(),
              &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RenderedTexture &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RenderedTexture &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const RenderedTexture &>(n).X3DNode::getStyle(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    return t;
  }();
  return table;
}

void RenderedTexture::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createRenderedTexture() {
  return std::make_shared<RenderedTexture>();
}
} // namespace factory_detail

} // namespace x3d::nodes
