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
// Header-only, namespace x3d::codec. Implements the X3DReader front-end so
// X3DParse.hpp dispatches a sniffed Encoding::VRML97 here.
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
  Encoding encoding() const override { return Encoding::VRML97; }

  /// Parse a complete VRML97 document. Returns a fully-built X3DDocument with
  /// routes resolved. In strict mode any collected warning is rethrown.
  runtime::X3DDocument readDocument(const std::string &text) override {
    warnings_.clear();
    dialectOn_ =
        true; // reset; onHeaderLine() may turn it off for a #X3D header
    runtime::X3DDocument doc = ClassicVrmlReader::readDocument(text);
    if (strict_ && !warnings_.empty())
      throw std::runtime_error("Vrml97Reader (strict): " + warnings_.front());
    return doc;
  }

  /// Promote collected warnings to a throw at end of parse (for CI fixtures
  /// that must parse cleanly). Off by default.
  void setStrict(bool strict) { strict_ = strict; }
  bool strict() const { return strict_; }

  /// Non-fatal issues collected during the last readDocument() call (unknown
  /// nodes/fields, charset notes, header anomalies).
  const std::vector<std::string> &warnings() const { return warnings_; }

protected:
  // -------------------------------------------------------------------------
  // Dialect hooks (override the identity base).
  // -------------------------------------------------------------------------

  std::string mapNodeName(const std::string &token) const override {
    return dialectOn_ ? vrml97::mapNodeName(token) : token;
  }

  std::string mapFieldName(const std::string &nodeType,
                           const std::string &token) const override {
    return dialectOn_ ? vrml97::mapFieldName(nodeType, token) : token;
  }

  void warn(const std::string &message) override {
    warnings_.push_back(message);
  }

  /// Header handling per the VRML97 reader spec:
  ///   `#VRML V2.0 <charset>` -> accept; default profile Immersive; a charset
  ///       other than utf8 is a warning (parsed as UTF-8 regardless).
  ///   `#VRML V1.0 ...`        -> throw (VRML 1.0 semantics unsupported).
  ///   `#X3D V3/V4 ...`        -> it is really ClassicVRML; switch the remap
  ///   OFF
  ///       so the same entry point reads `.x3dv` correctly.
  ///   missing/garbled         -> warn, assume VRML97, attempt the parse.
  void onHeaderLine(std::string_view src, runtime::X3DDocument &doc) override {
    std::string_view line = firstLine(src);
    std::vector<std::string> parts = splitOnSpace(line);

    if (parts.empty() || parts[0].empty() || parts[0][0] != '#') {
      warn("missing VRML header line; assuming VRML97 (#VRML V2.0 utf8)");
      doc.profile = runtime::Profile::Immersive;
      doc.version = "3.0";
      return;
    }

    const std::string &magic = parts[0]; // e.g. "#VRML" or "#X3D"
    const std::string version = parts.size() >= 2 ? parts[1] : std::string();
    const std::string charset = parts.size() >= 3 ? parts[2] : std::string();

    if (magic == "#X3D") {
      // A ClassicVRML payload behind a .wrl/sniff: turn the VRML97 remap off so
      // names resolve directly. (doc.version was already set by
      // parseHeaderLine.)
      dialectOn_ = false;
      return;
    }

    if (magic != "#VRML") {
      warn("unrecognized header '" + magic + "'; assuming VRML97");
      doc.profile = runtime::Profile::Immersive;
      doc.version = "3.0";
      return;
    }

    // #VRML — inspect the version.
    if (version == "V1.0" || version == "v1.0")
      throw std::runtime_error(
          "VRML 1.0 not supported (this reader handles VRML97 / V2.0)");

    if (version != "V2.0" && version != "v2.0")
      warn("unexpected VRML version '" + version + "'; parsing as VRML97");

    if (!charset.empty() && charset != "utf8" && charset != "UTF8" &&
        charset != "utf-8" && charset != "UTF-8")
      warn("non-utf8 charset '" + charset + "'; parsing as UTF-8");

    // VRML97 has no profile/component concept; it maps to the X3D Immersive
    // baseline. (Writers emit an X3D header on output regardless.)
    doc.profile = runtime::Profile::Immersive;
    doc.version = "3.0";
  }

private:
  /// Split a line into whitespace-delimited tokens (header lexing only).
  static std::vector<std::string> splitOnSpace(std::string_view line) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : line) {
      if (c == ' ' || c == '\t') {
        if (!cur.empty()) {
          parts.push_back(cur);
          cur.clear();
        }
      } else {
        cur += c;
      }
    }
    if (!cur.empty())
      parts.push_back(cur);
    return parts;
  }

  bool strict_ = false;
  bool dialectOn_ = true;
  std::vector<std::string> warnings_;
};

} // namespace x3d::codec

#endif // X3D_PARSE_VRML97_READER_HPP
