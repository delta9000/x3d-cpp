#include "XmlReaderAdapter.hpp"

namespace x3d::codec {

Encoding XmlReaderAdapter::encoding() const { return Encoding::XML; }

runtime::X3DDocument XmlReaderAdapter::readDocument(const std::string &text) {
  return reader_.readDocument(text);
}

} // namespace x3d::codec
