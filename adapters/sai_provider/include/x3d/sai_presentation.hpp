#ifndef X3D_CPP_SAI_PRESENTATION_HPP
#define X3D_CPP_SAI_PRESENTATION_HPP
#include <x3d/sai/experimental/provider.hpp>
#include "Mat4.hpp"
#include "RenderItem.hpp"

namespace x3d::sai::experimental::native {
namespace detail {
struct presentation_state;
struct presentation_access;
}
// Native presentation values, not portable node authority. Numeric keys are
// meaningful only within the frame's opaque scene identity.
struct presentation_item {
  std::uint64_t placement_key = 0, mesh_key = 0;
  x3d::runtime::Mat4 world_transform;
  x3d::runtime::extract::MeshRef mesh;
};
struct presentation_frame {
  provider::scene source;
  std::uint64_t native_tick = 0;
  std::optional<event_time> host_time;
  std::vector<presentation_item> items;
};
class render_feed {
public:
  render_feed() = default;
  // Explicit owner-thread pull of the latest complete frame. A slow host skips
  // intermediate presentation frames; portable notifications are unchanged.
  // Retained immutable frames remain usable after the service is destroyed.
  result<std::shared_ptr<const presentation_frame>>
  snapshot(const provider::service &) const;
private:
  std::weak_ptr<const detail::presentation_state> state_;
  friend struct detail::presentation_access;
};
struct presented_scene {
  std::unique_ptr<provider::service> service;
  provider::scene scene;
  render_feed feed;
};
// Creates one ordinary setup scene with a bound native feed. All subsequent
// authoring, activation, gates, turns, callbacks and close use service.
result<presented_scene> make_presented_scene(provider::resource_limits = {});
} // namespace x3d::sai::experimental::native
#endif
