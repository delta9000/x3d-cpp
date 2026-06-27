// X3DBoundedObject.hpp
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
 * @class X3DBoundedObject
 * @brief X3DBoundedObject indicates that bounding box values can be provided
 * (or computed) to encompass this node and any children.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html#X3DBoundedObject
 */
class X3DBoundedObject {
public:
  /**
   * @brief Default constructor for X3DBoundedObject
   */
  X3DBoundedObject() = default;

  /**
   * @brief Virtual destructor for X3DBoundedObject
   */
  virtual ~X3DBoundedObject() = default;

  /**
   * @brief Get the default value for bboxCenter
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for bboxDisplay
   * @return SFBool The default value
   */
  static SFBool getDefaultBboxDisplay() { return false; }

  /**
   * @brief Get the default value for bboxSize
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  /**
   * @brief Get the default value for visible
   * @return SFBool The default value
   */
  static SFBool getDefaultVisible() { return true; }

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
  static std::string componentName() { return "Grouping"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of bboxCenter. AccessType: initializeOnly
   * @details
   * @return SFVec3f The current value of bboxCenter.
   */
  SFVec3f getBboxCenter() const { return _bboxCenter; }
  /**
   * @brief Data-layer write of bboxCenter (reader/init ingest path).
   * @details bboxCenter is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBboxCenter().
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * @brief Gets the value of bboxDisplay. AccessType: inputOutput
   * @details
   * @return SFBool The current value of bboxDisplay.
   */
  SFBool getBboxDisplay() const { return _bboxDisplay; }

  /**
   * @brief Sets the value of bboxDisplay. AccessType: inputOutput
   * @details
   * @param value The new value for bboxDisplay.
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * @brief Gets the value of bboxSize. AccessType: initializeOnly
   * @details
   * @return SFVec3f The current value of bboxSize.
   */
  SFVec3f getBboxSize() const { return _bboxSize; }
  /**
   * @brief Data-layer write of bboxSize (reader/init ingest path).
   * @details bboxSize is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBboxSize().
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * @brief Gets the value of visible. AccessType: inputOutput
   * @details
   * @return SFBool The current value of visible.
   */
  SFBool getVisible() const { return _visible; }

  /**
   * @brief Sets the value of visible. AccessType: inputOutput
   * @details
   * @param value The new value for visible.
   */
  void setVisible(const SFBool &value) { _visible = value; }

private:
  /**
   * @brief Member variable for bboxCenter.
   */

  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for bboxDisplay.
   */

  SFBool _bboxDisplay{false};

  /**
   * @brief Member variable for bboxSize.
   */

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  /**
   * @brief Member variable for visible.
   */

  SFBool _visible{true};
};

} // namespace x3d::nodes
