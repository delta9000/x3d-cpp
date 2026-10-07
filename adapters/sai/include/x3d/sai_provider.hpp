#ifndef X3D_CPP_SAI_PROVIDER_HPP
#define X3D_CPP_SAI_PROVIDER_HPP

#include <memory>
#include <x3d/sai/experimental/provider.hpp>

namespace x3d::runtime {
class Scene;

// EXPERIMENTAL offline provider pilot, not the legacy Script SaiContext and not
// an ISO conformance claim. This provider owns real x3d-cpp scene/node state.
class SaiOfflineProvider final
    : public x3d::sai::experimental::offline_provider {
public:
  SaiOfflineProvider();
  ~SaiOfflineProvider() override;

  // Native extension, deliberately absent from the common provider interface.
  // Inspect the authoritative scene, or mutate its existing Transform fields
  // serially on the creating thread. No live context/extractor is attached.
  // Retaining this storage after close does not preserve SAI handle authority.
  x3d::sai::experimental::result<std::shared_ptr<Scene>> native_scene() const;

private:
  struct state;
  std::unique_ptr<state> state_;
  x3d::sai::experimental::result<std::uint64_t>
      do_create_node(std::string_view) override;
  x3d::sai::experimental::result<void> do_append_root(std::uint64_t) override;
  x3d::sai::experimental::result<std::vector<std::uint64_t>>
  do_roots() const override;
  x3d::sai::experimental::result<void>
      do_define_name(std::uint64_t, std::string_view) override;
  x3d::sai::experimental::result<std::uint64_t>
      do_lookup_name(std::string_view) const override;
  x3d::sai::experimental::result<std::string>
      do_node_type(std::uint64_t) const override;
  x3d::sai::experimental::result<
      std::vector<x3d::sai::experimental::provider_field>>
      do_fields(std::uint64_t) const override;
  x3d::sai::experimental::result<x3d::sai::experimental::value>
      do_read_field(std::uint64_t, std::string_view) const override;
  x3d::sai::experimental::result<void>
      do_write_field(std::uint64_t, std::string_view,
                     x3d::sai::experimental::vec3f) override;
  void do_close() noexcept override;
};
} // namespace x3d::runtime
#endif
