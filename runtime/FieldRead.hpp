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
#ifndef X3D_RUNTIME_FIELD_READ_HPP
#define X3D_RUNTIME_FIELD_READ_HPP

#include "x3d/nodes/X3DNode.hpp"

#include <any>
#include <cassert>
#include <string>

namespace x3d::runtime {

/// The value in `v` as a T, or nullptr if `v` is empty. The pointer aliases
/// `v`, so keep `v` alive while using it.
template <class T> const T *fieldValueAs(const std::any &v) {
  const T *p = std::any_cast<T>(&v);
  assert((p || !v.has_value()) &&
         "fieldValueAs: field value boxed as an unexpected type");
  return p;
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
