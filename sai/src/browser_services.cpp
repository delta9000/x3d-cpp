#include "browser_service_state.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <string_view>
#include <utility>

namespace x3d::sai::experimental::detail {
namespace {

unexpected service_error(error_code code, const char *operation,
                         std::string message, std::string field = {}) {
  sai_error error;
  error.code = code;
  error.operation = operation;
  error.message = std::move(message);
  error.field = std::move(field);
  return failure(std::move(error));
}

template <std::size_t N>
bool one_of(std::string_view value,
            const std::array<std::string_view, N> &choices) {
  return std::find(choices.begin(), choices.end(), value) != choices.end();
}

result<void> validate_option(const std::string &name,
                             const browser_property_value &value) {
  constexpr auto operation = "browser.set_browser_option";
  constexpr std::array boolean_names{
      std::string_view{"Antialiased"}, std::string_view{"Dashboard"},
      std::string_view{"EnableInlineViewpoints"},
      std::string_view{"MotionBlur"}, std::string_view{"SplashScreen"}};
  if (one_of(name, boolean_names)) {
    if (std::holds_alternative<bool>(value))
      return {};
    return service_error(error_code::type_mismatch, operation,
                         "browser option requires a boolean", name);
  }
  if (name != "PrimitiveQuality" && name != "TextureQuality" &&
      name != "QualityWhenMoving" && name != "Shading")
    return service_error(error_code::invalid_name, operation,
                         "unknown browser option (names are case-sensitive)",
                         name);
  const auto *text = std::get_if<std::string>(&value);
  if (!text)
    return service_error(error_code::type_mismatch, operation,
                         "browser option requires a string", name);
  constexpr std::array quality{std::string_view{"Low"},
                               std::string_view{"Medium"},
                               std::string_view{"High"}};
  constexpr std::array shading{
      std::string_view{"Wireframe"}, std::string_view{"Flat"},
      std::string_view{"Gouraud"}, std::string_view{"Phong"}};
  const bool valid = name == "Shading"
                         ? one_of(*text, shading)
                         : one_of(*text, quality) ||
                               (name == "QualityWhenMoving" && *text == "Same");
  if (!valid)
    return service_error(error_code::invalid_value, operation,
                         "browser option value is outside its defined choices",
                         name);
  return {};
}

bool positive_pixel_count(std::string_view value) {
  return !value.empty() && std::all_of(value.begin(), value.end(), [](char c) {
    return c >= '0' && c <= '9';
  }) && std::any_of(value.begin(), value.end(), [](char c) {
    return c != '0';
  });
}

result<void> validate_rendering_properties(const browser_property_map &values) {
  constexpr auto operation = "browser.publish_rendering_properties";
  for (const auto &[name, value] : values) {
    if (name.empty())
      return service_error(error_code::invalid_name, operation,
                           "rendering property name must not be empty");
    bool type_valid = true;
    bool value_valid = true;
    if (name == "Shading" || name == "MaxTextureSize") {
      const auto *text = std::get_if<std::string>(&value);
      type_valid = text != nullptr;
      if (text && name == "MaxTextureSize") {
        const auto separator = text->find('x');
        value_valid =
            separator != std::string::npos &&
            positive_pixel_count(
                std::string_view{*text}.substr(0, separator)) &&
            positive_pixel_count(std::string_view{*text}.substr(separator + 1));
      }
      // Shading's source lists typical values, not an exhaustive enumeration.
    } else if (name == "TextureUnits" || name == "ColorDepth") {
      const auto *integer = std::get_if<std::int32_t>(&value);
      type_valid = integer != nullptr;
      value_valid = !integer || *integer >= 0;
    } else if (name == "AntiAliased") {
      type_valid = std::holds_alternative<bool>(value);
    } else if (name == "TextureMemory") {
      const auto *number = std::get_if<float>(&value);
      type_valid = number != nullptr;
      value_valid = !number || (std::isfinite(*number) && *number >= 0.f);
    }
    if (!type_valid)
      return service_error(error_code::type_mismatch, operation,
                           "rendering property does not have its defined type",
                           name);
    if (!value_valid)
      return service_error(error_code::invalid_value, operation,
                           "rendering property contains an invalid value",
                           name);
  }
  return {};
}

} // namespace

browser_service_state::browser_service_state(
    browser_configuration configuration)
    : configuration_(std::move(configuration)) {}

result<std::optional<std::string>> browser_service_state::get_name() const {
  return configuration_.name;
}

result<std::optional<std::string>> browser_service_state::get_version() const {
  return configuration_.version;
}

result<float> browser_service_state::get_current_speed(
    generation_id current_generation) const {
  std::lock_guard lock(mutex_);
  return metrics_generation_ == current_generation ? metrics_.current_speed
                                                   : 0.f;
}

result<float> browser_service_state::get_current_frame_rate(
    generation_id current_generation) const {
  std::lock_guard lock(mutex_);
  return metrics_generation_ == current_generation ? metrics_.current_frame_rate
                                                   : 0.f;
}

result<void>
browser_service_state::publish_metrics(generation_id current_generation,
                                       browser_metrics metrics) {
  if (!std::isfinite(metrics.current_speed) || metrics.current_speed < 0.f ||
      !std::isfinite(metrics.current_frame_rate) ||
      metrics.current_frame_rate < 0.f)
    return service_error(error_code::invalid_value, "browser.publish_metrics",
                         "browser metrics must be finite and nonnegative");
  std::lock_guard lock(mutex_);
  metrics_ = metrics;
  metrics_generation_ = current_generation;
  return {};
}

result<std::string> browser_service_state::description() const {
  std::lock_guard lock(mutex_);
  return description_;
}

result<void> browser_service_state::set_description(std::string description) {
  std::lock_guard lock(mutex_);
  description_.swap(description);
  return {};
}

result<browser_property_map>
browser_service_state::get_rendering_properties() const {
  std::lock_guard lock(mutex_);
  return rendering_properties_;
}

result<void> browser_service_state::publish_rendering_properties(
    browser_property_map properties) {
  auto valid = validate_rendering_properties(properties);
  if (!valid)
    return valid;
  std::lock_guard lock(mutex_);
  rendering_properties_.swap(properties);
  return {};
}

result<browser_property_map>
browser_service_state::get_browser_properties() const {
  // None of these complete ISO capabilities follows from this kernel's
  // generated vocabulary, runtime seam or bounded external-session pilot.
  return browser_property_map{{"ABSTRACT_NODES", false},
                              {"CONCRETE_NODES", false},
                              {"EXTERNAL_INTERACTIONS", false},
                              {"PROTOTYPE_CREATE", false},
                              {"DOM_IMPORT", false},
                              {"XML_ENCODING", false},
                              {"CLASSIC_VRML_ENCODING", false},
                              {"COMPRESSED_BINARY_ENCODING", false}};
}

result<void> browser_service_state::set_browser_option_handler(
    browser_option_handler handler) {
  std::shared_ptr<const browser_option_handler> replacement;
  if (handler)
    replacement =
        std::make_shared<const browser_option_handler>(std::move(handler));
  {
    std::lock_guard lock(mutex_);
    option_handler_.swap(replacement);
  }
  // The old callable and its application resources are destroyed unlocked.
  return {};
}

result<bool>
browser_service_state::set_browser_option(std::string name,
                                          browser_property_value value) {
  auto valid = validate_option(name, value);
  if (!valid)
    return failure(valid.error());
  std::shared_ptr<const browser_option_handler> handler;
  {
    std::lock_guard lock(mutex_);
    if (setting_option_)
      return service_error(error_code::operation_in_progress,
                           "browser.set_browser_option",
                           "a browser option handler is already executing");
    handler = option_handler_;
    if (!handler)
      return false;
    setting_option_ = true;
  }
  struct busy_guard {
    browser_service_state &owner;
    ~busy_guard() {
      std::lock_guard lock(owner.mutex_);
      owner.setting_option_ = false;
    }
  } guard{*this};
  // No browser, scene or service mutex is held during application code. The
  // guard resets before handler destruction, including every failure path.
  try {
    return (*handler)(name, value);
  } catch (const std::exception &exception) {
    return service_error(error_code::callback_failed,
                         "browser.set_browser_option", exception.what(), name);
  } catch (...) {
    return service_error(
        error_code::callback_failed, "browser.set_browser_option",
        "browser option handler threw a non-standard exception", name);
  }
}

} // namespace x3d::sai::experimental::detail
