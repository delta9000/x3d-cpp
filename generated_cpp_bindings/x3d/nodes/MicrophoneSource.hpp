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

#include "x3d/nodes/X3DTimeDependentNode.hpp"

#include "x3d/nodes/X3DSoundNode.hpp"

#include "x3d/nodes/X3DSoundSourceNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class MicrophoneSource
 * @brief MicrophoneSource captures input from a physical microphone in the real
 * world.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/sound.html#MicrophoneSource
 */
class MicrophoneSource : public virtual X3DSoundSourceNode {
public:
  MicrophoneSource() = default;

  ~MicrophoneSource() = default;

  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Sound"; }

  static int componentLevel() { return 2; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details mediaDeviceID field provides a unique identifier for the active
   * device that corresponds to deviceId functionality defined in W3C Web Audio
   * API.
   */
  const SFString &getMediaDeviceID() const { return _mediaDeviceID; }

  /**
   * Access: inputOutput.
   * @details mediaDeviceID field provides a unique identifier for the active
   * device that corresponds to deviceId functionality defined in W3C Web Audio
   * API.
   */
  void setMediaDeviceID(const SFString &value) { _mediaDeviceID = value; }

  void setMediaDeviceID(SFString &&value) { _mediaDeviceID = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Earliest X3D version that defines this node type (ADR-0077).
   */
  X3DVersion introducedIn() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  SFString _mediaDeviceID{};
};

} // namespace x3d::nodes
