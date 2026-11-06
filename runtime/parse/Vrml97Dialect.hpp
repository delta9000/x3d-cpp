// ISO/IEC 14772-1 field-name aliases. Unlisted names pass through unchanged.

#ifndef X3D_PARSE_VRML97_DIALECT_HPP
#define X3D_PARSE_VRML97_DIALECT_HPP

#include <string>

namespace x3d::codec::vrml97 {

inline std::string mapNodeName(const std::string &token) { return token; }

/// Unlisted node/field pairs pass through unchanged.
inline std::string mapFieldName(const std::string &nodeType,
                                const std::string &token) {
  if (nodeType == "LOD" && token == "level")
    return "children";
  if (nodeType == "Switch" && token == "choice")
    return "children";
  return token;
}

} // namespace x3d::codec::vrml97

#endif // X3D_PARSE_VRML97_DIALECT_HPP
