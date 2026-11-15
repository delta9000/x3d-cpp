// CanonicalXmlWriter.hpp
// X3D Canonical Form (X3DC14N) serializer.
//
// Produces the deterministic X3D Canonical Form as defined by X3DJSAIL
// (-canonical mode / X3dCanonicalizer). Rules:
//   1. XML prolog: <?xml version="1.0" encoding="UTF-8"?>
//   2. X3D DOCTYPE line (version-appropriate).
//   3. Attributes sorted alphabetically by name within each element.
//   4. Single-quote attribute delimiters.
//   5. X3DC14N escaping in attribute values: &amp; &lt; &gt; &apos;
//      (not &quot; — single-quoted attrs need &apos; for the apostrophe).
//   6. 2-space-per-level indentation; one element per line.
//   7. Minimal number formatting: shortest round-trip float/double
//      representation (0.8 not 0.800000, 0 not 0.0, integer via to_string).
//   8. DEF/USE, ProtoDeclare/field/connect, ROUTE, IMPORT/EXPORT, head meta,
//      xmlns:xsd and xsd:noNamespaceSchemaLocation on <X3D> all preserved.
//
// See docs/wiki/explanation/decisions/0008-canonicalize-source-provenance.md (ADR-0008).
#ifndef X3D_CANONICAL_XML_WRITER_HPP
#define X3D_CANONICAL_XML_WRITER_HPP

#include <any>
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

namespace x3d::core {
enum class X3DFieldType;
} // namespace x3d::core

namespace x3d::xml {
struct Element;
} // namespace x3d::xml

namespace x3d::codec {
using namespace x3d::core;
using x3d::nodes::X3DNode;
// X3DC14N attribute escaping (single-quote context):
//   &amp; for &, &lt; for <, &gt; for >, &apos; for ' (the delimiter).
//   Double-quote does not need escaping in single-quote context.
std::string canonEscape(const std::string &s);
// X3DC14N minimal number formatting.
//
// Uses std::to_chars with chars_format::general to produce the shortest
// decimal string that round-trips back to the same float/double value.
// Integers are emitted without a decimal point (0, 1, 2 not 0.0, 1.0).
// This matches X3DJSAIL's behaviour: `0.9607843` not `0.960784316`.

/// Shortest round-trip float representation, X3DC14N style.
std::string canonFmtFloat(float v);

/// Canonical doubles retain a decimal point for integral values, including 0.0.
std::string canonFmtDouble(double v);
// X3DC14N formatValue: like FieldValueIO::formatValue but uses canonical
// (shortest round-trip) number formatting.

std::string canonFormatValue(X3DFieldType type, const std::any &v);
// X3DC14N DOCTYPE line for a given version string.
// Covers the X3D 3.x and 4.x versions encountered in the wild.
std::string x3dDoctype(const std::string &version);
// X3DC14N schema location for a given version.
std::string x3dSchemaLocation(const std::string &version);

class CanonicalXmlWriter {
public:
  std::string writeDocument(const runtime::X3DDocument &doc);

private:
  static void indentCan(std::ostringstream &os, int depth);

  void renderCanonical(const xml::Element &el, std::ostringstream &os,
                       int depth);
};

} // namespace x3d::codec

#endif // X3D_CANONICAL_XML_WRITER_HPP
