#include "x3d/authoring.hpp"

#include <memory>

int main() {
  x3d::authoring::X3DDocument doc;
  doc.profile = x3d::authoring::Profile::Interchange;
  doc.scene.rootNodes.push_back(std::make_shared<x3d::nodes::Box>());
  return x3d::authoring::XmlWriter{}.writeDocument(doc).empty() ? 1 : 0;
}
