#include "x3d/sdk.hpp"
#include <cassert>
#include <iostream>
#include <string>

// Regression test for expandInlines walk guard (stack-overflow on cyclic scenes).
// A scene with <Transform DEF="A"><Transform USE="A"/></Transform> contains a
// containment cycle: the unguarded walk lambda in expandInlines would recurse
// infinitely before the fix (walk A -> walk A's child which IS A -> walk A -> ...).
// After the fix the visited-set short-circuits on the second visit to A and the
// parse terminates normally.
int main(int argc, char** argv) {
  assert(argc >= 2);
  std::string dir = argv[1];
  // Must terminate without stack overflow/SIGSEGV and return a valid document.
  auto doc = x3d::sdk::parseFile(dir + "/cycle_containment.x3d");
  // If we reach here the fix is working — infinite recursion was prevented.
  std::cout << "inline_containment_cycle_test OK (terminated, scene roots="
            << doc.scene.rootNodes.size() << ")\n";
  return 0;
}
