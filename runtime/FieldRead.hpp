// FieldRead.hpp — exception-free reads of reflected field values.
// namespace x3d::runtime.
//
// Reflection hands field values out as std::any. The throwing form,
// std::any_cast<T>(value), invites a try/catch(...) around every read, and such a
// catch-all also swallows real errors (bad_alloc, an exception from a nested
// walk), so a boxing bug or an out-of-memory silently drops part of the scene.
// These helpers use the pointer form instead and throw nothing of their own.
//
// A value that is PRESENT but boxed as a different type than its X3DFieldType
// promises is a boxing-invariant violation (a generator or reader bug, like
// geombounds::FieldRead::TypeMismatch): it asserts in debug builds so tests fail
// loud, and reads as absent in release so production degrades gracefully.
//
// Zero-copy reads (ADR-0049): fieldPtr<T> borrows a generated field's stored
// member through FieldInfo::view; FieldRef<T> does the same and falls back to a
// boxed get() for fields with nothing to borrow (synthesized author fields);
// forEachChildNode walks a node's SFNode/MFNode children without copying a
// child vector or touching a refcount.
#ifndef X3D_RUNTIME_FIELD_READ_HPP
#define X3D_RUNTIME_FIELD_READ_HPP

#include "x3d/nodes/X3DNode.hpp"

#include <any>
#include <cassert>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

namespace x3d::runtime {

/// The value in `v` as a T, or nullptr if `v` is empty. The pointer aliases
/// `v`, so keep `v` alive while using it.
template <class T> const T *fieldValueAs(const std::any &v) {
  const T *p = std::any_cast<T>(&v);
  assert((p || !v.has_value()) &&
         "fieldValueAs: field value boxed as an unexpected type");
  return p;
}

/// A borrowed pointer to `f`'s stored value in `n` as a T, or nullptr if the
/// field has no view or its value is not a T. No boxing, no copy; the pointer
/// stays valid while `n` lives, and a later write to the field changes what it
/// reads.
template <class T>
const T *fieldPtr(const x3d::nodes::X3DNode &n, const x3d::core::FieldInfo &f) {
  if (!f.view) return nullptr;
  const x3d::core::FieldView v = f.view(n);
  if (!v.data || !v.type) return nullptr;
  const bool match = *v.type == typeid(T);
  assert(match && "fieldPtr: field viewed as an unexpected type");
  return match ? static_cast<const T *>(v.data) : nullptr;
}

/// Read access to a field's value that borrows when it can (fieldPtr) and boxes
/// through get() only when it must. Falsy if the field is unreadable or not a T.
/// Not copyable or movable: the fallback pointer aims into its own box.
template <class T> class FieldRef {
public:
  FieldRef(const x3d::nodes::X3DNode &n, const x3d::core::FieldInfo &f) {
    if ((ptr_ = fieldPtr<T>(n, f))) return;
    if (f.view || !f.get) return;  // viewable but mistyped, or unreadable
    box_ = f.get(n);
    ptr_ = fieldValueAs<T>(box_);
  }
  FieldRef(const FieldRef &) = delete;
  FieldRef &operator=(const FieldRef &) = delete;

  explicit operator bool() const { return ptr_ != nullptr; }
  const T *get() const { return ptr_; }
  const T &operator*() const { return *ptr_; }
  const T *operator->() const { return ptr_; }

private:
  std::any box_;
  const T *ptr_ = nullptr;
};

/// Call `fn(field, child)` for every non-null child in `n`'s readable SFNode
/// and MFNode fields, in field-table order (`child` is a
/// `const std::shared_ptr<X3DNode>&`). Borrows each field's value; never copies
/// a child vector. `fn` must not write the field it is being called for.
template <class Fn> void forEachChildNode(const x3d::nodes::X3DNode &n, Fn &&fn) {
  using x3d::core::X3DFieldType;
  using NodePtr = std::shared_ptr<x3d::nodes::X3DNode>;
  for (const auto &f : n.fields()) {
    if (f.type == X3DFieldType::SFNode) {
      FieldRef<NodePtr> c(n, f);
      if (c && *c) fn(f, *c);
    } else if (f.type == X3DFieldType::MFNode) {
      FieldRef<std::vector<NodePtr>> cs(n, f);
      if (!cs) continue;
      for (const auto &c : *cs)
        if (c) fn(f, c);
    }
  }
}

/// An SFEnum/MFEnum field's X3D token string(s) via FieldInfo::getEnumString,
/// which keeps callers decoupled from the generated enum-class types. `dflt` if
/// the field is absent, is not an enum, or its token string is empty.
inline std::string enumToken(const x3d::nodes::X3DNode &n, const char *name,
                             const std::string &dflt = {}) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) {
      if (!f.getEnumString) return dflt;
      std::string s = f.getEnumString(n);
      return s.empty() ? dflt : s;
    }
  return dflt;
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_FIELD_READ_HPP
