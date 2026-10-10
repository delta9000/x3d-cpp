#ifndef X3D_SAI_BROWSER_SERVICE_STATE_HPP
#define X3D_SAI_BROWSER_SERVICE_STATE_HPP

#include "x3d/sai/experimental/browser_services.hpp"

#include <memory>
#include <mutex>

namespace x3d::sai::experimental::detail {

// Browser-owned immediate service state, deliberately outside scene revisions,
// authoring transactions, external-session gates and field-event queues.
class browser_service_state {
public:
  explicit browser_service_state(browser_configuration configuration);
  result<std::optional<std::string>> get_name() const;
  result<std::optional<std::string>> get_version() const;
  result<float> get_current_speed(generation_id current_generation) const;
  result<float> get_current_frame_rate(generation_id current_generation) const;
  result<void> publish_metrics(generation_id current_generation,
                               browser_metrics metrics);
  result<std::string> description() const;
  result<void> set_description(std::string description);
  result<browser_property_map> get_rendering_properties() const;
  result<void> publish_rendering_properties(browser_property_map properties);
  result<browser_property_map> get_browser_properties() const;
  result<void> set_browser_option_handler(browser_option_handler handler);
  result<bool> set_browser_option(std::string name,
                                  browser_property_value value);

private:
  const browser_configuration configuration_;
  mutable std::mutex mutex_;
  generation_id metrics_generation_ = 0;
  browser_metrics metrics_;
  std::string description_;
  browser_property_map rendering_properties_;
  std::shared_ptr<const browser_option_handler> option_handler_;
  bool setting_option_ = false;
};

} // namespace x3d::sai::experimental::detail

#endif
