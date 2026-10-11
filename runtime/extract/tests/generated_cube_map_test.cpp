// generated_cube_map_test.cpp — REQ-CUBE, §34.4.2 GeneratedCubeMapTexture.
//
// The extractor surfaces a generated cube (TextureRef::generatedCube: node,
// update, size) for the consumer to render, and GeneratedCubeMapSystem resets
// update NEXT_FRAME_ONLY to NONE at the start of the frame after the one that
// drew it, emitting update_changed. Live update edits reach the render feed.
#include "RuntimeSession.hpp"
#include "X3DParse.hpp"

#include "doctest/doctest.h"

#include <any>
#include <string>

using namespace x3d::runtime;
using extract::TextureRef;
using Update = x3d::core::GeneratedCubeMapTextureUpdateChoices;

namespace {

const TextureRef *cubeRef(RuntimeSession &s) {
  for (extract::RenderItemId id = 0; id < s.extractor().itemCount(); ++id)
    for (const TextureRef &t : s.extractor().item(id).material.textures)
      if (t.generatedCube.node) return &t;
  return nullptr;
}

std::string update(RuntimeSession &s, const char *def) {
  return enumToken(*s.scene().resolve(def), "update", "?");
}

} // namespace

TEST_CASE("GeneratedCubeMapTexture: descriptor and NEXT_FRAME_ONLY reset (REQ-CUBE)") {
  auto s = RuntimeSession::create(x3d::codec::parseDocument(R"(
<X3D profile='Full' version='4.0'><Scene>
<Shape><Appearance>
  <GeneratedCubeMapTexture DEF='G' update='NEXT_FRAME_ONLY' size='64'/>
</Appearance><Box/></Shape>
<Shape><Appearance><GeneratedCubeMapTexture DEF='H'/></Appearance>
  <Sphere/></Shape>
<ROUTE fromNode='G' fromField='update_changed' toNode='H' toField='set_update'/>
</Scene></X3D>)"));
  s->fullSnapshot();
  const TextureRef *ref = cubeRef(*s);
  REQUIRE(ref);
  CHECK(ref->source == TextureRef::Source::Cube);
  CHECK(ref->generatedCube.node == s->scene().resolve("G").get());
  CHECK(ref->generatedCube.size == 64);
  CHECK(ref->generatedCube.update == "NEXT_FRAME_ONLY");

  // The authored value is drawn after the first tick, reset at the second.
  s->tick(0);
  s->delta();
  CHECK(cubeRef(*s)->generatedCube.update == "NEXT_FRAME_ONLY");
  s->tick(0.1);
  CHECK(update(*s, "G") == "NONE");
  s->delta();
  CHECK(cubeRef(*s)->generatedCube.update == "NONE");

  // ALWAYS persists.
  auto *g = s->scene().resolve("G").get();
  s->context().postEvent(g, "update", std::any(Update::ALWAYS));
  s->tick(0.2);
  s->tick(0.3);
  CHECK(update(*s, "G") == "ALWAYS");
  CHECK(update(*s, "H") == "ALWAYS"); // update_changed was routed

  // An event during tick N is drawn after N and reset at the start of N + 1,
  // and the reset reaches ROUTEs as update_changed.
  s->context().postEvent(g, "update", std::any(Update::NEXT_FRAME_ONLY));
  s->tick(0.4);
  s->delta();
  CHECK(update(*s, "G") == "NEXT_FRAME_ONLY");
  CHECK(cubeRef(*s)->generatedCube.update == "NEXT_FRAME_ONLY");
  s->tick(0.5);
  CHECK(update(*s, "G") == "NONE");
  CHECK(update(*s, "H") == "NONE");
}
