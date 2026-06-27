// X3DStatement.hpp
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
  /**
   * @brief Default constructor for X3DStatement
   */
  X3DStatement() = default;

  /**
   * @brief Virtual destructor for X3DStatement
   */
  virtual ~X3DStatement() = default;

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
  static std::string componentName() { return "Core"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

private:
};

} // namespace x3d::nodes
