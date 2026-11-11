#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor type;
  type.name = "Measure";
  sai::field_descriptor field;
  field.name = "value";
  field.kind = sai::value_kind::sf_int32;
  field.access = sai::access_type::input_output;
  field.default_value = std::int32_t{0};
  type.fields.push_back(std::move(field));
  REQUIRE(types.define(std::move(type)));
  return types;
}
} // namespace

TEST_CASE("scene metadata preserves values and ordered discovery") {
  std::optional<std::string> retained_title;
  {
    sai::browser host{registry()};
    auto scene = host.current_scene();
    const auto empty = scene.snapshot();
    CHECK(empty.metadata().empty());
    CHECK_FALSE(empty.metadata("title"));
    auto edit = scene.edit();
    REQUIRE(edit.set_metadata("title", "First world"));
    REQUIRE(edit.set_metadata("creator", "Ben"));
    REQUIRE(edit.set_metadata("empty", ""));
    REQUIRE(edit.set_metadata("title", "Second world"));
    REQUIRE(edit.set_metadata("Title", "Case matters"));
    REQUIRE(edit.set_metadata("", "Empty key"));
    REQUIRE(edit.set_metadata("description", "世界 — café"));
    REQUIRE(edit.validate());
    CHECK(scene.snapshot().metadata().empty());
    const auto published = edit.commit();
    REQUIRE(published);
    REQUIRE(published->changes.size() == 7);
    const auto &updated = std::get<sai::metadata_change>(published->changes[3]);
    CHECK(updated.key == "title");
    CHECK(updated.before == "First world");
    CHECK(updated.after == "Second world");
    CHECK(sai::change_kind_of(published->changes[3]) ==
          sai::semantic_change_kind{sai::metadata_change_kind::updated});
    CHECK(sai::node_change(published->changes[3]) == nullptr);
    const auto &added = std::get<sai::metadata_change>(published->changes[2]);
    CHECK_FALSE(added.before);
    REQUIRE(added.after);
    CHECK(added.after->empty());
    const auto before = scene.snapshot();
    CHECK(before.metadata() == std::vector<sai::metadata_entry>{
        {"title", "Second world"}, {"creator", "Ben"}, {"empty", ""},
        {"Title", "Case matters"}, {"", "Empty key"},
        {"description", "世界 — café"}});
    CHECK(before.metadata("title") == "Second world");
    REQUIRE(before.metadata("empty"));
    CHECK(before.metadata("empty")->empty());
    CHECK(before.metadata("") == "Empty key");
    CHECK_FALSE(before.metadata("TITLE"));
    CHECK_FALSE(empty.metadata("title"));
    retained_title = before.metadata("title");

    auto remove = scene.edit();
    REQUIRE(remove.set_metadata("title", std::nullopt));
    REQUIRE(remove.set_metadata("missing", std::nullopt));
    const auto removed = remove.commit();
    REQUIRE(removed);
    REQUIRE(removed->changes.size() == 1);
    const auto &change = std::get<sai::metadata_change>(removed->changes.front());
    CHECK(change.key == "title");
    CHECK(change.before == "Second world");
    CHECK_FALSE(change.after);
    CHECK_FALSE(scene.snapshot().metadata("title"));
    CHECK(before.metadata("title") == "Second world");
    auto reinsert = scene.edit();
    REQUIRE(reinsert.set_metadata("title", "Third world"));
    REQUIRE(reinsert.commit());
    const auto entries = scene.snapshot().metadata();
    REQUIRE(entries.size() == 6);
    CHECK(entries.front().key == "creator");
    CHECK(entries.back() == sai::metadata_entry{"title", "Third world"});
  }
  CHECK(retained_title == "Second world");
}

TEST_CASE("scene metadata shares graph publication and queued observation") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("Measure").value();
  const auto value = setup.field(node, "value").value();
  REQUIRE(setup.append_root(node));
  REQUIRE(setup.commit());
  int notifications = 0;
  int field_events = 0;
  std::optional<sai::change_set> observed;
  auto changes = scene.observe([&](const sai::change_set &change) {
    ++notifications;
    observed = change;
  });
  auto fields = scene.observe(value, [&](const auto &) { ++field_events; }).value();
  auto positive = scene.events({1});
  REQUIRE(positive.send(value, sai::value{std::int32_t{1}}));
  REQUIRE(positive.commit());
  CHECK(scene.drain().delivered == 2);
  CHECK(notifications == 1);
  CHECK(field_events == 1);
  notifications = 0;
  field_events = 0;
  observed.reset();
  const auto before = scene.snapshot();
  auto edit = scene.edit();
  REQUIRE(edit.set_metadata("title", "Mixed revision"));
  REQUIRE(edit.set(value, sai::value{std::int32_t{7}}));
  REQUIRE(edit.set_metadata("creator", "An editor"));
  REQUIRE(edit.validate());
  CHECK_FALSE(scene.snapshot().metadata("title"));
  CHECK(scene.snapshot().read(value).value() == sai::value{std::int32_t{1}});
  const auto committed = edit.commit();
  REQUIRE(committed);
  CHECK(committed->before_revision == before.revision());
  CHECK(committed->after_revision == before.revision() + 1);
  REQUIRE(committed->changes.size() == 3);
  CHECK(std::get<sai::metadata_change>(committed->changes[0]).key == "title");
  REQUIRE(sai::node_change(committed->changes[1]));
  CHECK(sai::node_change(committed->changes[1])->field == "value");
  CHECK(std::get<sai::metadata_change>(committed->changes[2]).key == "creator");
  CHECK(notifications == 0);
  CHECK(scene.snapshot().metadata("title") == "Mixed revision");
  CHECK(scene.snapshot().read(value).value() == sai::value{std::int32_t{7}});
  CHECK(scene.drain().delivered == 1);
  CHECK(notifications == 1);
  CHECK(field_events == 0);
  REQUIRE(observed);
  CHECK(observed->after_revision == committed->after_revision);
  REQUIRE(observed->changes.size() == 3);
  CHECK(std::get<sai::metadata_change>(observed->changes[0]).after == "Mixed revision");
  CHECK_FALSE(before.metadata("title"));
  CHECK(before.read(value).value() == sai::value{std::int32_t{1}});

  auto metadata_only = scene.edit();
  REQUIRE(metadata_only.set_metadata("title", "Metadata revision"));
  REQUIRE(metadata_only.commit());
  CHECK(scene.snapshot().is_realized(node).value());
  CHECK(scene.snapshot().read(value).value() == sai::value{std::int32_t{7}});
  CHECK(scene.drain().delivered == 1);
  CHECK(field_events == 0);
  const auto revision = scene.snapshot().revision();
  auto unchanged = scene.edit();
  REQUIRE(unchanged.set_metadata("title", "Metadata revision"));
  REQUIRE(unchanged.set_metadata("absent", std::nullopt));
  const auto no_op = unchanged.commit();
  REQUIRE(no_op);
  CHECK(no_op->before_revision == revision);
  CHECK(no_op->after_revision == revision);
  CHECK(no_op->changes.empty());
  CHECK(scene.drain().delivered == 0);
  CHECK(notifications == 2);
}

TEST_CASE("scene metadata rejects conflicting poisoned and retired edits") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto node = setup.create_node("Measure").value();
  const auto value = setup.field(node, "value").value();
  REQUIRE(setup.set_metadata("title", "Published world"));
  REQUIRE(setup.commit());
  const auto before = scene.snapshot();
  auto edit = scene.edit();
  REQUIRE(edit.set_metadata("title", "Unpublished world"));
  REQUIRE(edit.set(value, sai::value{std::int32_t{8}}));
  int notifications = 0;
  auto observer = scene.observe([&](const auto &) { ++notifications; });
  auto expected_title = std::string{"Published world"};
  auto expected_revision = before.revision();
  SUBCASE("discard") { REQUIRE(edit.validate()); }
  SUBCASE("poison") {
    const auto wrong = edit.set(value, sai::value{std::string{"wrong kind"}});
    REQUIRE_FALSE(wrong);
    CHECK(wrong.error().code == sai::error_code::type_mismatch);
    const auto metadata = edit.set_metadata("creator", "Must not publish");
    REQUIRE_FALSE(metadata);
    CHECK(metadata.error().operation == wrong.error().operation);
    CHECK_FALSE(edit.validate());
    CHECK_FALSE(edit.commit());
  }
  SUBCASE("conflict") {
    REQUIRE(edit.validate());
    auto winner = scene.edit();
    REQUIRE(winner.set_metadata("title", "Concurrent world"));
    REQUIRE(winner.commit());
    CHECK(scene.drain().delivered == 1);
    notifications = 0;
    expected_title = "Concurrent world";
    ++expected_revision;
    const auto rejected = edit.commit();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::stale_revision);
    CHECK(rejected.error().base_revision == before.revision());
  }
  SUBCASE("world retirement") {
    REQUIRE(host.replace_world(host.create_scene()));
    const auto rejected = edit.commit();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::stale_handle);
  }
  CHECK(scene.snapshot().metadata("title") == expected_title);
  CHECK_FALSE(scene.snapshot().metadata("creator"));
  CHECK(scene.snapshot().read(value).value() == sai::value{std::int32_t{0}});
  CHECK(scene.snapshot().revision() == expected_revision);
  CHECK(before.metadata("title") == "Published world");
  CHECK(scene.drain().delivered == 0);
  CHECK(notifications == 0);
}

TEST_CASE("scene metadata survives runtime frames and world replacement") {
  sai::browser host{registry()};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto node = edit.create_node("Measure").value();
  const auto value = edit.field(node, "value").value();
  REQUIRE(edit.set_metadata("value", "A document string"));
  REQUIRE(edit.commit());
  const auto history = scene.snapshot();
  REQUIRE(host.set_runtime_processor(scene, [&](sai::runtime_step &step) {
    return step.emit(value, sai::value{std::int32_t{9}});
  }));
  REQUIRE(scene.events({1}).commit());
  CHECK(scene.snapshot().read(value).value() == sai::value{std::int32_t{9}});
  CHECK(scene.snapshot().metadata("value") == "A document string");
  CHECK(history.read(value).value() == sai::value{std::int32_t{0}});
  CHECK(history.metadata("value") == "A document string");
  auto replacement = host.create_scene();
  auto author = replacement.edit();
  REQUIRE(author.set_metadata("value", "A different world"));
  REQUIRE(author.commit());
  REQUIRE(host.replace_world(replacement));
  CHECK(host.current_scene().snapshot().metadata("value") == "A different world");
  CHECK(history.metadata("value") == "A document string");
  CHECK(scene.snapshot().metadata("value") == "A document string");
}
