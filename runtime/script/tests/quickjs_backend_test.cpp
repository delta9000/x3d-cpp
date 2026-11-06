// QuickJS-specific ECMAScript backend checks.
#include "QuickJsBackend.hpp"
#include "ScriptBackendTestCases.hpp"

#include <chrono>
#include <cstddef>

using namespace script_backend_tests;

int main() {
  QuickJsBackend backend;
  runSharedCases(backend, "T18: ");

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

  // QuickJS supports ES2015 Date subclassing; Duktape supports ES5.
  {
    Fixture fix;
    fix.ctx.tick(12.5);
    fix.ctx.authorFields().addAuthorField(
        fix.script, AuthorFieldDecl{"subclassOk", X3DFieldType::SFBool,
                                    AccessType::OutputOnly, {}});
    ScriptHandle h = loadSource(backend, fix,
        "function initialize() {"
        "  class SubDate extends Date {}"
        "  var sd = new SubDate();"
        "  subclassOk = (sd instanceof SubDate) && (sd instanceof Date) &&"
        "               (sd.getTime() === Date.now());"
        "}");
    check(h != kInvalidScriptHandle, "T18: load Date subclass script");
    backend.initialize(h);
    std::any sk = fix.ctx.authorFields().getValue(fix.script, "subclassOk");
    check(sk.has_value() && std::any_cast<bool>(sk),
          "T18: class SubDate extends Date instanceof SubDate holds");
    backend.shutdown(h);
    fix.ctx.authorFields().erase(fix.script);
  }

  if (failures == 0) {
    std::cout << "All quickjs_backend tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test(s) FAILED.\n";
  return 1;
}
