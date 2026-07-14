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
//      (NOT &quot; — single-quoted attrs need &apos; for the apostrophe).
//   6. 2-space-per-level indentation; one element per line.
//   7. Minimal number formatting: shortest round-trip float/double
//      representation (0.8 not 0.800000, 0 not 0.0, integer via to_string).
//   8. DEF/USE, ProtoDeclare/field/connect, ROUTE, IMPORT/EXPORT, head meta,
//      xmlns:xsd and xsd:noNamespaceSchemaLocation on <X3D> all preserved.
//
// The DEFAULT XmlWriter path is COMPLETELY UNCHANGED — this is a separate
// class that makes no modifications to any shared code paths.
//
// Public declaration, namespace x3d::codec. The implementation is compiled in
// x3d_cpp_authoring_runtime.
#ifndef X3D_CANONICAL_XML_WRITER_HPP
#define X3D_CANONICAL_XML_WRITER_HPP

#include <any>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace x3d::nodes {
class Script;
class X3DNode;
} // namespace x3d::nodes

namespace x3d::runtime {
class ExternProtoDeclaration;
class IsConnection;
class ProtoDeclaration;
class ProtoField;
class ProtoInstance;
class Scene;
class X3DDocument;
} // namespace x3d::runtime

namespace x3d::core {
enum class X3DFieldType;
struct FieldInfo;
} // namespace x3d::core

namespace x3d::xml {
struct Element;
} // namespace x3d::xml

namespace x3d::codec {
using namespace x3d::core;
using x3d::nodes::Script;
using x3d::nodes::X3DNode;

// ---------------------------------------------------------------------------
// X3DC14N attribute escaping (single-quote context):
//   &amp; for &, &lt; for <, &gt; for >, &apos; for ' (the delimiter).
//   Double-quote does NOT need escaping in single-quote context.
// ---------------------------------------------------------------------------
std::string canonEscape(const std::string &s);

// ---------------------------------------------------------------------------
// X3DC14N minimal number formatting.
//
// Uses std::to_chars with chars_format::general to produce the shortest
// decimal string that round-trips back to the same float/double value.
// Integers are emitted without a decimal point (0, 1, 2 not 0.0, 1.0).
// This matches X3DJSAIL's behaviour: `0.9607843` not `0.960784316`.
// ---------------------------------------------------------------------------

/// Shortest round-trip float representation, X3DC14N style.
std::string canonFmtFloat(float v);

/// Shortest round-trip double representation, X3DC14N style.
///
/// Unlike floats, doubles in X3D (SFDouble, SFTime) MUST retain a decimal
/// point even for whole-number values — `0.0` not `0`, `1.0` not `1`.
/// X3DJSAIL always produces `0.0` for double zero; dropping the decimal
/// would cause a type mismatch when re-parsed (XmlReader treats `0` as
/// SFInt32 context; the spec says SFDouble fields serialise with a decimal).
/// This is also required for Tier-1 idempotence: `0` re-parses as `0` then
/// emits as `0` (OK), but some readers would treat `0.0` and `0` differently
/// in MF contexts. The safest and X3DC14N-correct choice is always decimal.
std::string canonFmtDouble(double v);

// ---------------------------------------------------------------------------
// X3DC14N formatValue: like FieldValueIO::formatValue but uses canonical
// (shortest round-trip) number formatting.
// ---------------------------------------------------------------------------

std::string canonFormatValue(X3DFieldType type, const std::any &v);

// ---------------------------------------------------------------------------
// X3DC14N DOCTYPE line for a given version string.
// Covers the X3D 3.x and 4.x versions encountered in the wild.
// ---------------------------------------------------------------------------
std::string x3dDoctype(const std::string &version);

// ---------------------------------------------------------------------------
// X3DC14N schema location for a given version.
// ---------------------------------------------------------------------------
std::string x3dSchemaLocation(const std::string &version);

/// Serializes the runtime document model to X3D Canonical Form (X3DC14N).
///
/// Design: build an xml::Element tree (same as XmlWriter) then render it with
/// canonical rules: sorted attrs, single-quote, 2-space indent. The tree build
/// reuses the same logic as XmlWriter but is fully self-contained so the
/// default writer path has zero risk of change.
class CanonicalXmlWriter {
public:
  /// Serialize a full document to its X3D Canonical Form string.
  std::string writeDocument(const runtime::X3DDocument &doc);

private:
  std::unordered_set<const X3DNode *> seen_;
  std::unordered_map<std::string, std::shared_ptr<X3DNode>> defaults_;
  const runtime::Scene *scene_ = nullptr;
  const std::vector<runtime::IsConnection> *bodyIsc_ = nullptr;

  void writeSceneInto(xml::Element *scene, const runtime::Scene &s);

  X3DNode *defaultFor(const std::string &typeName);

  std::unique_ptr<xml::Element>
  writeNodeElement(const std::shared_ptr<X3DNode> &node,
                   const std::string &containerOverride);

  void writeScriptAuthorFields(xml::Element &el, const Script &script);

  void writeNodeField(xml::Element &parent,
                      const std::shared_ptr<X3DNode> &owner,
                      const FieldInfo &f);

  std::unique_ptr<xml::Element>
  writeProtoInstanceElement(const runtime::ProtoInstance &src,
                            const std::string &containerOverride);

  std::unique_ptr<xml::Element>
  writeProtoFieldElement(const runtime::ProtoField &f);

  std::unique_ptr<xml::Element>
  writeProtoDeclareElement(const runtime::ProtoDeclaration &d);

  std::unique_ptr<xml::Element>
  writeExternProtoDeclareElement(const runtime::ExternProtoDeclaration &d);

  static void attachIsBlocks(xml::Element &nodeEl,
                             const std::shared_ptr<X3DNode> &node,
                             const std::vector<runtime::IsConnection> &isc);

  static const runtime::ProtoField *
  interfaceField(const runtime::ProtoInstance &src, const std::string &name);

  // ── Canonical rendering ──────────────────────────────────────────────────

  static void indentCan(std::ostringstream &os, int depth);

  /// Collect and sort attrs alphabetically (stable — attr names are unique per
  /// XML element; for canonical form identical-name attrs would be an error).
  static std::vector<std::pair<std::string, std::string>>
  sortedAttrs(const std::vector<std::pair<std::string, std::string>> &attrs);

  void renderCanonical(const xml::Element &el, std::ostringstream &os,
                       int depth);
};

} // namespace x3d::codec

#endif // X3D_CANONICAL_XML_WRITER_HPP
