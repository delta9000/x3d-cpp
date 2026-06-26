// examples/01_load_validate_convert.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Headless LOAD → VALIDATE → CONVERT using only the x3d::sdk façade.
//
//   • Load any of the 4 X3D encodings (XML/ClassicVRML/VRML97/JSON, + gzip)
//     from a file path, or fall back to a built-in inline scene.
//   • Report conformance signals: version/profile, range warnings (lenient read),
//     PROTO/EXTERNPROTO expansion diagnostics.
//   • Convert the loaded document to all three writable encodings and print a
//     short preview of each.
//
// Build: configured automatically when X3D_CPP_BUILD_EXAMPLES=ON.
// Run:   01_load_validate_convert [path/to/scene.x3d|.x3dv|.wrl|.json]
// ─────────────────────────────────────────────────────────────────────────────
#include "x3d/sdk.hpp"

#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static const char *kInlineScene = R"X3D(<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <head><meta name="title" content="facade demo"/></head>
  <Scene>
    <Viewpoint position="0 0 8"/>
    <Transform translation="0 1 0">
      <Shape>
        <Appearance><Material diffuseColor="0.2 0.6 0.9"/></Appearance>
        <Sphere radius="1.5"/>
      </Shape>
    </Transform>
  </Scene>
</X3D>)X3D";

static void preview(const std::string &label, const std::string &text) {
  std::cout << "\n--- " << label << " (" << text.size() << " bytes) ---\n";
  std::string head = text.substr(0, 240);
  std::cout << head;
  if (text.size() > head.size())
    std::cout << "\n... [truncated]";
  std::cout << "\n";
}

int main(int argc, char **argv) {
  // 1. LOAD. parseFile sniffs encoding + inflates gzip; parseDocument takes text.
  sdk::X3DDocument doc;
  try {
    if (argc > 1) {
      std::cout << "Loading " << argv[1] << " ...\n";
      doc = sdk::parseFile(argv[1]);
    } else {
      std::cout << "No path given; using the built-in inline scene.\n";
      doc = sdk::parseDocument(kInlineScene, sdk::Encoding::XML);
    }
  } catch (const std::exception &e) {
    std::cerr << "Load failed: " << e.what() << "\n";
    return 1;
  }

  // 2. VALIDATE / report conformance signals.
  std::cout << "\n== Document ==\n";
  std::cout << "version : " << doc.version << "\n";
  std::cout << "profile : " << doc.profileName() << "\n";
  std::cout << "roots   : " << doc.scene.rootNodes.size() << "\n";
  std::cout << "defs    : " << doc.scene.defs.size() << "\n";
  std::cout << "routes  : " << doc.scene.routes.size() << "\n";

  std::cout << "\n== Conformance ==\n";
  std::cout << "rangeWarnings : " << doc.rangeWarnings.size() << "\n";
  for (const auto &w : doc.rangeWarnings)
    std::cout << "  - " << w.message() << "\n";
  std::cout << "protoWarnings : " << doc.protoWarnings.size() << "\n";

  // 3. CONVERT to each writable encoding (reflection-driven writers).
  preview("X3D-XML", sdk::XmlWriter{}.writeDocument(doc));
  preview("X3D-JSON", sdk::JsonWriter{}.writeDocument(doc));
  preview("ClassicVRML", sdk::VrmlWriter{}.writeDocument(doc));

  std::cout << "\nOK\n";
  return 0;
}
