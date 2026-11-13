#include "doctest/doctest.h"
#include "x3d/sdk.hpp"
#include <iostream>
#include <string>

TEST_CASE("an Inline cycle a -> b -> a terminates") {
  std::string dir = X3D_TEST_DATA_DIR;
  // a -> b -> a. Must terminate (no stack overflow / hang) and not crash.
  auto doc = x3d::sdk::parseFile(dir + "/cycle_a.x3d");
  // The outermost Inline resolves b; b's Inline back to a is refused by the
  // cycle guard (returns null) -> b loads with an un-expanded Inline, which the
  // outer a splices in. Either way: we got here, so it terminated.
  CHECK(doc.scene.rootNodes.size() >= 1);
}
