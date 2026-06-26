// QuickJsBackend.cpp
// The single TU where quickjs-ng (https://github.com/quickjs-ng/quickjs) meets
// the ScriptEngine seam. This is the ONLY place a quickjs.h type appears; the
// header (QuickJsBackend.hpp) is QuickJS-free via the Impl pImpl, mirroring the
// JoltBackend isolation rule so consumers of x3d_quickjs never inherit QuickJS's
// headers or flags.
//
// U2 SCOPE: FULL PARITY with the Duktape EcmaScriptBackend —
//   - pushValue(std::any, X3DFieldType) -> JSValue and the inverse
//     toValue(JSValue, X3DFieldType) -> std::any for every SF/MF type
//     (Bool/Int32/Float/Double/Time/String, Vec2/3/4 f+d, Color, ColorRGBA,
//     Rotation, Node; all MF* as JS arrays; SFMatrix3/4 f+d as flat row-major
//     arrays; SFImage/MFImage as the ISO 19777-1 {x,y,comp,array} shape),
//     matching the JS SHAPE Duktape exposes case-for-case;
//   - the Browser global (getName/getVersion/currentTime/currentFrameRate/
//     addRoute/deleteRoute/print) backed by the SaiContext;
//   - author-field seeding + readback (initialValue -> JS globals at init, and
//     outputOnly/inputOutput writes read back into the store + emitted as
//     cascade events), mirroring EcmaScriptBackend's mechanism via SaiContext;
//   - invoke() marshals `value` via pushValue and calls handler(value,timestamp).
//
// REFCOUNT DISCIPLINE (the top bug risk, design §Risks 1): every JSValue that
// JS_Eval / JS_Get* / JS_Call / JS_New* hands back is OWNED and must be
// JS_FreeValue'd. To make that mechanical we wrap owned values in a small RAII
// guard (JsValue below) so no path leaks or double-frees. JS_Free'ing the
// special tags (UNDEFINED/NULL/EXCEPTION/bool/int) is a documented no-op, so the
// guard is safe to apply uniformly. The key QuickJS idioms vs Duktape:
//   - JS_SetPropertyStr / JS_SetPropertyUint32 CONSUME the value ref passed in
//     (do NOT wrap a value you hand to them in a JsValue — ownership transfers).
//   - JS_GetPropertyStr / JS_GetPropertyUint32 / JS_Call RETURN owned refs.
//   - JS_NewString COPIES its input, so a std::any temporary holding the source
//     string must stay alive until the call returns (carried as the U1 review
//     note (a)).

#include "QuickJsBackend.hpp"

#include "DynamicField.hpp" // author-field store (mirrors EcmaScriptBackend §3.5)
#include "SaiContext.hpp"   // post author outputs into the cascade
#include "X3DNode.hpp"      // SFNode wrapping (X3DNode*)
#include "X3Dtypes.hpp"     // SF*/MF* concrete C++ types

#include "quickjs.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace x3d::runtime {

namespace {

// ---------------------------------------------------------------------------
// RAII guard for an OWNED JSValue. Frees on scope exit against the context it
// was created in. Move-only (transfers ownership); release() hands the raw
// value out without freeing (for the rare case ownership must pass on).
// ---------------------------------------------------------------------------
class JsValue {
public:
  JsValue(JSContext *ctx, JSValue v) : ctx_(ctx), v_(v) {}
  ~JsValue() {
    if (ctx_) JS_FreeValue(ctx_, v_);
  }
  JsValue(JsValue &&o) noexcept : ctx_(o.ctx_), v_(o.v_) {
    o.ctx_ = nullptr;
    o.v_ = JS_UNDEFINED;
  }
  JsValue &operator=(JsValue &&o) noexcept {
    if (this != &o) {
      if (ctx_) JS_FreeValue(ctx_, v_);
      ctx_ = o.ctx_;
      v_ = o.v_;
      o.ctx_ = nullptr;
      o.v_ = JS_UNDEFINED;
    }
    return *this;
  }
  JsValue(const JsValue &) = delete;
  JsValue &operator=(const JsValue &) = delete;

  const JSValue &get() const { return v_; }
  // Hand the raw value out WITHOUT freeing (ownership passes to the caller).
  JSValue release() {
    JSValue out = v_;
    ctx_ = nullptr;
    v_ = JS_UNDEFINED;
    return out;
  }
  bool isException() const { return JS_IsException(v_); }
  bool isUndefined() const { return JS_IsUndefined(v_); }

private:
  JSContext *ctx_ = nullptr;
  JSValue v_ = JS_UNDEFINED;
};

// Drain and log a pending QuickJS exception (clears it). Used after any
// JS_Eval / JS_Call that returned JS_EXCEPTION so the context stays clean.
void logException(JSContext *ctx, const char *where) {
  JsValue exc(ctx, JS_GetException(ctx));
  const char *str = JS_ToCString(ctx, exc.get());
  std::cerr << "[QuickJsBackend] " << where << " error: "
            << (str ? str : "<unprintable>") << "\n";
  if (str) JS_FreeCString(ctx, str);
}

// ---------------------------------------------------------------------------
// Scalar reader helpers (the JS_To* family returns -1 on a failed coercion;
// we treat that as the zero value, matching Duktape's lenient duk_to_*).
// ---------------------------------------------------------------------------

double toNumber(JSContext *ctx, JSValueConst v) {
  double d = 0.0;
  JS_ToFloat64(ctx, &d, v);  // leaves d untouched (0) on error
  return d;
}
int32_t toInt32(JSContext *ctx, JSValueConst v) {
  int32_t i = 0;
  JS_ToInt32(ctx, &i, v);
  return i;
}
bool toBool(JSContext *ctx, JSValueConst v) {
  int b = JS_ToBool(ctx, v);
  return b > 0;
}
SFString toStdString(JSContext *ctx, JSValueConst v) {
  size_t len = 0;
  const char *s = JS_ToCStringLen(ctx, &len, v);
  if (!s) return SFString{};
  SFString out(s, len);
  JS_FreeCString(ctx, s);
  return out;
}

// Read a named numeric property off a JS object (owned ref freed here).
double getNumberProp(JSContext *ctx, JSValueConst obj, const char *name) {
  JsValue p(ctx, JS_GetPropertyStr(ctx, obj, name));
  return toNumber(ctx, p.get());
}

// ---------------------------------------------------------------------------
// SFNode <-> JS opaque handle. QuickJS has no pointer value, so a non-null node
// becomes a plain JS object carrying the X3DNode* in a hidden own property; a
// null node becomes JS null (mirroring Duktape's pushNode/extractNode). The
// pointer is stashed as a 64-bit integer split across two number fields so it
// survives QuickJS's number representation losslessly on LP64.
// ---------------------------------------------------------------------------

constexpr const char *kNodePtrLoKey = "\xff" "x3dNodeLo";
constexpr const char *kNodePtrHiKey = "\xff" "x3dNodeHi";

JSValue pushNode(JSContext *ctx, const SFNode &node) {
  if (!node) return JS_NULL;
  JSValue obj = JS_NewObject(ctx);
  if (JS_IsException(obj)) return obj;
  const auto bits = reinterpret_cast<uintptr_t>(node.get());
  const auto lo = static_cast<uint32_t>(bits & 0xFFFFFFFFu);
  const auto hi = static_cast<uint32_t>((static_cast<uint64_t>(bits) >> 32) &
                                        0xFFFFFFFFu);
  // JS_SetPropertyStr consumes the value ref it is handed.
  JS_SetPropertyStr(ctx, obj, kNodePtrLoKey, JS_NewUint32(ctx, lo));
  JS_SetPropertyStr(ctx, obj, kNodePtrHiKey, JS_NewUint32(ctx, hi));
  return obj;
}

X3DNode *extractNode(JSContext *ctx, JSValueConst v) {
  if (JS_IsNull(v) || JS_IsUndefined(v) || !JS_IsObject(v)) return nullptr;
  JsValue lo(ctx, JS_GetPropertyStr(ctx, v, kNodePtrLoKey));
  JsValue hi(ctx, JS_GetPropertyStr(ctx, v, kNodePtrHiKey));
  if (lo.isUndefined() || hi.isUndefined()) return nullptr;
  uint32_t loBits = 0, hiBits = 0;
  JS_ToUint32(ctx, &loBits, lo.get());
  JS_ToUint32(ctx, &hiBits, hi.get());
  const auto bits = (static_cast<uint64_t>(hiBits) << 32) |
                    static_cast<uint64_t>(loBits);
  return reinterpret_cast<X3DNode *>(static_cast<uintptr_t>(bits));
}

// ---------------------------------------------------------------------------
// Structured-SF pushers — each returns a fresh OWNED JS object whose named
// numeric fields mirror the Duktape shape exactly (SFVec3f -> {x,y,z};
// SFColor -> {r,g,b}; SFRotation -> {x,y,z,angle}; etc).
// ---------------------------------------------------------------------------

void setNum(JSContext *ctx, JSValue obj, const char *name, double v) {
  JS_SetPropertyStr(ctx, obj, name, JS_NewFloat64(ctx, v));  // consumes ref
}

JSValue pushSFVec2f(JSContext *c, const SFVec2f &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  return o;
}
JSValue pushSFVec2d(JSContext *c, const SFVec2d &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  return o;
}
JSValue pushSFVec3f(JSContext *c, const SFVec3f &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  setNum(c, o, "z", v.z); return o;
}
JSValue pushSFVec3d(JSContext *c, const SFVec3d &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  setNum(c, o, "z", v.z); return o;
}
JSValue pushSFVec4f(JSContext *c, const SFVec4f &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  setNum(c, o, "z", v.z); setNum(c, o, "w", v.w); return o;
}
JSValue pushSFVec4d(JSContext *c, const SFVec4d &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  setNum(c, o, "z", v.z); setNum(c, o, "w", v.w); return o;
}
JSValue pushSFColor(JSContext *c, const SFColor &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "r", v.r); setNum(c, o, "g", v.g);
  setNum(c, o, "b", v.b); return o;
}
JSValue pushSFColorRGBA(JSContext *c, const SFColorRGBA &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "r", v.r); setNum(c, o, "g", v.g);
  setNum(c, o, "b", v.b); setNum(c, o, "a", v.a); return o;
}
JSValue pushSFRotation(JSContext *c, const SFRotation &v) {
  JSValue o = JS_NewObject(c); setNum(c, o, "x", v.x); setNum(c, o, "y", v.y);
  setNum(c, o, "z", v.z); setNum(c, o, "angle", v.angle); return o;
}

// ---------------------------------------------------------------------------
// Structured-SF poppers — read named numeric fields back off a JS object.
// ---------------------------------------------------------------------------

SFVec2f toSFVec2f(JSContext *c, JSValueConst i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y")};
}
SFVec2d toSFVec2d(JSContext *c, JSValueConst i) {
  return {getNumberProp(c, i, "x"), getNumberProp(c, i, "y")};
}
SFVec3f toSFVec3f(JSContext *c, JSValueConst i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y"),
          (float)getNumberProp(c, i, "z")};
}
SFVec3d toSFVec3d(JSContext *c, JSValueConst i) {
  return {getNumberProp(c, i, "x"), getNumberProp(c, i, "y"),
          getNumberProp(c, i, "z")};
}
SFVec4f toSFVec4f(JSContext *c, JSValueConst i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y"),
          (float)getNumberProp(c, i, "z"), (float)getNumberProp(c, i, "w")};
}
SFVec4d toSFVec4d(JSContext *c, JSValueConst i) {
  return {getNumberProp(c, i, "x"), getNumberProp(c, i, "y"),
          getNumberProp(c, i, "z"), getNumberProp(c, i, "w")};
}
SFColor toSFColor(JSContext *c, JSValueConst i) {
  return {(float)getNumberProp(c, i, "r"), (float)getNumberProp(c, i, "g"),
          (float)getNumberProp(c, i, "b")};
}
SFColorRGBA toSFColorRGBA(JSContext *c, JSValueConst i) {
  return {(float)getNumberProp(c, i, "r"), (float)getNumberProp(c, i, "g"),
          (float)getNumberProp(c, i, "b"), (float)getNumberProp(c, i, "a")};
}
SFRotation toSFRotation(JSContext *c, JSValueConst i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y"),
          (float)getNumberProp(c, i, "z"), (float)getNumberProp(c, i, "angle")};
}

// ---------------------------------------------------------------------------
// Generic MF push/pop via element functors. JS_NewArray + JS_SetPropertyUint32
// (which consumes the element ref) for push; length + JS_GetPropertyUint32
// (owned ref, freed per element) for pop.
// ---------------------------------------------------------------------------

template <typename Vec, typename PushElem>
JSValue pushMF(JSContext *c, const Vec &v, PushElem pushElem) {
  JSValue arr = JS_NewArray(c);
  if (JS_IsException(arr)) return arr;
  for (std::size_t i = 0; i < v.size(); ++i) {
    JSValue elem = pushElem(c, v[i]);          // owned
    JS_SetPropertyUint32(c, arr, (uint32_t)i, elem);  // consumes elem
  }
  return arr;
}

template <typename Vec, typename ToElem>
Vec toMF(JSContext *c, JSValueConst arr, ToElem toElem) {
  Vec out;
  if (!JS_IsObject(arr)) return out;
  uint32_t n = 0;
  {
    JsValue len(c, JS_GetPropertyStr(c, arr, "length"));
    JS_ToUint32(c, &n, len.get());
  }
  out.reserve(n);
  for (uint32_t i = 0; i < n; ++i) {
    JsValue elem(c, JS_GetPropertyUint32(c, arr, i));
    out.push_back(toElem(c, elem.get()));
  }
  return out;
}

// SFMatrix <-> JS: a flat row-major array of N*N numbers (19777-1 ECMAScript).
template <typename M, int N> JSValue pushSFMatrix(JSContext *c, const M &m) {
  JSValue arr = JS_NewArray(c);
  if (JS_IsException(arr)) return arr;
  uint32_t k = 0;
  for (int r = 0; r < N; ++r)
    for (int col = 0; col < N; ++col)
      JS_SetPropertyUint32(c, arr, k++,
                           JS_NewFloat64(c, static_cast<double>(m.matrix[r][col])));
  return arr;
}
template <typename M, int N, typename T>
M toSFMatrix(JSContext *c, JSValueConst arr) {
  M m{};
  if (!JS_IsObject(arr)) return m;
  uint32_t k = 0;
  for (int r = 0; r < N; ++r)
    for (int col = 0; col < N; ++col) {
      JsValue cell(c, JS_GetPropertyUint32(c, arr, k++));
      m.matrix[r][col] = static_cast<T>(toNumber(c, cell.get()));
    }
  return m;
}

// SFImage <-> JS (ISO 19777-1 ECMAScript binding): an object with x, y, comp,
// and array (MFInt32 of packed pixels, high-byte-first — same packing as the
// wire form). Each pixel packs numComponents bytes into one unsigned integer.
JSValue pushSFImage(JSContext *c, const SFImage &img) {
  JSValue obj = JS_NewObject(c);
  if (JS_IsException(obj)) return obj;
  setNum(c, obj, "x", img.width);
  setNum(c, obj, "y", img.height);
  setNum(c, obj, "comp", img.numComponents);
  JSValue arr = JS_NewArray(c);
  const int nc = img.numComponents;
  const std::size_t pixels =
      (nc > 0) ? (img.data.size() / static_cast<std::size_t>(nc)) : 0;
  for (std::size_t p = 0; p < pixels; ++p) {
    unsigned long packed = 0;
    for (int b = 0; b < nc; ++b)
      packed = (packed << 8) |
               static_cast<unsigned long>(img.data[p * nc + b]);
    JS_SetPropertyUint32(c, arr, (uint32_t)p,
                         JS_NewFloat64(c, static_cast<double>(packed)));
  }
  JS_SetPropertyStr(c, obj, "array", arr);  // consumes arr
  return obj;
}
SFImage toSFImage(JSContext *c, JSValueConst obj) {
  SFImage img{0, 0, 0, {}};
  if (!JS_IsObject(obj)) return img;
  img.width = static_cast<int>(getNumberProp(c, obj, "x"));
  img.height = static_cast<int>(getNumberProp(c, obj, "y"));
  img.numComponents = static_cast<int>(getNumberProp(c, obj, "comp"));
  const int nc = img.numComponents;
  const std::size_t pixels = static_cast<std::size_t>(img.width) *
                             static_cast<std::size_t>(img.height);
  JsValue arr(c, JS_GetPropertyStr(c, obj, "array"));
  if (JS_IsObject(arr.get())) {
    uint32_t n = 0;
    {
      JsValue len(c, JS_GetPropertyStr(c, arr.get(), "length"));
      JS_ToUint32(c, &n, len.get());
    }
    for (uint32_t p = 0; p < n && p < pixels; ++p) {
      JsValue cell(c, JS_GetPropertyUint32(c, arr.get(), p));
      double d = 0.0;
      JS_ToFloat64(c, &d, cell.get());
      unsigned long packed = static_cast<unsigned long>(d);
      for (int b = nc - 1; b >= 0; --b)
        img.data.push_back(
            static_cast<unsigned char>((packed >> (8 * b)) & 0xFF));
    }
  }
  return img;
}

// ---------------------------------------------------------------------------
// pushValue: std::any -> JS, by X3DFieldType. Returns an OWNED JSValue (the
// caller wraps it or hands it where QuickJS takes ownership). Mirrors the
// Duktape EcmaScriptBackend::pushValue case-for-case.
// ---------------------------------------------------------------------------
JSValue pushValue(JSContext *ctx, const std::any &v, X3DFieldType type) {
  const bool has = v.has_value();
  switch (type) {
    // --- scalars ---
    case X3DFieldType::SFBool:
      return JS_NewBool(ctx, has && std::any_cast<SFBool>(v));
    case X3DFieldType::SFInt32:
      return JS_NewInt32(ctx, has ? std::any_cast<SFInt32>(v) : 0);
    case X3DFieldType::SFFloat:
      return JS_NewFloat64(ctx,
                           has ? static_cast<double>(std::any_cast<SFFloat>(v))
                               : 0.0);
    case X3DFieldType::SFDouble:
      return JS_NewFloat64(ctx, has ? std::any_cast<SFDouble>(v) : 0.0);
    case X3DFieldType::SFTime:
      return JS_NewFloat64(ctx, has ? std::any_cast<SFTime>(v) : 0.0);
    case X3DFieldType::SFString: {
      // U1 review note (a): keep the std::any temporary alive until
      // JS_NewString has copied the bytes.
      SFString s = has ? std::any_cast<SFString>(v) : SFString{};
      return JS_NewStringLen(ctx, s.c_str(), s.size());
    }
    // --- structured SF ---
    case X3DFieldType::SFVec2f:
      return pushSFVec2f(ctx, has ? std::any_cast<SFVec2f>(v) : SFVec2f{});
    case X3DFieldType::SFVec2d:
      return pushSFVec2d(ctx, has ? std::any_cast<SFVec2d>(v) : SFVec2d{});
    case X3DFieldType::SFVec3f:
      return pushSFVec3f(ctx, has ? std::any_cast<SFVec3f>(v) : SFVec3f{});
    case X3DFieldType::SFVec3d:
      return pushSFVec3d(ctx, has ? std::any_cast<SFVec3d>(v) : SFVec3d{});
    case X3DFieldType::SFVec4f:
      return pushSFVec4f(ctx, has ? std::any_cast<SFVec4f>(v) : SFVec4f{});
    case X3DFieldType::SFVec4d:
      return pushSFVec4d(ctx, has ? std::any_cast<SFVec4d>(v) : SFVec4d{});
    case X3DFieldType::SFColor:
      return pushSFColor(ctx, has ? std::any_cast<SFColor>(v) : SFColor{});
    case X3DFieldType::SFColorRGBA:
      return pushSFColorRGBA(ctx,
                             has ? std::any_cast<SFColorRGBA>(v) : SFColorRGBA{});
    case X3DFieldType::SFRotation:
      return pushSFRotation(ctx,
                            has ? std::any_cast<SFRotation>(v) : SFRotation{});
    case X3DFieldType::SFNode:
      return pushNode(ctx, has ? std::any_cast<SFNode>(v) : SFNode{});
    // --- MF ---
    case X3DFieldType::MFBool: {
      MFBool d = has ? std::any_cast<MFBool>(v) : MFBool{};
      return pushMF(ctx, d, [](JSContext *c, bool b) { return JS_NewBool(c, b); });
    }
    case X3DFieldType::MFInt32: {
      MFInt32 d = has ? std::any_cast<MFInt32>(v) : MFInt32{};
      return pushMF(ctx, d, [](JSContext *c, int x) { return JS_NewInt32(c, x); });
    }
    case X3DFieldType::MFFloat: {
      MFFloat d = has ? std::any_cast<MFFloat>(v) : MFFloat{};
      return pushMF(ctx, d, [](JSContext *c, float x) {
        return JS_NewFloat64(c, static_cast<double>(x));
      });
    }
    case X3DFieldType::MFDouble: {
      MFDouble d = has ? std::any_cast<MFDouble>(v) : MFDouble{};
      return pushMF(ctx, d,
                    [](JSContext *c, double x) { return JS_NewFloat64(c, x); });
    }
    case X3DFieldType::MFTime: {
      MFTime d = has ? std::any_cast<MFTime>(v) : MFTime{};
      return pushMF(ctx, d,
                    [](JSContext *c, double x) { return JS_NewFloat64(c, x); });
    }
    case X3DFieldType::MFString: {
      MFString d = has ? std::any_cast<MFString>(v) : MFString{};
      return pushMF(ctx, d, [](JSContext *c, const std::string &s) {
        return JS_NewStringLen(c, s.c_str(), s.size());
      });
    }
    case X3DFieldType::MFColor: {
      MFColor d = has ? std::any_cast<MFColor>(v) : MFColor{};
      return pushMF(ctx, d, pushSFColor);
    }
    case X3DFieldType::MFColorRGBA: {
      MFColorRGBA d = has ? std::any_cast<MFColorRGBA>(v) : MFColorRGBA{};
      return pushMF(ctx, d, pushSFColorRGBA);
    }
    case X3DFieldType::MFVec2f: {
      MFVec2f d = has ? std::any_cast<MFVec2f>(v) : MFVec2f{};
      return pushMF(ctx, d, pushSFVec2f);
    }
    case X3DFieldType::MFVec2d: {
      MFVec2d d = has ? std::any_cast<MFVec2d>(v) : MFVec2d{};
      return pushMF(ctx, d, pushSFVec2d);
    }
    case X3DFieldType::MFVec3f: {
      MFVec3f d = has ? std::any_cast<MFVec3f>(v) : MFVec3f{};
      return pushMF(ctx, d, pushSFVec3f);
    }
    case X3DFieldType::MFVec3d: {
      MFVec3d d = has ? std::any_cast<MFVec3d>(v) : MFVec3d{};
      return pushMF(ctx, d, pushSFVec3d);
    }
    case X3DFieldType::MFVec4f: {
      MFVec4f d = has ? std::any_cast<MFVec4f>(v) : MFVec4f{};
      return pushMF(ctx, d, pushSFVec4f);
    }
    case X3DFieldType::MFVec4d: {
      MFVec4d d = has ? std::any_cast<MFVec4d>(v) : MFVec4d{};
      return pushMF(ctx, d, pushSFVec4d);
    }
    case X3DFieldType::MFRotation: {
      MFRotation d = has ? std::any_cast<MFRotation>(v) : MFRotation{};
      return pushMF(ctx, d, pushSFRotation);
    }
    case X3DFieldType::MFNode: {
      MFNode d = has ? std::any_cast<MFNode>(v) : MFNode{};
      return pushMF(ctx, d, pushNode);
    }
    // --- matrices: flat row-major arrays ---
    case X3DFieldType::SFMatrix3f:
      return pushSFMatrix<SFMatrix3f, 3>(
          ctx, has ? std::any_cast<SFMatrix3f>(v) : SFMatrix3f{});
    case X3DFieldType::SFMatrix4f:
      return pushSFMatrix<SFMatrix4f, 4>(
          ctx, has ? std::any_cast<SFMatrix4f>(v) : SFMatrix4f{});
    case X3DFieldType::SFMatrix3d:
      return pushSFMatrix<SFMatrix3d, 3>(
          ctx, has ? std::any_cast<SFMatrix3d>(v) : SFMatrix3d{});
    case X3DFieldType::SFMatrix4d:
      return pushSFMatrix<SFMatrix4d, 4>(
          ctx, has ? std::any_cast<SFMatrix4d>(v) : SFMatrix4d{});
    // --- images ---
    case X3DFieldType::SFImage:
      return pushSFImage(ctx, has ? std::any_cast<SFImage>(v) : SFImage{});
    case X3DFieldType::MFImage: {
      MFImage d = has ? std::any_cast<MFImage>(v) : MFImage{};
      return pushMF(ctx, d, pushSFImage);
    }
    // --- MF matrices: arrays of flat row-major matrices ---
    case X3DFieldType::MFMatrix3f: {
      MFMatrix3f d = has ? std::any_cast<MFMatrix3f>(v) : MFMatrix3f{};
      return pushMF(ctx, d, [](JSContext *c, const SFMatrix3f &m) {
        return pushSFMatrix<SFMatrix3f, 3>(c, m);
      });
    }
    case X3DFieldType::MFMatrix4f: {
      MFMatrix4f d = has ? std::any_cast<MFMatrix4f>(v) : MFMatrix4f{};
      return pushMF(ctx, d, [](JSContext *c, const SFMatrix4f &m) {
        return pushSFMatrix<SFMatrix4f, 4>(c, m);
      });
    }
    case X3DFieldType::MFMatrix3d: {
      MFMatrix3d d = has ? std::any_cast<MFMatrix3d>(v) : MFMatrix3d{};
      return pushMF(ctx, d, [](JSContext *c, const SFMatrix3d &m) {
        return pushSFMatrix<SFMatrix3d, 3>(c, m);
      });
    }
    case X3DFieldType::MFMatrix4d: {
      MFMatrix4d d = has ? std::any_cast<MFMatrix4d>(v) : MFMatrix4d{};
      return pushMF(ctx, d, [](JSContext *c, const SFMatrix4d &m) {
        return pushSFMatrix<SFMatrix4d, 4>(c, m);
      });
    }
    default:
      // Unsupported/exotic type (enums): JS undefined (mirrors Duktape).
      return JS_UNDEFINED;
  }
}

// ---------------------------------------------------------------------------
// toValue: JS -> std::any, by X3DFieldType. Mirrors the Duktape
// EcmaScriptBackend::toValue case-for-case.
// ---------------------------------------------------------------------------
std::any toValue(JSContext *ctx, JSValueConst i, X3DFieldType type) {
  switch (type) {
    case X3DFieldType::SFBool:
      return SFBool(toBool(ctx, i));
    case X3DFieldType::SFInt32:
      return SFInt32(toInt32(ctx, i));
    case X3DFieldType::SFFloat:
      return SFFloat((float)toNumber(ctx, i));
    case X3DFieldType::SFDouble:
      return SFDouble(toNumber(ctx, i));
    case X3DFieldType::SFTime:
      return SFTime(toNumber(ctx, i));
    case X3DFieldType::SFString:
      return toStdString(ctx, i);
    case X3DFieldType::SFVec2f: return toSFVec2f(ctx, i);
    case X3DFieldType::SFVec2d: return toSFVec2d(ctx, i);
    case X3DFieldType::SFVec3f: return toSFVec3f(ctx, i);
    case X3DFieldType::SFVec3d: return toSFVec3d(ctx, i);
    case X3DFieldType::SFVec4f: return toSFVec4f(ctx, i);
    case X3DFieldType::SFVec4d: return toSFVec4d(ctx, i);
    case X3DFieldType::SFColor: return toSFColor(ctx, i);
    case X3DFieldType::SFColorRGBA: return toSFColorRGBA(ctx, i);
    case X3DFieldType::SFRotation: return toSFRotation(ctx, i);
    case X3DFieldType::SFNode:
      return SFNode(extractNode(ctx, i), [](X3DNode *) {});  // non-owning alias
    case X3DFieldType::MFBool:
      return toMF<MFBool>(ctx, i, [](JSContext *c, JSValueConst j) {
        return SFBool(toBool(c, j));
      });
    case X3DFieldType::MFInt32:
      return toMF<MFInt32>(ctx, i, [](JSContext *c, JSValueConst j) {
        return SFInt32(toInt32(c, j));
      });
    case X3DFieldType::MFFloat:
      return toMF<MFFloat>(ctx, i, [](JSContext *c, JSValueConst j) {
        return SFFloat((float)toNumber(c, j));
      });
    case X3DFieldType::MFDouble:
      return toMF<MFDouble>(ctx, i, [](JSContext *c, JSValueConst j) {
        return SFDouble(toNumber(c, j));
      });
    case X3DFieldType::MFTime:
      return toMF<MFTime>(ctx, i, [](JSContext *c, JSValueConst j) {
        return SFTime(toNumber(c, j));
      });
    case X3DFieldType::MFString:
      return toMF<MFString>(ctx, i, [](JSContext *c, JSValueConst j) {
        return toStdString(c, j);
      });
    case X3DFieldType::MFColor:
      return toMF<MFColor>(ctx, i, toSFColor);
    case X3DFieldType::MFColorRGBA:
      return toMF<MFColorRGBA>(ctx, i, toSFColorRGBA);
    case X3DFieldType::MFVec2f:
      return toMF<MFVec2f>(ctx, i, toSFVec2f);
    case X3DFieldType::MFVec2d:
      return toMF<MFVec2d>(ctx, i, toSFVec2d);
    case X3DFieldType::MFVec3f:
      return toMF<MFVec3f>(ctx, i, toSFVec3f);
    case X3DFieldType::MFVec3d:
      return toMF<MFVec3d>(ctx, i, toSFVec3d);
    case X3DFieldType::MFVec4f:
      return toMF<MFVec4f>(ctx, i, toSFVec4f);
    case X3DFieldType::MFVec4d:
      return toMF<MFVec4d>(ctx, i, toSFVec4d);
    case X3DFieldType::MFRotation:
      return toMF<MFRotation>(ctx, i, toSFRotation);
    case X3DFieldType::MFNode:
      return toMF<MFNode>(ctx, i, [](JSContext *c, JSValueConst j) {
        return SFNode(extractNode(c, j), [](X3DNode *) {});
      });
    case X3DFieldType::SFMatrix3f:
      return toSFMatrix<SFMatrix3f, 3, float>(ctx, i);
    case X3DFieldType::SFMatrix4f:
      return toSFMatrix<SFMatrix4f, 4, float>(ctx, i);
    case X3DFieldType::SFMatrix3d:
      return toSFMatrix<SFMatrix3d, 3, double>(ctx, i);
    case X3DFieldType::SFMatrix4d:
      return toSFMatrix<SFMatrix4d, 4, double>(ctx, i);
    case X3DFieldType::SFImage:
      return toSFImage(ctx, i);
    case X3DFieldType::MFImage:
      return toMF<MFImage>(ctx, i, toSFImage);
    case X3DFieldType::MFMatrix3f:
      return toMF<MFMatrix3f>(ctx, i, [](JSContext *c, JSValueConst j) {
        return toSFMatrix<SFMatrix3f, 3, float>(c, j);
      });
    case X3DFieldType::MFMatrix4f:
      return toMF<MFMatrix4f>(ctx, i, [](JSContext *c, JSValueConst j) {
        return toSFMatrix<SFMatrix4f, 4, float>(c, j);
      });
    case X3DFieldType::MFMatrix3d:
      return toMF<MFMatrix3d>(ctx, i, [](JSContext *c, JSValueConst j) {
        return toSFMatrix<SFMatrix3d, 3, double>(c, j);
      });
    case X3DFieldType::MFMatrix4d:
      return toMF<MFMatrix4d>(ctx, i, [](JSContext *c, JSValueConst j) {
        return toSFMatrix<SFMatrix4d, 4, double>(c, j);
      });
    default:
      return {};
  }
}

} // namespace

// ===========================================================================
// Impl — all QuickJS state lives here (keeps the header QuickJS-free).
// ===========================================================================

struct QuickJsBackend::Impl {
  // One loaded script's per-context state (the Duktape Entry model).
  struct Entry {
    JSContext *ctx = nullptr;   // owned; freed in shutdown()/destructor
    X3DNode *node = nullptr;    // owning Script node (not owned here)
    SaiContext *sai = nullptr;  // SAI surface (not owned here)
  };

  JSRuntime *rt = nullptr;
  ScriptHandle nextHandle = 1;  // start at 1 so 0 stays kInvalidScriptHandle
  std::unordered_map<ScriptHandle, Entry> entries;

  Impl() { rt = JS_NewRuntime(); }

  ~Impl() {
    for (auto &[handle, e] : entries) {
      if (e.ctx) JS_FreeContext(e.ctx);
    }
    entries.clear();
    if (rt) JS_FreeRuntime(rt);
  }

  Entry *entryFor(ScriptHandle handle) {
    auto it = entries.find(handle);
    return (it != entries.end()) ? &it->second : nullptr;
  }

  // Look up a global by name; returns an OWNED JsValue (UNDEFINED if absent).
  JsValue getGlobal(JSContext *ctx, const char *name) {
    JsValue global(ctx, JS_GetGlobalObject(ctx));
    return JsValue(ctx, JS_GetPropertyStr(ctx, global.get(), name));
  }

  // Set a global by name to `value` (CONSUMES the value ref).
  void setGlobal(JSContext *ctx, const char *name, JSValue value) {
    JsValue global(ctx, JS_GetGlobalObject(ctx));
    JS_SetPropertyStr(ctx, global.get(), name, value);  // consumes value
  }

  // Call a global function `name` with `argc` already-built args. The args are
  // BORROWED (the caller still owns/frees them). No-op if the global is missing
  // or not callable. Logs+clears any exception.
  void callGlobal(JSContext *ctx, const char *name, int argc, JSValueConst *argv) {
    JsValue fn = getGlobal(ctx, name);
    if (!JS_IsFunction(ctx, fn.get())) return;
    JsValue ret(ctx, JS_Call(ctx, fn.get(), JS_UNDEFINED, argc, argv));
    if (ret.isException()) logException(ctx, name);
  }

  void callGlobalNoArgs(JSContext *ctx, const char *name) {
    callGlobal(ctx, name, 0, nullptr);
  }

  // -------------------------------------------------------------------------
  // Author-field seeding / readback (mirrors EcmaScriptBackend §3.5).
  // -------------------------------------------------------------------------

  // boxed initialValue -> JS global, per readable author field
  // (initializeOnly/inputOutput). Lets the script read its authored defaults.
  void seedAuthorGlobals(Entry &e) {
    if (!e.node || !e.ctx) return;
    JSContext *ctx = e.ctx;
    for (const FieldInfo &info : dynamicFieldStore().authorFields(*e.node)) {
      if (!info.isReadable() || !info.get) continue;
      std::any v = info.get(*e.node);
      if (!v.has_value()) continue;
      setGlobal(ctx, info.x3dName.c_str(), pushValue(ctx, v, info.type));
    }
  }

  // Canonical JSON of a JS value (for no-op-write suppression), via the
  // context's own JSON.stringify. Returns "" on failure (treated as "changed").
  std::string jsonOf(JSContext *ctx, JSValueConst v) {
    JsValue str(ctx, JS_JSONStringify(ctx, v, JS_UNDEFINED, JS_UNDEFINED));
    if (str.isException() || str.isUndefined()) return {};
    return toStdString(ctx, str.get());
  }

  // JS global -> store + cascade event, per readable author field
  // (outputOnly/inputOutput) the script wrote. Suppresses no-op re-emits.
  void readbackAuthorGlobals(Entry &e, double timestamp) {
    if (!e.node || !e.ctx || !e.sai) return;
    JSContext *ctx = e.ctx;
    for (const FieldInfo &info : dynamicFieldStore().authorFields(*e.node)) {
      if (!info.isReadable()) continue;
      JsValue g = getGlobal(ctx, info.x3dName.c_str());
      if (g.isUndefined()) continue;  // not defined / never assigned
      // Suppress no-op re-emit: skip if the JS value equals the stored value.
      std::any prev = dynamicFieldStore().getValue(*e.node, info.x3dName);
      if (prev.has_value()) {
        JsValue prevJs(ctx, pushValue(ctx, prev, info.type));
        if (jsonOf(ctx, g.get()) == jsonOf(ctx, prevJs.get())) continue;
      }
      std::any value = toValue(ctx, g.get(), info.type);
      if (!value.has_value()) continue;
      dynamicFieldStore().setValue(*e.node, info.x3dName, value);
      (void)timestamp;
      e.sai->setField(e.node, info.x3dName, value);
    }
  }
};

// ===========================================================================
// Browser global — native callbacks bound to the script's SaiContext.
//
// One JSContext backs exactly one loaded script, so we thread the SaiContext to
// the C callbacks via JS_SetContextOpaque(ctx, sai) / JS_GetContextOpaque(ctx)
// (the QuickJS analogue of Duktape's per-function \xff sai stamp, but simpler
// because the context is 1:1 with the script).
// ===========================================================================

namespace {

SaiContext *saiOf(JSContext *ctx) {
  return static_cast<SaiContext *>(JS_GetContextOpaque(ctx));
}

JSValue browser_print(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
  SaiContext *sai = saiOf(ctx);
  if (sai && argc > 0) {
    const char *s = JS_ToCString(ctx, argv[0]);
    if (s) {
      sai->print(s);
      JS_FreeCString(ctx, s);
    }
  }
  return JS_UNDEFINED;
}

JSValue browser_getName(JSContext *ctx, JSValueConst, int, JSValueConst *) {
  SaiContext *sai = saiOf(ctx);
  return JS_NewString(ctx, sai ? sai->getName().c_str() : "");
}

JSValue browser_getVersion(JSContext *ctx, JSValueConst, int, JSValueConst *) {
  SaiContext *sai = saiOf(ctx);
  return JS_NewString(ctx, sai ? sai->getVersion().c_str() : "");
}

JSValue browser_getCurrentTime(JSContext *ctx, JSValueConst, int,
                               JSValueConst *) {
  SaiContext *sai = saiOf(ctx);
  return JS_NewFloat64(ctx, sai ? sai->currentTime() : 0.0);
}

JSValue browser_getCurrentFrameRate(JSContext *ctx, JSValueConst, int,
                                    JSValueConst *) {
  SaiContext *sai = saiOf(ctx);
  return JS_NewFloat64(ctx, sai ? sai->currentFrameRate() : 0.0);
}

// addRoute(fromNode, fromField, toNode, toField)
JSValue browser_addRoute(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
  SaiContext *sai = saiOf(ctx);
  if (!sai || argc < 4) return JS_UNDEFINED;
  X3DNode *from = extractNode(ctx, argv[0]);
  X3DNode *to = extractNode(ctx, argv[2]);
  const char *fromField = JS_ToCString(ctx, argv[1]);
  const char *toField = JS_ToCString(ctx, argv[3]);
  JSValue result = JS_UNDEFINED;
  try {
    sai->addRoute(from, fromField ? fromField : "", to, toField ? toField : "");
  } catch (const std::exception &e) {
    result = JS_ThrowTypeError(ctx, "addRoute: %s", e.what());
  }
  if (fromField) JS_FreeCString(ctx, fromField);
  if (toField) JS_FreeCString(ctx, toField);
  return result;
}

JSValue browser_deleteRoute(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
  SaiContext *sai = saiOf(ctx);
  if (!sai || argc < 4) return JS_UNDEFINED;
  X3DNode *from = extractNode(ctx, argv[0]);
  X3DNode *to = extractNode(ctx, argv[2]);
  const char *fromField = JS_ToCString(ctx, argv[1]);
  const char *toField = JS_ToCString(ctx, argv[3]);
  JSValue result = JS_UNDEFINED;
  try {
    sai->deleteRoute(from, fromField ? fromField : "", to,
                     toField ? toField : "");
  } catch (const std::exception &e) {
    result = JS_ThrowTypeError(ctx, "deleteRoute: %s", e.what());
  }
  if (fromField) JS_FreeCString(ctx, fromField);
  if (toField) JS_FreeCString(ctx, toField);
  return result;
}

// Define one Browser method. JS_SetPropertyStr consumes the function ref.
void defineBrowserMethod(JSContext *ctx, JSValue browser, const char *name,
                         JSCFunction *fn, int nargs) {
  JS_SetPropertyStr(ctx, browser, name, JS_NewCFunction(ctx, fn, name, nargs));
}

// Define a Browser accessor (getter) property backed by a C function, so it
// reads like a bare property (Browser.currentTime) exactly as Duktape exposes.
void defineBrowserGetter(JSContext *ctx, JSValue browser, const char *name,
                         JSCFunction *fn) {
  JSAtom atom = JS_NewAtom(ctx, name);
  JSValue getter = JS_NewCFunction2(ctx, fn, name, 0, JS_CFUNC_getter, 0);
  JS_DefinePropertyGetSet(ctx, browser, atom, getter, JS_UNDEFINED,
                          JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);
}

// Install the Browser global on a fresh context. The SaiContext is already
// stamped on the context opaque by load() before this runs.
void installBrowser(JSContext *ctx) {
  JsValue global(ctx, JS_GetGlobalObject(ctx));
  JSValue browser = JS_NewObject(ctx);

  defineBrowserGetter(ctx, browser, "currentTime", browser_getCurrentTime);
  defineBrowserGetter(ctx, browser, "currentFrameRate",
                      browser_getCurrentFrameRate);

  defineBrowserMethod(ctx, browser, "getName", browser_getName, 0);
  defineBrowserMethod(ctx, browser, "getVersion", browser_getVersion, 0);
  defineBrowserMethod(ctx, browser, "print", browser_print, 1);
  defineBrowserMethod(ctx, browser, "addRoute", browser_addRoute, 4);
  defineBrowserMethod(ctx, browser, "deleteRoute", browser_deleteRoute, 4);

  JS_SetPropertyStr(ctx, global.get(), "Browser", browser);  // consumes browser
}

} // namespace

// ===========================================================================
// Construction / destruction.
// ===========================================================================

QuickJsBackend::QuickJsBackend() : impl_(std::make_unique<Impl>()) {}
QuickJsBackend::~QuickJsBackend() = default;

// ===========================================================================
// load(): create a JSContext, install Browser, eval the source, mint a handle.
// ===========================================================================

ScriptHandle QuickJsBackend::load(X3DNode &scriptNode,
                                  const std::string &source, SaiContext &sai) {
  if (!impl_->rt) return kInvalidScriptHandle;
  JSContext *ctx = JS_NewContext(impl_->rt);
  if (!ctx) return kInvalidScriptHandle;

  // Thread the SaiContext to Browser callbacks (1 context : 1 script).
  JS_SetContextOpaque(ctx, &sai);
  installBrowser(ctx);

  // Evaluate the source to define global functions (initialize/handlers/...).
  JsValue result(ctx, JS_Eval(ctx, source.c_str(), source.size(),
                              "<script>", JS_EVAL_TYPE_GLOBAL));
  if (result.isException()) {
    logException(ctx, "eval");
    JS_FreeContext(ctx);
    return kInvalidScriptHandle;
  }

  ScriptHandle handle = impl_->nextHandle++;
  impl_->entries[handle] = Impl::Entry{ctx, &scriptNode, &sai};
  return handle;
}

// ===========================================================================
// initialize(): seed author globals, call initialize(), read author outputs.
// ===========================================================================

void QuickJsBackend::initialize(ScriptHandle handle) {
  Impl::Entry *e = impl_->entryFor(handle);
  if (!e) return;
  // Seed initializeOnly/inputOutput author defaults BEFORE initialize() runs.
  impl_->seedAuthorGlobals(*e);
  impl_->callGlobalNoArgs(e->ctx, "initialize");
  // initialize() may write outputOnly/inputOutput author fields (§29.2.3) —
  // read them back (timestamp 0 = scene start).
  impl_->readbackAuthorGlobals(*e, 0.0);
}

// ===========================================================================
// shutdown(): call shutdown() if defined, then free the context.
// ===========================================================================

void QuickJsBackend::shutdown(ScriptHandle handle) {
  Impl::Entry *e = impl_->entryFor(handle);
  if (!e) return;
  impl_->callGlobalNoArgs(e->ctx, "shutdown");
  JS_FreeContext(e->ctx);
  impl_->entries.erase(handle);
}

// ===========================================================================
// prepareEvents(): call prepareEvents(now), then read author outputs.
// ===========================================================================

void QuickJsBackend::prepareEvents(ScriptHandle handle, double now) {
  Impl::Entry *e = impl_->entryFor(handle);
  if (!e) return;
  JsValue arg(e->ctx, JS_NewFloat64(e->ctx, now));
  JSValueConst argv[1] = {arg.get()};
  impl_->callGlobal(e->ctx, "prepareEvents", 1, argv);
  // §29.2.5: prepareEvents may generate events — read author outputs back.
  impl_->readbackAuthorGlobals(*e, now);
}

// ===========================================================================
// invoke(): dispatch one inputOnly event — call handler(value, timestamp).
// ===========================================================================

void QuickJsBackend::invoke(ScriptHandle handle, const std::string &eventName,
                            const std::any &value, X3DFieldType type,
                            double timestamp) {
  Impl::Entry *e = impl_->entryFor(handle);
  if (!e) return;
  JSContext *ctx = e->ctx;
  JsValue arg0(ctx, pushValue(ctx, value, type));   // arg 0: the field value
  JsValue arg1(ctx, JS_NewFloat64(ctx, timestamp)); // arg 1: the timestamp
  JSValueConst argv[2] = {arg0.get(), arg1.get()};
  impl_->callGlobal(ctx, eventName.c_str(), 2, argv);
  // §3.5: read any author outputOnly/inputOutput field the handler wrote and
  // emit it into the cascade carrying the triggering timestamp.
  impl_->readbackAuthorGlobals(*e, timestamp);
}

// ===========================================================================
// eventsProcessed(): call eventsProcessed(), then read author outputs.
// ===========================================================================

void QuickJsBackend::eventsProcessed(ScriptHandle handle, double timestamp) {
  Impl::Entry *e = impl_->entryFor(handle);
  if (!e) return;
  impl_->callGlobalNoArgs(e->ctx, "eventsProcessed");
  // §29.2.4: events from eventsProcessed() enter the cascade with the timestamp
  // of the last event processed — read author outputs back.
  impl_->readbackAuthorGlobals(*e, timestamp);
}

} // namespace x3d::runtime
