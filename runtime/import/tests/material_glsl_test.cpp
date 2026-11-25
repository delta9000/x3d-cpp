// material_glsl_test.cpp — materialGlsl() bakes the material into the
// canonical UsdPreviewSurface shader.
#include "MaterialGlsl.hpp"

#include "doctest/doctest.h"

#include <string>

namespace imp = x3d::runtime::importer;

TEST_CASE("materialGlsl: scalar uniforms become constants") {
  imp::ImportMaterial m;
  m.pbr = imp::PbrParams{{0.25f, 0.5f, 0.75f, 1.0f}, 0.1f, 0.4f};
  m.ior = 1.25f;
  m.clearcoat = 0.5f;
  m.useSpecularWorkflow = true;
  const std::string glsl = imp::materialGlsl(m);
  CHECK(glsl.find("#version") != std::string::npos);
  CHECK(glsl.find("const float uIor = 1.25") != std::string::npos);
  CHECK(glsl.find("const float uClearcoat = 0.5") != std::string::npos);
  CHECK(glsl.find("uniform float uIor") == std::string::npos);
  // Samplers stay uniforms for the host to bind.
  CHECK(glsl.find("uniform sampler2D") != std::string::npos);
}
