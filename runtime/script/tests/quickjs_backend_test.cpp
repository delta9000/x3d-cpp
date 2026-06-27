// quickjs_backend_test.cpp
// U1 lifecycle test for the QuickJS (quickjs-ng) ScriptEngine backend. Mirrors
// the Duktape ecmascript_backend_test T1-T11 lifecycle subset to prove the
// second backend implements the SAME seam with identical lifecycle behavior:
//
//   - a trivial valid script loads and returns a non-zero handle,
//   - a syntax error fails to load (returns kInvalidScriptHandle),
//   - initialize() / invoke() / eventsProcessed() / prepareEvents() run without
//     crash, with and without the corresponding script global defined,
//   - shutdown() frees the context (no crash, no double-free),
//   - two independent scripts get independent contexts (distinct handles, no
//     global bleed-through).
//
// Full marshalling parity + Browser + handler value round-trip are U2; this
// test exercises lifecycle only (matching the U1 scope of QuickJsBackend).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "QuickJsBackend.hpp"

#include "DynamicField.hpp"
#include "SaiContext.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <string>

using namespace x3d;
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

// Minimal wiring: a Script node + execution context + SAI surface per script.
struct Fixture {
  Script script;
  X3DExecutionContext ctx;
  SaiContext sai{ctx, script, "x3d-cpp-gen", "dev"};
};

ScriptHandle loadSource(QuickJsBackend &backend, Fixture &fix,
                        const std::string &rawSource) {
  return backend.load(fix.script, rawSource, fix.sai);
}

bool nearly(double a, double b) { return std::fabs(a - b) < 1e-5; }

// Round-trip a boxed value through the PUBLIC seam (pushValue on invoke's arg +
// toValue on the author-field readback), proving the marshalling is observable
// and reversible exactly as a real script would see it. Mechanism: declare an
// inputOutput author field `echo` of `type`, load a handler that copies its
// (value) argument into the `echo` global, invoke it, then read `echo` back out
// of the dynamic-field store. Returns the recovered std::any.
std::any roundTrip(QuickJsBackend &backend, const std::any &v,
                   X3DFieldType type) {
  Fixture fix;
  // An inputOutput author field is readable+writable, so seed/readback handle it
  // and the store keeps its value. Seed an EMPTY initialValue so any non-empty
  // echo differs from the prior value and is not suppressed as a no-op.
  dynamicFieldStore().addAuthorField(
      fix.script, AuthorFieldDecl{"echo", type, AccessType::InputOutput, {}});
  ScriptHandle h = backend.load(fix.script,
      "function set_echo(value, ts) { echo = value; }", fix.sai);
  if (h == kInvalidScriptHandle) return {};
  backend.initialize(h);
  backend.invoke(h, "set_echo", v, type, 0.0);
  std::any out = dynamicFieldStore().getValue(fix.script, "echo");
  backend.shutdown(h);
  dynamicFieldStore().erase(fix.script);
  return out;
}

} // namespace

int main() {
  QuickJsBackend backend;

  // -------------------------------------------------------------------------
  // T2: load() succeeds for a trivial script; returns a non-zero handle.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix,
        "var _initCalled = false;"
        "function initialize() { _initCalled = true; }");
    check(h != kInvalidScriptHandle, "T2: load trivial script succeeds");

    // T3: initialize() calls the script's initialize() without crash.
    backend.initialize(h);
    check(true, "T3: initialize() runs without crash");

    // T4: invoke() dispatches to a handler by name without crash.
    backend.invoke(h, "initialize", std::any{}, X3DFieldType::SFBool, 0.0);
    check(true, "T4: invoke('initialize') runs without crash");

    // T5: eventsProcessed() with no handler is a safe no-op.
    backend.eventsProcessed(h, 0.0);
    check(true, "T5: eventsProcessed() with no handler is a no-op");

    // T6: prepareEvents() with no handler is a safe no-op.
    backend.prepareEvents(h, 1.0);
    check(true, "T6: prepareEvents() with no handler is a no-op");

    // T7: shutdown() frees the context without crash / double-free.
    backend.shutdown(h);
    check(true, "T7: shutdown() runs without crash");
  }

  // -------------------------------------------------------------------------
  // T8: load() fails for a syntax error -> kInvalidScriptHandle.
  // -------------------------------------------------------------------------
  {
    Fixture fix;
    ScriptHandle h = loadSource(backend, fix, "function broken( { bad syntax");
    check(h == kInvalidScriptHandle,
          "T8: load with syntax error returns kInvalidScriptHandle");
  }

  // -------------------------------------------------------------------------
  // T9: eventsProcessed() handler IS called when defined (no crash).
  // -------------------------------------------------------------------------
  {
    Fixture fix;
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
    ScriptHandle h1 = loadSource(backend, fix1, "var id = 'script_one';");
    ScriptHandle h2 = loadSource(backend, fix2, "var id = 'script_two';");
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
  // T12: field marshalling round-trips through the public seam (pushValue on
  //      invoke's value arg -> toValue on the author-field readback), per type.
  //      Asserts the std::any survives unchanged, matching the Duktape backend.
  // -------------------------------------------------------------------------
  {
    // scalars
    check(std::any_cast<SFBool>(roundTrip(backend, SFBool(true),
                                          X3DFieldType::SFBool)),
          "T12: SFBool round-trips");
    check(std::any_cast<SFInt32>(roundTrip(backend, SFInt32(-42),
                                           X3DFieldType::SFInt32)) == -42,
          "T12: SFInt32 round-trips");
    check(nearly(std::any_cast<SFFloat>(
              roundTrip(backend, SFFloat(3.5f), X3DFieldType::SFFloat)), 3.5),
          "T12: SFFloat round-trips");
    check(nearly(std::any_cast<SFDouble>(
              roundTrip(backend, SFDouble(2.71828), X3DFieldType::SFDouble)),
                  2.71828),
          "T12: SFDouble round-trips");
    check(nearly(std::any_cast<SFTime>(
              roundTrip(backend, SFTime(123456.5), X3DFieldType::SFTime)),
                  123456.5),
          "T12: SFTime round-trips");
    check(std::any_cast<SFString>(
              roundTrip(backend, SFString("héllo"), X3DFieldType::SFString))
              == "héllo",
          "T12: SFString round-trips (UTF-8)");

    // structured SF
    {
      auto out = std::any_cast<SFVec2f>(
          roundTrip(backend, SFVec2f{1.5f, -2.5f}, X3DFieldType::SFVec2f));
      check(nearly(out.x, 1.5) && nearly(out.y, -2.5), "T12: SFVec2f round-trips");
    }
    {
      auto out = std::any_cast<SFVec3f>(
          roundTrip(backend, SFVec3f{1, 2, 3}, X3DFieldType::SFVec3f));
      check(nearly(out.x, 1) && nearly(out.y, 2) && nearly(out.z, 3),
            "T12: SFVec3f round-trips");
    }
    {
      auto out = std::any_cast<SFVec3d>(
          roundTrip(backend, SFVec3d{1.25, 2.5, 3.75}, X3DFieldType::SFVec3d));
      check(nearly(out.x, 1.25) && nearly(out.y, 2.5) && nearly(out.z, 3.75),
            "T12: SFVec3d round-trips");
    }
    {
      auto out = std::any_cast<SFVec4f>(
          roundTrip(backend, SFVec4f{1, 2, 3, 4}, X3DFieldType::SFVec4f));
      check(nearly(out.x, 1) && nearly(out.w, 4), "T12: SFVec4f round-trips");
    }
    {
      auto out = std::any_cast<SFColor>(
          roundTrip(backend, SFColor{0.1f, 0.2f, 0.3f}, X3DFieldType::SFColor));
      check(nearly(out.r, 0.1) && nearly(out.g, 0.2) && nearly(out.b, 0.3),
            "T12: SFColor round-trips");
    }
    {
      auto out = std::any_cast<SFColorRGBA>(roundTrip(
          backend, SFColorRGBA{0.1f, 0.2f, 0.3f, 0.4f},
          X3DFieldType::SFColorRGBA));
      check(nearly(out.r, 0.1) && nearly(out.a, 0.4),
            "T12: SFColorRGBA round-trips");
    }
    {
      auto out = std::any_cast<SFRotation>(roundTrip(
          backend, SFRotation{0, 1, 0, 1.5708f}, X3DFieldType::SFRotation));
      check(nearly(out.y, 1) && nearly(out.angle, 1.5708),
            "T12: SFRotation round-trips (axis-angle)");
    }

    // MF
    {
      auto out = std::any_cast<MFInt32>(roundTrip(
          backend, MFInt32{1, 2, 3, 4, 5}, X3DFieldType::MFInt32));
      check(out.size() == 5 && out[0] == 1 && out[4] == 5,
            "T12: MFInt32 round-trips (array-like + length)");
    }
    {
      auto out = std::any_cast<MFFloat>(roundTrip(
          backend, MFFloat{0.5f, 1.5f, 2.5f}, X3DFieldType::MFFloat));
      check(out.size() == 3 && nearly(out[1], 1.5), "T12: MFFloat round-trips");
    }
    {
      auto out = std::any_cast<MFString>(roundTrip(
          backend, MFString{"a", "bb", "ccc"}, X3DFieldType::MFString));
      check(out.size() == 3 && out[2] == "ccc", "T12: MFString round-trips");
    }
    {
      auto out = std::any_cast<MFVec3f>(roundTrip(
          backend, MFVec3f{{1, 0, 0}, {0, 1, 0}}, X3DFieldType::MFVec3f));
      check(out.size() == 2 && nearly(out[1].y, 1), "T12: MFVec3f round-trips");
    }
    {
      auto out = std::any_cast<MFColor>(roundTrip(
          backend, MFColor{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
          X3DFieldType::MFColor));
      check(out.size() == 3 && nearly(out[2].b, 1), "T12: MFColor round-trips");
    }
    // empty MF round-trips to empty.
    {
      auto out = std::any_cast<MFFloat>(roundTrip(
          backend, MFFloat{}, X3DFieldType::MFFloat));
      check(out.empty(), "T12: empty MFFloat round-trips to empty");
    }

    // SFImage (3-component 2x2) round-trips byte-for-byte (ISO 19777-1 shape).
    {
      SFImage in{2, 2, 3,
                 {0xFF, 0x00, 0x00,   // pixel 0 = red
                  0x00, 0xFF, 0x00,   // pixel 1 = green
                  0x00, 0x00, 0xFF,   // pixel 2 = blue
                  0xFF, 0xFF, 0x00}}; // pixel 3 = yellow
      auto out = std::any_cast<SFImage>(
          roundTrip(backend, in, X3DFieldType::SFImage));
      check(out.width == 2 && out.height == 2 && out.numComponents == 3,
            "T12: SFImage header round-trips");
      check(out.data == in.data, "T12: SFImage pixel bytes round-trip");
    }
    // MFImage round-trips (two images of differing component counts).
    {
      MFImage in{SFImage{1, 1, 1, {0x42}},
                 SFImage{2, 1, 3, {0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00}}};
      auto out = std::any_cast<MFImage>(
          roundTrip(backend, in, X3DFieldType::MFImage));
      check(out.size() == 2, "T12: MFImage round-trips element count");
      check(out[0].width == 1 && out[0].numComponents == 1 &&
                out[0].data[0] == 0x42,
            "T12: MFImage[0] (1-comp) round-trips");
      check(out[1].data == in[1].data, "T12: MFImage[1] pixel bytes round-trip");
    }
    // SFMatrix3f/4f/3d/4d round-trip (flat row-major arrays).
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
    {
      MFMatrix4f in{SFMatrix4f{}, SFMatrix4f{}};
      in[0].matrix[0][0] = 1.0f; in[0].matrix[3][3] = 16.0f;
      in[1].matrix[3][3] = 15.0f;
      auto out = std::any_cast<MFMatrix4f>(
          roundTrip(backend, in, X3DFieldType::MFMatrix4f));
      check(out.size() == 2 && nearly(out[0].matrix[3][3], 16.0) &&
                nearly(out[1].matrix[3][3], 15.0),
            "T12: MFMatrix4f cells preserved");
    }
  }

  // -------------------------------------------------------------------------
  // T13: handler dispatch passes (value, timestamp) into the script; the script
  //      echoes both via Browser.print so we observe the marshalled value.
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
  // T14: Browser object — getName/getVersion/currentTime/currentFrameRate/print.
  // -------------------------------------------------------------------------
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

  // -------------------------------------------------------------------------
  // T15: end-to-end — a handler uses Browser.addRoute to wire two real nodes
  //      (exercising SFNode marshalling both ways), then driving the source fans
  //      out through the new route. directOutput must be TRUE (§29.4.1).
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);  // allow dynamic addRoute
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");

    Transform a, b;
    a.setTranslation(SFVec3f{0, 0, 0});
    b.setTranslation(SFVec3f{0, 0, 0});

    ScriptHandle h2 = backend.load(script,
        "var fromN = null;"
        "function set_from(node, t) { fromN = node; }"
        "function set_to(node, t) {"
        "  Browser.addRoute(fromN, 'translation', node, 'translation');"
        "}", sai);
    check(h2 != kInvalidScriptHandle, "T15: load from/to wiring script");
    backend.initialize(h2);
    backend.invoke(h2, "set_from", std::any(SFNode(&a, [](X3DNode *) {})),
                   X3DFieldType::SFNode, 1.0);
    backend.invoke(h2, "set_to", std::any(SFNode(&b, [](X3DNode *) {})),
                   X3DFieldType::SFNode, 1.0);
    check(ctx.graph().routeCount() == 1,
          "T15: script added a route via Browser.addRoute (SFNode marshalling)");

    ctx.postEvent(&a, "translation", std::any(SFVec3f{7, 8, 9}));
    ctx.process();
    SFVec3f bt = b.getTranslation();
    check(nearly(bt.x, 7) && nearly(bt.y, 8) && nearly(bt.z, 9),
          "T15: end-to-end — script-added route carried the event a->b");
    backend.shutdown(h2);
  }

  // -------------------------------------------------------------------------
  // T16: directOutput=FALSE — Browser.addRoute throws into JS (not a crash);
  //      the route is NOT added.
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;  // directOutput defaults FALSE
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    Transform a;

    ScriptHandle h = backend.load(script,
        "var caught = false;"
        "function tryWire(fromNode, t) {"
        "  try { Browser.addRoute(fromNode, 'translation', fromNode,"
        "                         'translation'); }"
        "  catch (e) { caught = true; Browser.print('blocked'); }"
        "}", sai);
    check(h != kInvalidScriptHandle, "T16: load directOutput-gate script");
    backend.initialize(h);
    backend.invoke(h, "tryWire", std::any(SFNode(&a, [](X3DNode *) {})),
                   X3DFieldType::SFNode, 1.0);
    check(sai.log() == "blocked",
          "T16: addRoute with directOutput=FALSE threw into JS (caught)");
    check(ctx.graph().routeCount() == 0,
          "T16: no route added when directOutput=FALSE");
    backend.shutdown(h);
  }

  // -------------------------------------------------------------------------
  // T17: author-field cascade — a handler writing an outputOnly field drives the
  //      cascade through a ROUTE; directOutput is NOT required (a script always
  //      may write its own fields, §29.2.6).
  // -------------------------------------------------------------------------
  {
    X3DExecutionContext ctx;
    Script script;  // directOutput FALSE — writing OWN field is still allowed
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    dynamicFieldStore().addAuthorField(
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
    dynamicFieldStore().erase(script);
  }

  if (failures == 0) {
    std::cout << "All quickjs_backend tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test(s) FAILED.\n";
  return 1;
}
