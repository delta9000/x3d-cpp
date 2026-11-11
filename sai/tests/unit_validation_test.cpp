#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <array>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
enum class write_path { authoring, event_input, runtime_output };

sai::type_registry unit_registry() {
  sai::type_registry registry;
  const auto define = [&](const char *name,
                          std::initializer_list<std::pair<const char *,
                                                          sai::access_type>> fields) {
    sai::node_type_descriptor type;
    type.name = name;
    for (const auto &[field_name, access] : fields) {
      sai::field_descriptor field;
      field.name = field_name;
      field.kind = sai::value_kind::sf_float;
      field.access = access;
      field.default_value = 0.f;
      field.unit_category = "length";
      type.fields.push_back(std::move(field));
    }
    REQUIRE(registry.define(std::move(type)));
  };
  define("Measure", {{"value", sai::access_type::input_output},
                      {"guard", sai::access_type::input_output},
                      {"initial", sai::access_type::initialize_only},
                      {"input", sai::access_type::input_only},
                      {"output", sai::access_type::output_only}});
  define("Other", {{"other", sai::access_type::input_output}});
  return registry;
}

void check_validation_priority(write_path path) {
  sai::browser host{unit_registry()};
  auto scene = host.current_scene();
  auto setup = scene.edit();
  REQUIRE(setup.declare_unit(
      {.category = "length", .name = "double_metre", .conversion_factor = 2.0}));
  const auto local = setup.create_node("Measure").value();
  const auto removed = setup.create_node("Measure").value();
  const auto templated = setup.create_node("Measure").value();
  const auto value = setup.field(local, "value").value();
  const auto guard = setup.field(local, "guard").value();
  const auto initial = setup.field(local, "initial").value();
  const auto input = setup.field(local, "input").value();
  const auto output = setup.field(local, "output").value();
  const auto stale = setup.field(removed, "value").value();
  const auto scoped = setup.field(templated, "value").value();
  REQUIRE(setup.remove_node(removed));
  REQUIRE(setup.append_root(local));
  sai::local_declaration_descriptor declaration;
  declaration.name = "Template";
  REQUIRE(setup.add_local_declaration(std::move(declaration),
                                      std::array{templated}));
  REQUIRE(setup.commit());

  auto foreign_scene = host.create_scene();
  auto foreign_edit = foreign_scene.edit();
  const auto foreign_node = foreign_edit.create_node("Measure").value();
  const auto foreign = foreign_edit.field(foreign_node, "value").value();
  REQUIRE(foreign_edit.commit());
  auto other_scene = host.create_scene();
  auto other_edit = other_scene.edit();
  const auto other_node = other_edit.create_node("Other").value();
  const auto other = other_edit.field(other_node, "other").value();
  REQUIRE(other_edit.commit());
  // Foreign IDs deliberately alias a live local record; authority must be
  // checked before its descriptor can be used for conversion.
  REQUIRE(foreign_node.id() == local.id());
  REQUIRE(other_node.id() == local.id());

  int changes = 0;
  int deliveries = 0;
  auto change_observer = scene.observe([&](const sai::change_set &) { ++changes; });
  auto field_observer = scene.observe(
      guard, [&](const sai::event_delivery &) { ++deliveries; });
  REQUIRE(field_observer);
  auto positive = scene.events({1});
  REQUIRE(positive.send(guard, sai::value{9.f}));
  REQUIRE(positive.commit());
  CHECK(scene.drain().delivered == 2);
  CHECK(changes == 1);
  CHECK(deliveries == 1);
  changes = 0;
  deliveries = 0;
  const auto before = scene.snapshot().revision();

  using code = sai::error_code;
  struct validation_case {
    const char *name;
    sai::dynamic_field field;
    std::array<code, 3> errors;
  };
  const std::vector<validation_case> cases{
      {"foreign descriptor alias", foreign,
       {code::stale_handle, code::stale_handle, code::stale_handle}},
      {"foreign unknown field alias", other,
       {code::stale_handle, code::stale_handle, code::stale_handle}},
      {"removed node", stale,
       {code::stale_handle, code::stale_handle, code::stale_handle}},
      {"realized initializeOnly", initial,
       {code::access_denied, code::access_denied, code::access_denied}},
      {"outputOnly", output,
       {code::access_denied, code::access_denied, code::type_mismatch}},
      {"inputOnly", input,
       {code::access_denied, code::type_mismatch, code::access_denied}},
      {"template scope", scoped,
       {code::type_mismatch, code::invalid_context, code::invalid_context}},
      {"valid destination with wrong representation", value,
       {code::type_mismatch, code::type_mismatch, code::type_mismatch}},
  };
  const auto index = static_cast<std::size_t>(path);
  const char *operation = path == write_path::authoring ? "scene_edit.set"
                         : path == write_path::event_input ? "event_batch.send"
                         : "runtime_step.emit";
  for (const auto &item : cases) {
    CAPTURE(std::string{item.name});
    for (bool typed : {false, true}) {
      // A well-typed overflowing authored value probes target admission. For
      // a valid destination that overflow is a legitimate conversion failure.
      if (typed && item.errors[index] == code::type_mismatch)
        continue;
      CAPTURE(typed);
      std::optional<sai::sai_error> canonical;
      for (const auto space : {sai::value_space::canonical,
                               sai::value_space::authored}) {
        CAPTURE(space);
        const auto submit = [&](auto invoke) -> sai::result<void> {
          if (typed)
            return invoke(item.field.as<float>().value(),
                          std::numeric_limits<float>::max(), space);
          return invoke(item.field,
                        sai::value{std::string{"wrong representation"}}, space);
        };
        std::optional<sai::sai_error> rejected;
        const auto check_rejection = [&](const sai::result<void> &submitted) {
          REQUIRE_FALSE(submitted);
          rejected = submitted.error();
          CHECK(rejected->code == item.errors[index]);
          CHECK(rejected->operation == operation);
          CHECK(rejected->generation == scene.generation());
          CHECK(rejected->base_revision == before);
          CHECK(rejected->node == item.field.node());
          CHECK(rejected->field == item.field.name());
        };
        if (path == write_path::authoring) {
          auto edit = scene.edit();
          REQUIRE(edit.set(guard, sai::value{1.f}));
          check_rejection(submit([&](const auto &field, const auto &payload,
                                     sai::value_space representation) {
            return edit.set(field, payload, representation);
          }));
          const auto admitted = edit.validate();
          REQUIRE_FALSE(admitted);
          CHECK(admitted.error().code == rejected->code);
          const auto committed = edit.commit();
          REQUIRE_FALSE(committed);
          CHECK(committed.error().code == rejected->code);
        } else if (path == write_path::event_input) {
          auto batch = scene.events({2});
          REQUIRE(batch.send(guard, sai::value{1.f}));
          check_rejection(submit([&](const auto &field, const auto &payload,
                                     sai::value_space representation) {
            return batch.send(field, payload, representation);
          }));
          const auto committed = batch.commit();
          REQUIRE_FALSE(committed);
          CHECK(committed.error().code == rejected->code);
        } else {
          REQUIRE(host.set_runtime_processor(
              scene, [&](sai::runtime_step &step) -> sai::result<void> {
                REQUIRE(step.emit(guard, sai::value{1.f}));
                check_rejection(submit([&](const auto &field, const auto &payload,
                                           sai::value_space representation) {
                  return step.emit(field, payload, representation);
                }));
                return {};
              }));
          const auto committed = scene.events({2}).commit();
          REQUIRE_FALSE(committed);
          REQUIRE(rejected);
          CHECK(committed.error().code == rejected->code);
          REQUIRE(host.set_runtime_processor(scene, {}));
        }
        REQUIRE(rejected);
        if (space == sai::value_space::canonical)
          canonical = rejected;
        else
          CHECK(rejected->message == canonical->message);
        CHECK(scene.snapshot().revision() == before);
        CHECK(scene.snapshot().read(guard).value() == sai::value{9.f});
        CHECK(scene.drain().delivered == 0);
        CHECK(changes == 0);
        CHECK(deliveries == 0);
      }
    }
  }
  // Rejections leave the timestamp and live mutation reservation reusable.
  auto retry = scene.events({2});
  REQUIRE(retry.send(guard, sai::value{2.f}, sai::value_space::authored));
  REQUIRE(retry.commit());
  CHECK(scene.snapshot().read(guard).value() == sai::value{4.f});
  CHECK(scene.drain().delivered == 2);
  CHECK(changes == 1);
  CHECK(deliveries == 1);
}
} // namespace

TEST_CASE("unit representation preserves validation priority in authored edits") {
  check_validation_priority(write_path::authoring);
}
TEST_CASE("unit representation preserves validation priority in event batches") {
  check_validation_priority(write_path::event_input);
}
TEST_CASE("unit representation preserves validation priority in runtime outputs") {
  check_validation_priority(write_path::runtime_output);
}
