// Duktape-specific ECMAScript backend checks.
#include "EcmaScriptBackend.hpp"
#include "ScriptBackendTestCases.hpp"

#include <chrono>
#include <cstddef>

using namespace script_backend_tests;

int main() {
  EcmaScriptBackend backend;
  runSharedCases(backend, "T17: ");

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
    // A script-built SFImage with a negative dimension or an out-of-range
    // comp decodes to an empty image instead of a wrapped size.
    duk_eval_string(c, "({x:-1, y:1, comp:9, array:[1]})");
    auto bad = std::any_cast<SFImage>(EcmaScriptBackend::toValue(c, -1, X3DFieldType::SFImage));
    check(bad.width == 0 && bad.numComponents == 4 && bad.data.empty(),
          "T12: SFImage with a negative width decodes empty");
    duk_pop(c);
    duk_destroy_heap(c);
  }

  // call budget. A handler (or the script's top level) that never
  //       returns is interrupted after callBudget() instead of hanging the
  //       host -- even if it tries to catch the interruption and keep going --
  //       and the script keeps working afterwards.

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
      ctx.authorFields().addAuthorField(
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
      std::any ok = ctx.authorFields().getValue(script, "ok");
      check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
            std::string("T15g: script still works after ") + c.what);
      backend.shutdown(h);
      ctx.authorFields().erase(script);
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

  // memory limit. A handler that allocates without bound is stopped by
  //       memoryLimit() -- well inside a generous call budget, so the memory
  //       cap, not the clock, ends it -- and the script keeps working. The
  //       check is causal, not a speed bound: the call must end well before
  //       the budget could have ended it. Sanitizer builds spend seconds in
  //       garbage collection near the cap, so a fixed wall-clock bound flakes.

  {
    using Clock = std::chrono::steady_clock;
    const auto budget = std::chrono::seconds(60);
    backend.setCallBudget(budget);
    backend.setMemoryLimit(std::size_t{16} << 20);

    X3DExecutionContext ctx;
    Script script;
    SaiContext sai(ctx, script, "x3d-cpp-gen", "dev");
    ctx.authorFields().addAuthorField(
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
    check(Clock::now() - start < budget / 2,
          "T15h: unbounded allocation is stopped by the memory limit");
    backend.invoke(h, "fine", std::any(1.0), X3DFieldType::SFTime, 2.0);
    std::any ok = ctx.authorFields().getValue(script, "ok");
    check(ok.has_value() && std::any_cast<float>(ok) == 7.0f,
          "T15h: script still works after hitting the memory limit");
    backend.shutdown(h);
    ctx.authorFields().erase(script);
    backend.setCallBudget(ScriptEngine::kDefaultCallBudget);
    backend.setMemoryLimit(ScriptEngine::kDefaultMemoryLimit);
  }

  if (failures == 0) {
    std::cout << "All ecmascript_backend tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test(s) FAILED.\n";
  return 1;
}
