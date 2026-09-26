// runtime/scene/tests/interface_registry_test.cpp
#include "x3d/nodes/X3DInterfaceRegistry.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "doctest/doctest.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace x3d::nodes;

TEST_CASE("interface_registry_test") {
  // String-keyed queries.
  CHECK((X3DInterfaceRegistry::nodeImplements("TimeSensor",
                                              InterfaceId::X3DTimeDependentNode)));
  CHECK((X3DInterfaceRegistry::nodeImplements("TimeSensor",
                                              InterfaceId::X3DSensorNode)));
  // Transitive closure: ProximitySensor is-a X3DEnvironmentalSensorNode and,
  // transitively, X3DSensorNode + X3DChildNode.
  CHECK((X3DInterfaceRegistry::nodeImplements(
      "ProximitySensor", InterfaceId::X3DEnvironmentalSensorNode)));
  CHECK((X3DInterfaceRegistry::nodeImplements("ProximitySensor",
                                              InterfaceId::X3DChildNode)));
  // Negative case.
  CHECK((!X3DInterfaceRegistry::nodeImplements("Box",
                                               InterfaceId::X3DSensorNode)));
  // Unknown type -> empty -> false (no crash).
  CHECK((!X3DInterfaceRegistry::nodeImplements("__nope__",
                                               InterfaceId::X3DChildNode)));
  // Live-node overload via the factory.
  auto n = createX3DNode("TimeSensor");
  CHECK((n));
  CHECK((X3DInterfaceRegistry::nodeImplements(n.get(),
                                              InterfaceId::X3DSensorNode)));

  // Concurrency: many threads racing the first build of the by-interface map
  // must not corrupt it. (The map is a function-local static, so C++11 makes
  // its initialization thread-safe; this exercises that path.)
  {
    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::vector<std::size_t> counts(kThreads, 0);
    for (int t = 0; t < kThreads; ++t)
      threads.emplace_back([&counts, t] {
        for (int r = 0; r < 500; ++r) {
          counts[t] = X3DInterfaceRegistry::nodesImplementing(
                          InterfaceId::X3DChildNode)
                          .size();
        }
      });
    for (auto& th : threads) th.join();
    CHECK(counts[0] > 0);
    for (std::size_t c : counts) CHECK(c == counts[0]);
  }

  std::cout << "interface_registry_test OK\n";
  return;
}
