// MicrophoneSource.hpp
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
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/sound.html#MicrophoneSource
 */
class MicrophoneSource : public virtual X3DSoundSourceNode {
public:
  /**
   * @brief Default constructor for MicrophoneSource
   */
  MicrophoneSource() = default;

  /**
   * @brief Destructor for MicrophoneSource
   */
  ~MicrophoneSource() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesSoundSource";
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
  static std::string componentName() { return "Sound"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of mediaDeviceID. AccessType: inputOutput
   * @details mediaDeviceID field provides ID parameter functionality.
   * @return SFString The current value of mediaDeviceID.
   */
  SFString getMediaDeviceID() const { return _mediaDeviceID; }

  /**
   * @brief Sets the value of mediaDeviceID. AccessType: inputOutput
   * @details mediaDeviceID field provides ID parameter functionality.
   * @param value The new value for mediaDeviceID.
   */
  void setMediaDeviceID(const SFString &value) { _mediaDeviceID = value; }

  void setMediaDeviceID(SFString &&value) { _mediaDeviceID = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "MicrophoneSource").
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

private:
  /**
   * @brief Member variable for mediaDeviceID.
   */

  SFString _mediaDeviceID{};
};

} // namespace x3d::nodes
