#include "doctest/doctest.h"
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

// Inline expansion terminates on a self-referential containment graph.
TEST_CASE("an Inline containment cycle terminates") {
  std::string dir = X3D_TEST_DATA_DIR;
  // Must terminate without stack overflow/SIGSEGV and return a valid document.
  auto doc = x3d::sdk::parseFile(dir + "/cycle_containment.x3d");
  CHECK(doc.scene.rootNodes.size() >= 1);

  // Parsing preserves the shared_ptr cycle; break it before leaving scope
  // because this test does not construct an execution context to sanitize it.
  x3d::runtime::breakContainmentCycles(doc.scene);
}
