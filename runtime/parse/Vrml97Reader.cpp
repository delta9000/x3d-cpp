#include "Vrml97Reader.hpp"

namespace x3d::codec {

Encoding Vrml97Reader::encoding() const { return Encoding::VRML97; }

runtime::X3DDocument Vrml97Reader::readDocument(const std::string &text) {
  warnings_.clear();
  dialectOn_ = true; // reset; onHeaderLine() may turn it off for a #X3D header
  runtime::X3DDocument doc = ClassicVrmlReader::readDocument(text);
  if (strict_ && !warnings_.empty())
    throw std::runtime_error("Vrml97Reader (strict): " + warnings_.front());
  return doc;
}

void Vrml97Reader::setStrict(bool strict) { strict_ = strict; }

bool Vrml97Reader::strict() const { return strict_; }

const std::vector<std::string> &Vrml97Reader::warnings() const {
  return warnings_;
}

std::string Vrml97Reader::mapNodeName(const std::string &token) const {
  return dialectOn_ ? vrml97::mapNodeName(token) : token;
}

std::string Vrml97Reader::mapFieldName(const std::string &nodeType,
                                       const std::string &token) const {
  return dialectOn_ ? vrml97::mapFieldName(nodeType, token) : token;
}

void Vrml97Reader::warn(const std::string &message) {
  warnings_.push_back(message);
}

void Vrml97Reader::onHeaderLine(std::string_view src,
                                runtime::X3DDocument &doc) {
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

std::vector<std::string> Vrml97Reader::splitOnSpace(std::string_view line) {
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

} // namespace x3d::codec
