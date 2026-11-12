// Shared ScriptEngine behavior cases for both ECMAScript backends.
#pragma once

#include "DynamicField.hpp"
#include "ScriptEngine.hpp"
#include "ScriptSystem.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace script_backend_tests {

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

// Minimal wiring: a Script node + execution context + SAI surface per script.
struct Fixture {
  Script script;
  X3DExecutionContext ctx;
  SaiContext sai{ctx, script, "x3d-cpp-gen", "dev"};
};

ScriptHandle loadSource(ScriptEngine &backend, Fixture &fix,
                       const std::string &rawSource) {
  return backend.load(fix.script, rawSource, fix.sai);
}

bool nearly(double a, double b) { return std::fabs(a - b) < 1e-5; }

// Round-trip a boxed value through the public seam (pushValue on invoke's arg +
// toValue on the author-field readback), proving the marshalling is observable
// and reversible exactly as a real script would see it. Mechanism: declare an
// inputOutput author field `echo` of `type`, load a handler that copies its
// (value) argument into the `echo` global, invoke it, then read `echo` back out
// of the dynamic-field store. Returns the recovered std::any.
std::any roundTrip(ScriptEngine &backend, const std::any &v,
                  X3DFieldType type) {
  Fixture fix;
  // An inputOutput author field is readable+writable, so seed/readback handle it
  // and the store keeps its value. Seed an empty initialValue so any non-empty
  // echo differs from the prior value and is not suppressed as a no-op.
  fix.ctx.authorFields().addAuthorField(
      fix.script, AuthorFieldDecl{"echo", type, AccessType::InputOutput, {}});
  ScriptHandle h = backend.load(fix.script,
      "function set_echo(value, ts) { echo = value; }", fix.sai);
  if (h == kInvalidScriptHandle) return {};
  backend.initialize(h);
  backend.invoke(h, "set_echo", v, type, 0.0);
  std::any out = fix.ctx.authorFields().getValue(fix.script, "echo");
  backend.shutdown(h);
  fix.ctx.authorFields().erase(fix.script);
  return out;
}

// Date cases keep their existing identifiers in each test binary.
void runSharedCases(ScriptEngine &backend, const char *dateCase) {
  // decodeInlineSource strips "ecmascript:" correctly.

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

  // load() succeeds for a trivial script; returns non-zero handle.

  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix,
        "var _initCalled = false;"
        "function initialize() { _initCalled = true; }");
    check(h != kInvalidScriptHandle, "T2: load trivial script succeeds");

    // initialize() calls the script's initialize() without crash.

    backend.initialize(h);
    check(true, "T3: initialize() runs without crash");

    // invoke() dispatches to a handler by name without crash.

    backend.invoke(h, "initialize", std::any{}, X3DFieldType::SFBool, 0.0);
    check(true, "T4: invoke('initialize') runs without crash");

    // eventsProcessed() calls the script's eventsProcessed if defined.

    backend.eventsProcessed(h, 0.0);  // not defined in this script — no-op, no crash
    check(true, "T5: eventsProcessed() with no handler is a no-op");

    // prepareEvents() dispatches with a numeric timestamp without crash.

    backend.prepareEvents(h, 1.0);
    check(true, "T6: prepareEvents() with no handler is a no-op");

    // shutdown() destroys the context without crash.

    backend.shutdown(h);
    check(true, "T7: shutdown() runs without crash");
  }

  // load() fails for a script with a syntax error -> kInvalidScriptHandle.

  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix, "function broken( { bad syntax");
    check(h == kInvalidScriptHandle,
          "T8: load with syntax error returns kInvalidScriptHandle");
  }

  // eventsProcessed() handler IS called when defined.

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

  // load multiple scripts — each gets an independent context.

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

  // prepareEvents() IS called when defined; passes a numeric timestamp.

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

  // field marshalling round-trips (std::any -> JS -> std::any) per type.

  {
    // scalars
    check(std::any_cast<SFBool>(roundTrip(backend, SFBool(true), X3DFieldType::SFBool)),
          "T12: SFBool round-trips");
    check(std::any_cast<SFInt32>(roundTrip(backend, SFInt32(-42), X3DFieldType::SFInt32))
              == -42,
          "T12: SFInt32 round-trips");
    check(nearly(std::any_cast<SFFloat>(
              roundTrip(backend, SFFloat(3.5f), X3DFieldType::SFFloat)), 3.5),
          "T12: SFFloat round-trips");
    check(nearly(std::any_cast<SFDouble>(
              roundTrip(backend, SFDouble(2.71828), X3DFieldType::SFDouble)), 2.71828),
          "T12: SFDouble round-trips");
    check(nearly(std::any_cast<SFTime>(
              roundTrip(backend, SFTime(123456.5), X3DFieldType::SFTime)), 123456.5),
          "T12: SFTime round-trips");
    check(std::any_cast<SFString>(
              roundTrip(backend, SFString("héllo"), X3DFieldType::SFString)) == "héllo",
          "T12: SFString round-trips (UTF-8)");
    {
      SFString in = SFString("héllo") + '\0' + "tail";
      auto out = std::any_cast<SFString>(
          roundTrip(backend, in, X3DFieldType::SFString));
      check(out == in, "T12: SFString round-trips with an embedded NUL");
    }

    // structured SF
    {
      SFVec2f in{1.5f, -2.5f};
      auto out = std::any_cast<SFVec2f>(roundTrip(backend, in, X3DFieldType::SFVec2f));
      check(nearly(out.x, 1.5) && nearly(out.y, -2.5), "T12: SFVec2f round-trips");
    }
    {
      SFVec3f in{1, 2, 3};
      auto out = std::any_cast<SFVec3f>(roundTrip(backend, in, X3DFieldType::SFVec3f));
      check(nearly(out.x, 1) && nearly(out.y, 2) && nearly(out.z, 3),
            "T12: SFVec3f round-trips");
    }
    {
      SFVec3d in{1.25, 2.5, 3.75};
      auto out = std::any_cast<SFVec3d>(roundTrip(backend, in, X3DFieldType::SFVec3d));
      check(nearly(out.x, 1.25) && nearly(out.y, 2.5) && nearly(out.z, 3.75),
            "T12: SFVec3d round-trips");
    }
    {
      SFVec4f in{1, 2, 3, 4};
      auto out = std::any_cast<SFVec4f>(roundTrip(backend, in, X3DFieldType::SFVec4f));
      check(nearly(out.x, 1) && nearly(out.w, 4), "T12: SFVec4f round-trips");
    }
    {
      SFColor in{0.1f, 0.2f, 0.3f};
      auto out = std::any_cast<SFColor>(roundTrip(backend, in, X3DFieldType::SFColor));
      check(nearly(out.r, 0.1) && nearly(out.g, 0.2) && nearly(out.b, 0.3),
            "T12: SFColor round-trips");
    }
    {
      SFColorRGBA in{0.1f, 0.2f, 0.3f, 0.4f};
      auto out =
          std::any_cast<SFColorRGBA>(roundTrip(backend, in, X3DFieldType::SFColorRGBA));
      check(nearly(out.r, 0.1) && nearly(out.a, 0.4),
            "T12: SFColorRGBA round-trips");
    }
    {
      SFRotation in{0, 1, 0, 1.5708f};
      auto out =
          std::any_cast<SFRotation>(roundTrip(backend, in, X3DFieldType::SFRotation));
      check(nearly(out.y, 1) && nearly(out.angle, 1.5708),
            "T12: SFRotation round-trips (axis-angle)");
    }

    // MF
    {
      MFInt32 in{1, 2, 3, 4, 5};
      auto out = std::any_cast<MFInt32>(roundTrip(backend, in, X3DFieldType::MFInt32));
      check(out.size() == 5 && out[0] == 1 && out[4] == 5,
            "T12: MFInt32 round-trips (array-like + length)");
    }
    {
      MFFloat in{0.5f, 1.5f, 2.5f};
      auto out = std::any_cast<MFFloat>(roundTrip(backend, in, X3DFieldType::MFFloat));
      check(out.size() == 3 && nearly(out[1], 1.5), "T12: MFFloat round-trips");
    }
    {
      MFString in{"a", "bb", "ccc"};
      auto out = std::any_cast<MFString>(roundTrip(backend, in, X3DFieldType::MFString));
      check(out.size() == 3 && out[2] == "ccc", "T12: MFString round-trips");
    }
    {
      MFString in{"", SFString("left") + '\0' + "right", "tail"};
      auto out = std::any_cast<MFString>(
          roundTrip(backend, in, X3DFieldType::MFString));
      check(out == in,
            "T12: MFString round-trips empty and embedded-NUL strings");
    }
    {
      MFVec3f in{{1, 0, 0}, {0, 1, 0}};
      auto out = std::any_cast<MFVec3f>(roundTrip(backend, in, X3DFieldType::MFVec3f));
      check(out.size() == 2 && nearly(out[1].y, 1), "T12: MFVec3f round-trips");
    }
    {
      MFColor in{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
      auto out = std::any_cast<MFColor>(roundTrip(backend, in, X3DFieldType::MFColor));
      check(out.size() == 3 && nearly(out[2].b, 1), "T12: MFColor round-trips");
    }
    // empty MF round-trips to empty.
    {
      MFFloat in{};
      auto out = std::any_cast<MFFloat>(roundTrip(backend, in, X3DFieldType::MFFloat));
      check(out.empty(), "T12: empty MFFloat round-trips to empty");
    }

    // SFImage (3-component 2x2) round-trips byte-for-byte.
    {
      SFImage in{2, 2, 3,
                 {0xFF, 0x00, 0x00,  // pixel 0 = red
                  0x00, 0xFF, 0x00,  // pixel 1 = green
                  0x00, 0x00, 0xFF,  // pixel 2 = blue
                  0xFF, 0xFF, 0x00}};// pixel 3 = yellow
      auto out = std::any_cast<SFImage>(roundTrip(backend, in, X3DFieldType::SFImage));
      check(out.width == 2 && out.height == 2 && out.numComponents == 3,
            "T12: SFImage header round-trips");
      check(out.data == in.data, "T12: SFImage pixel bytes round-trip");
    }
    // SFImage empty (0 0 0) round-trips.
    {
      SFImage in{0, 0, 0, {}};
      auto out = std::any_cast<SFImage>(roundTrip(backend, in, X3DFieldType::SFImage));
      check(out.width == 0 && out.height == 0 && out.numComponents == 0,
            "T12: SFImage empty header round-trips");
      check(out.data.empty(), "T12: SFImage empty data round-trips");
    }
    // MFImage round-trips (two images of differing component counts).
    {
      MFImage in{SFImage{1, 1, 1, {0x42}},
                 SFImage{2, 1, 3, {0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00}}};
      auto out = std::any_cast<MFImage>(roundTrip(backend, in, X3DFieldType::MFImage));
      check(out.size() == 2, "T12: MFImage round-trips element count");
      check(out[0].width == 1 && out[0].numComponents == 1 && out[0].data[0] == 0x42,
            "T12: MFImage[0] (1-comp) round-trips");
      check(out[1].width == 2 && out[1].numComponents == 3,
            "T12: MFImage[1] (3-comp) header round-trips");
      check(out[1].data == in[1].data, "T12: MFImage[1] pixel bytes round-trip");
    }
    // Scalar matrices use flat row-major arrays.
    {
      SFMatrix3f in{}; in.matrix[0][0] = 1.5f; in.matrix[2][2] = 9.5f;
      auto out = std::any_cast<SFMatrix3f>(
          roundTrip(backend, in, X3DFieldType::SFMatrix3f));
      check(nearly(out.matrix[0][0], 1.5) && nearly(out.matrix[2][2], 9.5),
            "T12: SFMatrix3f cells preserved");
    }
    {
      SFMatrix4d in{}; in.matrix[0][0] = 3.25; in.matrix[3][3] = 19.75;
      auto out = std::any_cast<SFMatrix4d>(
          roundTrip(backend, in, X3DFieldType::SFMatrix4d));
      check(nearly(out.matrix[0][0], 3.25) && nearly(out.matrix[3][3], 19.75),
            "T12: SFMatrix4d cells preserved");
    }
    // Multi-value matrices preserve their count and selected cells.
    {
      MFMatrix3f in{SFMatrix3f{}, SFMatrix3f{}};
      in[0].matrix[0][0] = 1.5f; in[0].matrix[2][2] = 9.5f;
      in[1].matrix[0][0] = -2.0f; in[1].matrix[2][2] = 7.0f;
      auto out = std::any_cast<MFMatrix3f>(roundTrip(backend, in, X3DFieldType::MFMatrix3f));
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
      auto out = std::any_cast<MFMatrix4f>(roundTrip(backend, in, X3DFieldType::MFMatrix4f));
      check(out.size() == 2, "T12: MFMatrix4f round-trips count");
      check(nearly(out[0].matrix[0][0], 1.0) && nearly(out[0].matrix[3][3], 16.0),
            "T12: MFMatrix4f[0] cells preserved");
      check(nearly(out[1].matrix[3][3], 15.0), "T12: MFMatrix4f[1] last cell preserved");
      check(out.size() == 2 && nearly(out[0].matrix[3][3], 16.0) &&
                nearly(out[1].matrix[3][3], 15.0),
            "T12: MFMatrix4f cells preserved");
    }
    {
      MFMatrix3d in{SFMatrix3d{}, SFMatrix3d{}};
      in[0].matrix[0][0] = 0.5; in[0].matrix[2][2] = 9.25;
      auto out = std::any_cast<MFMatrix3d>(roundTrip(backend, in, X3DFieldType::MFMatrix3d));
      check(out.size() == 2, "T12: MFMatrix3d round-trips count");
      check(nearly(out[0].matrix[0][0], 0.5) && nearly(out[0].matrix[2][2], 9.25),
            "T12: MFMatrix3d[0] cells preserved");
    }
    {
      MFMatrix4d in{SFMatrix4d{}, SFMatrix4d{}};
      in[0].matrix[0][0] = 3.25; in[0].matrix[3][3] = 19.75;
      auto out = std::any_cast<MFMatrix4d>(roundTrip(backend, in, X3DFieldType::MFMatrix4d));
      check(out.size() == 2, "T12: MFMatrix4d round-trips count");
      check(nearly(out[0].matrix[0][0], 3.25) && nearly(out[0].matrix[3][3], 19.75),
            "T12: MFMatrix4d[0] cells preserved");
    }
  }

  // Handler dispatch exposes both the value and timestamp through Browser.print.

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

  // Browser object — getName/getVersion/currentTime/print.

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

  // Browser object — getName/getVersion/currentTime/currentFrameRate/print.

  {
    Fixture fix;
    fix.ctx.tick(5.0);                 // advance clock so currentTime reads 5
    fix.sai.setCurrentFrameRate(60.0); // so currentFrameRate reads 60
    ScriptHandle h = loadSource(backend, fix,
        "function initialize() {"
        "  Browser.print(Browser.getName() + '|' + Browser.getVersion()"
        "                + '|' + Browser.currentTime"
        "                + '|' + Browser.currentFrameRate);"
        "}");
    check(h != kInvalidScriptHandle, "T14: load Browser script");
    backend.initialize(h);
    check(fix.sai.log() == "x3d-cpp-gen|dev|5|60",
          "T14: Browser getName/getVersion/currentTime/currentFrameRate backed "
          "by SaiContext");
    backend.shutdown(h);
  }

  // end-to-end — a handler uses Browser.addRoute to wire two real nodes,
  //      and driving the source then fans out through the new route. directOutput
  //      must be TRUE for the script to add routes (§29.4.1).

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

  // SFNode handles are unforgeable — a script that clones a node
  //       handle's own properties onto a plain object (or guesses the internal
  //       key) must not obtain a node reference. A forged handle would let
  //       hostile content aim the runtime at an arbitrary address.

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

  // a script's SFNode is a real node reference (ISO/IEC 19777-1): an
  // SFNode the script writes out shares ownership with the scene (like
  //       DEF/USE), and a handle whose node the scene has since dropped
  //       resolves to null — Browser.addRoute reports INVALID_NODE instead of
  //       touching freed memory.

  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.authorFields().addAuthorField(
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

    std::any out = ctx.authorFields().getValue(script, "echo");
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
    ctx.authorFields().erase(script);
    check(weak.expired(),
          "T15c: the script's handle does not keep a dropped node alive");
    backend.invoke(h, "wire", std::any(1.0), X3DFieldType::SFTime, 2.0);
    check(sai.log() == "invalid" && ctx.graph().routeCount() == 0,
          "T15c: a handle to a destroyed node is INVALID_NODE, not a "
          "dangling pointer");
    backend.shutdown(h);
  }

  // Browser.addRoute validates like a document ROUTE (ISO/IEC 19775-2
  //       INVALID_FIELD): unknown field, wrong direction and type mismatch are
  //       rejected; set_/_changed aliases of an inputOutput field are accepted.

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

  // hostile author-field values cannot crash the host. Reading a JS
  //       global back after a handler runs can execute script code (getters,
  //       toJSON, Proxy traps, a global accessor). A throw there must be
  //       contained: no process abort, no event emitted for that field, and the
  //       script keeps working afterwards.

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
      ctx.authorFields().addAuthorField(
          script, AuthorFieldDecl{"out", c.type, AccessType::OutputOnly, {}});
      ctx.authorFields().addAuthorField(
          script, AuthorFieldDecl{"ok", X3DFieldType::SFFloat,
                                  AccessType::OutputOnly, {}});
      std::string src = std::string(c.source) +
                        "function fine(v, t) { ok = 7; }";
      ScriptHandle h = backend.load(script, src, sai);
      check(h != kInvalidScriptHandle,
            std::string("T15f: load hostile script (") + c.what + ")");
      backend.initialize(h);
      backend.invoke(h, "go", std::any(1.0), X3DFieldType::SFTime, 1.0);
      check(ctx.authorFields().getValue(script, "out").has_value() ==
                c.firstReadbackEmits,
            std::string("T15f: ") + c.what +
                " is contained (no crash; emits only a valid value)");
      backend.invoke(h, "fine", std::any(1.0), X3DFieldType::SFTime, 2.0);
      std::any ok = ctx.authorFields().getValue(script, "ok");
      check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
            std::string("T15f: script still works after ") + c.what);
      backend.shutdown(h);
      ctx.authorFields().erase(script);
    }
  }

  // updateField. An inputOutput author field written from outside the
  //       script reaches its JS global, is not echoed back as an output, and a
  //       throwing setter on that global is contained.

  {
    X3DExecutionContext ctx;
    Script script;
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.authorFields().addAuthorField(
        script, AuthorFieldDecl{"level", X3DFieldType::SFFloat,
                                AccessType::InputOutput, std::any(0.0f)});
    ctx.authorFields().addAuthorField(
        script, AuthorFieldDecl{"seen", X3DFieldType::SFFloat,
                                AccessType::OutputOnly, {}});
    ScriptHandle h = backend.load(script,
        "function poke(v, t) { seen = level; }", sai);
    check(h != kInvalidScriptHandle, "T15i: load updateField script");
    backend.initialize(h);
    ctx.authorFields().setValue(script, "level", std::any(0.5f));
    backend.updateField(h, "level", std::any(0.5f), X3DFieldType::SFFloat);
    backend.invoke(h, "poke", std::any(1.0), X3DFieldType::SFTime, 1.0);
    std::any seen = ctx.authorFields().getValue(script, "seen");
    check(seen.has_value() && std::any_cast<float>(seen) == 0.5f,
          "T15i: the script sees the updated inputOutput value");
    std::any level = ctx.authorFields().getValue(script, "level");
    check(level.has_value() && std::any_cast<float>(level) == 0.5f,
          "T15i: the update is not reverted by readback");
    backend.shutdown(h);
    ctx.authorFields().erase(script);

    Script hostile;
    SaiContext sai2(ctx, hostile, "x3d-cpp-gen", "dev");
    ctx.authorFields().addAuthorField(
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
    std::any ok = ctx.authorFields().getValue(hostile, "ok");
    check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
          "T15i: a throwing setter on update is contained");
    backend.shutdown(h2);
    ctx.authorFields().erase(hostile);
  }

  // an explicit same-value assignment to an
  //      outputOnly field still generates an event (§29.2.4 / ISO 19777-1).
  //      The ROUTE target is reset out-of-band between ticks, so a second,
  //      identical assignment re-driving it proves an event was emitted (a
  //      suppressed re-emit would leave the reset value untouched).

  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    script.setMustEvaluateUnchecked(true);
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.authorFields().addAuthorField(
        script, AuthorFieldDecl{"out", X3DFieldType::SFVec3f,
                                AccessType::OutputOnly, {}});
    Transform sink;
    sink.setTranslation(SFVec3f{0, 0, 0});
    ScriptHandle h = backend.load(
        script, "function emit_out(v, t) { out = { x: v, y: v, z: v }; }", sai);
    check(h != kInvalidScriptHandle, "T18: load re-emit script");
    backend.initialize(h);
    ctx.addRoute(FieldAddress{&script, "out"},
                 FieldAddress{&sink, "translation"});

    backend.invoke(h, "emit_out", std::any(4.0), X3DFieldType::SFTime, 1.0);
    ctx.process();
    check(sink.getTranslation().x == 4.0f && sink.getTranslation().y == 4.0f,
          "T18-pre: the first explicit outputOnly assignment emits");

    // Reset the target out-of-band, then assign the same value again.
    sink.setTranslation(SFVec3f{0, 0, 0});
    backend.invoke(h, "emit_out", std::any(4.0), X3DFieldType::SFTime, 2.0);
    ctx.process();
    check(sink.getTranslation().x == 4.0f,
          "T18: a same-value outputOnly re-assignment still generates an event");

    backend.shutdown(h);
    ctx.authorFields().erase(script);
  }

  // directOutput=FALSE — Browser.addRoute throws into JS (not a crash);
  // the route is not added.

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

  // author-field cascade — a handler writing an outputOnly field drives the
  //      cascade through a ROUTE; directOutput is not required (a script always
  //      may write its own fields, §29.2.6).

  {
    X3DExecutionContext ctx;
    Script script;  // directOutput FALSE — writing own field is still allowed
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.authorFields().addAuthorField(
        script,
        AuthorFieldDecl{"value_changed", X3DFieldType::SFVec3f,
                        AccessType::OutputOnly, {}});
    Transform target;
    target.setTranslation(SFVec3f{0, 0, 0});

    ScriptHandle h = backend.load(script,
        "function set_trigger(value, ts) {"
        "  value_changed = { x: 4, y: 5, z: 6 };"
        "}", sai);
    check(h != kInvalidScriptHandle, "T17: load outputOnly-cascade script");
    backend.initialize(h);
    // Wire the script's outputOnly field to the target's translation.
    ctx.addRoute(FieldAddress{&script, "value_changed"},
                 FieldAddress{&target, "translation"});
    backend.invoke(h, "set_trigger", std::any(SFBool(true)),
                   X3DFieldType::SFBool, 2.0);
    ctx.process();
    SFVec3f tt = target.getTranslation();
    check(nearly(tt.x, 4) && nearly(tt.y, 5) && nearly(tt.z, 6),
          "T17: handler write to outputOnly field drove the cascade via ROUTE");
    backend.shutdown(h);
    ctx.authorFields().erase(script);
  }

  // deterministic Date. Date.now() and a zero-argument `new Date()` /
  //      `Date()` read the injected execution clock (SaiContext::currentTime(),
  //      seconds -> ms); Date with explicit arguments, Date.parse/UTC and
  //      instanceof are preserved.

  {
    X3DExecutionContext ctx;
    Script script;
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.tick(12.5);  // injected clock: 12.5 s -> 12500 ms

    const char *outFields[] = {"now", "newGet", "year", "sameInstance",
                               "funcType", "parsed", "utc", "fixedDiffers",
                               "bypassCtorNow", "bypassCtorNewGet",
                               "bypassProtoNow", "dateLength", "protoCtorIsDate",
                               "funcMatches"};
    for (const char *f : outFields) {
      X3DFieldType t = X3DFieldType::SFTime;
      if (std::string(f) == "sameInstance") t = X3DFieldType::SFBool;
      if (std::string(f) == "funcType") t = X3DFieldType::SFString;
      if (std::string(f) == "fixedDiffers") t = X3DFieldType::SFBool;
      if (std::string(f) == "protoCtorIsDate") t = X3DFieldType::SFBool;
      if (std::string(f) == "funcMatches") t = X3DFieldType::SFBool;
      if (std::string(f) == "dateLength") t = X3DFieldType::SFInt32;
      ctx.authorFields().addAuthorField(
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
        "  var iso = new Date(Date.UTC(2020, 0, 1)).toISOString();"
        "  parsed = Date.parse(iso);"
        "  utc = Date.UTC(2020, 0, 1);"
        "}", sai);
    check(h != kInvalidScriptHandle, std::string(dateCase) + "load deterministic-Date script");
    backend.initialize(h);

    std::any now = ctx.authorFields().getValue(script, "now");
    std::any newGet = ctx.authorFields().getValue(script, "newGet");
    check(now.has_value() && std::any_cast<double>(now) == 12500.0,
          std::string(dateCase) + "Date.now() == injected clock in ms");
    check(newGet.has_value() && std::any_cast<double>(newGet) == 12500.0,
          std::string(dateCase) + "new Date().getTime() == injected clock in ms");
    std::any year = ctx.authorFields().getValue(script, "year");
    check(year.has_value() && std::any_cast<double>(year) == 2020.0,
          std::string(dateCase) + "new Date(2020,0,1) is unaffected (year 2020)");
    std::any differs = ctx.authorFields().getValue(script, "fixedDiffers");
    check(differs.has_value() && std::any_cast<bool>(differs),
          std::string(dateCase) + "explicit-argument Date is not the injected clock");
    std::any inst = ctx.authorFields().getValue(script, "sameInstance");
    check(inst.has_value() && std::any_cast<bool>(inst),
          std::string(dateCase) + "new Date() instanceof Date holds");
    std::any ft = ctx.authorFields().getValue(script, "funcType");
    check(ft.has_value() && std::any_cast<std::string>(ft) == "string",
          std::string(dateCase) + "Date() as a function still returns a string");
    std::any parsed = ctx.authorFields().getValue(script, "parsed");
    std::any utc = ctx.authorFields().getValue(script, "utc");
    check(parsed.has_value() && utc.has_value() &&
              std::any_cast<double>(parsed) == std::any_cast<double>(utc),
          std::string(dateCase) + "Date.parse / Date.UTC still work");

    // Bypass paths (review (a)): all must reach the injected clock, not the
    // wall clock.
    std::any b1 = ctx.authorFields().getValue(script, "bypassCtorNow");
    std::any b2 = ctx.authorFields().getValue(script, "bypassCtorNewGet");
    std::any b3 = ctx.authorFields().getValue(script, "bypassProtoNow");
    check(b1.has_value() && std::any_cast<double>(b1) == 12500.0,
          std::string(dateCase) + "(new Date()).constructor.now() == injected clock");
    check(b2.has_value() && std::any_cast<double>(b2) == 12500.0,
          std::string(dateCase) + "new (Date.prototype.constructor)() == injected clock");
    check(b3.has_value() && std::any_cast<double>(b3) == 12500.0,
          std::string(dateCase) + "Object.getPrototypeOf(new Date()).constructor.now() == clock");
    // Semantics (review (b)).
    std::any len = ctx.authorFields().getValue(script, "dateLength");
    check(len.has_value() && std::any_cast<SFInt32>(len) == 7,
          std::string(dateCase) + "Date.length === 7");
    std::any pc = ctx.authorFields().getValue(script, "protoCtorIsDate");
    check(pc.has_value() && std::any_cast<bool>(pc),
          std::string(dateCase) + "Date.prototype.constructor === Date");
    std::any fm = ctx.authorFields().getValue(script, "funcMatches");
    check(fm.has_value() && std::any_cast<bool>(fm),
          std::string(dateCase) + "Date() returns the injected time's string");

    backend.shutdown(h);
    ctx.authorFields().erase(script);
  }

  // Field-type objects (ISO/IEC 19777-1 clause 7): constructors, methods and
  // index access, on values the script builds and on values it receives.
  {
    Fixture fix;
    auto out = [&](const char *name, X3DFieldType t) {
      fix.ctx.authorFields().addAuthorField(
          fix.script, AuthorFieldDecl{name, t, AccessType::OutputOnly, {}});
    };
    out("sum", X3DFieldType::SFVec3f);
    out("crossed", X3DFieldType::SFVec3f);
    out("len", X3DFieldType::SFFloat);
    out("indexed", X3DFieldType::SFFloat);
    out("text", X3DFieldType::SFString);
    out("turned", X3DFieldType::SFVec3f);
    out("fromTo", X3DFieldType::SFRotation);
    out("composed", X3DFieldType::SFVec3f);
    out("half", X3DFieldType::SFRotation);
    out("hsv", X3DFieldType::SFColor);
    out("points", X3DFieldType::MFVec3f);
    out("names", X3DFieldType::MFString);
    out("v2", X3DFieldType::SFVec2f);
    out("rgba", X3DFieldType::SFColorRGBA);
    out("received", X3DFieldType::SFVec3f);
    out("receivedIsVec", X3DFieldType::SFBool);
    ScriptHandle h = backend.load(fix.script, R"JS(
      function initialize() {
        var a = new SFVec3f(1, 2, 3), b = new SFVec3f(4, 5, 6);
        sum = a.add(b).subtract(new SFVec3f(1, 1, 1)).multiply(2).divide(4);
        crossed = new SFVec3f(1, 0, 0).cross(new SFVec3f(0, 1, 0));
        len = new SFVec3f(3, 4, 0).length() + new SFVec3f(0, 0, 9).normalize().z;
        a[1] = 20;
        indexed = a[0] + a[1] + a[2] + a.dot(new SFVec3f(1, 0, 0));
        text = String(new SFVec3f(1, 2.5, -3)) + '|' +
               new SFRotation(0, 1, 0, 0.5).toString() + '|' + new SFColor(1, 0, 0);
        var quarter = new SFRotation(0, 0, 1, Math.PI / 2);
        turned = quarter.multVec(new SFVec3f(1, 0, 0));
        fromTo = new SFRotation(new SFVec3f(1, 0, 0), new SFVec3f(0, 0, -1));
        // a.multiply(b): a first, then b. X to Y (about Z), then Y to Z (about X).
        composed = quarter.multiply(new SFRotation(1, 0, 0, Math.PI / 2))
                       .multVec(new SFVec3f(1, 0, 0));
        half = new SFRotation(0, 1, 0, 0).slerp(new SFRotation(0, 1, 0, 1), 0.5)
                   .multiply(new SFRotation(new SFVec3f(0, 1, 0), 0.25).inverse());
        var c = new SFColor();
        c.setHSV(120, 1, 0.5);
        var back = c.getHSV();
        hsv = new SFColor(c.r + back[0] / 1200, c.g, c.b);
        points = new MFVec3f(new SFVec3f(1, 2, 3), new SFVec3f(4, 5, 6));
        points[2] = new SFVec3f(7, 8, 9);
        names = new MFString('a', 'b');
        v2 = new SFVec2f(1, 2).add(new SFVec2f(3, 4)).negate();
        rgba = new SFColorRGBA(0.1, 0.2, 0.3, 0.4);
      }
      function set_in(value, ts) {
        receivedIsVec = typeof value.add === 'function' && value[2] === value.z;
        received = value.add(new SFVec3f(1, 1, 1));
      }
    )JS", fix.sai);
    check(h != kInvalidScriptHandle, "field types: script using constructors loads");
    if (h != kInvalidScriptHandle) {
      backend.initialize(h);
      auto &store = fix.ctx.authorFields();
      auto vec = [&](const char *n) {
        std::any v = store.getValue(fix.script, n);
        return v.has_value() ? std::any_cast<SFVec3f>(v) : SFVec3f{-99, -99, -99};
      };
      auto rot = [&](const char *n) {
        std::any v = store.getValue(fix.script, n);
        return v.has_value() ? std::any_cast<SFRotation>(v) : SFRotation{0, 0, 0, -99};
      };
      auto flt = [&](const char *n) {
        std::any v = store.getValue(fix.script, n);
        return v.has_value() ? std::any_cast<SFFloat>(v) : -99.0f;
      };
      SFVec3f s3 = vec("sum");
      check(nearly(s3.x, 2) && nearly(s3.y, 3) && nearly(s3.z, 4),
            "field types: SFVec3f add/subtract/multiply/divide");
      SFVec3f cr = vec("crossed");
      check(nearly(cr.x, 0) && nearly(cr.y, 0) && nearly(cr.z, 1),
            "field types: SFVec3f cross");
      check(nearly(flt("len"), 6), "field types: SFVec3f length/normalize");
      check(nearly(flt("indexed"), 25), "field types: index read/write and dot");
      std::any txt = store.getValue(fix.script, "text");
      check(txt.has_value() &&
                std::any_cast<SFString>(txt) == "1 2.5 -3|0 1 0 0.5|1 0 0",
            "field types: toString");
      SFVec3f tu = vec("turned");
      check(nearly(tu.x, 0) && nearly(tu.y, 1) && nearly(tu.z, 0),
            "field types: SFRotation multVec");
      SFRotation ft = rot("fromTo");
      check(nearly(ft.x, 0) && nearly(ft.y, 1) && nearly(ft.z, 0) &&
                nearly(ft.angle, M_PI / 2),
            "field types: SFRotation(from, to)");
      SFVec3f co = vec("composed");
      check(nearly(co.x, 0) && nearly(co.y, 0) && nearly(co.z, 1),
            "field types: SFRotation multiply applies this first");
      SFRotation hf = rot("half");
      check(nearly(std::fabs(hf.y), 1) && nearly(hf.angle * hf.y, 0.25),
            "field types: SFRotation slerp, axis constructor, inverse");
      std::any hv = store.getValue(fix.script, "hsv");
      check(hv.has_value() && nearly(std::any_cast<SFColor>(hv).r, 0.1) &&
                nearly(std::any_cast<SFColor>(hv).g, 0.5) &&
                nearly(std::any_cast<SFColor>(hv).b, 0),
            "field types: SFColor setHSV/getHSV");
      std::any pts = store.getValue(fix.script, "points");
      check(pts.has_value() && std::any_cast<MFVec3f>(pts).size() == 3 &&
                nearly(std::any_cast<MFVec3f>(pts)[2].y, 8),
            "field types: MFVec3f constructor and index growth");
      std::any nm = store.getValue(fix.script, "names");
      check(nm.has_value() && std::any_cast<MFString>(nm).size() == 2 &&
                std::any_cast<MFString>(nm)[1] == "b",
            "field types: MFString constructor");
      std::any v2 = store.getValue(fix.script, "v2");
      check(v2.has_value() && nearly(std::any_cast<SFVec2f>(v2).x, -4) &&
                nearly(std::any_cast<SFVec2f>(v2).y, -6),
            "field types: SFVec2f");
      std::any ca = store.getValue(fix.script, "rgba");
      check(ca.has_value() && nearly(std::any_cast<SFColorRGBA>(ca).a, 0.4),
            "field types: SFColorRGBA");

      backend.invoke(h, "set_in", std::any(SFVec3f{1, 2, 3}),
                     X3DFieldType::SFVec3f, 0.0);
      std::any isVec = store.getValue(fix.script, "receivedIsVec");
      check(isVec.has_value() && std::any_cast<bool>(isVec),
            "field types: a received SFVec3f has methods and index access");
      SFVec3f rc = vec("received");
      check(nearly(rc.x, 2) && nearly(rc.y, 3) && nearly(rc.z, 4),
            "field types: methods on a received value");
      backend.shutdown(h);
    }
    fix.ctx.authorFields().erase(fix.script);
  }

  // A script that replaces a field-type global cannot change the prototype
  // given to values the runtime pushes.
  {
    Fixture fix;
    fix.ctx.authorFields().addAuthorField(
        fix.script, AuthorFieldDecl{"ok", X3DFieldType::SFBool,
                                    AccessType::OutputOnly, {}});
    ScriptHandle h = backend.load(fix.script,
        "SFVec3f = function () {};\n"
        "function set_in(value, ts) { ok = typeof value.cross === 'function'; }",
        fix.sai);
    check(h != kInvalidScriptHandle, "field types: overriding a global loads");
    if (h != kInvalidScriptHandle) {
      backend.initialize(h);
      backend.invoke(h, "set_in", std::any(SFVec3f{1, 2, 3}),
                     X3DFieldType::SFVec3f, 0.0);
      std::any ok = fix.ctx.authorFields().getValue(fix.script, "ok");
      check(ok.has_value() && std::any_cast<bool>(ok),
            "field types: pushed values keep the built-in prototype");
      backend.shutdown(h);
    }
    fix.ctx.authorFields().erase(fix.script);
  }

  // SFNode field access: a node handle exposes the node's fields as
  // properties, under the directOutput gate.
  {
    auto run = [&](bool directOutput) {
      Fixture fix;
      fix.script.setDirectOutputUnchecked(directOutput);
      auto target = std::make_shared<Transform>();
      target->setTranslation(SFVec3f{1, 2, 3});
      auto &store = fix.ctx.authorFields();
      store.addAuthorField(fix.script, AuthorFieldDecl{
          "target", X3DFieldType::SFNode, AccessType::InitializeOnly,
          std::any(SFNode(target))});
      auto out = [&](const char *name, X3DFieldType t) {
        store.addAuthorField(
            fix.script, AuthorFieldDecl{name, t, AccessType::OutputOnly, {}});
      };
      out("readBack", X3DFieldType::SFVec3f);
      out("unknownIsUndefined", X3DFieldType::SFBool);
      out("writeError", X3DFieldType::SFString);
      out("badFieldThrew", X3DFieldType::SFBool);
      ScriptHandle h = backend.load(fix.script, R"JS(
        function initialize() {
          readBack = target.translation.add(new SFVec3f(1, 1, 1));
          unknownIsUndefined = target.noSuchField === undefined;
          writeError = '';
          try {
            target.translation = new SFVec3f(7, 8, 9);
            target.set_scale = new SFVec3f(2, 2, 2);
          } catch (e) { writeError = String(e); }
          badFieldThrew = false;
          try { target.noSuchField = 1; } catch (e) { badFieldThrew = true; }
        }
      )JS", fix.sai);
      const std::string tag =
          directOutput ? "node fields (directOutput): " : "node fields (gated): ";
      check(h != kInvalidScriptHandle, tag + "script loads");
      if (h == kInvalidScriptHandle) return;
      backend.initialize(h);
      std::any rb = store.getValue(fix.script, "readBack");
      check(rb.has_value() && nearly(std::any_cast<SFVec3f>(rb).x, 2) &&
                nearly(std::any_cast<SFVec3f>(rb).z, 4),
            tag + "node.translation reads the field, with methods");
      std::any un = store.getValue(fix.script, "unknownIsUndefined");
      check(un.has_value() && std::any_cast<bool>(un),
            tag + "an unknown property reads as undefined");
      std::any bad = store.getValue(fix.script, "badFieldThrew");
      check(bad.has_value() && std::any_cast<bool>(bad),
            tag + "writing an unknown field throws");
      std::any we = store.getValue(fix.script, "writeError");
      const std::string err = we.has_value() ? std::any_cast<SFString>(we) : "?";
      if (directOutput) {
        check(err.empty(), tag + "writes succeed");
        check(nearly(target->getTranslation().x, 7) &&
                  nearly(target->getTranslation().z, 9),
              tag + "node.translation = v writes the field");
        check(nearly(target->getScale().x, 2),
              tag + "the set_ alias writes the inputOutput field");
      } else {
        check(err.find("directOutput") != std::string::npos,
              tag + "a cross-node write throws");
        check(nearly(target->getTranslation().x, 1),
              tag + "the gated write changed nothing");
      }
      backend.shutdown(h);
      store.erase(fix.script);
    };
    run(true);
    run(false);
  }

  // Browser.createX3DFromString / createVrmlFromString: the host's scene
  // factory builds the nodes; the script inserts them through a node field.
  {
    Fixture fix;
    fix.script.setDirectOutputUnchecked(true);
    std::string seenText;
    std::vector<std::string> seenSources;
    fix.sai.setSceneFactory([&](const std::string &text) {
      seenText = text;
      seenSources.push_back(text);
      if (text == "bad") throw std::runtime_error("parse error");
      auto made = std::make_shared<Transform>();
      made->setTranslation(SFVec3f{4, 5, 6});
      return MFNode{made};
    });
    auto group = std::make_shared<Transform>();
    auto &store = fix.ctx.authorFields();
    store.addAuthorField(fix.script, AuthorFieldDecl{
        "group", X3DFieldType::SFNode, AccessType::InitializeOnly,
        std::any(SFNode(group))});
    auto out = [&](const char *name, X3DFieldType t) {
      store.addAuthorField(
          fix.script, AuthorFieldDecl{name, t, AccessType::OutputOnly, {}});
    };
    out("count", X3DFieldType::SFInt32);
    out("sameViaRootNodes", X3DFieldType::SFBool);
    out("madeX", X3DFieldType::SFFloat);
    out("badThrew", X3DFieldType::SFBool);
    out("json", X3DFieldType::SFString);
    ScriptHandle h = backend.load(fix.script, R"JS(
      function initialize() {
        var scene = Browser.createX3DFromString('<Transform/>');
        count = scene.length;
        sameViaRootNodes = scene.rootNodes.length === 1 &&
                           scene.rootNodes[0].translation.x === scene[0].translation.x;
        madeX = scene[0].translation.x;
        json = '' + scene[0] + JSON.stringify(scene);
        group.children = scene.rootNodes;
        Browser.createVrmlFromString('<Transform/>\u0000<Shape/>');
        badThrew = false;
        try { Browser.createVrmlFromString('bad'); } catch (e) { badThrew = true; }
      }
    )JS",
                                  fix.sai);
    check(h != kInvalidScriptHandle, "create: script loads");
    if (h != kInvalidScriptHandle) {
      backend.initialize(h);
      std::any c = store.getValue(fix.script, "count");
      check(c.has_value() && std::any_cast<SFInt32>(c) == 1,
            "create: createX3DFromString returns the created nodes");
      std::any same = store.getValue(fix.script, "sameViaRootNodes");
      check(same.has_value() && std::any_cast<bool>(same),
            "create: the result is also reachable as .rootNodes");
      std::any mx = store.getValue(fix.script, "madeX");
      check(mx.has_value() && nearly(std::any_cast<SFFloat>(mx), 4),
            "create: a created node's fields are readable");
      check(group->getChildren().size() == 1 &&
                nearly(std::dynamic_pointer_cast<Transform>(group->getChildren()[0])
                           ? std::dynamic_pointer_cast<Transform>(
                                 group->getChildren()[0])->getTranslation().y
                           : 0, 5),
            "create: the script inserted the created node into the scene");
      std::any js = store.getValue(fix.script, "json");
      check(js.has_value() &&
                std::any_cast<SFString>(js) == "[SFNode Transform][{}]",
            "create: a node has a string form and the result stringifies");
      std::any bad = store.getValue(fix.script, "badThrew");
      check(bad.has_value() && std::any_cast<bool>(bad) && seenText == "bad",
            "create: a factory error becomes a catchable script error");
      check(seenSources.size() == 3 &&
                seenSources[1] ==
                    std::string("<Transform/>") + '\0' + "<Shape/>",
            "create: the factory receives the full string, including embedded "
            "NUL");
      backend.shutdown(h);
    }
    store.erase(fix.script);
  }

  // With no scene factory the call throws instead of returning nothing.
  {
    Fixture fix;
    fix.ctx.authorFields().addAuthorField(
        fix.script, AuthorFieldDecl{"threw", X3DFieldType::SFBool,
                                    AccessType::OutputOnly, {}});
    ScriptHandle h = backend.load(fix.script,
        "function initialize() { threw = false;"
        " try { Browser.createX3DFromString('<Group/>'); } catch (e) { threw = true; } }",
        fix.sai);
    check(h != kInvalidScriptHandle, "create: no-factory script loads");
    if (h != kInvalidScriptHandle) {
      backend.initialize(h);
      std::any t = fix.ctx.authorFields().getValue(fix.script, "threw");
      check(t.has_value() && std::any_cast<bool>(t),
            "create: without a scene factory the call throws");
      backend.shutdown(h);
    }
    fix.ctx.authorFields().erase(fix.script);
  }

  // Allocating the returned node handles must either finish or throw; a heap
  // limit must never turn a successful factory result into a partial array.
  {
    backend.setMemoryLimit(std::size_t{1} << 20);
    backend.setCallBudget(std::chrono::seconds(60));
    Fixture fix;
    fix.sai.setSceneFactory([](const std::string &) {
      return MFNode(32768, std::make_shared<Transform>());
    });
    auto &store = fix.ctx.authorFields();
    store.addAuthorField(fix.script, AuthorFieldDecl{"caught",
                                                     X3DFieldType::SFBool,
                                                     AccessType::OutputOnly,
                                                     {}});
    store.addAuthorField(fix.script, AuthorFieldDecl{"alive",
                                                     X3DFieldType::SFBool,
                                                     AccessType::OutputOnly,
                                                     {}});
    ScriptHandle h =
        backend.load(fix.script,
                     "function initialize() { caught = false;"
                     " try { Browser.createX3DFromString('<Transform/>'); }"
                     " catch (e) { caught = true; } }"
                     "function fine(v, t) { alive = true; }",
                     fix.sai);
    check(h != kInvalidScriptHandle, "create: limited-heap script loads");
    if (h != kInvalidScriptHandle) {
      backend.initialize(h);
      std::any caught = store.getValue(fix.script, "caught");
      check(caught.has_value() && std::any_cast<bool>(caught),
            "create: exhausting memory while returning nodes throws");
      backend.invoke(h, "fine", std::any{}, X3DFieldType::SFBool, 0.0);
      std::any alive = store.getValue(fix.script, "alive");
      check(alive.has_value() && std::any_cast<bool>(alive),
            "create: the script recovers after node conversion runs out of "
            "memory");
      backend.shutdown(h);
    }
    store.erase(fix.script);
    backend.setMemoryLimit(ScriptEngine::kDefaultMemoryLimit);
    backend.setCallBudget(ScriptEngine::kDefaultCallBudget);
  }
}

} // namespace script_backend_tests
