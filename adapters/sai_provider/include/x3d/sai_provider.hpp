#ifndef X3D_CPP_SAI_PROVIDER_HPP
#define X3D_CPP_SAI_PROVIDER_HPP
#include <x3d/sai/experimental/provider.hpp>

namespace x3d::runtime {
class Scene;
}
namespace x3d::sai::experimental::native {
namespace detail { struct presentation_access; }
// One native storage backend for both setup authoring and host-driven runtime.
// The final portable service owns this backend and all public handle authority.
// No reference-kernel model or metadata is linked or mirrored here.
class backend : public provider::backend {
public:
  backend();
  ~backend() override;
  provider::capabilities supported() const override;

  // Explicit native-only diagnostic/extraction extension. Storage IDs here are
  // backend addresses, not portable authority. Retaining setup storage does not
  // retain a service or handle. Setup storage cannot be requested after
  // activate.
  result<std::shared_ptr<x3d::runtime::Scene>>
  native_scene(std::uint64_t scene) const;
  struct native_evidence {
    std::uint64_t tick = 0;
    double host_time = 0;
    std::size_t routes = 0, snapshot_items = 0, input_occurrences = 0;
    bool translation_authored = false, key_authored = false,
         key_value_authored = false;
    vec3f interpolated, translation, world_translation;
    std::size_t added = 0, removed = 0, transformed = 0, geometry = 0,
                material = 0;
    std::vector<vec3f> rendered_translations;
  };
  native_evidence inspect(std::uint64_t scene) const;

protected:
  result<std::uint64_t> do_create_scene() override;
  result<std::vector<unit_declaration>> do_units(std::uint64_t) const override;
  result<void> do_declare_unit(std::uint64_t,
                               const unit_declaration &) override;
  result<std::vector<metadata_entry>> do_metadata(std::uint64_t) const override;
  result<void> do_set_metadata(std::uint64_t, std::string_view,
                               const std::optional<std::string> &) override;
  result<std::uint64_t> do_create_node(std::uint64_t,
                                       std::string_view) override;
  result<void> do_append_root(const address &) override;
  result<void> do_define_name(const address &, std::string_view) override;
  result<std::vector<std::uint64_t>> do_roots(std::uint64_t) const override;
  result<std::uint64_t> do_named(std::uint64_t,
                                 std::string_view) const override;
  result<std::string> do_type_name(const address &) const override;
  result<std::vector<provider::field_info>>
  do_fields(const address &) const override;
  result<backend_value> do_read(const address &) const override;
  result<void> do_author(const address &, const backend_value &) override;
  result<void> do_add_route(const address &, const address &) override;
  result<void>
  do_validate_configuration(std::uint64_t,
                            const std::vector<admitted_write> &) const override;
  result<void> do_activate(std::uint64_t) override;
  backend_result do_turn(std::uint64_t, event_time,
                         const std::vector<admitted_write> &) override;
  void do_retire(std::uint64_t) noexcept override;

private:
  friend struct detail::presentation_access;
  struct storage;
  std::unique_ptr<storage> state_;
};
std::unique_ptr<provider::backend> make_backend();
result<std::unique_ptr<provider::service>>
    make_service(provider::resource_limits = {});
} // namespace x3d::sai::experimental::native
#endif
