// EcmaScriptBackend.cpp
// ScriptEngine implementation backed by Duktape 2.7.0 (U4: full marshalling +
// Browser object + handler dispatch). See EcmaScriptBackend.hpp and the design
// spec docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md §4.

#include "EcmaScriptBackend.hpp"

#include "DynamicField.hpp" // author-field store (S1 un-tabling, design §3.5)
#include "SaiContext.hpp"   // post author outputs into the cascade
#include "x3d/nodes/X3DNode.hpp"      // SFNode wrapping (X3DNode*)
#include "x3d/core/X3Dtypes.hpp"     // SF*/MF* concrete C++ types

#include <iostream>  // diagnostics
#include <stdexcept>

using namespace x3d::core;

namespace x3d::runtime {

namespace {

// Hidden Duktape property key (\xff prefix → non-enumerable internal key).
constexpr const char *kNodePtrKey = "\xff" "x3dNode";   // SFNode handle: X3DNode*

// -------------------------------------------------------------------------
// SFNode <-> JS opaque handle object.
// A non-null node becomes a JS object carrying the X3DNode* in a hidden
// pointer property; a null node becomes JS null. extractNode reverses it.
// -------------------------------------------------------------------------

void pushNode(duk_context *ctx, const SFNode &node) {
  if (!node) {
    duk_push_null(ctx);
    return;
  }
  duk_push_object(ctx);
  duk_push_pointer(ctx, node.get());
  duk_put_prop_string(ctx, -2, kNodePtrKey);
}

// Recover the X3DNode* from a JS node-handle object (or null) at index idx.
X3DNode *extractNode(duk_context *ctx, duk_idx_t idx) {
  if (duk_is_null_or_undefined(ctx, idx)) return nullptr;
  if (!duk_is_object(ctx, idx)) return nullptr;
  if (!duk_get_prop_string(ctx, idx, kNodePtrKey)) {
    duk_pop(ctx);
    return nullptr;
  }
  void *p = duk_get_pointer(ctx, -1);
  duk_pop(ctx);
  return static_cast<X3DNode *>(p);
}

// -------------------------------------------------------------------------
// Structured-SF helpers (named numeric properties).
// -------------------------------------------------------------------------

void putNumberProp(duk_context *ctx, const char *name, double v) {
  duk_push_number(ctx, v);
  duk_put_prop_string(ctx, -2, name);
}

double getNumberProp(duk_context *ctx, duk_idx_t objIdx, const char *name) {
  duk_get_prop_string(ctx, objIdx, name);
  double v = duk_is_number(ctx, -1) ? duk_get_number(ctx, -1) : 0.0;
  duk_pop(ctx);
  return v;
}

// -------------------------------------------------------------------------
// Single-SF pushers (used directly and as MF element pushers).
// -------------------------------------------------------------------------

void pushSFVec2f(duk_context *c, const SFVec2f &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
}
void pushSFVec2d(duk_context *c, const SFVec2d &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
}
void pushSFVec3f(duk_context *c, const SFVec3f &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
  putNumberProp(c, "z", v.z);
}
void pushSFVec3d(duk_context *c, const SFVec3d &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
  putNumberProp(c, "z", v.z);
}
void pushSFVec4f(duk_context *c, const SFVec4f &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
  putNumberProp(c, "z", v.z); putNumberProp(c, "w", v.w);
}
void pushSFVec4d(duk_context *c, const SFVec4d &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
  putNumberProp(c, "z", v.z); putNumberProp(c, "w", v.w);
}
void pushSFColor(duk_context *c, const SFColor &v) {
  duk_push_object(c); putNumberProp(c, "r", v.r); putNumberProp(c, "g", v.g);
  putNumberProp(c, "b", v.b);
}
void pushSFColorRGBA(duk_context *c, const SFColorRGBA &v) {
  duk_push_object(c); putNumberProp(c, "r", v.r); putNumberProp(c, "g", v.g);
  putNumberProp(c, "b", v.b); putNumberProp(c, "a", v.a);
}
void pushSFRotation(duk_context *c, const SFRotation &v) {
  duk_push_object(c); putNumberProp(c, "x", v.x); putNumberProp(c, "y", v.y);
  putNumberProp(c, "z", v.z); putNumberProp(c, "angle", v.angle);
}

// -------------------------------------------------------------------------
// Single-SF poppers.
// -------------------------------------------------------------------------

SFVec2f toSFVec2f(duk_context *c, duk_idx_t i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y")};
}
SFVec2d toSFVec2d(duk_context *c, duk_idx_t i) {
  return {getNumberProp(c, i, "x"), getNumberProp(c, i, "y")};
}
SFVec3f toSFVec3f(duk_context *c, duk_idx_t i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y"),
          (float)getNumberProp(c, i, "z")};
}
SFVec3d toSFVec3d(duk_context *c, duk_idx_t i) {
  return {getNumberProp(c, i, "x"), getNumberProp(c, i, "y"),
          getNumberProp(c, i, "z")};
}
SFVec4f toSFVec4f(duk_context *c, duk_idx_t i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y"),
          (float)getNumberProp(c, i, "z"), (float)getNumberProp(c, i, "w")};
}
SFVec4d toSFVec4d(duk_context *c, duk_idx_t i) {
  return {getNumberProp(c, i, "x"), getNumberProp(c, i, "y"),
          getNumberProp(c, i, "z"), getNumberProp(c, i, "w")};
}
SFColor toSFColor(duk_context *c, duk_idx_t i) {
  return {(float)getNumberProp(c, i, "r"), (float)getNumberProp(c, i, "g"),
          (float)getNumberProp(c, i, "b")};
}
SFColorRGBA toSFColorRGBA(duk_context *c, duk_idx_t i) {
  return {(float)getNumberProp(c, i, "r"), (float)getNumberProp(c, i, "g"),
          (float)getNumberProp(c, i, "b"), (float)getNumberProp(c, i, "a")};
}
SFRotation toSFRotation(duk_context *c, duk_idx_t i) {
  return {(float)getNumberProp(c, i, "x"), (float)getNumberProp(c, i, "y"),
          (float)getNumberProp(c, i, "z"), (float)getNumberProp(c, i, "angle")};
}

// -------------------------------------------------------------------------
// Generic MF push/pop via element functors.
// -------------------------------------------------------------------------

template <typename Vec, typename PushElem>
void pushMF(duk_context *c, const Vec &v, PushElem pushElem) {
  duk_push_array(c);
  for (std::size_t i = 0; i < v.size(); ++i) {
    pushElem(c, v[i]);
    duk_put_prop_index(c, -2, (duk_uarridx_t)i);
  }
}

template <typename Vec, typename ToElem>
Vec toMF(duk_context *c, duk_idx_t arrIdx, ToElem toElem) {
  Vec out;
  // Normalize to absolute index so element access stays valid as we push.
  duk_idx_t a = duk_normalize_index(c, arrIdx);
  if (!duk_is_object(c, a)) return out;
  duk_get_prop_string(c, a, "length");
  duk_uarridx_t n = duk_is_number(c, -1) ? (duk_uarridx_t)duk_get_uint(c, -1) : 0;
  duk_pop(c);
  out.reserve(n);
  for (duk_uarridx_t i = 0; i < n; ++i) {
    duk_get_prop_index(c, a, i);
    out.push_back(toElem(c, -1));
    duk_pop(c);
  }
  return out;
}

// SFMatrix <-> JS: a flat row-major array of N*N numbers (19777-1 ECMAScript).
template <typename M, int N> void pushSFMatrix(duk_context *c, const M &m) {
  duk_push_array(c);
  duk_uarridx_t k = 0;
  for (int r = 0; r < N; ++r)
    for (int col = 0; col < N; ++col) {
      duk_push_number(c, static_cast<double>(m.matrix[r][col]));
      duk_put_prop_index(c, -2, k++);
    }
}
template <typename M, int N, typename T>
M toSFMatrix(duk_context *c, duk_idx_t idx) {
  M m{};
  duk_idx_t a = duk_normalize_index(c, idx);
  if (!duk_is_object(c, a)) return m;
  duk_uarridx_t k = 0;
  for (int r = 0; r < N; ++r)
    for (int col = 0; col < N; ++col) {
      duk_get_prop_index(c, a, k++);
      m.matrix[r][col] = static_cast<T>(duk_to_number(c, -1));
      duk_pop(c);
    }
  return m;
}

// SFImage <-> JS (ISO 19777-1 ECMAScript binding): an object with x, y, comp,
// and array (MFInt32 of packed pixels, high-byte-first — same packing as the
// wire form fmtImage/parseImage in FieldValueIO). `data` holds the raw
// width*height*numComponents bytes; each pixel packs numComponents bytes into
// one unsigned integer.
void pushSFImage(duk_context *c, const SFImage &img) {
  duk_push_object(c);
  putNumberProp(c, "x", img.width);
  putNumberProp(c, "y", img.height);
  putNumberProp(c, "comp", img.numComponents);
  duk_push_array(c);
  const int nc = img.numComponents;
  const std::size_t pixels =
      (nc > 0) ? (img.data.size() / static_cast<std::size_t>(nc)) : 0;
  for (std::size_t p = 0; p < pixels; ++p) {
    unsigned long packed = 0;
    for (int b = 0; b < nc; ++b) {
      packed = (packed << 8) |
               static_cast<unsigned long>(img.data[p * nc + b]);
    }
    duk_push_number(c, static_cast<double>(packed));
    duk_put_prop_index(c, -2, (duk_uarridx_t)p);
  }
  duk_put_prop_string(c, -2, "array");
}
SFImage toSFImage(duk_context *c, duk_idx_t idx) {
  SFImage img{0, 0, 0, {}};
  duk_idx_t a = duk_normalize_index(c, idx);
  if (!duk_is_object(c, a)) return img;
  img.width = static_cast<int>(getNumberProp(c, a, "x"));
  img.height = static_cast<int>(getNumberProp(c, a, "y"));
  img.numComponents = static_cast<int>(getNumberProp(c, a, "comp"));
  const int nc = img.numComponents;
  const std::size_t pixels = static_cast<std::size_t>(img.width) *
                             static_cast<std::size_t>(img.height);
  duk_get_prop_string(c, a, "array");
  duk_idx_t arr = duk_normalize_index(c, -1);
  if (duk_is_object(c, arr)) {
    duk_get_prop_string(c, arr, "length");
    duk_uarridx_t n =
        duk_is_number(c, -1) ? (duk_uarridx_t)duk_get_uint(c, -1) : 0;
    duk_pop(c);
    for (duk_uarridx_t p = 0; p < n && p < pixels; ++p) {
      duk_get_prop_index(c, arr, p);
      unsigned long packed = static_cast<unsigned long>(duk_get_uint(c, -1));
      duk_pop(c);
      for (int b = nc - 1; b >= 0; --b) {
        img.data.push_back(
            static_cast<unsigned char>((packed >> (8 * b)) & 0xFF));
      }
    }
  }
  duk_pop(c);  // array
  return img;
}

} // namespace

// ---------------------------------------------------------------------------
// pushValue: std::any -> JS, by X3DFieldType.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::pushValue(duk_context *ctx, const std::any &v,
                                  X3DFieldType type) {
  // Helper: cast-or-default for scalar SF types when the any is empty.
  auto has = v.has_value();
  switch (type) {
    // --- scalars ---
    case X3DFieldType::SFBool:
      duk_push_boolean(ctx, has && std::any_cast<SFBool>(v)); return;
    case X3DFieldType::SFInt32:
      duk_push_int(ctx, has ? std::any_cast<SFInt32>(v) : 0); return;
    case X3DFieldType::SFFloat:
      duk_push_number(ctx, has ? std::any_cast<SFFloat>(v) : 0.0f); return;
    case X3DFieldType::SFDouble:
      duk_push_number(ctx, has ? std::any_cast<SFDouble>(v) : 0.0); return;
    case X3DFieldType::SFTime:
      duk_push_number(ctx, has ? std::any_cast<SFTime>(v) : 0.0); return;
    case X3DFieldType::SFString:
      duk_push_string(ctx, has ? std::any_cast<SFString>(v).c_str() : ""); return;
    // --- structured SF ---
    case X3DFieldType::SFVec2f:
      pushSFVec2f(ctx, has ? std::any_cast<SFVec2f>(v) : SFVec2f{}); return;
    case X3DFieldType::SFVec2d:
      pushSFVec2d(ctx, has ? std::any_cast<SFVec2d>(v) : SFVec2d{}); return;
    case X3DFieldType::SFVec3f:
      pushSFVec3f(ctx, has ? std::any_cast<SFVec3f>(v) : SFVec3f{}); return;
    case X3DFieldType::SFVec3d:
      pushSFVec3d(ctx, has ? std::any_cast<SFVec3d>(v) : SFVec3d{}); return;
    case X3DFieldType::SFVec4f:
      pushSFVec4f(ctx, has ? std::any_cast<SFVec4f>(v) : SFVec4f{}); return;
    case X3DFieldType::SFVec4d:
      pushSFVec4d(ctx, has ? std::any_cast<SFVec4d>(v) : SFVec4d{}); return;
    case X3DFieldType::SFColor:
      pushSFColor(ctx, has ? std::any_cast<SFColor>(v) : SFColor{}); return;
    case X3DFieldType::SFColorRGBA:
      pushSFColorRGBA(ctx, has ? std::any_cast<SFColorRGBA>(v) : SFColorRGBA{});
      return;
    case X3DFieldType::SFRotation:
      pushSFRotation(ctx, has ? std::any_cast<SFRotation>(v) : SFRotation{});
      return;
    case X3DFieldType::SFNode:
      pushNode(ctx, has ? std::any_cast<SFNode>(v) : SFNode{}); return;
    // --- MF ---
    case X3DFieldType::MFBool: {
      MFBool d = has ? std::any_cast<MFBool>(v) : MFBool{};
      pushMF(ctx, d, [](duk_context *c, bool b) { duk_push_boolean(c, b); });
      return;
    }
    case X3DFieldType::MFInt32: {
      MFInt32 d = has ? std::any_cast<MFInt32>(v) : MFInt32{};
      pushMF(ctx, d, [](duk_context *c, int x) { duk_push_int(c, x); });
      return;
    }
    case X3DFieldType::MFFloat: {
      MFFloat d = has ? std::any_cast<MFFloat>(v) : MFFloat{};
      pushMF(ctx, d, [](duk_context *c, float x) { duk_push_number(c, x); });
      return;
    }
    case X3DFieldType::MFDouble: {
      MFDouble d = has ? std::any_cast<MFDouble>(v) : MFDouble{};
      pushMF(ctx, d, [](duk_context *c, double x) { duk_push_number(c, x); });
      return;
    }
    case X3DFieldType::MFTime: {
      MFTime d = has ? std::any_cast<MFTime>(v) : MFTime{};
      pushMF(ctx, d, [](duk_context *c, double x) { duk_push_number(c, x); });
      return;
    }
    case X3DFieldType::MFString: {
      MFString d = has ? std::any_cast<MFString>(v) : MFString{};
      pushMF(ctx, d, [](duk_context *c, const std::string &s) {
        duk_push_string(c, s.c_str());
      });
      return;
    }
    case X3DFieldType::MFColor: {
      MFColor d = has ? std::any_cast<MFColor>(v) : MFColor{};
      pushMF(ctx, d, pushSFColor); return;
    }
    case X3DFieldType::MFColorRGBA: {
      MFColorRGBA d = has ? std::any_cast<MFColorRGBA>(v) : MFColorRGBA{};
      pushMF(ctx, d, pushSFColorRGBA); return;
    }
    case X3DFieldType::MFVec2f: {
      MFVec2f d = has ? std::any_cast<MFVec2f>(v) : MFVec2f{};
      pushMF(ctx, d, pushSFVec2f); return;
    }
    case X3DFieldType::MFVec2d: {
      MFVec2d d = has ? std::any_cast<MFVec2d>(v) : MFVec2d{};
      pushMF(ctx, d, pushSFVec2d); return;
    }
    case X3DFieldType::MFVec3f: {
      MFVec3f d = has ? std::any_cast<MFVec3f>(v) : MFVec3f{};
      pushMF(ctx, d, pushSFVec3f); return;
    }
    case X3DFieldType::MFVec3d: {
      MFVec3d d = has ? std::any_cast<MFVec3d>(v) : MFVec3d{};
      pushMF(ctx, d, pushSFVec3d); return;
    }
    case X3DFieldType::MFVec4f: {
      MFVec4f d = has ? std::any_cast<MFVec4f>(v) : MFVec4f{};
      pushMF(ctx, d, pushSFVec4f); return;
    }
    case X3DFieldType::MFVec4d: {
      MFVec4d d = has ? std::any_cast<MFVec4d>(v) : MFVec4d{};
      pushMF(ctx, d, pushSFVec4d); return;
    }
    case X3DFieldType::MFRotation: {
      MFRotation d = has ? std::any_cast<MFRotation>(v) : MFRotation{};
      pushMF(ctx, d, pushSFRotation); return;
    }
    case X3DFieldType::MFNode: {
      MFNode d = has ? std::any_cast<MFNode>(v) : MFNode{};
      pushMF(ctx, d, pushNode); return;
    }
    // --- matrices: flat row-major arrays ---
    case X3DFieldType::SFMatrix3f:
      pushSFMatrix<SFMatrix3f, 3>(ctx, has ? std::any_cast<SFMatrix3f>(v)
                                            : SFMatrix3f{});
      return;
    case X3DFieldType::SFMatrix4f:
      pushSFMatrix<SFMatrix4f, 4>(ctx, has ? std::any_cast<SFMatrix4f>(v)
                                            : SFMatrix4f{});
      return;
    case X3DFieldType::SFMatrix3d:
      pushSFMatrix<SFMatrix3d, 3>(ctx, has ? std::any_cast<SFMatrix3d>(v)
                                            : SFMatrix3d{});
      return;
    case X3DFieldType::SFMatrix4d:
      pushSFMatrix<SFMatrix4d, 4>(ctx, has ? std::any_cast<SFMatrix4d>(v)
                                             : SFMatrix4d{});
      return;
    // --- images: spec-canonical {x, y, comp, array} (19777-1 ECMAScript) ---
    case X3DFieldType::SFImage:
      pushSFImage(ctx, has ? std::any_cast<SFImage>(v) : SFImage{});
      return;
    case X3DFieldType::MFImage: {
      MFImage d = has ? std::any_cast<MFImage>(v) : MFImage{};
      pushMF(ctx, d, pushSFImage); return;
    }
    // --- MF matrices: arrays of flat row-major matrices ---
    case X3DFieldType::MFMatrix3f: {
      MFMatrix3f d = has ? std::any_cast<MFMatrix3f>(v) : MFMatrix3f{};
      pushMF(ctx, d, [](duk_context *c, const SFMatrix3f &m) {
        pushSFMatrix<SFMatrix3f, 3>(c, m);
      });
      return;
    }
    case X3DFieldType::MFMatrix4f: {
      MFMatrix4f d = has ? std::any_cast<MFMatrix4f>(v) : MFMatrix4f{};
      pushMF(ctx, d, [](duk_context *c, const SFMatrix4f &m) {
        pushSFMatrix<SFMatrix4f, 4>(c, m);
      });
      return;
    }
    case X3DFieldType::MFMatrix3d: {
      MFMatrix3d d = has ? std::any_cast<MFMatrix3d>(v) : MFMatrix3d{};
      pushMF(ctx, d, [](duk_context *c, const SFMatrix3d &m) {
        pushSFMatrix<SFMatrix3d, 3>(c, m);
      });
      return;
    }
    case X3DFieldType::MFMatrix4d: {
      MFMatrix4d d = has ? std::any_cast<MFMatrix4d>(v) : MFMatrix4d{};
      pushMF(ctx, d, [](duk_context *c, const SFMatrix4d &m) {
        pushSFMatrix<SFMatrix4d, 4>(c, m);
      });
      return;
    }
    default:
      // Unsupported/exotic type (enums): push undefined.
      duk_push_undefined(ctx);
      return;
  }
}

// ---------------------------------------------------------------------------
// toValue: JS -> std::any, by X3DFieldType.
// ---------------------------------------------------------------------------

std::any EcmaScriptBackend::toValue(duk_context *ctx, duk_idx_t i,
                                    X3DFieldType type) {
  i = duk_normalize_index(ctx, i);
  switch (type) {
    case X3DFieldType::SFBool:
      return SFBool(duk_to_boolean(ctx, i));
    case X3DFieldType::SFInt32:
      return SFInt32(duk_to_int(ctx, i));
    case X3DFieldType::SFFloat:
      return SFFloat((float)duk_to_number(ctx, i));
    case X3DFieldType::SFDouble:
      return SFDouble(duk_to_number(ctx, i));
    case X3DFieldType::SFTime:
      return SFTime(duk_to_number(ctx, i));
    case X3DFieldType::SFString:
      return SFString(duk_to_string(ctx, i) ? duk_to_string(ctx, i) : "");
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
      return toMF<MFBool>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return SFBool(duk_to_boolean(c, j));
      });
    case X3DFieldType::MFInt32:
      return toMF<MFInt32>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return SFInt32(duk_to_int(c, j));
      });
    case X3DFieldType::MFFloat:
      return toMF<MFFloat>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return SFFloat((float)duk_to_number(c, j));
      });
    case X3DFieldType::MFDouble:
      return toMF<MFDouble>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return SFDouble(duk_to_number(c, j));
      });
    case X3DFieldType::MFTime:
      return toMF<MFTime>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return SFTime(duk_to_number(c, j));
      });
    case X3DFieldType::MFString:
      return toMF<MFString>(ctx, i, [](duk_context *c, duk_idx_t j) {
        const char *s = duk_to_string(c, j);
        return SFString(s ? s : "");
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
      return toMF<MFNode>(ctx, i, [](duk_context *c, duk_idx_t j) {
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
      return toMF<MFMatrix3f>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return toSFMatrix<SFMatrix3f, 3, float>(c, j);
      });
    case X3DFieldType::MFMatrix4f:
      return toMF<MFMatrix4f>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return toSFMatrix<SFMatrix4f, 4, float>(c, j);
      });
    case X3DFieldType::MFMatrix3d:
      return toMF<MFMatrix3d>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return toSFMatrix<SFMatrix3d, 3, double>(c, j);
      });
    case X3DFieldType::MFMatrix4d:
      return toMF<MFMatrix4d>(ctx, i, [](duk_context *c, duk_idx_t j) {
        return toSFMatrix<SFMatrix4d, 4, double>(c, j);
      });
    default:
      return {};
  }
}

// ===========================================================================
// Browser global object — native callbacks bound to the script's SaiContext.
// ===========================================================================

namespace {

// Helper: fetch the SaiContext for the running native call (or nullptr).
// The SaiContext* is stamped on each Browser method/getter as a hidden own
// property at install time (see installBrowser), so the C callback recovers it
// from the currently-running function without touching the private Entry type.
SaiContext *saiOf(duk_context *ctx) {
  duk_push_current_function(ctx);
  duk_get_prop_string(ctx, -1, "\xff" "sai");
  void *p = duk_get_pointer(ctx, -1);
  duk_pop_2(ctx);
  return static_cast<SaiContext *>(p);
}

duk_ret_t browser_print(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  const char *s = duk_safe_to_string(ctx, 0);
  if (sai && s) sai->print(s);
  return 0;
}

duk_ret_t browser_getName(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  duk_push_string(ctx, sai ? sai->getName().c_str() : "");
  return 1;
}

duk_ret_t browser_getVersion(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  duk_push_string(ctx, sai ? sai->getVersion().c_str() : "");
  return 1;
}

duk_ret_t browser_getCurrentTime(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  duk_push_number(ctx, sai ? sai->currentTime() : 0.0);
  return 1;
}

duk_ret_t browser_getCurrentFrameRate(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  duk_push_number(ctx, sai ? sai->currentFrameRate() : 0.0);
  return 1;
}

// addRoute(fromNode, fromField, toNode, toField)
duk_ret_t browser_addRoute(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  if (!sai) return 0;
  X3DNode *from = extractNode(ctx, 0);
  const char *fromField = duk_to_string(ctx, 1);
  X3DNode *to = extractNode(ctx, 2);
  const char *toField = duk_to_string(ctx, 3);
  try {
    sai->addRoute(from, fromField ? fromField : "", to, toField ? toField : "");
  } catch (const std::exception &e) {
    return duk_error(ctx, DUK_ERR_ERROR, "addRoute: %s", e.what());
  }
  return 0;
}

duk_ret_t browser_deleteRoute(duk_context *ctx) {
  SaiContext *sai = saiOf(ctx);
  if (!sai) return 0;
  X3DNode *from = extractNode(ctx, 0);
  const char *fromField = duk_to_string(ctx, 1);
  X3DNode *to = extractNode(ctx, 2);
  const char *toField = duk_to_string(ctx, 3);
  try {
    sai->deleteRoute(from, fromField ? fromField : "", to,
                     toField ? toField : "");
  } catch (const std::exception &e) {
    return duk_error(ctx, DUK_ERR_ERROR, "deleteRoute: %s", e.what());
  }
  return 0;
}

// Define one Browser method, stamping the SaiContext* on it so the C callback
// can recover it (avoids depending on the private Entry layout).
void defineBrowserMethod(duk_context *ctx, const char *name,
                         duk_c_function fn, duk_idx_t nargs, SaiContext *sai) {
  duk_push_c_function(ctx, fn, nargs);
  duk_push_pointer(ctx, sai);
  duk_put_prop_string(ctx, -2, "\xff" "sai");
  duk_put_prop_string(ctx, -2, name);
}

// Define a Browser accessor (getter) property backed by a C function.
void defineBrowserGetter(duk_context *ctx, const char *name, duk_c_function fn,
                         SaiContext *sai) {
  duk_push_string(ctx, name);
  duk_push_c_function(ctx, fn, 0);
  duk_push_pointer(ctx, sai);
  duk_put_prop_string(ctx, -2, "\xff" "sai");
  duk_def_prop(ctx, -3,
               DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_SET_ENUMERABLE |
                   DUK_DEFPROP_SET_CONFIGURABLE);
}

} // namespace

void EcmaScriptBackend::installBrowser(duk_context *ctx, Entry *entry) {
  SaiContext *sai = entry ? entry->sai : nullptr;

  // global Browser object.
  duk_push_global_object(ctx);
  duk_push_object(ctx);  // Browser

  // properties: currentTime, currentFrameRate (getters).
  defineBrowserGetter(ctx, "currentTime", browser_getCurrentTime, sai);
  defineBrowserGetter(ctx, "currentFrameRate", browser_getCurrentFrameRate, sai);

  // methods.
  defineBrowserMethod(ctx, "getName", browser_getName, 0, sai);
  defineBrowserMethod(ctx, "getVersion", browser_getVersion, 0, sai);
  defineBrowserMethod(ctx, "print", browser_print, 1, sai);
  defineBrowserMethod(ctx, "addRoute", browser_addRoute, 4, sai);
  defineBrowserMethod(ctx, "deleteRoute", browser_deleteRoute, 4, sai);

  duk_put_prop_string(ctx, -2, "Browser");  // global.Browser = {...}
  duk_pop(ctx);                              // pop global object
}

// ---------------------------------------------------------------------------
// Destructor — destroy every live duk_context.
// ---------------------------------------------------------------------------

EcmaScriptBackend::~EcmaScriptBackend() {
  for (auto &[handle, entry] : entries_) {
    if (entry.ctx) {
      duk_destroy_heap(entry.ctx);
    }
  }
}

// ---------------------------------------------------------------------------
// load(): create duk_context, install Browser, eval source, return handle.
// ---------------------------------------------------------------------------

ScriptHandle EcmaScriptBackend::load(X3DNode &scriptNode,
                                     const std::string &source,
                                     SaiContext &sai) {
  duk_context *ctx = duk_create_heap_default();
  if (!ctx) return kInvalidScriptHandle;

  ScriptHandle handle = nextHandle_++;
  Entry &entry = entries_[handle];
  entry = Entry{ctx, &scriptNode, &sai};

  installBrowser(ctx, &entry);

  // Evaluate the source to define global functions.
  if (duk_peval_string(ctx, source.c_str()) != 0) {
    std::cerr << "[EcmaScriptBackend] eval error: "
              << duk_safe_to_string(ctx, -1) << "\n";
    duk_pop(ctx);
    duk_destroy_heap(ctx);
    entries_.erase(handle);
    return kInvalidScriptHandle;
  }
  duk_pop(ctx);  // pop eval result
  return handle;
}

// ---------------------------------------------------------------------------
// initialize(): call the script's initialize() if defined.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::initialize(ScriptHandle handle) {
  Entry *e = entryFor(handle);
  if (!e) return;
  // §3.5: seed author-field globals from their boxed initialValue BEFORE the
  // script's initialize() runs, so the script reads its authored defaults.
  seedAuthorGlobals(*e);
  callGlobalNoArgs(e->ctx, "initialize");
  // initialize() may itself write outputOnly/inputOutput author fields (§29.2.3
  // permits it); read them back so an authored default emitted at init drives
  // any wired ROUTE (timestamp 0 = scene start).
  readbackAuthorGlobals(*e, 0.0);
}

// ---------------------------------------------------------------------------
// shutdown(): call shutdown() if defined, then destroy context.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::shutdown(ScriptHandle handle) {
  Entry *e = entryFor(handle);
  if (!e) return;
  callGlobalNoArgs(e->ctx, "shutdown");
  duk_destroy_heap(e->ctx);
  entries_.erase(handle);
}

// ---------------------------------------------------------------------------
// prepareEvents(): call the script's prepareEvents(now) if defined.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::prepareEvents(ScriptHandle handle, double now) {
  Entry *e = entryFor(handle);
  if (!e) return;
  duk_context *ctx = e->ctx;
  if (!duk_get_global_string(ctx, "prepareEvents")) {
    duk_pop(ctx);
    return;
  }
  if (!duk_is_callable(ctx, -1)) {
    duk_pop(ctx);
    return;
  }
  duk_push_number(ctx, now);
  if (duk_pcall(ctx, 1) != 0) {
    std::cerr << "[EcmaScriptBackend] prepareEvents error: "
              << duk_safe_to_string(ctx, -1) << "\n";
  }
  duk_pop(ctx);

  // §29.2.5: prepareEvents may "generate events to be handled by the X3D
  // browser's normal event processing sequence" — read back any author
  // outputOnly/inputOutput global the script wrote and emit it into the cascade,
  // exactly as invoke() does for an eventIn handler. Without this a Script that
  // emits via JS globals from prepareEvents (the common, directOutput=FALSE
  // pattern) produces no events at all.
  readbackAuthorGlobals(*e, now);
}

// ---------------------------------------------------------------------------
// invoke(): dispatch one inputOnly event — call handler(value, timestamp).
// ---------------------------------------------------------------------------

void EcmaScriptBackend::invoke(ScriptHandle handle,
                               const std::string &eventName,
                               const std::any &value,
                               X3DFieldType type,
                               double timestamp) {
  Entry *e = entryFor(handle);
  if (!e) return;
  duk_context *ctx = e->ctx;
  if (!duk_get_global_string(ctx, eventName.c_str())) {
    duk_pop(ctx);
    return;
  }
  if (!duk_is_callable(ctx, -1)) {
    duk_pop(ctx);
    return;
  }
  pushValue(ctx, value, type);       // arg 0: the field value
  duk_push_number(ctx, timestamp);   // arg 1: the timestamp (SFTime)
  if (duk_pcall(ctx, 2) != 0) {
    std::cerr << "[EcmaScriptBackend] handler '" << eventName
              << "' error: " << duk_safe_to_string(ctx, -1) << "\n";
  }
  duk_pop(ctx);  // pop result / error

  // §3.5: after the handler runs, read any author outputOnly/inputOutput field
  // the script wrote back into the store and emit it as a cascade event carrying
  // the TRIGGERING timestamp (so it fans out along ROUTEs from the author field).
  readbackAuthorGlobals(*e, timestamp);
}

// ---------------------------------------------------------------------------
// eventsProcessed(): call the script's eventsProcessed() if defined.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::eventsProcessed(ScriptHandle handle, double timestamp) {
  Entry *e = entryFor(handle);
  if (!e) return;
  callGlobalNoArgs(e->ctx, "eventsProcessed");
  // §29.2.4: events generated from eventsProcessed() enter the cascade with the
  // timestamp of the last event processed — read back any author
  // outputOnly/inputOutput global the script wrote, exactly as invoke() does.
  readbackAuthorGlobals(*e, timestamp);
}

// ---------------------------------------------------------------------------
// Private helpers.
// ---------------------------------------------------------------------------

EcmaScriptBackend::Entry *EcmaScriptBackend::entryFor(ScriptHandle handle) {
  auto it = entries_.find(handle);
  return (it != entries_.end()) ? &it->second : nullptr;
}

namespace {

// Canonical JSON form of the JS value at top-of-stack (popped on exit) so two
// field values can be compared for equality without per-type operator== (the
// generated SF/MF structs have none). Used to suppress no-op author-field
// re-emits (an inputOutput field the handler did not actually change). Reuses
// the engine's own JSON.stringify, so scalars/vec-objects/arrays all serialize
// deterministically. Returns "" if stringify fails (treated as "changed").
std::string jsonOfTop(duk_context *ctx) {
  duk_dup(ctx, -1);                      // value to stringify
  if (duk_json_encode(ctx, -1) == nullptr) {
    duk_pop_2(ctx);
    return {};
  }
  const char *s = duk_get_string(ctx, -1);
  std::string out = s ? s : "";
  duk_pop(ctx);                          // pop the JSON string
  return out;
}

// Canonical JSON of a boxed std::any of `type` (independent throwaway push).
std::string jsonOfAny(duk_context *ctx, const std::any &v, X3DFieldType type) {
  EcmaScriptBackend::pushValue(ctx, v, type);
  std::string out = jsonOfTop(ctx);
  duk_pop(ctx);                          // pop the pushed value
  return out;
}

} // namespace

// ---------------------------------------------------------------------------
// seedAuthorGlobals(): boxed initialValue -> JS global, per readable author
// field. Pushes initializeOnly/inputOutput defaults so the script reads them.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::seedAuthorGlobals(Entry &e) {
  if (!e.node || !e.ctx) return;
  duk_context *ctx = e.ctx;
  for (const FieldInfo &info : dynamicFieldStore().authorFields(*e.node)) {
    // Only fields with a persistent value (initializeOnly/inputOutput) seed a
    // global; inputOnly/outputOnly carry no initial value. isReadable() == has
    // a get thunk, which the store synthesizes exactly for those two accesses.
    if (!info.isReadable() || !info.get) continue;
    std::any v = info.get(*e.node);
    if (!v.has_value()) continue;
    pushValue(ctx, v, info.type);
    duk_put_global_string(ctx, info.x3dName.c_str());
  }
}

// ---------------------------------------------------------------------------
// readbackAuthorGlobals(): JS global -> store + cascade event, per readable
// author field (outputOnly/inputOutput) the script defined.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::readbackAuthorGlobals(Entry &e, double timestamp) {
  if (!e.node || !e.ctx || !e.sai) return;
  duk_context *ctx = e.ctx;
  for (const FieldInfo &info : dynamicFieldStore().authorFields(*e.node)) {
    // Read back only fields the script may emit: outputOnly + inputOutput. Both
    // synthesize a get thunk in the store; inputOnly does not (isReadable false).
    if (!info.isReadable()) continue;
    if (!duk_get_global_string(ctx, info.x3dName.c_str())) {
      duk_pop(ctx);  // not defined yet
      continue;
    }
    if (duk_is_undefined(ctx, -1)) {
      duk_pop(ctx);  // handler never assigned it
      continue;
    }
    // Suppress no-op re-emit: skip if the JS value equals the stored value.
    std::any prev = dynamicFieldStore().getValue(*e.node, info.x3dName);
    if (prev.has_value() &&
        jsonOfTop(ctx) == jsonOfAny(ctx, prev, info.type)) {
      duk_pop(ctx);  // unchanged
      continue;
    }
    std::any value = toValue(ctx, -1, info.type);
    duk_pop(ctx);  // pop the global
    if (!value.has_value()) continue;
    // Record the new value, then post it as an event on the script's OWN field
    // (always permitted, §29.2.6) so it fans out along ROUTEs at the triggering
    // timestamp. (The cascade carries the value, not the timestamp itself; the
    // triggering-timestamp contract is satisfied by emitting within this
    // cascade — see ScriptSystem deliverInputEvent.)
    dynamicFieldStore().setValue(*e.node, info.x3dName, value);
    (void)timestamp;
    e.sai->setField(e.node, info.x3dName, value);
  }
}

bool EcmaScriptBackend::callGlobalNoArgs(duk_context *ctx,
                                         const char *fnName) {
  if (!duk_get_global_string(ctx, fnName)) {
    duk_pop(ctx);
    return false;
  }
  if (!duk_is_callable(ctx, -1)) {
    duk_pop(ctx);
    return false;
  }
  if (duk_pcall(ctx, 0) != 0) {
    std::cerr << "[EcmaScriptBackend] " << fnName
              << " error: " << duk_safe_to_string(ctx, -1) << "\n";
  }
  duk_pop(ctx);  // pop result / error
  return true;
}

} // namespace x3d::runtime
