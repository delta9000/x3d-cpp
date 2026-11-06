// VRML97 reader (ISO/IEC 14772-1). Unknown node and field names are skipped
// and reported in warnings. Strict mode promotes warnings to exceptions.
// Unrecoverable syntax errors and unsupported headers always throw.

#ifndef X3D_PARSE_VRML97_READER_HPP
#define X3D_PARSE_VRML97_READER_HPP

#include "ClassicVrmlReader.hpp"

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
  // Dialect hooks (override the identity base).

  std::string mapFieldName(const std::string &nodeType,
                           const std::string &token) const override;

  void warn(const std::string &message) override;

  /// Header handling per the VRML97 reader spec:
  ///   `#VRML V2.0 <charset>` -> accept; default profile Immersive; a charset
  ///       other than utf8 is a warning (parsed as UTF-8 regardless).
  ///   `#VRML V1.0 ...`        -> throw (VRML 1.0 semantics unsupported).
  ///   `#X3D V3/V4 ...`        -> it is really ClassicVRML; switch the remap
  ///   off
  ///       so the same entry point reads `.x3dv` correctly.
  ///   missing/garbled         -> warn, assume VRML97, attempt the parse.
  void onHeaderLine(const std::vector<std::string> &parts,
                     runtime::X3DDocument &doc) override;

private:
  bool strict_ = false;
  bool dialectOn_ = true;
  std::vector<std::string> warnings_;
};

} // namespace x3d::codec

#endif // X3D_PARSE_VRML97_READER_HPP
