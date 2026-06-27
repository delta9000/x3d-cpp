// X3DProgrammableShaderObject.hpp
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
 * @class X3DProgrammableShaderObject
 * @brief Base type for all nodes that specify arbitrary fields for interfacing
 * with per-object attribute values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#X3DProgrammableShaderObject
 */
class X3DProgrammableShaderObject {
public:
  /**
   * @brief Default constructor for X3DProgrammableShaderObject
   */
  X3DProgrammableShaderObject() = default;

  /**
   * @brief Virtual destructor for X3DProgrammableShaderObject
   */
  virtual ~X3DProgrammableShaderObject() = default;

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shaders"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

private:
};

} // namespace x3d::nodes
