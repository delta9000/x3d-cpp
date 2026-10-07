#include "X3DDocument.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include "x3d/sai_provider.hpp"
#include <iostream>

#ifdef X3D_SAI_EXPERIMENTAL_KERNEL_HPP
#error "The independent native provider must not include the reference kernel"
#endif
#ifdef X3D_SAI_EXPERIMENTAL_METADATA_HPP
#error "The independent native provider must not include reference metadata"
#endif

namespace sai = x3d::sai::experimental;
static void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

// This is intentionally separate from the common backend-blind oracle.
static void native_authority_proof() {
  x3d::runtime::SaiOfflineProvider provider;
  auto scene_result = provider.native_scene();
  require(bool(scene_result), "native extension must return the owned scene");
  auto scene = *scene_result;
  auto created = provider.create_node("Transform");
  require(bool(created), "native creation must succeed");
  require(bool(provider.define_name(*created, "NativeRoot")),
          "native naming must succeed");
  require(bool(provider.append_root(*created)) &&
              bool(provider.append_root(*created)),
          "native root occurrences must append");
  auto native = std::dynamic_pointer_cast<x3d::nodes::Transform>(
      scene->resolve("NativeRoot"));
  require(bool(native), "DEF must resolve to a real native Transform");
  require(
      scene->rootNodes.size() == 2 && scene->rootNodes[0] == native &&
          scene->rootNodes[1] == native,
      "native Scene DEF and both root occurrences must share the same node");
  require(native->getDEF() == "NativeRoot",
          "native node DEF must match Scene naming");
  require(
      bool(provider.write_field(*created, "translation", sai::vec3f{4, 5, 6})),
      "adapter write must succeed");
  require(native->getTranslation() == x3d::core::SFVec3f{4, 5, 6},
          "adapter must mutate the authoritative native node");
  require(scene->authoredScalarFields.contains(native, "translation"),
          "adapter must record native authored-field presence");
  native->setTranslation(x3d::core::SFVec3f{7, 8, 9});
  auto read = provider.read_field(*created, "translation");
  require(
      read && std::get<sai::vec3f>(*read) == sai::vec3f{7, 8, 9},
      "native mutation must be visible immediately without resynchronization");
  auto named = provider.lookup_name("NativeRoot");
  require(named && *named == *created,
          "native lookup must retain common identity");
  sai::result<std::shared_ptr<x3d::runtime::Scene>> off_thread = scene;
  std::thread other([&] { off_thread = provider.native_scene(); });
  other.join();
  require(!off_thread &&
              off_thread.error().code == sai::error_code::wrong_thread,
          "native extension must honor thread confinement too");
  require(bool(provider.close()), "native provider must close");
  require(created->expired() &&
              native->getTranslation() == x3d::core::SFVec3f{7, 8, 9},
          "retained native storage must not preserve SAI authority");
  auto closed = provider.native_scene();
  require(!closed && closed.error().code == sai::error_code::stale_handle,
          "closed provider must reject native extension access");
}

int main() {
  try {
    const auto report = sai::testing::run_provider_fixture(
        []() -> sai::result<std::unique_ptr<sai::offline_provider>> {
          return std::unique_ptr<sai::offline_provider>{
              std::make_unique<x3d::runtime::SaiOfflineProvider>()};
        });
    native_authority_proof();
    std::cout << "native offline provider: " << report.checks
              << " common checks plus two-way native authority proof passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
