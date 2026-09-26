// VersionHeader.hpp
// Shared codec helper: floor a document version token before it reaches an
// emitted header.
//
// X3D has no version below 3.0 (VP-2 §8). Readers floor a legacy/sub-3.0 token
// at read time, but an X3DDocument may also be built by hand with version "2.0"
// (or empty). All three primary writers route doc.version through
// headerVersion() so the emitted X3D-VRML / X3D-XML / X3D-JSON header is always
// valid, regardless of how the document was constructed.
#ifndef X3D_CODECS_VERSION_HEADER_HPP
#define X3D_CODECS_VERSION_HEADER_HPP

#include <charconv>
#include <string>

namespace x3d::codec {

/// X3D has no version below 3.0; floor a sub-3.0/legacy token to "3.0" so the
/// emitted header is always valid (VP-2 §8). >= 3.0 (incl. future) passes
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
