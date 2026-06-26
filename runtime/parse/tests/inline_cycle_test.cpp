#include "x3d/sdk.hpp"
#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  assert(argc >= 2);
  std::string dir = argv[1];
  // a -> b -> a. Must terminate (no stack overflow / hang) and not crash.
  auto doc = x3d::sdk::parseFile(dir + "/cycle_a.x3d");
  // The outermost Inline resolves b; b's Inline back to a is refused by the
  // cycle guard (returns null) -> b loads with an un-expanded Inline, which the
  // outer a splices in. Either way: we got here, so it terminated.
  std::cout << "inline_cycle_test OK (terminated, scene roots="
            << doc.scene.rootNodes.size() << ")\n";
  return 0;
}
