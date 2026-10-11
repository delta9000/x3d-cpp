// JSON string literals shared by the JSON writer and Classic metadata comments.
#ifndef X3D_CODECS_JSON_STRING_HPP
#define X3D_CODECS_JSON_STRING_HPP

#include <cstdio>
#include <string>

namespace x3d::json {
inline std::string quote(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\t':
      out += "\\t";
      break;
    case '\r':
      out += "\\r";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        // RFC 8259: control characters MUST be escaped; strict consumers
        // (and X3DJSAIL's JSON path) reject raw bytes here (ENC-JSON-CTRL).
        char buf[8];
        std::snprintf(buf, sizeof buf, "\\u%04x",
                      static_cast<unsigned>(static_cast<unsigned char>(c)));
        out += buf;
      } else {
        out += c;
      }
      break;
    }
  }
  out += "\"";
  return out;
}

} // namespace x3d::json
#endif
