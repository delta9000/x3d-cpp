// The "Build a scene in code" hello world (docs/wiki/how-to/build-a-scene-in-code.md), built against the slim authoring
// surface: x3d/authoring.hpp + the x3d_cpp::authoring target (no runtime, no
// extraction). x3d/sdk.hpp re-exports the same builder as sdk::SceneBuilder.
// verify_install_embed.sh runs this binary from a scratch dir and then feeds
// the hello.x3d it writes to x3d_embed_minimal (the consumer half).
#include "x3d/authoring.hpp"
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Sphere.hpp"

#include <fstream>

int main() {
  namespace nodes = x3d::nodes;
  x3d::authoring::SceneBuilder scene; // authoring default: version 4.0

  // add<T>() puts the child in the field its type belongs in (appearance,
  // material, geometry) and throws if the parent has no such field.
  auto ball = scene.add<nodes::Shape>();
  ball.add<nodes::Appearance>().add<nodes::Material>().with(
      [](nodes::Material &red) { red.setDiffuseColor({0.875f, 0.25f, 0.25f}); });
  ball.add<nodes::Sphere>();

  std::ofstream out("hello.x3d");
  out << x3d::authoring::XmlWriter{}.writeDocument(scene.document());
  return out.good() ? 0 : 1;
}
