// X3DMetadataObject.hpp
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
 * @class X3DMetadataObject
 * @brief Each node inheriting the X3DMetadataObject interface contains a single
 * array of strictly typed values: MFBool, MFInt32, MFFloat, MFDouble, MFString,
 * or MFNode, the latter having children that are all Metadata nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DMetadataObject
 */
class X3DMetadataObject {
public:
  /**
   * @brief Default constructor for X3DMetadataObject
   */
  X3DMetadataObject() = default;

  /**
   * @brief Virtual destructor for X3DMetadataObject
   */
  virtual ~X3DMetadataObject() = default;

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

  /**
   * @brief Gets the value of name. AccessType: inputOutput
   * @details
   * @return SFString The current value of name.
   */
  SFString getName() const { return _name; }

  /**
   * @brief Sets the value of name. AccessType: inputOutput
   * @details
   * @param value The new value for name.
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * @brief Gets the value of reference. AccessType: inputOutput
   * @details
   * @return SFString The current value of reference.
   */
  SFString getReference() const { return _reference; }

  /**
   * @brief Sets the value of reference. AccessType: inputOutput
   * @details
   * @param value The new value for reference.
   */
  void setReference(const SFString &value) { _reference = value; }

  void setReference(SFString &&value) { _reference = std::move(value); }

private:
  /**
   * @brief Member variable for name.
   */

  SFString _name{};

  /**
   * @brief Member variable for reference.
   */

  SFString _reference{};
};

} // namespace x3d::nodes
