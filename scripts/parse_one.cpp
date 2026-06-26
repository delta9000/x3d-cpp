// parse_one.cpp — parse a single file and print the exact error. Diagnostic only.
#include "X3DParse.hpp"
#include <iostream>
int main(int argc, char **argv) {
  if (argc < 2) { std::cerr << "usage: parse_one <file>\n"; return 2; }
  try {
    auto doc = x3d::codec::parseFile(argv[1]);
    std::cout << "OK: " << argv[1] << "  roots=" << doc.scene.rootNodes.size() << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cout << "ERR: " << argv[1] << "\n     " << e.what() << "\n";
    return 1;
  }
}
