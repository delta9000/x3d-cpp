// registry_test.cpp — backend selection by extension, priority and name.
#include "AssetImport.hpp"

#include "doctest/doctest.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace imp = x3d::runtime::importer;

namespace {
struct Fixed : imp::ImportSource {
  imp::ImportScene load(const std::string &) override {
    imp::ImportScene s;
    s.warnings.push_back("fixed");
    return s;
  }
};
imp::ImportBackend backend(std::string name, int score) {
  return {std::move(name), {".x"}, [score](const std::string &p) { return imp::hasExtension(p, {".x"}) ? score : -1; },
          [] { return std::unique_ptr<imp::ImportSource>(std::make_unique<Fixed>()); }};
}
} // namespace

TEST_CASE("registry: extensions are compared without case") {
  CHECK(imp::lowerExtension("dir.v2/Model.GLB") == ".glb");
  CHECK(imp::lowerExtension("dir.v2/model") == "");
  CHECK(imp::hasExtension("a.GlTf", {".gltf", ".glb"}));
  CHECK_FALSE(imp::hasExtension("a.obj", {".gltf", ".glb"}));
}

TEST_CASE("registry: the highest score wins and ties go to the first added") {
  imp::BackendRegistry r;
  r.add(backend("low", 10));
  r.add(backend("high", 50));
  r.add(backend("tie", 50));
  REQUIRE(r.select("m.x") != nullptr);
  CHECK(r.select("m.x")->name == "high");
  CHECK(r.select("m.y") == nullptr);
  CHECK(r.byName("low") != nullptr);
  CHECK(r.byName("none") == nullptr);
}

TEST_CASE("registry: the built-in registry reads glTF") {
  const imp::BackendRegistry r = imp::builtinBackends();
  REQUIRE(r.select("scene.glb") != nullptr);
  CHECK(r.select("scene.glb")->name == "gltf");
  CHECK(r.select("scene.gltf")->name == "gltf");
  CHECK(r.select("scene.obj") == nullptr);
}

TEST_CASE("registry: importFile names the backends it has when none fits") {
  try {
    imp::importFile("model.obj");
    FAIL("expected an exception");
  } catch (const std::runtime_error &e) {
    const std::string msg = e.what();
    CHECK(msg.find("no import backend reads 'model.obj'") != std::string::npos);
    CHECK(msg.find("available: gltf") != std::string::npos);
  }
  imp::ImportOptions o;
  o.backend = "fbx-magic";
  CHECK_THROWS_WITH_AS(imp::importFile("model.glb", o), doctest::Contains("unknown import backend 'fbx-magic'"),
                       std::runtime_error);
}

TEST_CASE("registry: importFile uses the backend it selected and reports it") {
  imp::BackendRegistry r;
  r.add(backend("fixed", 1));
  imp::ImportOptions o;
  o.textures = false;
  const imp::ImportResult res = imp::importFile("m.x", o, r);
  CHECK(res.report.backend == "fixed");
  REQUIRE(res.report.warnings.size() == 1);
  CHECK(res.report.warnings[0] == "fixed");
}
