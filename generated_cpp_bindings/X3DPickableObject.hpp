// X3DPickableObject.hpp
#ifndef X3DPICKABLEOBJECT_HPP
#define X3DPICKABLEOBJECT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

/**
 * @class X3DPickableObject
 * @brief The X3DPickableObject abstract interface marks a node as being capable
 * of having customized picking performed on its contents or children.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#X3DPickableObject
 */
class X3DPickableObject {
public:
  /**
   * @brief Default constructor for X3DPickableObject
   */
  X3DPickableObject() = default;

  /**
   * @brief Virtual destructor for X3DPickableObject
   */
  virtual ~X3DPickableObject() = default;

  /**
   * @brief Get the default value for pickable
   * @return SFBool The default value
   */
  static SFBool getDefaultPickable() { return true; }

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
  static std::string componentName() { return "Picking"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of pickable. AccessType: inputOutput
   * @details
   * @return SFBool The current value of pickable.
   */
  SFBool getPickable() const { return _pickable; }

  /**
   * @brief Sets the value of pickable. AccessType: inputOutput
   * @details
   * @param value The new value for pickable.
   */
  void setPickable(const SFBool &value) { _pickable = value; }

private:
  /**
   * @brief Member variable for pickable.
   */

  SFBool _pickable{true};
};

#endif // X3DPICKABLEOBJECT_HPP