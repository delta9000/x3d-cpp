// QuickJS-ng ScriptEngine implementation; see ADR-0022.
// Engine types and headers stay private to QuickJsBackend.cpp.
#ifndef X3D_RUNTIME_QUICKJS_BACKEND_HPP
#define X3D_RUNTIME_QUICKJS_BACKEND_HPP

#include "ScriptEngine.hpp"

#include <any>
#include <memory>
#include <string>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {

class SaiContext;

/**
 * @brief QuickJS (quickjs-ng) backend implementing the ScriptEngine seam.
 * @details One QuickJsBackend instance backs the whole "ecmascript" language and
 *          may hold many loaded scripts, each addressed by a ScriptHandle. A
 *          single shared JSRuntime owns the heap; each loaded script gets its own
 *          JSContext (so globals defined by one script never leak into another),
 *          keyed by the handle returned from load(). The handle is a simple
 *          1-based counter so 0 stays kInvalidScriptHandle and the runtime never
 *          interprets the bits.
 */
class QuickJsBackend : public ScriptEngine {
public:
  QuickJsBackend();
  ~QuickJsBackend() override;

  // Disallow copy/move — Impl holds raw QuickJS runtime/context pointers.
  QuickJsBackend(const QuickJsBackend &) = delete;
  QuickJsBackend &operator=(const QuickJsBackend &) = delete;

  // ScriptEngine interface (the 6 seam methods).

  /**
   * @brief Create a fresh JSContext and JS_Eval the source to define globals.
   * @details On a syntax/eval error the context is freed and
   *          kInvalidScriptHandle is returned; otherwise a non-zero handle is
   *          minted. The source is the decoded inline body (the "ecmascript:"
   *          prefix is stripped by ScriptSystem before we see it).
   */
  ScriptHandle load(X3DNode &scriptNode, const std::string &source,
                    SaiContext &sai) override;

  /** @brief Call the script's initialize() global if it exists (§29.2.3). */
  void initialize(ScriptHandle handle) override;

  /** @brief Call shutdown() if it exists, then free the JSContext (§29.2.4). */
  void shutdown(ScriptHandle handle) override;

  /** @brief Call prepareEvents(now) if it exists (§29.2.5). */
  void prepareEvents(ScriptHandle handle, double now) override;

  /**
   * @brief Deliver one inputOnly event: call the named global handler with
   *        (value, timestamp), value marshalled from the boxed std::any by its
   *        X3DFieldType (full SF/MF parity), then read author outputs back.
   */
  void invoke(ScriptHandle handle, const std::string &eventName,
              const std::any &value, X3DFieldType type,
              double timestamp) override;

  /** @brief Call eventsProcessed() if it exists (§29.2.2). */
  void eventsProcessed(ScriptHandle handle, double timestamp) override;
  void updateField(ScriptHandle handle, const std::string &name,
                   const std::any &value, X3DFieldType type) override;

private:
  // pImpl: every JSRuntime / JSContext / JSValue lives here, in the .cpp, so
  // this header never names a QuickJS type.
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_QUICKJS_BACKEND_HPP
