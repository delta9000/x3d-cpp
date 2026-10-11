// texture_projector_test.cpp — REQ-PROJECTION, §42 texture projectors.
//
// SceneExtractor::projectors() surfaces each active TextureProjector and
// TextureProjectorParallel world-resolved (ADR-0061): the projection volume
// and texture coordinates through ProjectorDesc::project(), near/far bounds,
// the aspect ratio of the resolved image, on/global scoping, and the
// aspectRatio output TextureProjectorSystem emits for a PixelTexture.
#include "RuntimeSession.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"

#include <any>
#include <cmath>
#include <string>

using namespace x3d::runtime;
using extract::ProjectorDesc;

namespace {

std::unique_ptr<RuntimeSession> session(const std::string &body,
                                        SessionOptions options = {}) {
  auto s = RuntimeSession::create(
      x3d::codec::parseDocument("<X3D profile='Full' version='4.0'><Scene>" + body +
                                "</Scene></X3D>"),
      std::move(options));
  s->fullSnapshot();
  return s;
}

const char *kTexture = "<PixelTexture containerField='texture' image='2 1 3 0xFF0000 0x00FF00'/>";

} // namespace

TEST_CASE("TextureProjector: perspective volume, texture coordinates and range") {
  auto s = session(std::string("<Transform translation='0 0 5'><TextureProjector "
                               "direction='0 0 -1' upVector='0 1 0' fieldOfView='1.5707963' "
                               "nearDistance='2' farDistance='8' color='1 0.5 0' intensity='0.7' "
                               "ambientIntensity='0.2'>") +
                   kTexture + "</TextureProjector></Transform>");
  const auto projectors = s->extractor().projectors();
  REQUIRE(projectors.size() == 1);
  const ProjectorDesc &p = projectors[0];
  CHECK(p.type == ProjectorDesc::Type::Perspective);
  CHECK(p.worldLocation.z == doctest::Approx(5));
  CHECK(p.worldDirection.z == doctest::Approx(-1));
  CHECK(p.color.g == doctest::Approx(0.5f));
  CHECK(p.intensity == doctest::Approx(0.7f));
  CHECK(p.ambientIntensity == doctest::Approx(0.2f));
  CHECK(p.global);
  CHECK(p.aspectRatio == doctest::Approx(2)); // 2 x 1 image
  CHECK(p.texture.source == extract::TextureRef::Source::Inline);

  float u = 0, v = 0;
  REQUIRE(p.project({0, 0, 0}, u, v)); // on the axis, 5 away
  CHECK(u == doctest::Approx(0.5f));
  CHECK(v == doctest::Approx(0.5f));
  // fieldOfView spans the shorter (vertical) side: tan(pi/4) * 5 = 5 up,
  // twice that across.
  REQUIRE(p.project({9, 4, 0}, u, v));
  CHECK(u == doctest::Approx(0.95f));
  CHECK(v == doctest::Approx(0.9f));
  CHECK_FALSE(p.project({0, 6, 0}, u, v));  // above the frustum
  CHECK_FALSE(p.project({11, 0, 0}, u, v)); // beside it
  CHECK_FALSE(p.project({0, 0, 4}, u, v));  // nearer than nearDistance
  CHECK_FALSE(p.project({0, 0, -4}, u, v)); // beyond farDistance
  CHECK_FALSE(p.project({0, 0, 6}, u, v));  // behind the projector
}

TEST_CASE("TextureProjectorParallel: extents scale with the local frame") {
  auto s = session(std::string("<Transform scale='2 2 2'><TextureProjectorParallel "
                               "direction='0 0 -1' fieldOfView='-1 -0.5 1 0.5'>") +
                   kTexture + "</TextureProjectorParallel></Transform>");
  const auto projectors = s->extractor().projectors();
  REQUIRE(projectors.size() == 1);
  const ProjectorDesc &p = projectors[0];
  CHECK(p.type == ProjectorDesc::Type::Parallel);
  float u = 0, v = 0;
  REQUIRE(p.project({1, 0.5f, -3}, u, v)); // the volume does not narrow
  CHECK(u == doctest::Approx(0.75f));
  CHECK(v == doctest::Approx(0.75f));
  REQUIRE(p.project({-1.9f, -0.9f, -30}, u, v));
  CHECK_FALSE(p.project({2.1f, 0, -1}, u, v));
  CHECK_FALSE(p.project({0, 0, 1}, u, v)); // behind
}

TEST_CASE("Texture projectors: on, scope and the default up vector") {
  auto s = session(std::string("<TextureProjector on='false'/>"
                               "<Group DEF='A'><TextureProjector global='false'/>"
                               "<Shape><Box/></Shape></Group>"
                               "<Group><Shape><Sphere/></Shape></Group>"
                               "<TextureProjector/>"));
  const auto projectors = s->extractor().projectors();
  REQUIRE(projectors.size() == 2); // on FALSE is skipped
  const ProjectorDesc &scoped = projectors[0], &global = projectors[1];
  CHECK_FALSE(scoped.global);
  // Default direction 0 0 1 and upVector 0 0 1 are parallel: up falls back to +Y.
  CHECK(global.worldUp.y == doctest::Approx(1));
  auto &ex = s->extractor();
  REQUIRE(ex.itemCount() == 2);
  int boxes = 0;
  for (extract::RenderItemId id = 0; id < ex.itemCount(); ++id) {
    const bool inA = ex.item(id).path.front() == s->scene().resolve("A").get();
    CHECK(extract::projectorApplies(scoped, ex.item(id)) == inA);
    CHECK(extract::projectorApplies(global, ex.item(id)));
    boxes += inA;
  }
  CHECK(boxes == 1);
}

TEST_CASE("Texture projectors: aspect ratio of a url image and the aspectRatio output") {
  SessionOptions options;
  options.textureResolver = [](const std::string &) {
    extract::TexturePixels pixels;
    pixels.width = 4;
    pixels.height = 1;
    pixels.rgba.assign(16, 255);
    return extract::TexturePixelResult::makeReady(std::move(pixels));
  };
  auto s = session("<TextureProjector DEF='U'><ImageTexture containerField='texture' "
                   "url='\"a.png\"'/></TextureProjector>"
                   "<TextureProjector DEF='P'><PixelTexture DEF='I' containerField='texture' "
                   "image='2 1 1 0 0'/></TextureProjector>",
                   std::move(options));
  const auto projectors = s->extractor().projectors();
  REQUIRE(projectors.size() == 2);
  CHECK(projectors[0].texture.resolvedPixels.ready());
  CHECK(projectors[0].aspectRatio == doctest::Approx(4));

  auto aspect = [&](const char *def) {
    return geombounds::getField<float>(*s->scene().resolve(def), "aspectRatio", -1);
  };
  s->tick(0);
  CHECK(aspect("P") == doctest::Approx(2));
  x3d::core::SFImage tall{1, 4, 1, {0, 0, 0, 0}};
  s->context().postEvent(s->scene().resolve("I").get(), "image", std::any(tall));
  s->tick(0.1);
  CHECK(aspect("P") == doctest::Approx(0.25f));
}
