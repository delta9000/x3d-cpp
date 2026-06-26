// ScriptEngine.hpp
// The language-agnostic Script backend seam (ISO/IEC 19775-1 §29.1: browsers
// are not required to support any specific language). A backend (ECMAScript via
// Duktape, or any other) implements this interface; ScriptSystem drives it.
//
// LANGUAGE-AGNOSTIC CONTRACT (spec §4.2): this seam carries NO scripting-language
// types. Every value crossing it is the runtime's own field representation
// (std::any boxing the field's C++ type, tagged by X3DFieldType) — never a JS
// object. Each backend privately marshals std::any <-> its own idiom. A Lua,
// Java, or Python backend would map the same std::any field values to its world.
#ifndef X3D_RUNTIME_SCRIPT_ENGINE_HPP
#define X3D_RUNTIME_SCRIPT_ENGINE_HPP

#include "X3DReflection.hpp"  // X3DFieldType (type tag carried alongside values)

#include <any>
#include <cstdint>
#include <string>

class X3DNode;

namespace x3d::runtime {

class SaiContext;

/**
 * @brief Opaque handle a backend returns from load() to identify a loaded
 *        script instance. Zero is the canonical "no/invalid handle" value.
 * @details The runtime never interprets the bits; it only passes a handle back
 *          to the same backend that minted it. A backend is free to encode an
 *          index, a pointer, or any token it likes.
 */
using ScriptHandle = std::uint64_t;

/** @brief The invalid / unloaded script handle. */
inline constexpr ScriptHandle kInvalidScriptHandle = 0;

/**
 * @brief Abstract Script backend: compiles a script and dispatches events to it.
 * @details One ScriptEngine instance backs a whole *language* (e.g. "ecmascript")
 *          and may hold many loaded scripts, each addressed by a ScriptHandle.
 *          The lifecycle mirrors ISO 19775-1 §29.2:
 *            - load(): compile/parse the source for a Script node.
 *            - initialize(): run the script's initialize() before its first event.
 *            - prepareEvents(): once per timestamp, BEFORE any ROUTE processing.
 *            - invoke(): deliver one inputOnly event to its handler.
 *            - eventsProcessed(): after a batch of received events.
 *            - shutdown(): on node removal or url change.
 *
 *          A backend reaches back into the scene (read/write fields, routes,
 *          browser info) through the SaiContext supplied at load time. The SAI
 *          surface is the *only* sanctioned channel from backend to runtime, and
 *          it enforces the directOutput gate.
 */
class ScriptEngine {
public:
  virtual ~ScriptEngine() = default;

  /**
   * @brief Compile `source` for `scriptNode`; return a handle (or invalid).
   * @param scriptNode The owning Script node (its author fields define the
   *        interface the script may read/write; directOutput gates the SAI).
   * @param source The decoded script body (inline ecmascript: text, or fetched
   *        external source) — already language-appropriate for this backend.
   * @param sai The in-process SAI surface the script calls back into. The engine
   *        must NOT outlive `sai`; the runtime owns lifetime ordering.
   * @return A non-zero handle on success, kInvalidScriptHandle on failure.
   */
  virtual ScriptHandle load(X3DNode &scriptNode, const std::string &source,
                            SaiContext &sai) = 0;

  /** @brief Run the script's initialize() (§29.2.3); before its first event. */
  virtual void initialize(ScriptHandle handle) = 0;

  /** @brief Run the script's shutdown() (§29.2.4); on removal / url change. */
  virtual void shutdown(ScriptHandle handle) = 0;

  /**
   * @brief Collect spontaneous/async data once per timestamp, before ROUTEs.
   * @details Maps to the script's prepareEvents() (§29.2.5). Called once per
   *          timestamp by ScriptSystem before any route processing, so a script
   *          may emit sensor-like outputs at the frame's leading edge.
   */
  virtual void prepareEvents(ScriptHandle handle, double now) = 0;

  /**
   * @brief Deliver one inputOnly event to the script (§29.2.2).
   * @param handle The loaded script.
   * @param eventName The author field name receiving the event (the handler the
   *        backend dispatches to; e.g. ECMAScript calls a function of this name).
   * @param value The event value, boxed in std::any (concrete C++ type per the
   *        field's X3DFieldType). NEVER a scripting-language object.
   * @param type The field's type tag, so the backend can marshal `value`.
   * @param timestamp The event's timestamp; values the script emits in response
   *        carry this same timestamp (conceptually zero elapsed time).
   */
  virtual void invoke(ScriptHandle handle, const std::string &eventName,
                      const std::any &value, X3DFieldType type,
                      double timestamp) = 0;

  /**
   * @brief Signal the end of a batch of received events (§29.2.2).
   * @details Lets order-independent scripts emit fewer events. Called at most
   *          once per script per cascade batch, after all inputOnly events for
   *          this timestamp were invoked.
   */
  virtual void eventsProcessed(ScriptHandle handle, double timestamp) = 0;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SCRIPT_ENGINE_HPP
