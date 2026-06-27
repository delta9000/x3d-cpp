// X3DField.hpp
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
 * @class X3DField
 * @brief X3DField is the abstract field type from which all single values field
 * types are derived. All fields directly derived from X3DField have names
 * beginning with SF (single-valued field). SF fields may only contain a single
 * value of the type indicated by the name of the field type.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html#X3DField
 */
class X3DField {
public:
  /**
   * @brief Default constructor for X3DField
   */
  X3DField() = default;

  /**
   * @brief Virtual destructor for X3DField
   */
  virtual ~X3DField() = default;

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

private:
};

} // namespace x3d::nodes
