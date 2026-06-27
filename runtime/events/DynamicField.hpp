// DynamicField.hpp
// S1 dynamic-field foundation: the reader->runtime contract for author-declared
// Script fields and the per-node side-table that makes them visible to every
// consumer that resolves fields through reflection.
//
// WHY A SIDE-TABLE (not a generator change). The generated reflection layer is
// codegen-frozen (golden byte-identical). Author <field> declarations are a
// Script-local concern, so rather than regenerate every X3DNode with a virtual
// effectiveFields() + an instance store (which would churn golden), we keep a
// per-node side-table keyed by node identity (const X3DNode*) — the same memo
// pattern as TransformSystem.world_ / BoundsSystem / PickSystem path cache.
// Lifetime: author fields live as long as their Script node, which the document
// owns, so the const X3DNode* key never dangles in the load->tick->extract model
// (see design §3.2).
//
// SEAM 1 — AuthorFieldDecl: a neutral, encoding-agnostic struct every reader
//   (XML/ClassicVRML/VRML97/JSON) emits for one author field declaration.
// SEAM 2 — DynamicFieldStore + effectiveFields(node): the storage model + the
//   "static fields() concatenated with the node's author fields" view that
//   X3DSceneBridge::findField and SaiContext::findField switch to.
#ifndef X3D_RUNTIME_DYNAMIC_FIELD_HPP
#define X3D_RUNTIME_DYNAMIC_FIELD_HPP

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3DReflection.hpp"

#include <any>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

using namespace x3d::core;

// AUD-MEM-1: validate that a boxed std::any value matches its declared X3DFieldType.
inline bool anyMatchesFieldType(const std::any &value, X3DFieldType type) {
  switch (type) {
  case X3DFieldType::SFBool:       return value.type() == typeid(bool);
  case X3DFieldType::SFColor:      return value.type() == typeid(SFColor);
  case X3DFieldType::SFColorRGBA:  return value.type() == typeid(SFColorRGBA);
  case X3DFieldType::SFDouble:     return value.type() == typeid(double);
  case X3DFieldType::SFFloat:      return value.type() == typeid(float);
  case X3DFieldType::SFImage:      return value.type() == typeid(SFImage);
  case X3DFieldType::SFInt32:      return value.type() == typeid(int);
  case X3DFieldType::SFMatrix3d:   return value.type() == typeid(SFMatrix3d);
  case X3DFieldType::SFMatrix3f:   return value.type() == typeid(SFMatrix3f);
  case X3DFieldType::SFMatrix4d:   return value.type() == typeid(SFMatrix4d);
  case X3DFieldType::SFMatrix4f:   return value.type() == typeid(SFMatrix4f);
  case X3DFieldType::SFNode:       return value.type() == typeid(std::shared_ptr<X3DNode>);
  case X3DFieldType::SFRotation:   return value.type() == typeid(SFRotation);
  case X3DFieldType::SFString:     return value.type() == typeid(std::string);
  case X3DFieldType::SFTime:       return value.type() == typeid(double);
  case X3DFieldType::SFVec2d:      return value.type() == typeid(SFVec2d);
  case X3DFieldType::SFVec2f:      return value.type() == typeid(SFVec2f);
  case X3DFieldType::SFVec3d:      return value.type() == typeid(SFVec3d);
  case X3DFieldType::SFVec3f:      return value.type() == typeid(SFVec3f);
  case X3DFieldType::SFVec4d:      return value.type() == typeid(SFVec4d);
  case X3DFieldType::SFVec4f:      return value.type() == typeid(SFVec4f);
  case X3DFieldType::MFBool:       return value.type() == typeid(std::vector<bool>);
  case X3DFieldType::MFColor:      return value.type() == typeid(std::vector<SFColor>);
  case X3DFieldType::MFColorRGBA:  return value.type() == typeid(std::vector<SFColorRGBA>);
  case X3DFieldType::MFDouble:     return value.type() == typeid(std::vector<double>);
  case X3DFieldType::MFFloat:      return value.type() == typeid(std::vector<float>);
  case X3DFieldType::MFImage:      return value.type() == typeid(std::vector<SFImage>);
  case X3DFieldType::MFInt32:      return value.type() == typeid(std::vector<int>);
  case X3DFieldType::MFMatrix3d:   return value.type() == typeid(std::vector<SFMatrix3d>);
  case X3DFieldType::MFMatrix3f:   return value.type() == typeid(std::vector<SFMatrix3f>);
  case X3DFieldType::MFMatrix4d:   return value.type() == typeid(std::vector<SFMatrix4d>);
  case X3DFieldType::MFMatrix4f:   return value.type() == typeid(std::vector<SFMatrix4f>);
  case X3DFieldType::MFNode:       return value.type() == typeid(std::vector<std::shared_ptr<X3DNode>>);
  case X3DFieldType::MFRotation:   return value.type() == typeid(std::vector<SFRotation>);
  case X3DFieldType::MFString:     return value.type() == typeid(std::vector<std::string>);
  case X3DFieldType::MFTime:       return value.type() == typeid(std::vector<double>);
  case X3DFieldType::MFVec2d:      return value.type() == typeid(std::vector<SFVec2d>);
  case X3DFieldType::MFVec2f:      return value.type() == typeid(std::vector<SFVec2f>);
  case X3DFieldType::MFVec3d:      return value.type() == typeid(std::vector<SFVec3d>);
  case X3DFieldType::MFVec3f:      return value.type() == typeid(std::vector<SFVec3f>);
  case X3DFieldType::MFVec4d:      return value.type() == typeid(std::vector<SFVec4d>);
  case X3DFieldType::MFVec4f:      return value.type() == typeid(std::vector<SFVec4f>);
  case X3DFieldType::SFEnum:       return value.type() == typeid(std::string);
  case X3DFieldType::MFEnum:       return value.type() == typeid(std::vector<std::string>);
  default:                          return false;
  }
}

/**
 * @brief Seam 1: one author-declared field, encoding-agnostic.
 * @details Readers parse `<field name accessType type value>` (XML) /
 *          `field SFFloat x 0.5` (VRML) / a JSON field member into this neutral
 *          struct, decoupling the four codecs from the storage model. The four
 *          access keywords map: XML `accessType` attribute verbatim; VRML
 *          `field`->InitializeOnly (or InputOutput per context), `eventIn`->
 *          InputOnly, `eventOut`->OutputOnly.
 */
struct AuthorFieldDecl {
  std::string x3dName;       ///< e.g. "fraction", "on"
  X3DFieldType type{};       ///< SFFloat, MFVec3f, ...
  AccessType access{};       ///< InputOnly | OutputOnly | InitializeOnly | InputOutput
  std::any initialValue;     ///< boxed default; empty for inputOnly/outputOnly
};

/**
 * @brief Seam 2: per-node side-table of synthesized author FieldInfos + values.
 * @details Keyed by node identity (const X3DNode*). For each node it holds:
 *            - the live boxed std::any value store (one entry per author field),
 *            - a vector of synthesized FieldInfo entries whose get/set thunks
 *              read/write that boxed value store.
 *          The synthesized FieldInfo obeys the existing reflection contract
 *          exactly (X3DReflection.hpp FieldInfo): `get` is empty for inputOnly,
 *          `set` is empty for read-only (initializeOnly/outputOnly); the cascade
 *          and ROUTE wiring treat author fields exactly like generated ones.
 *
 *          Thread-safety: a process-global instance is exposed via
 *          dynamicFieldStore(); the table is guarded by an internal mutex so
 *          concurrent reader population is safe. The synthesized get/set thunks
 *          capture a shared_ptr to the per-node value store, so they remain
 *          valid for the node's lifetime independent of rehash.
 */
class DynamicFieldStore {
public:
  /**
   * @brief Add author fields to `node` from a vector of decls.
   * @details Idempotent per (node, x3dName): re-adding a name overwrites that
   *          node's existing entry (last writer wins) rather than duplicating
   *          it, so a reader may call this incrementally. Synthesizes one
   *          FieldInfo per decl with get/set thunks bound to the per-node value
   *          store. The boxed initialValue seeds the value store (empty std::any
   *          for inputOnly/outputOnly, which have no persistent value).
   */
  void addAuthorFields(const X3DNode &node,
                       const std::vector<AuthorFieldDecl> &decls) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<NodeEntry> entry = entryFor(&node);
    for (const AuthorFieldDecl &decl : decls) {
      addOne(*entry, decl);
    }
  }

  /**
   * @brief Add a single author field to `node` (last-writer-wins per name).
   */
  void addAuthorField(const X3DNode &node, const AuthorFieldDecl &decl) {
    std::lock_guard<std::mutex> lock(mutex_);
    addOne(*entryFor(&node), decl);
  }

  /**
   * @brief The node's author FieldInfo list (empty if the node has none).
   * @details Returns by value (a copy) so callers can concatenate it onto the
   *          static fields() table without holding the store lock. The thunks in
   *          each FieldInfo capture the live value store, so the copy stays
   *          functional.
   */
  std::vector<FieldInfo> authorFields(const X3DNode &node) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_.find(&node);
    if (it == table_.end()) return {};
    return it->second->infos;
  }

  /** @brief True if `node` has at least one author field. */
  bool hasAuthorFields(const X3DNode &node) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_.find(&node);
    return it != table_.end() && !it->second->infos.empty();
  }

  /**
   * @brief Read a boxed author-field value by (node, name).
   * @details Empty std::any if the node/name is unknown or the field is
   *          write-only (inputOnly carries no persistent value). This is the
   *          direct store read; the synthesized FieldInfo::get thunk routes here.
   */
  std::any getValue(const X3DNode &node, const std::string &name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_.find(&node);
    if (it == table_.end()) return {};
    auto vIt = it->second->values.find(name);
    return vIt == it->second->values.end() ? std::any{} : vIt->second;
  }

  /**
   * @brief Write a boxed author-field value by (node, name). No-op if unknown.
   * @details The direct store write; the synthesized FieldInfo::set thunk routes
   *          here. Writing an inputOnly field is permitted (an inputOnly field's
   *          set thunk records the last received event value here even though it
   *          has no get); a read-only (outputOnly/initializeOnly) field has no
   *          set thunk so the cascade never calls this for it.
   */
  void setValue(const X3DNode &node, const std::string &name, std::any value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_.find(&node);
    if (it == table_.end()) return;
    auto declIt = it->second->declared.find(name);
    if (declIt == it->second->declared.end()) return;
    // AUD-MEM-1: reject mismatched types at the API boundary rather than
    // deferring the error to a later bad_any_cast in getValue. This is a
    // hardened boundary — it must tolerate arbitrary mismatched input without
    // crashing (hostile/buggy callers), so the drop is non-fatal. But a silent
    // drop is near-impossible to trace, so we COUNT drops: a nonzero
    // typeMismatchDrops() flags either bad input or (more usefully) an internal
    // boxing-invariant violation where a producer boxed the wrong C++ type for
    // the declared X3DFieldType.
    const FieldInfo &fi = it->second->infos[declIt->second];
    if (!anyMatchesFieldType(value, fi.type)) {
      ++typeMismatchDrops_;
      return;
    }
    it->second->values[name] = std::move(value);
  }

  /** @brief Drop all author fields for `node` (dynamic removal; out-of-scope but
   *         provided so a future node-destroy path can keep the table tidy). */
  void erase(const X3DNode &node) {
    std::lock_guard<std::mutex> lock(mutex_);
    table_.erase(&node);
  }

  /** @brief Drop the entire table (test isolation). */
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    table_.clear();
    typeMismatchDrops_ = 0;
  }

  /**
   * @brief AUD-MEM-1: number of setValue() writes dropped for a type mismatch.
   * @details Makes the otherwise-silent drop observable. A nonzero value flags
   *          either hostile/buggy input or an internal boxing-invariant
   *          violation (a producer boxed the wrong C++ type for the declared
   *          X3DFieldType). Reset by clear().
   */
  std::size_t typeMismatchDrops() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return typeMismatchDrops_;
  }

private:
  /// Per-node state: the live value store + the synthesized FieldInfos. Held by
  /// shared_ptr so the get/set thunks can capture it and survive table rehash.
  struct NodeEntry {
    std::unordered_map<std::string, std::any> values;  ///< live boxed values
    std::unordered_map<std::string, std::size_t> declared; ///< name -> infos idx
    std::vector<FieldInfo> infos;                      ///< synthesized reflection
  };

  std::shared_ptr<NodeEntry> entryFor(const X3DNode *node) {
    auto it = table_.find(node);
    if (it != table_.end()) return it->second;
    auto entry = std::make_shared<NodeEntry>();
    table_.emplace(node, entry);
    return entry;
  }

  // Synthesize one FieldInfo obeying the reflection contract and seed its value.
  // Caller holds mutex_.
  static void addOne(NodeEntry &entry, const AuthorFieldDecl &decl) {
    const bool readable =
        decl.access == AccessType::OutputOnly ||
        decl.access == AccessType::InputOutput ||
        decl.access == AccessType::InitializeOnly;
    const bool writable =
        decl.access == AccessType::InputOnly ||
        decl.access == AccessType::InputOutput;

    // Seed the live value store. inputOnly/outputOnly start with no persistent
    // value (empty std::any); initializeOnly/inputOutput take the boxed default.
    if (decl.access == AccessType::InitializeOnly ||
        decl.access == AccessType::InputOutput) {
      entry.values[decl.x3dName] = decl.initialValue;
    } else {
      entry.values[decl.x3dName] = std::any{};
    }

    FieldInfo info;
    info.x3dName = decl.x3dName;
    info.type = decl.type;
    info.access = decl.access;
    // get/set route through the per-node value store. They locate the node's
    // entry afresh on each call (the store outlives all nodes), keyed by the
    // node passed in — which IS the entry's node, so a by-name map lookup on the
    // captured entry pointer is correct and lock-free at call time.
    NodeEntry *entryPtr = &entry;
    const std::string name = decl.x3dName;
    if (readable) {
      info.get = [entryPtr, name](const X3DNode &) -> std::any {
        auto vIt = entryPtr->values.find(name);
        return vIt == entryPtr->values.end() ? std::any{} : vIt->second;
      };
    }
    if (writable) {
      info.set = [entryPtr, name](X3DNode &, const std::any &value) {
        entryPtr->values[name] = value;
      };
    }

    auto dIt = entry.declared.find(decl.x3dName);
    if (dIt != entry.declared.end()) {
      entry.infos[dIt->second] = std::move(info);  // last-writer-wins
    } else {
      entry.declared[decl.x3dName] = entry.infos.size();
      entry.infos.push_back(std::move(info));
    }
  }

  mutable std::mutex mutex_;
  std::unordered_map<const X3DNode *, std::shared_ptr<NodeEntry>> table_;
  std::size_t typeMismatchDrops_ = 0; ///< AUD-MEM-1: count of dropped writes
};

/**
 * @brief Process-global author-field store.
 * @details effectiveFields() and both findField sites read this single shared
 *          store. CHOICE (design §3.1): a global accessor — rather than a store
 *          ref threaded through every consumer — because X3DSceneBridge is a
 *          stateless free-function bridge and SaiContext is constructed per
 *          Script with no store handle; a shared global integrates cleanly with
 *          both without widening their constructors or signatures, and matches
 *          the document-scoped lifetime of author fields. Readers populate it;
 *          consumers read it. Tests call clear() for isolation.
 */
inline DynamicFieldStore &dynamicFieldStore() {
  static DynamicFieldStore store;
  return store;
}

/**
 * @brief The node's effective field table: static fields() + author fields.
 * @details Returns a freshly-built FieldTable that is the node's generated
 *          reflection table concatenated with its author FieldInfos from the
 *          process-global store. Consumers that must see author fields
 *          (ROUTE endpoint resolution in X3DSceneBridge, script get/set/route in
 *          SaiContext) resolve through this instead of node.fields(). Geometry/
 *          extraction/bounds/range-validate/material/texture sites stay on
 *          node.fields() — they operate on generated nodes only (design §3.1).
 *
 *          A free function (not an X3DNode method) precisely because the storage
 *          is a side-table: keeping it out of the generated node preserves the
 *          golden-byte-identical invariant.
 */
inline FieldTable effectiveFields(const X3DNode &node) {
  FieldTable table = node.fields();  // copy of the static table
  std::vector<FieldInfo> author = dynamicFieldStore().authorFields(node);
  table.insert(table.end(), std::make_move_iterator(author.begin()),
               std::make_move_iterator(author.end()));
  return table;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DYNAMIC_FIELD_HPP
