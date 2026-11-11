#ifndef X3D_SAI_EXPERIMENTAL_TESTING_PROVIDER_FIXTURE_HPP
#define X3D_SAI_EXPERIMENTAL_TESTING_PROVIDER_FIXTURE_HPP
#include "../provider.hpp"
#include "x3d/sai/experimental/bindings/Group.hpp"
#include "x3d/sai/experimental/bindings/Transform.hpp"
#include "x3d/sai/experimental/bindings/PositionInterpolator.hpp"
#include "x3d/sai/experimental/bindings/Shape.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <thread>
#include <tuple>

namespace x3d::sai::experimental::testing {
namespace p = provider;
using provider_factory = std::function<result<std::unique_ptr<p::service>>() >;
// One backend-blind oracle. Checks stay active under NDEBUG. This bounded
// experimental contract fixture is not a claim of full ISO SAI conformance.
struct provider_fixture_report {
  std::size_t checks = 0;
  std::vector<p::field_info> fields;
  std::vector<vec3f> read_trace, typed_read_trace, observations;
  std::vector<bool> bool_trace;
  std::vector<std::string> user_data_trace;
  std::vector<rotation> rotation_trace;
  std::vector<std::vector<std::optional<std::size_t>>> node_read_trace;
  std::vector<std::pair<std::uint64_t, std::string>> notifications;
  std::vector<p::request_state> requests;
  friend bool operator==(const provider_fixture_report &, const provider_fixture_report &) = default;
};
// Run against both bundled backends so name discovery cannot depend on native
// storage or reference snapshot APIs at the application boundary.
inline void check_provider_named_nodes(const provider_factory &factory) {
  const auto require = [](bool ok, const char *why) {
    if (!ok) throw std::runtime_error(why);
  };
  const auto rejected = [&](const auto &result, error_code code) {
    require(!result && result.error().code == code, "named-node authority/error mismatch");
  };
  auto service = factory().value();
  const auto scene = service->create_scene().value();
  const auto context = service->root_context(scene).value();
  require(service->named_nodes(context).value().empty(), "empty name table");
  const auto z = service->create_node(context, "Transform").value();
  const auto a = service->create_node(context, "Transform").value();
  const auto unnamed = service->create_node(context, "Transform").value();
  require(service->names(unnamed).value().empty(), "unnamed node has no names");
  require(bool(service->define_name(z, "Zulu")), "name detached node");
  const auto retained = service->named_nodes(context).value();
  require(bool(service->define_name(a, "Alpha")), "name second node");
  require(bool(service->append_root(context, a)), "append named root");
  rejected(service->define_name(unnamed, "Alpha"), error_code::duplicate_name);
  rejected(service->define_name(z, "Alias"), error_code::duplicate_name);
  const std::vector<p::named_node> expected{{"Alpha", a}, {"Zulu", z}};
  require(service->named_nodes(context).value() == expected,
          "sorted enumeration includes detached nodes and excludes unnamed nodes");
  require(service->names(z).value() == std::vector<std::string>{"Zulu"}, "reverse name");
  for (const auto &entry : expected)
    require(service->named(context, entry.name).value() == entry.target, "lookup identity round trip");
  const auto other_scene = service->create_scene().value();
  const auto other_context = service->root_context(other_scene).value();
  const auto other_node = service->create_node(other_context, "Transform").value();
  require(bool(service->define_name(other_node, "Alpha")), "scene-local duplicate name");
  require(service->named_nodes(other_context).value() ==
              std::vector<p::named_node>{{"Alpha", other_node}}, "scene isolation");
  auto foreign = factory().value();
  rejected(foreign->named_nodes(context), error_code::invalid_context);
  rejected(foreign->names(z), error_code::invalid_context);
  auto disposed_context = context;
  disposed_context.dispose();
  auto disposed_node = z;
  disposed_node.dispose();
  rejected(service->named_nodes(disposed_context), error_code::disposed);
  rejected(service->names(disposed_node), error_code::disposed);
  rejected(service->named_nodes(p::context{}), error_code::stale_handle);
  rejected(service->names(p::node{}), error_code::stale_handle);
  bool wrong_context = false, wrong_node = false;
  std::thread worker([&] {
    const auto entries = service->named_nodes(context);
    const auto names = service->names(z);
    wrong_context = !entries && entries.error().code == error_code::wrong_thread;
    wrong_node = !names && names.error().code == error_code::wrong_thread;
  });
  worker.join();
  require(wrong_context && wrong_node, "discovery requires owner thread");
  require(bool(service->activate(scene)), "activate named scene");
  require(service->named_nodes(context).value() == expected, "names survive activation");
  require(service->names(z).value() == std::vector<std::string>{"Zulu"}, "live reverse name");
  require(bool(service->retire(scene)), "retire named scene");
  rejected(service->named_nodes(context), error_code::stale_handle);
  rejected(service->names(z), error_code::stale_handle);
  service.reset();
  require(retained == std::vector<p::named_node>{{"Zulu", z}}, "owning enumeration snapshot survives edits and teardown");
}
namespace detail {
struct provider_fixture_keys {
  static constexpr std::string_view x3d_name = "Transform";
  inline static constexpr field_key<provider_fixture_keys, bool> wrong_kind{"translation", access_type::input_output};
  inline static constexpr field_key<provider_fixture_keys, vec3f> wrong_access{"translation", access_type::output_only};
  inline static constexpr field_key<provider_fixture_keys, node_list> nodes_wrong_kind{"translation", access_type::input_output};
  inline static constexpr field_key<provider_fixture_keys, node_list> nodes_wrong_access{"children", access_type::output_only};
  inline static constexpr field_key<provider_fixture_keys, node_list> nodes_unknown{"noSuchField", access_type::input_output};
  inline static constexpr field_key<provider_fixture_keys, vec3f> unknown{"noSuchField", access_type::input_output};
};
template <class Key, class Payload>
concept provider_key_writable = requires(p::service &service, p::node node, Key key, Payload value) { service.author(node, key, value); };
static_assert(provider_key_writable<decltype(bindings::Transform::translation), vec3f>);
static_assert(!provider_key_writable<decltype(bindings::Transform::translation), bool>);
static_assert(!provider_key_writable<decltype(bindings::Transform::metadata), node_id>);
static_assert(!provider_key_writable<decltype(bindings::Transform::children), node_list>);
static_assert(provider_key_writable<decltype(bindings::Transform::metadata), p::node_value>);
static_assert(provider_key_writable<decltype(bindings::Transform::children), p::node_values>);
static_assert(!std::is_constructible_v<p::payload, node_id>);
static_assert(!std::is_constructible_v<p::payload, node_list>);
inline p::context setup_context(p::service &service) {
  return service.root_context(service.create_scene().value()).value();
}
inline result<p::node> separate_node(p::service &service) {
  return service.create_node(setup_context(service), "Transform");
}
// These private test helpers only compose public field lookup with the single
// read/author/user_data operation; they hold no authority or alternative state.
inline result<p::payload> read(p::service &s, const p::node &n, std::string_view name) {
  auto field=s.get_field(n,name); if(!field) return failure(field.error()); return s.read(*field);
}
template<class O,class T> auto read(p::service &s,const p::node &n,const field_key<O,T> &key) { return s.read(n,key); }
inline result<void> author(p::service &s,const p::node &n,std::string_view name,p::payload value) {
  auto field=s.get_field(n,name); if(!field) return failure(field.error()); return s.author(*field,std::move(value));
}
template<class O,class T> result<void> author(p::service &s,const p::node &n,const field_key<O,T> &key,p::semantic_value_t<T> value) { return s.author(n,key,std::move(value)); }
template<class Key> result<p::node_values> nodes(p::service &s,const p::node &n,const Key &key) {
  auto field=s.get_field(n,key); if(!field) return failure(field.error());
  auto value=s.read(*field); if(!value) return failure(value.error()); return std::get<p::node_values>(*value);
}
template<class Key> result<void> set_nodes(p::service &s,const p::node &n,const Key &key,p::node_values value) {
  auto field=s.get_field(n,key); if(!field) return failure(field.error()); return s.author(*field,p::payload{std::move(value)});
}
template<class Key> result<user_data_value> user_data(p::service &s,const p::node &n,const Key &key) {
  auto field=s.get_field(n,key); if(!field) return failure(field.error()); return s.user_data(*field);
}
template<class Key> result<void> set_user_data(p::service &s,const p::node &n,const Key &key,user_data_value value) {
  auto field=s.get_field(n,key); if(!field) return failure(field.error()); return s.set_user_data(*field,std::move(value));
}
inline bool same_capabilities(const p::capabilities &a,const p::capabilities &b) {
  return a.backend_name==b.backend_name && a.contract_version==b.contract_version && a.services==b.services && a.nodes==b.nodes && a.field_kinds==b.field_kinds;
}
} // namespace detail
// Cleanup probes intentionally call back into the provider. Their state lives
// outside payload ownership so destruction/replacement can be verified exactly.
struct provider_cleanup_probe {
  explicit provider_cleanup_probe(std::function<void()> action)
      : action(std::move(action)) {}
  ~provider_cleanup_probe() noexcept { action(); }
  std::function<void()> action;
};

inline void run_provider_user_data_fixture(
    const provider_factory &factory,
    provider_fixture_report &report) {
  const auto require = [&](bool condition, const char *message) {
    ++report.checks;
    if (!condition)
      throw std::runtime_error(message);
  };
  const auto expect_error = [&](const auto &result, error_code code,
                                const char *message) {
    require(!result && result.error().code == code, message);
  };
  const auto create = [&]() {
    auto made = factory();
    require(made && bool(*made), "user-data provider creation must succeed");
    return std::move(*made);
  };
  auto provider = create();
  auto &scene = *provider;
  const auto context = detail::setup_context(scene);
  auto node = *scene.create_node(context, "Transform");
  const auto second = *scene.create_node(context, "Transform");
  require(bool(scene.define_name(node, "MetadataOwner")) &&
              bool(scene.append_root(context, node)),
          "metadata node must support identity reacquisition");
  auto field = scene.get_field(node, "translation");
  require(field && field->name() == "translation" && !field->is_null() &&
              !field->disposed() && !field->expired(),
          "field handle must own canonical identity with provider authority");
  require(*scene.get_field(node, "set_translation") == *field &&
              *scene.get_field(node, "translation_changed") == *field &&
              *scene.get_field(node, bindings::Transform::translation) == *field,
          "canonical dynamic aliases and generated keys must share a field");
  auto empty = scene.user_data(node);
  auto empty_field = scene.user_data(*field);
  require(empty && empty_field && !empty->has_value() &&
              !empty_field->has_value() && empty->as<int>() &&
              !*empty->as<int>(),
          "empty node and field associations must return empty owning values");
  require(bool(scene.set_user_data(node, user_data_value::make<std::string>("node"))),
          "node user data must be writable immediately");
  auto owned = scene.user_data(node);
  auto text = owned->as<std::string>();
  require(text && *text && **text == "node", "exact payload type must be preserved");
  expect_error(owned->as<int>(), error_code::type_mismatch,
               "user data must never coerce the stored type");
  auto copy = node;
  require(*scene.user_data(copy)->as<std::string>() == *text &&
              *scene.user_data(*scene.named(context, "MetadataOwner"))->as<std::string>() == *text &&
              *scene.user_data(scene.roots(context)->front())->as<std::string>() == *text,
          "copied named and rooted node references must share owning payloads");
  require(!scene.user_data(second)->has_value() &&
              !detail::user_data(scene, second, "translation")->has_value(),
          "distinct nodes must not share user data");
  // Include SFNode, initializeOnly and inputOnly fields with unavailable scalar
  // operations. Metadata support is independent of field read/write flags.
  const auto before_fields = *scene.fields(node);
  const auto before_roots = *scene.roots(context);
  const auto before_value = *detail::read(scene, node, "translation");
  int ordinal = 0;
  for (const auto &descriptor : before_fields) {
    const auto handle = scene.get_field(node, descriptor.name);
    require(handle && bool(scene.set_user_data(
                          *handle, user_data_value::make<int>(++ordinal))),
            "every semantic field must accept user data");
    const auto data = detail::user_data(scene, node, descriptor.name);
    require(data && data->as<int>() && **data->as<int>() == ordinal,
            "canonical field lookup must retrieve its independent metadata");
  }
  require(detail::user_data(scene, node, bindings::Transform::metadata)->as<int>() &&
              detail::user_data(scene, node, bindings::Transform::addChildren)->as<int>(),
          "generated node/event keys must support metadata without value support");
  require(bool(detail::set_user_data(scene, node, "set_translation",
                                   user_data_value::make<std::string>("field"))) &&
              **detail::user_data(scene, node, "translation_changed")->as<std::string>() == "field" &&
              **detail::user_data(scene, node, bindings::Transform::translation)->as<std::string>() == "field",
          "all field spellings must share metadata");
  report.user_data_trace.push_back(**scene.user_data(node)->as<std::string>());
  report.user_data_trace.push_back(**scene.user_data(*field)->as<std::string>());
  require(*scene.fields(node) == before_fields && *scene.roots(context) == before_roots &&
              *detail::read(scene, node, "translation") == before_value &&
              *scene.named(context, "MetadataOwner") == node,
          "metadata must not alter field support, scene values, roots or names");
  // Alias widening is intentionally shared by dynamic scalar/node operations.
  require(*detail::read(scene, node, "set_translation") == before_value &&
              bool(detail::author(scene, node, "translation_changed", vec3f{3, 4, 5})) &&
              *detail::read(scene, node, bindings::Transform::translation) == vec3f{3, 4, 5} &&
              bool(detail::author(scene, node, "set_translation", std::get<vec3f>(before_value))),
          "dynamic value aliases must resolve the same canonical scalar field");
  require(bool(detail::set_nodes(scene, node, "set_children", {second})) &&
              *detail::nodes(scene, node, "children_changed") == p::node_values{second} &&
              bool(detail::set_nodes(scene, node, "children", {})),
          "dynamic node aliases must preserve owner-bearing list operations");
  for (const auto name : {"noSuchField", "set_bboxCenter", "bboxSize_changed",
                          "set_addChildren", "removeChildren_changed", "DEF"}) {
    expect_error(scene.get_field(node, name), error_code::unknown_field,
                 "aliases apply only to actual inputOutput fields");
    expect_error(detail::user_data(scene, node, name), error_code::unknown_field,
                 "unknown field metadata reads must fail");
    expect_error(detail::set_user_data(scene, node, name, {}), error_code::unknown_field,
                 "unknown field metadata writes must fail");
  }
  expect_error(scene.get_field(node, bindings::Group::children), error_code::type_mismatch,
               "metadata key owner must match the node type");
  expect_error(detail::user_data(scene, node, detail::provider_fixture_keys::wrong_kind),
               error_code::type_mismatch, "metadata keys must preserve exact kinds");
  expect_error(detail::set_user_data(scene, node, detail::provider_fixture_keys::wrong_access, {}),
               error_code::invalid_descriptor, "metadata keys must preserve exact access");
  expect_error(detail::user_data(scene, node, detail::provider_fixture_keys::unknown),
               error_code::unknown_field, "unknown metadata key must fail");
  auto foreign_provider = create();
  const auto foreign = *detail::separate_node(*foreign_provider);
  const auto foreign_field = *foreign_provider->get_field(foreign, "translation");
  expect_error(scene.user_data(foreign), error_code::invalid_context,
               "foreign node cannot access local metadata");
  expect_error(scene.set_user_data(foreign, {}), error_code::invalid_context,
               "foreign node cannot write local metadata");
  expect_error(scene.user_data(foreign_field), error_code::invalid_context,
               "foreign field cannot access local metadata");
  expect_error(scene.set_user_data(foreign_field, {}), error_code::invalid_context,
               "foreign field cannot write local metadata");
  node.dispose();
  require(**scene.user_data(*field)->as<std::string>() == "field",
          "field handle remains usable after origin node wrapper disposal");
  expect_error(scene.user_data(node), error_code::disposed,
               "disposed node cannot read metadata");
  expect_error(scene.set_user_data(node, {}), error_code::disposed,
               "disposed node cannot write metadata");
  auto disposed_field = *field;
  disposed_field.dispose();
  require(disposed_field == *field && !field->disposed() &&
              **scene.user_data(*scene.get_field(copy, "translation"))->as<std::string>() == "field",
          "field disposal preserves copied/reacquired identity and metadata");
  expect_error(scene.user_data(disposed_field), error_code::disposed,
               "disposed field cannot read metadata");
  expect_error(scene.set_user_data(disposed_field, {}), error_code::disposed,
               "disposed field cannot write metadata");
  expect_error(scene.user_data(p::node{}), error_code::stale_handle,
               "raw empty node has no metadata authority");
  expect_error(scene.user_data(p::field{}), error_code::stale_handle,
               "raw empty field has no metadata authority");
  foreign_provider.reset();
  expect_error(scene.user_data(foreign), error_code::stale_handle,
               "expired node precedes foreign context errors");
  expect_error(scene.user_data(foreign_field), error_code::stale_handle,
               "expired field precedes foreign context errors");
  result<user_data_value> off_thread_node, off_thread_field;
  result<void> off_thread_node_write, off_thread_field_write;
  result<p::field> off_thread_lookup;
  std::thread wrong_thread([&] {
    off_thread_node = scene.user_data(copy);
    off_thread_field = scene.user_data(*field);
    off_thread_node_write = scene.set_user_data(copy, {});
    off_thread_field_write = scene.set_user_data(*field, {});
    off_thread_lookup = scene.get_field(copy, "translation");
  });
  wrong_thread.join();
  expect_error(off_thread_node, error_code::wrong_thread, "node metadata read is confined");
  expect_error(off_thread_field, error_code::wrong_thread, "field metadata read is confined");
  expect_error(off_thread_node_write, error_code::wrong_thread, "node metadata write is confined");
  expect_error(off_thread_field_write, error_code::wrong_thread, "field metadata write is confined");
  expect_error(off_thread_lookup, error_code::wrong_thread, "field handle lookup is confined");
  bool replacement_observed = false, clear_observed = false;
  require(bool(scene.set_user_data(copy, user_data_value::make<provider_cleanup_probe>([&] {
    auto latest = scene.user_data(copy);
    replacement_observed = latest && latest->as<int>() && **latest->as<int>() == 42;
  }))), "node cleanup probe must install");
  require(bool(scene.set_user_data(copy, user_data_value::make<int>(42))) && replacement_observed,
          "replacement cleanup must re-enter and observe the new node association");
  require(bool(scene.set_user_data(*field, user_data_value::make<provider_cleanup_probe>([&] {
    auto latest = scene.user_data(*field);
    clear_observed = latest && !latest->has_value();
  }))) && bool(scene.set_user_data(*field, {})) && clear_observed,
          "clear cleanup must re-enter and observe an empty field association");
  require(bool(scene.close()) && field->expired() && copy.expired(),
          "close must expire node and field handles together");
  require(**text == "node" && **owned->as<std::string>() == "node",
          "owning metadata reads must survive replacement and provider close");
  expect_error(scene.user_data(copy), error_code::stale_handle,
               "closed provider cannot read node metadata");
  expect_error(scene.user_data(*field), error_code::stale_handle,
               "closed provider cannot read field metadata");
  expect_error(scene.set_user_data(*field, {}), error_code::stale_handle,
               "closed provider cannot write field metadata");

  // Both replacement and clear may destroy the final payload reference. In
  // either case closing from cleanup must not leave a dangling state access.
  for (const bool field_target : {false, true}) {
    for (const bool clear : {false, true}) {
      auto closing = create();
      const auto target = *detail::separate_node(*closing);
      const auto target_field = *closing->get_field(target, "translation");
      int destroyed = 0;
      bool nested_close = false;
      auto payload = user_data_value::make<provider_cleanup_probe>([&] {
        ++destroyed;
        nested_close = bool(closing->close());
      });
      require(bool(field_target ? closing->set_user_data(target_field, std::move(payload))
                                : closing->set_user_data(target, std::move(payload))),
              "closing cleanup probe must install");
      auto next = clear ? user_data_value{} : user_data_value::make<int>(7);
      auto changed = field_target ? closing->set_user_data(target_field, std::move(next))
                                  : closing->set_user_data(target, std::move(next));
      require(changed && destroyed == 1 && nested_close && target.expired() && target_field.expired(),
              "replacement/clear cleanup may close the provider safely");
    }
  }
  // A read pins just its immutable payload, never provider handle authority.
  {
    auto ownership = create();
    const auto target = *detail::separate_node(*ownership);
    int destroyed = 0;
    require(bool(ownership->set_user_data(
                target, user_data_value::make<provider_cleanup_probe>([&] { ++destroyed; }))),
            "retained ownership probe must install");
    auto retained = *ownership->user_data(target);
    auto typed = *retained.as<provider_cleanup_probe>();
    require(bool(ownership->set_user_data(target, {})) && destroyed == 0 &&
                !ownership->user_data(target)->has_value() && bool(ownership->close()) &&
                target.expired() && destroyed == 0,
            "retained metadata must outlive clear and close without retaining handle authority");
    retained = {};
    require(destroyed == 0, "typed payload ownership survives value-wrapper release");
    typed.reset();
    require(destroyed == 1, "the final owning read releases a payload exactly once");
  }
  // Explicit and implicit owner-thread teardown retire authority before payloads.
  for (const bool implicit : {false, true}) {
    auto retiring = create(); auto *raw = retiring.get();
    const auto target=detail::separate_node(*raw).value();
    const auto target_field=raw->get_field(target,"translation").value();
    int destroyed=0; bool guarded=true;
    const auto cleanup=[&] {
      ++destroyed;
      auto a=raw->user_data(target); auto b=raw->user_data(target_field);
      guarded=guarded && !a && a.error().code==error_code::stale_handle &&
              !b && b.error().code==error_code::stale_handle && bool(raw->close());
    };
    require(bool(raw->set_user_data(target,user_data_value::make<provider_cleanup_probe>(cleanup))) &&
            bool(raw->set_user_data(target_field,user_data_value::make<provider_cleanup_probe>(cleanup))),
            "teardown cleanup probes must install");
    result<void> denied;
    std::thread wrong([&]{denied=raw->close();}); wrong.join();
    expect_error(denied,error_code::wrong_thread,"wrong-thread close must fail before teardown");
    require(destroyed==0 && !target.expired(),"rejected close keeps metadata and authority alive");
    if(implicit) retiring.reset(); else require(bool(raw->close()),"explicit metadata teardown succeeds");
    require(destroyed==2 && guarded && target.expired() && target_field.expired(),
            "teardown cleanup sees retired authority without accessing backend state");
  }
}

inline provider_fixture_report run_provider_authoring_fixture(
    const provider_factory &factory) {
  provider_fixture_report report;
  run_provider_user_data_fixture(factory, report);
  const auto require = [&](bool condition, const char *message) {
    ++report.checks;
    if (!condition)
      throw std::runtime_error(message);
  };
  const auto expect_error = [&](const auto &result, error_code code,
                                const char *message) {
    require(!result && result.error().code == code, message);
  };
  const auto same_success = [](const auto &a, const auto &b) {
    return a && b && *a == *b;
  };
  auto made = factory();
  require(bool(made) && bool(*made), "factory must create a provider");
  auto provider = std::move(*made);
  auto &scene = *provider;
  const auto context = detail::setup_context(scene);
  const auto caps = scene.supported();
  require(caps.contract_version == 1 && caps.supports(p::service_id::scene_authoring) &&
              caps.supports(p::service_id::host_turns) && caps.supports(p::service_id::node_values) &&
              caps.supports(p::service_id::user_data),
          "versioned capabilities advertise the exercised setup and live services");
  auto roots = scene.roots(context);
  require(roots && roots->empty(), "new scene must be empty");
  expect_error(scene.create_node(context, "NotAnX3DNode"), error_code::unknown_type,
               "unknown type must fail");

  auto created = scene.create_node(context, "Transform");
  auto second = scene.create_node(context, "Transform");
  require(bool(created) && bool(second), "Transform creation must succeed");
  auto node = *created;
  require(node != *second && !node.is_null() && !node.expired(),
          "created node identity must be distinct and live");
  require(same_success(scene.type_name(node), result<std::string>{"Transform"}),
          "type discovery must reflect the created node");
  require(bool(scene.define_name(node, "Root_1")), "DEF naming must succeed");
  auto named = scene.named(context, "Root_1");
  require(named && *named == node, "name lookup must preserve identity");
  expect_error(scene.define_name(*second, "Root_1"), error_code::duplicate_name,
               "duplicate name must fail");
  expect_error(scene.define_name(node, "Alias"), error_code::duplicate_name,
               "renaming is outside this subset");
  expect_error(scene.named(context, "Alias"), error_code::unknown_node,
               "failed rename must not publish an alias");
  expect_error(scene.define_name(*second, ""), error_code::invalid_name,
               "empty name must fail");
  expect_error(scene.define_name(*second, "bad name"), error_code::invalid_name,
               "unsupported DEF grammar must fail");
  expect_error(scene.define_name(*second, "7bad"), error_code::invalid_name,
               "numeric initial character must fail");
  expect_error(scene.named(context, "Missing"), error_code::unknown_node,
               "missing DEF must fail");
  require(same_success(scene.named(context, "Root_1"), named),
          "rejected names must not replace existing bindings");
  require(bool(scene.append_root(context, node)) && bool(scene.append_root(context, *second)) &&
              bool(scene.append_root(context, node)),
          "distinct and shared root occurrences must be accepted");
  roots = scene.roots(context);
  require(roots && roots->size() == 3 && (*roots)[0] == node &&
              (*roots)[1] == *second && (*roots)[2] == node,
          "distinct root ordering and shared-node identity must be preserved");
  auto fields = scene.fields(node);
  require(bool(fields), "field discovery must succeed");
  report.fields = *fields;
  // All semantic Transform fields, including inherited fields. Document syntax
  // (DEF/USE/IS/class/id/style) must not leak into field discovery.
  require(fields->size() == bindings::Transform::field_keys.size(),
          "generated keys and discovered semantic field counts must match");
  std::vector<p::field_info> expected = *fields;
  for (const auto &key : bindings::Transform::field_keys) {
    const auto descriptor=std::find_if(fields->begin(),fields->end(),[&](const auto &f){return f.name==key.name;});
    require(descriptor!=fields->end() && descriptor->kind==key.kind && descriptor->access==key.access,
            "every existing generated key matches a discovered semantic descriptor");
  }
  std::sort(report.fields.begin(),report.fields.end(),[](const auto &a,const auto &b){return a.name<b.name;});
  const auto read_typed = [&](const field_key<bindings::Transform, vec3f> &key,
                              vec3f expected_value) {
    auto typed = detail::read(scene, node, key);
    auto dynamic = detail::read(scene, node, key.name());
    require(
        typed && *typed == expected_value && dynamic &&
            std::holds_alternative<vec3f>(*dynamic) &&
            *typed == std::get<vec3f>(*dynamic),
        "generated-key and dynamic reads must return the same owning value");
    report.typed_read_trace.push_back(*typed);
  };
  const auto read = [&](std::string_view name, vec3f expected_value) {
    auto value = detail::read(scene, node, name);
    require(value && std::holds_alternative<vec3f>(*value) &&
                std::get<vec3f>(*value) == expected_value,
            "SFVec3f read must match authoritative state");
    report.read_trace.push_back(std::get<vec3f>(*value));
  };
  read("translation", {0, 0, 0});
  read("center", {0, 0, 0});
  read("bboxCenter", {0, 0, 0});
  read("scale", {1, 1, 1});
  read("bboxSize", {-1, -1, -1});
  read_typed(bindings::Transform::translation, {0, 0, 0});
  read_typed(bindings::Transform::center, {0, 0, 0});
  read_typed(bindings::Transform::bboxCenter, {0, 0, 0});
  read_typed(bindings::Transform::scale, {1, 1, 1});
  read_typed(bindings::Transform::bboxSize, {-1, -1, -1});
  for (const auto &key :
       {bindings::Transform::translation, bindings::Transform::center,
        bindings::Transform::scale}) {
    require(bool(detail::author(scene, node, key.name(), vec3f{2, 3, 4})),
            "SFVec3f dynamic authoring write must succeed");
    read_typed(key, {2, 3, 4});
    require(bool(detail::author(scene, node, key, vec3f{5, 6, 7})),
            "SFVec3f generated-key authoring write must succeed");
    read(key.name(), {5, 6, 7});
    read_typed(key, {5, 6, 7});
    require(bool(detail::author(scene, node, key, vec3f{2, 3, 4})),
            "repeated generated-key write must succeed");
    read(key.name(), {2, 3, 4});
  }
  // Independent stores execute the same owning bool and rotation vocabulary.
  // Preserve old returned values after subsequent service calls.
  for (const auto &key : {bindings::Transform::bboxDisplay,
                          bindings::Transform::visible}) {
    const bool initial = key.name() == "visible";
    auto old_value = detail::read(scene, node, key);
    require(old_value && *old_value == initial, "SFBool defaults must match");
    require(bool(detail::author(scene, node, key.name(), !initial)),
            "dynamic SFBool write must succeed");
    auto typed = detail::read(scene, node, key);
    require(typed && *typed == !initial && *old_value == initial,
            "owning bool read must survive later mutation");
    report.bool_trace.push_back(*typed);
    require(bool(detail::author(scene, node, key, initial)),
            "typed SFBool write must succeed");
    auto dynamic = detail::read(scene, node, key.name());
    require(dynamic && std::get<bool>(*dynamic) == initial,
            "typed and dynamic bool access must agree");
    report.bool_trace.push_back(std::get<bool>(*dynamic));
    expect_error(detail::author(scene, node, key.name(), vec3f{}),
                 error_code::type_mismatch, "bool wrong-kind write must fail");
    require(*detail::read(scene, node, key) == initial,
            "rejected bool write must preserve storage");
  }
  for (const auto &key : {bindings::Transform::rotation,
                          bindings::Transform::scaleOrientation}) {
    auto old_value = detail::read(scene, node, key);
    require(old_value && *old_value == rotation{}, "SFRotation defaults must match");
    const rotation first{1, 0, 0, 0.75f};
    require(bool(detail::author(scene, node, key.name(), first)),
            "dynamic SFRotation write must succeed");
    auto typed = detail::read(scene, node, key);
    require(typed && *typed == first && *old_value == rotation{},
            "owning rotation read must survive later mutation");
    report.rotation_trace.push_back(*typed);
    const rotation second_rotation{0, 1, 0, -0.5f};
    require(bool(detail::author(scene, node, key, second_rotation)),
            "typed SFRotation write must succeed");
    auto dynamic = detail::read(scene, node, key.name());
    require(dynamic && std::get<rotation>(*dynamic) == second_rotation,
            "typed and dynamic rotation access must agree");
    report.rotation_trace.push_back(std::get<rotation>(*dynamic));
    expect_error(detail::author(scene, node, key.name(), vec3f{}),
                 error_code::type_mismatch, "rotation wrong-kind write must fail");
    require(*detail::read(scene, node, key) == second_rotation,
            "rejected rotation write must preserve storage");
  }
  // Negative scale reflects; zero scale hides geometry. Neither is invalid.
  require(bool(detail::author(scene, node, bindings::Transform::scale, vec3f{-2, 0, 3})),
          "negative and zero Transform scale must remain valid");
  read_typed(bindings::Transform::scale, {-2, 0, 3});
  require(bool(detail::author(scene, node, bindings::Transform::scale, vec3f{2, 3, 4})),
          "scale restore must succeed");
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  for (const auto &key : {bindings::Transform::rotation,
                          bindings::Transform::scaleOrientation}) {
    const auto before = detail::read(scene, node, key).value();
    for (const auto &invalid : {rotation{0, 0, 0, 0}, rotation{2, 0, 0, 0},
                                rotation{1, 1, 0, 1}, rotation{nan, 0, 0, 0},
                                rotation{0, inf, 0, 1}}) {
      expect_error(detail::author(scene, node, key.name(), invalid),
                   error_code::invalid_value, "invalid dynamic rotation axis must fail");
      expect_error(detail::author(scene, node, key, invalid),
                   error_code::invalid_value, "invalid typed rotation axis must fail");
      require(detail::read(scene, node, key).value() == before,
              "rejected rotation axis must not mutate storage");
    }
    const rotation rounded_unit{0.57735026f, 0.57735026f, 0.57735026f, 1000};
    require(bool(detail::author(scene, node, key, rounded_unit)),
            "rounded normalized axis and large angle must succeed");
    require(detail::read(scene, node, key).value() == rounded_unit,
            "accepted rotation components must not be silently normalized");
    report.rotation_trace.push_back(detail::read(scene, node, key).value());
    require(bool(detail::author(scene, node, key, before)), "rotation restore must succeed");
  }
  auto second_before = detail::read(scene, *second, "translation");
  require(second_before && std::get<vec3f>(*second_before) == vec3f{0, 0, 0},
          "writing A must not change B's default field storage");
  require(bool(detail::author(scene, *second, "translation", vec3f{11, 12, 13})),
          "writing a distinct node must succeed");
  auto second_after = detail::read(scene, *second, "translation");
  require(second_after && std::get<vec3f>(*second_after) == vec3f{11, 12, 13},
          "B must retain its own field value");
  report.read_trace.push_back(std::get<vec3f>(*second_before));
  report.read_trace.push_back(std::get<vec3f>(*second_after));
  read("translation", {2, 3, 4});
  expect_error(detail::author(scene, node, "translation", true),
               error_code::type_mismatch,
               "wrong kind must fail before mutation");
  expect_error(detail::author(scene, node, "translation", std::monostate{}),
               error_code::type_mismatch,
               "empty value must fail before mutation");
  require(bool(detail::author(scene, node, "bboxCenter", vec3f{9, 9, 9})),
          "initializeOnly fields are writable during setup");

  expect_error(detail::read(scene, node, "addChildren"), error_code::access_denied,
               "inputOnly read must be denied");
  for (const auto &name :
       {"DEF", "USE", "IS", "class", "id", "style", "noSuchField"}) {
    expect_error(detail::read(scene, node, name), error_code::unknown_field,
                 "non-semantic field read must fail");
    expect_error(detail::author(scene, node, name, vec3f{}),
                 error_code::unknown_field,
                 "non-semantic field write must fail");
  }
  expect_error(detail::read(scene, node, bindings::Group::bboxCenter),
               error_code::type_mismatch,
               "equal name/kind/access cannot grant a different key owner");
  expect_error(detail::author(scene, node, bindings::Group::bboxCenter, vec3f{}),
               error_code::type_mismatch,
               "wrong owner must fail before field support checks");
  expect_error(
      detail::read(scene, node, detail::provider_fixture_keys::wrong_kind),
      error_code::type_mismatch, "typed key kind must be checked");
  expect_error(
      detail::author(scene, node, detail::provider_fixture_keys::wrong_kind, true),
      error_code::type_mismatch, "typed write key kind must be checked");
  expect_error(
      detail::read(scene, node, detail::provider_fixture_keys::wrong_access),
      error_code::invalid_descriptor, "typed key access must be checked");
  expect_error(detail::author(scene,
                   node, detail::provider_fixture_keys::wrong_access, vec3f{}),
               error_code::invalid_descriptor,
               "typed key cannot invent different access");
  expect_error(detail::read(scene, node, detail::provider_fixture_keys::unknown),
               error_code::unknown_field, "unknown typed read key must fail");
  expect_error(
      detail::author(scene, node, detail::provider_fixture_keys::unknown, vec3f{}),
      error_code::unknown_field, "unknown typed write key must fail");
  require(bool(detail::author(scene, node, bindings::Transform::bboxCenter, vec3f{})),
          "generated initializeOnly key shares setup authoring");

  expect_error(detail::read(scene, node, bindings::Transform::addChildren),
               error_code::access_denied,
               "generated-key inputOnly read must be denied");
  require(!scene.read(node, bindings::Transform::metadata).value().has_value(),
          "default SFNode is explicit owning NULL, never an exposed numeric ID");
  auto sf_owner=scene.create_node(context,"Shape").value();
  auto sf_child=scene.create_node(context,"Box").value();
  require(bool(scene.author(sf_owner, bindings::Shape::geometry, p::node_value{sf_child})) &&
          scene.read(sf_owner, bindings::Shape::geometry).value()==p::node_value{sf_child},
          "generated SFNode key projects owner-bearing node authority");
  require(bool(scene.author(sf_owner, bindings::Shape::geometry, p::node_value{})) &&
          !scene.read(sf_owner, bindings::Shape::geometry).value().has_value(),
          "explicit SFNode NULL clears an owning node field");
  expect_error(scene.author(sf_owner,bindings::Shape::geometry,p::node_value{node}),
               error_code::type_mismatch,"geometry requires a compatible geometry node");
  expect_error(scene.author(node,bindings::Transform::metadata,p::node_value{*second}),
               error_code::type_mismatch,"metadata requires a compatible metadata node");
  read("translation", {2, 3, 4});
  read("bboxCenter", {0, 0, 0});
  require(same_success(scene.roots(context), roots) &&
              same_success(scene.named(context, "Root_1"), named),
          "rejected operations must preserve structure and names");

  auto foreign_made = factory();
  require(bool(foreign_made), "second provider must be created");
  auto foreign = detail::separate_node(**foreign_made);
  require(bool(foreign), "foreign node creation must succeed");
  require(*foreign != node, "identity includes the provider domain");
  expect_error(scene.author(node,bindings::Transform::metadata,p::node_value{*foreign}),
               error_code::invalid_context,"SFNode import rejects foreign service authority");
  expect_error(scene.author(node,bindings::Transform::metadata,p::node_value{p::node{}}),
               error_code::stale_handle,"engaged empty SFNode is invalid rather than NULL");
  auto other_context=detail::setup_context(scene);
  auto other_context_node=scene.create_node(other_context,"Transform").value();
  expect_error(scene.author(node,bindings::Transform::metadata,p::node_value{other_context_node}),
               error_code::invalid_context,"SFNode import rejects another lexical scene");
  expect_error(scene.append_root(context,other_context_node),error_code::invalid_context,
               "root mutation cannot move a node across lexical scenes");
  expect_error(detail::read(scene, *foreign, "translation"),
               error_code::invalid_context, "foreign handle must be rejected");
  expect_error(detail::read(scene, *foreign, bindings::Transform::translation),
               error_code::invalid_context,
               "typed foreign read must be rejected");
  expect_error(
      detail::author(scene, *foreign, bindings::Transform::translation, vec3f{}),
      error_code::invalid_context, "typed foreign write must be rejected");
  expect_error(scene.append_root(context, *foreign), error_code::invalid_context,
               "foreign mutation must be rejected");
  expect_error(scene.type_name(p::node{}), error_code::stale_handle,
               "empty handle must fail");

  auto disposed = node;
  disposed.dispose();
  disposed.dispose();
  require(disposed.disposed() && disposed == node && !node.disposed(),
          "disposal must be local, idempotent and identity preserving");
  expect_error(scene.author(sf_owner,bindings::Shape::geometry,p::node_value{disposed}),
               error_code::disposed,"engaged disposed SFNode does not become NULL");
  expect_error(detail::read(scene, disposed, "translation"),
               error_code::disposed, "disposed wrapper must fail");
  expect_error(detail::read(scene, disposed, bindings::Transform::translation),
               error_code::disposed, "typed disposed read must fail");
  expect_error(
      detail::author(scene, disposed, bindings::Transform::translation, vec3f{}),
      error_code::disposed, "typed disposed write must fail");
  read("translation", {2, 3, 4});

  // Owner-bearing MFNode children use the same generated keys, with explicit
  // NULL slots and one completed authoring operation per call.
  std::vector<p::node> known{node, *second};
  const auto make_node = [&]() {
    auto made_node = scene.create_node(context, "Transform");
    require(bool(made_node), "children fixture node creation must succeed");
    known.push_back(*made_node);
    return *made_node;
  };
  const auto child_a = make_node();
  const auto child_b = make_node();
  const auto other_parent = make_node();
  require(bool(scene.define_name(child_a, "RetainedChild")),
          "detached child naming must succeed");
  const auto nodes_read = [&](const p::node &parent,
                              const p::node_values &expected_nodes) {
    const auto dynamic = detail::nodes(scene, parent, "children");
    const auto typed = detail::nodes(scene, parent, bindings::Transform::children);
    require(dynamic && typed && *dynamic == expected_nodes &&
                *typed == *dynamic,
            "typed and dynamic children reads must preserve order and NULL");
    std::vector<std::optional<std::size_t>> trace;
    for (const auto &item : *dynamic) {
      if (!item) {
        trace.emplace_back(std::nullopt);
        continue;
      }
      const auto position = std::find(known.begin(), known.end(), *item);
      require(position != known.end(),
              "children reads must preserve known identity");
      trace.emplace_back(static_cast<std::size_t>(position - known.begin()));
    }
    report.node_read_trace.push_back(std::move(trace));
  };
  const auto graph = [&]() {
    std::vector<p::node_values> result;
    for (const auto &item : known) {
      auto children = detail::nodes(scene, item, "children");
      require(bool(children), "valid graph must remain readable");
      result.push_back(*children);
    }
    return result;
  };
  const auto reject_nodes = [&](auto action, error_code code) {
    const auto before = graph();
    expect_error(action(), code,
                 "rejected node operation must report expected error");
    require(graph() == before && same_success(scene.roots(context), roots) &&
                same_success(scene.named(context, "Root_1"), named) &&
                same_success(scene.named(context, "RetainedChild"),
                             result<p::node>{child_a}) &&
                same_success(
                    detail::read(scene, node, bindings::Transform::translation),
                    result<vec3f>{vec3f{2, 3, 4}}),
            "ordinary rejected node operations must preserve graph values "
            "roots and names");
  };
  nodes_read(node, {});
  require(
      bool(detail::set_nodes(scene, node, "children", {child_a, std::nullopt, child_b})),
      "dynamic owner-bearing children write must succeed");
  nodes_read(node, {child_a, std::nullopt, child_b});
  require(bool(detail::set_nodes(scene,
              node, bindings::Transform::children,
              {std::nullopt, child_b, std::nullopt, child_a, std::nullopt})),
          "generated children key must preserve multiple NULL slots");
  nodes_read(node,
             {std::nullopt, child_b, std::nullopt, child_a, std::nullopt});
  require(bool(detail::set_nodes(scene, other_parent, "children", {child_a})),
          "one child may appear under distinct parents");
  nodes_read(other_parent, {child_a});
  reject_nodes(
      [&] { return detail::set_nodes(scene, node, "children", {child_a, child_a}); },
      error_code::invalid_value);
  reject_nodes(
      [&] {
        return detail::set_nodes(scene, node, bindings::Transform::children,
                               {child_a, std::nullopt, child_a});
      },
      error_code::invalid_value);
  reject_nodes(
      [&] { return detail::set_nodes(scene, node, "children", {child_a, *foreign}); },
      error_code::invalid_context);
  reject_nodes(
      [&] {
        return detail::set_nodes(scene, node, bindings::Transform::children,
                               {child_a, p::node{}});
      },
      error_code::stale_handle);
  auto disposed_child = child_b;
  disposed_child.dispose();
  reject_nodes(
      [&] {
        return detail::set_nodes(scene, node, "children", {child_a, disposed_child});
      },
      error_code::disposed);
  auto expired_made = factory();
  require(expired_made && bool(*expired_made),
          "temporary provider creation must succeed");
  auto expired_child = detail::separate_node(**expired_made);
  require(bool(expired_child), "temporary node creation must succeed");
  (*expired_made).reset();
  expect_error(scene.author(sf_owner,bindings::Shape::geometry,p::node_value{*expired_child}),
               error_code::stale_handle,"engaged expired SFNode does not become NULL");
  reject_nodes(
      [&] {
        return detail::set_nodes(scene, node, "children", {child_a, *expired_child});
      },
      error_code::stale_handle);
  for (const auto &bad_parent : {p::node{}, *expired_child}) {
    reject_nodes([&] { return detail::set_nodes(scene, bad_parent, "children", {}); },
                 error_code::stale_handle);
    expect_error(detail::nodes(scene, bad_parent, bindings::Transform::children),
                 error_code::stale_handle,
                 "invalid node wrapper cannot read children");
  }
  reject_nodes([&] { return detail::set_nodes(scene, disposed, "children", {}); },
               error_code::disposed);
  expect_error(detail::nodes(scene,disposed,bindings::Transform::children),error_code::disposed,
               "disposed owner cannot read a node-valued field");
  reject_nodes([&] { return detail::set_nodes(scene,node,"children",{other_context_node}); },
               error_code::invalid_context);
  reject_nodes([&] { return detail::set_nodes(scene,node,"children",{sf_child}); },
               error_code::type_mismatch);
  reject_nodes(
      [&] {
        return detail::set_nodes(scene, *foreign, bindings::Transform::children, {});
      },
      error_code::invalid_context);
  expect_error(detail::nodes(scene, *foreign, "children"),
               error_code::invalid_context,
               "foreign owner cannot read local node lists");
  reject_nodes([&] { return detail::set_nodes(scene, node, "translation", {}); },
               error_code::type_mismatch);
  reject_nodes([&] { return detail::set_nodes(scene, node, "metadata", {}); },
               error_code::type_mismatch);
  reject_nodes([&] { return detail::set_nodes(scene, node, "noSuchField", {}); },
               error_code::unknown_field);
  for (const auto name : {"addChildren", "removeChildren"}) {
    reject_nodes([&] { return detail::set_nodes(scene, node, name, {child_a}); },
                 error_code::unsupported_field_type);
    expect_error(detail::nodes(scene, node, name), error_code::access_denied,
                 "children event fields are not readable");
  }
  reject_nodes(
      [&] { return detail::set_nodes(scene, node, bindings::Group::children, {}); },
      error_code::type_mismatch);
  expect_error(detail::nodes(scene, node, bindings::Group::children),
               error_code::type_mismatch,
               "node-list keys must match the exact owner");
  reject_nodes(
      [&] {
        return detail::set_nodes(scene,
            node, detail::provider_fixture_keys::nodes_wrong_kind, {});
      },
      error_code::type_mismatch);
  expect_error(
      detail::nodes(scene, node, detail::provider_fixture_keys::nodes_wrong_kind),
      error_code::type_mismatch,
      "node-list key kind must match the descriptor");
  reject_nodes(
      [&] {
        return detail::set_nodes(scene,
            node, detail::provider_fixture_keys::nodes_wrong_access, {});
      },
      error_code::invalid_descriptor);
  expect_error(
      detail::nodes(scene, node, detail::provider_fixture_keys::nodes_wrong_access),
      error_code::invalid_descriptor,
      "node-list key access must match the descriptor");
  reject_nodes(
      [&] {
        return detail::set_nodes(scene,
            node, detail::provider_fixture_keys::nodes_unknown, {});
      },
      error_code::unknown_field);
  expect_error(
      detail::nodes(scene, node, detail::provider_fixture_keys::nodes_unknown),
      error_code::unknown_field, "unknown node-list key must fail");
  reject_nodes([&] { return detail::set_nodes(scene, node, "children", {node}); },
               error_code::containment_cycle);
  reject_nodes([&] { return detail::set_nodes(scene, child_a, "children", {node}); },
               error_code::containment_cycle);
  // Neither detached component is reachable from any scene root.
  const auto detached_a = make_node();
  const auto detached_b = make_node();
  require(bool(detail::set_nodes(scene, detached_a, "children", {detached_b})),
          "detached containment must be accepted");
  reject_nodes(
      [&] { return detail::set_nodes(scene, detached_b, "children", {detached_a}); },
      error_code::containment_cycle);
  reject_nodes(
      [&] { return detail::set_nodes(scene, detached_a, "children", {detached_a}); },
      error_code::containment_cycle);
  require(bool(detail::set_nodes(scene, node, bindings::Transform::children, {})),
          "empty node list clears children");
  nodes_read(node, {});
  require(same_success(scene.named(context, "RetainedChild"),
                       result<p::node>{child_a}) &&
              same_success(scene.type_name(child_a),
                           result<std::string>{"Transform"}),
          "clearing children retains detached node identity and DEF name");
  // A shared descendant is legal. DFS must distinguish DAG joins from cycles.
  require(bool(detail::set_nodes(scene, other_parent, "children", {})) &&
              bool(detail::set_nodes(scene, node, "children", {child_a, child_b})) &&
              bool(detail::set_nodes(scene, child_a, "children", {other_parent})) &&
              bool(detail::set_nodes(scene, child_b, "children", {other_parent})),
          "diamond DAG joins must be accepted");
  nodes_read(node, {child_a, child_b});
  nodes_read(child_a, {other_parent});
  nodes_read(child_b, {other_parent});
  // Exercise a deeper detached DAG and a late back edge without recursion in
  // native preflight. Kernel snapshot/edit cost intentionally bounds this test.
  std::vector<p::node> chain;
  for (std::size_t i = 0; i < 128; ++i)
    chain.push_back(make_node());
  for (std::size_t i = chain.size() - 1; i > 0; --i)
    require(bool(detail::set_nodes(scene, chain[i - 1], "children", {chain[i]})),
            "deep detached DAG must be accepted");
  reject_nodes(
      [&] {
        return detail::set_nodes(scene, chain.back(), "children", {chain.front()});
      },
      error_code::containment_cycle);
  nodes_read(chain.front(), {chain[1]});
  const auto retained_nodes = detail::nodes(scene, node, "children");
  require(retained_nodes && retained_nodes->size() == 2,
          "owning node-list read must succeed before provider close");

  result<vec3f> threaded_typed_read;
  result<void> threaded_typed_write;
  result<void> threaded_write;
  result<p::node_values> threaded_nodes_read;
  result<void> threaded_nodes_write;
  result<void> threaded_close;
  p::capabilities threaded_caps;
  std::thread other([&] {
    threaded_write = detail::author(scene, node, "translation", vec3f{99, 99, 99});
    threaded_typed_read =
        detail::read(scene, node, bindings::Transform::translation);
    threaded_typed_write = detail::author(scene,
        node, bindings::Transform::translation, vec3f{99, 99, 99});
    threaded_nodes_read = detail::nodes(scene, node, bindings::Transform::children);
    threaded_nodes_write = detail::set_nodes(scene, node, "children", {});
    threaded_close = scene.close();
    threaded_caps = scene.supported();
  });
  other.join();
  expect_error(threaded_write, error_code::wrong_thread,
               "non-owner thread write must fail");
  expect_error(threaded_typed_read, error_code::wrong_thread,
               "non-owner thread typed read must fail");
  expect_error(threaded_typed_write, error_code::wrong_thread,
               "non-owner thread typed write must fail");
  expect_error(threaded_close, error_code::wrong_thread,
               "non-owner thread close must fail");
  expect_error(threaded_nodes_read, error_code::wrong_thread,
               "wrong thread cannot read owner-bearing nodes");
  expect_error(threaded_nodes_write, error_code::wrong_thread,
               "wrong thread cannot mutate containment");
  nodes_read(node, {child_a, child_b});
  require(detail::same_capabilities(threaded_caps, caps),
          "constant capability facts are thread-independent");
  read("translation", {2, 3, 4});
  require(bool(scene.close()) && bool(scene.close()),
          "close must be idempotent");
  require(detail::same_capabilities(scene.supported(), caps),
          "constant capability facts remain queryable after close");
  require(node.expired() && !node.disposed(),
          "close must invalidate all retained wrappers");
  expect_error(detail::read(scene, node, "translation"), error_code::stale_handle,
               "closed provider must reject reads");
  expect_error(detail::read(scene, node, bindings::Transform::translation),
               error_code::stale_handle,
               "closed provider must reject typed reads");
  expect_error(
      detail::author(scene, node, bindings::Transform::translation, vec3f{}),
      error_code::stale_handle, "closed provider must reject typed writes");
  expect_error(detail::nodes(scene, node, bindings::Transform::children),
               error_code::stale_handle,
               "closed provider cannot read children");
  expect_error(detail::set_nodes(scene, node, "children", {}), error_code::stale_handle,
               "closed provider cannot author children");
  require((*retained_nodes)[0] && (*retained_nodes)[0]->expired() &&
              *(*retained_nodes)[0] == child_a,
          "owning node lists preserve identity without preserving provider "
          "lifetime");
  expect_error(scene.create_node(context, "Transform"), error_code::stale_handle,
               "closed provider must reject creation");
  expect_error(scene.begin_update(p::client{}), error_code::stale_handle,
               "closed state precedes invalid client service");
  (*foreign_made).reset();
  require(foreign->expired(),
          "provider destruction must invalidate retained wrappers");
  require(*fields == expected,
          "returned descriptors must own their data after close");
  // Owner-thread close relinquishes authority before a different thread may
  // destroy the closed service. An open backend is never destroyed off-thread.
  auto closed_elsewhere = factory().value();
  auto closed_node = detail::separate_node(*closed_elsewhere).value();
  require(bool(closed_elsewhere->close()), "owner closes before cross-thread destruction");
  std::thread destroy_closed([owned=std::move(closed_elsewhere)]() mutable { owned.reset(); });
  destroy_closed.join();
  require(closed_node.expired(), "closed service can be destroyed off-thread without authority");
  result<std::unique_ptr<p::service>> thread_owned;
  result<void> creator_closed;
  std::thread creator([&] {
    thread_owned=factory();
    if(thread_owned) creator_closed=(*thread_owned)->close();
  });
  creator.join();
  require(thread_owned && bool(*thread_owned) && creator_closed,
          "creator thread can close its provider before its lifetime ends");
  result<p::scene> reused_thread_create;
  result<void> reused_thread_close;
  std::thread later([&] {
    reused_thread_create=(*thread_owned)->create_scene();
    reused_thread_close=(*thread_owned)->close();
  });
  later.join();
  expect_error(reused_thread_create,error_code::wrong_thread,
               "a later thread lifetime cannot inherit creator authority");
  expect_error(reused_thread_close,error_code::wrong_thread,
               "a recycled thread ID cannot authorize even checked close");
  (*thread_owned).reset();
  return report;
}

// Ordinary authoring, shared by both real backends. No bootstrap backend hook,
// native pointers, parser, kernel object or representation-specific API is used.
inline result<p::scene> make_provider_route_scene(p::service &service) {
  auto scene=service.create_scene(); if(!scene) return failure(scene.error());
  const auto build=[&]() -> result<void> {
    auto context=service.root_context(*scene); if(!context) return failure(context.error());
    auto interpolator=service.create_node(*context,"PositionInterpolator");
    auto transform=service.create_node(*context,"Transform");
    auto mirror=service.create_node(*context,"Transform");
    auto shape=service.create_node(*context,"Shape");
    auto box=service.create_node(*context,"Box");
    if(!interpolator) return failure(interpolator.error());
    if(!transform) return failure(transform.error());
    if(!mirror) return failure(mirror.error());
    if(!shape) return failure(shape.error());
    if(!box) return failure(box.error());
    for(const auto &[node,name]:{std::pair{*interpolator,"Interpolator"},
                                std::pair{*transform,"Transform"},std::pair{*mirror,"Mirror"}}) {
      if(auto value=service.define_name(node,name);!value) return value;
      if(auto value=service.append_root(*context,node);!value) return value;
    }
    if(auto value=service.define_name(*shape,"Shape");!value) return value;
    if(auto value=service.define_name(*box,"Box");!value) return value;
    if(auto value=detail::author(service,*shape,"geometry",p::node_value{*box});!value) return value;
    if(auto value=service.author(*transform,bindings::Transform::children,p::node_values{*shape});!value) return value;
    if(auto value=service.author(*interpolator,bindings::PositionInterpolator::key,float_list{0,1});!value) return value;
    if(auto value=service.author(*interpolator,bindings::PositionInterpolator::keyValue,vec3f_list{{0,0,0},{10,20,30}});!value) return value;
    return service.add_route(*context,service.get_field(*interpolator,"value_changed").value(),
                             service.get_field(*transform,"translation").value());
  };
  if(auto built=build();!built) { (void)service.retire(*scene); return failure(built.error()); }
  return *scene;
}
// X3D 4.0 Part 1, 19.3.1: a pre-input interpolator getter uses the
// first authored keyValue, or its field type's default when that list is empty.
// This initializes stored output without inventing a fraction event or ROUTE.
inline void run_provider_initial_output_fixture(const provider_factory &factory,
                                                provider_fixture_report &report) {
  const auto require=[&](bool okay,const char *why) {
    ++report.checks; if(!okay) throw std::runtime_error(why);
  };
  auto service=factory().value();
  auto scene=service->create_scene().value();
  auto context=service->root_context(scene).value();
  auto interpolator=service->create_node(context,"PositionInterpolator").value();
  auto transform=service->create_node(context,"Transform").value();
  auto output=service->get_field(interpolator,"value_changed").value();
  auto target=service->get_field(transform,"translation").value();
  require(service->roots(context)->empty(),"initial-output probe uses detached unnamed nodes");
  require(std::get<vec3f>(service->read(output).value())==vec3f{},
          "empty initial keyValue yields the SFVec3f default");
  require(bool(service->author(interpolator,bindings::PositionInterpolator::key,float_list{0,1})) &&
          bool(service->author(interpolator,bindings::PositionInterpolator::keyValue,
                               vec3f_list{{2,4,6},{10,20,30}})),
          "ordinary authoring establishes a nonzero initial interpolation value");
  require(std::get<vec3f>(service->read(output).value())==vec3f{2,4,6} &&
          service->read(interpolator,bindings::PositionInterpolator::value_changed).value()==vec3f{2,4,6},
          "dynamic and typed pre-activation getters read the first authored keyValue");
  require(bool(service->author(interpolator,bindings::PositionInterpolator::keyValue,vec3f_list{})),
          "temporary setup may clear keyValue before repairing the configuration");
  require(std::get<vec3f>(service->read(output).value())==vec3f{},
          "clearing setup keyValue restores the initial field-type default");
  require(bool(service->author(interpolator,bindings::PositionInterpolator::keyValue,
                               vec3f_list{{2,4,6},{10,20,30}})),
          "setup keyValue can be restored before activation");
  auto denied=service->author(output,vec3f{99,99,99});
  require(!denied && denied.error().code==error_code::access_denied,
          "derived initialization never grants application outputOnly write access");
  require(bool(service->add_route(context,output,target)),
          "detached unnamed endpoints can be connected through canonical fields");
  auto active=service->activate(scene).value();
  require(service->roots(context)->empty() && std::get<vec3f>(service->read(output).value())==vec3f{2,4,6},
          "activation retains the already initialized output without rooting nodes");
  require(std::get<vec3f>(service->read(target).value())==vec3f{},
          "initial getter storage is not a synthetic routed event");
  std::vector<p::notification> notices;
  auto watch=service->observe(active,output,[&](const auto &notice){notices.push_back(notice);}).value();
  require(service->dispatch_notifications().value()==0 && notices.empty(),
          "activation produces no initial output notification");
  auto empty=service->pump(active,{-100}).value();
  require(empty.turn==1 && !empty.request && empty.time.seconds==-100 &&
          std::get<vec3f>(service->read(output).value())==vec3f{2,4,6} &&
          std::get<vec3f>(service->read(target).value())==vec3f{},
          "an empty first negative-time host turn neither invents input nor overwrites initial output");
  require(service->dispatch_notifications().value()==0 && notices.empty(),
          "the empty host turn emits no synthetic interpolation event");
  auto client=service->connect(active).value();
  auto accepted=service->enqueue(client,interpolator,bindings::PositionInterpolator::set_fraction,.5f).value();
  auto turn=service->pump(active,{-100}).value();
  require(turn.turn==2 && turn.request==accepted.id() && accepted.status()->state==p::request_state::processed,
          "real fraction input processes at the same explicit host time");
  require(std::get<vec3f>(service->read(output).value())==vec3f{6,12,18} &&
          std::get<vec3f>(service->read(target).value())==vec3f{6,12,18},
          "detached interpolation and routing update real retained field values");
  require(notices.empty(),"initial-value probe callbacks remain explicitly deferred");
  require(service->dispatch_notifications().value()==1 && notices.size()==1 &&
          notices.front().turn==2 && notices.front().time.seconds==-100 &&
          notices.front().value==p::payload{vec3f{6,12,18}},
          "only the real fraction turn publishes its owning output notification");
}
// These extra integration cases exercise the approved gate semantics on both
// real backends. Backend-specific event evidence verifies occurrence delivery;
// portable observations deliberately retain architectural output loop breakers.
inline void run_provider_gate_fixture(const provider_factory &factory,
                                      provider_fixture_report &report) {
  const auto require=[&](bool okay,const char *why) {
    ++report.checks; if(!okay) throw std::runtime_error(why);
  };
  auto service=factory().value();
  auto scene=make_provider_route_scene(*service).value();
  auto context=service->root_context(scene).value();
  auto transform=service->named(context,"Transform").value();
  auto interpolator=service->named(context,"Interpolator").value();
  auto translation=service->get_field(transform,"translation").value();
  auto alias=service->get_field(transform,"set_translation").value();
  auto fraction=service->get_field(interpolator,"set_fraction").value();
  auto key=service->get_field(interpolator,"key").value();
  auto values=service->get_field(interpolator,"keyValue").value();
  auto active=service->activate(scene).value();
  auto held=service->connect(active).value(),other=service->connect(active).value();
  require(bool(service->end_update(held)),"unmatched EndUpdate is a no-op");
  require(bool(service->begin_update(held)) && bool(service->begin_update(held)),
          "repeated BeginUpdate retains one open gate");
  auto first=service->enqueue(held,{{translation,vec3f{1,2,3}}}).value();
  auto second=service->enqueue(held,{{alias,vec3f{4,5,6}},{translation,vec3f{7,8,9}}}).value();
  auto free=service->enqueue(other,{{translation,vec3f{10,11,12}}}).value();
  require(first.status()->state==p::request_state::accepted &&
          second.status()->state==p::request_state::accepted,
          "all held calls stay accepted until one EndUpdate");
  require(bool(service->end_update(held)),"one End releases complete held submission");
  require(first.status()->submission!=0 && first.status()->submission==second.status()->submission,
          "held call receipts identify one released submission");
  auto free_turn=service->pump(active,{4}).value();
  require(free_turn.request==free.id() &&
          service->read(transform,bindings::Transform::translation).value()==vec3f{10,11,12},
          "another client's earlier released work retains FIFO position");
  auto held_turn=service->pump(active,{4}).value();
  require(held_turn.request==first.id() && first.status()->turn==second.status()->turn &&
          first.status()->state==p::request_state::processed &&
          second.status()->state==p::request_state::processed,
          "one logical turn processes every receipt in the held submission");
  require(service->read(transform,bindings::Transform::translation).value()==vec3f{7,8,9},
          "canonical stateful writes coalesce to last value across calls and aliases");
  require(!service->pump(active,{4})->request,
          "grouped calls do not leave extra submissions to replay");
  require(bool(service->end_update(held)),"End after release is also a no-op");
  require(bool(service->begin_update(held)),"begin indexed configuration group");
  auto keys=service->enqueue(held,{{key,float_list{0,.5f,1}}}).value();
  auto whole=service->enqueue(held,{{values,vec3f_list{{0,0,0},{5,10,15},{10,20,30}}}}).value();
  auto edit=service->enqueue(held,{{values,vec3f{6,12,18},p::write_mode::set_element,1}}).value();
  require(bool(service->end_update(held)),"final paired MF candidate is validated at release");
  require(bool(service->pump(active,{4})),"indexed candidate processes on real backend");
  require(keys.status()->turn==whole.status()->turn && whole.status()->turn==edit.status()->turn,
          "whole and indexed MF edits share submission outcome");
  require(service->read(interpolator,bindings::PositionInterpolator::keyValue).value()==
          vec3f_list{{0,0,0},{6,12,18},{10,20,30}},
          "indexed MF edits aggregate in order into owning pending candidate");
  auto out_of_range=service->enqueue(other,{{values,vec3f{},p::write_mode::set_element,99}});
  require(!out_of_range,"out-of-range indexed edit is rejected before processing");
  require(bool(service->begin_update(held)),"begin repeated input occurrence group");
  auto input_a=service->enqueue(held,{{fraction,.25f},{fraction,.25f}}).value();
  auto input_b=service->enqueue(held,{{fraction,.75f}}).value();
  require(bool(service->end_update(held)),"equal inputOnly occurrences are admitted without coalescing rejection");
  auto input_turn=service->pump(active,{4}).value();
  require(!input_turn.error && input_turn.request==input_a.id() &&
          input_a.status()->state==p::request_state::processed &&
          input_b.status()->state==p::request_state::processed &&
          input_a.status()->turn==input_b.status()->turn,
          "whole input occurrences execute within one successful submission");
  require(!service->pump(active,{4})->request,"input occurrences are not split into extra turns");
}
// A released graph candidate is one retained state, even when its individual
// seed prefixes would contain a cycle. Ordinary routed deliveries still follow
// their sources and inputOnly occurrences keep their own ordered payloads.
inline void run_provider_graph_gate_fixture(const provider_factory &factory,
                                            provider_fixture_report &report) {
  const auto require=[&](bool okay,const char *why) {
    ++report.checks; if(!okay) throw std::runtime_error(why);
  };
  const auto reject=[&](const auto &result,error_code code,const char *why) {
    require(!result && result.error().code==code,why);
  };
  auto service=factory().value();
  auto scene=make_provider_route_scene(*service).value();
  auto context=service->root_context(scene).value();
  auto a=service->named(context,"Transform").value();
  auto b=service->create_node(context,"Transform").value();
  auto c=service->create_node(context,"Transform").value();
  auto shape=service->named(context,"Shape").value();
  auto box=service->named(context,"Box").value();
  auto interpolator=service->named(context,"Interpolator").value();
  auto a_children=service->get_field(a,"children").value();
  auto b_children=service->get_field(b,"children").value();
  auto c_children=service->get_field(c,"children").value();
  auto geometry=service->get_field(shape,"geometry").value();
  auto translation=service->get_field(a,"translation").value();
  auto fraction=service->get_field(interpolator,"set_fraction").value();
  auto values=service->get_field(interpolator,"keyValue").value();
  auto output=service->get_field(interpolator,"value_changed").value();
  require(bool(service->author(a_children,p::node_values{b})),
          "graph gate starts with A containing B");
  require(bool(service->add_route(context,b_children,c_children)),
          "graph gate routes B children to C children");
  auto active=service->activate(scene).value();
  auto held=service->connect(active).value(),other=service->connect(active).value();
  std::vector<p::notification> notices;
  std::vector<p::subscription> watches;
  for(const auto &field:{a_children,b_children,c_children,output,translation})
    watches.push_back(service->observe(active,field,
        [&](const auto &notice){notices.push_back(notice);}).value());
  const auto snapshot=[&]() {
    std::vector<p::payload> result;
    for(const auto &field:{a_children,b_children,c_children,geometry,translation,values,output})
      result.push_back(service->read(field).value());
    return result;
  };
  const auto before=snapshot();
  require(bool(service->begin_update(held)),"begin graph swap gate");
  auto first=service->enqueue(held,{{fraction,.25f},{b_children,p::node_values{a}}}).value();
  auto second=service->enqueue(held,{{fraction,.75f},{a_children,p::node_values{}},
      {values,vec3f_list{{0,0,0},{20,40,60}}}}).value();
  require(snapshot()==before,"accepted graph swap does not mutate retained storage");
  require(bool(service->end_update(held)),"acyclic final graph swap releases despite cyclic prefix");
  require(first.status()->submission!=0 && first.status()->submission==second.status()->submission,
          "graph swap receipts share one released submission");
  auto turn=service->pump(active,{8}).value();
  require(!turn.error && turn.state==p::activation_state::active && turn.request==first.id() &&
          first.status()->state==p::request_state::processed &&
          second.status()->state==p::request_state::processed &&
          first.status()->turn==second.status()->turn,
          "valid final graph swap processes in one healthy turn");
  require(service->read(a_children).value()==p::payload{p::node_values{}} &&
          service->read(b_children).value()==p::payload{p::node_values{a}} &&
          service->read(c_children).value()==p::payload{p::node_values{a}},
          "final graph contains B and C pointing to A without a cycle");
  require(service->read(output).value()==p::payload{vec3f{5,10,15}} &&
          service->read(translation).value()==p::payload{vec3f{5,10,15}},
          "ordered inputOnly occurrences use final retained curve and route the first output");
  require(notices.empty(),"graph and interpolation callbacks stay deferred during evaluation");
  require(service->dispatch_notifications().value()==5 && notices.size()==5,
          "graph swap publishes three graph notices and two interpolation notices");
  const auto notice_for=[&](const p::field &field) {
    return std::find_if(notices.begin(),notices.end(),[&](const auto &n){return n.source==field;});
  };
  auto a_notice=notice_for(a_children),b_notice=notice_for(b_children),c_notice=notice_for(c_children);
  auto output_notice=notice_for(output),translation_notice=notice_for(translation);
  require(a_notice!=notices.end() && b_notice!=notices.end() && c_notice!=notices.end() &&
          output_notice!=notices.end() && translation_notice!=notices.end(),
          "every expected source and routed destination is observed");
  require(b_notice<a_notice && b_notice<c_notice && output_notice<translation_notice,
          "retained seeds preserve order and each source precedes its routed destination");
  require(a_notice->value==p::payload{p::node_values{}} &&
          b_notice->value==p::payload{p::node_values{a}} &&
          c_notice->value==p::payload{p::node_values{a}} &&
          output_notice->value==p::payload{vec3f{5,10,15}} &&
          translation_notice->value==p::payload{vec3f{5,10,15}},
          "deferred graph and ordered runtime notifications own their admitted payloads");
  require(std::all_of(notices.begin(),notices.end(),[&](const auto &n) {
            return n.turn==turn.turn && n.time.seconds==8 &&
                   n.activation_epoch==notices.front().activation_epoch;
          }),"graph swap notifications retain one host turn and activation");
  notices.clear();
  const std::vector<std::pair<p::write,error_code>> invalids{
      {{a_children,p::node_values{a}},error_code::containment_cycle},
      {{a_children,p::node_values{b}},error_code::containment_cycle},
      {{b_children,p::node_values{a,a}},error_code::invalid_value},
      {{b_children,p::node_values{box}},error_code::type_mismatch},
      {{geometry,p::node_value{a}},error_code::type_mismatch}};
  for(const auto &[invalid,code]:invalids) {
    const auto unchanged=snapshot();
    auto earlier=service->enqueue(other,{{fraction,.5f}}).value();
    reject(service->enqueue(held,{invalid}),code,"invalid ungated graph is rejected before admission");
    require(bool(service->begin_update(held)),"hold an invalid final graph candidate");
    auto bad=service->enqueue(held,{invalid}).value();
    auto companion=service->enqueue(held,{{translation,vec3f{99,99,99}}}).value();
    reject(service->end_update(held),code,"invalid final graph rejects at gate release");
    const auto bad_status=bad.status().value(),companion_status=companion.status().value();
    require(bad_status.state==p::request_state::failed && companion_status.state==p::request_state::failed &&
            bad_status.effects==p::failure_effect::none && companion_status.effects==p::failure_effect::none &&
            bad_status.turn==0 && companion_status.turn==0 && bad_status.submission!=0 &&
            bad_status.submission==companion_status.submission &&
            bad_status.error && bad_status.error->code==code &&
            companion_status.error && companion_status.error->code==code,
            "release rejection gives all held receipts one no-effect failure before any turn");
    require(snapshot()==unchanged && service->dispatch_notifications().value()==0 && notices.empty(),
            "rejected graph candidate mutates no fields and emits no notifications");
    require(service->state(scene).value()==p::activation_state::active &&
            earlier.status()->state==p::request_state::submitted,
            "release rejection preserves healthy activation and earlier admitted work");
    auto prior_turn=service->pump(active,{8}).value();
    require(!prior_turn.error && prior_turn.request==earlier.id() &&
            earlier.status()->state==p::request_state::processed &&
            service->read(translation).value()==p::payload{vec3f{10,20,30}},
            "earlier admitted input still processes after invalid graph release");
    require(service->dispatch_notifications().value()==2 && notices.size()==2 &&
            std::all_of(notices.begin(),notices.end(),[](const auto &n) {
              return n.value==p::payload{vec3f{10,20,30}};
            }),"only earlier admitted interpolation publishes after rejected graph release");
    notices.clear();
  }
  auto repair=service->enqueue(held,{{b_children,p::node_values{}}}).value();
  auto repaired=service->pump(active,{8}).value();
  require(!repaired.error && repair.status()->state==p::request_state::processed &&
          service->read(b_children).value()==p::payload{p::node_values{}} &&
          service->read(c_children).value()==p::payload{p::node_values{}},
          "later valid node writes and graph ROUTEs remain operational after rejection");
  require(service->dispatch_notifications().value()==2,
          "later valid graph write publishes its source and routed destination");
}
// This entire frontend program is compiled unmodified against each independent
// backend. Construction of the bounded fixture is backend-owned. Values,
// identities, access, admission, scheduling and observation are live calls.
// No kernel revisions, native pointers, render IDs or fan-in winner are compared.
inline provider_fixture_report run_provider_live_fixture(
    const provider_factory &factory) {
  namespace h = provider;
  provider_fixture_report report;
  auto require = [&](bool value, const char *why) {
    ++report.checks; if (!value) throw std::runtime_error(why);
  };
  auto reject = [&](const auto &value, error_code code, const char *why) {
    require(!value && value.error().code == code, why);
  };
  auto made = factory(); require(made && bool(*made), "create first provider service");
  auto host = std::move(*made);
  auto other_made = factory(); require(other_made && bool(*other_made), "create foreign provider service");
  auto foreign = std::move(*other_made);
  const auto caps = host->supported();
  require(caps.supports(h::service_id::scene_authoring) && caps.supports(h::service_id::host_turns) &&
          caps.supports(h::service_id::notifications), "versioned live capabilities are truthful");
  require(!caps.supports(h::service_id::reference_snapshots) &&
          !caps.supports(h::service_id::reference_transactions),
          "portable provider does not claim reference-only transaction or snapshot extensions");
  auto first = make_provider_route_scene(*host).value();
  auto second = make_provider_route_scene(*host).value();
  auto outside = make_provider_route_scene(*foreign).value();
  require(!(first == second), "separate scene identity");
  auto ctx = host->root_context(first).value();
  auto ctx2 = host->root_context(second).value();
  reject(host->root_context(h::scene{}),error_code::stale_handle,"empty scene has no authority");
  reject(host->roots(h::context{}),error_code::stale_handle,"empty context has no authority");
  auto disposed_scene=first; disposed_scene.dispose();
  reject(host->root_context(disposed_scene),error_code::disposed,"scene disposal is local and explicit");
  auto disposed_context=ctx; disposed_context.dispose();
  reject(host->roots(disposed_context),error_code::disposed,"context disposal is local and explicit");
  require(ctx==host->root_context(first).value(),"original scene/context survive copied-wrapper disposal");
  require(ctx == host->root_context(first).value(), "lexical context identity reacquires");
  auto transform = host->named(ctx, "Transform").value();
  auto interpolator = host->named(ctx, "Interpolator").value();
  auto mirror = host->named(ctx, "Mirror").value();
  auto other_transform = host->named(ctx2, "Transform").value();
  require(!(transform == other_transform), "identical local names never alias across scenes");
  auto roots = host->roots(ctx).value();
  require(roots.size() == 3 && std::find(roots.begin(), roots.end(), transform) != roots.end() &&
          std::find(roots.begin(), roots.end(), interpolator) != roots.end() &&
          std::find(roots.begin(), roots.end(), mirror) != roots.end(), "roots preserve named identities");
  require(host->type_name(transform).value() == "Transform" && host->type_name(interpolator).value() == "PositionInterpolator",
          "real node type discovery");
  auto fields = host->fields(interpolator).value();
  for (const auto &[name, kind, access] : std::vector<std::tuple<std::string, value_kind, access_type>>{
      {"key", value_kind::mf_float, access_type::input_output},
      {"keyValue", value_kind::mf_vec3f, access_type::input_output},
      {"set_fraction", value_kind::sf_float, access_type::input_only},
      {"value_changed", value_kind::sf_vec3f, access_type::output_only}}) {
    auto found = std::find_if(fields.begin(), fields.end(), [&](const auto &f) { return f.name == name; });
    require(found != fields.end() && found->kind == kind && found->access == access && found->supported,
            "four-kind discovery matches the contract");
  }
  auto translation = host->get_field(transform, "translation").value();
  auto fraction = host->get_field(interpolator, "set_fraction").value();
  auto output = host->get_field(interpolator, "value_changed").value();
  auto key = host->get_field(interpolator, "key").value();
  auto key_value = host->get_field(interpolator, "keyValue").value();
  auto mirror_translation = host->get_field(mirror, "translation").value();
  auto bbox = host->get_field(transform, "bboxSize").value();
  require(translation == host->get_field(transform, "set_translation").value() &&
          translation == host->get_field(transform, "translation_changed").value(), "IO aliases are canonical identity");
  require(key == host->get_field(interpolator, "set_key").value(), "MF alias canonicalizes");
  require(translation == host->get_field(transform, bindings::Transform::translation).value(), "existing generated key resolves canonical field");
  require(fraction == host->get_field(interpolator, bindings::PositionInterpolator::set_fraction).value(), "generated input key shares dynamic identity");
  reject(host->get_field(transform, bindings::Group::children), error_code::type_mismatch, "generated key owner authority checked");
  reject(host->get_field(transform, detail::provider_fixture_keys::wrong_kind), error_code::type_mismatch, "key kind checked against backend descriptor");
  reject(host->get_field(transform, detail::provider_fixture_keys::wrong_access), error_code::invalid_descriptor, "key access checked against backend descriptor");
  reject(host->get_field(interpolator, "fraction"), error_code::unknown_field, "inputOnly names are not invented aliases");
  reject(host->read(fraction), error_code::access_denied, "inputOnly read denied");
  require(std::get<rotation>(host->read(host->get_field(transform, "rotation").value()).value())==rotation{}, "authoring and live discovery use the same full supported vocabulary");
  reject(host->named(ctx, "Missing"), error_code::unknown_node, "unknown DEF rejected");
  auto foreign_node = foreign->named(foreign->root_context(outside).value(), "Transform").value();
  reject(host->type_name(foreign_node), error_code::invalid_context, "foreign authority rejected before backend access");
  auto disposed = transform; disposed.dispose();
  reject(host->fields(disposed), error_code::disposed, "node wrapper disposal is local");
  require(host->type_name(transform).value() == "Transform", "copy survives wrapper disposal");
  auto disposed_field = translation; disposed_field.dispose();
  reject(host->read(disposed_field), error_code::disposed, "field wrapper disposal is local");
  require(host->read(translation).has_value(), "canonical field copy survives disposal");
  require(host->author(key, std::vector<float>{0, 1}).has_value(), "author MFFloat");
  require(host->author(key_value, std::vector<vec3f>{{0, 0, 0}, {10, 20, 30}}).has_value(), "author MFVec3f");
  require(host->author(transform, bindings::Transform::translation, vec3f{0, 0, 0}).has_value(), "typed author uses existing SFVec3f vocabulary");
  require(host->read(transform, bindings::Transform::translation).value() == vec3f{}, "typed and dynamic reads agree");
  require(host->author(host->get_field(transform,"rotation").value(),rotation{}).has_value(),"rotation setup uses the same provider field");
  reject(host->author(translation, h::payload{}), error_code::type_mismatch, "empty vocabulary value is not a valid field payload");
  require(host->author(bbox, vec3f{2, 2, 2}).has_value(), "initializeOnly authoring before activation");
  require(std::get<std::vector<float>>(host->read(key).value()) == std::vector<float>{0, 1}, "MFFloat owning read");
  require(std::get<std::vector<vec3f>>(host->read(key_value).value()) == std::vector<vec3f>{{0, 0, 0}, {10, 20, 30}}, "MFVec3f owning read");
  reject(host->author(translation, 1.f), error_code::type_mismatch, "author exact kind checked");
  reject(host->author(output, vec3f{}), error_code::access_denied, "outputOnly authoring denied");
  reject(host->add_route(ctx, fraction, key), error_code::invalid_route, "route rejects inputOnly source");
  reject(host->add_route(ctx, output, key), error_code::invalid_route, "route rejects differing kinds");
  reject(host->add_route(ctx, output, bbox), error_code::invalid_route, "route rejects initializeOnly destination");
  reject(host->add_route(ctx, output, host->get_field(other_transform, "translation").value()), error_code::invalid_context, "route rejects cross-scene destination");
  reject(host->add_route(ctx, output, translation), error_code::invalid_route, "duplicate authored route rejected");
  require(host->add_route(ctx, output, mirror_translation).has_value(), "checked compatible route created");
  reject(host->add_route(ctx, output, mirror_translation), error_code::invalid_route, "new duplicate route rejected");
  auto world = host->activate(first).value();
  auto world2 = host->activate(second).value();
  require(host->state(first).value() == h::activation_state::active, "activation state explicit");
  reject(host->activate(first), error_code::access_denied, "activation not silently repeated");
  reject(host->author(key, std::vector<float>{0, 1}), error_code::access_denied, "authoring cannot mutate live state");
  reject(host->add_route(ctx, output, mirror_translation), error_code::access_denied, "live topology editing explicitly excluded");
  auto client_a = host->connect(world).value(), client_b = host->connect(world).value();
  auto client2 = host->connect(world2).value();
  reject(host->enqueue(client_a, {{output, vec3f{}}}), error_code::access_denied, "outputOnly live write denied");
  reject(host->enqueue(client_a, {{bbox, vec3f{}}}), error_code::access_denied, "initializeOnly live write denied");
  reject(host->enqueue(client_a, {{fraction, vec3f{}}}), error_code::type_mismatch, "live payload kind exact");
  reject(host->enqueue(client_a, {{fraction, std::numeric_limits<float>::quiet_NaN()}}), error_code::invalid_value, "nonfinite input rejected before queuing");
  reject(host->enqueue(client_a, {{host->get_field(other_transform, "translation").value(), vec3f{}}}), error_code::invalid_context, "live cross-scene write denied");

  reject(host->enqueue(client_a, {}), error_code::invalid_value, "empty submitted write batch rejected");
  std::vector<h::notification> notices;
  auto on_event = [&](const h::notification &event) { notices.push_back(event); };
  auto watch_output = host->observe(world, output, on_event).value();
  auto watch_translation = host->observe(world, translation, on_event).value();
  auto watch_mirror = host->observe(world, mirror_translation, on_event).value();
  reject(host->observe(world, fraction, on_event), error_code::access_denied, "inputOnly cannot be observed");
  reject(host->observe(world, host->get_field(other_transform, "translation").value(), on_event), error_code::invalid_context, "foreign scene observer rejected");
  auto vector_read = [&](const h::field &f) { return std::get<vec3f>(host->read(f).value()); };
  auto observe_turn = [&](std::uint64_t turn, vec3f expected) {
    require(vector_read(translation) == expected && vector_read(mirror_translation) == expected, "real interpolation fans out through both routes");
    require(vector_read(output) == expected, "outputOnly readback is retained by the actual backend field");
    report.observations.push_back(vector_read(translation));
    auto drained = host->dispatch_notifications(); require(drained.has_value(), "explicit notification drain succeeds");
    auto source = std::find_if(notices.begin(), notices.end(), [&](const auto &n) { return n.turn == turn && n.source == output; });
    auto target = std::find_if(notices.begin(), notices.end(), [&](const auto &n) { return n.turn == turn && n.source == translation; });
    auto mirrored = std::find_if(notices.begin(), notices.end(), [&](const auto &n) { return n.turn == turn && n.source == mirror_translation; });
    require(source != notices.end() && target != notices.end() && mirrored != notices.end(), "runtime output and routed notifications captured");
    require(source < target && source < mirrored, "source precedes its routed destinations");
    require(source->value == h::payload{expected} && target->value == h::payload{expected} && mirrored->value == h::payload{expected}, "deferred payloads own routed values");
    require(source->time.seconds == 10 && target->time.seconds == 10 && source->activation_epoch == target->activation_epoch, "one host turn preserves time and activation causality");
    // Fan-out siblings have no portable total order; record a normalized set.
    std::vector<std::pair<std::uint64_t, std::string>> canonical;
    for (const auto &n : notices) if (n.turn == turn) canonical.emplace_back(n.turn, n.source == output ? "output" : n.source == translation ? "translation" : "mirror");
    std::sort(canonical.begin(), canonical.end()); report.notifications.insert(report.notifications.end(), canonical.begin(), canonical.end());
    notices.clear();
  };
  require(host->begin_update(client_a).has_value(), "hold client A gate");
  require(host->begin_update(client_a).has_value(), "repeated BeginUpdate is a no-op");
  auto held = host->enqueue(client_a, {{fraction, .25f}}).value();
  require(held.status()->state == h::request_state::accepted && held.status()->turn == 0, "accepted held request is not submitted");
  auto free = host->enqueue(client_b, interpolator, bindings::PositionInterpolator::set_fraction, .5f).value();
  require(free.status()->state == h::request_state::submitted, "other client submission passes held gate");
  require(vector_read(translation) == vec3f{}, "queued writes are not immediate authoring mutations");
  auto turn1 = host->pump(world, event_time{10}).value();
  require(turn1.request == free.id() && turn1.turn == 1 && turn1.state == h::activation_state::active, "host processes released FIFO with explicit turn");
  require(notices.empty(), "application callbacks never run inside native cascade");
  require(free.status()->state == h::request_state::processed && held.status()->state == h::request_state::accepted, "processed differs from accepted and submitted");
  report.requests.push_back(free.status()->state);
  observe_turn(1, {5, 10, 15});
  require(vector_read(host->get_field(other_transform, "translation").value()) == vec3f{}, "independent second scene unchanged");
  require(host->end_update(client_a).has_value(), "release accepted client request");
  require(held.status()->state == h::request_state::submitted, "release marks submission");
  auto turn2 = host->pump(world, event_time{10}).value();
  require(turn2.turn == 2 && turn2.request == held.id(), "equal host time is a new logical turn");
  observe_turn(2, {2.5f, 5, 7.5f});
  reject(host->pump(world, event_time{9}), error_code::event_time_regression, "decreasing host time rejected without turn");
  reject(host->pump(world, event_time{std::numeric_limits<double>::infinity()}), error_code::invalid_value, "nonfinite clock rejected");
  auto cancelled = host->enqueue(client_b, {{fraction, 1.f}}).value();
  reject(host->cancel(client_a, cancelled), error_code::invalid_context, "client cannot cancel another client request");
  require(host->cancel(client_b, cancelled).has_value(), "cancel released request before processing");
  require(cancelled.status()->state == h::request_state::cancelled, "cancel status terminal");
  report.requests.push_back(cancelled.status()->state);
  auto empty = host->pump(world, event_time{10}).value();
  require(!empty.request && empty.turn == 3 && vector_read(translation) == vec3f{2.5f, 5, 7.5f}, "empty turn ticks without replaying cancelled writes");
  require(host->dispatch_notifications().value() == 0, "empty no-output turn has no notices");
  std::size_t cancelled_calls = 0;
  auto cancelled_watch = host->observe(world, output, [&](const auto &) { ++cancelled_calls; }).value();
  auto final = host->enqueue(client_b, {{fraction, 1.f}}).value();
  auto turn4 = host->pump(world, event_time{10}).value();
  require(turn4.turn == 4 && turn4.request == final.id(), "new request processes after empty turn");
  cancelled_watch.cancel();
  observe_turn(4, {10, 20, 30});
  require(cancelled_calls == 0, "subscription cancellation suppresses already queued callbacks");
  auto second_fraction = host->get_field(host->named(ctx2, "Interpolator").value(), "set_fraction").value();
  require(host->enqueue(client_b, {{fraction, .1f}}).has_value(), "prepare queued notice before retirement");
  require(host->pump(world, event_time{10}).has_value(), "process without dispatch before retirement");
  auto pending = host->enqueue(client_a, {{fraction, 0.f}}).value();
  require(host->retire(world).has_value(), "host retires first activation");
  require(host->state(first).value() == h::activation_state::retired && pending.status()->state == h::request_state::cancelled, "retirement cancels pending work explicitly");
  require(host->dispatch_notifications().value() == 0 && notices.empty(), "retirement suppresses already queued notifications");
  reject(host->read(translation), error_code::stale_handle, "retired scene handles never dereference native storage");
  reject(host->enqueue(client_a, {{fraction, 0.f}}), error_code::stale_handle, "retired clients cannot enqueue");
  reject(host->pump(world, event_time{10}), error_code::stale_handle, "retired activation cannot tick");
  require(host->enqueue(client2, {{second_fraction, .75f}}).has_value(), "other scene survives retirement");
  require(host->pump(world2, event_time{10})->turn == 1, "turn clock belongs to its activation");
  require(vector_read(host->get_field(other_transform, "translation").value()) == vec3f{7.5f, 15, 22.5f}, "second independently owned native context remains operable");
  auto disconnected = host->enqueue(client2, {{second_fraction, 0.f}}).value();
  require(host->disconnect(client2).has_value(), "explicit disconnect");
  require(disconnected.status()->state == h::request_state::cancelled, "disconnect cancels own requests");
  reject(host->enqueue(client2, {{second_fraction, 0.f}}), error_code::stale_handle, "disconnected client denied");
  bool wrong_thread = false;
  std::thread worker([&] { auto r = host->roots(ctx2); wrong_thread = !r && r.error().code == error_code::wrong_thread; });
  worker.join(); require(wrong_thread, "cross-thread service entry rejected");
  require(host->close().has_value(), "owner-thread close succeeds");
  require(other_transform.expired(), "closing releases service authority");
  require(free.status()->state == h::request_state::processed &&
          pending.status()->state == h::request_state::cancelled &&
          disconnected.status()->state == h::request_state::cancelled,
          "owned terminal receipts remain inspectable after close");
  auto copied_receipt = free;
  free = {};
  reject(free.status(), error_code::stale_handle, "empty receipt status is checked");
  bool cross_thread_snapshot = false;
  std::thread receipt_reader([&] {
    const auto snapshot = copied_receipt.status();
    cross_thread_snapshot = snapshot && snapshot->state == h::request_state::processed;
  });
  receipt_reader.join();
  require(cross_thread_snapshot, "owning receipt snapshots are safe on another thread");
  reject(host->roots(ctx2), error_code::stale_handle, "closed host rejects access");
  // Coupled configuration is validated at boundaries, with no backend
  // mutation on rejected admission and no false rollback after execution.
  // Default admission limits apply to accepted work as well as released work.
  auto bounded = factory().value();
  auto bounded_scene = make_provider_route_scene(*bounded).value();
  auto bounded_node = bounded->named(bounded->root_context(bounded_scene).value(), "Interpolator").value();
  auto bounded_input = bounded->get_field(bounded_node, "set_fraction").value();
  auto bounded_world = bounded->activate(bounded_scene).value();
  auto bounded_client = bounded->connect(bounded_world).value();
  require(bounded->begin_update(bounded_client).has_value(), "bounded gate holds accepted work");
  std::vector<h::ticket> receipts;
  for (std::size_t index = 0; index < h::resource_limits{}.pending_requests; ++index) {
    auto accepted = bounded->enqueue(bounded_client, {{bounded_input, .5f}});
    if (!accepted) throw std::runtime_error("default request capacity rejects early");
    receipts.push_back(std::move(*accepted));
  }
  require(receipts.size() == 1024, "default accepts exactly 1024 pending requests");
  reject(bounded->enqueue(bounded_client, {{bounded_input, .75f}}), error_code::resource_limit,
         "held requests count toward per-service backpressure");
  require(bounded->cancel(bounded_client, receipts.front()).has_value(), "cancellation recovers pending capacity");
  auto replacement = bounded->enqueue(bounded_client, {{bounded_input, .75f}}).value();
  require(replacement.status()->state == h::request_state::accepted,
          "replacement is admitted without releasing the gate");
  require(bounded->retire(bounded_world).has_value(), "bounded scene retirement releases accepted work");
  bounded.reset();
  require(receipts.back().status()->state == h::request_state::cancelled &&
          replacement.status()->state == h::request_state::cancelled && bounded_node.expired(),
          "consumer receipts survive backend and service destruction without authority retention");
  auto checked_made = factory(); require(checked_made && bool(*checked_made), "create configuration host");
  auto checked = std::move(*checked_made);
  auto configured_scene = make_provider_route_scene(*checked).value();
  auto configured_context = checked->root_context(configured_scene).value();
  auto configured_node = checked->named(configured_context, "Interpolator").value();
  auto configured_key = checked->get_field(configured_node, "key").value();
  auto configured_values = checked->get_field(configured_node, "keyValue").value();
  auto configured_fraction = checked->get_field(configured_node, "set_fraction").value();
  auto configured_output = checked->get_field(configured_node, "value_changed").value();
  auto configured_translation = checked->get_field(checked->named(configured_context, "Transform").value(), "translation").value();
  require(checked->author(configured_key, std::vector<float>{0, .5f, 1}).has_value(), "temporary setup coupling mismatch allowed");
  reject(checked->activate(configured_scene), error_code::invalid_value, "invalid final setup rejected before activation");
  require(checked->state(configured_scene).value() == h::activation_state::setup, "preflight rejection keeps editable setup");
  require(checked->author(configured_values, std::vector<vec3f>{{0,0,0}, {5,10,15}, {10,20,30}}).has_value(), "repair setup curve coupling");
  auto configured_world = checked->activate(configured_scene).value();
  auto configured_client = checked->connect(configured_world).value();
  reject(checked->enqueue(configured_client, {{configured_key, std::vector<float>{0,1}}}), error_code::invalid_value, "mismatched whole-batch admission rejected");
  reject(checked->enqueue(configured_client, {{configured_key, std::vector<float>{0,1,.5f}}}), error_code::invalid_value, "unordered keys rejected before queue");
  reject(checked->enqueue(configured_client, {{configured_key, std::vector<float>{0,.5f,.5f}}}), error_code::invalid_value, "duplicate keys explicitly outside bounded curve support");
  const float huge = std::numeric_limits<float>::max();
  reject(checked->enqueue(configured_client, {
      {configured_key, std::vector<float>{-huge, huge}},
      {configured_values, std::vector<vec3f>{{0,0,0}, {1,2,3}}}}), error_code::invalid_value, "unrepresentable native key span rejected");
  reject(checked->enqueue(configured_client, {
      {configured_key, std::vector<float>{0,1}},
      {configured_values, std::vector<vec3f>{{-huge,0,0}, {huge,0,0}}}}), error_code::invalid_value, "unrepresentable native interpolation span rejected");
  require(checked->state(configured_scene).value() == h::activation_state::active, "rejected admission does not fault activation");
  auto resized = checked->enqueue(configured_client, {
      {configured_fraction, .5f}, {configured_key, std::vector<float>{0,1}},
      {configured_values, std::vector<vec3f>{{0,0,0}, {20,40,60}}}}).value();
  auto resized_turn = checked->pump(configured_world, event_time{1}).value();
  require(resized_turn.request == resized.id() && resized_turn.state == h::activation_state::active, "complete paired curve update processes");
  require(std::get<vec3f>(checked->read(configured_output).value()) == vec3f{10,20,30}, "fraction listed first sees admitted configuration-first curve");
  report.observations.push_back(std::get<vec3f>(checked->read(configured_translation).value()));
  std::size_t normal_calls = 0;
  auto throwing = checked->observe(configured_world, configured_output, [](const auto &) { throw std::runtime_error("application failure"); }).value();
  auto normal = checked->observe(configured_world, configured_output, [&](const auto &) { ++normal_calls; }).value();
  require(checked->enqueue(configured_client, {{configured_fraction, .25f}}).has_value(), "queue callback-isolation request");
  require(checked->pump(configured_world, event_time{1}).has_value(), "native processing independent of callback errors");
  reject(checked->dispatch_notifications(), error_code::callback_failed, "application exception reported only at deferred drain");
  require(normal_calls == 1 && checked->state(configured_scene).value() == h::activation_state::active, "other callback runs and native activation stays active");
  throwing.cancel(); normal.cancel();
  require(checked->dispatch_notifications().value() == 0, "failed application callback is not replayed");
  std::vector<h::notification> completed_history;
  auto history_subscription = checked->observe(configured_world, configured_output,
      [&](const auto &notice) { completed_history.push_back(notice); }).value();
  const auto completed_ticket = checked->enqueue(configured_client, {{configured_fraction, .75f}}).value();
  const auto completed_turn = checked->pump(configured_world, event_time{1}).value();
  require(completed_turn.request == completed_ticket.id() &&
          completed_ticket.status()->state == h::request_state::processed,
          "successful turn completes before later activation fault");
  require(completed_history.empty(), "completed history stays deferred until explicit drain");
  auto grow = checked->enqueue(configured_client, {
      {configured_key, std::vector<float>{0,.5f,1}},
      {configured_values, std::vector<vec3f>{{0,0,0}, {1,2,3}, {2,4,6}}}}).value();
  auto stale_projection = checked->enqueue(configured_client, {{configured_key, std::vector<float>{0,1}}}).value();
  auto behind_failure = checked->enqueue(configured_client, {{configured_fraction, 1.f}}).value();
  require(checked->pump(configured_world, event_time{1})->request == grow.id(), "earlier queued curve changes actual admission state");
  auto failed_turn = checked->pump(configured_world, event_time{1}).value();
  require(failed_turn.request == stale_projection.id() && failed_turn.state == h::activation_state::active &&
          failed_turn.effects == h::failure_effect::none && failed_turn.error.has_value(), "known-no-effect execution failure fails only this submission");
  auto failed_status = stale_projection.status().value();
  require(failed_status.state == h::request_state::failed && failed_status.effects == h::failure_effect::none, "failed request effects retained");
  require(behind_failure.status()->state == h::request_state::submitted, "later independent work survives harmless execution failure");
  require(std::get<std::vector<float>>(checked->read(configured_key).value()) == std::vector<float>{0,.5f,1}, "failed configuration preflight makes no native write");
  require(checked->dispatch_notifications().value() == 1 && completed_history.size() == 1,
          "later fault preserves previously completed observable history");
  require(completed_history.front().turn == completed_turn.turn &&
          completed_history.front().value == h::payload{vec3f{15,30,45}},
          "preserved notification owns its original successful turn and value");
  require(checked->dispatch_notifications().value() == 0,
          "failed-turn notifications are absent and completed history is not replayed");
  require(checked->pump(configured_world,event_time{1})->request==behind_failure.id() &&
          behind_failure.status()->state==h::request_state::processed,
          "host can process later work after known-no-mutation rejection");
  require(checked->enqueue(configured_client,{{configured_fraction,.5f}}).has_value(),
          "active world admits new work after harmless rejection");
  require(checked->retire(configured_world).has_value(), "faulted activation can be retired explicitly");
  report.requests.push_back(failed_status.state);
  run_provider_gate_fixture(factory, report);
  run_provider_graph_gate_fixture(factory, report);
  run_provider_initial_output_fixture(factory, report);
  return report;
}

// Scene information is backend-owned, but has one authority, lifecycle and SI
// value contract. Keep this oracle backend-blind, including detached nodes.
inline void run_provider_scene_information_fixture(
    const provider_factory &factory, provider_fixture_report &report) {
  const auto require = [&](bool okay, const char *why) {
    ++report.checks;
    if (!okay) throw std::runtime_error(why);
  };
  const auto reject = [&](const auto &result, error_code code, const char *why) {
    require(!result && result.error().code == code, why);
  };
  const auto create = [&] {
    auto made = factory();
    require(made && bool(*made), "scene-information provider creation succeeds");
    return std::move(*made);
  };
  auto service = create();
  const auto scene = service->create_scene().value();
  const auto context = service->root_context(scene).value();
  const auto separate = service->create_scene().value();
  const auto separate_context = service->root_context(separate).value();
  const std::vector<unit_declaration> defaults{
      {"angle", "radian", 1}, {"force", "newton", 1},
      {"length", "metre", 1}, {"mass", "kilogram", 1}};
  require(service->supported().supports(p::service_id::scene_units) &&
              service->supported().supports(p::service_id::scene_metadata),
          "provider advertises both optional scene-information services");
  require(service->units(context).value() == defaults,
          "new context returns all four effective SI units in stable order");
  require(service->metadata(scene)->empty() &&
              !service->metadata(scene, "missing").value(),
          "new scene has empty META discovery and absent keyed lookup");

  // Each invalid attempt is isolated: no implicit declaration, node, state
  // transition or poisoning may leak through before subsequent valid setup.
  std::vector<unit_declaration> invalid{
      {"", "unit", 1}, {"temperature", "kelvin", 1},
      {"Length", "metre", 1}, {"length ", "metre", 1},
      {"length", "", 1}, {"length", "metre", 0},
      {"length", "metre", -1}, {"length", "metre", -0.0},
      {"length", "metre", std::numeric_limits<double>::infinity()},
      {"length", "metre", -std::numeric_limits<double>::infinity()},
      {"length", "metre", std::numeric_limits<double>::quiet_NaN()}};
  for (const char whitespace : std::string{" \t\r\n\f\v"})
    invalid.push_back({"length", std::string{"half"} + whitespace + "metre", 0.5});
  for (const auto &unit : invalid) {
    reject(service->declare_unit(scene, unit), error_code::invalid_value,
           "invalid unit category, name or conversion must be rejected");
    require(service->units(context).value() == defaults &&
                service->metadata(scene)->empty() && service->roots(context)->empty() &&
                service->state(scene).value() == p::activation_state::setup,
            "invalid unit leaves effective defaults and setup state unchanged");
  }
  reject(service->create_node(context, "NotAnX3DNode"), error_code::unknown_type,
         "failed node creation precedes unit declarations without sealing setup");
  auto effective = defaults;
  // Deliberately declare in reverse discovery order; the names are not DEFs.
  for (const auto &unit : std::vector<unit_declaration>{
           {"mass", "gram", 0.001}, {"length", "centi-metre", 0.01},
           {"force", "double-newton", 2}, {"angle", "half-radian", 0.5}}) {
    require(bool(service->declare_unit(scene, unit)),
            "every standard unit category accepts an explicit declaration");
    for (auto &entry : effective)
      if (entry.category == unit.category) entry = unit;
    require(service->units(context).value() == effective,
            "explicit override preserves SI defaults and category discovery order");
    reject(service->declare_unit(scene, unit), error_code::invalid_value,
           "even an identical explicit category cannot be declared twice");
    reject(service->declare_unit(scene, {unit.category, "replacement", 3}),
           error_code::invalid_value, "a duplicate category cannot replace its factor");
    require(service->units(context).value() == effective &&
                service->state(scene).value() == p::activation_state::setup,
            "duplicate declaration preserves prior units without poisoning setup");
  }
  require(service->units(separate_context).value() == defaults,
          "units are scoped to a scene rather than shared by one service");

  const std::string description = "Owning scene description: caf\xc3\xa9 and \xe4\xb8\x96\xe7\x95\x8c";
  require(bool(service->set_metadata(scene, "title", "First world")) &&
              bool(service->set_metadata(scene, "creator", "Author")) &&
              bool(service->set_metadata(scene, "empty", "")) &&
              bool(service->set_metadata(scene, "Title", "Case matters")) &&
              bool(service->set_metadata(scene, "", "Empty key")) &&
              bool(service->set_metadata(scene, "description", description)),
          "scene META permits case-sensitive and empty keys and owning UTF-8 values");
  const auto original_metadata = service->metadata(scene).value();
  const auto original_title = service->metadata(scene, "title").value();
  require(original_metadata == std::vector<metadata_entry>{
              {"title", "First world"}, {"creator", "Author"}, {"empty", ""},
              {"Title", "Case matters"}, {"", "Empty key"}, {"description", description}},
          "scene META discovery retains insertion order");
  require(service->metadata(scene, "empty").value() == std::optional<std::string>{""} &&
              service->metadata(scene, "").value() == std::optional<std::string>{"Empty key"} &&
              !service->metadata(scene, "TITLE").value(),
          "empty values, empty keys and missing case-sensitive keys remain distinct");
  require(bool(service->set_metadata(scene, "title", "Second world")) &&
              bool(service->set_metadata(scene, "missing", std::nullopt)),
          "replacement and absent-key removal are immediate setup operations");
  auto expected_metadata = original_metadata;
  expected_metadata.front().value = "Second world";
  require(service->metadata(scene).value() == expected_metadata,
          "replacement keeps insertion position and absent removal is a no-op");
  require(bool(service->set_metadata(scene, "title", std::nullopt)) &&
              !service->metadata(scene, "title").value() &&
              bool(service->set_metadata(scene, "title", "Third world")),
          "a removed META key can be reinserted");
  expected_metadata.erase(expected_metadata.begin());
  expected_metadata.push_back({"title", "Third world"});
  require(service->metadata(scene).value() == expected_metadata &&
              original_metadata.front().value == "First world" &&
              original_title == std::optional<std::string>{"First world"},
          "reinsert appends while earlier discovery and lookup copies stay unchanged");
  auto altered_units = service->units(context).value();
  auto altered_metadata = service->metadata(scene).value();
  altered_units.front().name = "consumer edit";
  altered_metadata.front().value = "consumer edit";
  require(service->units(context).value() == effective &&
              service->metadata(scene).value() == expected_metadata &&
              service->metadata(separate)->empty(),
          "owning return values cannot mutate backend storage or another scene");

  // Ordinary authority checks also govern every scene-information entry point.
  const auto reject_authority = [&](const p::scene &target,
                                    const p::context &target_context,
                                    error_code code) {
    reject(service->units(target_context), code, "unit discovery validates context authority");
    reject(service->declare_unit(target, {"length", "metre", 1}), code,
           "unit declaration validates scene authority before authoring state");
    reject(service->metadata(target), code, "META discovery validates scene authority");
    reject(service->metadata(target, "title"), code, "META lookup validates scene authority");
    reject(service->set_metadata(target, "title", "invalid"), code,
           "META authoring validates scene authority");
  };
  reject_authority({}, {}, error_code::stale_handle);
  auto foreign_service = create();
  const auto foreign_scene = foreign_service->create_scene().value();
  const auto foreign_context = foreign_service->root_context(foreign_scene).value();
  reject_authority(foreign_scene, foreign_context, error_code::invalid_context);
  auto disposed_scene = scene;
  auto disposed_context = context;
  disposed_scene.dispose();
  disposed_context.dispose();
  reject_authority(disposed_scene, disposed_context, error_code::disposed);
  require(service->units(context).value() == effective &&
              service->metadata(scene).value() == expected_metadata,
          "disposing wrappers and rejecting foreign authority leave copied handles usable");
  foreign_service.reset();
  reject_authority(foreign_scene, foreign_context, error_code::stale_handle);
  result<std::vector<unit_declaration>> wrong_units;
  result<void> wrong_declaration, wrong_set;
  result<std::vector<metadata_entry>> wrong_metadata;
  result<std::optional<std::string>> wrong_lookup;
  std::thread wrong_thread([&] {
    wrong_units = service->units(context);
    wrong_declaration = service->declare_unit(separate, {"length", "off-thread", 2});
    wrong_metadata = service->metadata(scene);
    wrong_lookup = service->metadata(scene, "title");
    wrong_set = service->set_metadata(scene, "title", "off-thread");
  });
  wrong_thread.join();
  reject(wrong_units, error_code::wrong_thread, "unit discovery is owner-thread affine");
  reject(wrong_declaration, error_code::wrong_thread, "unit declaration is owner-thread affine");
  reject(wrong_metadata, error_code::wrong_thread, "META discovery is owner-thread affine");
  reject(wrong_lookup, error_code::wrong_thread, "META lookup is owner-thread affine");
  reject(wrong_set, error_code::wrong_thread, "META authoring is owner-thread affine");
  require(service->units(separate_context).value() == defaults &&
              service->metadata(scene).value() == expected_metadata,
          "wrong-thread writes leave both scenes unchanged");

  const auto transform = service->create_node(context, "Transform").value();
  const auto detached = service->create_node(context, "Transform").value();
  const auto interpolator = service->create_node(context, "PositionInterpolator").value();
  const auto box = service->create_node(context, "Box").value();
  require(service->roots(context)->empty(),
          "successful detached node creation counts as scene content");
  const auto sealed = service->create_node(separate_context, "Transform").value();
  (void)sealed;
  reject(service->declare_unit(separate, {"length", "late-metre", 2}),
         error_code::access_denied, "first successful detached node seals unit declarations");
  require(service->units(separate_context).value() == defaults,
          "late declaration cannot change the effective units of detached content");
  require(service->read(transform, bindings::Transform::translation).value() == vec3f{} &&
              service->read(transform, bindings::Transform::rotation).value() == rotation{} &&
              std::get<vec3f>(detail::read(*service, box, "size").value()) == vec3f{2,2,2},
          "Transform and Box defaults remain canonical SI under non-SI declarations");
  const vec3f setup_translation{2,4,6};
  const rotation setup_rotation{0,1,0,0.5f};
  const vec3f_list setup_values{{2,4,6}, {10,20,30}};
  require(bool(service->author(transform, bindings::Transform::translation, setup_translation)) &&
              bool(service->author(transform, bindings::Transform::rotation, setup_rotation)) &&
              bool(service->author(detached, bindings::Transform::translation, setup_translation)) &&
              bool(service->author(detached, bindings::Transform::rotation, setup_rotation)) &&
              bool(service->author(interpolator, bindings::PositionInterpolator::key, float_list{0,1})) &&
              bool(service->author(interpolator, bindings::PositionInterpolator::keyValue, setup_values)),
          "setup authoring accepts canonical metre/radian values for rooted and detached nodes");
  const auto translation = service->get_field(transform, "translation").value();
  const auto detached_translation = service->get_field(detached, "translation").value();
  const auto rotation_field = service->get_field(transform, "rotation").value();
  const auto output = service->get_field(interpolator, "value_changed").value();
  require(bool(service->append_root(context, transform)) &&
              bool(service->add_route(context, output, translation)) &&
              bool(service->add_route(context, output, detached_translation)),
          "one detached interpolator can route canonical values to rooted and detached targets");
  const auto before_roots = service->roots(context).value();
  const auto before_fields = service->fields(transform).value();
  const auto before_node_metadata = service->read(transform, bindings::Transform::metadata).value();
  require(bool(service->set_user_data(transform, user_data_value::make<std::string>("node data"))) &&
              bool(service->set_user_data(translation, user_data_value::make<std::string>("field data"))) &&
              bool(service->set_metadata(scene, "metadata", "scene data")),
          "scene META stays editable after node creation alongside node and field user data");
  expected_metadata.push_back({"metadata", "scene data"});
  require(service->metadata(scene).value() == expected_metadata &&
              service->read(transform, bindings::Transform::metadata).value() == before_node_metadata &&
              **service->user_data(transform)->as<std::string>() == "node data" &&
              **service->user_data(translation)->as<std::string>() == "field data" &&
              service->roots(context).value() == before_roots &&
              service->fields(transform).value() == before_fields,
          "scene META is independent of SFNode metadata, user data, fields and roots");
  const auto require_canonical_setup = [&] {
    require(service->read(transform, bindings::Transform::translation).value() == setup_translation &&
                service->read(detached, bindings::Transform::translation).value() == setup_translation &&
                service->read(transform, bindings::Transform::rotation).value() == setup_rotation &&
                service->read(detached, bindings::Transform::rotation).value() == setup_rotation &&
                service->read(interpolator, bindings::PositionInterpolator::keyValue).value() == setup_values &&
                service->read(interpolator, bindings::PositionInterpolator::value_changed).value() == setup_values.front() &&
                std::get<vec3f>(detail::read(*service, box, "size").value()) == vec3f{2,2,2},
            "setup fields, detached interpolation output and Box defaults remain canonical SI");
  };
  require_canonical_setup();
  const auto active = service->activate(scene).value();
  require_canonical_setup();
  require(service->units(context).value() == effective &&
              service->metadata(scene).value() == expected_metadata &&
              service->metadata(scene, "title").value() == std::optional<std::string>{"Third world"},
          "unit and META reads are unchanged across activation");
  std::vector<p::notification> notices;
  const auto record = [&](const p::notification &notice) { notices.push_back(notice); };
  auto watch_translation = service->observe(active, translation, record).value();
  auto watch_rotation = service->observe(active, rotation_field, record).value();
  auto watch_output = service->observe(active, output, record).value();
  reject(service->declare_unit(scene, {"length", "live-metre", 4}), error_code::access_denied,
         "active scenes cannot change unit declarations");
  reject(service->set_metadata(scene, "title", "live title"), error_code::access_denied,
         "active scenes cannot replace scene META");
  reject(service->set_metadata(scene, "missing", std::nullopt), error_code::access_denied,
         "even absent-key removal is denied outside setup");
  require(service->dispatch_notifications().value() == 0 && notices.empty(),
          "setup scene information and activation create no field callbacks");
  require(service->pump(active, {1}).has_value() &&
              service->dispatch_notifications().value() == 0 && notices.empty(),
          "empty live turn after scene-information reads and rejected edits stays quiet");
  require(service->units(context).value() == effective &&
              service->metadata(scene).value() == expected_metadata,
          "rejected active edits preserve declared units and META values");
  const auto client = service->connect(active).value();
  const vec3f live_translation{3,6,9};
  const rotation live_rotation{0,1,0,1.25f};
  auto direct = service->enqueue(client, {{translation, live_translation},
                                         {rotation_field, live_rotation}}).value();
  require(service->pump(active, {2})->request == direct.id() &&
              direct.status()->state == p::request_state::processed &&
              service->read(transform, bindings::Transform::translation).value() == live_translation &&
              service->read(transform, bindings::Transform::rotation).value() == live_rotation,
          "live scalar ingress and readback retain canonical metres and radians");
  require(notices.empty(), "canonical live events remain deferred until explicit dispatch");
  require(service->dispatch_notifications().value() == 2 && notices.size() == 2 &&
              std::any_of(notices.begin(), notices.end(), [&](const auto &n) {
                return n.source == translation && n.value == p::payload{live_translation};
              }) && std::any_of(notices.begin(), notices.end(), [&](const auto &n) {
                return n.source == rotation_field && n.value == p::payload{live_rotation};
              }),
          "live translation and rotation notifications carry unscaled canonical values");
  notices.clear();
  const vec3f_list live_values{{4,8,12}, {12,24,36}};
  const vec3f midpoint{8,16,24};
  auto routed = service->enqueue(client, {
      {service->get_field(interpolator, "keyValue").value(), live_values},
      {service->get_field(interpolator, "set_fraction").value(), 0.5f}}).value();
  require(service->pump(active, {3})->request == routed.id() &&
              routed.status()->state == p::request_state::processed &&
              service->read(interpolator, bindings::PositionInterpolator::keyValue).value() == live_values &&
              service->read(interpolator, bindings::PositionInterpolator::value_changed).value() == midpoint &&
              service->read(transform, bindings::Transform::translation).value() == midpoint &&
              service->read(detached, bindings::Transform::translation).value() == midpoint,
          "live MFVec3f ingress, interpolation and routing stay canonical for detached nodes");
  require(service->dispatch_notifications().value() == 2 && notices.size() == 2 &&
              std::all_of(notices.begin(), notices.end(), [&](const auto &n) {
                return (n.source == output || n.source == translation) &&
                    n.value == p::payload{midpoint} && n.time.seconds == 3;
              }),
          "routed output and target notifications retain canonical metres");
  require(service->units(context).value() == effective &&
              service->metadata(scene).value() == expected_metadata &&
              service->roots(context).value() == before_roots,
          "host turns do not alter declared units, scene META or detached-node ownership");

  // Activation itself seals units even when no node was ever created.
  const auto empty_scene = service->create_scene().value();
  const auto empty_context = service->root_context(empty_scene).value();
  require(bool(service->activate(empty_scene)), "empty scene can activate");
  reject(service->declare_unit(empty_scene, {"length", "late-empty-unit", 2}),
         error_code::access_denied, "activation seals declarations even without nodes");
  require(service->units(empty_context).value() == defaults,
          "rejected post-activation declaration preserves all SI defaults");
  const auto retained_units = service->units(context).value();
  const auto retained_metadata = service->metadata(scene).value();
  const auto retained_description = service->metadata(scene, "description").value();
  require(bool(service->retire(active)), "scene-information scene retires normally");
  reject_authority(scene, context, error_code::stale_handle);
  require(bool(service->retire(separate)), "setup scene with information can retire");
  reject_authority(separate, separate_context, error_code::stale_handle);
  const auto replacement = service->create_scene().value();
  require(service->units(service->root_context(replacement).value()).value() == defaults &&
              service->metadata(replacement)->empty(),
          "new scenes never inherit information from retired backend storage");
  service.reset();
  require(scene.expired() && context.expired() && retained_units == effective &&
              retained_metadata == expected_metadata &&
              retained_description == std::optional<std::string>{description} &&
              original_title == std::optional<std::string>{"First world"},
          "owning unit vectors, META vectors and keyed strings survive provider destruction");
}

inline provider_fixture_report run_provider_fixture(const provider_factory &factory) {
  auto report=run_provider_authoring_fixture(factory);
  auto live=run_provider_live_fixture(factory);
  report.checks+=live.checks;
  report.observations=std::move(live.observations);
  report.notifications=std::move(live.notifications);
  report.requests=std::move(live.requests);
  run_provider_scene_information_fixture(factory, report);
  return report;
}
} // namespace x3d::sai::experimental::testing
#endif
