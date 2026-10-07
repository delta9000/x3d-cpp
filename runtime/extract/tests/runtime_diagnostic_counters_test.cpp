#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Transform.hpp"
#include "doctest/doctest.h"

#include <array>
#include <barrier>
#include <thread>
#include <type_traits>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;

static_assert(std::is_same_v<decltype(TransformSystem::localMatrixCallCount()), std::uint64_t>);
static_assert(std::is_same_v<decltype(X3DExecutionContext::pickCallCount()), std::uint64_t>);

TEST_CASE("Runtime diagnostics: distinct owner-thread worlds share race-free counters") {
  constexpr std::size_t owners = 4;
  constexpr std::size_t iterations = 5000;
  std::barrier phase(static_cast<std::ptrdiff_t>(owners + 1));
  std::array<bool, owners> correct{};
  std::vector<std::thread> threads;
  for (std::size_t owner = 0; owner < owners; ++owner) {
    threads.emplace_back([&, owner] {
      // Every mutable scene, node and context is created, used and destroyed on
      // its own owner thread. Only the process-wide diagnostic counters overlap.
      Scene scene;
      auto transform = std::make_shared<x3d::nodes::Transform>();
      transform->setTranslation(SFVec3f{static_cast<float>(owner), 0, 0});
      scene.rootNodes = {transform};
      X3DExecutionContext context;
      context.buildSceneGraph(scene);
      (void)context.pick(Ray{}); // warm the independent pick index before sampling
      phase.arrive_and_wait();
      phase.arrive_and_wait();
      bool matches = true;
      for (std::size_t i = 0; i < iterations; ++i) {
        const auto matrix = TransformSystem::localMatrix(transform.get());
        matches &= matrix.m[12] == static_cast<float>(owner);
        (void)context.pick(Ray{});
      }
      correct[owner] = matches;
    });
  }
  phase.arrive_and_wait();
  const auto matrices = TransformSystem::localMatrixCallCount();
  const auto picks = X3DExecutionContext::pickCallCount();
  phase.arrive_and_wait();
  for (auto &thread : threads) thread.join();
  for (bool matches : correct) CHECK(matches);
  CHECK(TransformSystem::localMatrixCallCount() - matrices == owners * iterations);
  CHECK(X3DExecutionContext::pickCallCount() - picks == owners * iterations);
}
