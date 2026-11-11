#include "x3d/sai/experimental/X3DSAIBindingCatalog.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include <iostream>

#ifdef X3D_SAI_EXPERIMENTAL_KERNEL_HPP
#error "Generated schema keys and the provider fixture must not include kernel.hpp"
#endif
#ifdef X3D_SAI_EXPERIMENTAL_METADATA_HPP
#error "Generated schema keys must not include the reference metadata adapter"
#endif
#if __has_include("x3d_sai/detail/X3DSemanticMetadataRegistry.hpp") || __has_include("x3d_sai/detail/metadata_types.hpp")
#error "The provider target must not expose private generated headers"
#endif

namespace s=x3d::sai::experimental;
namespace p=s::provider;

// Deliberately malformed metadata/value storage tests the public frontend without
// either real backend, the semantic kernel, generated objects, or render IDs.
class malformed_backend final : public p::backend {
public:
  std::string type="Transform";
  p::field_info descriptor{"translation",s::value_kind::sf_vec3f,s::access_type::input_output,true};
  std::vector<p::field_info> extra;
  backend_value payload=true;
  mutable int reads=0;
  int writes=0;
  p::capabilities supported() const override {
    return {"malformed-schema-probe",1,{{p::service_id::scene_authoring,1},
            {p::service_id::host_turns,1},{p::service_id::node_values,1},
            {p::service_id::user_data,1}},{{"Transform","Grouping",1}},
            {s::value_kind::sf_bool,s::value_kind::sf_vec3f,s::value_kind::sf_rotation,
             s::value_kind::sf_node,s::value_kind::mf_node}};
  }
private:
  s::result<std::uint64_t> do_create_scene() override { return 1; }
  s::result<std::uint64_t> do_create_node(std::uint64_t,std::string_view) override { return 1; }
  s::result<void> do_append_root(const address &) override { return {}; }
  s::result<void> do_define_name(const address &,std::string_view) override { return {}; }
  s::result<std::vector<std::uint64_t>> do_roots(std::uint64_t) const override { return std::vector<std::uint64_t>{1}; }
  s::result<std::uint64_t> do_named(std::uint64_t,std::string_view) const override { return 1; }
  s::result<std::string> do_type_name(const address &a) const override {
    if(a.node!=1) return error(s::error_code::unknown_node,"schema.type","dangling backend node ID");
    return type;
  }
  s::result<std::vector<p::field_info>> do_fields(const address &) const override {
    auto fields=extra; fields.insert(fields.begin(),descriptor); return fields;
  }
  s::result<backend_value> do_read(const address &) const override { ++reads; return payload; }
  s::result<void> do_author(const address &,const backend_value &) override { ++writes; return {}; }
  s::result<void> do_add_route(const address &,const address &) override { return {}; }
  s::result<void> do_validate_configuration(std::uint64_t,const std::vector<admitted_write> &) const override { return {}; }
  s::result<void> do_activate(std::uint64_t) override { return {}; }
  backend_result do_turn(std::uint64_t,s::event_time,const std::vector<admitted_write> &) override { return {}; }
  void do_retire(std::uint64_t) noexcept override {}
};

int main() {
  static_assert(!s::bindings::node_keys.empty());
  static_assert(!std::is_constructible_v<p::field,std::uint64_t,std::string>);
  static_assert(!std::is_constructible_v<p::node,std::uint64_t>);
  static_assert(std::is_same_v<decltype(std::declval<const p::service &>().get_field(
      std::declval<const p::node &>(),s::bindings::Transform::metadata)),s::result<p::field>>);
  static_assert(std::is_same_v<decltype(std::declval<const p::service &>().user_data(
      std::declval<const p::field &>())),s::result<s::user_data_value>>);
  static_assert(std::is_same_v<decltype(std::declval<s::user_data_value>().as<std::unique_ptr<int>>()),
      s::result<std::shared_ptr<const std::unique_ptr<int>>>>);
  static_assert(std::is_same_v<decltype(std::declval<const p::service &>().read(
      std::declval<const p::node &>(),s::bindings::Transform::translation)),s::result<s::vec3f>>);
  static_assert(std::is_same_v<decltype(std::declval<const p::service &>().read(
      std::declval<const p::node &>(),s::bindings::Transform::children)),s::result<p::node_values>>);
  static_assert(std::is_same_v<decltype(std::declval<const p::service &>().read(
      std::declval<const p::node &>(),s::bindings::Transform::metadata)),s::result<p::node_value>>);
  static_assert(!std::is_constructible_v<p::payload,s::node_id>);
  static_assert(!std::is_constructible_v<p::payload,s::node_list>);
  std::size_t checks=0;
  const auto require=[&](bool ok,const char *message) { ++checks; if(!ok) throw std::runtime_error(message); };
  const auto reject=[&](const auto &result,s::error_code code) {
    require(!result && result.error().code==code,"provider must reject inconsistent schema/value before dispatch");
  };
  auto owned=std::make_unique<malformed_backend>(); auto &backend=*owned;
  p::service provider(std::move(owned));
  const auto scene=provider.create_scene().value();
  const auto context=provider.root_context(scene).value();
  const auto node=provider.create_node(context,"Transform").value();
  const auto &key=s::bindings::Transform::translation;
  const auto field=[&](std::string_view name) { return provider.get_field(node,name).value(); };
  reject(provider.read(field("translation")),s::error_code::type_mismatch);
  reject(provider.read(node,key),s::error_code::type_mismatch);
  backend.reads=0;
  backend.descriptor.kind=s::value_kind::sf_bool;
  reject(provider.read(node,key),s::error_code::type_mismatch);
  reject(provider.author(node,key,s::vec3f{}),s::error_code::type_mismatch);
  backend.descriptor.kind=s::value_kind::sf_vec3f;
  backend.descriptor.access=s::access_type::output_only;
  reject(provider.read(node,key),s::error_code::invalid_descriptor);
  reject(provider.author(node,key,s::vec3f{}),s::error_code::invalid_descriptor);
  backend.descriptor.access=s::access_type::input_output;
  backend.type="Group";
  reject(provider.read(node,key),s::error_code::type_mismatch);
  reject(provider.author(node,key,s::vec3f{}),s::error_code::type_mismatch);
  require(backend.reads==0 && backend.writes==0,"bad generated keys never reach value dispatch");
  backend.type="Transform";

  // Node IDs are a backend storage convention. Export must resolve them before
  // granting an owner-bearing public node. Zero is the only backend NULL value.
  backend.descriptor={"metadata",s::value_kind::sf_node,s::access_type::input_output,true};
  backend.payload=s::node_id{123};
  reject(provider.read(field("metadata")),s::error_code::unknown_node);
  reject(provider.read(node,s::bindings::Transform::metadata),s::error_code::unknown_node);
  backend.payload=s::node_id{};
  require(!provider.read(node,s::bindings::Transform::metadata).value(),"backend zero exports explicit SFNode NULL");
  backend.payload=s::node_id{1};
  require(provider.read(node,s::bindings::Transform::metadata).value()==p::node_value{node},"backend node export retains public identity");
  backend.descriptor={"children",s::value_kind::mf_node,s::access_type::input_output,true};
  backend.payload=s::node_list{s::node_id{1},s::node_id{},s::node_id{1}};
  require(provider.read(node,s::bindings::Transform::children).value()==p::node_values{node,std::nullopt,node},
          "generic MFNode projection preserves storage order, duplicates and explicit NULL");
  // Whether a particular child collection permits repeated identity is its
  // node-semantic backend rule and is tested in the shared real-backend fixture.
  backend.payload=s::node_list{s::node_id{123}};
  reject(provider.read(node,s::bindings::Transform::children),s::error_code::unknown_node);
  backend.reads=0;
  reject(provider.author(node,s::bindings::Transform::children,p::node_values{node,p::node{}}),s::error_code::stale_handle);
  backend.descriptor.supported=false;
  reject(provider.read(field("children")),s::error_code::unsupported_field_type);
  reject(provider.author(field("children"),p::node_values{}),s::error_code::unsupported_field_type);
  backend.descriptor={"addChildren",s::value_kind::mf_node,s::access_type::input_only,true};
  reject(provider.read(field("addChildren")),s::error_code::access_denied);
  reject(provider.author(field("addChildren"),p::node_values{}),s::error_code::access_denied);
  require(backend.reads==0 && backend.writes==0,"unsupported/access/authority failures do not dispatch values");

  backend.descriptor={"inventedChildren",s::value_kind::mf_node,s::access_type::input_output,true};
  backend.payload=s::node_list{s::node_id{},s::node_id{1}};
  require(std::get<p::node_values>(provider.read(field("inventedChildren")).value())==p::node_values{std::nullopt,node},
          "generic supported MFNode descriptors project owning values without hard-coded field names");
  require(bool(provider.author(field("inventedChildren"),p::node_values{node,std::nullopt})),
          "generic supported node fields use the same canonical authoring dispatch");
  backend.writes=0;

  backend.descriptor={"bboxCenter",s::value_kind::sf_vec3f,s::access_type::initialize_only,true};
  require(bool(provider.author(node,s::bindings::Transform::bboxCenter,s::vec3f{})),"initializeOnly authoring is supported during setup");
  require(backend.writes==1,"successful initializeOnly authoring dispatches once");
  backend.writes=0;
  backend.descriptor={"rotation",s::value_kind::sf_rotation,s::access_type::input_output,true};
  backend.payload=s::rotation{0,0,0,0};
  reject(provider.read(field("rotation")),s::error_code::invalid_value);
  reject(provider.read(node,s::bindings::Transform::rotation),s::error_code::invalid_value);
  reject(provider.author(field("rotation"),s::rotation{2,0,0,1}),s::error_code::invalid_value);
  require(backend.writes==0,"invalid rotation cannot mutate backend");

  backend.descriptor={"translation",s::value_kind::sf_vec3f,s::access_type::input_output,true};
  backend.extra={{"set_translation",s::value_kind::sf_bool,s::access_type::input_only,false},
                 {"translation_changed",s::value_kind::sf_bool,s::access_type::output_only,false}};
  const auto canonical=field("translation"),exact_input=field("set_translation"),exact_output=field("translation_changed");
  require(canonical.name()=="translation" && exact_input.name()=="set_translation" &&
          exact_output.name()=="translation_changed" && canonical!=exact_input && canonical!=exact_output,
          "exact semantic names take precedence over IO aliases");
  require(bool(provider.set_user_data(canonical,s::user_data_value::make<int>(1))) &&
          bool(provider.set_user_data(exact_input,s::user_data_value::make<int>(2))) &&
          bool(provider.set_user_data(exact_output,s::user_data_value::make<int>(3))) &&
          **provider.user_data(canonical)->as<int>()==1 &&
          **provider.user_data(exact_input)->as<int>()==2 &&
          **provider.user_data(exact_output)->as<int>()==3,
          "exact alias-shaped descriptors own independent metadata even without value support");
  backend.extra.clear();
  reject(provider.user_data(exact_input),s::error_code::unknown_field);
  reject(provider.user_data(exact_output),s::error_code::unknown_field);
  require(field("set_translation")==canonical,"fallback alias retains canonical authority");
  backend.descriptor.access=s::access_type::output_only;
  reject(provider.user_data(canonical),s::error_code::invalid_descriptor);
  reject(provider.read(canonical),s::error_code::invalid_descriptor);
  backend.descriptor.access=s::access_type::input_output;
  backend.extra={backend.descriptor};
  reject(provider.fields(node),s::error_code::invalid_descriptor);
  reject(provider.get_field(node,"translation"),s::error_code::invalid_descriptor);
  backend.extra.clear();
  backend.descriptor.name.clear();
  reject(provider.fields(node),s::error_code::invalid_descriptor);
  backend.descriptor.name="translation";
  backend.descriptor.kind=static_cast<s::value_kind>(999);
  reject(provider.fields(node),s::error_code::invalid_descriptor);
  backend.descriptor.kind=s::value_kind::sf_vec3f;
  backend.descriptor.access=static_cast<s::access_type>(999);
  reject(provider.fields(node),s::error_code::invalid_descriptor);
  backend.descriptor.access=s::access_type::input_output;
  auto move_only=s::user_data_value::make<std::unique_ptr<int>>(std::make_unique<int>(7));
  require(bool(provider.set_user_data(node,std::move(move_only))) &&
          ***provider.user_data(node)->as<std::unique_ptr<int>>()==7,
          "frontend-owned user metadata supports move-only payloads");
  std::cout << "provider schema: " << checks << " checks passed\n";
}
