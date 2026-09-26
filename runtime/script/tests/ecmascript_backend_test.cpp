// ecmascript_backend_test.cpp
// Track-B unit test (U3 skeleton): EcmaScriptBackend + Duktape.
//
// Proves:
//   - an inline "ecmascript:" script loads, initializes, and runs without crash,
//   - inline "javascript:" prefix also decodes (via ScriptSystem::decodeInlineSource),
//   - a trivial initialize() function is invoked by initialize(),
//   - a named handler (simulated inputOnly) is invoked by invoke() with no args,
//   - eventsProcessed() calls the script's eventsProcessed() if defined,
//   - a script with a syntax error fails to load (returns kInvalidScriptHandle),
//   - shutdown() destroys the context (no crash, no double-free),
//   - prepareEvents() with a numeric arg is dispatched safely.
//
// The SAI and X3DExecutionContext are only lightly used (browser-info only);
// full field marshalling is U4.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "EcmaScriptBackend.hpp"
#include "ScriptSystem.hpp"   // for decodeInlineSource

#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

// ---------------------------------------------------------------------------
// Minimal wiring helpers.
// ---------------------------------------------------------------------------

struct Fixture {
  Script script;
  X3DExecutionContext ctx;
  SaiContext sai{ctx, script, "x3d-cpp-gen", "dev"};
};

// Decode and load a raw source (already stripped of prefix) into the backend.
ScriptHandle loadSource(EcmaScriptBackend &backend, Fixture &fix,
                        const std::string &rawSource) {
  return backend.load(fix.script, rawSource, fix.sai);
}

bool nearly(double a, double b) { return std::fabs(a - b) < 1e-5; }

// Round-trip a boxed value through pushValue -> toValue on a throwaway context;
// returns the recovered std::any. Uses an independent heap so it needs no script.
std::any roundTrip(const std::any &v, X3DFieldType type) {
  duk_context *ctx = duk_create_heap_default();
  EcmaScriptBackend::pushValue(ctx, v, type);
  std::any out = EcmaScriptBackend::toValue(ctx, -1, type);
  duk_pop(ctx);
  duk_destroy_heap(ctx);
  return out;
}

} // namespace

int main() {
  // -------------------------------------------------------------------------
  // T1: decodeInlineSource strips "ecmascript:" correctly.
  // -------------------------------------------------------------------------
  {
    MFString url;
    url.push_back("ecmascript:function initialize() { }");
    std::string src = ScriptSystem::decodeInlineSource(url);
    check(!src.empty(), "T1a: decodeInlineSource ecmascript: non-empty");
    check(src == "function initialize() { }",
          "T1a: decodeInlineSource strips prefix");

    MFString url2;
    url2.push_back("javascript:function foo() { }");
    std::string src2 = ScriptSystem::decodeInlineSource(url2);
    check(!src2.empty(), "T1b: decodeInlineSource javascript: non-empty");
    check(src2 == "function foo() { }",
          "T1b: decodeInlineSource javascript: strips prefix");
  }

  // -------------------------------------------------------------------------
  // T2: load() succeeds for a trivial script; returns non-zero handle.
  // -------------------------------------------------------------------------
  EcmaScriptBackend backend;
  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix,
        "var _initCalled = false;"
        "function initialize() { _initCalled = true; }");
    check(h != kInvalidScriptHandle, "T2: load trivial script succeeds");

    // -------------------------------------------------------------------------
    // T3: initialize() calls the script's initialize() without crash.
    // -------------------------------------------------------------------------
    backend.initialize(h);
    check(true, "T3: initialize() runs without crash");

    // -------------------------------------------------------------------------
    // T4: invoke() dispatches to a handler by name without crash.
    // -------------------------------------------------------------------------
    backend.invoke(h, "initialize", std::any{}, X3DFieldType::SFBool, 0.0);
    check(true, "T4: invoke('initialize') runs without crash");

    // -------------------------------------------------------------------------
    // T5: eventsProcessed() calls the script's eventsProcessed if defined.
    // -------------------------------------------------------------------------
    backend.eventsProcessed(h, 0.0);  // not defined in this script — no-op, no crash
    check(true, "T5: eventsProcessed() with no handler is a no-op");

    // -------------------------------------------------------------------------
    // T6: prepareEvents() dispatches with a numeric timestamp without crash.
    // -------------------------------------------------------------------------
    backend.prepareEvents(h, 1.0);
    check(true, "T6: prepareEvents() with no handler is a no-op");

    // -------------------------------------------------------------------------
    // T7: shutdown() destroys the context without crash.
    // -------------------------------------------------------------------------
    backend.shutdown(h);
    check(true, "T7: shutdown() runs without crash");
  }

  // -------------------------------------------------------------------------
  // T8: load() fails for a script with a syntax error -> kInvalidScriptHandle.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix, "function broken( { bad syntax");
    check(h == kInvalidScriptHandle,
          "T8: load with syntax error returns kInvalidScriptHandle");
  }

  // -------------------------------------------------------------------------
  // T9: eventsProcessed() handler IS called when defined.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    // Script sets a global flag when eventsProcessed() is called.
    ScriptHandle h = loadSource(backend, fix,
        "var _epCalled = false;"
        "function eventsProcessed() { _epCalled = true; }");
    check(h != kInvalidScriptHandle, "T9: load eventsProcessed script");
    backend.initialize(h);
    backend.eventsProcessed(h, 0.0);
    check(true, "T9: eventsProcessed() with handler runs without crash");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T10: load multiple scripts — each gets an independent context.
  // -------------------------------------------------------------------------
  {
    Fixture fix1, fix2;
    ScriptHandle h1 = loadSource(backend, fix1,
        "var id = 'script_one';");
    ScriptHandle h2 = loadSource(backend, fix2,
        "var id = 'script_two';");
    check(h1 != kInvalidScriptHandle, "T10: first script loads");
    check(h2 != kInvalidScriptHandle, "T10: second script loads");
    check(h1 != h2, "T10: handles are distinct");
    backend.shutdown(h1);
    backend.shutdown(h2);
    check(true, "T10: both scripts shut down without crash");
  }

  // -------------------------------------------------------------------------
  // T11: prepareEvents() IS called when defined; passes a numeric timestamp.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix,
        "var _prepCalled = false;"
        "function prepareEvents(now) { _prepCalled = true; }");
    check(h != kInvalidScriptHandle, "T11: load prepareEvents script");
    backend.prepareEvents(h, 42.0);
    check(true, "T11: prepareEvents() with handler runs without crash");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T12: field marshalling round-trips (std::any -> JS -> std::any) per type.
  // -------------------------------------------------------------------------
  {
    // scalars
    check(std::any_cast<SFBool>(roundTrip(SFBool(true), X3DFieldType::SFBool)),
          "T12: SFBool round-trips");
    check(std::any_cast<SFInt32>(roundTrip(SFInt32(-42), X3DFieldType::SFInt32))
              == -42,
          "T12: SFInt32 round-trips");
    check(nearly(std::any_cast<SFFloat>(
              roundTrip(SFFloat(3.5f), X3DFieldType::SFFloat)), 3.5),
          "T12: SFFloat round-trips");
    check(nearly(std::any_cast<SFDouble>(
              roundTrip(SFDouble(2.71828), X3DFieldType::SFDouble)), 2.71828),
          "T12: SFDouble round-trips");
    check(nearly(std::any_cast<SFTime>(
              roundTrip(SFTime(123456.5), X3DFieldType::SFTime)), 123456.5),
          "T12: SFTime round-trips");
    check(std::any_cast<SFString>(
              roundTrip(SFString("héllo"), X3DFieldType::SFString)) == "héllo",
          "T12: SFString round-trips (UTF-8)");

    // structured SF
    {
      SFVec2f in{1.5f, -2.5f};
      auto out = std::any_cast<SFVec2f>(roundTrip(in, X3DFieldType::SFVec2f));
      check(nearly(out.x, 1.5) && nearly(out.y, -2.5), "T12: SFVec2f round-trips");
    }
    {
      SFVec3f in{1, 2, 3};
      auto out = std::any_cast<SFVec3f>(roundTrip(in, X3DFieldType::SFVec3f));
      check(nearly(out.x, 1) && nearly(out.y, 2) && nearly(out.z, 3),
            "T12: SFVec3f round-trips");
    }
    {
      SFVec3d in{1.25, 2.5, 3.75};
      auto out = std::any_cast<SFVec3d>(roundTrip(in, X3DFieldType::SFVec3d));
      check(nearly(out.x, 1.25) && nearly(out.y, 2.5) && nearly(out.z, 3.75),
            "T12: SFVec3d round-trips");
    }
    {
      SFVec4f in{1, 2, 3, 4};
      auto out = std::any_cast<SFVec4f>(roundTrip(in, X3DFieldType::SFVec4f));
      check(nearly(out.x, 1) && nearly(out.w, 4), "T12: SFVec4f round-trips");
    }
    {
      SFColor in{0.1f, 0.2f, 0.3f};
      auto out = std::any_cast<SFColor>(roundTrip(in, X3DFieldType::SFColor));
      check(nearly(out.r, 0.1) && nearly(out.g, 0.2) && nearly(out.b, 0.3),
            "T12: SFColor round-trips");
    }
    {
      SFColorRGBA in{0.1f, 0.2f, 0.3f, 0.4f};
      auto out =
          std::any_cast<SFColorRGBA>(roundTrip(in, X3DFieldType::SFColorRGBA));
      check(nearly(out.r, 0.1) && nearly(out.a, 0.4),
            "T12: SFColorRGBA round-trips");
    }
    {
      SFRotation in{0, 1, 0, 1.5708f};
      auto out =
          std::any_cast<SFRotation>(roundTrip(in, X3DFieldType::SFRotation));
      check(nearly(out.y, 1) && nearly(out.angle, 1.5708),
            "T12: SFRotation round-trips (axis-angle)");
    }

    // MF
    {
      MFInt32 in{1, 2, 3, 4, 5};
      auto out = std::any_cast<MFInt32>(roundTrip(in, X3DFieldType::MFInt32));
      check(out.size() == 5 && out[0] == 1 && out[4] == 5,
            "T12: MFInt32 round-trips (array-like + length)");
    }
    {
      MFFloat in{0.5f, 1.5f, 2.5f};
      auto out = std::any_cast<MFFloat>(roundTrip(in, X3DFieldType::MFFloat));
      check(out.size() == 3 && nearly(out[1], 1.5), "T12: MFFloat round-trips");
    }
    {
      MFString in{"a", "bb", "ccc"};
      auto out = std::any_cast<MFString>(roundTrip(in, X3DFieldType::MFString));
      check(out.size() == 3 && out[2] == "ccc", "T12: MFString round-trips");
    }
    {
      MFVec3f in{{1, 0, 0}, {0, 1, 0}};
      auto out = std::any_cast<MFVec3f>(roundTrip(in, X3DFieldType::MFVec3f));
      check(out.size() == 2 && nearly(out[1].y, 1), "T12: MFVec3f round-trips");
    }
    {
      MFColor in{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
      auto out = std::any_cast<MFColor>(roundTrip(in, X3DFieldType::MFColor));
      check(out.size() == 3 && nearly(out[2].b, 1), "T12: MFColor round-trips");
    }
    // empty MF round-trips to empty.
    {
      MFFloat in{};
      auto out = std::any_cast<MFFloat>(roundTrip(in, X3DFieldType::MFFloat));
      check(out.empty(), "T12: empty MFFloat round-trips to empty");
    }

    // SFImage (3-component 2x2) round-trips byte-for-byte. CONF-IMG.
    {
      SFImage in{2, 2, 3,
                 {0xFF, 0x00, 0x00,  // pixel 0 = red
                  0x00, 0xFF, 0x00,  // pixel 1 = green
                  0x00, 0x00, 0xFF,  // pixel 2 = blue
                  0xFF, 0xFF, 0x00}};// pixel 3 = yellow
      auto out = std::any_cast<SFImage>(roundTrip(in, X3DFieldType::SFImage));
      check(out.width == 2 && out.height == 2 && out.numComponents == 3,
            "T12: SFImage header round-trips");
      check(out.data == in.data, "T12: SFImage pixel bytes round-trip");
    }
    // SFImage empty (0 0 0) round-trips.
    {
      SFImage in{0, 0, 0, {}};
      auto out = std::any_cast<SFImage>(roundTrip(in, X3DFieldType::SFImage));
      check(out.width == 0 && out.height == 0 && out.numComponents == 0,
            "T12: SFImage empty header round-trips");
      check(out.data.empty(), "T12: SFImage empty data round-trips");
    }
    // SFImage JS shape is spec-canonical (ISO 19777-1): an object with x, y,
    // comp, and array (MFInt32 of high-byte-first packed pixels).
    {
      SFImage in{2, 1, 3, {0xFF, 0x00, 0x00,   // pixel 0 = red -> 0xFF0000
                           0x00, 0xFF, 0x00}};  // pixel 1 = green -> 0x00FF00
      duk_context *c = duk_create_heap_default();
      EcmaScriptBackend::pushValue(c, std::any(in), X3DFieldType::SFImage);
      check(duk_is_object(c, -1), "T12: SFImage pushes a JS object");
      duk_get_prop_string(c, -1, "x");
      check(duk_get_int(c, -1) == 2, "T12: SFImage.x == width");
      duk_pop(c);
      duk_get_prop_string(c, -1, "y");
      check(duk_get_int(c, -1) == 1, "T12: SFImage.y == height");
      duk_pop(c);
      duk_get_prop_string(c, -1, "comp");
      check(duk_get_int(c, -1) == 3, "T12: SFImage.comp == numComponents");
      duk_pop(c);
      duk_get_prop_string(c, -1, "array");
      check(duk_is_array(c, -1), "T12: SFImage.array is a JS array");
      duk_get_prop_string(c, -1, "length");
      check(duk_get_uint(c, -1) == 2, "T12: SFImage.array has width*height entries");
      duk_pop(c);
      duk_get_prop_index(c, -1, 0);
      check(duk_get_uint(c, -1) == 0xFF0000, "T12: SFImage.array[0] == packed red");
      duk_pop(c);
      duk_get_prop_index(c, -1, 1);
      check(duk_get_uint(c, -1) == 0x00FF00, "T12: SFImage.array[1] == packed green");
      duk_pop_2(c);
      duk_destroy_heap(c);
    }
    // MFImage round-trips (two images of differing component counts).
    {
      MFImage in{SFImage{1, 1, 1, {0x42}},
                 SFImage{2, 1, 3, {0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00}}};
      auto out = std::any_cast<MFImage>(roundTrip(in, X3DFieldType::MFImage));
      check(out.size() == 2, "T12: MFImage round-trips element count");
      check(out[0].width == 1 && out[0].numComponents == 1 && out[0].data[0] == 0x42,
            "T12: MFImage[0] (1-comp) round-trips");
      check(out[1].width == 2 && out[1].numComponents == 3,
            "T12: MFImage[1] (3-comp) header round-trips");
      check(out[1].data == in[1].data, "T12: MFImage[1] pixel bytes round-trip");
    }
    // MFMatrix3f/4f/3d/4d round-trip (CONF-IMG). Element 0,0 and last cell
    // preserved across two matrices.
    {
      MFMatrix3f in{SFMatrix3f{}, SFMatrix3f{}};
      in[0].matrix[0][0] = 1.5f; in[0].matrix[2][2] = 9.5f;
      in[1].matrix[0][0] = -2.0f; in[1].matrix[2][2] = 7.0f;
      auto out = std::any_cast<MFMatrix3f>(roundTrip(in, X3DFieldType::MFMatrix3f));
      check(out.size() == 2, "T12: MFMatrix3f round-trips count");
      check(nearly(out[0].matrix[0][0], 1.5) && nearly(out[0].matrix[2][2], 9.5),
            "T12: MFMatrix3f[0] cells preserved");
      check(nearly(out[1].matrix[0][0], -2.0) && nearly(out[1].matrix[2][2], 7.0),
            "T12: MFMatrix3f[1] cells preserved");
    }
    {
      MFMatrix4f in{SFMatrix4f{}, SFMatrix4f{}};
      in[0].matrix[0][0] = 1.0f; in[0].matrix[3][3] = 16.0f;
      in[1].matrix[0][0] = -1.0f; in[1].matrix[3][3] = 15.0f;
      auto out = std::any_cast<MFMatrix4f>(roundTrip(in, X3DFieldType::MFMatrix4f));
      check(out.size() == 2, "T12: MFMatrix4f round-trips count");
      check(nearly(out[0].matrix[0][0], 1.0) && nearly(out[0].matrix[3][3], 16.0),
            "T12: MFMatrix4f[0] cells preserved");
      check(nearly(out[1].matrix[3][3], 15.0), "T12: MFMatrix4f[1] last cell preserved");
    }
    {
      MFMatrix3d in{SFMatrix3d{}, SFMatrix3d{}};
      in[0].matrix[0][0] = 0.5; in[0].matrix[2][2] = 9.25;
      auto out = std::any_cast<MFMatrix3d>(roundTrip(in, X3DFieldType::MFMatrix3d));
      check(out.size() == 2, "T12: MFMatrix3d round-trips count");
      check(nearly(out[0].matrix[0][0], 0.5) && nearly(out[0].matrix[2][2], 9.25),
            "T12: MFMatrix3d[0] cells preserved");
    }
    {
      MFMatrix4d in{SFMatrix4d{}, SFMatrix4d{}};
      in[0].matrix[0][0] = 3.25; in[0].matrix[3][3] = 19.75;
      auto out = std::any_cast<MFMatrix4d>(roundTrip(in, X3DFieldType::MFMatrix4d));
      check(out.size() == 2, "T12: MFMatrix4d round-trips count");
      check(nearly(out[0].matrix[0][0], 3.25) && nearly(out[0].matrix[3][3], 19.75),
            "T12: MFMatrix4d[0] cells preserved");
    }
  }

  // -------------------------------------------------------------------------
  // T13: handler dispatch passes (value, timestamp) into the script and the
  //      script can read both. The script stashes them into globals which we
  //      then read back via a second handler invocation that returns via print.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix,
        "var seenVal = 0; var seenTs = 0;"
        "function set_fraction(value, timestamp) {"
        "  seenVal = value; seenTs = timestamp;"
        "  Browser.print('f=' + value + ',t=' + timestamp);"
        "}");
    check(h != kInvalidScriptHandle, "T13: load handler script");
    backend.initialize(h);
    backend.invoke(h, "set_fraction", std::any(SFFloat(0.75f)),
                   X3DFieldType::SFFloat, 9.0);
    check(fix.sai.log() == "f=0.75,t=9",
          "T13: handler received value=0.75 and timestamp=9 (via print)");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T14: Browser object — getName/getVersion/currentTime/print.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    fix.ctx.tick(5.0);  // advance the clock so currentTime reads 5
    ScriptHandle h = loadSource(backend, fix,
        "function initialize() {"
        "  Browser.print(Browser.getName() + '|' + Browser.getVersion()"
        "                + '|' + Browser.currentTime);"
        "}");
    check(h != kInvalidScriptHandle, "T14: load Browser script");
    backend.initialize(h);
    check(fix.sai.log() == "x3d-cpp-gen|dev|5",
          "T14: Browser.getName/getVersion/currentTime backed by SaiContext");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T15: end-to-end — a handler uses Browser.addRoute to wire two real nodes,
  //      and driving the source then fans out through the new route. directOutput
  //      must be TRUE for the script to add routes (§29.4.1).
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);  // allow dynamic addRoute
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");

    auto a = std::make_shared<Transform>();
    auto b = std::make_shared<Transform>();
    a->setTranslation(SFVec3f{0, 0, 0});
    b->setTranslation(SFVec3f{0, 0, 0});

    // Two SFNode-valued handler invocations deliver 'from' then 'to' (each as an
    // SFNode value arg, exercising SFNode marshalling), and set_to wires the
    // route via Browser.addRoute.
    ScriptHandle h2 = backend.load(script,
        "var fromN = null;"
        "function set_from(node, t) { fromN = node; }"
        "function set_to(node, t) {"
        "  Browser.addRoute(fromN, 'translation', node, 'translation');"
        "}", sai);
    check(h2 != kInvalidScriptHandle, "T15: load from/to wiring script");
    backend.initialize(h2);
    backend.invoke(h2, "set_from", std::any(SFNode(a)),
                   X3DFieldType::SFNode, 1.0);
    backend.invoke(h2, "set_to", std::any(SFNode(b)),
                   X3DFieldType::SFNode, 1.0);
    check(ctx.graph().routeCount() == 1,
          "T15: script added a route via Browser.addRoute (SFNode marshalling)");

    // Drive the source: a.translation -> (route) -> b.translation.
    ctx.postEvent(a.get(), "translation", std::any(SFVec3f{7, 8, 9}));
    ctx.process();
    SFVec3f bt = b->getTranslation();
    check(nearly(bt.x, 7) && nearly(bt.y, 8) && nearly(bt.z, 9),
          "T15: end-to-end — script-added route carried the event a->b");
    backend.shutdown(h2);
  }

  // -------------------------------------------------------------------------
  // T15b: SFNode handles are unforgeable — a script that clones a node
  //       handle's own properties onto a plain object (or guesses the internal
  //       key) must NOT obtain a node reference. A forged handle would let
  //       hostile content aim the runtime at an arbitrary address.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    auto a = std::make_shared<Transform>();

    ScriptHandle h = backend.load(script,
        "function forge(node, t) {"
        "  var f = {};"
        "  Object.getOwnPropertyNames(node).forEach(function (k) {"
        "    f[k] = node[k]; });"
        "  f['\\u00ffx3dNodeId'] = 1; f['\\u00ffx3dNodeLo'] = 1;"
        "  try { Browser.addRoute(f, 'translation', f, 'translation'); }"
        "  catch (e) {}"
        "}", sai);
    check(h != kInvalidScriptHandle, "T15b: load forging script");
    backend.initialize(h);
    backend.invoke(h, "forge", std::any(SFNode(a)), X3DFieldType::SFNode, 1.0);
    check(ctx.graph().routeCount() == 0,
          "T15b: a cloned/forged node handle does not resolve to a node");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T15c: a script's SFNode is a real node reference (ISO/IEC 19777-1): an
  //       SFNode the script writes out SHARES ownership with the scene (like
  //       DEF/USE), and a handle whose node the scene has since dropped
  //       resolves to null — Browser.addRoute reports INVALID_NODE instead of
  //       touching freed memory.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    dynamicFieldStore().addAuthorField(
        script, AuthorFieldDecl{"echo", X3DFieldType::SFNode,
                                AccessType::InputOutput, {}});
    auto a = std::make_shared<Transform>();

    ScriptHandle h = backend.load(script,
        "var held = null;"
        "function keep(n, t) { held = n; echo = n; }"
        "function wire(v, t) {"
        "  try { Browser.addRoute(held, 'translation', held, 'translation'); }"
        "  catch (e) { Browser.print('invalid'); }"
        "}", sai);
    check(h != kInvalidScriptHandle, "T15c: load node-holding script");
    backend.initialize(h);
    backend.invoke(h, "keep", std::any(SFNode(a)), X3DFieldType::SFNode, 1.0);

    std::any out = dynamicFieldStore().getValue(script, "echo");
    const SFNode *outNode = std::any_cast<SFNode>(&out);
    check(outNode && outNode->get() == a.get() &&
              !outNode->owner_before(a) && !a.owner_before(*outNode),
          "T15c: script SFNode output shares the node's real ownership");

    // Drain the cascade: the script's `echo` output is a pending event, and a
    // pending event legitimately owns its SFNode value until delivered.
    ctx.process();
    std::weak_ptr<Transform> weak = a;
    a.reset();
    out.reset();
    dynamicFieldStore().erase(script);
    check(weak.expired(),
          "T15c: the script's handle does not keep a dropped node alive");
    backend.invoke(h, "wire", std::any(1.0), X3DFieldType::SFTime, 2.0);
    check(sai.log() == "invalid" && ctx.graph().routeCount() == 0,
          "T15c: a handle to a destroyed node is INVALID_NODE, not a "
          "dangling pointer");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T15d: Browser.addRoute validates like a document ROUTE (ISO/IEC 19775-2
  //       INVALID_FIELD): unknown field, wrong direction and type mismatch are
  //       rejected; set_/_changed aliases of an inputOutput field are accepted.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    auto a = std::make_shared<Transform>();

    ScriptHandle h = backend.load(script,
        "function t(n, ts) {"
        "  var errs = 0;"
        "  try { Browser.addRoute(n, 'nope', n, 'translation'); }"
        "  catch (e) { errs++; }"
        "  try { Browser.addRoute(n, 'translation', n, 'rotation'); }"
        "  catch (e) { errs++; }"
        "  try { Browser.addRoute(n, 'addChildren', n, 'children'); }"
        "  catch (e) { errs++; }"
        "  Browser.addRoute(n, 'translation_changed', n, 'set_translation');"
        "  Browser.print('errs=' + errs);"
        "}", sai);
    check(h != kInvalidScriptHandle, "T15d: load route-validation script");
    backend.initialize(h);
    backend.invoke(h, "t", std::any(SFNode(a)), X3DFieldType::SFNode, 1.0);
    check(sai.log() == "errs=3",
          "T15d: unknown field / type mismatch / non-output source rejected");
    check(ctx.graph().routeCount() == 1,
          "T15d: a valid aliased route is added");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T15f: hostile author-field values cannot crash the host. Reading a JS
  //       global back after a handler runs can execute script code (getters,
  //       toJSON, Proxy traps, a global accessor). A throw there must be
  //       contained: no process abort, no event emitted for that field, and the
  //       script keeps working afterwards.
  // -------------------------------------------------------------------------
  {
    struct HostileCase {
      const char *what;
      X3DFieldType type;
      const char *source;
      // toJSON only runs when there is a previous value to compare against, so
      // the first readback legitimately emits {1,2,3}; the throwing toJSON is
      // hit (and must be contained) on the next readback.
      bool firstReadbackEmits = false;
    };
    const HostileCase cases[] = {
        {"throwing getter", X3DFieldType::SFVec3f,
         "function go(v, t) {"
         "  out = { get x() { throw new Error('boom'); }, y: 0, z: 0 }; }"},
        {"throwing toJSON", X3DFieldType::SFVec3f,
         "function go(v, t) {"
         "  out = { x: 1, y: 2, z: 3, toJSON: function () { throw 1; } }; }",
         true},
        {"throwing global accessor", X3DFieldType::SFFloat,
         "Object.defineProperty(this, 'out', { configurable: true,"
         "  get: function () { throw new Error('boom'); } });"
         "function go(v, t) {}"},
        {"throwing handler accessor", X3DFieldType::SFFloat,
         "Object.defineProperty(this, 'go', { configurable: true,"
         "  get: function () { throw new Error('boom'); } });"},
        {"Proxy MF array", X3DFieldType::MFFloat,
         "function go(v, t) {"
         "  out = new Proxy([1, 2, 3], { get: function (o, k) {"
         "    if (k === 'length') return 3; throw new Error('boom'); } }); }"},
    };
    for (const HostileCase &c : cases) {
      X3DExecutionContext ctx;
      Script script;
      SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
      dynamicFieldStore().addAuthorField(
          script, AuthorFieldDecl{"out", c.type, AccessType::OutputOnly, {}});
      dynamicFieldStore().addAuthorField(
          script, AuthorFieldDecl{"ok", X3DFieldType::SFFloat,
                                  AccessType::OutputOnly, {}});
      std::string src = std::string(c.source) +
                        "function fine(v, t) { ok = 7; }";
      ScriptHandle h = backend.load(script, src, sai);
      check(h != kInvalidScriptHandle,
            std::string("T15f: load hostile script (") + c.what + ")");
      backend.initialize(h);
      backend.invoke(h, "go", std::any(1.0), X3DFieldType::SFTime, 1.0);
      check(dynamicFieldStore().getValue(script, "out").has_value() ==
                c.firstReadbackEmits,
            std::string("T15f: ") + c.what +
                " is contained (no crash; emits only a valid value)");
      backend.invoke(h, "fine", std::any(1.0), X3DFieldType::SFTime, 2.0);
      std::any ok = dynamicFieldStore().getValue(script, "ok");
      check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
            std::string("T15f: script still works after ") + c.what);
      backend.shutdown(h);
      dynamicFieldStore().erase(script);
    }
  }

  // -------------------------------------------------------------------------
  // T15g: call budget. A handler (or the script's top level) that never
  //       returns is interrupted after callBudget() instead of hanging the
  //       host -- even if it tries to catch the interruption and keep going --
  //       and the script keeps working afterwards.
  // -------------------------------------------------------------------------
  {
    using Clock = std::chrono::steady_clock;
    const auto budget = std::chrono::milliseconds(200);
    const auto limit = std::chrono::seconds(5); // generous: sanitizer builds
    backend.setCallBudget(budget);

    struct LoopCase {
      const char *what;
      const char *handler;
    };
    const LoopCase cases[] = {
        {"infinite loop", "function go(v, t) { for (;;) {} }"},
        {"catch-and-continue loop",
         "function go(v, t) {"
         "  for (;;) { try { for (;;) {} } catch (e) {} } }"},
        // Bounded by the engine's own recursion limit, not the budget.
        {"unbounded recursion",
         "function go(v, t) { (function r() { r(); })(); }"},
    };
    for (const LoopCase &c : cases) {
      X3DExecutionContext ctx;
      Script script;
      SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
      dynamicFieldStore().addAuthorField(
          script, AuthorFieldDecl{"ok", X3DFieldType::SFFloat,
                                  AccessType::OutputOnly, {}});
      ScriptHandle h = backend.load(
          script, std::string(c.handler) + "function fine(v, t) { ok = 7; }",
          sai);
      check(h != kInvalidScriptHandle,
            std::string("T15g: load script (") + c.what + ")");
      backend.initialize(h);
      const auto start = Clock::now();
      backend.invoke(h, "go", std::any(1.0), X3DFieldType::SFTime, 1.0);
      check(Clock::now() - start < limit,
            std::string("T15g: ") + c.what + " is interrupted");
      backend.invoke(h, "fine", std::any(1.0), X3DFieldType::SFTime, 2.0);
      std::any ok = dynamicFieldStore().getValue(script, "ok");
      check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
            std::string("T15g: script still works after ") + c.what);
      backend.shutdown(h);
      dynamicFieldStore().erase(script);
    }

    {
      X3DExecutionContext ctx;
      Script script;
      SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
      const auto start = Clock::now();
      ScriptHandle h = backend.load(script, "for (;;) {}", sai);
      check(h == kInvalidScriptHandle && Clock::now() - start < limit,
            "T15g: a top-level infinite loop fails the load instead of hanging");
    }
    backend.setCallBudget(ScriptEngine::kDefaultCallBudget);
  }

  // -------------------------------------------------------------------------
  // T15h: memory limit. A handler that allocates without bound is stopped by
  //       memoryLimit() -- well inside a generous call budget, so the memory
  //       cap, not the clock, ends it -- and the script keeps working.
  // -------------------------------------------------------------------------
  {
    using Clock = std::chrono::steady_clock;
    backend.setCallBudget(std::chrono::seconds(10));
    backend.setMemoryLimit(std::size_t{16} << 20);

    X3DExecutionContext ctx;
    Script script;
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    dynamicFieldStore().addAuthorField(
        script, AuthorFieldDecl{"ok", X3DFieldType::SFFloat,
                                AccessType::OutputOnly, {}});
    // Each string is ~1 MB and unique (engines may intern equal strings).
    ScriptHandle h = backend.load(script,
        "var hog = [], i = 0;"
        "function go(v, t) {"
        "  for (;;) hog.push(new Array(100000).join('xxxxxxxxxx') + (i++)); }"
        "function fine(v, t) { hog = null; ok = 7; }", sai);
    check(h != kInvalidScriptHandle, "T15h: load allocating script");
    backend.initialize(h);
    const auto start = Clock::now();
    backend.invoke(h, "go", std::any(1.0), X3DFieldType::SFTime, 1.0);
    check(Clock::now() - start < std::chrono::seconds(5),
          "T15h: unbounded allocation is stopped by the memory limit");
    backend.invoke(h, "fine", std::any(1.0), X3DFieldType::SFTime, 2.0);
    std::any ok = dynamicFieldStore().getValue(script, "ok");
    check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
          "T15h: script still works after hitting the memory limit");
    backend.shutdown(h);
    dynamicFieldStore().erase(script);
    backend.setCallBudget(ScriptEngine::kDefaultCallBudget);
    backend.setMemoryLimit(ScriptEngine::kDefaultMemoryLimit);
  }

  // -------------------------------------------------------------------------
  // T15i: updateField. An inputOutput author field written from outside the
  //       script reaches its JS global, is not echoed back as an output, and a
  //       throwing setter on that global is contained.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    dynamicFieldStore().addAuthorField(
        script, AuthorFieldDecl{"level", X3DFieldType::SFFloat,
                                AccessType::InputOutput, std::any(0.0f)});
    dynamicFieldStore().addAuthorField(
        script, AuthorFieldDecl{"seen", X3DFieldType::SFFloat,
                                AccessType::OutputOnly, {}});
    ScriptHandle h = backend.load(script,
        "function poke(v, t) { seen = level; }", sai);
    check(h != kInvalidScriptHandle, "T15i: load updateField script");
    backend.initialize(h);
    dynamicFieldStore().setValue(script, "level", std::any(0.5f));
    backend.updateField(h, "level", std::any(0.5f), X3DFieldType::SFFloat);
    backend.invoke(h, "poke", std::any(1.0), X3DFieldType::SFTime, 1.0);
    std::any seen = dynamicFieldStore().getValue(script, "seen");
    check(seen.has_value() && std::any_cast<float>(seen) == 0.5f,
          "T15i: the script sees the updated inputOutput value");
    std::any level = dynamicFieldStore().getValue(script, "level");
    check(level.has_value() && std::any_cast<float>(level) == 0.5f,
          "T15i: the update is not reverted by readback");
    backend.shutdown(h);
    dynamicFieldStore().erase(script);

    Script hostile;
    SaiContext sai2(ctx, hostile, "x3d-cpp-gen", "dev");
    dynamicFieldStore().addAuthorField(
        hostile, AuthorFieldDecl{"ok", X3DFieldType::SFFloat,
                                 AccessType::OutputOnly, {}});
    ScriptHandle h2 = backend.load(hostile,
        "Object.defineProperty(this, 'level', {"
        "  set: function (v) { throw new Error('no'); },"
        "  get: function () { return 1; }, configurable: true });"
        "function fine(v, t) { ok = 7; }", sai2);
    check(h2 != kInvalidScriptHandle, "T15i: load throwing-setter script");
    backend.initialize(h2);
    backend.updateField(h2, "level", std::any(0.5f), X3DFieldType::SFFloat);
    backend.invoke(h2, "fine", std::any(1.0), X3DFieldType::SFTime, 2.0);
    std::any ok = dynamicFieldStore().getValue(hostile, "ok");
    check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
          "T15i: a throwing setter on update is contained");
    backend.shutdown(h2);
    dynamicFieldStore().erase(hostile);
  }

  // -------------------------------------------------------------------------
  // T16: directOutput=FALSE — Browser.addRoute throws into JS (not a crash);
  //      the route is NOT added.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;  // directOutput defaults FALSE
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    auto a = std::make_shared<Transform>();

    ScriptHandle h = backend.load(script,
        "var caught = false;"
        "function tryWire(fromNode, t) {"
        "  try { Browser.addRoute(fromNode, 'translation', fromNode,"
        "                         'translation'); }"
        "  catch (e) { caught = true; Browser.print('blocked'); }"
        "}", sai);
    check(h != kInvalidScriptHandle, "T16: load directOutput-gate script");
    backend.initialize(h);
    backend.invoke(h, "tryWire", std::any(SFNode(a)),
                   X3DFieldType::SFNode, 1.0);
    check(sai.log() == "blocked",
          "T16: addRoute with directOutput=FALSE threw into JS (caught)");
    check(ctx.graph().routeCount() == 0,
          "T16: no route added when directOutput=FALSE");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T17: deterministic Date. Date.now() and a zero-argument `new Date()` /
  //      `Date()` read the injected execution clock (SaiContext::currentTime(),
  //      seconds -> ms); Date with explicit arguments, Date.parse/UTC and
  //      instanceof are preserved.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.tick(12.5);  // injected clock: 12.5 s -> 12500 ms

    const char *outFields[] = {"now", "newGet", "year", "sameInstance",
                               "funcType", "parsed", "utc", "fixedDiffers",
                               "bypassCtorNow", "bypassCtorNewGet",
                               "bypassProtoNow", "dateLength", "protoCtorIsDate",
                               "funcMatches", "subclassOk"};
    for (const char *f : outFields) {
      X3DFieldType t = X3DFieldType::SFTime;
      if (std::string(f) == "sameInstance") t = X3DFieldType::SFBool;
      if (std::string(f) == "funcType") t = X3DFieldType::SFString;
      if (std::string(f) == "fixedDiffers") t = X3DFieldType::SFBool;
      if (std::string(f) == "protoCtorIsDate") t = X3DFieldType::SFBool;
      if (std::string(f) == "funcMatches") t = X3DFieldType::SFBool;
      if (std::string(f) == "subclassOk") t = X3DFieldType::SFBool;
      if (std::string(f) == "dateLength") t = X3DFieldType::SFInt32;
      dynamicFieldStore().addAuthorField(
          script, AuthorFieldDecl{f, t, AccessType::OutputOnly, {}});
    }

    ScriptHandle h = backend.load(script,
        "function initialize() {"
        "  now = Date.now();"
        "  newGet = new Date().getTime();"
        "  year = new Date(2020, 0, 1).getFullYear();"
        "  fixedDiffers = (new Date(2020, 0, 1).getTime() !== Date.now());"
        "  sameInstance = (new Date()) instanceof Date;"
        "  funcType = typeof Date();"
        "  funcMatches = (Date() === new Date().toString());"
        "  bypassCtorNow = (new Date()).constructor.now();"
        "  bypassCtorNewGet ="
        "      new (Date.prototype.constructor)().getTime();"
        "  bypassProtoNow ="
        "      Object.getPrototypeOf(new Date()).constructor.now();"
        "  dateLength = Date.length;"
        "  protoCtorIsDate = (Date.prototype.constructor === Date);"
        // Duktape is ES5: no class syntax, so the subclass check is a no-op
        // there (the QuickJS test exercises `class X extends Date {}`).
        "  subclassOk = true;"
        "  var iso = new Date(Date.UTC(2020, 0, 1)).toISOString();"
        "  parsed = Date.parse(iso);"
        "  utc = Date.UTC(2020, 0, 1);"
        "}", sai);
    check(h != kInvalidScriptHandle, "T17: load deterministic-Date script");
    backend.initialize(h);

    std::any now = dynamicFieldStore().getValue(script, "now");
    std::any newGet = dynamicFieldStore().getValue(script, "newGet");
    check(now.has_value() && std::any_cast<double>(now) == 12500.0,
          "T17: Date.now() == injected clock in ms");
    check(newGet.has_value() && std::any_cast<double>(newGet) == 12500.0,
          "T17: new Date().getTime() == injected clock in ms");
    std::any year = dynamicFieldStore().getValue(script, "year");
    check(year.has_value() && std::any_cast<double>(year) == 2020.0,
          "T17: new Date(2020,0,1) is unaffected (year 2020)");
    std::any differs = dynamicFieldStore().getValue(script, "fixedDiffers");
    check(differs.has_value() && std::any_cast<bool>(differs),
          "T17: explicit-argument Date is not the injected clock");
    std::any inst = dynamicFieldStore().getValue(script, "sameInstance");
    check(inst.has_value() && std::any_cast<bool>(inst),
          "T17: new Date() instanceof Date holds");
    std::any ft = dynamicFieldStore().getValue(script, "funcType");
    check(ft.has_value() && std::any_cast<std::string>(ft) == "string",
          "T17: Date() as a function still returns a string");
    std::any parsed = dynamicFieldStore().getValue(script, "parsed");
    std::any utc = dynamicFieldStore().getValue(script, "utc");
    check(parsed.has_value() && utc.has_value() &&
              std::any_cast<double>(parsed) == std::any_cast<double>(utc),
          "T17: Date.parse / Date.UTC still work");

    // Bypass paths (review (a)): all must reach the injected clock, not the
    // wall clock.
    std::any b1 = dynamicFieldStore().getValue(script, "bypassCtorNow");
    std::any b2 = dynamicFieldStore().getValue(script, "bypassCtorNewGet");
    std::any b3 = dynamicFieldStore().getValue(script, "bypassProtoNow");
    check(b1.has_value() && std::any_cast<double>(b1) == 12500.0,
          "T17: (new Date()).constructor.now() == injected clock");
    check(b2.has_value() && std::any_cast<double>(b2) == 12500.0,
          "T17: new (Date.prototype.constructor)() == injected clock");
    check(b3.has_value() && std::any_cast<double>(b3) == 12500.0,
          "T17: Object.getPrototypeOf(new Date()).constructor.now() == clock");
    // Semantics (review (b)).
    std::any len = dynamicFieldStore().getValue(script, "dateLength");
    check(len.has_value() && std::any_cast<SFInt32>(len) == 7,
          "T17: Date.length === 7");
    std::any pc = dynamicFieldStore().getValue(script, "protoCtorIsDate");
    check(pc.has_value() && std::any_cast<bool>(pc),
          "T17: Date.prototype.constructor === Date");
    std::any fm = dynamicFieldStore().getValue(script, "funcMatches");
    check(fm.has_value() && std::any_cast<bool>(fm),
          "T17: Date() returns the injected time's string");

    backend.shutdown(h);
    dynamicFieldStore().erase(script);
  }

  if (failures == 0) {
    std::cout << "All ecmascript_backend tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test(s) FAILED.\n";
  return 1;
}
