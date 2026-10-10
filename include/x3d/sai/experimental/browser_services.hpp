#ifndef X3D_SAI_EXPERIMENTAL_BROWSER_SERVICES_HPP
#define X3D_SAI_EXPERIMENTAL_BROWSER_SERVICES_HPP

#include "types.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <variant>

namespace x3d::sai::experimental {

// Owning modern replacements for the published binding's inconsistent
// string/primitive property-array spellings. Mutating a returned map never
// changes the browser. Strings and property names preserve exact case.
using browser_property_value =
    std::variant<bool, std::int32_t, float, std::string>;
using browser_property_map =
    std::map<std::string, browser_property_value, std::less<>>;

struct browser_configuration {
  // Absence is the unsupported identity result required by the source; an
  // explicitly supplied empty string remains distinct from absence.
  std::optional<std::string> name;
  std::optional<std::string> version;
};

struct browser_metrics {
  // Host measurements for the active layer/current world. Speed is in the
  // viewpoint's base speed units; frame rate is frames per second. This
  // kernel does not infer either quantity from NavigationInfo or event time.
  float current_speed = 0.f;
  float current_frame_rate = 0.f;
};

// Return true only once the host has actually applied the requested option;
// false means unavailable/refused. The arguments live for the invocation.
// The host owns rendering/UI effects and must leave them unchanged on failure.
using browser_option_handler = std::function<result<bool>(
    const std::string &, const browser_property_value &)>;

} // namespace x3d::sai::experimental

#endif
