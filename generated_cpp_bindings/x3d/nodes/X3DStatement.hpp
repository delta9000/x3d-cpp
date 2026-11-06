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
 * @class X3DStatement
 * @brief X3DStatement is a marker interface that identifies statements relating
 * to nonrenderable scene graph structure. X3DStatement does not extend from any
 * other node type since it is not an explicit part of the X3D node interface
 * hierarchy, and DEF/USE is not appropriate for such statements.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#scenegraph
 */
class X3DStatement {
public:
  X3DStatement() = default;

  virtual ~X3DStatement() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

private:
};

} // namespace x3d::nodes
