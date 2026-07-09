#include "backend_registry.hpp"
#include "doctest/doctest.h"
using namespace x3d::asset_import;

// A backend that ignores its make() call; only priority/name matter for selection.
static ImportBackend fake(std::string n, std::function<int(const std::string&)> p) {
  return ImportBackend{std::move(n), std::move(p),
                       [] { return std::unique_ptr<ImportSource>{}; }};
}
static int ext_is(const std::string& in, const char* e, int prio) {
  auto d = in.find_last_of('.');
  return d != std::string::npos && in.substr(d) == e ? prio : -1;
}

TEST_CASE("registry selects highest positive priority") {
  BackendRegistry r;
  r.add(fake("cgltf", [](const std::string& i) { return ext_is(i, ".glb", 100); }));
  r.add(fake("assimp", [](const std::string& i) { return ext_is(i, ".glb", 10); }));
  REQUIRE(r.select("model.glb") != nullptr);
  CHECK(r.select("model.glb")->name == "cgltf"); // 100 beats 10
  CHECK(r.select("model.fbx") == nullptr);       // nobody claims it
}

TEST_CASE("byName overrides priority and ignores decline") {
  BackendRegistry r;
  r.add(fake("cgltf", [](const std::string& i) { return ext_is(i, ".glb", 100); }));
  r.add(fake("assimp", [](const std::string& i) { return ext_is(i, ".glb", 10); }));
  CHECK(r.byName("assimp") != nullptr); // forced even though cgltf outranks
  CHECK(r.byName("nope") == nullptr);
}

TEST_CASE("fixture prefix routes by whole input, not extension") {
  BackendRegistry r;
  r.add(fake("fixture",
             [](const std::string& i) { return i.rfind("fixture:", 0) == 0 ? 100 : -1; }));
  CHECK(r.select("fixture:cube") != nullptr);
  CHECK(r.select("fixture:cube")->name == "fixture");
  CHECK(r.select("plain.obj") == nullptr);
}

TEST_CASE("names lists registered backends in order") {
  BackendRegistry r;
  r.add(fake("cgltf", [](const std::string&) { return -1; }));
  r.add(fake("assimp", [](const std::string&) { return -1; }));
  CHECK(r.names() == std::vector<std::string>{"cgltf", "assimp"});
}
