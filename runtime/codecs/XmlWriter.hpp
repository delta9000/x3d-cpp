// X3D-XML writer. Fields equal to factory defaults are omitted.
// Shared node identities emit DEF on first use and USE on subsequent visits.

#ifndef X3D_XML_WRITER_HPP
#define X3D_XML_WRITER_HPP

#include <iosfwd>
#include <memory>
#include <string>

namespace x3d::nodes {
class X3DNode;
} // namespace x3d::nodes

namespace x3d::runtime {
class Scene;
class DynamicFieldStore;
class X3DDocument;
} // namespace x3d::runtime

namespace x3d::xml {
struct Element;
} // namespace x3d::xml

namespace x3d::codec {
using x3d::nodes::X3DNode;

class XmlWriter {
public:
  /// Serialize a full document to an X3D-XML string (with <?xml?> prolog).
  std::string writeDocument(const runtime::X3DDocument &doc);

  /// Serialize just a Scene's children/routes (no <X3D> wrapper).
  std::string writeScene(const runtime::Scene &scene);

  /// Serialize a single subtree. Pass its owner to include author fields;
  /// without an owner this is generated-fields-only (plus Script source).
  std::string
  writeNode(const std::shared_ptr<X3DNode> &node,
            const runtime::DynamicFieldStore *authorFields = nullptr);

private:
  static void indent(std::ostringstream &os, int depth);

  void render(const xml::Element &el, std::ostringstream &os, int depth);
};

} // namespace x3d::codec

#endif // X3D_XML_WRITER_HPP
