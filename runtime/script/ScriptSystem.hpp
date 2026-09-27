// ScriptSystem.hpp
// The System that makes x3d::nodes::Script nodes run: it owns each enrolled x3d::nodes::Script's
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
// dispatch is driven through ScriptSystem::deliverInputEvent (the seam a x3d::nodes::Script
// inputOnly handler / the cascade calls), which forwards to engine.invoke — it
// does not require a per-field reflection thunk on the x3d::nodes::Script class. set_url is
// delivered through ScriptSystem::setUrl (the inputOnly handler for url), so the
// reload (shutdown + load + initialize) is observed without a codegen hook.
#ifndef X3D_RUNTIME_SCRIPT_SYSTEM_HPP
#define X3D_RUNTIME_SCRIPT_SYSTEM_HPP

#include "SaiContext.hpp"
#include "ScriptEngine.hpp"

#include "AssetResolver.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/core/X3DReflection.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/Script.hpp"

#include <algorithm>
#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief Drives x3d::nodes::Script-node lifecycle + event delivery through the cascade.
 * @details One ScriptSystem backs one *language* engine (the ScriptEngine seam);
 *          it enrolls x3d::nodes::Script nodes via attach() and owns their per-script SAI
 *          surface + load handle. It is a System so the context drives its
 *          prepareEvents phase each tick; the eventsProcessed phase is driven by
 *          the post-cascade hook installed by addScriptSystem().
 */
class ScriptSystem : public System {
public:
  /**
   * @brief Construct with the backend engine and browser identity.
   * @param engine The language backend (e.g. ECMAx3d::nodes::Script). Shared so a test can
   *        inspect the recorded calls and so the context can retain the system.
   * @param browserName Reported to scripts via SaiContext::getName().
   * @param browserVersion Reported via SaiContext::getVersion().
   * @param resolver The AssetResolver seam used to fetch a Script's external
   *        `url` entries (CONF-CRITIC-2) and to re-fetch them on the
   *        `autoRefresh` interval (SCR-005). Defaults to the IO-free null stub
   *        (always Failed), so a default-configured ScriptSystem stays
   *        byte-identical and never does I/O. The SDK ships no concrete backend;
   *        an app injects one (e.g. the CLI wires io::file::makeFileResolver).
   */
  ScriptSystem(std::shared_ptr<ScriptEngine> engine, std::string browserName,
               std::string browserVersion,
               extract::AssetResolver resolver = nullptr)
      : engine_(std::move(engine)), name_(std::move(browserName)),
        version_(std::move(browserVersion)),
        resolver_(resolver ? std::move(resolver)
                           : extract::makeNullAssetResolver()) {}

  /**
   * @brief Teardown: shut down every still-loaded script (§29.2.3, SCR-002).
   * @details Destroying the ScriptSystem (e.g. with its execution context) is
   *          the "world unloaded/replaced" case where each x3d::nodes::Script's shutdown()
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
   * @brief Enroll a x3d::nodes::Script node; load + initialize it if load=TRUE.
   * @details Per §29.2.3, initialize() runs before the script's first event, so
   *          we load+initialize at enroll time when the x3d::nodes::Script's load field is
   *          TRUE (the default) and a usable url is present. load=FALSE defers
   *          loading until the author flips load (delivered as a set_load event;
   *          re-attach / a future set_load handler triggers the load then).
   */
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    x3d::nodes::Script *script = dynamic_cast<x3d::nodes::Script *>(node);
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

  void detach(X3DNode *node, X3DExecutionContext &) override {
    auto *script = dynamic_cast<x3d::nodes::Script *>(node);
    if (!script) return;
    scripts_.erase(std::remove_if(scripts_.begin(), scripts_.end(),
        [&](const auto &entry) {
          if (entry->script != script) return false;
          if (engine_ && entry->handle != kInvalidScriptHandle)
            engine_->shutdown(entry->handle);
          return true;
        }), scripts_.end());
  }

  /**
   * @brief STEP 1: prepareEvents for every loaded script (once per timestamp).
   * @details Runs BEFORE the cascade drain (§29.2.5). Implemented as update() so
   *          the context's per-tick System pass invokes it ahead of route
   *          processing — register the ScriptSystem FIRST among systems.
   *          Before that, two URL-driven phases run once per tick: a retry of any
   *          fetch that came back Pending (CONF-CRITIC-2), then the autoRefresh
   *          interval check (SCR-005).
   */
  void update(double now, X3DExecutionContext &ctx) override {
    const std::uint64_t gen = ctx.tickGeneration();
    if (lastUpdateGen_ != gen) {
      lastUpdateGen_ = gen;
      retryPendingLoads(ctx);
      runAutoRefresh(now, ctx);
    }
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
  // Event delivery seam (called from the cascade / a x3d::nodes::Script inputOnly handler).
  // --------------------------------------------------------------------------

  /**
   * @brief Deliver one inputOnly author event to a x3d::nodes::Script (§29.2.2).
   * @details mustEvaluate=TRUE: invoke immediately (eager). mustEvaluate=FALSE:
   *          may be deferred to the batch flush (runEventsProcessed) — a
   *          permitted delay (§29.4.1). Either way the script is marked as having
   *          received an event so eventsProcessed() fires this cascade, and the
   *          last-event timestamp is tracked for eventsProcessed()'s outputs.
   * @param script The receiving x3d::nodes::Script.
   * @param eventName The inputOnly author field name (the handler dispatched to).
   * @param value Boxed event value (concrete C++ type per `type`).
   * @param type The field's type tag (so the backend can marshal `value`).
   * @param timestamp The event's timestamp; the script's outputs carry it.
   */
  void deliverInputEvent(x3d::nodes::Script *script, const std::string &eventName,
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
   * @brief Cascade listener: an event was delivered to an author inputOnly
   *        field (installed by X3DExecutionContext::addScriptSystem).
   * @details ISO/IEC 19775-1 §29.2: an event arriving at a Script's eventIn
   *          (inputOnly) invokes the script's function of that name with the
   *          value and the current timestamp. An event to an inputOutput author
   *          field updates the script's view of that field, so the script reads
   *          the new value and does not later write its stale copy back. Ignores
   *          nodes that are not Scripts enrolled in this system (another
   *          language's system, or a shader's uniform).
   */
  void onAuthorInput(X3DNode *node, const FieldInfo &info,
                     const std::any &value, X3DExecutionContext &ctx) {
    auto *script = dynamic_cast<x3d::nodes::Script *>(node);
    Entry *e = script ? entryFor(script) : nullptr;
    if (!e) return;
    if (info.access == AccessType::InputOnly) {
      deliverInputEvent(script, info.x3dName, value, info.type, ctx.now());
    } else if (e->handle != kInvalidScriptHandle) {
      engine_->updateField(e->handle, info.x3dName, value, info.type);
    }
  }

  /**
   * @brief Deliver a set_url event: shutdown old, swap url, load + initialize.
   * @details §29.2.2/§29.2.3: changing url shuts the running script down then
   *          loads + initializes the new content. The url preference list is
   *          re-run from the top: inline entries decode, external entries go
   *          through the AssetResolver seam (CONF-CRITIC-2), and the autoRefresh
   *          window (SCR-005) restarts from the new load.
   */
  void setUrl(x3d::nodes::Script *script, const MFString &newUrl,
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
    if (e) {
      // A url change re-runs the preference list from the top; the autoRefresh
      // window restarts from the new load.
      e->urlCandidate = 0;
      e->pendingFetch = false;
      e->refreshScheduled = false;
      e->windowStarted = false;
    }
    if (e && script->getLoad()) {
      loadAndInitialize(*e, ctx);
    }
  }

  /** @brief The SAI surface for a script (null if not enrolled). */
  SaiContext *saiFor(x3d::nodes::Script *script) {
    Entry *e = entryFor(script);
    return e ? &e->sai : nullptr;
  }

  /** @brief The load handle for a script (kInvalidScriptHandle if unloaded). */
  ScriptHandle handleFor(x3d::nodes::Script *script) {
    Entry *e = entryFor(script);
    return e ? e->handle : kInvalidScriptHandle;
  }

  /**
   * @brief Decode a single inline ecmascript:/javascript:/vrmlscript: url entry.
   * @details Returns the text after the scheme prefix, or empty if `entry` is
   *          not an inline script url.
   */
  static std::string decodeInlineUrl(const std::string &entry) {
    static const char *kSchemes[] = {"ecmascript:", "javascript:",
                                     "vrmlscript:"};
    for (const char *scheme : kSchemes) {
      const std::size_t n = std::char_traits<char>::length(scheme);
      if (entry.size() >= n && entry.compare(0, n, scheme) == 0) {
        return entry.substr(n);
      }
    }
    return {};
  }

  /** @brief True if `entry` carries its script body inline (no fetch needed). */
  static bool isInlineScheme(const std::string &entry) {
    static const char *kSchemes[] = {"ecmascript:", "javascript:",
                                     "vrmlscript:"};
    for (const char *scheme : kSchemes) {
      if (entry.rfind(scheme, 0) == 0) return true;
    }
    return false;
  }

  /**
   * @brief Decode an inline ecmascript:/javascript: url to its source body.
   * @details Returns the text after the first recognized inline scheme prefix,
   *          or empty if no entry in the preference list is an inline script.
   *          The url is a preference-ordered list (§29.2.8): the first inline
   *          entry wins. External (non-inline) entries are NOT fetched here —
   *          that needs the AssetResolver seam and is done by resolveSource().
   */
  static std::string decodeInlineSource(const MFString &url) {
    for (const std::string &entry : url) {
      if (isInlineScheme(entry)) return decodeInlineUrl(entry);
    }
    return {};
  }

  /**
   * @brief The script's IO-free body: sourceCode if non-empty, else inline url.
   * @details §3.3 of the design (file-authored x3d::nodes::Script un-tabling): readers write
   *          an inline `<![CDATA[...]]>` block / JSON source member / VRML body
   *          into x3d::nodes::Script.sourceCode, so prefer it. When sourceCode is empty (the
   *          programmatic / inline-url path) fall back to the url inline scheme
   *          decode (ecmascript:/javascript:/vrmlscript:). An external url yields
   *          empty here — resolveSource() is the path that fetches those through
   *          the AssetResolver seam.
   */
  static std::string scriptSource(const x3d::nodes::Script &script) {
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
    Entry(X3DExecutionContext &ctx, x3d::nodes::Script &script, const std::string &name,
          const std::string &version)
        : script(&script), sai(ctx, script, name, version) {}

    x3d::nodes::Script *script;
    SaiContext sai;
    ScriptHandle handle = kInvalidScriptHandle;
    bool receivedEventThisTick = false;
    double lastEventTimestamp = 0.0;
    std::vector<DeferredEvent> deferred;

    // ── External-url fetch state (CONF-CRITIC-2) ──────────────────────────
    std::size_t urlCandidate = 0;  ///< next url entry to try (preference order)
    bool pendingFetch = false;     ///< last resolver call was Pending -> retry
    std::size_t fetchAttempts = 0; ///< resolver calls made (diagnostic)
    // ── autoRefresh state (SCR-005) ───────────────────────────────────────
    /// Scene time the auto-refresh window opened (first load after attach or a
    /// url change). `autoRefreshTimeLimit` is measured from here and it does NOT
    /// move on a refresh — otherwise the window would never elapse.
    double refreshWindowStart = 0.0;
    bool windowStarted = false;    ///< refreshWindowStart is meaningful
    double nextRefreshAt = 0.0;    ///< scene time of the next scheduled refresh
    bool refreshScheduled = false; ///< a refresh interval is armed
  };

  Entry *entryFor(x3d::nodes::Script *script) {
    for (auto &up : scripts_)
      if (up->script == script) return up.get();
    return nullptr;
  }

  /**
   * @brief Resolve the script's body: sourceCode, else url preference order.
   * @details The url is a preference-ordered list (§29.2.8). Inline-scheme
   *          entries (ecmascript:/javascript:/vrmlscript:) decode directly.
   *          External entries go through the AssetResolver seam, tried in order:
   *            - Ready   -> the fetched bytes are the source; stop;
   *            - Pending -> remember the position and retry the SAME entry on a
   *                         later tick (return nullopt for now);
   *            - Failed  -> advance to the next entry.
   *          Returns nullopt when there is no content yet (a pending fetch, or
   *          every entry exhausted) — the script stays inert. This is a
   *          permitted delay; the spec's load is asynchronous (§29.2.3).
   */
  std::optional<std::string> resolveSource(Entry &e) {
    const SFString &src = e.script->getSourceCode();
    if (!src.empty()) return src;
    const MFString &url = e.script->getUrl();
    while (e.urlCandidate < url.size()) {
      const std::string &u = url[e.urlCandidate];
      if (isInlineScheme(u)) return decodeInlineUrl(u);
      if (u.empty()) {
        ++e.urlCandidate;
        continue;
      }
      ++e.fetchAttempts;
      const extract::AssetResult r = resolver_(u, extract::AssetKind::Inline);
      if (r.ready()) {
        e.pendingFetch = false;
        return std::string(r.bytes.begin(), r.bytes.end());
      }
      if (r.pending()) {
        e.pendingFetch = true;
        return std::nullopt; // retry this same candidate on a later tick
      }
      ++e.urlCandidate; // Failed -> try the next url in preference order.
    }
    e.pendingFetch = false;
    return std::nullopt;
  }

  /**
   * @brief Load (resolve source + engine.load) then initialize a Script.
   * @details No-op if already loaded. Resolves from sourceCode (the reader-CDATA
   *          path, §3.3) or the url preference list (inline decode, else an
   *          AssetResolver fetch). A Pending/absent source yields no handle (the
   *          script stays inert until content arrives). initialize() runs
   *          immediately after a successful load so it precedes the script's
   *          first event (§29.2.3).
   */
  void loadAndInitialize(Entry &e, X3DExecutionContext &ctx) {
    if (e.handle != kInvalidScriptHandle) return; // already loaded
    std::optional<std::string> source = resolveSource(e);
    if (!source) return; // pending fetch / no content -> inert
    e.handle = engine_->load(*e.script, *source, e.sai);
    if (e.handle != kInvalidScriptHandle) {
      engine_->initialize(e.handle);
      if (!e.windowStarted) {
        e.refreshWindowStart = ctx.now();
        e.windowStarted = true;
      }
      scheduleRefresh(e, ctx.now());
    }
  }

  /**
   * @brief Arm (or clear) the `autoRefresh` interval for a loaded script.
   * @details SCR-005 / §29.3.1: `autoRefresh` > 0 reloads the content every that
   *          many seconds; `autoRefreshTimeLimit` bounds how long refreshing
   *          continues (0 = no limit). Both are re-read live each tick, so an
   *          author change takes effect without a reload.
   */
  void scheduleRefresh(Entry &e, double now) {
    const double interval = e.script->getAutoRefresh();
    if (interval <= 0.0) {
      e.refreshScheduled = false;
      return;
    }
    e.nextRefreshAt = now + interval;
    e.refreshScheduled = true;
  }

  /**
   * @brief Retry any fetch that last came back Pending (CONF-CRITIC-2).
   * @details A Pending result is the resolver's "not ready yet — retry next
   *          frame" contract; a Pending fetch never blocks a frame. Retried here
   *          once per tick, at the same url candidate.
   */
  void retryPendingLoads(X3DExecutionContext &ctx) {
    for (auto &up : scripts_) {
      Entry &e = *up;
      if (e.handle != kInvalidScriptHandle || !e.pendingFetch) continue;
      if (!e.script->getLoad()) continue;
      loadAndInitialize(e, ctx);
      if (e.handle != kInvalidScriptHandle) e.pendingFetch = false;
    }
  }

  /**
   * @brief Run the autoRefresh interval for every script (SCR-005).
   * @details When `now` reaches the armed interval, the content is re-fetched and
   *          the script re-initialized: shutdown -> resolve+load -> initialize
   *          (§29.2.3, the same sequence as a url change). Refreshing stops once
   *          `autoRefreshTimeLimit` (measured from the load that opened the
   *          refresh window) has elapsed;
   *          a limit of 0 means no limit. A refresh whose fetch is Pending leaves
   *          the script unloaded and arms a retry, exactly like the initial load.
   */
  void runAutoRefresh(double now, X3DExecutionContext &ctx) {
    for (auto &up : scripts_) {
      Entry &e = *up;
      const double interval = e.script->getAutoRefresh();
      if (interval <= 0.0) {
        e.refreshScheduled = false;
        continue;
      }
      if (!e.refreshScheduled) {
        scheduleRefresh(e, now);
        continue;
      }
      if (now < e.nextRefreshAt) continue;
      const double limit = e.script->getAutoRefreshTimeLimit();
      if (limit > 0.0 && now - e.refreshWindowStart > limit) {
        e.refreshScheduled = false; // window elapsed: stop refreshing
        continue;
      }
      if (!e.script->getLoad()) continue;
      refresh(e, now, ctx);
    }
  }

  /**
   * @brief Shut down, re-resolve from url, and re-initialize a loaded script.
   * @details The refresh window (refreshWindowStart/windowStarted) is left
   *          intact — autoRefreshTimeLimit bounds the whole refresh period, not
   *          an individual interval, so it must not restart here.
   */
  void refresh(Entry &e, double now, X3DExecutionContext &ctx) {
    if (e.handle != kInvalidScriptHandle) {
      engine_->shutdown(e.handle);
      e.handle = kInvalidScriptHandle;
    }
    e.urlCandidate = 0; // re-run the url preference list from the top
    e.pendingFetch = false;
    e.nextRefreshAt = now + e.script->getAutoRefresh();
    e.refreshScheduled = true;
    loadAndInitialize(e, ctx);
  }

  std::shared_ptr<ScriptEngine> engine_;
  std::string name_;
  std::string version_;
  extract::AssetResolver resolver_;
  std::vector<std::unique_ptr<Entry>> scripts_;
  // §29.2.5 once-per-timestamp guard for the prepareEvents phase (SCR-001).
  bool havePrepared_ = false;
  double preparedAt_ = 0.0;
  // Once-per-tick guard for the URL phases (retry-pending + autoRefresh).
  std::uint64_t lastUpdateGen_ = ~std::uint64_t{0};
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCRIPT_SYSTEM_HPP
