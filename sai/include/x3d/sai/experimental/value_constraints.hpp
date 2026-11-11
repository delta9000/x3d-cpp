#ifndef X3D_SAI_EXPERIMENTAL_VALUE_CONSTRAINTS_HPP
#define X3D_SAI_EXPERIMENTAL_VALUE_CONSTRAINTS_HPP

#include "types.hpp"

#include <cmath>
#include <limits>

namespace x3d::sai::experimental::detail {

// ISO/IEC 19775-1, 5.3.13 requires a normalized rotation axis. The numerical
// tolerance and rejection (rather than normalization) are reference-policy
// choices: eight float epsilons in squared length, evaluated in double to avoid
// overflow when checking arbitrary float inputs. Accepted values are unchanged.
inline constexpr double rotation_axis_squared_norm_tolerance =
    8.0 * std::numeric_limits<float>::epsilon();

inline bool valid_rotation_axis(const rotation &candidate) noexcept {
  if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) ||
      !std::isfinite(candidate.z))
    return false;
  // The C++ binding's normative Annex A.4.21 also bounds each axis component.
  if (std::abs(candidate.x) > 1.f || std::abs(candidate.y) > 1.f ||
      std::abs(candidate.z) > 1.f)
    return false;
  const double x = candidate.x;
  const double y = candidate.y;
  const double z = candidate.z;
  const double squared_norm = x * x + y * y + z * z;
  // The angle is intentionally not constrained here. Its value and any
  // field-specific bounds are independent of the axis normalization rule.
  return std::abs(squared_norm - 1.0) <= rotation_axis_squared_norm_tolerance;
}

} // namespace x3d::sai::experimental::detail

#endif
