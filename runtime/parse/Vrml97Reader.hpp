// Vrml97Reader.hpp
// Node-agnostic VRML97 (.wrl, ISO/IEC 14772-1) reader. A one-way VRML97 -> X3D
// bridge: it ingests a `#VRML V2.0 utf8` document into the existing runtime
// document model (x3d::runtime::X3DDocument), reusing the codec architecture
// (X3DNodeFactory, reflection FieldInfo thunks, FieldValueIO) so that ZERO
// per-node code is added. The output is a normal X3DDocument that the existing
// XML/JSON/VRML writers and the event runtime consume — VRML97 in, X3D out, no
// separate object model.
//
// VRML97 and X3D ClassicVRML share ~90% of their concrete syntax, so the
// tokenizer/parser/build core is inherited WHOLE from ClassicVrmlReader; this
// class only overrides the dialect hooks:
//   * mapNodeName / mapFieldName  -> the Vrml97Dialect rename table.
//   * onHeaderLine                -> accept `#VRML V2.0`, default profile to
//                                    Immersive, reject VRML 1.0, switch the
//                                    remap OFF for a stray `#X3D` header.
//   * warn                        -> collect non-fatal issues into warnings().
//
// The access-type keyword aliases (eventIn/eventOut/field/exposedField) and the
// TRUE/FALSE boolean literals are ALREADY handled in the shared base
// (ClassicVrmlReader::accessTypeFromString and FieldValueIO::parseValue
// respectively), so they need no override here.
//
// Tolerance: unknown node types / unknown field names are skipped gracefully
// (balanced block / value consumed) and recorded in warnings(), never thrown.
// setStrict(true) promotes any collected warning to a std::runtime_error at the
// end of readDocument(), for fixtures that must parse cleanly. Genuinely
// unrecoverable malformation (unbalanced braces, unterminated string) and an
// unsupported header throw regardless.
//
// Public declaration, namespace x3d::codec. The implementation is compiled in
// x3d_cpp_runtime.
#ifndef X3D_PARSE_VRML97_READER_HPP
#define X3D_PARSE_VRML97_READER_HPP

#include "ClassicVrmlReader.hpp"
#include "Encoding.hpp"
#include "Vrml97Dialect.hpp"
#include "X3DRuntime.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::codec {

/// Reads VRML97 (.wrl) into the runtime document model, applying the
/// VRML97->X3D dialect remap. Inherits the entire ClassicVRML grammar;
/// overrides only the dialect hooks.
class Vrml97Reader : public ClassicVrmlReader {
public:
  Encoding encoding() const override;

  /// Parse a complete VRML97 document. Returns a fully-built X3DDocument with
  /// routes resolved. In strict mode any collected warning is rethrown.
  runtime::X3DDocument readDocument(const std::string &text) override;

  /// Promote collected warnings to a throw at end of parse (for CI fixtures
  /// that must parse cleanly). Off by default.
  void setStrict(bool strict);
  bool strict() const;

  /// Non-fatal issues collected during the last readDocument() call (unknown
  /// nodes/fields, charset notes, header anomalies).
  const std::vector<std::string> &warnings() const;

protected:
  // -------------------------------------------------------------------------
  // Dialect hooks (override the identity base).
  // -------------------------------------------------------------------------

  std::string mapNodeName(const std::string &token) const override;

  std::string mapFieldName(const std::string &nodeType,
                           const std::string &token) const override;

  void warn(const std::string &message) override;

  /// Header handling per the VRML97 reader spec:
  ///   `#VRML V2.0 <charset>` -> accept; default profile Immersive; a charset
  ///       other than utf8 is a warning (parsed as UTF-8 regardless).
  ///   `#VRML V1.0 ...`        -> throw (VRML 1.0 semantics unsupported).
  ///   `#X3D V3/V4 ...`        -> it is really ClassicVRML; switch the remap
  ///   OFF
  ///       so the same entry point reads `.x3dv` correctly.
  ///   missing/garbled         -> warn, assume VRML97, attempt the parse.
  void onHeaderLine(std::string_view src, runtime::X3DDocument &doc) override;

private:
  /// Split a line into whitespace-delimited tokens (header lexing only).
  static std::vector<std::string> splitOnSpace(std::string_view line);

  bool strict_ = false;
  bool dialectOn_ = true;
  std::vector<std::string> warnings_;
};

} // namespace x3d::codec

#endif // X3D_PARSE_VRML97_READER_HPP
