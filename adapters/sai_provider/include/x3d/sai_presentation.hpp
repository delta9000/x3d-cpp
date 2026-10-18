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
  x3d::runtime::extract::MaterialDesc material;
  // Opaque invalidation revision, not a material content hash. A rebuild may
  // advance it even when the extracted material has the same value.
  std::uint64_t material_revision = 0;
};
struct presentation_frame {
  provider::scene source;
  std::uint64_t native_tick = 0;
  std::optional<event_time> host_time;
  std::vector<presentation_item> items;
};
struct presentation_delta {
  std::vector<std::uint64_t> added, removed;
  std::vector<std::uint64_t> updated_transform, updated_geometry, updated_material;
};
// Compare provider-produced frames, including skipped ticks. Null previous
// means initial upload (all current placements added). Keys in update buckets
// refer to current.items; added/removed placements have no update entries.
// Material invalidations are conservative; transform comparisons are exact.
// Reject foreign scenes and decreasing native ticks. No live service required.
result<presentation_delta> changes_between(const presentation_frame *previous,
                                          const presentation_frame &current);
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
