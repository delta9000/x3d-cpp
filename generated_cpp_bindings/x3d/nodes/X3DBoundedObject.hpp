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
  X3DBoundedObject() = default;

  virtual ~X3DBoundedObject() = default;

  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultBboxDisplay() { return false; }

  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  static SFBool getDefaultVisible() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Grouping"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxCenter() const { return _bboxCenter; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getBboxDisplay() const { return _bboxDisplay; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxSize() const { return _bboxSize; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getVisible() const { return _visible; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVisible(const SFBool &value) { _visible = value; }

private:
  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  SFBool _bboxDisplay{false};

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  SFBool _visible{true};
};

} // namespace x3d::nodes
