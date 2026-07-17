// XmlLite.hpp
// A tiny, dependency-free XML parser/DOM sufficient for X3D documents.
//
// Scope: handles elements, attributes (single- or double-quoted), self-closing
// tags, nested children, XML declarations (<?xml ...?>), DOCTYPE (<!DOCTYPE ...>),
// comments (<!-- ... -->), and the five predefined entities (&amp; &lt; &gt;
// &quot; &apos;) plus numeric character references. It deliberately does NOT aim
// at full XML conformance (no namespaces, no CDATA processing beyond passthrough,
// no DTD validation) — only what is needed to round-trip the X3D-XML encoding
// emitted by the writer in this project, and to read typical hand-authored .x3d.
//
// Header-only, namespace x3d::xml. No external dependencies.
#ifndef X3D_XML_LITE_HPP
#define X3D_XML_LITE_HPP

#include "RecursionLimits.hpp"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace x3d::xml {

/// Emit a non-fatal parser diagnostic. Under the "lenient read" policy a
/// recovery (e.g. widening a too-eagerly-closed attribute value) must stay
/// visible rather than silent, so we surface it on stderr.
inline void xmlLiteWarn(const std::string &msg) {
  std::cerr << "X3D XML parser warning: " << msg << "\n";
}

/// One XML element: tag name, ordered attributes, and child elements. Text
/// content is captured but X3D uses element/attribute structure, not mixed text.
struct Element {
  std::string name;
  // Attributes kept in source order (X3D attribute order is not semantically
  // significant, but preserving it keeps round-tripped output stable/readable).
  std::vector<std::pair<std::string, std::string>> attributes;
  std::vector<std::unique_ptr<Element>> children;
  std::string text; // concatenated character data directly inside this element

  /// Look up an attribute by name; returns the value or `fallback` if absent.
  const std::string *attr(const std::string &key) const {
    for (const auto &a : attributes) {
      if (a.first == key) {
        return &a.second;
      }
    }
    return nullptr;
  }

  std::string attrOr(const std::string &key, const std::string &fallback) const {
    const std::string *v = attr(key);
    return v ? *v : fallback;
  }

  bool hasAttr(const std::string &key) const { return attr(key) != nullptr; }

  void setAttr(const std::string &key, std::string value) {
    attributes.emplace_back(key, std::move(value));
  }

  Element *addChild(const std::string &childName) {
    auto child = std::make_unique<Element>();
    child->name = childName;
    Element *raw = child.get();
    children.push_back(std::move(child));
    return raw;
  }
};

/// Escape a string for use as XML element text / attribute value content.
inline std::string escape(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
    case '&': out += "&amp;"; break;
    case '<': out += "&lt;"; break;
    case '>': out += "&gt;"; break;
    case '"': out += "&quot;"; break;
    case '\'': out += "&apos;"; break;
    default: out += c; break;
    }
  }
  return out;
}

/// Make text safe for a CDATA body: a literal `]]>` would close the section
/// early (silently truncating e.g. Script source — ENC-CDATA-SCRIPT), so split
/// it across two sections: `]]` ends one, `>` starts the next. The reader
/// concatenates consecutive CDATA sections, restoring the original text.
inline std::string cdataEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  std::size_t i = 0;
  while (i < s.size()) {
    std::size_t hit = s.find("]]>", i);
    if (hit == std::string::npos) {
      out.append(s, i, std::string::npos);
      break;
    }
    out.append(s, i, hit - i);
    out += "]]]]><![CDATA[>";
    i = hit + 3;
  }
  return out;
}

/// Decode the predefined entities and numeric character references in `s`.
inline std::string unescape(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size();) {
    if (s[i] != '&') {
      out += s[i++];
      continue;
    }
    std::size_t semi = s.find(';', i);
    if (semi == std::string::npos) {
      out += s[i++];
      continue;
    }
    std::string ent = s.substr(i + 1, semi - i - 1);
    if (ent == "amp") {
      out += '&';
    } else if (ent == "lt") {
      out += '<';
    } else if (ent == "gt") {
      out += '>';
    } else if (ent == "quot") {
      out += '"';
    } else if (ent == "apos") {
      out += '\'';
    } else if (!ent.empty() && ent[0] == '#') {
      // Numeric character reference: decode to UTF-8.
      long code = 0;
      if (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X')) {
        code = std::strtol(ent.c_str() + 2, nullptr, 16);
      } else {
        code = std::strtol(ent.c_str() + 1, nullptr, 10);
      }
      auto cp = static_cast<std::uint32_t>(code);
      if (cp < 0x80) {
        out += static_cast<char>(cp);
      } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
      } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
      } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
      }
    } else {
      // Unknown entity: pass through verbatim (keeps it lossless-ish).
      out += '&';
      out += ent;
      out += ';';
      i = semi + 1;
      continue;
    }
    i = semi + 1;
  }
  return out;
}

/// A minimal recursive-descent XML parser producing an Element tree.
class Parser {
public:
  explicit Parser(std::string source) : src_(std::move(source)) {}

  /// Parse and return the single root element. Throws std::runtime_error on a
  /// structural error (unbalanced tags, EOF inside a tag, etc.).
  std::unique_ptr<Element> parse() {
    skipProlog();
    if (pos_ >= src_.size() || src_[pos_] != '<') {
      throw std::runtime_error("XML: expected root element");
    }
    auto root = parseElement();
    return root;
  }

private:
  std::string src_;
  std::size_t pos_ = 0;
  std::size_t depth_ = 0; // SEC-1: element nesting depth (DoS guard).

  static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  void skipSpace() {
    while (pos_ < src_.size() && isSpace(src_[pos_])) {
      ++pos_;
    }
  }

  /// Skip the XML declaration, DOCTYPE, comments, and whitespace before root.
  void skipProlog() {
    for (;;) {
      skipSpace();
      if (pos_ + 1 >= src_.size() || src_[pos_] != '<') {
        return;
      }
      if (src_.compare(pos_, 4, "<!--") == 0) {
        skipComment();
      } else if (src_[pos_ + 1] == '?') {
        std::size_t end = src_.find("?>", pos_);
        if (end == std::string::npos) {
          throw std::runtime_error("XML: unterminated <? ?>");
        }
        pos_ = end + 2;
      } else if (src_[pos_ + 1] == '!') {
        // DOCTYPE or other declaration. A DOCTYPE may carry an internal subset
        // (<!DOCTYPE name [ ... ]>) whose entity/element decls contain their own
        // '>' chars, so skip past the matching ']' before seeking the closing
        // '>' — otherwise we stop inside the subset and miss the root element.
        std::size_t end = src_.find('>', pos_);
        std::size_t bracket = src_.find('[', pos_);
        if (end != std::string::npos && bracket != std::string::npos &&
            bracket < end) {
          std::size_t close = src_.find(']', bracket);
          if (close == std::string::npos) {
            throw std::runtime_error(
                "XML: unterminated DOCTYPE internal subset");
          }
          end = src_.find('>', close);
        }
        if (end == std::string::npos) {
          throw std::runtime_error("XML: unterminated <! ...>");
        }
        pos_ = end + 1;
      } else {
        return; // start of the root element
      }
    }
  }

  void skipComment() {
    std::size_t end = src_.find("-->", pos_);
    if (end == std::string::npos) {
      throw std::runtime_error("XML: unterminated comment");
    }
    pos_ = end + 3;
  }

  std::string parseName() {
    std::size_t start = pos_;
    while (pos_ < src_.size()) {
      char c = src_[pos_];
      if (isSpace(c) || c == '>' || c == '/' || c == '=') {
        break;
      }
      ++pos_;
    }
    return src_.substr(start, pos_ - start);
  }

  std::unique_ptr<Element> parseElement() {
    // Precondition: src_[pos_] == '<' and not a comment/PI/decl.
    NestingGuard guard(depth_, "XML"); // SEC-1: bound parseElement<->parseContent.
    ++pos_; // consume '<'
    auto el = std::make_unique<Element>();
    el->name = parseName();
    if (el->name.empty()) {
      throw std::runtime_error("XML: empty element name");
    }

    // Attributes.
    for (;;) {
      skipSpace();
      if (pos_ >= src_.size()) {
        throw std::runtime_error("XML: EOF inside start tag");
      }
      char c = src_[pos_];
      if (c == '/') {
        // Self-closing tag.
        if (pos_ + 1 >= src_.size() || src_[pos_ + 1] != '>') {
          throw std::runtime_error("XML: malformed self-closing tag");
        }
        pos_ += 2;
        return el;
      }
      if (c == '>') {
        ++pos_;
        break; // proceed to children/content
      }
      // Attribute name="value".
      std::string key = parseName();
      skipSpace();
      if (pos_ >= src_.size() || src_[pos_] != '=') {
        throw std::runtime_error("XML: expected '=' after attribute name '" +
                                 key + "'");
      }
      ++pos_; // '='
      skipSpace();
      if (pos_ >= src_.size() || (src_[pos_] != '"' && src_[pos_] != '\'')) {
        throw std::runtime_error("XML: expected quoted attribute value");
      }
      char quote = src_[pos_++];
      std::size_t vstart = pos_;
      while (pos_ < src_.size() && src_[pos_] != quote) {
        ++pos_;
      }
      if (pos_ >= src_.size()) {
        throw std::runtime_error("XML: unterminated attribute value");
      }
      // Lenient recovery for an inner same-kind quote inside the value (e.g.
      // single-quoted content that itself contains an apostrophe:
      // content='it's here'). In well-formed XML the char immediately after the
      // closing quote is whitespace, '>', '/', or EOF. If instead it is some
      // other character, this quote almost certainly closed the value too early
      // and the *real* close is a later same-kind quote. Widen the value to the
      // last same-kind quote that still lies before the start tag's '>' (and
      // before any other tag), warn, and continue. This shape cannot occur in
      // valid XML, so well-formed documents always take the unchanged path.
      {
        std::size_t after = pos_ + 1;
        char next = (after < src_.size()) ? src_[after] : '\0';
        bool wellFormedClose =
            (after >= src_.size()) || isSpace(next) || next == '>' ||
            next == '/';
        if (!wellFormedClose) {
          // Find the start-tag close '>' that bounds this attribute list.
          std::size_t tagEnd = src_.find('>', pos_);
          // Scan for the last same-kind quote strictly before tagEnd.
          std::size_t lastQuote = std::string::npos;
          std::size_t scan = pos_;
          while (scan < src_.size() && (tagEnd == std::string::npos ||
                                        scan < tagEnd)) {
            if (src_[scan] == quote)
              lastQuote = scan;
            ++scan;
          }
          if (lastQuote != std::string::npos && lastQuote > pos_) {
            xmlLiteWarn("recovered attribute value for '" + key +
                        "' containing an inner " +
                        (quote == '\'' ? std::string("apostrophe")
                                       : std::string("quote")) +
                        "; widened to the last quote before tag end");
            pos_ = lastQuote;
          }
          // else: no later same-kind quote before tag end — fall through and
          // accept the original (narrow) value, matching prior behavior.
        }
      }
      std::string raw = src_.substr(vstart, pos_ - vstart);
      ++pos_; // closing quote
      el->attributes.emplace_back(key, unescape(raw));
    }

    // Content: children + text until the matching end tag.
    parseContent(*el);
    return el;
  }

  void parseContent(Element &parent) {
    for (;;) {
      if (pos_ >= src_.size()) {
        throw std::runtime_error("XML: EOF before </" + parent.name + ">");
      }
      if (src_[pos_] == '<') {
        if (src_.compare(pos_, 4, "<!--") == 0) {
          skipComment();
          continue;
        }
        if (src_.compare(pos_, 9, "<![CDATA[") == 0) {
          std::size_t end = src_.find("]]>", pos_);
          if (end == std::string::npos) {
            throw std::runtime_error("XML: unterminated CDATA");
          }
          parent.text += src_.substr(pos_ + 9, end - (pos_ + 9));
          pos_ = end + 3;
          continue;
        }
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
          // End tag.
          pos_ += 2;
          std::string close = parseName();
          skipSpace();
          if (pos_ >= src_.size() || src_[pos_] != '>') {
            throw std::runtime_error("XML: malformed end tag </" + close + ">");
          }
          ++pos_;
          if (close != parent.name) {
            throw std::runtime_error("XML: mismatched end tag </" + close +
                                     "> for <" + parent.name + ">");
          }
          return;
        }
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '?') {
          std::size_t end = src_.find("?>", pos_);
          if (end == std::string::npos) {
            throw std::runtime_error("XML: unterminated <? ?>");
          }
          pos_ = end + 2;
          continue;
        }
        // Child element.
        parent.children.push_back(parseElement());
      } else {
        // Text run.
        std::size_t start = pos_;
        while (pos_ < src_.size() && src_[pos_] != '<') {
          ++pos_;
        }
        std::string chunk = src_.substr(start, pos_ - start);
        // Only keep non-whitespace text (X3D text content is rare).
        bool allSpace = true;
        for (char c : chunk) {
          if (!isSpace(c)) {
            allSpace = false;
            break;
          }
        }
        if (!allSpace) {
          parent.text += unescape(chunk);
        }
      }
    }
  }
};

/// Convenience: parse a full XML document string into its root element.
inline std::unique_ptr<Element> parse(const std::string &source) {
  Parser p(source);
  return p.parse();
}

} // namespace x3d::xml

#endif // X3D_XML_LITE_HPP
