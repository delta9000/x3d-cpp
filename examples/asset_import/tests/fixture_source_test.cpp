#include "import_source.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

TEST_CASE("ir_default_scene_is_empty") {
  ImportScene s;
  CHECK(s.rootNode == -1);
  CHECK(s.meshes.empty());
}
