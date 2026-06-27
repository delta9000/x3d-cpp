// JsonLite.hpp
// A tiny, dependency-free JSON parser/DOM sufficient for the X3D-JSON encoding.
//
// Scope: handles objects, arrays, strings (with the standard JSON escapes
// \" \\ \/ \b \f \n \r \t and \uXXXX), numbers (int/float/exponent), the
// literals true/false/null, comments are NOT supported (JSON has none). It is a
// deliberate sibling to XmlLite: just enough to read back the X3D-JSON the
// JsonWriter in this project emits, plus typical hand-authored .json. It does
// NOT aim at exhaustive RFC 8259 validation (e.g. it is lenient about trailing
// garbage after the root value, and accepts a leading UTF-8 BOM).
//
// Number values are kept BOTH as a parsed double and as their original lexeme
// so a reader can re-emit integers without a float artifact (e.g. `3` not
// `3.0`) when round-tripping into the X3D wire-string form.
//
// Header-only, namespace x3d::json. No external dependencies.
#ifndef X3D_JSON_LITE_HPP
#define X3D_JSON_LITE_HPP

#include "RecursionLimits.hpp"

#include <cctype>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace x3d::json {

/// JSON value kinds.
enum class Type { Null, Bool, Number, String, Array, Object };

/// One JSON value. Objects preserve key order (X3D-JSON ordering is not
/// semantically significant, but preserving it keeps reads deterministic).
struct Value {
  Type type = Type::Null;

  bool boolean = false;
  double number = 0.0;
  std::string numberLexeme; // original token, e.g. "3" or "1.5e2"
  std::string str;          // for String

  std::vector<std::unique_ptr<Value>> array;
  std::vector<std::pair<std::string, std::unique_ptr<Value>>> object;

  bool isObject() const { return type == Type::Object; }
  bool isArray() const { return type == Type::Array; }
  bool isString() const { return type == Type::String; }
  bool isNumber() const { return type == Type::Number; }
  bool isBool() const { return type == Type::Bool; }
  bool isNull() const { return type == Type::Null; }

  /// Look up an object member by key; null if absent or not an object.
  const Value *member(const std::string &key) const {
    if (type != Type::Object)
      return nullptr;
    for (const auto &kv : object)
      if (kv.first == key)
        return kv.second.get();
    return nullptr;
  }
};

namespace detail {

/// Recursive-descent JSON parser over a string. Throws std::runtime_error on
/// malformed input, with a byte offset for diagnostics.
class Parser {
public:
  explicit Parser(const std::string &text) : s_(text) {}

  std::unique_ptr<Value> parse() {
    skipBom();
    skipWs();
    auto v = parseValue();
    skipWs();
    return v;
  }

private:
  const std::string &s_;
  std::size_t i_ = 0;
  std::size_t depth_ = 0; // SEC-1: value nesting depth (DoS guard).

  [[noreturn]] void fail(const std::string &msg) const {
    throw std::runtime_error("JsonLite: " + msg + " at offset " +
                             std::to_string(i_));
  }

  void skipBom() {
    if (s_.size() >= 3 && static_cast<unsigned char>(s_[0]) == 0xEF &&
        static_cast<unsigned char>(s_[1]) == 0xBB &&
        static_cast<unsigned char>(s_[2]) == 0xBF)
      i_ = 3;
  }

  void skipWs() {
    while (i_ < s_.size()) {
      char c = s_[i_];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        ++i_;
      else
        break;
    }
  }

  char peek() const { return i_ < s_.size() ? s_[i_] : '\0'; }

  std::unique_ptr<Value> parseValue() {
    NestingGuard guard(depth_, "JsonLite"); // SEC-1: bound recursive value nesting.
    skipWs();
    if (i_ >= s_.size())
      fail("unexpected end of input");
    char c = s_[i_];
    switch (c) {
    case '{':
      return parseObject();
    case '[':
      return parseArray();
    case '"':
      return parseString();
    case 't':
    case 'f':
      return parseBool();
    case 'n':
      return parseNull();
    default:
      if (c == '-' || (c >= '0' && c <= '9'))
        return parseNumber();
      fail(std::string("unexpected character '") + c + "'");
    }
  }

  std::unique_ptr<Value> parseObject() {
    auto v = std::make_unique<Value>();
    v->type = Type::Object;
    ++i_; // '{'
    skipWs();
    if (peek() == '}') {
      ++i_;
      return v;
    }
    for (;;) {
      skipWs();
      if (peek() != '"')
        fail("expected string key in object");
      auto key = parseString();
      skipWs();
      if (peek() != ':')
        fail("expected ':' after object key");
      ++i_; // ':'
      auto val = parseValue();
      v->object.emplace_back(std::move(key->str), std::move(val));
      skipWs();
      char d = peek();
      if (d == ',') {
        ++i_;
        continue;
      }
      if (d == '}') {
        ++i_;
        break;
      }
      fail("expected ',' or '}' in object");
    }
    return v;
  }

  std::unique_ptr<Value> parseArray() {
    auto v = std::make_unique<Value>();
    v->type = Type::Array;
    ++i_; // '['
    skipWs();
    if (peek() == ']') {
      ++i_;
      return v;
    }
    for (;;) {
      v->array.push_back(parseValue());
      skipWs();
      char d = peek();
      if (d == ',') {
        ++i_;
        continue;
      }
      if (d == ']') {
        ++i_;
        break;
      }
      fail("expected ',' or ']' in array");
    }
    return v;
  }

  std::unique_ptr<Value> parseString() {
    auto v = std::make_unique<Value>();
    v->type = Type::String;
    ++i_; // opening quote
    std::string out;
    while (i_ < s_.size()) {
      char c = s_[i_++];
      if (c == '"') {
        v->str = std::move(out);
        return v;
      }
      if (c == '\\') {
        if (i_ >= s_.size())
          fail("unterminated escape in string");
        char e = s_[i_++];
        switch (e) {
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case '/':
          out += '/';
          break;
        case 'b':
          out += '\b';
          break;
        case 'f':
          out += '\f';
          break;
        case 'n':
          out += '\n';
          break;
        case 'r':
          out += '\r';
          break;
        case 't':
          out += '\t';
          break;
        case 'u': {
          unsigned cp = parseHex4();
          // JSON surrogate-pair decoding (RFC 8259 §8): a high surrogate
          // (0xD800-0xDBFF) must be followed by a low surrogate (0xDC00-0xDFFF)
          // to form a supplementary-plane code point. Be lenient on a paired
          // surrogate (decode correctly) and pass the lone surrogate through
          // unchanged (the 3-byte branch below then emits invalid UTF-8 for
          // it, but the parser does not throw on the common case).
          if (cp >= 0xD800 && cp <= 0xDBFF && i_ + 1 < s_.size() &&
              s_[i_] == '\\' && s_[i_ + 1] == 'u') {
            i_ += 2; // consume the literal backslash-u
            unsigned low = parseHex4();
            if (low >= 0xDC00 && low <= 0xDFFF)
              cp = 0x10000u + ((cp - 0xD800u) << 10) + (low - 0xDC00u);
          }
          appendUtf8(out, cp);
          break;
        }
        default:
          fail("invalid escape in string");
        }
      } else {
        out += c;
      }
    }
    fail("unterminated string");
  }

  unsigned parseHex4() {
    if (i_ + 4 > s_.size())
      fail("truncated \\u escape");
    unsigned cp = 0;
    for (int k = 0; k < 4; ++k) {
      char h = s_[i_++];
      cp <<= 4;
      if (h >= '0' && h <= '9')
        cp |= unsigned(h - '0');
      else if (h >= 'a' && h <= 'f')
        cp |= unsigned(h - 'a' + 10);
      else if (h >= 'A' && h <= 'F')
        cp |= unsigned(h - 'A' + 10);
      else
        fail("invalid hex digit in \\u escape");
    }
    return cp;
  }

  static void appendUtf8(std::string &out, unsigned cp) {
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
      // 4-byte UTF-8 for supplementary-plane code points (U+10000..U+10FFFF).
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  std::unique_ptr<Value> parseNumber() {
    std::size_t start = i_;
    if (peek() == '-')
      ++i_;
    while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_])))
      ++i_;
    if (peek() == '.') {
      ++i_;
      while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_])))
        ++i_;
    }
    if (peek() == 'e' || peek() == 'E') {
      ++i_;
      if (peek() == '+' || peek() == '-')
        ++i_;
      while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_])))
        ++i_;
    }
    auto v = std::make_unique<Value>();
    v->type = Type::Number;
    v->numberLexeme = s_.substr(start, i_ - start);
    try {
      v->number = std::stod(v->numberLexeme);
    } catch (...) {
      fail("invalid number");
    }
    return v;
  }

  std::unique_ptr<Value> parseBool() {
    if (s_.compare(i_, 4, "true") == 0) {
      i_ += 4;
      auto v = std::make_unique<Value>();
      v->type = Type::Bool;
      v->boolean = true;
      return v;
    }
    if (s_.compare(i_, 5, "false") == 0) {
      i_ += 5;
      auto v = std::make_unique<Value>();
      v->type = Type::Bool;
      v->boolean = false;
      return v;
    }
    fail("invalid literal");
  }

  std::unique_ptr<Value> parseNull() {
    if (s_.compare(i_, 4, "null") == 0) {
      i_ += 4;
      auto v = std::make_unique<Value>();
      v->type = Type::Null;
      return v;
    }
    fail("invalid literal");
  }
};

} // namespace detail

/// Parse a JSON document. Returns the root value (never null on success);
/// throws std::runtime_error on malformed input.
inline std::unique_ptr<Value> parse(const std::string &text) {
  detail::Parser p(text);
  return p.parse();
}

} // namespace x3d::json

#endif // X3D_JSON_LITE_HPP
