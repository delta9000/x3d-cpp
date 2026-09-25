// EcmaScriptBackend.cpp
// ScriptEngine implementation backed by Duktape 2.7.0 (U4: full marshalling +
// Browser object + handler dispatch). See EcmaScriptBackend.hpp and the design
// spec docs/superpowers/specs/2026-06-16-script-sai-runtime-design.md §4.

#include "EcmaScriptBackend.hpp"

#include "DynamicField.hpp" // author-field store (S1 un-tabling, design §3.5)
#include "SaiContext.hpp"   // post author outputs into the cascade
#include "x3d/nodes/X3DNode.hpp"      // SFNode wrapping (X3DNode*)
#include "x3d/core/X3Dtypes.hpp"     // SF*/MF* concrete C++ types

#include <chrono>
#include <cstdio>
#include <memory>
#include <iostream>  // diagnostics
#include <stdexcept>

using namespace x3d::core;

namespace x3d::runtime {

namespace {

// Hidden Duktape property key. A key starting with the \xff byte is a hidden
// symbol: ECMAScript code can neither read, enumerate nor create it.
constexpr const char *kNodeIdKey = "\xff" "x3dNodeId";  // SFNode handle id
constexpr const char *kStashSaiKey = "sai";  // global-stash slot: SaiContext*

// -------------------------------------------------------------------------
// SFNode <-> JS opaque handle object.
// A non-null node becomes a JS object carrying its NodeHandleTable id in a
// hidden property; a null node becomes JS null. extractNode resolves the id
// through this script's table to the node's owning shared_ptr, or null if the
// id is unknown or the node is gone (see NodeHandleTable.hpp). The SaiContext
// that owns the table lives in the global stash, which scripts cannot reach.
// -------------------------------------------------------------------------

SaiContext *stashedSai(duk_context *ctx) {
  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, kStashSaiKey);
  void *p = duk_get_pointer(ctx, -1);
  duk_pop_2(ctx);
  return static_cast<SaiContext *>(p);
}

void pushNode(duk_context *ctx, const SFNode &node) {
  SaiContext *sai = stashedSai(ctx);
  if (!node || !sai) {
    duk_push_null(ctx);
    return;
  }
  duk_push_object(ctx);
  duk_push_uint(ctx, sai->nodeHandles().intern(node));
  duk_put_prop_string(ctx, -2, kNodeIdKey);
}

// Resolve the JS node-handle object (or null) at index idx.
SFNode extractNode(duk_context *ctx, duk_idx_t idx) {
  if (!duk_is_object(ctx, idx)) return nullptr;
  SaiContext *sai = stashedSai(ctx);
  if (!sai) return nullptr;
  if (!duk_get_prop_string(ctx, idx, kNodeIdKey)) {
    duk_pop(ctx);
    return nullptr;
  }
  const duk_uint_t id = duk_is_number(ctx, -1) ? duk_get_uint(ctx, -1) : 0;
  duk_pop(ctx);
  return sai->nodeHandles().resolve(static_cast<NodeHandleTable::Id>(id));
}

// -------------------------------------------------------------------------
// Call budget. Duktape polls DUK_USE_EXEC_TIMEOUT_CHECK (duk_config.h) while
// running bytecode; once the armed deadline has passed it raises a RangeError
// and keeps raising it until the deadline is disarmed, so a script cannot catch
// its way past the limit. ArmDeadline arms it for the duration of one public
// entry point (see ScriptEngine::setCallBudget).
// -------------------------------------------------------------------------

class ArmDeadline {
public:
  ArmDeadline(EcmaScriptBackend::CallDeadline *d,
              std::chrono::milliseconds budget)
      : d_(budget.count() > 0 ? d : nullptr) {
    if (!d_) return;
    d_->at = std::chrono::steady_clock::now() + budget;
    d_->armed = true;
  }
  ~ArmDeadline() {
    if (d_) d_->armed = false;
  }
  ArmDeadline(const ArmDeadline &) = delete;
  ArmDeadline &operator=(const ArmDeadline &) = delete;

private:
  EcmaScriptBackend::CallDeadline *d_;
};

// -------------------------------------------------------------------------
// Protected entry into the engine.
//
// Much of what the backend does can run script code: reading a global can hit a
// script-defined accessor, JSON-encoding a value calls toJSON, reading a
// structured value or an array can hit getters or Proxy traps. A script error
// there is raised outside any handler's pcall. With no catchpoint Duktape calls
// its fatal handler, which aborts the host process, so any document with a
// Script could crash the embedder.
//
// Every engine entry therefore runs under duk_safe_call. Duktape is compiled
// with DUK_USE_CPP_EXCEPTIONS (duk_config.h), so the error unwinds the C++
// frames inside `fn` as an exception, with destructors run, and lands here.
// On failure the error is logged, the value stack is restored to its entry
// height, and false is returned.
// -------------------------------------------------------------------------

template <typename Fn>
bool protectedRun(duk_context *ctx, const std::string &what, Fn fn) {
  const duk_int_t rc = duk_safe_call(
      ctx,
      [](duk_context *c, void *udata) -> duk_ret_t {
        (*static_cast<Fn *>(udata))(c);
        return 0;
      },
      &fn, 0 /*nargs*/, 1 /*nrets: undefined, or the error*/);
  if (rc != DUK_EXEC_SUCCESS) {
    std::cerr << "[EcmaScriptBackend] " << what
              << " error: " << duk_safe_to_string(ctx, -1) << "\n";
  }
  duk_pop(ctx);
  return rc == DUK_EXEC_SUCCESS;
}

// Call the script's global function `name` with the `nargs` values pushArgs
// pushes, under protectedRun. The lookup is protected too: a script can make
// the global itself a throwing accessor. A missing or non-callable global is a
// no-op. Returns true if the function was found and called (even if it threw).
template <typename PushArgs>
bool callGlobal(duk_context *ctx, const std::string &what, const char *name,
                duk_idx_t nargs, PushArgs pushArgs) {
  bool called = false;
  protectedRun(ctx, what, [&](duk_context *c) {
    duk_get_global_string(c, name);
    if (!duk_is_callable(c, -1)) return;
    pushArgs(c);
    called = true;
    duk_call(c, nargs);
  });
  return called;
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
      return extractNode(ctx, i);
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
        return extractNode(c, j);
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

// Browser.addRoute / deleteRoute (fromNode, fromField, toNode, toField).
//
// Errors raised here unwind as C++ exceptions (DUK_USE_CPP_EXCEPTIONS), so
// destructors run either way. The field names are still coerced first, and the
// JS error is raised only after the inner scope (the owning SFNodes and the
// caught std::exception) has unwound, so no C++ object is live mid-throw.
template <typename Op>
duk_ret_t routeOp(duk_context *ctx, const char *name, Op op) {
  SaiContext *sai = saiOf(ctx);
  if (!sai) return 0;
  const char *fromField = duk_to_string(ctx, 1);
  const char *toField = duk_to_string(ctx, 3);
  char err[256] = {0};
  {
    SFNode from = extractNode(ctx, 0);
    SFNode to = extractNode(ctx, 2);
    try {
      op(*sai, from.get(), fromField ? fromField : "", to.get(),
         toField ? toField : "");
    } catch (const std::exception &e) {
      std::snprintf(err, sizeof err, "%s", e.what());
      if (!err[0]) std::snprintf(err, sizeof err, "failed");
    }
  }
  if (err[0]) return duk_error(ctx, DUK_ERR_ERROR, "%s: %s", name, err);
  return 0;
}

duk_ret_t browser_addRoute(duk_context *ctx) {
  return routeOp(ctx, "addRoute",
                 [](SaiContext &sai, X3DNode *from, const char *fromField,
                    X3DNode *to, const char *toField) {
                   sai.addRoute(from, fromField, to, toField);
                 });
}

duk_ret_t browser_deleteRoute(duk_context *ctx) {
  return routeOp(ctx, "deleteRoute",
                 [](SaiContext &sai, X3DNode *from, const char *fromField,
                    X3DNode *to, const char *toField) {
                   sai.deleteRoute(from, fromField, to, toField);
                 });
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
  auto deadline = std::make_unique<CallDeadline>();
  duk_context *ctx =
      duk_create_heap(nullptr, nullptr, nullptr, deadline.get(), nullptr);
  if (!ctx) return kInvalidScriptHandle;

  ScriptHandle handle = nextHandle_++;
  Entry &entry = entries_[handle];
  entry = Entry{ctx, &scriptNode, &sai, std::move(deadline)};

  bool ok = false;
  {
    // The script's top level runs here too, so it gets a budget of its own.
    // Scoped: a failed load frees the deadline with the entry below.
    ArmDeadline armed(entry.deadline.get(), callBudget());

    // Stash the SaiContext for the SFNode marshalling helpers (pushNode /
    // extractNode), which run outside any Browser method, then install
    // Browser.
    const bool installed = protectedRun(ctx, "install", [&](duk_context *c) {
      duk_push_global_stash(c);
      duk_push_pointer(c, &sai);
      duk_put_prop_string(c, -2, kStashSaiKey);
      duk_pop(c);
      installBrowser(c, &entry);
    });

    // Evaluate the source to define global functions.
    if (installed) {
      if (duk_peval_string(ctx, source.c_str()) != 0) {
        std::cerr << "[EcmaScriptBackend] eval error: "
                  << duk_safe_to_string(ctx, -1) << "\n";
      } else {
        ok = true;
      }
      duk_pop(ctx);  // pop eval result / error
    }
  }
  if (!ok) {
    duk_destroy_heap(ctx);
    entries_.erase(handle);
    return kInvalidScriptHandle;
  }
  return handle;
}

// ---------------------------------------------------------------------------
// initialize(): call the script's initialize() if defined.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::initialize(ScriptHandle handle) {
  Entry *e = entryFor(handle);
  if (!e) return;
  ArmDeadline armed(e->deadline.get(), callBudget());
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
  {
    // Scoped: the deadline is freed with the entry below.
    ArmDeadline armed(e->deadline.get(), callBudget());
    callGlobalNoArgs(e->ctx, "shutdown");
  }
  duk_destroy_heap(e->ctx);
  entries_.erase(handle);
}

// ---------------------------------------------------------------------------
// prepareEvents(): call the script's prepareEvents(now) if defined.
// ---------------------------------------------------------------------------

void EcmaScriptBackend::prepareEvents(ScriptHandle handle, double now) {
  Entry *e = entryFor(handle);
  if (!e) return;
  ArmDeadline armed(e->deadline.get(), callBudget());
  callGlobal(e->ctx, "prepareEvents", "prepareEvents", 1,
             [now](duk_context *c) { duk_push_number(c, now); });

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
  ArmDeadline armed(e->deadline.get(), callBudget());
  callGlobal(e->ctx, "handler '" + eventName + "'", eventName.c_str(), 2,
             [&](duk_context *c) {
               pushValue(c, value, type);        // arg 0: the field value
               duk_push_number(c, timestamp);    // arg 1: the timestamp (SFTime)
             });

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
  ArmDeadline armed(e->deadline.get(), callBudget());
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
    // Protected: the script's top level may already have made this global a
    // throwing accessor.
    protectedRun(ctx, "seed of '" + info.x3dName + "'", [&](duk_context *c) {
      pushValue(c, v, info.type);
      duk_put_global_string(c, info.x3dName.c_str());
    });
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
    // Read the global and convert it under protection: the value may be an
    // accessor, carry a toJSON, or be a Proxy, and any of those can throw. A
    // throw drops this field's event for this callback (logged), nothing else.
    std::any prev = dynamicFieldStore().getValue(*e.node, info.x3dName);
    std::any value;
    protectedRun(ctx, "readback of '" + info.x3dName + "'",
                 [&](duk_context *c) {
      duk_get_global_string(c, info.x3dName.c_str());
      if (duk_is_undefined(c, -1)) return;  // never defined / never assigned
      // Suppress no-op re-emit: skip if the JS value equals the stored value.
      if (prev.has_value() && jsonOfTop(c) == jsonOfAny(c, prev, info.type))
        return;
      value = toValue(c, -1, info.type);
    });
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
  return callGlobal(ctx, fnName, fnName, 0, [](duk_context *) {});
}

} // namespace x3d::runtime

// Duktape's exec-timeout hook (DUK_USE_EXEC_TIMEOUT_CHECK in duk_config.h). The
// heap udata is the script's CallDeadline (see EcmaScriptBackend::load).
extern "C" int x3d_duk_exec_timeout_check(void *udata) {
  const auto *d =
      static_cast<const x3d::runtime::EcmaScriptBackend::CallDeadline *>(udata);
  return d && d->armed && std::chrono::steady_clock::now() >= d->at;
}
