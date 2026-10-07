#ifndef X3D_CPP_SAI_HOSTED_HPP
#define X3D_CPP_SAI_HOSTED_HPP
#include <x3d/sai/experimental/hosted.hpp>

namespace x3d::runtime {
// Optional, bounded hosted SAI adapter. This is NOT a complete X3D browser or
// profile: no arbitrary node factory, PROTO, Script, Geo, Inline or resource
// I/O. Every fixture owns an independent document and RuntimeSession. No native
// pointer escapes through public handles or the copy-only diagnostics seam.
class SaiHostedService : public x3d::sai::experimental::hosted::service {
public:
  SaiHostedService();
  ~SaiHostedService() override;

protected:
  // Subclass/testing seam: inspect an already-known backend scene after calling
  // do_turn. It cannot mutate the native graph, retain native storage or call
  // application code from inside the runtime. All values are owning copies.
  struct native_evidence {
    std::uint64_t tick = 0;
    double host_time = 0;
    std::size_t routes = 0, snapshot_items = 0;
    bool translation_authored = false, key_authored = false,
         key_value_authored = false;
    x3d::sai::experimental::vec3f interpolated, translation, world_translation;
    std::size_t added = 0, removed = 0, transformed = 0, geometry = 0,
                material = 0;
    std::vector<x3d::sai::experimental::vec3f> rendered_translations;
  };
  native_evidence inspect_builtin(std::uint64_t scene) const;
  x3d::sai::experimental::result<std::uint64_t> do_create_fixture() override;
  x3d::sai::experimental::result<std::vector<std::uint64_t>>
      do_roots(std::uint64_t) const override;
  x3d::sai::experimental::result<std::uint64_t>
      do_named(std::uint64_t, std::string_view) const override;
  x3d::sai::experimental::result<std::string>
  do_type_name(const address &) const override;
  x3d::sai::experimental::result<
      std::vector<x3d::sai::experimental::hosted::field_info>>
  do_fields(const address &) const override;
  x3d::sai::experimental::result<x3d::sai::experimental::hosted::payload>
  do_read(const address &) const override;
  x3d::sai::experimental::result<void>
  do_author(const address &,
            const x3d::sai::experimental::hosted::payload &) override;
  x3d::sai::experimental::result<void> do_add_route(const address &,
                                                    const address &) override;
  x3d::sai::experimental::result<void>
  do_validate_configuration(std::uint64_t,
                            const std::vector<admitted_write> &) const override;
  x3d::sai::experimental::result<void> do_activate(std::uint64_t) override;
  backend_result do_turn(std::uint64_t, x3d::sai::experimental::event_time,
                         const std::vector<admitted_write> &) override;
  void do_retire(std::uint64_t) noexcept override;

private:
  struct storage;
  std::unique_ptr<storage> state_;
};
} // namespace x3d::runtime
#endif
