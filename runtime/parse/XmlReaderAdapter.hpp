// XmlReaderAdapter.hpp
// Thin adapter that plugs the existing codecs/XmlReader into the unified
// parsing front-end. The XmlReader itself is NOT moved or modified; this just
// implements the X3DReader interface by delegating to it, so X3D-XML dispatches
// through the same front door (X3DParse.hpp) as the text/JSON readers.
//
// Public declaration, namespace x3d::codec. The implementation is compiled in
// x3d_cpp_runtime.
#ifndef X3D_PARSE_XML_READER_ADAPTER_HPP
#define X3D_PARSE_XML_READER_ADAPTER_HPP

#include "X3DReader.hpp"
#include "XmlReader.hpp" // codec::XmlReader (in runtime/codecs/)

#include <string>

namespace x3d::codec {

/// Wraps codec::XmlReader to satisfy the X3DReader front-end interface.
/// XmlReader::readDocument already applies scene.resolveRoutes(), so the
/// uniform front-end contract is met without extra work.
class XmlReaderAdapter : public X3DReader {
public:
  Encoding encoding() const override;

  runtime::X3DDocument readDocument(const std::string &text) override;

private:
  XmlReader reader_;
};

} // namespace x3d::codec

#endif // X3D_PARSE_XML_READER_ADAPTER_HPP
