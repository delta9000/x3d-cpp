// X3DReader.hpp
// The unified parsing front-end interface every encoding reader implements.
//
// A reader turns in-memory document text of one X3D encoding into the runtime
// document model (x3d::runtime::X3DDocument): Head metadata, a Scene with
// DEF/USE identity, ROUTEs, and IMPORT/EXPORT. Concrete readers
// (XmlReaderAdapter, ClassicVrmlReader, Vrml97Reader, JsonReader) implement
// this and nothing else, so the front door (X3DParse.hpp) can dispatch on a
// sniffed Encoding without knowing the concrete type.
//
// Header-only, namespace x3d::codec. Stateless per call.
#ifndef X3D_PARSE_X3D_READER_HPP
#define X3D_PARSE_X3D_READER_HPP

#include "Encoding.hpp"
#include "X3DRuntime.hpp" // x3d::runtime::X3DDocument

#include <string>

namespace x3d::codec {

/// Common front-end for every encoding reader.
class X3DReader {
public:
  virtual ~X3DReader() = default;

  /// The encoding this reader handles.
  virtual Encoding encoding() const = 0;

  /// Parse a complete document from in-memory text. Throws std::runtime_error
  /// only on unrecoverable malformation; unknown node/field names are skipped
  /// gracefully (consistent with XmlReader). The returned document already has
  /// scene.resolveRoutes() applied.
  virtual runtime::X3DDocument readDocument(const std::string &text) = 0;
};

} // namespace x3d::codec

#endif // X3D_PARSE_X3D_READER_HPP
