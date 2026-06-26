// X3DHeader.hpp
// Hand-written runtime model for the X3D document <head> section:
// <component>, <unit>, and <meta> statements.
//
// These are X3D *statements*, not nodes (they do not derive from X3DNode).
// They are modeled here as plain value structs because a document needs to
// carry them and round-trip them through the XML / JSON / ClassicVRML codecs.
#ifndef X3D_RUNTIME_HEADER_HPP
#define X3D_RUNTIME_HEADER_HPP

#include <string>
#include <vector>

namespace x3d::runtime {

/**
 * @brief A <component> statement: requests a support level for an X3D component.
 * @details Encodes as `<component name='...' level='N'/>` in XML, or
 *          `COMPONENT name:N` in ClassicVRML.
 */
struct Component {
  std::string name;
  int level = 1;
};

/**
 * @brief A <unit> statement: redefines the conversion factor for a category.
 * @details `category` is one of "angle", "force", "length", "mass". Encodes as
 *          `<unit category='...' name='...' conversionFactor='...'/>` in XML, or
 *          `UNIT category name factor` in ClassicVRML.
 */
struct Unit {
  std::string category;        // e.g. "length"
  std::string name;            // e.g. "kilometer"
  double conversionFactor = 1.0;
};

/**
 * @brief A <meta> statement: free-form document metadata.
 * @details Encodes as `<meta name='...' content='...'/>` in XML, or
 *          `META "name" "content"` in ClassicVRML. The optional attributes
 *          (`dir`, `httpEquiv`, `lang`, `scheme`) mirror the UOM definition and
 *          are preserved verbatim for round-tripping; most documents only use
 *          name + content.
 */
struct Meta {
  std::string name;
  std::string content;
  std::string dir;        // optional
  std::string httpEquiv;  // optional (maps to XML "http-equiv")
  std::string lang;       // optional
  std::string scheme;     // optional
};

/**
 * @brief The X3D document <head>: component / unit / meta statement lists.
 */
struct Head {
  std::vector<Component> components;
  std::vector<Unit> units;
  std::vector<Meta> meta;

  bool empty() const {
    return components.empty() && units.empty() && meta.empty();
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_HEADER_HPP
