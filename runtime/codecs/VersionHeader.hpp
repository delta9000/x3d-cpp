// Floor emitted document versions at X3D 3.0, including hand-built documents.
#ifndef X3D_CODECS_VERSION_HEADER_HPP
#define X3D_CODECS_VERSION_HEADER_HPP

#include <charconv>
#include <string>

namespace x3d::codec {

/// X3D has no version below 3.0; floor a sub-3.0/legacy token to "3.0" so the
/// emitted header is always valid. >= 3.0 (incl. future) passes
/// through, and a token with no leading integer major floors to "3.0".
inline std::string headerVersion(const std::string &v) {
  int major = 0;
  const char *end = v.data() + v.size();
  if (std::from_chars(v.data(), end, major).ec != std::errc{})
    return "3.0"; // no leading integer major
  return (major < 3) ? std::string("3.0") : v;
}

} // namespace x3d::codec

#endif // X3D_CODECS_VERSION_HEADER_HPP
