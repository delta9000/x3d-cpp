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
 * @class X3DUrlOutputObject
 * @brief X3DUrlOutputObject indicates that a node publish data on the local
 * file system or to a network address.
 * @details
 * https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-1v4.1-CD/Part01/components/networking.html#X3DUrlOutputObject
 */
class X3DUrlOutputObject {
public:
  X3DUrlOutputObject() = default;

  virtual ~X3DUrlOutputObject() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Networking"; }

  static int componentLevel() { return 4; }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFString &getUrl() const { return _url; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setUrl(const MFString &value) { _url = value; }

  void setUrl(MFString &&value) { _url = std::move(value); }

private:
  MFString _url{};
};

} // namespace x3d::nodes
