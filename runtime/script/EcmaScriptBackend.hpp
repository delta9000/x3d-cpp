// EcmaScriptBackend.hpp
// ScriptEngine implementation backed by the vendored Duktape 2.7.0 ECMAScript
// engine (runtime/script/vendor/duktape/). This is the reference backend for
// the X3D ECMAScript language binding (ISO/IEC 19777-1).
//
// U4 (this revision) completes the backend:
//   - field marshalling std::any <-> Duktape JS, both directions, for the core
//     SF/MF types (Bool/Int32/Float/Double/Time/String, Vec2/3/4 f+d, Color,
//     ColorRGBA, Rotation, Node; MF* as JS array-like);
//   - the Browser global object backed by SaiContext (currentTime,
//     currentFrameRate, getName, getVersion, addRoute, deleteRoute, print);
//   - handler dispatch: invoke() calls the handler function with (value,
//     timestamp), value marshalled from the boxed std::any by its X3DFieldType.
//
// Registered in the scheme->backend registry (ScriptSystem) under "ecmascript".
// The seam (ScriptEngine) carries NO JS types — the backend is the only place
// where Duktape API calls appear, and the marshalling is its private concern.
#ifndef X3D_RUNTIME_ECMASCRIPT_BACKEND_HPP
#define X3D_RUNTIME_ECMASCRIPT_BACKEND_HPP

#include "ScriptEngine.hpp"
#include "SaiContext.hpp"

// Duktape is a C library — include with extern "C" linkage guard so the C++
// compiler generates matching call conventions (Duktape itself is compiled as
// plain C by CMake, producing C-ABI symbols).
extern "C" {
#include "vendor/duktape/duktape.h"
}

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {

/**
 * @brief ECMAScript (Duktape) backend implementing the ScriptEngine seam.
 * @details One EcmaScriptBackend instance is shared across all ECMAScript
 *          Script nodes registered with the same ScriptSystem.  Each loaded
 *          script gets its own duk_context, keyed by the handle returned from
 *          load().  The handle is a simple 1-based index into the live-contexts
 *          table so the runtime never interprets the bits.
 *
 *          Browser native callbacks recover their SaiContext from a hidden own
 *          property stamped on each Browser method/getter at install time, so
 *          no engine-global state is needed to route a call back to its script.
 */
class EcmaScriptBackend : public ScriptEngine {
public:
  EcmaScriptBackend() = default;
  ~EcmaScriptBackend() override;

  // Disallow copy/move — the context table holds raw pointers.
  EcmaScriptBackend(const EcmaScriptBackend &) = delete;
  EcmaScriptBackend &operator=(const EcmaScriptBackend &) = delete;

  // -------------------------------------------------------------------------
  // ScriptEngine interface.
  // -------------------------------------------------------------------------

  /**
   * @brief Compile and eval the source in a fresh duk_context.
   * @details The source is the decoded inline script body (the prefix has been
   *          stripped by ScriptSystem::decodeInlineSource before we see it).
   *          A fresh context is created, the Browser global installed, then the
   *          source evaluated to define globals.  On Duktape error the context
   *          is destroyed and kInvalidScriptHandle is returned.
   */
  ScriptHandle load(X3DNode &scriptNode, const std::string &source,
                    SaiContext &sai) override;

  /**
   * @brief Call the script's initialize() function (§29.2.3) if it exists.
   * @details A missing initialize() is not an error (spec: it's optional).
   */
  void initialize(ScriptHandle handle) override;

  /**
   * @brief Call the script's shutdown() function (§29.2.4) and destroy the
   *        duk_context.
   */
  void shutdown(ScriptHandle handle) override;

  /**
   * @brief Call prepareEvents(now) if the function exists in the script.
   */
  void prepareEvents(ScriptHandle handle, double now) override;

  /**
   * @brief Deliver one inputOnly event: call the handler with (value,timestamp).
   * @details The handler is the global function whose name == eventName. `value`
   *          (a std::any tagged by `type`) is marshalled to a JS value and
   *          passed as the first argument; `timestamp` as the second (§29.2.8
   *          two-argument convention).  A missing handler is a no-op.
   */
  void invoke(ScriptHandle handle, const std::string &eventName,
              const std::any &value, X3DFieldType type,
              double timestamp) override;

  /**
   * @brief Call eventsProcessed() if it exists in the script.
   */
  void eventsProcessed(ScriptHandle handle, double timestamp) override;

  // -------------------------------------------------------------------------
  // Marshalling (public so the unit test can round-trip without a script).
  // These are the ISO/IEC 19777-1 std::any <-> JS conversions; they are the
  // backend's private concern w.r.t. the seam, but exposed here for testing.
  // -------------------------------------------------------------------------

  /**
   * @brief Push a boxed field value onto the Duktape stack as its JS form.
   * @param ctx The Duktape context.
   * @param value The boxed value (concrete C++ type per `type`); empty std::any
   *        pushes the type's JS zero/empty value.
   * @param type The field's type tag selecting the marshalling.
   */
  static void pushValue(duk_context *ctx, const std::any &value,
                        X3DFieldType type);

  /**
   * @brief Read a JS value at stack index `idx` back into a boxed std::any.
   * @param ctx The Duktape context.
   * @param idx The stack index of the JS value.
   * @param type The target field type tag selecting the marshalling.
   * @return The value boxed in the concrete C++ type for `type` (empty std::any
   *         only for an unsupported type).
   */
  static std::any toValue(duk_context *ctx, duk_idx_t idx, X3DFieldType type);

private:
  struct Entry {
    duk_context *ctx = nullptr;
    X3DNode *node = nullptr;  // owning Script node (not owned here)
    SaiContext *sai = nullptr; // SAI surface (not owned here)
  };

  // Retrieve the entry for `handle`; returns nullptr if invalid.
  Entry *entryFor(ScriptHandle handle);

  // Call a zero-arg global function by name; returns true if called, false if
  // the function does not exist. Pops whatever is on the stack on completion.
  bool callGlobalNoArgs(duk_context *ctx, const char *fnName);

  // Author-field marshalling (S1 file-authored Script un-tabling, design §3.5).
  // seedAuthorGlobals: push each readable author field's boxed initialValue
  //   (from the DynamicFieldStore) into a JS global of the field's name, so the
  //   script reads its authored defaults. Called once at initialize() time.
  // readbackAuthorGlobals: after a handler runs, read each readable author field
  //   (outputOnly/inputOutput) JS global back into the store and post it as a
  //   cascade event (carrying the triggering `timestamp`) via the script's
  //   SaiContext, so an author output write drives ROUTEs. Skips a field whose
  //   JS global is undefined (the handler never assigned it).
  static void seedAuthorGlobals(Entry &e);
  static void readbackAuthorGlobals(Entry &e, double timestamp);

  // Install the Browser global object on a fresh context, bound to the Entry's
  // SaiContext (stamped on each Browser method so native callbacks recover it).
  static void installBrowser(duk_context *ctx, Entry *entry);

  // Next handle to mint (starts at 1 so 0 stays kInvalidScriptHandle).
  ScriptHandle nextHandle_ = 1;

  // Live contexts indexed by handle value. Stable addresses (node-based map) so
  // the stashed Entry* pointer remains valid for the context's lifetime.
  std::unordered_map<ScriptHandle, Entry> entries_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_ECMASCRIPT_BACKEND_HPP
