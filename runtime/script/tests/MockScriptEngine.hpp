// MockScriptEngine.hpp  (TEST-ONLY)
// A ScriptEngine that records every call and can replay scripted field writes /
// route changes through the SaiContext — so the language-agnostic runtime
// (SaiContext, and later ScriptSystem) is testable with ZERO JavaScript.
//
// It is deliberately dumb: it never parses `source`. Instead the test arms it
// with reactions ("when invoke(eventName) fires, perform these SAI calls") and
// then asserts on the recorded call log. This makes the seam itself the unit
// under test, independent of any real engine.
#ifndef X3D_RUNTIME_TEST_MOCK_SCRIPT_ENGINE_HPP
#define X3D_RUNTIME_TEST_MOCK_SCRIPT_ENGINE_HPP

#include "SaiContext.hpp"
#include "ScriptEngine.hpp"

#include <any>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime::test {

/**
 * @brief Recording ScriptEngine for Track-A tests (no real language backend).
 * @details Each loaded script is one slot keyed by its handle. The mock:
 *            - records load/initialize/prepareEvents/invoke/eventsProcessed/
 *              shutdown calls in declaration order (the `calls` log), and
 *            - on invoke(eventName), runs any reaction the test registered for
 *              that event name, handing it the SaiContext so the reaction can
 *              exercise getField/setField/addRoute/etc. exactly as a real
 *              backend would.
 */
class MockScriptEngine : public ScriptEngine {
public:
  /// A reaction a test attaches to an invoked event name. It receives the live
  /// SaiContext (to call back into the runtime) and the event value.
  using Reaction = std::function<void(SaiContext &, const std::any &value,
                                      double timestamp)>;

  /// One recorded call (kind + the salient arguments).
  struct Call {
    std::string kind;       // "load","initialize","prepareEvents","invoke",
                            // "eventsProcessed","shutdown"
    ScriptHandle handle = kInvalidScriptHandle;
    std::string eventName;  // for invoke
    double timestamp = 0.0; // for invoke / prepareEvents (now)
  };

  ScriptHandle load(X3DNode &, const std::string &source,
                    SaiContext &sai) override {
    ScriptHandle h = nextHandle_++;
    slots_[h] = Slot{&sai, source};
    calls.push_back({"load", h, "", 0.0});
    return h;
  }

  void initialize(ScriptHandle handle) override {
    calls.push_back({"initialize", handle, "", 0.0});
  }

  void shutdown(ScriptHandle handle) override {
    calls.push_back({"shutdown", handle, "", 0.0});
  }

  void prepareEvents(ScriptHandle handle, double now) override {
    calls.push_back({"prepareEvents", handle, "", now});
  }

  void invoke(ScriptHandle handle, const std::string &eventName,
              const std::any &value, X3DFieldType /*type*/,
              double timestamp) override {
    calls.push_back({"invoke", handle, eventName, timestamp});
    auto slot = slots_.find(handle);
    if (slot == slots_.end() || !slot->second.sai) return;
    auto r = reactions_.find(eventName);
    if (r != reactions_.end() && r->second) {
      r->second(*slot->second.sai, value, timestamp);
    }
  }

  void eventsProcessed(ScriptHandle handle, double timestamp) override {
    calls.push_back({"eventsProcessed", handle, "", timestamp});
  }

  // ---- test arming API ------------------------------------------------------

  /// Register what to do when `eventName` is invoked (replaces any prior one).
  void on(const std::string &eventName, Reaction reaction) {
    reactions_[eventName] = std::move(reaction);
  }

  /// Count recorded calls of a given kind.
  std::size_t count(const std::string &kind) const {
    std::size_t n = 0;
    for (const auto &c : calls)
      if (c.kind == kind) ++n;
    return n;
  }

  /// The SaiContext a script was loaded with (null if unknown handle).
  SaiContext *saiFor(ScriptHandle h) const {
    auto it = slots_.find(h);
    return it == slots_.end() ? nullptr : it->second.sai;
  }

  /// The recorded call log, in call order.
  std::vector<Call> calls;

private:
  struct Slot {
    SaiContext *sai = nullptr;
    std::string source;
  };

  std::unordered_map<ScriptHandle, Slot> slots_;
  std::unordered_map<std::string, Reaction> reactions_;
  ScriptHandle nextHandle_ = 1;  // 0 is reserved (kInvalidScriptHandle)
};

} // namespace x3d::runtime::test

#endif // X3D_RUNTIME_TEST_MOCK_SCRIPT_ENGINE_HPP
