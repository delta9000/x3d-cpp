#ifndef NATIVE_AUTHORING_FIXTURE_HPP
#define NATIVE_AUTHORING_FIXTURE_HPP
#include <x3d/sai/experimental/bindings/Transform.hpp>
#include <x3d/sai_provider.hpp>
namespace sai = x3d::sai::experimental;
namespace p = sai::provider;
// Test-local convenience only. Every operation below uses the single final
// service and its owner-bearing handles; this is not an installed/public API.
struct native_authoring_fixture {
  std::unique_ptr<sai::native::backend> allocated =
      std::make_unique<sai::native::backend>();
  sai::native::backend *backend = allocated.get();
  p::service service{std::move(allocated)};
  p::scene scene = *service.create_scene();
  p::context context = *service.root_context(scene);
  std::thread::id thread = std::this_thread::get_id();
  bool closed = false;
  sai::result<std::shared_ptr<x3d::runtime::Scene>> native_scene() const {
    if (closed) {
      sai::sai_error e;
      e.code = sai::error_code::stale_handle;
      return sai::failure(std::move(e));
    }
    return backend->native_scene(1);
  }
  auto create_node(std::string_view type) {
    return service.create_node(context, type);
  }
  auto append_root(const p::node &n) { return service.append_root(context, n); }
  auto define_name(const p::node &n, std::string_view name) {
    return service.define_name(n, name);
  }
  auto lookup_name(std::string_view name) {
    return service.named(context, name);
  }
  template <class Key> auto field(const p::node &n, const Key &key) const {
    return service.get_field(n, key);
  }
  template <class Key>
  sai::result<p::payload> read_field(const p::node &n, const Key &key) const {
    auto f = field(n, key);
    if (!f)
      return sai::failure(f.error());
    return service.read(*f);
  }
  template <class O, class T>
  auto read_field(const p::node &n, const sai::field_key<O, T> &key) const {
    return service.read(n, key);
  }
  template <class Key>
  sai::result<void> write_field(const p::node &n, const Key &key,
                                p::payload value) {
    auto f = field(n, key);
    if (!f)
      return sai::failure(f.error());
    return service.author(*f, std::move(value));
  }
  template <class Key>
  sai::result<p::node_values> read_nodes(const p::node &n,
                                         const Key &key) const {
    auto v = read_field(n, key);
    if (!v)
      return sai::failure(v.error());
    return std::get<p::node_values>(*v);
  }
  template <class O>
  sai::result<p::node_values>
  read_nodes(const p::node &n,
             const sai::field_key<O, sai::node_list> &key) const {
    return service.read(n, key);
  }
  template <class Key>
  auto set_nodes(const p::node &n, const Key &key, p::node_values value) {
    return write_field(n, key, p::payload{std::move(value)});
  }
  auto user_data(const p::node &n) const { return service.user_data(n); }
  auto user_data(const p::field &f) const { return service.user_data(f); }
  template <class Key>
  sai::result<sai::user_data_value> user_data(const p::node &n,
                                              const Key &key) const {
    auto f = field(n, key);
    if (!f)
      return sai::failure(f.error());
    return service.user_data(*f);
  }
  auto set_user_data(const p::node &n, sai::user_data_value value) {
    return service.set_user_data(n, std::move(value));
  }
  template <class Key>
  sai::result<void> set_user_data(const p::node &n, const Key &key,
                                  sai::user_data_value value) {
    auto f = field(n, key);
    if (!f)
      return sai::failure(f.error());
    return service.set_user_data(*f, std::move(value));
  }
  sai::result<void> close() {
    auto r = service.close();
    if (r)
      closed = true;
    return r;
  }
};
#endif
