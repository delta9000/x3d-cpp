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
  X3DProgrammableShaderObject() = default;

  virtual ~X3DProgrammableShaderObject() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Shaders"; }

  static int componentLevel() { return 1; }

private:
};

} // namespace x3d::nodes
