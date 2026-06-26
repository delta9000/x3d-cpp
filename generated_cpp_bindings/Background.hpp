// Background.hpp
#ifndef BACKGROUND_HPP
#define BACKGROUND_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBindableNode.hpp"

#include "X3DBackgroundNode.hpp"

/**
 * @class Background
 * @brief Background simulates ground and sky, using vertical arrays of
 * wraparound color values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#Background
 */
class Background : public virtual X3DBackgroundNode {
public:
  /**
   * @brief Default constructor for Background
   */
  Background() = default;

  /**
   * @brief Destructor for Background
   */
  ~Background() = default;

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
  static std::string componentName() { return "EnvironmentalEffects"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of backUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @return MFString The current value of backUrl.
   */
  MFString getBackUrl() const { return _backUrl; }

  /**
   * @brief Sets the value of backUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @param value The new value for backUrl.
   */
  void setBackUrl(const MFString &value) { _backUrl = value; }

  void setBackUrl(MFString &&value) { _backUrl = std::move(value); }

  /**
   * @brief Gets the value of bottomUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @return MFString The current value of bottomUrl.
   */
  MFString getBottomUrl() const { return _bottomUrl; }

  /**
   * @brief Sets the value of bottomUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @param value The new value for bottomUrl.
   */
  void setBottomUrl(const MFString &value) { _bottomUrl = value; }

  void setBottomUrl(MFString &&value) { _bottomUrl = std::move(value); }

  /**
   * @brief Gets the value of frontUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @return MFString The current value of frontUrl.
   */
  MFString getFrontUrl() const { return _frontUrl; }

  /**
   * @brief Sets the value of frontUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @param value The new value for frontUrl.
   */
  void setFrontUrl(const MFString &value) { _frontUrl = value; }

  void setFrontUrl(MFString &&value) { _frontUrl = std::move(value); }

  /**
   * @brief Gets the value of leftUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @return MFString The current value of leftUrl.
   */
  MFString getLeftUrl() const { return _leftUrl; }

  /**
   * @brief Sets the value of leftUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @param value The new value for leftUrl.
   */
  void setLeftUrl(const MFString &value) { _leftUrl = value; }

  void setLeftUrl(MFString &&value) { _leftUrl = std::move(value); }

  /**
   * @brief Gets the value of rightUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @return MFString The current value of rightUrl.
   */
  MFString getRightUrl() const { return _rightUrl; }

  /**
   * @brief Sets the value of rightUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @param value The new value for rightUrl.
   */
  void setRightUrl(const MFString &value) { _rightUrl = value; }

  void setRightUrl(MFString &&value) { _rightUrl = std::move(value); }

  /**
   * @brief Gets the value of topUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @return MFString The current value of topUrl.
   */
  MFString getTopUrl() const { return _topUrl; }

  /**
   * @brief Sets the value of topUrl. AccessType: inputOutput
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   * @param value The new value for topUrl.
   */
  void setTopUrl(const MFString &value) { _topUrl = value; }

  void setTopUrl(MFString &&value) { _topUrl = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "Background").
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
   * @brief Member variable for backUrl.
   */

  MFString _backUrl{};

  /**
   * @brief Member variable for bottomUrl.
   */

  MFString _bottomUrl{};

  /**
   * @brief Member variable for frontUrl.
   */

  MFString _frontUrl{};

  /**
   * @brief Member variable for leftUrl.
   */

  MFString _leftUrl{};

  /**
   * @brief Member variable for rightUrl.
   */

  MFString _rightUrl{};

  /**
   * @brief Member variable for topUrl.
   */

  MFString _topUrl{};
};

#endif // BACKGROUND_HPP