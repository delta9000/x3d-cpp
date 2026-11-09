#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DBindableNode.hpp"

#include "x3d/nodes/X3DBackgroundNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Background
 * @brief Background simulates ground and sky, using vertical arrays of
 * wraparound color values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#Background
 */
class Background : public virtual X3DBackgroundNode {
public:
  Background() = default;

  ~Background() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalEffects"; }

  static int componentLevel() { return 1; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  const MFString &getBackUrl() const { return _backUrl; }

  /**
   * Access: inputOutput.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  void setBackUrl(const MFString &value) { _backUrl = value; }

  void setBackUrl(MFString &&value) { _backUrl = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  const MFString &getBottomUrl() const { return _bottomUrl; }

  /**
   * Access: inputOutput.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  void setBottomUrl(const MFString &value) { _bottomUrl = value; }

  void setBottomUrl(MFString &&value) { _bottomUrl = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  const MFString &getFrontUrl() const { return _frontUrl; }

  /**
   * Access: inputOutput.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  void setFrontUrl(const MFString &value) { _frontUrl = value; }

  void setFrontUrl(MFString &&value) { _frontUrl = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  const MFString &getLeftUrl() const { return _leftUrl; }

  /**
   * Access: inputOutput.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  void setLeftUrl(const MFString &value) { _leftUrl = value; }

  void setLeftUrl(MFString &&value) { _leftUrl = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  const MFString &getRightUrl() const { return _rightUrl; }

  /**
   * Access: inputOutput.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  void setRightUrl(const MFString &value) { _rightUrl = value; }

  void setRightUrl(MFString &&value) { _rightUrl = std::move(value); }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  const MFString &getTopUrl() const { return _topUrl; }

  /**
   * Access: inputOutput.
   * @details Image background panorama between ground/sky backdrop and scene's
   * geometry.
   */
  void setTopUrl(const MFString &value) { _topUrl = value; }

  void setTopUrl(MFString &&value) { _topUrl = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  MFString _backUrl{};

  MFString _bottomUrl{};

  MFString _frontUrl{};

  MFString _leftUrl{};

  MFString _rightUrl{};

  MFString _topUrl{};
};

} // namespace x3d::nodes
