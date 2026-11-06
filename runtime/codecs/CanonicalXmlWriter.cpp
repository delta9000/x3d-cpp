#include "CanonicalXmlWriter.hpp"

#include "FieldValueIO.hpp"
#include "X3DRuntime.hpp"
#include "XmlGraphBuilder.hpp"
#include "XmlLite.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>

namespace x3d::codec {

std::string CanonicalXmlWriter::writeDocument(const runtime::X3DDocument &doc) {
  XmlGraphBuilder graph({.formatValue = canonFormatValue,
                         .authoredChildOrder = false,
                         .sortAttributes = true});
  auto root = std::make_unique<xml::Element>();
  root->name = "X3D";
  // X3DC14N <X3D> attrs (sorted alphabetically, so profile < version <
  // xmlns:xsd < xsd:noNamespaceSchemaLocation):
  root->setAttr("profile", doc.profileName());
  root->setAttr("version", doc.version);
  root->setAttr("xmlns:xsd", "http://www.w3.org/2001/XMLSchema-instance");
  root->setAttr("xsd:noNamespaceSchemaLocation",
                x3dSchemaLocation(doc.version));

  // <head>
  if (!doc.head.empty()) {
    xml::Element *head = root->addChild("head");
    for (const auto &c : doc.head.components) {
      xml::Element *e = head->addChild("component");
      e->setAttr("level", std::to_string(c.level));
      e->setAttr("name", c.name);
    }
    for (const auto &u : doc.head.units) {
      xml::Element *e = head->addChild("unit");
      e->setAttr("category", u.category);
      e->setAttr("conversionFactor", canonFmtDouble(u.conversionFactor));
      e->setAttr("name", u.name);
    }
    for (const auto &m : doc.head.meta) {
      xml::Element *e = head->addChild("meta");
      e->setAttr("content", m.content);
      if (!m.dir.empty())
        e->setAttr("dir", m.dir);
      if (!m.httpEquiv.empty())
        e->setAttr("http-equiv", m.httpEquiv);
      if (!m.lang.empty())
        e->setAttr("lang", m.lang);
      e->setAttr("name", m.name);
      if (!m.scheme.empty())
        e->setAttr("scheme", m.scheme);
    }
  }

  // <Scene>
  xml::Element *scene = root->addChild("Scene");
  graph.writeSceneInto(scene, doc.scene);

  std::ostringstream os;
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  os << x3dDoctype(doc.version) << "\n";
  graph.orderAttributes(*root);
  renderCanonical(*root, os, 0);
  return os.str();
}

void CanonicalXmlWriter::indentCan(std::ostringstream &os, int depth) {
  for (int i = 0; i < depth; ++i)
    os << "  ";
}

void CanonicalXmlWriter::renderCanonical(const xml::Element &el,
                                         std::ostringstream &os, int depth) {
  indentCan(os, depth);
  os << "<" << el.name;
  for (const auto &a : el.attributes) {
    os << " " << a.first << "='" << canonEscape(a.second) << "'";
  }
  if (el.children.empty() && el.text.empty()) {
    os << "/>\n";
    return;
  }
  os << ">\n";
  for (const auto &c : el.children)
    renderCanonical(*c, os, depth + 1);
  if (!el.text.empty()) {
    indentCan(os, depth + 1);
    os << "<![CDATA[" << xml::cdataEscape(el.text) << "]]>\n";
  }
  indentCan(os, depth);
  os << "</" << el.name << ">\n";
}

std::string canonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '\'':
      out += "&apos;";
      break;
    case '\t':
      out += "&#x9;";
      break;
    case '\n':
      out += "&#xA;";
      break;
    case '\r':
      out += "&#xD;";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

std::string canonFmtFloat(float v) {
  // Integer fast-path (no decimal needed).
  if (v == static_cast<float>(static_cast<long long>(v)) &&
      std::fabs(v) < 1e15f) {
    return std::to_string(static_cast<long long>(v));
  }
  // Shortest representation via to_chars general (Ryu/Dragonfly algo).
  std::array<char, 32> buf{};
  auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v,
                                 std::chars_format::general);
  if (ec == std::errc{}) {
    return std::string(buf.data(), ptr);
  }
  // Fallback: use 7 significant digits (enough for float round-trip).
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os.precision(7);
  os << v;
  return os.str();
}

std::string canonFmtDouble(double v) {
  // Always use to_chars for doubles (no integer fast-path).
  // This ensures `0.0` is emitted as `0.0` to match X3DJSAIL.
  std::array<char, 32> buf{};
  auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v,
                                 std::chars_format::general);
  if (ec == std::errc{}) {
    std::string s(buf.data(), ptr);
    // If to_chars produced an integer-looking string (no '.', no 'e', no 'E'),
    // append ".0" to distinguish from integer types.
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos) {
      s += ".0";
    }
    return s;
  }
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os.precision(17);
  os << v;
  return os.str();
}

std::string canonFormatValue(X3DFieldType type, const std::any &v) {
  switch (type) {
  case X3DFieldType::SFBool:
    return fmtBool(std::any_cast<SFBool>(v));
  case X3DFieldType::SFInt32:
    return std::to_string(std::any_cast<int>(v));
  case X3DFieldType::SFFloat:
    return canonFmtFloat(std::any_cast<float>(v));
  case X3DFieldType::SFDouble:
  case X3DFieldType::SFTime:
    return canonFmtDouble(std::any_cast<double>(v));
  case X3DFieldType::SFString:
    return std::any_cast<std::string>(v);
  case X3DFieldType::SFColor: {
    auto c = std::any_cast<SFColor>(v);
    return canonFmtFloat(c.r) + " " + canonFmtFloat(c.g) + " " +
           canonFmtFloat(c.b);
  }
  case X3DFieldType::SFColorRGBA: {
    auto c = std::any_cast<SFColorRGBA>(v);
    return canonFmtFloat(c.r) + " " + canonFmtFloat(c.g) + " " +
           canonFmtFloat(c.b) + " " + canonFmtFloat(c.a);
  }
  case X3DFieldType::SFVec2f: {
    auto p = std::any_cast<SFVec2f>(v);
    return canonFmtFloat(p.x) + " " + canonFmtFloat(p.y);
  }
  case X3DFieldType::SFVec2d: {
    auto p = std::any_cast<SFVec2d>(v);
    return canonFmtDouble(p.x) + " " + canonFmtDouble(p.y);
  }
  case X3DFieldType::SFVec3f: {
    auto p = std::any_cast<SFVec3f>(v);
    return canonFmtFloat(p.x) + " " + canonFmtFloat(p.y) + " " +
           canonFmtFloat(p.z);
  }
  case X3DFieldType::SFVec3d: {
    auto p = std::any_cast<SFVec3d>(v);
    return canonFmtDouble(p.x) + " " + canonFmtDouble(p.y) + " " +
           canonFmtDouble(p.z);
  }
  case X3DFieldType::SFVec4f: {
    auto p = std::any_cast<SFVec4f>(v);
    return canonFmtFloat(p.x) + " " + canonFmtFloat(p.y) + " " +
           canonFmtFloat(p.z) + " " + canonFmtFloat(p.w);
  }
  case X3DFieldType::SFVec4d: {
    auto p = std::any_cast<SFVec4d>(v);
    return canonFmtDouble(p.x) + " " + canonFmtDouble(p.y) + " " +
           canonFmtDouble(p.z) + " " + canonFmtDouble(p.w);
  }
  case X3DFieldType::SFRotation: {
    auto r = std::any_cast<SFRotation>(v);
    return canonFmtFloat(r.x) + " " + canonFmtFloat(r.y) + " " +
           canonFmtFloat(r.z) + " " + canonFmtFloat(r.angle);
  }
  case X3DFieldType::MFBool: {
    const auto &vec = std::any_cast<std::vector<bool>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += fmtBool(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFInt32: {
    const auto &vec = std::any_cast<std::vector<int>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += std::to_string(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFFloat: {
    const auto &vec = std::any_cast<std::vector<float>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFDouble:
  case X3DFieldType::MFTime: {
    const auto &vec = std::any_cast<std::vector<double>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtDouble(vec[i]);
    }
    return out;
  }
  case X3DFieldType::MFString:
    return fmtMFString(std::any_cast<std::vector<std::string>>(v));
  case X3DFieldType::MFColor: {
    const auto &vec = std::any_cast<std::vector<SFColor>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i].r) + " " + canonFmtFloat(vec[i].g) + " " +
             canonFmtFloat(vec[i].b);
    }
    return out;
  }
  case X3DFieldType::MFColorRGBA: {
    const auto &vec = std::any_cast<std::vector<SFColorRGBA>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i].r) + " " + canonFmtFloat(vec[i].g) + " " +
             canonFmtFloat(vec[i].b) + " " + canonFmtFloat(vec[i].a);
    }
    return out;
  }
  case X3DFieldType::MFVec2f: {
    const auto &vec = std::any_cast<std::vector<SFVec2f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y);
    }
    return out;
  }
  case X3DFieldType::MFVec3f: {
    const auto &vec = std::any_cast<std::vector<SFVec3f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y) + " " +
             canonFmtFloat(vec[i].z);
    }
    return out;
  }
  case X3DFieldType::MFVec2d: {
    const auto &vec = std::any_cast<std::vector<SFVec2d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtDouble(vec[i].x) + " " + canonFmtDouble(vec[i].y);
    }
    return out;
  }
  case X3DFieldType::MFVec3d: {
    const auto &vec = std::any_cast<std::vector<SFVec3d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtDouble(vec[i].x) + " " + canonFmtDouble(vec[i].y) + " " +
             canonFmtDouble(vec[i].z);
    }
    return out;
  }
  case X3DFieldType::MFVec4f: {
    const auto &vec = std::any_cast<std::vector<SFVec4f>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y) + " " +
             canonFmtFloat(vec[i].z) + " " + canonFmtFloat(vec[i].w);
    }
    return out;
  }
  case X3DFieldType::MFVec4d: {
    const auto &vec = std::any_cast<std::vector<SFVec4d>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtDouble(vec[i].x) + " " + canonFmtDouble(vec[i].y) + " " +
             canonFmtDouble(vec[i].z) + " " + canonFmtDouble(vec[i].w);
    }
    return out;
  }
  case X3DFieldType::MFRotation: {
    const auto &vec = std::any_cast<std::vector<SFRotation>>(v);
    std::string out;
    for (std::size_t i = 0; i < vec.size(); ++i) {
      if (i)
        out += ' ';
      out += canonFmtFloat(vec[i].x) + " " + canonFmtFloat(vec[i].y) + " " +
             canonFmtFloat(vec[i].z) + " " + canonFmtFloat(vec[i].angle);
    }
    return out;
  }
  case X3DFieldType::SFMatrix3f:
    return fmtMatrixF<SFMatrix3f, 3>(std::any_cast<SFMatrix3f>(v));
  case X3DFieldType::SFMatrix4f:
    return fmtMatrixF<SFMatrix4f, 4>(std::any_cast<SFMatrix4f>(v));
  case X3DFieldType::SFMatrix3d:
    return fmtMatrixD<SFMatrix3d, 3>(std::any_cast<SFMatrix3d>(v));
  case X3DFieldType::SFMatrix4d:
    return fmtMatrixD<SFMatrix4d, 4>(std::any_cast<SFMatrix4d>(v));
  case X3DFieldType::SFImage:
    return fmtImage(std::any_cast<SFImage>(v));
  case X3DFieldType::MFImage:
  case X3DFieldType::MFMatrix3f:
  case X3DFieldType::MFMatrix3d:
  case X3DFieldType::MFMatrix4f:
  case X3DFieldType::MFMatrix4d:
    return formatValue(type, v);
  default:
    return "";
  }
}

std::string x3dDoctype(const std::string &version) {
  // Public/system identifier pattern:
  //   PUBLIC "ISO//Web3D//DTD X3D <ver>//EN"
  //          "https://www.web3d.org/specifications/x3d-<ver>.dtd"
  return "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D " + version +
         "//EN\" \"https://www.web3d.org/specifications/x3d-" + version +
         ".dtd\">";
}

std::string x3dSchemaLocation(const std::string &version) {
  return "https://www.web3d.org/specifications/x3d-" + version + ".xsd";
}

} // namespace x3d::codec
