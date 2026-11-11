#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"
#include <array>
#include <cstdlib>
#include <memory>
#include <new>
#include <optional>
#include <string>

namespace {
thread_local int fail_after = -1;
}
void *operator new(std::size_t size) {
  if (fail_after >= 0 && fail_after-- == 0) {
    fail_after = -1;
    throw std::bad_alloc{};
  }
  if (void *p = std::malloc(size ? size : 1))
    return p;
  throw std::bad_alloc{};
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
namespace sai = x3d::sai::experimental;
namespace {
sai::type_registry registry() {
  sai::type_registry r;
  sai::node_type_descriptor type;
  type.name = "Group";
  const auto add = [&](std::string name, sai::value_kind kind,
                       sai::access_type access, sai::value initial,
                       bool containment = false) {
    sai::field_descriptor field;
    field.name = std::move(name);
    field.kind = kind;
    field.access = access;
    field.default_value = std::move(initial);
    field.containment = containment;
    type.fields.push_back(std::move(field));
  };
  add("children", sai::value_kind::mf_node, sai::access_type::input_output,
      sai::node_list{}, true);
  add("input", sai::value_kind::mf_node, sai::access_type::input_only,
      sai::node_list{});
  add("target", sai::value_kind::sf_node, sai::access_type::input_output,
      sai::node_id{});
  add("value", sai::value_kind::sf_int32, sai::access_type::input_output,
      std::int32_t{0});
  REQUIRE(r.define(std::move(type)));
  return r;
}
} // namespace

TEST_CASE("event containment validation rejects cycles without publication") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto author = scene.edit();
  auto a = author.create_node("Group").value();
  auto b = author.create_node("Group").value();
  REQUIRE(author.append_root(a));
  auto children = author.field(a, "children").value();
  auto others = author.field(b, "children").value();
  REQUIRE(author.commit());
  const auto before = scene.snapshot();
  int calls = 0;
  auto changes = scene.observe([&](const auto &) { ++calls; });
  auto fields = scene.observe(children, [&](const auto &) { ++calls; }).value();
  auto events = scene.events({1});
  SUBCASE("self cycle") { REQUIRE(events.send(children, std::array{a})); }
  SUBCASE("mutual cycle") {
    REQUIRE(events.send(children, std::array{b}));
    REQUIRE(events.send(others, std::array{a}));
  }
  auto rejected = events.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::containment_cycle);
  CHECK(scene.snapshot().revision() == before.revision());
  CHECK(scene.snapshot().read(children).value() ==
        sai::value{sai::node_list{}});
  CHECK(scene.drain().delivered == 0);
  CHECK(calls == 0);
  auto retry = scene.events({1});
  REQUIRE(retry.send(children, std::array{b, b}));
  REQUIRE(retry.commit());
  CHECK(scene.snapshot().occurrences().size() == 3);
}

TEST_CASE("node events cannot cross declaration scope even when transient") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto author = scene.edit();
  auto live = author.create_node("Group").value();
  auto body = author.create_node("Group").value();
  sai::local_declaration_descriptor declaration;
  declaration.name = "Template";
  REQUIRE(
      author.add_local_declaration(std::move(declaration), std::array{body}));
  REQUIRE(author.commit());
  auto events = scene.events({1});
  SUBCASE("SFNode into ordinary scene") {
    auto f = scene.snapshot().field(live, "target").value();
    const auto rejected = events.send(f, body);
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_context);
  }
  SUBCASE("MFNode into ordinary scene") {
    auto f = scene.snapshot().field(live, "children").value();
    const auto rejected = events.send(f, std::array{body});
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_context);
  }
  SUBCASE("transient MFNode into template") {
    auto f = scene.snapshot().field(body, "input").value();
    const auto rejected = events.send(f, std::array{live});
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::invalid_context);
  }
  REQUIRE_FALSE(events.commit());
  CHECK(scene.snapshot().revision() == 1);
}

TEST_CASE("allocation failure cannot partially publish edits or events") {
  bool reached_success = false;
  bool event_mode = false;
  SUBCASE("retained edit") { event_mode = false; }
  SUBCASE("event batch") { event_mode = true; }
  for (int allocation = 0; allocation < 1000 && !reached_success;
       ++allocation) {
    sai::browser host{registry()};
    auto scene = host.current_scene();
    auto author = scene.edit();
    auto node = author.create_node("Group").value();
    auto field = author.field(node, "value").value();
    REQUIRE(author.commit());
    int change_calls = 0, event_calls = 0;
    auto changes = scene.observe([&](const auto &) { ++change_calls; });
    auto fields =
        scene.observe(field, [&](const auto &) { ++event_calls; }).value();
    auto prior = scene.edit();
    REQUIRE(prior.set(field, std::int32_t{1}));
    REQUIRE(prior.set_metadata("title", std::string(120, 'a')));
    REQUIRE(prior.commit());
    auto edit = scene.edit();
    auto events = scene.events({1});
    if (event_mode)
      REQUIRE(events.send(field, std::int32_t{2}));
    else {
      REQUIRE(edit.set(field, std::int32_t{2}));
      REQUIRE(edit.set_metadata("title", std::string(140, 'b')));
    }
    bool threw = false;
    bool accepted = false;
    fail_after = allocation;
    try {
      accepted = event_mode ? bool(events.commit()) : bool(edit.commit());
    } catch (const std::bad_alloc &) {
      threw = true;
    }
    fail_after = -1;
    INFO("allocation index " << allocation << ", event mode " << event_mode);
    if (threw) {
      CHECK(scene.snapshot().metadata("title") == std::string(120, 'a'));
      CHECK(scene.snapshot().revision() == 2);
      CHECK(scene.snapshot().read(field).value() ==
            sai::value{std::int32_t{1}});
      CHECK(scene.drain().delivered == 1);
      CHECK(change_calls == 1);
      CHECK(event_calls == 0);
      if (event_mode) {
        auto retry = scene.events({1});
        REQUIRE(retry.send(field, std::int32_t{3}));
        REQUIRE(retry.commit());
      }
    } else {
      REQUIRE(accepted);
      reached_success = true;
      CHECK(scene.snapshot().metadata("title") ==
            (event_mode ? std::string(120, 'a') : std::string(140, 'b')));
      CHECK(scene.snapshot().revision() == 3);
      CHECK(scene.snapshot().read(field).value() ==
            sai::value{std::int32_t{2}});
      CHECK(scene.drain().delivered == (event_mode ? 3 : 2));
      CHECK(change_calls == 2);
      CHECK(event_calls == (event_mode ? 1 : 0));
    }
  }
  REQUIRE(reached_success);
}

TEST_CASE("allocation failure preserves staged scene metadata changes") {
  bool reached_success = false;
  int failures = 0;
  const auto key = std::string(80, 'k');
  const auto data = std::string(120, 'v');
  for (int allocation = 0; allocation < 1000 && !reached_success; ++allocation) {
    sai::browser host{registry()};
    auto scene = host.current_scene();
    auto edit = scene.edit();
    REQUIRE(edit.set_metadata("guard", "Already staged"));
    bool threw = false;
    bool accepted = false;
    fail_after = allocation;
    try {
      accepted = bool(edit.set_metadata(key, data));
    } catch (const std::bad_alloc &) {
      threw = true;
    }
    fail_after = -1;
    INFO("metadata allocation index " << allocation);
    CHECK(scene.snapshot().metadata().empty());
    const auto committed = edit.commit();
    REQUIRE(committed);
    CHECK(scene.snapshot().metadata("guard") == "Already staged");
    if (threw) {
      ++failures;
      CHECK_FALSE(scene.snapshot().metadata(key));
      REQUIRE(committed->changes.size() == 1);
      CHECK(std::get<sai::metadata_change>(committed->changes.front()).key == "guard");
    } else {
      REQUIRE(accepted);
      reached_success = true;
      CHECK(scene.snapshot().metadata(key) == data);
      REQUIRE(committed->changes.size() == 2);
      const auto &change = std::get<sai::metadata_change>(committed->changes.back());
      CHECK(change.key == key);
      CHECK_FALSE(change.before);
      CHECK(change.after == data);
    }
  }
  CHECK(failures > 0);
  REQUIRE(reached_success);
}

TEST_CASE(
    "allocation failure preserves committed import retention atomically") {
  bool reached_success = false;
  for (int allocation = 0; allocation < 1000 && !reached_success;
       ++allocation) {
    sai::browser host{registry()};
    auto importer = host.current_scene();
    auto source = host.create_scene();
    auto create = source.edit();
    auto old_node = create.create_node("Group").value();
    auto new_node = create.create_node("Group").value();
    REQUIRE(create.export_node("Old", old_node));
    REQUIRE(create.export_node("New", new_node));
    REQUIRE(create.commit());
    auto initial = importer.edit();
    REQUIRE(initial.import_node("Local", source, "Old"));
    REQUIRE(initial.commit());
    auto replace = importer.edit();
    REQUIRE(replace.remove_import(importer.snapshot().imports().front()));
    REQUIRE(replace.import_node("Local", source, "New"));
    bool threw = false, accepted = false;
    fail_after = allocation;
    try {
      accepted = bool(replace.commit());
    } catch (const std::bad_alloc &) {
      threw = true;
    }
    fail_after = -1;
    INFO("allocation index " << allocation);
    const auto imports = importer.snapshot().imports();
    REQUIRE(imports.size() == 1);
    if (threw) {
      CHECK(imports.front().target.local == old_node.id());
      auto retained = source.edit();
      REQUIRE(retained.remove_export(source.snapshot().exports()[0]));
      CHECK_FALSE(retained.remove_node(old_node));
      auto unretained = source.edit();
      REQUIRE(unretained.remove_export(source.snapshot().exports()[1]));
      CHECK(unretained.remove_node(new_node));
    } else {
      REQUIRE(accepted);
      reached_success = true;
      CHECK(imports.front().target.local == new_node.id());
      auto retained = source.edit();
      REQUIRE(retained.remove_export(source.snapshot().exports()[1]));
      CHECK_FALSE(retained.remove_node(new_node));
      auto unretained = source.edit();
      REQUIRE(unretained.remove_export(source.snapshot().exports()[0]));
      CHECK(unretained.remove_node(old_node));
    }
  }
  REQUIRE(reached_success);
}

TEST_CASE(
    "routed containment cycles are rejected but reference cycles are valid") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto author = scene.edit();
  auto source = author.create_node("Group").value();
  auto sink = author.create_node("Group").value();
  bool containment = true;
  SUBCASE("a valid seed becomes a cycle at the routed sink") {
    containment = true;
  }
  SUBCASE("noncontainment references may form a cycle") { containment = false; }
  auto source_field =
      author.field(source, containment ? "children" : "target").value();
  auto sink_field =
      author.field(sink, containment ? "children" : "target").value();
  if (containment)
    REQUIRE(author.add_route(source_field, sink_field));
  REQUIRE(author.commit());
  auto events = scene.events({1});
  if (containment) {
    REQUIRE(events.send(source_field, std::array{sink}));
    auto rejected = events.commit();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::containment_cycle);
    CHECK(scene.snapshot().revision() == 1);
  } else {
    REQUIRE(events.send(source_field, sink));
    REQUIRE(events.send(sink_field, source));
    REQUIRE(events.commit());
    CHECK(scene.snapshot().revision() == 2);
  }
}

TEST_CASE("failed checkpoint publication releases host state outside locks") {
  struct cleanup_probe {
    const sai::execution_context *scene;
    sai::revision_id expected_revision = 1;
    int destroyed = 0;
  };
  struct checkpoint {
    std::unique_ptr<int> resource = std::make_unique<int>(7);
    std::weak_ptr<cleanup_probe> probe;
    explicit checkpoint(std::weak_ptr<cleanup_probe> owner)
        : probe(std::move(owner)) {}
    checkpoint(checkpoint &&) noexcept = default;
    ~checkpoint() noexcept {
      if (resource) {
        if (auto owner = probe.lock()) {
          CHECK(owner->scene->snapshot().revision() ==
                owner->expected_revision);
          ++owner->destroyed;
        }
      }
    }
  };
  struct frame {
    std::weak_ptr<cleanup_probe> probe;
    int allocation;
    sai::result<void> process(sai::runtime_step &) { return {}; }
    sai::result<checkpoint> finish() {
      checkpoint state{probe};
      fail_after = allocation;
      return state;
    }
  };
  bool reached_success = false;
  for (int allocation = 0; allocation < 1000 && !reached_success;
       ++allocation) {
    sai::browser host{registry()};
    auto scene = host.current_scene();
    auto author = scene.edit();
    REQUIRE(author.create_node("Group"));
    REQUIRE(author.commit());
    auto probe = std::make_shared<cleanup_probe>(cleanup_probe{&scene});
    int observations = 0;
    auto observer = scene.observe([&](const auto &) { ++observations; });
    REQUIRE(host.set_runtime_factory(
        scene,
        [weak = std::weak_ptr{probe},
         allocation](const sai::scene_snapshot &,
                     sai::event_time) -> sai::result<sai::runtime_frame> {
          return sai::runtime_frame{frame{weak, allocation}};
        }));
    bool accepted = false;
    try {
      accepted = bool(scene.events({1}).commit());
    } catch (const std::bad_alloc &) {
    }
    fail_after = -1;
    INFO("checkpoint allocation index " << allocation);
    if (!accepted) {
      CHECK(scene.snapshot().revision() == 1);
      CHECK_FALSE(scene.snapshot().runtime_state<checkpoint>().value());
      CHECK(scene.drain().delivered == 0);
      CHECK(observations == 0);
      CHECK(probe->destroyed == 1);
      REQUIRE(host.set_runtime_processor(
          scene, [](sai::runtime_step &) -> sai::result<void> { return {}; }));
      REQUIRE(scene.events({1}).commit());
    } else {
      reached_success = true;
      CHECK(scene.snapshot().revision() == 2);
      CHECK(*scene.snapshot().runtime_state<checkpoint>().value()->resource ==
            7);
      CHECK(scene.drain().delivered == 1);
      CHECK(observations == 1);
      observer.cancel();
      REQUIRE(host.set_runtime_processor(
          scene, [](sai::runtime_step &) -> sai::result<void> { return {}; }));
      probe->expected_revision = 3;
      REQUIRE(scene.events({2}).commit());
      CHECK(probe->destroyed == 1);
    }
  }
  REQUIRE(reached_success);
}

TEST_CASE("declaration update allocation failure preserves staged payload and "
          "journal") {
  int failures = 0;
  int successes = 0;
  for (int budget = 0; budget != 32; ++budget) {
    sai::browser host{registry()};
    auto scene = host.current_scene();
    auto setup = scene.edit();
    sai::local_declaration_descriptor original;
    original.name = "Declaration_with_an_owning_long_name";
    original.appinfo =
        "Original application information outside small string storage";
    auto declaration = setup.add_local_declaration(original).value();
    REQUIRE(setup.commit());
    const auto before = scene.snapshot();
    auto replacement = original;
    replacement.appinfo =
        "Replacement application information outside small string storage";
    auto edit = scene.edit();
    REQUIRE(edit.set_metadata("pending", "preserved"));
    bool threw = false;
    fail_after = budget;
    try {
      (void)edit.update_declaration(declaration, std::move(replacement));
    } catch (const std::bad_alloc &) {
      threw = true;
    }
    fail_after = -1;
    const auto committed = edit.commit();
    REQUIRE(committed);
    const auto after = scene.snapshot();
    const auto description = after.describe(declaration).value();
    const auto &actual =
        std::get<sai::local_declaration_descriptor>(description.payload);
    if (threw) {
      ++failures;
      CHECK(after.revision() == before.revision() + 1);
      CHECK(actual.appinfo == original.appinfo);
      CHECK(committed->changes.size() == 1);
      CHECK(after.metadata("pending").value() == "preserved");
    } else {
      ++successes;
      CHECK(after.revision() == before.revision() + 1);
      CHECK(actual.appinfo ==
            "Replacement application information outside small string storage");
      REQUIRE(committed->changes.size() == 2);
      const auto *change =
          sai::declaration_change_of(committed->changes.back());
      REQUIRE(change);
      REQUIRE(change->before);
      REQUIRE(change->after);
      CHECK(std::get<sai::local_declaration_descriptor>(change->before->payload)
                .appinfo == original.appinfo);
      CHECK(std::get<sai::local_declaration_descriptor>(change->after->payload)
                .appinfo == actual.appinfo);
    }
  }
  CHECK(failures > 0);
  CHECK(successes > 0);
}
