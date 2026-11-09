// Adapt the XML codec to the X3DReader interface.
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
