// DirectionalLight.hpp
#ifndef DIRECTIONALLIGHT_HPP
#define DIRECTIONALLIGHT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DLightNode.hpp"

/**
 * @class DirectionalLight
 * @brief DirectionalLight might not be scoped by parent Group or Transform at
 * levels 1 or 2.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html#DirectionalLight
 */
class DirectionalLight : public virtual X3DLightNode {
public:
  /**
   * @brief Default constructor for DirectionalLight
   */
  DirectionalLight() = default;

  /**
   * @brief Destructor for DirectionalLight
   */
  ~DirectionalLight() = default;

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, -1}; }

  /**
   * @brief Get the default value for global
   * @return SFBool The default value
   */
  static SFBool getDefaultGlobal() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Lighting"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details Orientation vector of light relative to local coordinate system.
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details Orientation vector of light relative to local coordinate system.
   * @param value The new value for direction.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * @brief Gets the value of global. AccessType: inputOutput
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   * @return SFBool The current value of global.
   */
  SFBool getGlobal() const { return _global; }

  /**
   * @brief Sets the value of global. AccessType: inputOutput
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   * @param value The new value for global.
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * @brief The X3D type name of this node (e.g. "DirectionalLight").
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
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 0, -1}};

  /**
   * @brief Member variable for global.
   */

  SFBool _global{false};
};

#endif // DIRECTIONALLIGHT_HPP