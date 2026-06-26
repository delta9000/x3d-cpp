// X3DArrayField.hpp
#ifndef X3DARRAYFIELD_HPP
#define X3DARRAYFIELD_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

/**
 * @class X3DArrayField
 * @brief X3DArrayField is the abstract field type from which all field types
 * that can contain multiple values are derived, implementing the X3DField
 * interface. All fields derived from X3DArrayField have names beginning with MF
 * (multiple-valued field). MF fields may zero or more values, each of which
 * shall be of the type indicated by the corresponding SF field type. It is
 * illegal for any MF field to mix values of different SF field types.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html#X3DArrayField
 */
class X3DArrayField {
public:
  /**
   * @brief Default constructor for X3DArrayField
   */
  X3DArrayField() = default;

  /**
   * @brief Virtual destructor for X3DArrayField
   */
  virtual ~X3DArrayField() = default;

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

private:
};

#endif // X3DARRAYFIELD_HPP