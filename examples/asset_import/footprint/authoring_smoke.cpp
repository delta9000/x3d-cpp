// Minimal authoring embed: builds a one-Shape doc and serializes it, using only
// x3d/authoring.hpp. The footprint gate scans this binary's symbols.
#include "x3d/authoring.hpp"
#include <iostream>

int main() {
  namespace a = x3d::authoring;
  a::X3DDocument doc;
  doc.version = "4.0";
  doc.profile = a::Profile::Interchange;
  auto shape = std::make_shared<x3d::nodes::Shape>();
  shape->setGeometry(std::make_shared<x3d::nodes::Box>());
  doc.scene.addRootNode(shape);
  const std::string xml = a::XmlWriter{}.writeDocument(doc);
  std::cout << xml.size() << "\n";
  return xml.empty() ? 1 : 0;
}
