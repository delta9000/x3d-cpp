// SaiContext.hpp
// The in-process Scene Access Interface surface a Script backend calls back into
// (ISO/IEC 19775-1 §29.4.1, §4.9; ISO 19775-2). Read/set fields, add/delete
// routes, query browser info, print. This is the only sanctioned channel from a
// backend into the running scene, and it enforces the Script's directOutput gate.
//
// Every value crossing this surface is the runtime's own
// std::any field representation, never a scripting-language object.
// This is the runtime's Script-backend API. It is separate from the SAI SDK's
// portable provider contract (sai/, served natively by adapters/sai_provider):
// its node pointers, field values, lifetime and event rules belong to this
// runtime and are not portable provider handles. Including both libraries does
// not connect a Script's context to a provider service.
//
// directOutput / addRoute / deleteRoute gates (spec-checked, §29.2.6 + §29.4.1):
//   - getField(node, name)        : always permitted (read access is unconditional).
//   - setField(self, name, value) : always permitted (a script may write its own
//                                   fields — chiefly its outputOnly outputs).
//   - setField(other, name, value): permitted only if directOutput==TRUE; else the
//                                   spec declares the result UNDEFINED, and we
//                                   take the safest conformant response: throw.
//   - addRoute / deleteRoute      : permitted only if directOutput==TRUE; else throw.
// A permitted self-write (to the Script's own field) is posted as an input event
// into the current cascade at the current timestamp, so it fans out along ROUTEs
// (§29.2.4 — the sanctioned output channel). A permitted cross-node write is a
// direct value mutation of the target field (SAI 19775-2 §4.5.2 / ADR-0029): it
// generates no event and does not form part of the cascade, so no ROUTE fans out
// from the written field. The target still generates its own output events per
// that field's access type, since the mutation is an ordinary write.
//
// §29.2.6 ("shall be part of the current event cascade") contradicts SAI §4.5.2
// ("generates no event... not part of the event cascade"); SAI governs this seam.
#ifndef X3D_RUNTIME_SAI_CONTEXT_HPP
#define X3D_RUNTIME_SAI_CONTEXT_HPP

#include "DynamicField.hpp"
#include "NodeHandleTable.hpp"
#include "X3DEventGraph.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DFieldAddress.hpp"
#include "x3d/core/X3DReflection.hpp"

#include "x3d/nodes/Script.hpp"  // Script::getDirectOutput()

#include <any>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {

using namespace x3d::core;

/**
 * @brief In-process SAI surface for one Script node's backend.
 * @details Bound to (a) the owning Script (for the directOutput gate + browser
 *          identity) and (b) the X3DExecutionContext (the cascade + route graph
 *          + clock the script acts on). A backend is handed this at load() and
 *          calls these methods from script code (e.g. an ECMAScript `Browser`
 *          object and field accessors forward here).
 *
 *          Lifetime: the SaiContext does not own the Script or the context; the
 *          context lease rejects runtime access after context retirement.
 */
class SaiContext {
public:
  /**
   * @brief Construct a SAI surface for one script.
   * @param ctx The execution context (cascade, routes, clock).
   * @param scriptNode The owning Script node (gates direct writes / routes).
   * @param name Browser name reported by getName() (e.g. "x3d-cpp-gen").
   * @param version Browser version reported by getVersion().
   */
  SaiContext(X3DExecutionContext &ctx, x3d::nodes::Script &scriptNode, std::string name,
             std::string version)
      : ctx_(ctx), script_(scriptNode), name_(std::move(name)),
        version_(std::move(version)), active_(ctx.callbackLease()) {}

  // Field access (§29.4.1 paragraph 1: read is unconditional).

  /**
   * @brief Read the last value of `fieldName` on `node` (boxed std::any).
   * @details Permitted regardless of directOutput. Returns an empty std::any if
   *          the node is null, the field is unknown, or the field is not readable
   *          (e.g. an inputOnly field has no get thunk).
   */
  std::any getField(X3DNode *node, const std::string &fieldName) const {
    requireActive();
    if (!node) return {};
    std::optional<FieldInfo> info = findField(*node, fieldName);
    if (!info || !info->get) return {};
    return info->get(*node);
  }

  /**
   * @brief Set `fieldName` on `node` to `value`.
   * @details Gate (§29.2.6/§29.4.1):
   *            - node == owning Script: always allowed (its own fields/outputs).
   *            - node != owning Script: allowed only if directOutput==TRUE;
   *              otherwise throws std::logic_error (spec says "undefined").
   *          Semantics differ by target (SAI §4.5.2 / ADR-0029):
   *            - a self-write is posted as an input event into the current
   *              cascade so it carries the current timestamp and fans out via
   *              ROUTEs (the Script's own output channel, §29.2.4);
   *            - a permitted cross-node write is a direct value mutation of the
   *              target field via the reflection setter — no event is generated
   *              and no ROUTE fans out from the written field. The target still
   *              produces its own output events per the field's access type.
   * @throws std::logic_error on a gated cross-node write with directOutput FALSE.
   */
  void setField(X3DNode *node, const std::string &fieldName, std::any value) {
    requireActive();
    if (!node) return;
    const bool self = node == static_cast<X3DNode *>(&script_);
    if (!self && !script_.getDirectOutput()) {
      // §29.4.1: directOutput FALSE + write to another node -> UNDEFINED.
      // Throwing is the safest conformant response (design §2).
      throw std::logic_error(
          "SaiContext::setField: Script directOutput=FALSE forbids writing "
          "field '" + fieldName + "' on another node");
    }
    if (self) {
      // The Script's own field write is its output channel: post it as a
      // cascade seed so it fans out along ROUTEs (§29.2.4).
      ctx_.postEvent(node, fieldName, std::move(value));
      return;
    }
    // directOutput cross-node write: a direct value mutation, non-routable
    // (SAI §4.5.2 / ADR-0029). writeField applies the reflection setter and the
    // dirty-tracker update atomically, so a failed (type-mismatched) write
    // changes nothing, including author fields (ISO/IEC 19775-1 §29.2.6).
    (void)ctx_.writeField(node, fieldName, std::move(value));
  }

  /**
   * @brief Descriptor of `fieldName` on `node`, static or author-declared.
   * @details Accepts the `set_xxx` / `xxx_changed` aliases of an inputOutput
   *          field (§4.4.2.2); the returned descriptor carries the canonical
   *          name. nullopt if the node has no such field. Backends use this to
   *          marshal a script's `node.field` read or write by field type.
   */
  std::optional<FieldInfo> fieldInfo(const X3DNode &node,
                                     const std::string &fieldName) const {
    requireActive();
    return findField(node, resolveFieldAlias(&node, fieldName));
  }

  // Routing (§29.4.1: dynamic route add/remove only when directOutput==TRUE).

  /**
   * @brief Dynamically add a ROUTE (from -> to). Requires directOutput==TRUE.
   * @details The route is validated with the same rules as a document ROUTE
   *          (X3DSceneBridge): both nodes must exist, both fields must exist
   *          (inputOutput `set_`/`_changed` aliases accepted, §4.4.2.2), the
   *          source must be an output (outputOnly/inputOutput), the sink an
   *          input (inputOnly/inputOutput), and the types must match exactly —
   *          X3D performs no implicit coercion across a ROUTE. The SAI names
   *          these failures INVALID_NODE / INVALID_FIELD (ISO/IEC 19775-2).
   * @throws std::logic_error if directOutput is FALSE.
   * @throws std::invalid_argument on an invalid node, field, direction or type.
   */
  void addRoute(X3DNode *fromNode, const std::string &fromField,
                X3DNode *toNode, const std::string &toField) {
    requireDirectOutput("addRoute");
    validateRoute(fromNode, fromField, toNode, toField);
    ctx_.addRoute(FieldAddress{fromNode, fromField},
                  FieldAddress{toNode, toField});
  }

  /**
   * @brief Dynamically delete a ROUTE (from -> to). Requires directOutput==TRUE.
   *        Deleting a route that does not exist is a no-op.
   * @throws std::logic_error if directOutput is FALSE.
   * @throws std::invalid_argument if either node is null (INVALID_NODE).
   */
  void deleteRoute(X3DNode *fromNode, const std::string &fromField,
                   X3DNode *toNode, const std::string &toField) {
    requireDirectOutput("deleteRoute");
    if (!fromNode || !toNode)
      throw std::invalid_argument(
          "SaiContext::deleteRoute: INVALID_NODE (null or disposed node)");
    ctx_.removeRoute(FieldAddress{fromNode, fromField},
                     FieldAddress{toNode, toField});
  }

  // Scene creation (SAI createX3DFromString / createVrmlFromString).

  /// Turns X3D source text into root nodes. Throws on unparsable text. The
  /// host injects it (ScriptSystem::setSceneFactory); the script layer itself
  /// has no parser.
  using SceneFactory = std::function<MFNode(const std::string &text)>;

  void setSceneFactory(SceneFactory factory) { sceneFactory_ = std::move(factory); }

  /**
   * @brief Parse `text` into new nodes (SAI Browser.createX3DFromString).
   * @details Not gated by directOutput: creating nodes does not change the
   *          scene; inserting them into another node's field does, and that
   *          write is gated by setField. The created roots stay owned by this
   *          SaiContext for the script's lifetime, because a script's node
   *          handle does not keep a node alive (ADR-0048) and the script may
   *          insert them at any later time.
   * @throws std::logic_error if the host supplied no scene factory.
   * @throws std::exception whatever the factory throws for invalid text.
   */
  MFNode createX3DFromString(const std::string &text) {
    requireActive();
    if (!sceneFactory_)
      throw std::logic_error(
          "SaiContext::createX3DFromString: the host supplied no scene factory");
    MFNode nodes = sceneFactory_(text);
    for (const auto &node : nodes)
      if (node) createdNodes_.push_back(node);
    return nodes;
  }

  // Browser info (§29.4.1; SAI Browser object).

  /** @brief Browser name (SAI Browser.getName()). */
  const std::string &getName() const { return name_; }

  /** @brief Browser version (SAI Browser.getVersion()). */
  const std::string &getVersion() const { return version_; }

  /** @brief Current scene time = the execution context clock (§29.2). */
  double currentTime() const { requireActive(); return ctx_.now(); }

  /** @brief Current frame rate (frames/sec). 0 until the embedder supplies it. */
  double currentFrameRate() const { return frameRate_; }

  /** @brief Set the reported frame rate (the embedder/ScriptSystem updates it). */
  void setCurrentFrameRate(double fps) { frameRate_ = fps; }

  /** @brief SAI print/console output. Default sink is the captured log. */
  void print(const std::string &message) { log_ += message; }

  /** @brief Accumulated print() output (test/inspection surface). */
  const std::string &log() const { return log_; }

  /** @brief The owning Script node. */
  x3d::nodes::Script &script() const { requireActive(); return script_; }

  /** @brief The execution context this SAI acts on. */
  X3DExecutionContext &context() const { requireActive(); return ctx_; }

  /** @brief This script's SFNode handle table (see NodeHandleTable.hpp). The
   *         engine backends intern every node they hand to script code here and
   *         resolve script-supplied handles back through it. */
  NodeHandleTable &nodeHandles() { requireActive(); return nodeHandles_; }

private:
  void requireActive() const {
    if (!active_()) throw std::logic_error("SaiContext: execution context retired");
  }

  void requireDirectOutput(const char *op) const {
    requireActive();
    if (!script_.getDirectOutput()) {
      throw std::logic_error(std::string("SaiContext::") + op +
          ": Script directOutput=FALSE forbids dynamic route changes");
    }
  }

  void validateRoute(X3DNode *fromNode, const std::string &fromField,
                            X3DNode *toNode, const std::string &toField) {
    if (!fromNode || !toNode)
      throw std::invalid_argument(
          "SaiContext::addRoute: INVALID_NODE (null or disposed node)");
    std::optional<FieldInfo> from =
        findField(*fromNode, resolveFieldAlias(fromNode, fromField));
    if (!from)
      throw std::invalid_argument("SaiContext::addRoute: INVALID_FIELD '" +
                                  fromField + "' on " +
                                  fromNode->nodeTypeName());
    std::optional<FieldInfo> to =
        findField(*toNode, resolveFieldAlias(toNode, toField));
    if (!to)
      throw std::invalid_argument("SaiContext::addRoute: INVALID_FIELD '" +
                                  toField + "' on " + toNode->nodeTypeName());
    if (from->access != AccessType::OutputOnly &&
        from->access != AccessType::InputOutput)
      throw std::invalid_argument("SaiContext::addRoute: source field '" +
                                  fromField + "' is not an output");
    if (to->access != AccessType::InputOnly &&
        to->access != AccessType::InputOutput)
      throw std::invalid_argument("SaiContext::addRoute: sink field '" +
                                  toField + "' is not an input");
    if (from->type != to->type)
      throw std::invalid_argument("SaiContext::addRoute: type mismatch '" +
                                  fromField + "' -> '" + toField + "'");
  }

  // Include static and author fields. Return a copy because effectiveFields()
  // builds a temporary table; a pointer into it would dangle. nullopt if absent.
  std::optional<FieldInfo> findField(const X3DNode &node,
                                            const std::string &name) const {
    for (FieldInfo &info : effectiveFields(node, ctx_.authorFields())) {
      if (info.x3dName == name) return std::move(info);
    }
    return std::nullopt;
  }

  X3DExecutionContext &ctx_;
  x3d::nodes::Script &script_;
  std::string name_;
  std::string version_;
  double frameRate_ = 0.0;
  std::string log_;
  NodeHandleTable nodeHandles_;
  SceneFactory sceneFactory_;
  MFNode createdNodes_;  // roots made by createX3DFromString (see there)
  std::function<bool()> active_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SAI_CONTEXT_HPP
