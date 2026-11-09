#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DPickableObject
 * @brief The X3DPickableObject abstract interface marks a node as being capable
 * of having customized picking performed on its contents or children.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#X3DPickableObject
 */
class X3DPickableObject {
public:
  X3DPickableObject() = default;

  virtual ~X3DPickableObject() = default;

  static SFBool getDefaultPickable() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Picking"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getPickable() const { return _pickable; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setPickable(const SFBool &value) { _pickable = value; }

private:
  SFBool _pickable{true};
};

} // namespace x3d::nodes
