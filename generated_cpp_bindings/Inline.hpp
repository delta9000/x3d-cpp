// Inline.hpp
#ifndef INLINE_HPP
#define INLINE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DUrlObject.hpp"

/**
 * @class Inline
 * @brief Inline can load another X3D or VRML model into the current scene via
 * url.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/networking.html#Inline
 */
class Inline : public virtual X3DChildNode,
               public virtual X3DBoundedObject,
               public virtual X3DUrlObject {
public:
  /**
   * @brief Default constructor for Inline
   */
  Inline() = default;

  /**
   * @brief Destructor for Inline
   */
  ~Inline() = default;

  /**
   * @brief Get the default value for global
   * @return SFBool The default value
   */
  static SFBool getDefaultGlobal() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DUrlObject";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Networking"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of global. AccessType: inputOutput
   * @details The global field controls potential external scoping effects of
   * lights found within an Inline scene.
   * @return SFBool The current value of global.
   */
  SFBool getGlobal() const { return _global; }

  /**
   * @brief Sets the value of global. AccessType: inputOutput
   * @details The global field controls potential external scoping effects of
   * lights found within an Inline scene.
   * @param value The new value for global.
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Inline").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  /**
   * @brief Member variable for global.
   */

  SFBool _global{false};
};

#endif // INLINE_HPP