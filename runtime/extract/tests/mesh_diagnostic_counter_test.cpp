#include "MeshBuilder.hpp"
#include "x3d/nodes/Box.hpp"
#include "doctest/doctest.h"

#include <array>
#include <barrier>
#include <thread>
#include <type_traits>
#include <vector>

using namespace x3d::runtime;
static_assert(std::is_same_v<decltype(extract::buildLocalMeshCallCount()), std::uint64_t>);

TEST_CASE("Mesh diagnostics: independent geometries share a race-free counter") {
  // Warm immutable generated reflection before disjoint-world work; this
  // test targets counter sharing, not generated-table initialization.
  x3d::nodes::Box warm;
  (void)extract::buildLocalMesh(&warm);
  constexpr std::size_t owners = 4, iterations = 1000;
  std::barrier phase(static_cast<std::ptrdiff_t>(owners + 1));
  std::array<bool, owners> correct{};
  std::vector<std::thread> threads;
  for (std::size_t owner = 0; owner < owners; ++owner) {
    threads.emplace_back([&, owner] {
      x3d::nodes::Box box;
      bool recognized = false;
      (void)extract::buildLocalMesh(&box, {}, &recognized);
      phase.arrive_and_wait();
      phase.arrive_and_wait();
      bool matches = recognized;
      for (std::size_t i = 0; i < iterations; ++i) {
        const auto mesh = extract::buildLocalMesh(&box, {}, &recognized);
        matches &= recognized && !mesh.positions.empty();
      }
      correct[owner] = matches;
    });
  }
  phase.arrive_and_wait();
  const auto before = extract::buildLocalMeshCallCount();
  phase.arrive_and_wait();
  for (auto &thread : threads) thread.join();
  for (bool matches : correct) CHECK(matches);
  CHECK(extract::buildLocalMeshCallCount() - before == owners * iterations);
}
