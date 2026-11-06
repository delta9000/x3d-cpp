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
 * @class X3DMetadataObject
 * @brief Each node inheriting the X3DMetadataObject interface contains a single
 * array of strictly typed values: MFBool, MFInt32, MFFloat, MFDouble, MFString,
 * or MFNode, the latter having children that are all Metadata nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DMetadataObject
 */
class X3DMetadataObject {
public:
  X3DMetadataObject() = default;

  virtual ~X3DMetadataObject() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Core"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getName() const { return _name; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setName(const SFString &value) { _name = value; }

  void setName(SFString &&value) { _name = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getReference() const { return _reference; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setReference(const SFString &value) { _reference = value; }

  void setReference(SFString &&value) { _reference = std::move(value); }

private:
  SFString _name{};

  SFString _reference{};
};

} // namespace x3d::nodes
