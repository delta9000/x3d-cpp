// ScriptSystem.hpp
// The System that makes Script nodes run: it owns each enrolled Script's
// lifecycle (load / initialize / shutdown), holds its in-process SAI surface,
// and threads its events through the X3DExecutionContext cascade in the exact
// order ISO/IEC 19775-1 §29.2 mandates.
//
// PER-TICK ORDERING (all NORMATIVE, §29.2 + §4.4.8.3):
//   STEP 1  prepareEvents()   — once per timestamp, BEFORE any ROUTE processing
//                               (§29.2.5). ScriptSystem::update() runs this, so
//                               registering the system FIRST puts it ahead of
//                               other sensor systems and the cascade drain.
//   STEP 3  invoke()          — each inputOnly author event delivered to the
//                               script in timestamp order (§29.2.2). outputOnly
//                               writes the script makes become cascade events
//                               carrying the TRIGGERING event's timestamp.
//   STEP 4  eventsProcessed() — after the batch, at most once per script per
//                               cascade (§29.2.4); only for scripts that
//                               received >=1 event this tick.
//
// The execution context calls runPrepareEvents() (via update) before the cascade
// drain and runEventsProcessed() after it (X3DExecutionContext::addScriptSystem
// wires the post-cascade phase). See X3DExecutionContext::tick().
//
// directOutput / mustEvaluate (§29.4.1) are honored: the SaiContext gates
// cross-node writes on directOutput; mustEvaluate=TRUE delivers inputs eagerly,
// FALSE may defer them to the batch flush (a permitted, spec-sanctioned delay).
//
// CODEGEN-FREE: this layer needs no generator change. inputOnly author-field
// dispatch is driven through ScriptSystem::deliverInputEvent (the seam a Script
// inputOnly handler / the cascade calls), which forwards to engine.invoke — it
// does not require a per-field reflection thunk on the Script class. set_url is
// delivered through ScriptSystem::setUrl (the inputOnly handler for url), so the
// reload (shutdown + load + initialize) is observed without a codegen hook.
#ifndef X3D_RUNTIME_SCRIPT_SYSTEM_HPP
#define X3D_RUNTIME_SCRIPT_SYSTEM_HPP

#include "SaiContext.hpp"
#include "ScriptEngine.hpp"

#include "X3DExecutionContext.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/Script.hpp"

#include <any>
#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Drives Script-node lifecycle + event delivery through the cascade.
 * @details One ScriptSystem backs one *language* engine (the ScriptEngine seam);
 *          it enrolls Script nodes via attach() and owns their per-script SAI
 *          surface + load handle. It is a System so the context drives its
 *          prepareEvents phase each tick; the eventsProcessed phase is driven by
 *          the post-cascade hook installed by addScriptSystem().
 */
class ScriptSystem : public System {
public:
  /**
   * @brief Construct with the backend engine and browser identity.
   * @param engine The language backend (e.g. ECMAScript). Shared so a test can
   *        inspect the recorded calls and so the context can retain the system.
   * @param browserName Reported to scripts via SaiContext::getName().
   * @param browserVersion Reported via SaiContext::getVersion().
   */
  ScriptSystem(std::shared_ptr<ScriptEngine> engine, std::string browserName,
               std::string browserVersion)
      : engine_(std::move(engine)), name_(std::move(browserName)),
        version_(std::move(browserVersion)) {}

  /**
   * @brief Teardown: shut down every still-loaded script (§29.2.3, SCR-002).
   * @details Destroying the ScriptSystem (e.g. with its execution context) is
   *          the "world unloaded/replaced" case where each Script's shutdown()
   *          must run. Done before the engine member is destroyed. Node-level
   *          deletion (dynamic SAI) remains deferred.
   */
  ~ScriptSystem() {
    if (!engine_) return;
    for (auto &up : scripts_) {
      if (up && up->handle != kInvalidScriptHandle) {
        engine_->shutdown(up->handle);
        up->handle = kInvalidScriptHandle; // guard against double-shutdown
      }
    }
  }

  // --------------------------------------------------------------------------
  // System interface.
  // --------------------------------------------------------------------------

  /**
   * @brief Enroll a Script node; load + initialize it if load=TRUE.
   * @details Per §29.2.3, initialize() runs before the script's first event, so
   *          we load+initialize at enroll time when the Script's load field is
   *          TRUE (the default) and a usable url is present. load=FALSE defers
   *          loading until the author flips load (delivered as a set_load event;
   *          re-attach / a future set_load handler triggers the load then).
   */
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    Script *script = dynamic_cast<Script *>(node);
    if (!script) return;
    Entry *e = entryFor(script);
    if (!e) {
      scripts_.push_back(std::make_unique<Entry>(ctx, *script, name_, version_));
      e = scripts_.back().get();
    }
    if (script->getLoad()) {
      loadAndInitialize(*e, ctx);
    }
  }

  /**
   * @brief STEP 1: prepareEvents for every loaded script (once per timestamp).
   * @details Runs BEFORE the cascade drain (§29.2.5). Implemented as update() so
   *          the context's per-tick System pass invokes it ahead of route
   *          processing — register the ScriptSystem FIRST among systems.
   */
  void update(double now, X3DExecutionContext &ctx) override {
    runPrepareEvents(now, ctx);
  }

  // --------------------------------------------------------------------------
  // Explicit phase hooks (the context calls these around the cascade drain).
  // --------------------------------------------------------------------------

  /** @brief STEP 1: §29.2.5 prepareEvents() for each loaded script. */
  void runPrepareEvents(double now, X3DExecutionContext & /*ctx*/) {
    // §29.2.5: prepareEvents() is called exactly once per timestamp, before any
    // ROUTE processing. The context re-invokes update() on every cascade
    // do-while iteration, so guard the phase to fire only on the first
    // invocation for a given timestamp (SCR-001); otherwise prepareEvents (and
    // the receivedEventThisTick reset below) would run N times per tick.
    if (havePrepared_ && now == preparedAt_) return;
    havePrepared_ = true;
    preparedAt_ = now;
    for (auto &up : scripts_) {
      Entry &e = *up;
      if (e.handle == kInvalidScriptHandle) continue;
      e.receivedEventThisTick = false;  // clear the per-tick receiver flag
      engine_->prepareEvents(e.handle, now);
    }
  }

  /**
   * @brief STEP 4: flush any deferred inputs, then §29.2.4 eventsProcessed().
   * @details For mustEvaluate=FALSE scripts, inputs may have been deferred; flush
   *          them now (invoke in arrival order) so their outputs still enter this
   *          cascade. Then call eventsProcessed() at most once per script that
   *          received >=1 event this cascade, with the timestamp of the LAST
   *          event it processed (§29.2.4). Outputs are drained by the context.
   */
  void runEventsProcessed(X3DExecutionContext &ctx) {
    for (auto &up : scripts_) {
      Entry &e = *up;
      if (e.handle == kInvalidScriptHandle) continue;
      // Flush deferred (lazy / mustEvaluate=FALSE) inputs.
      for (auto &ev : e.deferred) {
        engine_->invoke(e.handle, ev.eventName, ev.value, ev.type, ev.timestamp);
        e.receivedEventThisTick = true;
        e.lastEventTimestamp = ev.timestamp;
      }
      e.deferred.clear();
      if (e.receivedEventThisTick) {
        engine_->eventsProcessed(e.handle, e.lastEventTimestamp);
        e.receivedEventThisTick = false;  // at most once per cascade
      }
    }
    ctx.process();  // drain any events eventsProcessed()/the flush produced
  }

  // --------------------------------------------------------------------------
  // Event delivery seam (called from the cascade / a Script inputOnly handler).
  // --------------------------------------------------------------------------

  /**
   * @brief Deliver one inputOnly author event to a Script (§29.2.2).
   * @details mustEvaluate=TRUE: invoke immediately (eager). mustEvaluate=FALSE:
   *          may be deferred to the batch flush (runEventsProcessed) — a
   *          permitted delay (§29.4.1). Either way the script is marked as having
   *          received an event so eventsProcessed() fires this cascade, and the
   *          last-event timestamp is tracked for eventsProcessed()'s outputs.
   * @param script The receiving Script.
   * @param eventName The inputOnly author field name (the handler dispatched to).
   * @param value Boxed event value (concrete C++ type per `type`).
   * @param type The field's type tag (so the backend can marshal `value`).
   * @param timestamp The event's timestamp; the script's outputs carry it.
   */
  void deliverInputEvent(Script *script, const std::string &eventName,
                         std::any value, X3DFieldType type, double timestamp) {
    Entry *e = entryFor(script);
    if (!e || e->handle == kInvalidScriptHandle) return;
    if (script->getMustEvaluate()) {
      engine_->invoke(e->handle, eventName, value, type, timestamp);
      e->receivedEventThisTick = true;
      e->lastEventTimestamp = timestamp;
    } else {
      e->deferred.push_back(DeferredEvent{eventName, std::move(value), type,
                                          timestamp});
    }
  }

  /**
   * @brief Deliver a set_url event: shutdown old, swap url, load + initialize.
   * @details §29.2.2/§29.2.3: changing url shuts the running script down then
   *          loads + initializes the new content. The inline url is decoded; an
   *          external url (no recognized inline scheme) leaves the script with no
   *          executable content (async fetch is deferred — SCR-ASYNC/SCR-REFRESH).
   */
  void setUrl(Script *script, const MFString &newUrl,
              X3DExecutionContext &ctx) {
    Entry *e = entryFor(script);
    if (!e) {
      attach(script, ctx);
      e = entryFor(script);
    }
    if (e && e->handle != kInvalidScriptHandle) {
      engine_->shutdown(e->handle);
      e->handle = kInvalidScriptHandle;
    }
    script->setUrl(newUrl);
    if (e && script->getLoad()) {
      loadAndInitialize(*e, ctx);
    }
  }

  /** @brief The SAI surface for a script (null if not enrolled). */
  SaiContext *saiFor(Script *script) {
    Entry *e = entryFor(script);
    return e ? &e->sai : nullptr;
  }

  /** @brief The load handle for a script (kInvalidScriptHandle if unloaded). */
  ScriptHandle handleFor(Script *script) {
    Entry *e = entryFor(script);
    return e ? e->handle : kInvalidScriptHandle;
  }

  /**
   * @brief Decode an inline ecmascript:/javascript: url to its source body.
   * @details Returns the text after the first recognized inline scheme prefix,
   *          or empty if no entry in the preference list is an inline script
   *          (external urls are not fetched here — SCR-ASYNC deferred). The url
   *          is a preference-ordered list (§29.2.8): the first inline entry wins.
   */
  static std::string decodeInlineSource(const MFString &url) {
    static const char *kSchemes[] = {"ecmascript:", "javascript:",
                                     "vrmlscript:"};
    for (const std::string &entry : url) {
      for (const char *scheme : kSchemes) {
        const std::size_t n = std::char_traits<char>::length(scheme);
        if (entry.size() >= n && entry.compare(0, n, scheme) == 0) {
          return entry.substr(n);
        }
      }
    }
    return {};
  }

  /**
   * @brief The script's executable body: sourceCode if non-empty, else url.
   * @details §3.3 of the design (file-authored Script un-tabling): readers write
   *          an inline `<![CDATA[...]]>` block / JSON source member / VRML body
   *          into Script.sourceCode, so prefer it. When sourceCode is empty (the
   *          programmatic / inline-url path) fall back to the existing url inline
   *          scheme decode (ecmascript:/javascript:/vrmlscript:). An external url
   *          with no recognized inline scheme still yields empty (the script
   *          stays inert until content arrives — async fetch deferred).
   */
  static std::string scriptSource(const Script &script) {
    const SFString &src = script.getSourceCode();
    if (!src.empty()) return src;
    return decodeInlineSource(script.getUrl());
  }

private:
  struct DeferredEvent {
    std::string eventName;
    std::any value;
    X3DFieldType type;
    double timestamp;
  };

  /// Per-enrolled-script state: its SAI surface, load handle, and tick flags.
  struct Entry {
    Entry(X3DExecutionContext &ctx, Script &script, const std::string &name,
          const std::string &version)
        : script(&script), sai(ctx, script, name, version) {}

    Script *script;
    SaiContext sai;
    ScriptHandle handle = kInvalidScriptHandle;
    bool receivedEventThisTick = false;
    double lastEventTimestamp = 0.0;
    std::vector<DeferredEvent> deferred;
  };

  Entry *entryFor(Script *script) {
    for (auto &up : scripts_)
      if (up->script == script) return up.get();
    return nullptr;
  }

  /**
   * @brief Load (decode source + engine.load) then initialize a Script.
   * @details No-op if already loaded. Sources from sourceCode (the reader-CDATA
   *          path, §3.3) else the inline url; an external/empty source yields no
   *          handle (the script is inert until content arrives — deferred async).
   *          initialize() runs immediately after a successful load so it precedes
   *          the script's first event (§29.2.3).
   */
  void loadAndInitialize(Entry &e, X3DExecutionContext & /*ctx*/) {
    if (e.handle != kInvalidScriptHandle) return;  // already loaded
    std::string source = scriptSource(*e.script);
    if (source.empty()) return;  // external/no inline content -> inert (deferred)
    e.handle = engine_->load(*e.script, source, e.sai);
    if (e.handle != kInvalidScriptHandle) {
      engine_->initialize(e.handle);
    }
  }

  std::shared_ptr<ScriptEngine> engine_;
  std::string name_;
  std::string version_;
  std::vector<std::unique_ptr<Entry>> scripts_;
  // §29.2.5 once-per-timestamp guard for the prepareEvents phase (SCR-001).
  bool havePrepared_ = false;
  double preparedAt_ = 0.0;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCRIPT_SYSTEM_HPP
