#include "doctest/doctest.h"
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

// Inline expansion terminates on a self-referential containment graph.
TEST_CASE("inline_containment_cycle_test") {
  std::string dir = X3D_TEST_DATA_DIR;
  // Must terminate without stack overflow/SIGSEGV and return a valid document.
  auto doc = x3d::sdk::parseFile(dir + "/cycle_containment.x3d");
  std::cout << "inline_containment_cycle_test OK (terminated, scene roots="
            << doc.scene.rootNodes.size() << ")\n";

  // Parsing preserves the shared_ptr cycle; break it before leaving scope
  // because this test does not construct an execution context to sanitize it.
  x3d::runtime::breakContainmentCycles(doc.scene);
}
