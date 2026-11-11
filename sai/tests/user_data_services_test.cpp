#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <atomic>
#include <cstdlib>
#include <functional>
#include <latch>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
thread_local int allocations_before_failure = -1;
}

// This test has its own executable, as do the callback fault-injection tests.
// Disarm before unwinding so destructor reentry can use the ordinary API.
void *operator new(std::size_t size) {
  if (allocations_before_failure >= 0 && allocations_before_failure-- == 0) {
    allocations_before_failure = -1;
    throw std::bad_alloc{};
  }
  if (void *allocation = std::malloc(size == 0 ? 1 : size))
    return allocation;
  throw std::bad_alloc{};
}
void operator delete(void *allocation) noexcept { std::free(allocation); }
void operator delete(void *allocation, std::size_t) noexcept {
  std::free(allocation);
}

namespace sai = x3d::sai::experimental;

namespace {
struct measure {
  static constexpr auto x3d_name = "Measure";
  static constexpr sai::field_key<measure, std::int32_t> value{
      "value", sai::access_type::input_output};
  static constexpr sai::field_key<measure, float> inaccurate{
      "value", sai::access_type::output_only};
  static constexpr sai::field_key<measure, std::int32_t>
  test_key(std::string_view name) {
    return {name, sai::access_type::input_output};
  }
};

sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "Measure";
  for (auto access : {sai::access_type::input_output,
                      sai::access_type::input_only,
                      sai::access_type::output_only,
                      sai::access_type::initialize_only}) {
    sai::field_descriptor field;
    field.name = access == sai::access_type::input_output ? "value" :
                 access == sai::access_type::input_only ? "input" :
                 access == sai::access_type::output_only ? "output" : "setup";
    field.kind = sai::value_kind::sf_int32;
    field.access = access;
    field.default_value = std::int32_t{0};
    type.fields.push_back(std::move(field));
  }
  REQUIRE(types.define(std::move(type)));
  return types;
}

sai::typed_node<measure> create(const sai::execution_context &scene) {
  auto edit = scene.edit();
  auto owner = edit.create<measure>().value();
  REQUIRE(edit.commit());
  return owner;
}

struct on_destroy {
  explicit on_destroy(std::function<void()> action) : action(std::move(action)) {}
  std::function<void()> action;
  ~on_destroy() noexcept { action(); }
};
struct throwing_destructor {
  ~throwing_destructor() noexcept(false) {}
};
template <class T>
concept can_store = requires { sai::user_data_value::make<T>(); };
static_assert(!can_store<throwing_destructor>);
static_assert(std::is_nothrow_copy_constructible_v<sai::user_data_value>);
static_assert(std::is_nothrow_move_constructible_v<sai::user_data_value>);

void check_error(const auto &result, sai::error_code code,
                 const std::string &operation, const sai::node &owner,
                 const std::string &field = {}) {
  REQUIRE_FALSE(result);
  CHECK(result.error().code == code);
  CHECK(result.error().operation == operation);
  CHECK(result.error().generation == owner.generation());
  CHECK(result.error().node == owner.id());
  CHECK(result.error().field == field);
  CHECK_FALSE(result.error().message.empty());
}
} // namespace

TEST_CASE("user data owns exact typed payloads including move-only values") {
  const sai::user_data_value empty;
  CHECK_FALSE(empty.has_value());
  REQUIRE(empty.as<std::string>());
  CHECK_FALSE(empty.as<std::string>().value());
  auto data = sai::user_data_value::make<std::unique_ptr<int>>(
      std::make_unique<int>(41));
  const auto copy = data;
  const auto pointer = data.as<std::unique_ptr<int>>().value();
  CHECK(**pointer == 41);
  CHECK(copy.as<std::unique_ptr<int>>().value() == pointer);
  data = {};
  CHECK(**pointer == 41);
  REQUIRE_FALSE(copy.as<int>());
  CHECK(copy.as<int>().error().code == sai::error_code::type_mismatch);
  CHECK(copy.as<int>().error().operation == "user_data_value.as");
}

TEST_CASE("node user data shares identity across copies names roots and history") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto owner = create(scene);
  auto edit = scene.edit();
  REQUIRE(edit.append_root(owner.dynamic()));
  REQUIRE(edit.define_name("Retained", owner.dynamic()));
  REQUIRE(edit.commit());
  const auto history = scene.snapshot();
  auto alias = history.named("Retained").value();
  REQUIRE(owner.user_data());
  CHECK_FALSE(owner.user_data()->has_value());
  auto original = sai::user_data_value::make<std::string>("node association");
  const auto pointer = original.as<std::string>().value();
  REQUIRE(alias.set_user_data(std::move(original)));
  CHECK(owner.user_data()->as<std::string>().value() == pointer);
  CHECK(history.named("Retained")->user_data()->as<std::string>().value() ==
        pointer);
  owner.dispose();
  alias.dispose();
  CHECK(history.named("Retained")->user_data()->as<std::string>().value() ==
        pointer);
  REQUIRE(history.named("Retained")->set_user_data({}));
  CHECK_FALSE(history.named("Retained")->user_data()->has_value());
  CHECK(*pointer == "node association");
}

TEST_CASE("field user data shares canonical aliases independently of node wrappers") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto owner = create(scene);
  auto canonical = owner.get_field("value").value();
  auto input_alias = owner.get_field("set_value").value();
  auto output_alias = owner.get_field("value_changed").value();
  auto typed = canonical.as<std::int32_t>().value();
  const auto history_field = scene.snapshot().field(owner.dynamic(), "value").value();
  REQUIRE(typed.set_user_data(sai::user_data_value::make<std::string>("field")));
  const auto pointer = typed.user_data()->as<std::string>().value();
  CHECK(input_alias.user_data()->as<std::string>().value() == pointer);
  CHECK(output_alias.user_data()->as<std::string>().value() == pointer);
  CHECK(history_field.user_data()->as<std::string>().value() == pointer);
  // Unchecked typed projections cache descriptors; metadata identity uses the
  // actual canonical field, as get_definition() already does.
  const auto projected = owner.field(measure::inaccurate);
  CHECK(projected.get_definition()->kind == sai::value_kind::sf_int32);
  CHECK(projected.user_data()->as<std::string>().value() == pointer);
  CHECK_FALSE(owner.user_data()->has_value());
  for (const auto *name : {"input", "output", "setup"}) {
    const auto other = owner.get_field(name).value();
    CHECK_FALSE(other.user_data()->has_value());
    REQUIRE(other.set_user_data(sai::user_data_value::make<std::string>(name)));
    CHECK(*other.user_data()->as<std::string>().value() == name);
  }
  canonical.dispose();
  input_alias.dispose();
  output_alias.dispose();
  typed.dispose();
  CHECK(owner.get_field("value")->user_data()->as<std::string>().value() == pointer);
  auto independent = owner.get_field(measure::value).value();
  owner.dispose();
  CHECK(independent.user_data()->as<std::string>().value() == pointer);
  REQUIRE(independent.set_user_data({}));
  CHECK_FALSE(history_field.user_data()->has_value());
}

TEST_CASE("user data changes are immediate metadata without scene or event effects") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto owner = create(scene);
  const auto field = owner.get_field(measure::value).value();
  const auto before = scene.snapshot();
  int notifications = 0;
  auto observer = scene.observe([&](const sai::change_set &) { ++notifications; });
  auto events = scene.observe(field.dynamic(),
      [&](const sai::event_delivery &) { ++notifications; }).value();
  auto pending = scene.edit();
  REQUIRE(pending.set(field, std::int32_t{7}));
  REQUIRE(owner.set_user_data(sai::user_data_value::make<int>(1)));
  REQUIRE(field.set_user_data(sai::user_data_value::make<int>(2)));
  CHECK(*owner.user_data()->as<int>().value() == 1);
  CHECK(*field.user_data()->as<int>().value() == 2);
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.snapshot().read(field).value() == 0);
  CHECK_FALSE(owner.is_realized().value());
  CHECK(scene.drain().delivered == 0);
  CHECK(notifications == 0);
  REQUIRE(pending.commit());
  CHECK(scene.snapshot().read(field).value() == 7);
  CHECK(*owner.user_data()->as<int>().value() == 1);
  CHECK(*field.user_data()->as<int>().value() == 2);
  CHECK(before.read(field).value() == 0);
  CHECK(scene.drain().delivered == 1);
  CHECK(notifications == 1);
}

TEST_CASE("user data handle failures preserve structured authority diagnostics") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto owner = create(scene);
  auto field = owner.get_field("value").value();
  auto typed = owner.get_field(measure::value).value();
  const auto original = owner.dynamic();
  SUBCASE("disposed node and independently acquired field") {
    owner.dispose();
    check_error(owner.user_data(), sai::error_code::disposed,
                "node.user_data", original);
    check_error(owner.set_user_data({}), sai::error_code::disposed,
                "node.set_user_data", original);
    REQUIRE(field.set_user_data(sai::user_data_value::make<int>(1)));
    REQUIRE(field.user_data());
  }
  SUBCASE("disposed field and typed forwarding") {
    field.dispose();
    typed.dispose();
    check_error(field.user_data(), sai::error_code::disposed,
                "dynamic_field.user_data", original, "value");
    check_error(field.set_user_data({}), sai::error_code::disposed,
                "dynamic_field.set_user_data", original, "value");
    check_error(typed.user_data(), sai::error_code::disposed,
                "dynamic_field.user_data", original, "value");
    check_error(typed.set_user_data({}), sai::error_code::disposed,
                "dynamic_field.set_user_data", original, "value");
    REQUIRE(owner.user_data());
  }
  SUBCASE("unpublished node") {
    auto pending = scene.edit();
    const auto absent = pending.create<measure>().value();
    check_error(absent.user_data(), sai::error_code::unknown_node,
                "node.user_data", absent.dynamic());
    check_error(absent.set_user_data({}), sai::error_code::unknown_node,
                "node.set_user_data", absent.dynamic());
  }
  SUBCASE("removed node") {
    auto remove = scene.edit();
    REQUIRE(remove.remove_node(original));
    REQUIRE(remove.commit());
    check_error(owner.user_data(), sai::error_code::unknown_node,
                "node.user_data", original);
    check_error(field.set_user_data({}), sai::error_code::unknown_node,
                "dynamic_field.set_user_data", original, "value");
  }
  SUBCASE("retired context") {
    REQUIRE(host.replace_world(host.create_scene()));
    check_error(owner.user_data(), sai::error_code::stale_handle,
                "node.user_data", original);
    check_error(field.set_user_data({}), sai::error_code::stale_handle,
                "dynamic_field.set_user_data", original, "value");
  }
  SUBCASE("unknown and empty fields cannot alias node data") {
    REQUIRE(owner.set_user_data(sai::user_data_value::make<int>(1)));
    for (const auto *name : {"absent", ""}) {
      const auto key = measure::test_key(name);
      const auto invalid = owner.field(key);
      check_error(invalid.user_data(), sai::error_code::unknown_field,
                  "dynamic_field.user_data", original, name);
      check_error(invalid.set_user_data({}), sai::error_code::unknown_field,
                  "dynamic_field.set_user_data", original, name);
    }
    CHECK(*owner.user_data()->as<int>().value() == 1);
  }
  const sai::node null;
  check_error(null.user_data(), sai::error_code::stale_handle,
              "node.user_data", null);
  check_error(null.set_user_data({}), sai::error_code::stale_handle,
              "node.set_user_data", null);
}

TEST_CASE("user data is isolated across same local identities in foreign contexts") {
  sai::browser first{registry()};
  sai::browser second{registry()};
  const auto a = create(first.current_scene());
  const auto b = create(second.current_scene());
  CHECK(a.id() == b.id());
  CHECK(a.dynamic().generation() == b.dynamic().generation());
  REQUIRE(a.set_user_data(sai::user_data_value::make<int>(1)));
  REQUIRE(b.set_user_data(sai::user_data_value::make<int>(2)));
  const auto af = a.get_field(measure::value).value();
  const auto bf = b.get_field(measure::value).value();
  REQUIRE(af.set_user_data(sai::user_data_value::make<int>(3)));
  REQUIRE(bf.set_user_data(sai::user_data_value::make<int>(4)));
  CHECK(*a.user_data()->as<int>().value() == 1);
  CHECK(*b.user_data()->as<int>().value() == 2);
  CHECK(*af.user_data()->as<int>().value() == 3);
  CHECK(*bf.user_data()->as<int>().value() == 4);
}

TEST_CASE("replacing and clearing user data destroys payloads outside internal locks") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto owner = create(scene);
  const auto field = owner.get_field(measure::value).value();
  int destroyed = 0;
  const auto action = [&] {
    CHECK(host.current_scene().generation() == scene.generation());
    CHECK(scene.snapshot().revision() == 1);
    REQUIRE(owner.user_data());
    REQUIRE(field.user_data());
    REQUIRE(owner.set_user_data(sai::user_data_value::make<int>(17)));
    ++destroyed;
  };
  SUBCASE("node replacement") {
    REQUIRE(owner.set_user_data(sai::user_data_value::make<on_destroy>(action)));
    REQUIRE(owner.set_user_data(sai::user_data_value::make<int>(2)));
  }
  SUBCASE("field replacement") {
    REQUIRE(field.set_user_data(sai::user_data_value::make<on_destroy>(action)));
    REQUIRE(field.set_user_data(sai::user_data_value::make<int>(2)));
  }
  SUBCASE("node clear") {
    REQUIRE(owner.set_user_data(sai::user_data_value::make<on_destroy>(action)));
    REQUIRE(owner.set_user_data({}));
  }
  SUBCASE("field clear") {
    REQUIRE(field.set_user_data(sai::user_data_value::make<on_destroy>(action)));
    REQUIRE(field.set_user_data({}));
  }
  CHECK(destroyed == 1);
  CHECK(*owner.user_data()->as<int>().value() == 17);
}

TEST_CASE("failed user data writes destroy incoming payloads after unlocking") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto owner = create(scene);
  auto field = owner.get_field("value").value();
  int destroyed = 0;
  const auto action = [&] {
    CHECK(host.current_scene().generation() != 0);
    CHECK(scene.snapshot().revision() >= 1);
    ++destroyed;
  };
  SUBCASE("disposed node") {
    owner.dispose();
    CHECK_FALSE(owner.set_user_data(sai::user_data_value::make<on_destroy>(action)));
  }
  SUBCASE("disposed field") {
    field.dispose();
    CHECK_FALSE(field.set_user_data(sai::user_data_value::make<on_destroy>(action)));
  }
  SUBCASE("retired node") {
    REQUIRE(host.replace_world(host.create_scene()));
    CHECK_FALSE(owner.set_user_data(sai::user_data_value::make<on_destroy>(action)));
  }
  SUBCASE("retired field") {
    REQUIRE(host.replace_world(host.create_scene()));
    CHECK_FALSE(field.set_user_data(sai::user_data_value::make<on_destroy>(action)));
  }
  SUBCASE("removed node") {
    auto edit = scene.edit();
    REQUIRE(edit.remove_node(owner.dynamic()));
    REQUIRE(edit.commit());
    CHECK_FALSE(owner.set_user_data(sai::user_data_value::make<on_destroy>(action)));
  }
  SUBCASE("unknown field") {
    const auto key = measure::test_key("unknown");
    CHECK_FALSE(owner.field(key).set_user_data(
        sai::user_data_value::make<on_destroy>(action)));
  }
  CHECK(destroyed == 1);
}

TEST_CASE("user data insertion failure preserves associations and destroys after unlock") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto owner = create(scene);
  const auto field = owner.get_field(measure::value).value();
  REQUIRE(owner.set_user_data(sai::user_data_value::make<int>(9)));
  int destroyed = 0;
  auto incoming = sai::user_data_value::make<on_destroy>([&] {
    CHECK(scene.snapshot().revision() == 1);
    CHECK(host.current_scene().generation() == scene.generation());
    CHECK(*owner.user_data()->as<int>().value() == 9);
    CHECK_FALSE(field.user_data()->has_value());
    ++destroyed;
  });
  bool failed = false;
  allocations_before_failure = 0;
  try {
    (void)field.set_user_data(std::move(incoming));
  } catch (const std::bad_alloc &) {
    failed = true;
  }
  allocations_before_failure = -1;
  CHECK(failed);
  CHECK(destroyed == 1);
  CHECK(*owner.user_data()->as<int>().value() == 9);
  CHECK_FALSE(field.user_data()->has_value());
  CHECK(scene.snapshot().revision() == 1);
  CHECK(scene.drain().delivered == 0);
}

TEST_CASE("user data ownership survives replacement removal and retirement until context release") {
  std::weak_ptr<const int> node_weak;
  std::weak_ptr<const int> field_weak;
  std::optional<sai::scene_snapshot> history;
  sai::node expired;
  std::optional<sai::dynamic_field> expired_field;
  {
    sai::browser host{registry()};
    const auto scene = host.current_scene();
    const auto owner = create(scene);
    const auto field = owner.get_field("value").value();
    expired = owner.dynamic();
    expired_field = field;
    auto node_data = sai::user_data_value::make<int>(1);
    auto field_data = sai::user_data_value::make<int>(2);
    node_weak = node_data.as<int>().value();
    field_weak = field_data.as<int>().value();
    REQUIRE(owner.set_user_data(std::move(node_data)));
    REQUIRE(field.set_user_data(std::move(field_data)));
    history = scene.snapshot();
    auto removal = scene.edit();
    REQUIRE(removal.remove_node(owner.dynamic()));
    REQUIRE(removal.commit());
    CHECK_FALSE(node_weak.expired());
    CHECK_FALSE(field_weak.expired());
    const auto later = create(scene);
    CHECK(later.id() != owner.id());
    CHECK_FALSE(later.user_data()->has_value());
    REQUIRE(host.replace_world(host.create_scene()));
    CHECK_FALSE(node_weak.expired());
    CHECK_FALSE(field_weak.expired());
  }
  CHECK_FALSE(node_weak.expired());
  CHECK_FALSE(field_weak.expired());
  history.reset();
  CHECK(node_weak.expired());
  CHECK(field_weak.expired());
  check_error(expired.user_data(), sai::error_code::stale_handle,
              "node.user_data", expired);
  check_error(expired_field->user_data(), sai::error_code::stale_handle,
              "dynamic_field.user_data", expired, "value");
}

TEST_CASE("user data concurrent reads retain payload lifetime while writes replace and clear") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  const auto owner = create(scene);
  const auto field = owner.get_field(measure::value).value();
  std::latch start{3};
  std::atomic_bool valid{true};
  std::vector<std::shared_ptr<const int>> retained;
  std::jthread writer([&] {
    start.arrive_and_wait();
    for (int i = 0; i != 1000; ++i) {
      if (!owner.set_user_data(sai::user_data_value::make<int>(i)) ||
          !field.set_user_data(sai::user_data_value::make<int>(i)))
        valid = false;
      if (i % 3 == 0 && (!owner.set_user_data({}) || !field.set_user_data({})))
        valid = false;
    }
  });
  std::jthread reader([&] {
    start.arrive_and_wait();
    for (int i = 0; i != 1000; ++i) {
      for (const auto &data : {owner.user_data(), field.user_data()}) {
        if (!data) { valid = false; continue; }
        const auto typed = data->as<int>();
        if (!typed) { valid = false; continue; }
        if (*typed) retained.push_back(*typed);
      }
    }
  });
  start.arrive_and_wait();
  for (int i = 0; i != 1000; ++i) {
    const auto snapshot = scene.snapshot();
    if (snapshot.revision() != 1 || snapshot.read(field).value() != 0)
      valid = false;
  }
  writer.join();
  reader.join();
  CHECK(valid.load());
  for (const auto &pointer : retained)
    CHECK((*pointer >= 0 && *pointer < 1000));
  CHECK(scene.snapshot().revision() == 1);
  CHECK(scene.drain().delivered == 0);
}

TEST_CASE("user data readers retain old payload ownership across replacement and clearing") {
  sai::browser host{registry()};
  const auto owner = create(host.current_scene());
  const auto field = owner.get_field(measure::value).value();
  int destroyed = 0;
  REQUIRE(owner.set_user_data(sai::user_data_value::make<on_destroy>([&] {
    ++destroyed;
  })));
  auto retained = owner.user_data().value();
  auto pointer = retained.as<on_destroy>().value();
  REQUIRE(owner.set_user_data(sai::user_data_value::make<int>(2)));
  CHECK(destroyed == 0);
  retained = {};
  CHECK(destroyed == 0);
  pointer.reset();
  CHECK(destroyed == 1);

  REQUIRE(field.set_user_data(sai::user_data_value::make<on_destroy>([&] {
    ++destroyed;
  })));
  retained = field.user_data().value();
  REQUIRE(field.set_user_data({}));
  CHECK(destroyed == 1);
  retained = {};
  CHECK(destroyed == 2);
}

TEST_CASE("final context release destroys user data outside locks and allows unrelated reentry") {
  sai::browser unrelated{registry()};
  const auto other_scene = unrelated.current_scene();
  const auto other = create(other_scene);
  int destroyed = 0;
  sai::node expired;
  {
    sai::browser host{registry()};
    const auto scene = host.current_scene();
    const auto owner = create(scene);
    const auto field = owner.get_field("value").value();
    expired = owner.dynamic();
    const auto action = [&] {
      CHECK(unrelated.current_scene().generation() == other_scene.generation());
      CHECK(other_scene.snapshot().revision() == 1);
      REQUIRE(other.set_user_data(sai::user_data_value::make<int>(17)));
      CHECK(expired.user_data().error().code == sai::error_code::stale_handle);
      ++destroyed;
    };
    REQUIRE(owner.set_user_data(sai::user_data_value::make<on_destroy>(action)));
    REQUIRE(field.set_user_data(sai::user_data_value::make<on_destroy>(action)));
    CHECK(destroyed == 0);
  }
  CHECK(destroyed == 2);
  CHECK(*other.user_data()->as<int>().value() == 17);
}
