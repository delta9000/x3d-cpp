// The README "hello world" (§Quickstart 2), built against the SLIM authoring
// surface: x3d/authoring.hpp + the x3d_cpp::authoring target (no runtime, no
// extraction). The README shows the same program with x3d/sdk.hpp — the
// graph-building lines are identical; only the umbrella header differs.
// verify_install_embed.sh runs this binary from a scratch dir and then feeds
// the hello.x3d it writes to x3d_embed_minimal (the consumer half).
#include "x3d/authoring.hpp"
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Sphere.hpp"

#include <fstream>
#include <memory>

int main() {
  auto ball = std::make_shared<x3d::nodes::Shape>();
  auto look = std::make_shared<x3d::nodes::Appearance>();
  auto red  = std::make_shared<x3d::nodes::Material>();
  red->setDiffuseColor({0.875f, 0.25f, 0.25f});
  look->setMaterial(red);
  ball->setAppearance(look);
  ball->setGeometry(std::make_shared<x3d::nodes::Sphere>());

  x3d::authoring::X3DDocument doc;
  doc.version = "4.0";
  doc.scene.rootNodes.push_back(ball);

  std::ofstream out("hello.x3d");
  out << x3d::authoring::XmlWriter{}.writeDocument(doc);
  return out.good() ? 0 : 1;
}
