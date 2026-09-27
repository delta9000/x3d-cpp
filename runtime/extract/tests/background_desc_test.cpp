// background_desc_test.cpp — the panorama reaches the extraction seam
// (SEAM-BACKGROUND, ENV-11) and ComposedCubeMapTexture surfaces its faces
// (CMT-1): BackgroundDesc carries the six faces + transparency for Background
// (*Url lists) and TextureBackground (*Texture nodes); refOf() maps a
// ComposedCubeMapTexture to Source::Cube with six face refs.
#include "SceneExtractor.hpp"
#include "TextureExtract.hpp"

#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DRangeValidate.hpp"
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/nodes/X3DBackgroundNode.hpp"

#include "doctest/doctest.h"

#include <any>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using extract::TextureRef;

namespace {
void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
std::shared_ptr<X3DNode> image(const std::string &url) {
  auto t = createX3DNode("ImageTexture");
  setF(t, "url", std::any(MFString{url}));
  return t;
}
extract::BackgroundDesc extractBound(const std::shared_ptr<X3DNode> &bg) {
  Scene scene;
  scene.addRootNode(bg);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.tick(0.0); // bind
  extract::SceneExtractor ex(ctx, scene);
  return ex.background();
}
} // namespace

TEST_CASE("BackgroundDesc: Background *Url faces and transparency reach the seam") {
  auto bg = createX3DNode("Background");
  setF(bg, "frontUrl", std::any(MFString{"missing.png", "front.png"}));
  setF(bg, "topUrl", std::any(MFString{"top.png"}));
  setF(bg, "transparency", std::any(0.25f));
  const extract::BackgroundDesc d = extractBound(bg);
  CHECK(d.front.source == TextureRef::Source::Url);
  CHECK(d.front.url == MFString{"missing.png", "front.png"}); // ordered fallback kept
  CHECK(d.top.url == MFString{"top.png"});
  CHECK(d.back.url.empty());
  CHECK(d.hasPanorama());
  CHECK(d.transparency == doctest::Approx(0.25f));
}

TEST_CASE("BackgroundDesc: a gradient-only Background has no panorama") {
  const extract::BackgroundDesc d = extractBound(createX3DNode("Background"));
  CHECK_FALSE(d.hasPanorama());
  CHECK(d.transparency == doctest::Approx(0.0f));
}

TEST_CASE("BackgroundDesc: TextureBackground face nodes reach the seam") {
  auto tb = createX3DNode("TextureBackground");
  setF(tb, "leftTexture", std::any(std::shared_ptr<X3DNode>(image("left.png"))));
  auto px = createX3DNode("PixelTexture");
  setF(px, "image", std::any(SFImage{1, 1, 3, {255, 0, 0}}));
  setF(tb, "bottomTexture", std::any(std::shared_ptr<X3DNode>(px)));
  const extract::BackgroundDesc d = extractBound(tb);
  CHECK(d.left.source == TextureRef::Source::Url);
  CHECK(d.left.url == MFString{"left.png"});
  CHECK(d.bottom.source == TextureRef::Source::Inline);
  CHECK(d.bottom.inlinePixels.width == 1);
  CHECK(d.front.url.empty());
  CHECK(d.hasPanorama());

  // Url faces resolve through the ordinary TextureResolver path.
  std::vector<TextureRef> faces{d.left};
  int calls = 0;
  extract::resolveTextureRefs(faces, [&](const std::string &u) {
    ++calls;
    CHECK(u == "left.png");
    return extract::TexturePixelResult::makeFailed();
  });
  CHECK(calls == 1);
}

TEST_CASE("refOf: ComposedCubeMapTexture surfaces six face refs (CMT-1)") {
  auto cube = createX3DNode("ComposedCubeMapTexture");
  setF(cube, "frontTexture", std::any(std::shared_ptr<X3DNode>(image("f.png"))));
  setF(cube, "topTexture", std::any(std::shared_ptr<X3DNode>(image("t.png"))));
  const TextureRef r = extract::matsys::refOf(cube, TextureRef::Slot::BaseColor);
  REQUIRE(r.source == TextureRef::Source::Cube);
  REQUIRE(r.cubeFaces.size() == 6);
  CHECK(r.cubeFaces[0].url == MFString{"f.png"}); // front
  CHECK(r.cubeFaces[4].url == MFString{"t.png"}); // top
  CHECK(r.cubeFaces[1].url.empty());               // back unauthored

  std::vector<TextureRef> refs{r};
  std::vector<std::string> asked;
  extract::resolveTextureRefs(refs, [&](const std::string &u) {
    asked.push_back(u);
    return extract::TexturePixelResult::makeFailed();
  });
  CHECK(asked == std::vector<std::string>{"f.png", "t.png"});
}

TEST_CASE("TextureBackground preserves MultiTexture panorama face") {
  auto tb = createX3DNode("TextureBackground");
  auto multi = createX3DNode("MultiTexture");
  setF(multi, "texture", std::any(std::vector<std::shared_ptr<X3DNode>>{
      image("first.png"), image("second.png")}));
  setF(tb, "frontTexture", std::any(std::shared_ptr<X3DNode>(multi)));
  const extract::BackgroundDesc d = extractBound(tb);
  CHECK(d.hasPanorama());
  CHECK(d.front.source == TextureRef::Source::Multi);
  REQUIRE(d.front.multiStages.size() == 2);
  CHECK(d.front.multiStages[0].url == MFString{"first.png"});
  CHECK(d.front.multiStages[1].url == MFString{"second.png"});
}

TEST_CASE("Background reports decreasing sky and ground angles") {
  auto bg = std::dynamic_pointer_cast<X3DBackgroundNode>(createX3DNode("Background"));
  REQUIRE(bg != nullptr);
  bg->setSkyAngle(MFFloat{2.0f, 1.0f});
  bg->setGroundAngle(MFFloat{1.0f, 0.5f});
  const auto warnings = collectRangeWarnings(*bg);
  CHECK(warnings.size() == 2);
  CHECK(warnings[0].fieldName == "skyAngle");
  CHECK(warnings[0].detail.find("BACKGROUND_ANGLE_ORDER") != std::string::npos);
  CHECK(warnings[1].fieldName == "groundAngle");
  CHECK(warnings[1].detail.find("BACKGROUND_ANGLE_ORDER") != std::string::npos);
}
