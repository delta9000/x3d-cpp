#include "doctest/doctest.h"
#include "x3d/sai/experimental/kernel.hpp"
#include <array>
#include <cstdlib>
#include <new>
#include <optional>

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
  REQUIRE(r.define({.name = "Group",
                    .fields = {{.name = "children",
                                .kind = sai::value_kind::mf_node,
                                .access = sai::access_type::input_output,
                                .default_value = sai::node_list{},
                                .containment = true},
                               {.name = "input",
                                .kind = sai::value_kind::mf_node,
                                .access = sai::access_type::input_only,
                                .default_value = sai::node_list{}},
                               {.name = "target",
                                .kind = sai::value_kind::sf_node,
                                .access = sai::access_type::input_output,
                                .default_value = sai::node_id{}},
                               {.name = "value",
                                .kind = sai::value_kind::sf_int32,
                                .access = sai::access_type::input_output,
                                .default_value = std::int32_t{0}}}}));
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
  REQUIRE(author.add_local_declaration({.name = "Template"}, std::array{body}));
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
    REQUIRE(prior.commit());
    auto edit = scene.edit();
    auto events = scene.events({1});
    if (event_mode)
      REQUIRE(events.send(field, std::int32_t{2}));
    else
      REQUIRE(edit.set(field, std::int32_t{2}));
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
