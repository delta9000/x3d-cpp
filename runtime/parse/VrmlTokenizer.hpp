// VrmlTokenizer.hpp
// Shared lexer for the ClassicVRML (.x3dv) and VRML97 (.wrl) text grammars
// (ISO/IEC 19776-2 / ISO/IEC 14772). Both grammars share the same lexical
// structure, so one tokenizer feeds both readers; they differ only in the
// statement grammar layered above.
//
// Lexical rules:
//   * Whitespace AND commas are delimiters (comma == whitespace in VRML).
//   * `#` starts a comment that runs to end-of-line. The very first line of a
//     file is the encoding header (`#X3D ...` / `#VRML ...`); the caller peels
//     that off before tokenizing the body, but a `firstLineIsHeader` flag is
//     offered for convenience.
//   * `{ } [ ]` are single-character punctuation tokens.
//   * A double-quoted string is one token; `\"` and `\\` escapes are unescaped
//     into the token text (matching FieldValueIO::parseMFString). The token's
//     `isString` flag records that it was quoted so a bare `USE`-like word is
//     never confused with the literal string "USE".
//   * Everything else is a bare token (identifier or number), delimited by
//     whitespace/comma/punctuation/quote/comment.
//
// Header-only, namespace x3d::codec. Depends only on the standard library.
#ifndef X3D_PARSE_VRML_TOKENIZER_HPP
#define X3D_PARSE_VRML_TOKENIZER_HPP

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::codec {

/// One lexical token. `text` is the unescaped contents (for strings) or the raw
/// lexeme (for identifiers/numbers/punctuation). `isString` distinguishes a
/// quoted string from a bare word; `line`/`col` are 1-based for diagnostics.
struct VrmlToken {
  enum class Kind { Identifier, Number, String, Punct, End };
  Kind kind = Kind::End;
  std::string text;
  bool isString = false; // true only for Kind::String
  std::size_t line = 0;
  std::size_t col = 0;

  bool isPunct(char c) const {
    return kind == Kind::Punct && text.size() == 1 && text[0] == c;
  }
  bool isWord(std::string_view w) const {
    return !isString && kind != Kind::String && kind != Kind::Punct &&
           kind != Kind::End && text == w;
  }
  bool atEnd() const { return kind == Kind::End; }
};

/// Streaming tokenizer with one-token lookahead (peek). Constructed over the
/// full body text (header line excluded, or with firstLineIsHeader=true to skip
/// the first physical line automatically).
class VrmlTokenizer {
public:
  explicit VrmlTokenizer(std::string_view src, bool firstLineIsHeader = false)
      : src_(src) {
    if (firstLineIsHeader)
      skipFirstLine();
    advance(lookahead_);  // prime first lookahead
    advance(lookahead2_); // prime second lookahead
  }

  /// The next token without consuming it.
  const VrmlToken &peek() const { return lookahead_; }

  /// The token AFTER the next one, without consuming anything. Used for the
  /// `Identifier '{'` two-token lookahead that distinguishes a node value
  /// (`Type {`) from a scalar where the grammar is otherwise ambiguous (e.g. an
  /// unknown proto-instance field). Returns the End token at stream end.
  const VrmlToken &peek2() const { return lookahead2_; }

  /// Consume and return the next token.
  VrmlToken next() {
    VrmlToken cur = lookahead_;
    lookahead_ = lookahead2_;
    advance(lookahead2_);
    return cur;
  }

  /// True once the stream is exhausted.
  bool atEnd() const { return lookahead_.kind == VrmlToken::Kind::End; }

private:
  void skipFirstLine() {
    while (pos_ < src_.size() && src_[pos_] != '\n')
      ++pos_;
    if (pos_ < src_.size())
      ++pos_; // consume the newline
    ++line_;
    col_ = 1;
  }

  void bump() {
    if (src_[pos_] == '\n') {
      ++line_;
      col_ = 1;
    } else {
      ++col_;
    }
    ++pos_;
  }

  void skipTrivia() {
    for (;;) {
      while (pos_ < src_.size()) {
        char c = src_[pos_];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',')
          bump();
        else
          break;
      }
      if (pos_ < src_.size() && src_[pos_] == '#') {
        while (pos_ < src_.size() && src_[pos_] != '\n')
          bump();
        continue; // loop to eat trailing whitespace / further comments
      }
      break;
    }
  }

  // Lex the next token into `out`. Two buffered tokens (lookahead_/lookahead2_)
  // are kept so callers can `peek()` and `peek2()`; `next()` shifts them.
  void advance(VrmlToken &out) {
    skipTrivia();
    if (pos_ >= src_.size()) {
      out = VrmlToken{VrmlToken::Kind::End, "", false, line_, col_};
      return;
    }
    std::size_t startLine = line_, startCol = col_;
    char c = src_[pos_];

    // Punctuation.
    if (c == '{' || c == '}' || c == '[' || c == ']') {
      bump();
      out = VrmlToken{VrmlToken::Kind::Punct, std::string(1, c), false,
                      startLine, startCol};
      return;
    }

    // Quoted string.
    if (c == '"') {
      bump(); // opening quote
      std::string str;
      while (pos_ < src_.size() && src_[pos_] != '"') {
        if (src_[pos_] == '\\' && pos_ + 1 < src_.size()) {
          char esc = src_[pos_ + 1];
          str += esc; // \" -> ", \\ -> \, others pass through unescaped
          bump();
          bump();
        } else {
          str += src_[pos_];
          bump();
        }
      }
      if (pos_ < src_.size())
        bump(); // closing quote
      out = VrmlToken{VrmlToken::Kind::String, str, true, startLine, startCol};
      return;
    }

    // Bare token: run until a delimiter.
    std::size_t begin = pos_;
    while (pos_ < src_.size()) {
      char d = src_[pos_];
      if (d == ' ' || d == '\t' || d == '\r' || d == '\n' || d == ',' ||
          d == '{' || d == '}' || d == '[' || d == ']' || d == '"' || d == '#')
        break;
      bump();
    }
    std::string text(src_.substr(begin, pos_ - begin));
    VrmlToken::Kind kind = looksNumeric(text) ? VrmlToken::Kind::Number
                                              : VrmlToken::Kind::Identifier;
    out = VrmlToken{kind, std::move(text), false, startLine, startCol};
  }

  static bool looksNumeric(std::string_view t) {
    if (t.empty())
      return false;
    char c0 = t[0];
    if (c0 == '+' || c0 == '-' || c0 == '.')
      return t.size() > 1 ? (std::isdigit(static_cast<unsigned char>(t[1])) ||
                             t[1] == '.')
                          : false;
    return std::isdigit(static_cast<unsigned char>(c0)) != 0;
  }

  std::string_view src_;
  std::size_t pos_ = 0;
  std::size_t line_ = 1;
  std::size_t col_ = 1;
  VrmlToken lookahead_;  // peek()
  VrmlToken lookahead2_; // peek2()
};

} // namespace x3d::codec

#endif // X3D_PARSE_VRML_TOKENIZER_HPP
