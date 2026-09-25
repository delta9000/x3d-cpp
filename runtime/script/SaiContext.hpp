// SaiContext.hpp
// The in-process Scene Access Interface surface a Script backend calls back into
// (ISO/IEC 19775-1 §29.4.1, §4.9; ISO 19775-2). Read/set fields, add/delete
// routes, query browser info, print. This is the ONLY sanctioned channel from a
// backend into the running scene, and it enforces the Script's directOutput gate.
//
// LANGUAGE-AGNOSTIC: every value crossing this surface is the runtime's own
// std::any field representation, never a scripting-language object.
//
// directOutput / addRoute / deleteRoute gates (spec-checked, §29.2.6 + §29.4.1):
//   - getField(node, name)        : always permitted (read access is unconditional).
//   - setField(self, name, value) : always permitted (a script may write its own
//                                   fields — chiefly its outputOnly outputs).
//   - setField(other, name, value): permitted ONLY if directOutput==TRUE; else the
//                                   spec declares the result UNDEFINED, and we
//                                   take the safest conformant response: throw.
//   - addRoute / deleteRoute      : permitted ONLY if directOutput==TRUE; else throw.
// A permitted setField is injected as an input event into the CURRENT cascade at
// the current timestamp (§29.2.6: such writes "shall be part of the current event
// cascade"), not a back-door reflection poke.
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
 *          runtime guarantees both outlive it.
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
        version_(std::move(version)) {}

  // --------------------------------------------------------------------------
  // Field access (§29.4.1 paragraph 1: read is unconditional).
  // --------------------------------------------------------------------------

  /**
   * @brief Read the last value of `fieldName` on `node` (boxed std::any).
   * @details Permitted regardless of directOutput. Returns an empty std::any if
   *          the node is null, the field is unknown, or the field is not readable
   *          (e.g. an inputOnly field has no get thunk).
   */
  std::any getField(X3DNode *node, const std::string &fieldName) const {
    if (!node) return {};
    std::optional<FieldInfo> info = findField(*node, fieldName);
    if (!info || !info->get) return {};
    return info->get(*node);
  }

  /**
   * @brief Set `fieldName` on `node` to `value`, as an event in this cascade.
   * @details Gate (§29.2.6/§29.4.1):
   *            - node == owning Script: always allowed (its own fields/outputs).
   *            - node != owning Script: allowed ONLY if directOutput==TRUE;
   *              otherwise throws std::logic_error (spec says "undefined").
   *          A permitted write is posted as an input event into the current
   *          cascade so it carries the current timestamp and fans out via ROUTEs.
   * @throws std::logic_error on a gated cross-node write with directOutput FALSE.
   */
  void setField(X3DNode *node, const std::string &fieldName, std::any value) {
    if (!node) return;
    if (node != static_cast<X3DNode *>(&script_) && !script_.getDirectOutput()) {
      // §29.4.1: directOutput FALSE + write to another node -> UNDEFINED.
      // Throwing is the safest conformant response (design §2).
      throw std::logic_error(
          "SaiContext::setField: Script directOutput=FALSE forbids writing "
          "field '" + fieldName + "' on another node");
    }
    ctx_.postEvent(node, fieldName, std::move(value));
  }

  // --------------------------------------------------------------------------
  // Routing (§29.4.1: dynamic route add/remove only when directOutput==TRUE).
  // --------------------------------------------------------------------------

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

  // --------------------------------------------------------------------------
  // Browser info (§29.4.1; SAI Browser object).
  // --------------------------------------------------------------------------

  /** @brief Browser name (SAI Browser.getName()). */
  const std::string &getName() const { return name_; }

  /** @brief Browser version (SAI Browser.getVersion()). */
  const std::string &getVersion() const { return version_; }

  /** @brief Current scene time = the execution context clock (§29.2). */
  double currentTime() const { return ctx_.now(); }

  /** @brief Current frame rate (frames/sec). 0 until the embedder supplies it. */
  double currentFrameRate() const { return frameRate_; }

  /** @brief Set the reported frame rate (the embedder/ScriptSystem updates it). */
  void setCurrentFrameRate(double fps) { frameRate_ = fps; }

  /** @brief SAI print/console output. Default sink is the captured log. */
  void print(const std::string &message) { log_ += message; }

  /** @brief Accumulated print() output (test/inspection surface). */
  const std::string &log() const { return log_; }

  /** @brief The owning Script node. */
  x3d::nodes::Script &script() const { return script_; }

  /** @brief The execution context this SAI acts on. */
  X3DExecutionContext &context() const { return ctx_; }

  /** @brief This script's SFNode handle table (see NodeHandleTable.hpp). The
   *         engine backends intern every node they hand to script code here and
   *         resolve script-supplied handles back through it. */
  NodeHandleTable &nodeHandles() { return nodeHandles_; }

private:
  void requireDirectOutput(const char *op) const {
    if (!script_.getDirectOutput()) {
      throw std::logic_error(std::string("SaiContext::") + op +
          ": Script directOutput=FALSE forbids dynamic route changes");
    }
  }

  static void validateRoute(X3DNode *fromNode, const std::string &fromField,
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

  // Locate a field on a node by x3dName via its EFFECTIVE table: static fields()
  // plus Script author fields from the dynamic-field store (S1). Returns a COPY
  // (std::optional) because effectiveFields() builds a temporary table holding
  // the synthesized author FieldInfos — a pointer into it would dangle.
  // std::nullopt if absent. (S1 NOTE closed: author fields now resolve here, so
  // script get/set/addRoute see them.)
  static std::optional<FieldInfo> findField(const X3DNode &node,
                                            const std::string &name) {
    for (FieldInfo &info : effectiveFields(node)) {
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
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SAI_CONTEXT_HPP
