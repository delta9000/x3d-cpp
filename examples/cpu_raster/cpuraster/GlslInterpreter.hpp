// GlslInterpreter.hpp — a tree-walking interpreter for a GLSL-ES fragment-shader
// SUBSET, so the CPU rasterizer can EXECUTE author ComposedShader GLSL on the
// CPU (the "GLSL emulation" headline feature). It is the interpreted sibling of
// MaterialShader.hpp's compiled-in ports: same value runtime (glsl.hpp), same
// fragment-shader plug (Rasterizer::FragmentShader).
//
// SUPPORTED SUBSET (enough for real ComposedShader fragment shaders):
//   types     : void float int bool vec2 vec3 vec4 mat3 mat4 sampler2D
//   top-level : #..., precision, layout/in/out/uniform/varying decls (consumed),
//               const + global var decls, function definitions.
//   stmts     : decl(+init), assignment(= += -= *= /=), if/else, for, return,
//               discard, break, continue, blocks, expression statements.
//   exprs     : literals, identifiers, calls, constructors, swizzles (.xyzw/rgba/
//               stpq, incl. as lvalues), indexing (vec/array/mat-column), unary
//               - !, ++/-- (pre/post), binary + - * / with scalar/vector/matrix
//               broadcast, comparisons, && || , ?:.
//   builtins  : normalize dot cross length distance reflect mix clamp max min pow
//               abs sqrt sin cos tan exp log floor ceil fract mod step smoothstep
//               sign radians degrees texture/texture2D dFdx dFdy fwidth.
//
// HOST CONVENTION: author fragment shaders read the same `in` varyings the PoC's
// lit.vert emits — vPosEye, vNormalEye, vColor, vTexCoord (+ gl_FrontFacing) —
// and write `FragColor` (or gl_FragColor). makeInterpretedShader() seeds those
// plus a generous uniform set from the MaterialDesc + eye-space lights.
//
// dFdx/dFdy are exact for the two varyings the rasterizer precomputes derivatives
// for (vPosEye, vTexCoord); any other argument yields zero (documented narrowing).
//
// Out-of-SDK consumer code. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_GLSL_INTERPRETER_HPP
#define X3D_CPURASTER_GLSL_INTERPRETER_HPP

#include "MaterialShader.hpp" // EyeLight, FragmentShader, MaterialTextures
#include "RenderItem.hpp"
#include "Rasterizer.hpp"
#include "Texture.hpp"
#include "glsl.hpp"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::cpuraster {

// ===========================================================================
// Value — the interpreter's runtime value.
// ===========================================================================
enum class VT : std::uint8_t { Void, Float, Int, Bool, Vec2, Vec3, Vec4, Mat3, Mat4, Sampler, Array };

struct Value {
  VT t = VT::Void;
  std::array<float, 16> f{}; // scalar in f[0]; vecN in f[0..n); matN col-major.
  const Texture *sampler = nullptr;
  std::shared_ptr<std::vector<Value>> arr; // Array elements.

  static Value flt(float v) { Value r; r.t = VT::Float; r.f[0] = v; return r; }
  static Value integer(int v) { Value r; r.t = VT::Int; r.f[0] = (float)v; return r; }
  static Value boolean(bool v) { Value r; r.t = VT::Bool; r.f[0] = v ? 1.f : 0.f; return r; }
  static Value v2(glsl::vec2 v) { Value r; r.t = VT::Vec2; r.f[0]=v.x; r.f[1]=v.y; return r; }
  static Value v3(glsl::vec3 v) { Value r; r.t = VT::Vec3; r.f[0]=v.x; r.f[1]=v.y; r.f[2]=v.z; return r; }
  static Value v4(glsl::vec4 v) { Value r; r.t = VT::Vec4; r.f[0]=v.x; r.f[1]=v.y; r.f[2]=v.z; r.f[3]=v.w; return r; }
  static Value samp(const Texture *t) { Value r; r.t = VT::Sampler; r.sampler = t; return r; }

  int comps() const {
    switch (t) {
      case VT::Float: case VT::Int: case VT::Bool: return 1;
      case VT::Vec2: return 2; case VT::Vec3: return 3; case VT::Vec4: return 4;
      case VT::Mat3: return 9; case VT::Mat4: return 16;
      default: return 0;
    }
  }
  bool isNum() const { return t == VT::Float || t == VT::Int || t == VT::Bool; }
  bool isVec() const { return t == VT::Vec2 || t == VT::Vec3 || t == VT::Vec4; }
  float scalar() const { return f[0]; }
  bool asBool() const { return f[0] != 0.0f; }
  glsl::vec2 vec2v() const { return {f[0], f[1]}; }
  glsl::vec3 vec3v() const { return {f[0], f[1], f[2]}; }
  glsl::vec4 vec4v() const { return {f[0], f[1], f[2], f[3]}; }
};

// Control-flow signals (clean, simple tree-walk).
struct ReturnSignal { Value v; };
struct BreakSignal {};
struct ContinueSignal {};
struct DiscardSignal {};
struct GlslError : std::runtime_error { using std::runtime_error::runtime_error; };

// ===========================================================================
// AST
// ===========================================================================
struct Expr;
using ExprP = std::shared_ptr<Expr>;
struct Expr {
  enum class K { Num, Bool, Ident, Call, Member, Index, Unary, Binary, Assign, Cond };
  K k;
  double num = 0; bool isInt = false; bool boolVal = false;
  std::string name;                 // ident / callee / swizzle / op
  std::vector<ExprP> args;          // call/ctor args
  ExprP a, b, c;                    // operands
};

struct Stmt;
using StmtP = std::shared_ptr<Stmt>;
struct Stmt {
  enum class K { Expr, Decl, If, For, Return, Discard, Block, Break, Continue };
  K k;
  std::string declType, declName; ExprP init;     // Decl
  ExprP expr;                                      // Expr / Return / If-cond
  std::vector<StmtP> body, elseBody;               // Block / If / For body
  StmtP forInit; ExprP forCond, forStep;           // For
};

struct Function {
  std::string retType, name;
  std::vector<std::pair<std::string, std::string>> params; // (type,name)
  std::vector<StmtP> body;
};

// ===========================================================================
// Lexer
// ===========================================================================
struct Token {
  enum class T { Id, Num, Punct, End };
  T t = T::End; std::string s; double num = 0; bool isInt = false;
};

class Lexer {
public:
  explicit Lexer(const std::string &src) { tokenize(src); }
  const std::vector<Token> &tokens() const { return toks_; }

private:
  void tokenize(const std::string &src) {
    std::size_t i = 0, n = src.size();
    auto atLineStartHash = [&](std::size_t p) {
      // # only meaningful at logical line start (after optional spaces).
      std::size_t q = p;
      while (q > 0 && (src[q - 1] == ' ' || src[q - 1] == '\t')) --q;
      return q == 0 || src[q - 1] == '\n';
    };
    while (i < n) {
      char c = src[i];
      if (c == '\n' || c == '\r' || c == ' ' || c == '\t') { ++i; continue; }
      // line comment
      if (c == '/' && i + 1 < n && src[i + 1] == '/') {
        while (i < n && src[i] != '\n') ++i;
        continue;
      }
      // block comment
      if (c == '/' && i + 1 < n && src[i + 1] == '*') {
        i += 2;
        while (i + 1 < n && !(src[i] == '*' && src[i + 1] == '/')) ++i;
        i += 2;
        continue;
      }
      // preprocessor line (#version, #define-ish) — skip the whole line.
      if (c == '#' && atLineStartHash(i)) {
        while (i < n && src[i] != '\n') ++i;
        continue;
      }
      if (std::isalpha((unsigned char)c) || c == '_') {
        std::size_t j = i;
        while (j < n && (std::isalnum((unsigned char)src[j]) || src[j] == '_')) ++j;
        Token t; t.t = Token::T::Id; t.s = src.substr(i, j - i);
        toks_.push_back(t); i = j; continue;
      }
      if (std::isdigit((unsigned char)c) ||
          (c == '.' && i + 1 < n && std::isdigit((unsigned char)src[i + 1]))) {
        std::size_t j = i; bool isFloat = false;
        while (j < n && std::isdigit((unsigned char)src[j])) ++j;
        if (j < n && src[j] == '.') { isFloat = true; ++j; while (j < n && std::isdigit((unsigned char)src[j])) ++j; }
        if (j < n && (src[j] == 'e' || src[j] == 'E')) {
          isFloat = true; ++j; if (j < n && (src[j]=='+'||src[j]=='-')) ++j;
          while (j < n && std::isdigit((unsigned char)src[j])) ++j;
        }
        if (j < n && (src[j] == 'f' || src[j] == 'F')) { isFloat = true; ++j; }
        Token t; t.t = Token::T::Num; t.num = std::stod(src.substr(i, j - i));
        t.isInt = !isFloat; toks_.push_back(t); i = j; continue;
      }
      // multi-char operators
      static const char *ops2[] = {"==", "!=", "<=", ">=", "&&", "||", "++", "--",
                                   "+=", "-=", "*=", "/="};
      bool matched = false;
      if (i + 1 < n) {
        std::string two = src.substr(i, 2);
        for (const char *o : ops2)
          if (two == o) { Token t; t.t = Token::T::Punct; t.s = two; toks_.push_back(t); i += 2; matched = true; break; }
      }
      if (matched) continue;
      Token t; t.t = Token::T::Punct; t.s = std::string(1, c);
      toks_.push_back(t); ++i;
    }
    toks_.push_back(Token{Token::T::End, "", 0, false});
  }
  std::vector<Token> toks_;
};

// ===========================================================================
// Parser — recursive descent + precedence climbing.
// ===========================================================================
class Parser {
public:
  Parser(const std::vector<Token> &toks) : t_(toks) {}

  // Parse a whole translation unit -> functions + global init statements.
  void parse(std::unordered_map<std::string, Function> &funcs,
             std::vector<StmtP> &globalInits) {
    while (!at(Token::T::End)) {
      // qualifiers / decls we consume without producing a function.
      if (isId("precision")) { skipToSemicolon(); continue; }
      if (isId("layout")) { // layout(...) qualifier prefix
        // consume up to the next ';' (covers layout(...) in/out var decls).
        skipToSemicolon(); continue;
      }
      if (isId("uniform") || isId("in") || isId("out") || isId("varying") ||
          isId("attribute") || isId("flat") || isId("smooth") ||
          isId("centroid") || isId("invariant") || isId("highp") ||
          isId("mediump") || isId("lowp")) {
        skipToSemicolon(); continue;
      }
      if (isId("const")) { next(); globalInits.push_back(parseDeclRest(true)); continue; }
      // Otherwise: type ident ( -> function ;  type ident [...] -> global var.
      std::string type = expectType();
      std::string name = expectId();
      if (peek().s == "(") {
        Function fn = parseFunctionRest(type, name);
        funcs[fn.name] = std::move(fn);
      } else {
        globalInits.push_back(parseGlobalVarRest(type, name));
      }
    }
  }

private:
  // ---- token helpers ----
  const Token &peek(int o = 0) const { return t_[std::min(i_ + o, t_.size() - 1)]; }
  const Token &next() { return t_[i_ < t_.size() ? i_++ : i_]; }
  bool at(Token::T tt) const { return peek().t == tt; }
  bool isId(const char *s) const { return peek().t == Token::T::Id && peek().s == s; }
  bool isPunct(const char *s) const { return peek().t == Token::T::Punct && peek().s == s; }
  void expectPunct(const char *s) {
    if (!isPunct(s)) throw GlslError("expected '" + std::string(s) + "' but got '" + peek().s + "'");
    next();
  }
  std::string expectId() {
    if (peek().t != Token::T::Id) throw GlslError("expected identifier, got '" + peek().s + "'");
    return next().s;
  }
  std::string expectType() {
    if (peek().t != Token::T::Id) throw GlslError("expected type, got '" + peek().s + "'");
    return next().s;
  }
  void skipToSemicolon() {
    int depth = 0;
    while (!at(Token::T::End)) {
      if (isPunct("(") || isPunct("{") || isPunct("[")) ++depth;
      else if (isPunct(")") || isPunct("}") || isPunct("]")) --depth;
      else if (isPunct(";") && depth <= 0) { next(); return; }
      next();
    }
  }

  // ---- declarations ----
  StmtP parseDeclRest(bool /*isConst*/) {
    std::string type = expectType();
    std::string name = expectId();
    return parseGlobalVarRest(type, name);
  }
  StmtP parseGlobalVarRest(const std::string &type, const std::string &name) {
    auto s = std::make_shared<Stmt>();
    s->k = Stmt::K::Decl; s->declType = type; s->declName = name;
    // ignore array-size suffix on globals (uniform arrays handled via host env).
    if (isPunct("[")) { while (!isPunct("]") && !at(Token::T::End)) next(); expectPunct("]"); }
    if (isPunct("=")) { next(); s->init = parseExpr(); }
    expectPunct(";");
    return s;
  }

  Function parseFunctionRest(const std::string &retType, const std::string &name) {
    Function fn; fn.retType = retType; fn.name = name;
    expectPunct("(");
    if (!isPunct(")")) {
      while (true) {
        // skip qualifiers like in/out/inout/const on params.
        while (isId("in") || isId("out") || isId("inout") || isId("const") ||
               isId("highp") || isId("mediump") || isId("lowp"))
          next();
        std::string pt = expectType();
        std::string pn = (peek().t == Token::T::Id) ? next().s : "";
        fn.params.emplace_back(pt, pn);
        if (isPunct(",")) { next(); continue; }
        break;
      }
    }
    expectPunct(")");
    expectPunct("{");
    fn.body = parseBlockBody();
    return fn;
  }

  std::vector<StmtP> parseBlockBody() {
    std::vector<StmtP> out;
    while (!isPunct("}") && !at(Token::T::End)) out.push_back(parseStmt());
    expectPunct("}");
    return out;
  }

  bool looksLikeType() const {
    static const char *types[] = {"float", "int", "bool", "vec2", "vec3", "vec4",
                                  "mat3", "mat4", "void"};
    if (peek().t != Token::T::Id) return false;
    for (const char *ty : types) if (peek().s == ty) return true;
    return false;
  }

  StmtP parseStmt() {
    if (isPunct("{")) { next(); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Block; s->body = parseBlockBody(); return s; }
    if (isId("if")) return parseIf();
    if (isId("for")) return parseFor();
    if (isId("return")) {
      next(); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Return;
      if (!isPunct(";")) s->expr = parseExpr();
      expectPunct(";"); return s;
    }
    if (isId("discard")) { next(); expectPunct(";"); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Discard; return s; }
    if (isId("break")) { next(); expectPunct(";"); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Break; return s; }
    if (isId("continue")) { next(); expectPunct(";"); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Continue; return s; }
    if (isId("const")) { next(); return parseDeclStmt(); }
    if (looksLikeType()) return parseDeclStmt();
    // expression statement
    auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Expr; s->expr = parseExpr();
    expectPunct(";"); return s;
  }

  StmtP parseDeclStmt() {
    std::string type = expectType();
    auto s = std::make_shared<Stmt>(); s->k = Stmt::K::Decl;
    s->declType = type; s->declName = expectId();
    if (isPunct("[")) { while (!isPunct("]") && !at(Token::T::End)) next(); expectPunct("]"); }
    if (isPunct("=")) { next(); s->init = parseExpr(); }
    expectPunct(";");
    return s;
  }

  StmtP parseIf() {
    next(); expectPunct("("); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::If;
    s->expr = parseExpr(); expectPunct(")");
    s->body.push_back(parseStmt());
    if (isId("else")) { next(); s->elseBody.push_back(parseStmt()); }
    return s;
  }

  StmtP parseFor() {
    next(); expectPunct("("); auto s = std::make_shared<Stmt>(); s->k = Stmt::K::For;
    // init
    if (isPunct(";")) { next(); }
    else if (isId("const") || looksLikeType()) { if (isId("const")) next(); s->forInit = parseDeclStmt(); }
    else { auto e = std::make_shared<Stmt>(); e->k = Stmt::K::Expr; e->expr = parseExpr(); expectPunct(";"); s->forInit = e; }
    // cond
    if (!isPunct(";")) s->forCond = parseExpr();
    expectPunct(";");
    // step
    if (!isPunct(")")) s->forStep = parseExpr();
    expectPunct(")");
    s->body.push_back(parseStmt());
    return s;
  }

  // ---- expressions (precedence climbing) ----
  ExprP parseExpr() { return parseAssign(); }

  ExprP parseAssign() {
    ExprP lhs = parseCond();
    if (isPunct("=") || isPunct("+=") || isPunct("-=") || isPunct("*=") || isPunct("/=")) {
      std::string op = next().s;
      ExprP rhs = parseAssign();
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Assign; e->name = op;
      e->a = lhs; e->b = rhs; return e;
    }
    return lhs;
  }

  ExprP parseCond() {
    ExprP c = parseBinary(0);
    if (isPunct("?")) {
      next(); ExprP t = parseAssign(); expectPunct(":"); ExprP f = parseAssign();
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Cond; e->a = c; e->b = t; e->c = f;
      return e;
    }
    return c;
  }

  int prec(const std::string &op) const {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=") return 3;
    if (op == "<" || op == ">" || op == "<=" || op == ">=") return 4;
    if (op == "+" || op == "-") return 5;
    if (op == "*" || op == "/") return 6;
    return -1;
  }

  ExprP parseBinary(int minPrec) {
    ExprP lhs = parseUnary();
    while (peek().t == Token::T::Punct) {
      std::string op = peek().s;
      int p = prec(op);
      if (p < 0 || p < minPrec) break;
      next();
      ExprP rhs = parseBinary(p + 1);
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Binary; e->name = op;
      e->a = lhs; e->b = rhs; lhs = e;
    }
    return lhs;
  }

  ExprP parseUnary() {
    if (isPunct("-") || isPunct("!") || isPunct("+")) {
      std::string op = next().s;
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Unary; e->name = op;
      e->a = parseUnary(); return e;
    }
    if (isPunct("++") || isPunct("--")) {
      std::string op = next().s;
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Unary; e->name = "pre" + op;
      e->a = parseUnary(); return e;
    }
    return parsePostfix();
  }

  ExprP parsePostfix() {
    ExprP e = parsePrimary();
    while (true) {
      if (isPunct(".")) {
        next(); std::string sw = expectId();
        auto m = std::make_shared<Expr>(); m->k = Expr::K::Member; m->name = sw; m->a = e; e = m;
      } else if (isPunct("[")) {
        next(); ExprP idx = parseExpr(); expectPunct("]");
        auto m = std::make_shared<Expr>(); m->k = Expr::K::Index; m->a = e; m->b = idx; e = m;
      } else if (isPunct("++") || isPunct("--")) {
        std::string op = next().s;
        auto m = std::make_shared<Expr>(); m->k = Expr::K::Unary; m->name = "post" + op; m->a = e; e = m;
      } else break;
    }
    return e;
  }

  ExprP parsePrimary() {
    if (peek().t == Token::T::Num) {
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Num; e->num = peek().num; e->isInt = peek().isInt; next(); return e;
    }
    if (isId("true") || isId("false")) {
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Bool; e->boolVal = (peek().s == "true"); next(); return e;
    }
    if (isPunct("(")) { next(); ExprP e = parseExpr(); expectPunct(")"); return e; }
    if (peek().t == Token::T::Id) {
      std::string id = next().s;
      if (isPunct("(")) { // call or constructor
        next();
        auto e = std::make_shared<Expr>(); e->k = Expr::K::Call; e->name = id;
        if (!isPunct(")")) {
          while (true) { e->args.push_back(parseAssign()); if (isPunct(",")) { next(); continue; } break; }
        }
        expectPunct(")");
        return e;
      }
      auto e = std::make_shared<Expr>(); e->k = Expr::K::Ident; e->name = id; return e;
    }
    throw GlslError("unexpected token '" + peek().s + "' in expression");
  }

  const std::vector<Token> &t_;
  std::size_t i_ = 0;
};

// ===========================================================================
// Interpreter
// ===========================================================================
class Interpreter {
public:
  Interpreter(const std::unordered_map<std::string, Function> &funcs) : funcs_(funcs) {}

  // Environment: a stack of scopes; scope[0] is global.
  std::vector<std::unordered_map<std::string, Value>> scopes{1};

  // Per-fragment derivative inputs for dFdx/dFdy (set by the host).
  const FragmentInput *frag = nullptr;

  void define(const std::string &n, Value v) { scopes.back()[n] = std::move(v); }
  void defineGlobal(const std::string &n, Value v) { scopes.front()[n] = std::move(v); }

  Value *find(const std::string &n) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto f = it->find(n);
      if (f != it->end()) return &f->second;
    }
    return nullptr;
  }

  void run(const std::vector<StmtP> &globalInits, const std::string &entry) {
    for (const StmtP &s : globalInits) exec(s);
    auto it = funcs_.find(entry);
    if (it == funcs_.end()) throw GlslError("no '" + entry + "' function");
    callUser(it->second, {});
  }

  // ---- statement execution ----
  void exec(const StmtP &s) {
    if (++steps_ > kStepCap) throw GlslError("step limit exceeded (runaway shader)");
    switch (s->k) {
      case Stmt::K::Expr: eval(s->expr); break;
      case Stmt::K::Decl: {
        Value v = s->init ? coerce(eval(s->init), s->declType) : zeroOf(s->declType);
        define(s->declName, v); break;
      }
      case Stmt::K::Block: {
        scopes.emplace_back();
        for (const StmtP &b : s->body) exec(b);
        scopes.pop_back(); break;
      }
      case Stmt::K::If: {
        if (eval(s->expr).asBool()) { for (const StmtP &b : s->body) exec(b); }
        else { for (const StmtP &b : s->elseBody) exec(b); }
        break;
      }
      case Stmt::K::For: {
        scopes.emplace_back();
        if (s->forInit) exec(s->forInit);
        int guard = 0;
        while (!s->forCond || eval(s->forCond).asBool()) {
          if (++guard > kLoopCap) throw GlslError("loop limit exceeded");
          try {
            for (const StmtP &b : s->body) exec(b);
          } catch (const BreakSignal &) { break; }
          catch (const ContinueSignal &) { /* fall to step */ }
          if (s->forStep) eval(s->forStep);
        }
        scopes.pop_back(); break;
      }
      case Stmt::K::Return: throw ReturnSignal{ s->expr ? eval(s->expr) : Value{} };
      case Stmt::K::Discard: throw DiscardSignal{};
      case Stmt::K::Break: throw BreakSignal{};
      case Stmt::K::Continue: throw ContinueSignal{};
    }
  }

  // ---- expression evaluation ----
  Value eval(const ExprP &e) {
    switch (e->k) {
      case Expr::K::Num: return e->isInt ? Value::integer((int)e->num) : Value::flt((float)e->num);
      case Expr::K::Bool: return Value::boolean(e->boolVal);
      case Expr::K::Ident: {
        if (e->name == "gl_FragColor") { Value *p = find("FragColor"); if (p) return *p; }
        Value *p = find(e->name);
        if (!p) throw GlslError("undefined identifier '" + e->name + "'");
        return *p;
      }
      case Expr::K::Member: return evalSwizzle(eval(e->a), e->name);
      case Expr::K::Index: {
        Value base = eval(e->a); Value idx = eval(e->b);
        int i = (int)idx.scalar();
        if (base.t == VT::Array) {
          if (!base.arr || i < 0 || i >= (int)base.arr->size()) throw GlslError("array index out of range");
          return (*base.arr)[i];
        }
        if (base.isVec() || base.t == VT::Float) return Value::flt(base.f[i]);
        if (base.t == VT::Mat3) return Value::v3({base.f[i*3], base.f[i*3+1], base.f[i*3+2]});
        if (base.t == VT::Mat4) return Value::v4({base.f[i*4], base.f[i*4+1], base.f[i*4+2], base.f[i*4+3]});
        throw GlslError("cannot index this value");
      }
      case Expr::K::Unary: return evalUnary(e);
      case Expr::K::Binary: return evalBinary(e->name, eval(e->a), eval(e->b));
      case Expr::K::Cond: return eval(e->a).asBool() ? eval(e->b) : eval(e->c);
      case Expr::K::Assign: return evalAssign(e);
      case Expr::K::Call: return evalCall(e);
    }
    return {};
  }

private:
  static constexpr long kStepCap = 50'000'000;
  static constexpr int kLoopCap = 100000;

  Value zeroOf(const std::string &type) {
    if (type == "float") return Value::flt(0);
    if (type == "int") return Value::integer(0);
    if (type == "bool") return Value::boolean(false);
    if (type == "vec2") return Value::v2({0,0});
    if (type == "vec3") return Value::v3({0,0,0});
    if (type == "vec4") return Value::v4({0,0,0,0});
    Value v; v.t = VT::Void; return v;
  }

  // Coerce a value to a declared scalar type (int<->float<->bool); vectors pass.
  Value coerce(Value v, const std::string &type) {
    if (type == "float" && v.isNum()) return Value::flt(v.scalar());
    if (type == "int" && v.isNum()) return Value::integer((int)v.scalar());
    if (type == "bool" && v.isNum()) return Value::boolean(v.asBool());
    return v;
  }

  // swizzle read: chars -> indices; 1 char -> Float, else VecN.
  static int swiz(char c) {
    switch (c) {
      case 'x': case 'r': case 's': return 0;
      case 'y': case 'g': case 't': return 1;
      case 'z': case 'b': case 'p': return 2;
      case 'w': case 'a': case 'q': return 3;
      default: return -1;
    }
  }
  Value evalSwizzle(const Value &base, const std::string &sw) {
    if (sw.size() == 1) {
      int i = swiz(sw[0]); if (i < 0) throw GlslError("bad swizzle ." + sw);
      return Value::flt(base.f[i]);
    }
    Value r; r.t = sw.size() == 2 ? VT::Vec2 : (sw.size() == 3 ? VT::Vec3 : VT::Vec4);
    if (sw.size() > 4) throw GlslError("bad swizzle ." + sw);
    for (std::size_t i = 0; i < sw.size(); ++i) {
      int idx = swiz(sw[i]); if (idx < 0) throw GlslError("bad swizzle ." + sw);
      r.f[i] = base.f[idx];
    }
    return r;
  }

  Value evalUnary(const ExprP &e) {
    if (e->name == "-") { Value v = eval(e->a); for (int i = 0; i < v.comps(); ++i) v.f[i] = -v.f[i]; return v; }
    if (e->name == "+") return eval(e->a);
    if (e->name == "!") return Value::boolean(!eval(e->a).asBool());
    // ++ / -- (pre/post) — operand must be an lvalue.
    if (e->name.size() >= 5) {
      const bool pre = e->name.rfind("pre", 0) == 0;
      const bool inc = e->name.find("++") != std::string::npos;
      Value *lv = lvalue(e->a);
      if (!lv) throw GlslError("++/-- requires an lvalue");
      Value old = *lv;
      lv->f[0] += inc ? 1.0f : -1.0f;
      return pre ? *lv : old;
    }
    throw GlslError("bad unary op " + e->name);
  }

  // Resolve an expression to a modifiable storage slot (Ident only; swizzle/index
  // handled in evalAssign by read-modify-write).
  Value *lvalue(const ExprP &e) {
    if (e->k == Expr::K::Ident) return find(e->name);
    return nullptr;
  }

  Value evalAssign(const ExprP &e) {
    const std::string &op = e->name;
    // Plain ident target.
    if (e->a->k == Expr::K::Ident) {
      Value rhs = eval(e->b);
      Value *cur = find(e->a->name);
      if (!cur) { defineGlobal(e->a->name, applyAssign(op, Value{}, rhs, true)); return *find(e->a->name); }
      *cur = applyAssign(op, *cur, rhs, false);
      return *cur;
    }
    // Swizzle target: x.yz = ... (read base, splice components, write back).
    if (e->a->k == Expr::K::Member) {
      Value *base = lvalue(e->a->a);
      if (!base) throw GlslError("swizzle assign requires an lvalue base");
      Value rhs = eval(e->b);
      const std::string &sw = e->a->name;
      for (std::size_t i = 0; i < sw.size(); ++i) {
        int idx = swiz(sw[i]); if (idx < 0) throw GlslError("bad swizzle ." + sw);
        float r = (sw.size() == 1) ? rhs.f[0] : rhs.f[i];
        base->f[idx] = applyScalarAssign(op, base->f[idx], r);
      }
      return *base;
    }
    // Index target on a vector (v[i] = ...).
    if (e->a->k == Expr::K::Index) {
      Value *base = lvalue(e->a->a);
      if (!base) throw GlslError("index assign requires an lvalue base");
      int i = (int)eval(e->a->b).scalar();
      Value rhs = eval(e->b);
      base->f[i] = applyScalarAssign(op, base->f[i], rhs.f[0]);
      return *base;
    }
    throw GlslError("invalid assignment target");
  }

  float applyScalarAssign(const std::string &op, float cur, float r) {
    if (op == "=") return r;
    if (op == "+=") return cur + r;
    if (op == "-=") return cur - r;
    if (op == "*=") return cur * r;
    if (op == "/=") return cur / r;
    throw GlslError("bad assign op " + op);
  }
  Value applyAssign(const std::string &op, Value cur, Value rhs, bool fresh) {
    if (op == "=" || fresh) return rhs;
    return evalBinary(std::string(1, op[0]), cur, rhs); // += -> +, etc.
  }

  // Binary ops with GLSL scalar/vector/matrix broadcast.
  Value evalBinary(const std::string &op, Value A, Value B) {
    // logical / comparison first.
    if (op == "&&") return Value::boolean(A.asBool() && B.asBool());
    if (op == "||") return Value::boolean(A.asBool() || B.asBool());
    if (op == "==") return Value::boolean(A.scalar() == B.scalar());
    if (op == "!=") return Value::boolean(A.scalar() != B.scalar());
    if (op == "<")  return Value::boolean(A.scalar() <  B.scalar());
    if (op == ">")  return Value::boolean(A.scalar() >  B.scalar());
    if (op == "<=") return Value::boolean(A.scalar() <= B.scalar());
    if (op == ">=") return Value::boolean(A.scalar() >= B.scalar());

    // matrix * vector / matrix * matrix.
    if ((A.t == VT::Mat3 || A.t == VT::Mat4) && op == "*") {
      if (A.t == VT::Mat3 && B.t == VT::Vec3) return Value::v3(toMat3(A) * B.vec3v());
      if (A.t == VT::Mat4 && B.t == VT::Vec4) return Value::v4(toMat4(A) * B.vec4v());
      if (A.t == B.t) { // mat*mat
        if (A.t == VT::Mat4) { glsl::mat4 r = toMat4(A) * toMat4(B); Value v; v.t = VT::Mat4; v.f = r.m; return v; }
      }
      if (B.isNum()) { Value r = A; for (int i = 0; i < A.comps(); ++i) r.f[i] *= B.scalar(); return r; }
    }

    // numeric scalar/vector elementwise with broadcast.
    const bool aVec = A.isVec(), bVec = B.isVec();
    auto applyf = [&](float x, float y) {
      if (op == "+") return x + y; if (op == "-") return x - y;
      if (op == "*") return x * y; if (op == "/") return x / y;
      throw GlslError("bad numeric op " + op);
    };
    if (aVec && bVec) {
      if (A.comps() != B.comps()) throw GlslError("vector size mismatch in '" + op + "'");
      Value r = A; for (int i = 0; i < A.comps(); ++i) r.f[i] = applyf(A.f[i], B.f[i]); return r;
    }
    if (aVec && B.isNum()) { Value r = A; for (int i = 0; i < A.comps(); ++i) r.f[i] = applyf(A.f[i], B.scalar()); return r; }
    if (A.isNum() && bVec) { Value r = B; for (int i = 0; i < B.comps(); ++i) r.f[i] = applyf(A.scalar(), B.f[i]); return r; }
    // scalar op scalar.
    float v = applyf(A.scalar(), B.scalar());
    return (A.t == VT::Int && B.t == VT::Int && op != "/") ? Value::integer((int)v) : Value::flt(v);
  }

  static glsl::mat3 toMat3(const Value &v) { glsl::mat3 m; for (int i = 0; i < 9; ++i) m.m[i] = v.f[i]; return m; }
  static glsl::mat4 toMat4(const Value &v) { glsl::mat4 m; for (int i = 0; i < 16; ++i) m.m[i] = v.f[i]; return m; }

  // ---- calls: constructors, builtins, user functions ----
  Value evalCall(const ExprP &e) {
    const std::string &name = e->name;

    // Constructors / casts.
    if (name == "float") return Value::flt(eval(e->args[0]).scalar());
    if (name == "int")   return Value::integer((int)eval(e->args[0]).scalar());
    if (name == "bool")  return Value::boolean(eval(e->args[0]).asBool());
    if (name == "vec2" || name == "vec3" || name == "vec4")
      return constructVec(name, e->args);
    if (name == "mat3" || name == "mat4")
      return constructMat(name, e->args);

    // dFdx / dFdy / fwidth — exact only for the precomputed varyings.
    if (name == "dFdx" || name == "dFdy" || name == "fwidth")
      return evalDerivative(name, e->args[0]);

    // Builtins.
    std::vector<Value> a; a.reserve(e->args.size());
    for (const ExprP &arg : e->args) a.push_back(eval(arg));
    Value out;
    if (callBuiltin(name, a, out)) return out;

    // User function.
    auto it = funcs_.find(name);
    if (it != funcs_.end()) return callUser(it->second, a);

    throw GlslError("unknown function '" + name + "'");
  }

  Value constructVec(const std::string &name, const std::vector<ExprP> &args) {
    const int n = name == "vec2" ? 2 : (name == "vec3" ? 3 : 4);
    std::array<float, 4> comp{}; int filled = 0;
    for (const ExprP &arg : args) {
      Value v = eval(arg);
      if (v.isNum()) { if (filled < n) comp[filled++] = v.scalar(); }
      else for (int i = 0; i < v.comps() && filled < n; ++i) comp[filled++] = v.f[i];
    }
    if (filled == 1) for (int i = 1; i < n; ++i) comp[i] = comp[0]; // vecN(scalar) splat.
    Value r; r.t = n == 2 ? VT::Vec2 : (n == 3 ? VT::Vec3 : VT::Vec4);
    for (int i = 0; i < n; ++i) r.f[i] = comp[i];
    return r;
  }

  Value constructMat(const std::string &name, const std::vector<ExprP> &args) {
    const int n = name == "mat3" ? 3 : 4;
    Value r; r.t = n == 3 ? VT::Mat3 : VT::Mat4;
    std::vector<float> flat;
    for (const ExprP &arg : args) { Value v = eval(arg); for (int i = 0; i < v.comps(); ++i) flat.push_back(v.f[i]); }
    if (flat.size() == 1) { // mat(scalar) -> scalar * identity
      for (int c = 0; c < n; ++c) r.f[c * n + c] = flat[0];
    } else {
      for (int i = 0; i < n * n && i < (int)flat.size(); ++i) r.f[i] = flat[i];
    }
    return r;
  }

  Value evalDerivative(const std::string &name, const ExprP &arg) {
    auto pick = [&](glsl::vec3 dx, glsl::vec3 dy) -> Value {
      if (name == "dFdx") return Value::v3(dx);
      if (name == "dFdy") return Value::v3(dy);
      return Value::v3(glsl::absv(dx) + glsl::absv(dy)); // fwidth
    };
    auto pick2 = [&](glsl::vec2 dx, glsl::vec2 dy) -> Value {
      if (name == "dFdx") return Value::v2(dx);
      if (name == "dFdy") return Value::v2(dy);
      return Value::v2({std::fabs(dx.x)+std::fabs(dy.x), std::fabs(dx.y)+std::fabs(dy.y)});
    };
    if (frag && arg->k == Expr::K::Ident) {
      if (arg->name == "vPosEye") return pick(frag->dPosEyeDx, frag->dPosEyeDy);
      if (arg->name == "vTexCoord") return pick2(frag->dTexDx, frag->dTexDy);
    }
    // Unknown derivand: zero of the operand's type (documented narrowing).
    Value v = eval(arg);
    for (int i = 0; i < v.comps(); ++i) v.f[i] = 0.0f;
    return v;
  }

  // Returns true and sets `out` if `name` is a known builtin.
  bool callBuiltin(const std::string &name, std::vector<Value> &a, Value &out) {
    auto unary = [&](float (*fn)(float)) {
      Value r = a[0]; for (int i = 0; i < r.comps(); ++i) r.f[i] = fn(r.f[i]); out = r;
    };
    if (name == "abs")   { unary([](float x){return std::fabs(x);}); return true; }
    if (name == "floor") { unary([](float x){return std::floor(x);}); return true; }
    if (name == "ceil")  { unary([](float x){return std::ceil(x);}); return true; }
    if (name == "fract") { unary([](float x){return x - std::floor(x);}); return true; }
    if (name == "sqrt")  { unary([](float x){return std::sqrt(x);}); return true; }
    if (name == "sin")   { unary([](float x){return std::sin(x);}); return true; }
    if (name == "cos")   { unary([](float x){return std::cos(x);}); return true; }
    if (name == "tan")   { unary([](float x){return std::tan(x);}); return true; }
    if (name == "exp")   { unary([](float x){return std::exp(x);}); return true; }
    if (name == "log")   { unary([](float x){return std::log(x);}); return true; }
    if (name == "sign")  { unary([](float x){return (float)((x>0)-(x<0));}); return true; }
    if (name == "radians"){ unary([](float x){return x*0.01745329252f;}); return true; }
    if (name == "degrees"){ unary([](float x){return x*57.2957795131f;}); return true; }

    if (name == "length")   { out = Value::flt(vlen(a[0])); return true; }
    if (name == "normalize"){ out = vnormalize(a[0]); return true; }
    if (name == "dot")      { out = Value::flt(vdot(a[0], a[1])); return true; }
    if (name == "distance") { out = Value::flt(vlen(vsub(a[0], a[1]))); return true; }
    if (name == "cross")    { out = Value::v3(glsl::cross(a[0].vec3v(), a[1].vec3v())); return true; }
    if (name == "reflect")  { out = Value::v3(glsl::reflect(a[0].vec3v(), a[1].vec3v())); return true; }
    if (name == "pow")      { out = binaryFn(a[0], a[1], [](float x,float y){return std::pow(x,y);}); return true; }
    if (name == "mod")      { out = binaryFn(a[0], a[1], [](float x,float y){return x - y*std::floor(x/y);}); return true; }
    if (name == "min")      { out = binaryFn(a[0], a[1], [](float x,float y){return x<y?x:y;}); return true; }
    if (name == "max")      { out = binaryFn(a[0], a[1], [](float x,float y){return x>y?x:y;}); return true; }
    if (name == "step")     { out = binaryFn(a[0], a[1], [](float edge,float x){return x<edge?0.f:1.f;}); return true; }
    if (name == "clamp")    { out = clampFn(a); return true; }
    if (name == "mix")      { out = mixFn(a); return true; }
    if (name == "smoothstep") { out = smoothstepFn(a); return true; }
    if (name == "texture" || name == "texture2D") {
      const Texture *s = a[0].sampler;
      glsl::vec2 uv = a[1].vec2v();
      out = Value::v4(s ? s->sample(uv) : glsl::vec4{1,1,1,1});
      return true;
    }
    return false;
  }

  // builtin helpers (component-aware).
  static float vlen(const Value &v) { float s = 0; for (int i = 0; i < v.comps(); ++i) s += v.f[i]*v.f[i]; return std::sqrt(s); }
  static float vdot(const Value &a, const Value &b) { float s = 0; for (int i = 0; i < a.comps(); ++i) s += a.f[i]*b.f[i]; return s; }
  static Value vsub(const Value &a, const Value &b) { Value r = a; for (int i = 0; i < a.comps(); ++i) r.f[i] = a.f[i]-b.f[i]; return r; }
  static Value vnormalize(const Value &v) { float L = vlen(v); Value r = v; if (L > 1e-12f) for (int i = 0; i < v.comps(); ++i) r.f[i] /= L; return r; }
  template <class F> static Value binaryFn(const Value &a, const Value &b, F fn) {
    if (a.isVec() && b.isNum()) { Value r = a; for (int i = 0; i < a.comps(); ++i) r.f[i] = fn(a.f[i], b.scalar()); return r; }
    if (a.isVec() && b.isVec()) { Value r = a; for (int i = 0; i < a.comps(); ++i) r.f[i] = fn(a.f[i], b.f[i]); return r; }
    if (a.isNum() && b.isVec()) { Value r = b; for (int i = 0; i < b.comps(); ++i) r.f[i] = fn(a.scalar(), b.f[i]); return r; }
    return Value::flt(fn(a.scalar(), b.scalar()));
  }
  static Value clampFn(std::vector<Value> &a) {
    Value x = a[0], lo = a[1], hi = a[2];
    Value r = x; for (int i = 0; i < x.comps(); ++i) {
      float l = lo.isVec() ? lo.f[i] : lo.scalar();
      float h = hi.isVec() ? hi.f[i] : hi.scalar();
      r.f[i] = glsl::clampf(x.f[i], l, h);
    }
    return r;
  }
  static Value mixFn(std::vector<Value> &a) {
    Value x = a[0], y = a[1], t = a[2];
    Value r = x; for (int i = 0; i < x.comps(); ++i) {
      float tt = t.isVec() ? t.f[i] : t.scalar();
      r.f[i] = glsl::mixf(x.f[i], y.f[i], tt);
    }
    return r;
  }
  static Value smoothstepFn(std::vector<Value> &a) {
    Value e0 = a[0], e1 = a[1], x = a[2];
    Value r = x; for (int i = 0; i < x.comps(); ++i) {
      float a0 = e0.isVec()?e0.f[i]:e0.scalar();
      float a1 = e1.isVec()?e1.f[i]:e1.scalar();
      float t = glsl::clampf((x.f[i]-a0)/(a1-a0), 0.f, 1.f);
      r.f[i] = t*t*(3.f-2.f*t);
    }
    return r;
  }

  Value callUser(const Function &fn, const std::vector<Value> &args) {
    scopes.emplace_back();
    for (std::size_t i = 0; i < fn.params.size() && i < args.size(); ++i)
      define(fn.params[i].second, coerce(args[i], fn.params[i].first));
    Value ret;
    try {
      for (const StmtP &s : fn.body) exec(s);
    } catch (const ReturnSignal &r) { ret = r.v; }
    scopes.pop_back();
    return ret;
  }

  const std::unordered_map<std::string, Function> &funcs_;
  long steps_ = 0;
};

// ===========================================================================
// InterpretedProgram — parse once, run many fragments.
// ===========================================================================
class InterpretedProgram {
public:
  bool compile(const std::string &src, std::string *err = nullptr) {
    try {
      Lexer lex(src);
      Parser parser(lex.tokens());
      funcs_.clear(); globalInits_.clear();
      parser.parse(funcs_, globalInits_);
      if (funcs_.find("main") == funcs_.end())
        throw GlslError("no main() in shader");
      compiled_ = true;
      return true;
    } catch (const std::exception &e) {
      if (err) *err = e.what();
      compiled_ = false;
      return false;
    }
  }
  bool compiled() const { return compiled_; }
  const std::unordered_map<std::string, Function> &functions() const { return funcs_; }
  const std::vector<StmtP> &globalInits() const { return globalInits_; }

private:
  std::unordered_map<std::string, Function> funcs_;
  std::vector<StmtP> globalInits_;
  bool compiled_ = false;
};

// ===========================================================================
// Host glue: seed uniforms/varyings, run main(), read FragColor.
// ===========================================================================

// Build the per-draw constant uniform environment from the material + lights.
inline std::unordered_map<std::string, Value>
seedUniforms(const x3d::runtime::extract::MaterialDesc &m,
             const std::vector<EyeLight> &lights, const MaterialTextures &tx) {
  std::unordered_map<std::string, Value> u;
  u["uDiffuse"] = Value::v4(glsl::vec4(m.toRGBA()));
  u["uBaseColor"] = Value::v4({m.physical.baseColor.r, m.physical.baseColor.g,
                               m.physical.baseColor.b, 1.0f - m.transparency});
  u["uEmissive"] = Value::v3(glsl::vec3(m.emissive));
  u["uAmbientColor"] = Value::v3(glsl::vec4(m.toRGBA()).xyz() * m.phong.ambientIntensity);
  u["uSpecular"] = Value::v3(glsl::vec3(m.phong.specular));
  u["uShininess"] = Value::flt(m.phong.shininess);
  u["uMetallic"] = Value::flt(m.physical.metallic);
  u["uRoughness"] = Value::flt(m.physical.roughness);
  u["uTransparency"] = Value::flt(m.transparency);
  u["uNormalScale"] = Value::flt(m.normalScale);
  u["uAlphaCutoff"] = Value::flt(m.alphaCutoff);
  u["uAlphaMode"] = Value::integer(static_cast<int>(m.alphaMode));
  u["uTime"] = Value::flt(0.0f);
  u["uNumLights"] = Value::integer(static_cast<int>(lights.size()));
  // Light arrays.
  auto dirArr = std::make_shared<std::vector<Value>>();
  auto colArr = std::make_shared<std::vector<Value>>();
  for (const EyeLight &L : lights) {
    dirArr->push_back(Value::v3(L.dirEye));
    colArr->push_back(Value::v3(L.color));
  }
  Value dv; dv.t = VT::Array; dv.arr = dirArr; u["uLightDirEye"] = dv;
  Value cv; cv.t = VT::Array; cv.arr = colArr; u["uLightColor"] = cv;
  u["uOcclusionStrength"] = Value::flt(m.physical.occlusionStrength);
  // UsdPreviewSurface fidelity params: the X3D PhysicalMaterial node cannot carry
  // these, so the direct render path seeds the spec-default fallbacks (metallic
  // workflow, ior 1.5, no clearcoat, opacityMode transparent). At these values the
  // portable shader is bit-parity with the native fixed-function PBR; only the
  // asset-import --emit-glsl path (which bakes constants) exercises the extras.
  u["uUseSpecularWorkflow"] = Value::integer(0);
  u["uSpecularColor"] = Value::v3(glsl::vec3(m.phong.specular));
  u["uIor"] = Value::flt(1.5f);
  u["uClearcoat"] = Value::flt(0.0f);
  u["uClearcoatRoughness"] = Value::flt(0.01f);
  u["uOpacityMode"] = Value::integer(0);
  u["uOpacityThreshold"] = Value::flt(m.alphaCutoff);
  // Texture slots. Every sampler pointer is bound unconditionally (the pointer is
  // stable even when the Texture is empty); the paired uHas*Tex flag tells the
  // shader whether to sample, so an author shader that guards on the flag never
  // reads an invalid slot. uTexture is retained as the legacy base-color alias.
  if (tx.base.valid()) u["uTexture"] = Value::samp(&tx.base);
  u["uBaseColorTex"] = Value::samp(&tx.base);
  u["uNormalTex"] = Value::samp(&tx.normal);
  u["uEmissiveTex"] = Value::samp(&tx.emissive);
  u["uMetallicRoughnessTex"] = Value::samp(&tx.mr);
  u["uOcclusionTex"] = Value::samp(&tx.occlusion);
  u["uHasBaseColorTex"] = Value::integer(tx.base.valid() ? 1 : 0);
  u["uHasNormalTex"] = Value::integer(tx.normal.valid() ? 1 : 0);
  u["uHasEmissiveTex"] = Value::integer(tx.emissive.valid() ? 1 : 0);
  u["uHasMetallicRoughnessTex"] = Value::integer(tx.mr.valid() ? 1 : 0);
  u["uHasOcclusionTex"] = Value::integer(tx.occlusion.valid() ? 1 : 0);
  return u;
}

// Make a FragmentShader that interprets `prog` per fragment. Captures the seeded
// uniforms + textures by value so the closure is self-contained.
inline FragmentShader
makeInterpretedShader(const InterpretedProgram &prog,
                      const x3d::runtime::extract::MaterialDesc &material,
                      const std::vector<EyeLight> &lights, bool hasColors) {
  auto tx = std::make_shared<MaterialTextures>(buildTextures(material));
  auto base = std::make_shared<std::unordered_map<std::string, Value>>(
      seedUniforms(material, lights, *tx));
  const InterpretedProgram *pp = &prog;
  glsl::vec4 fallback = glsl::vec4(material.toRGBA());

  return [pp, base, tx, hasColors, fallback](const FragmentInput &f,
                                             glsl::vec4 &out) -> bool {
    Interpreter interp(pp->functions());
    interp.frag = &f;
    // Global scope: uniforms + varyings.
    for (const auto &kv : *base) interp.defineGlobal(kv.first, kv.second);
    interp.defineGlobal("vPosEye", Value::v3(f.posEye));
    interp.defineGlobal("vNormalEye", Value::v3(f.normalEye));
    interp.defineGlobal("vColor", Value::v4(f.color));
    interp.defineGlobal("vTexCoord", Value::v2(f.texcoord));
    interp.defineGlobal("gl_FrontFacing", Value::boolean(f.frontFacing));
    interp.defineGlobal("uHasColors", Value::integer(hasColors ? 1 : 0));
    interp.defineGlobal("FragColor", Value::v4(fallback));
    try {
      interp.run(pp->globalInits(), "main");
    } catch (const DiscardSignal &) {
      return false;
    } catch (const std::exception &) {
      out = fallback; // a runtime error draws the flat material color (robust).
      return true;
    }
    Value *fc = interp.find("FragColor");
    out = fc ? fc->vec4v() : fallback;
    return true;
  };
}

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_GLSL_INTERPRETER_HPP
