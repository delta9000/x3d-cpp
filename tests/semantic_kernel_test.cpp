#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DSemanticMetadataRegistry.hpp"

#include "doctest/doctest.h"

#include <array>
#include <concepts>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

namespace sai = x3d::sai::experimental;

namespace {

struct copy_bomb_observer {
  std::shared_ptr<bool> armed;
  int *calls = nullptr;

  copy_bomb_observer(std::shared_ptr<bool> armed_value, int &call_count)
      : armed(std::move(armed_value)), calls(&call_count) {}
  copy_bomb_observer(const copy_bomb_observer &other)
      : armed(other.armed), calls(other.calls) {
    if (*armed)
      throw std::runtime_error("callback copied during dispatch");
  }
  copy_bomb_observer(copy_bomb_observer &&) noexcept = default;
  void operator()(const sai::change_set &) const { ++*calls; }
};

template <class Edit>
concept accepts_imported_containment = requires(
    Edit &edit, const sai::node &parent, const sai::imported_node &child) {
  edit.append(parent, "children", child);
};

template <class Edit>
concept accepts_imported_retained_write =
    requires(Edit &edit, const sai::dynamic_field &target,
             const sai::imported_node &value) { edit.set(target, value); };

template <class Batch>
concept accepts_imported_event_target =
    requires(Batch &batch, const sai::dynamic_imported_field &target) {
      batch.send(target, sai::value{sai::vec3f{}});
    };

sai::field_descriptor test_field(std::string name, sai::value_kind kind,
                                 sai::access_type access,
                                 sai::value default_value,
                                 bool containment = false) {
  return sai::field_descriptor{
      .name = std::move(name),
      .kind = kind,
      .access = access,
      .default_value = std::move(default_value),
      .default_origin = sai::default_source::field_type,
      .containment = containment,
      .accepted_node_types = {},
      .unit_category = std::nullopt,
  };
}

sai::node_type_descriptor
test_node_type(std::string name, std::vector<sai::field_descriptor> fields) {
  return sai::node_type_descriptor{
      .name = std::move(name),
      .fields = std::move(fields),
      .component = {},
      .component_level = 0,
      .interfaces = {},
      .abstract = false,
  };
}

sai::type_registry graph_registry() {
  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type(
      "Group", {test_field("children", sai::value_kind::node_list,
                            sai::access_type::input_output, sai::node_list{},
                            true)})));
  REQUIRE(registry.define(test_node_type(
      "Transform",
      {
          test_field("translation", sai::value_kind::vec3f,
                     sai::access_type::input_output, sai::vec3f{}),
          test_field("children", sai::value_kind::node_list,
                     sai::access_type::input_output, sai::node_list{}, true),
          test_field("worldTranslation", sai::value_kind::vec3f,
                     sai::access_type::output_only, sai::vec3f{}),
          test_field("initialLabel", sai::value_kind::string,
                     sai::access_type::initialize_only, std::string{}),
          test_field("set_translation", sai::value_kind::vec3f,
                     sai::access_type::input_only, sai::vec3f{}),
      })));
  REQUIRE(registry.define(test_node_type(
      "AccessNode",
      {
          test_field("initialChildren", sai::value_kind::node_list,
                     sai::access_type::initialize_only, sai::node_list{}, true),
          test_field("receivedChildren", sai::value_kind::node_list,
                     sai::access_type::input_only, sai::node_list{}, true),
          test_field("observedChildren", sai::value_kind::node_list,
                     sai::access_type::output_only, sai::node_list{}, true),
      })));
  REQUIRE(registry.define(test_node_type(
      "Link",
      {
          test_field("target", sai::value_kind::node,
                     sai::access_type::input_output, sai::node_id{}),
          test_field("targets", sai::value_kind::node_list,
                     sai::access_type::input_output, sai::node_list{}),
      })));
  return registry;
}

} // namespace

TEST_CASE("experimental SAI starts as an empty descriptor-governed context") {
  sai::type_registry registry;
  auto defined = registry.define(test_node_type(
      "Group", {test_field("children", sai::value_kind::node_list,
                            sai::access_type::input_output, sai::node_list{},
                            true)}));
  REQUIRE(defined);

  auto duplicate = registry.define(test_node_type("Group", {}));
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::duplicate_type);
  CHECK(duplicate.error().operation == "type_registry.define");

  sai::browser host{std::move(registry)};
  sai::execution_context context = host.current_scene();
  sai::scene_snapshot snapshot = context.snapshot();

  CHECK(context.generation() == 1);
  CHECK(snapshot.revision() == 0);
  CHECK(snapshot.roots().empty());
  CHECK(host.capabilities().authoring);
  CHECK(host.capabilities().inspection);
  CHECK_FALSE(host.capabilities().live);
  CHECK_FALSE(host.capabilities().async_loading);
  CHECK_FALSE(host.capabilities().rendering);
}

TEST_CASE("generated metadata authors without runtime node identity") {
  const std::array<std::string_view, 1> selected{"Box"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);

  sai::browser host{std::move(registry).value()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto box = edit.create_node("Box");
  REQUIRE(box);
  REQUIRE(edit.append_root(box.value()));
  REQUIRE(edit.commit());

  const auto described = context.snapshot().describe(box.value());
  REQUIRE(described);
  CHECK(described.value().name == "Box");
  CHECK(described.value().component == "Geometry3D");
  CHECK(described.value().component_level == 1);
  REQUIRE(described.value().fields.size() == 9);
  CHECK(described.value().fields[0].name == "IS");
  CHECK(described.value().fields[0].accepted_node_types ==
        std::vector<std::string>{"IS"});
  CHECK_FALSE(described.value().fields[0].unit_category.has_value());
  CHECK(described.value().fields[0].default_origin ==
        sai::default_source::schema);
  CHECK(described.value().fields[2].name == "size");
  CHECK(described.value().fields[2].default_origin ==
        sai::default_source::schema);
  CHECK(described.value().fields[2].default_value ==
        sai::value{sai::vec3f{2, 2, 2}});
  CHECK(described.value().fields[4].name == "DEF");
  CHECK(described.value().fields[4].default_origin ==
        sai::default_source::field_type);
}

TEST_CASE("generated metadata adapter has exhaustive ordered descriptor parity") {
  const auto source = x3d::nodes::X3DSemanticMetadataRegistry::nodes();
  const auto adapted = sai::generated_metadata_catalog();
  CHECK(adapted.specification_version == "4.0");
  CHECK(adapted.model_fingerprint ==
        x3d::nodes::X3DSemanticMetadataRegistry::modelFingerprint());
  CHECK(adapted.model_fingerprint.size() == 64);
  CHECK(adapted.generator_version ==
        x3d::nodes::X3DSemanticMetadataRegistry::generatorVersion());
  CHECK_FALSE(adapted.generator_version.empty());
  CHECK_FALSE(adapted.unit_categories_complete);
  REQUIRE(adapted.nodes.size() == source.size());

  for (std::size_t node_index = 0; node_index < source.size(); ++node_index) {
    const auto &source_node = source[node_index];
    const auto &adapted_node = adapted.nodes[node_index];
    CHECK(adapted_node.name == source_node.name);
    CHECK(adapted_node.abstract == source_node.abstract);
    CHECK(adapted_node.component == source_node.component);
    CHECK(adapted_node.component_level == source_node.level);
    CHECK(adapted_node.interfaces == source_node.interfaces);
    REQUIRE(adapted_node.fields.size() == source_node.fields.size());

    for (std::size_t field_index = 0; field_index < source_node.fields.size();
         ++field_index) {
      const auto &source_field = source_node.fields[field_index];
      const auto &adapted_field = adapted_node.fields[field_index];
      CHECK(adapted_field.name == source_field.name);
      CHECK(static_cast<int>(adapted_field.type) ==
            static_cast<int>(source_field.type));
      CHECK(static_cast<int>(adapted_field.access) ==
            static_cast<int>(source_field.access));
      CHECK(adapted_field.declared_default == source_field.defaultValue);
      CHECK(adapted_field.accepted_node_types ==
            source_field.acceptableNodeTypes);
      CHECK(adapted_field.unit_category == source_field.unitCategory);
    }

    if (!source_node.abstract) {
      const auto live = x3d::nodes::X3DNodeFactory::create(source_node.name);
      REQUIRE(live);
      const auto &reflected = live->fields();
      REQUIRE(reflected.size() == source_node.fields.size());
      for (std::size_t field_index = 0; field_index < reflected.size();
           ++field_index) {
        CHECK(reflected[field_index].x3dName ==
              source_node.fields[field_index].name);
        CHECK(reflected[field_index].type == source_node.fields[field_index].type);
        CHECK(reflected[field_index].access ==
              source_node.fields[field_index].access);
      }
    }
  }
}

TEST_CASE("generated abstract interfaces remain non-instantiable") {
  const std::array<std::string_view, 1> selected{"X3DNode"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);

  sai::browser host{std::move(registry).value()};
  auto edit = host.current_scene().edit();
  auto abstract_node = edit.create_node("X3DNode");
  REQUIRE_FALSE(abstract_node);
  CHECK(abstract_node.error().code == sai::error_code::abstract_type);
}

TEST_CASE("generated metadata fails closed instead of collapsing ISO types") {
  const std::array<std::string_view, 1> selected{"TimeSensor"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE_FALSE(registry);
  CHECK(registry.error().code == sai::error_code::unsupported_field_type);
  CHECK(registry.error().field == "cycleInterval");
}

TEST_CASE("generated node constraints govern containment authoring") {
  const std::array<std::string_view, 3> selected{"Group", "Shape", "Box"};
  auto registry = sai::generated_type_registry(selected);
  REQUIRE(registry);

  sai::browser host{std::move(registry).value()};
  auto context = host.current_scene();
  auto valid = context.edit();
  auto group = valid.create_node("Group");
  auto shape = valid.create_node("Shape");
  REQUIRE(group);
  REQUIRE(shape);
  REQUIRE(valid.append(group.value(), "children", shape.value()));
  REQUIRE(valid.commit());

  auto invalid = context.edit();
  auto box = invalid.create_node("Box");
  REQUIRE(box);
  auto rejected = invalid.append(group.value(), "children", box.value());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::type_mismatch);
  REQUIRE_FALSE(invalid.commit());
}

TEST_CASE("registry rejects descriptors whose defaults or containment lie") {
  sai::type_registry registry;
  auto wrong_default = registry.define(test_node_type(
      "WrongDefault",
      {test_field("translation", sai::value_kind::vec3f,
                  sai::access_type::input_output,
                  std::string{"not a vector"})}));
  REQUIRE_FALSE(wrong_default);
  CHECK(wrong_default.error().code == sai::error_code::invalid_descriptor);

  auto wrong_containment = registry.define(test_node_type(
      "WrongContainment",
      {test_field("translation", sai::value_kind::vec3f,
                  sai::access_type::input_output, sai::vec3f{}, true)}));
  REQUIRE_FALSE(wrong_containment);
  CHECK(wrong_containment.error().code == sai::error_code::invalid_descriptor);
}

TEST_CASE("one shared node has one identity and two occurrences") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();

  auto root = edit.create_node("Group");
  auto shared = edit.create_node("Transform");
  REQUIRE(root);
  REQUIRE(shared);
  REQUIRE(edit.append_root(root.value()));
  REQUIRE(edit.append(root.value(), "children", shared.value()));
  REQUIRE(edit.append(root.value(), "children", shared.value()));

  auto committed = edit.commit();
  REQUIRE(committed);
  CHECK(committed.value().before_revision == 0);
  CHECK(committed.value().after_revision == 1);

  auto snapshot = context.snapshot();
  auto occurrences = snapshot.occurrences();
  REQUIRE(occurrences.size() == 3);
  CHECK(occurrences[0].node == root.value().id());
  CHECK(occurrences[1].node == shared.value().id());
  CHECK(occurrences[2].node == shared.value().id());
  CHECK(occurrences[1].path != occurrences[2].path);
  CHECK(occurrences[1].parent_occurrence == 0);
  CHECK(occurrences[2].parent_occurrence == 0);
}

TEST_CASE("a containment cycle rejects the complete edit") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto group = edit.create_node("Group");
  REQUIRE(group);
  REQUIRE(edit.append_root(group.value()));
  REQUIRE(edit.append(group.value(), "children", group.value()));

  auto rejected = edit.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::containment_cycle);
  CHECK(context.snapshot().revision() == 0);
  CHECK(context.snapshot().roots().empty());
}

TEST_CASE("dynamic discovery and typed fields are substitutable") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  auto before = context.snapshot();
  auto dynamic = before.field(transform.value(), "translation");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::vec3f>();
  REQUIRE(typed);
  CHECK(before.read(dynamic.value()).value() == sai::value{sai::vec3f{}});
  CHECK(before.read(typed.value()).value() == sai::vec3f{});

  auto wrong_type = dynamic.value().as<std::string>();
  REQUIRE_FALSE(wrong_type);
  CHECK(wrong_type.error().code == sai::error_code::type_mismatch);
  CHECK(wrong_type.error().node == transform.value().id());
  CHECK(wrong_type.error().field == "translation");

  auto edit = context.edit();
  REQUIRE(edit.set(typed.value(), sai::vec3f{1, 2, 3}));
  auto committed = edit.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 1);
  CHECK(committed.value().changes[0].kind == sai::change_kind::field_changed);
  CHECK(committed.value().changes[0].before == sai::value{sai::vec3f{}});
  CHECK(committed.value().changes[0].after == sai::value{sai::vec3f{1, 2, 3}});

  auto after = context.snapshot();
  CHECK(after.read(dynamic.value()).value() == sai::value{sai::vec3f{1, 2, 3}});
  CHECK(after.read(typed.value()).value() == sai::vec3f{1, 2, 3});
  CHECK(before.read(typed.value()).value() == sai::vec3f{});
}

TEST_CASE("inspection discovers node types and ordered field definitions") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto snapshot = context.snapshot();
  REQUIRE(snapshot.roots().size() == 1);
  auto discovered_node = snapshot.lookup(snapshot.roots().front());
  REQUIRE(discovered_node);
  auto descriptor = snapshot.describe(discovered_node.value());
  REQUIRE(descriptor);
  CHECK(descriptor.value().name == "Transform");
  REQUIRE(descriptor.value().fields.size() == 5);
  CHECK(descriptor.value().fields[0].name == "translation");
  CHECK(descriptor.value().fields[0].kind == sai::value_kind::vec3f);
  CHECK(descriptor.value().fields[0].access == sai::access_type::input_output);

  auto discovered_field = snapshot.field(discovered_node.value(),
                                         descriptor.value().fields[0].name);
  REQUIRE(discovered_field);
  CHECK(snapshot.read(discovered_field.value()).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("node-valued writes require a context-bearing node handle") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto link = author.create_node("Link");
  auto target = author.create_node("Transform");
  REQUIRE(link);
  REQUIRE(target);
  auto target_field = author.field(link.value(), "target");
  REQUIRE(target_field);
  REQUIRE(author.set(target_field.value(), target.value()));
  REQUIRE(author.append_root(link.value()));
  REQUIRE(author.commit());
  CHECK(context.snapshot().read(target_field.value()).value() ==
        sai::value{target.value().id()});

  auto bare_id = context.edit();
  auto denied_bare =
      bare_id.set(target_field.value(), sai::value{target.value().id()});
  REQUIRE_FALSE(denied_bare);
  CHECK(denied_bare.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(bare_id.commit());

  sai::browser foreign_host{graph_registry()};
  auto foreign_context = foreign_host.current_scene();
  auto foreign_author = foreign_context.edit();
  auto foreign_target = foreign_author.create_node("Transform");
  REQUIRE(foreign_target);
  REQUIRE(foreign_author.commit());

  auto cross_domain = context.edit();
  auto denied_foreign =
      cross_domain.set(target_field.value(), foreign_target.value());
  REQUIRE_FALSE(denied_foreign);
  CHECK(denied_foreign.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(cross_domain.commit());
  CHECK(context.snapshot().revision() == 1);
}

TEST_CASE("node ranges validate every element before staging") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto link = author.create_node("Link");
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(link);
  REQUIRE(first);
  REQUIRE(second);
  auto targets = author.field(link.value(), "targets");
  REQUIRE(targets);
  const std::array nodes{first.value(), second.value()};
  REQUIRE(author.set(targets.value(), std::span<const sai::node>{nodes}));
  REQUIRE(author.commit());
  CHECK(context.snapshot().read(targets.value()).value() ==
        sai::value{sai::node_list{first.value().id(), second.value().id()}});

  sai::browser foreign_host{graph_registry()};
  auto foreign_edit = foreign_host.current_scene().edit();
  auto foreign = foreign_edit.create_node("Transform");
  REQUIRE(foreign);
  REQUIRE(foreign_edit.commit());

  auto invalid = context.edit();
  const std::array mixed{first.value(), foreign.value()};
  auto denied = invalid.set(targets.value(), std::span<const sai::node>{mixed});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().revision() == 1);
}

TEST_CASE("one invalid write poisons the edit and publishes nothing") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto base = context.snapshot();
  auto translation = base.field(transform.value(), "translation");
  auto output = base.field(transform.value(), "worldTranslation");
  REQUIRE(translation);
  REQUIRE(output);

  auto edit = context.edit();
  REQUIRE(edit.set(translation.value(), sai::vec3f{4, 5, 6}));
  auto denied = edit.set(output.value(), sai::vec3f{7, 8, 9});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);
  CHECK(denied.error().operation == "scene_edit.set");

  auto rejected = edit.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::access_denied);
  CHECK(context.snapshot().revision() == base.revision());
  CHECK(context.snapshot().read(translation.value()).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("field access is a lifecycle rule rather than a setter shape") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  auto initial = author.field(transform.value(), "initialLabel");
  REQUIRE(initial);
  REQUIRE(author.set(initial.value(), std::string{"authored"}));
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  auto snapshot = context.snapshot();
  auto input = snapshot.field(transform.value(), "set_translation");
  REQUIRE(input);
  auto unreadable = snapshot.read(input.value());
  REQUIRE_FALSE(unreadable);
  CHECK(unreadable.error().code == sai::error_code::access_denied);

  auto relabel = context.edit();
  auto denied_initial = relabel.set(initial.value(), std::string{"too late"});
  REQUIRE_FALSE(denied_initial);
  CHECK(denied_initial.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(relabel.commit());

  auto input_edit = context.edit();
  auto denied_input = input_edit.set(input.value(), sai::vec3f{1, 2, 3});
  REQUIRE_FALSE(denied_input);
  CHECK(denied_input.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(input_edit.commit());
  CHECK(context.snapshot().read(initial.value()).value() ==
        sai::value{std::string{"authored"}});
}

TEST_CASE("inputOnly event intent is transient and access governed") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto snapshot = context.snapshot();
  auto input = snapshot.field(transform.value(), "set_translation");
  auto output = snapshot.field(transform.value(), "worldTranslation");
  auto initial = snapshot.field(transform.value(), "initialLabel");
  REQUIRE(input);
  REQUIRE(output);
  REQUIRE(initial);

  auto events = context.events(sai::event_time{42.0});
  REQUIRE(events.send(input.value(), sai::vec3f{1, 2, 3}));
  auto delivered = events.commit();
  REQUIRE(delivered);
  CHECK(delivered.value().time == sai::event_time{42.0});
  CHECK(delivered.value().before_revision == snapshot.revision());
  CHECK(delivered.value().after_revision == snapshot.revision());
  REQUIRE(delivered.value().deliveries.size() == 1);
  CHECK(delivered.value().deliveries[0].target == transform.value().id());
  CHECK(delivered.value().deliveries[0].field == "set_translation");
  CHECK(delivered.value().deliveries[0].payload ==
        sai::value{sai::vec3f{1, 2, 3}});
  CHECK(context.snapshot().revision() == snapshot.revision());
  CHECK_FALSE(context.snapshot().read(input.value()));

  auto denied_output = context.events(sai::event_time{43.0});
  auto output_result = denied_output.send(output.value(), sai::vec3f{4, 5, 6});
  REQUIRE_FALSE(output_result);
  CHECK(output_result.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(denied_output.commit());

  auto denied_initial = context.events(sai::event_time{43.0});
  auto initial_result =
      denied_initial.send(initial.value(), std::string{"late"});
  REQUIRE_FALSE(initial_result);
  CHECK(initial_result.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(denied_initial.commit());
  CHECK(context.snapshot().revision() == snapshot.revision());
}

TEST_CASE("inputOutput event intent publishes one coherent retained revision") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  const auto before = context.snapshot();
  auto dynamic = before.field(transform.value(), "translation");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::vec3f>();
  REQUIRE(typed);
  auto events = context.events(sai::event_time{50.0});
  REQUIRE(events.send(typed.value(), sai::vec3f{4, 5, 6}));
  auto delivered = events.commit();
  REQUIRE(delivered);
  CHECK(delivered.value().before_revision == before.revision());
  CHECK(delivered.value().after_revision == before.revision() + 1);
  REQUIRE(delivered.value().state_changes);
  REQUIRE(delivered.value().state_changes->changes.size() == 1);
  CHECK(delivered.value().state_changes->changes[0].kind ==
        sai::change_kind::field_changed);
  CHECK(delivered.value().state_changes->changes[0].before ==
        sai::value{sai::vec3f{}});
  CHECK(delivered.value().state_changes->changes[0].after ==
        sai::value{sai::vec3f{4, 5, 6}});
  CHECK(before.read(typed.value()).value() == sai::vec3f{});
  CHECK(context.snapshot().read(dynamic.value()).value() ==
        sai::value{sai::vec3f{4, 5, 6}});
}

TEST_CASE("typed and dynamic event intent have equal traces and failures") {
  sai::browser dynamic_host{graph_registry()};
  auto dynamic_context = dynamic_host.current_scene();
  auto dynamic_author = dynamic_context.edit();
  auto dynamic_node = dynamic_author.create_node("Transform");
  REQUIRE(dynamic_node);
  REQUIRE(dynamic_author.commit());
  auto dynamic_field =
      dynamic_context.snapshot().field(dynamic_node.value(), "translation");
  auto dynamic_output = dynamic_context.snapshot().field(dynamic_node.value(),
                                                         "worldTranslation");
  REQUIRE(dynamic_field);
  REQUIRE(dynamic_output);

  sai::browser typed_host{graph_registry()};
  auto typed_context = typed_host.current_scene();
  auto typed_author = typed_context.edit();
  auto typed_node = typed_author.create_node("Transform");
  REQUIRE(typed_node);
  REQUIRE(typed_author.commit());
  auto typed_dynamic =
      typed_context.snapshot().field(typed_node.value(), "translation");
  auto typed_output_dynamic =
      typed_context.snapshot().field(typed_node.value(), "worldTranslation");
  REQUIRE(typed_dynamic);
  REQUIRE(typed_output_dynamic);
  auto typed_field = typed_dynamic.value().as<sai::vec3f>();
  auto typed_output = typed_output_dynamic.value().as<sai::vec3f>();
  REQUIRE(typed_field);
  REQUIRE(typed_output);

  auto dynamic_events = dynamic_context.events(sai::event_time{55.0});
  REQUIRE(dynamic_events.send(dynamic_field.value(),
                              sai::value{sai::vec3f{7, 8, 9}}));
  auto dynamic_result = dynamic_events.commit();
  REQUIRE(dynamic_result);
  auto typed_events = typed_context.events(sai::event_time{55.0});
  REQUIRE(typed_events.send(typed_field.value(), sai::vec3f{7, 8, 9}));
  auto typed_result = typed_events.commit();
  REQUIRE(typed_result);

  CHECK(dynamic_result.value().time == typed_result.value().time);
  CHECK(dynamic_result.value().before_revision ==
        typed_result.value().before_revision);
  CHECK(dynamic_result.value().after_revision ==
        typed_result.value().after_revision);
  REQUIRE(dynamic_result.value().deliveries.size() == 1);
  REQUIRE(typed_result.value().deliveries.size() == 1);
  const auto &dynamic_delivery = dynamic_result.value().deliveries.front();
  const auto &typed_delivery = typed_result.value().deliveries.front();
  CHECK(dynamic_delivery.target == typed_delivery.target);
  CHECK(dynamic_delivery.field == typed_delivery.field);
  CHECK(dynamic_delivery.payload == typed_delivery.payload);
  CHECK(dynamic_delivery.cause == typed_delivery.cause);
  CHECK(dynamic_delivery.route_index == typed_delivery.route_index);
  CHECK(dynamic_context.snapshot().read(dynamic_field.value()).value() ==
        sai::value{typed_context.snapshot().read(typed_field.value()).value()});

  auto denied_dynamic = dynamic_context.events(sai::event_time{56.0});
  auto dynamic_failure = denied_dynamic.send(dynamic_output.value(),
                                             sai::value{sai::vec3f{1, 1, 1}});
  auto denied_typed = typed_context.events(sai::event_time{56.0});
  auto typed_failure =
      denied_typed.send(typed_output.value(), sai::vec3f{1, 1, 1});
  REQUIRE_FALSE(dynamic_failure);
  REQUIRE_FALSE(typed_failure);
  CHECK(dynamic_failure.error().code == typed_failure.error().code);
  CHECK(dynamic_failure.error().operation == typed_failure.error().operation);
  CHECK(dynamic_failure.error().node == typed_failure.error().node);
  CHECK(dynamic_failure.error().field == typed_failure.error().field);
}

TEST_CASE("node event payloads require context-bearing handles and ranges") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto link = author.create_node("Link");
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(link);
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(author.commit());
  auto snapshot = context.snapshot();
  auto target = snapshot.field(link.value(), "target");
  auto targets = snapshot.field(link.value(), "targets");
  REQUIRE(target);
  REQUIRE(targets);

  auto events = context.events(sai::event_time{60.0});
  REQUIRE(events.send(target.value(), first.value()));
  const std::array nodes{first.value(), second.value()};
  REQUIRE(events.send(targets.value(), std::span<const sai::node>{nodes}));
  REQUIRE(events.commit());
  CHECK(context.snapshot().read(target.value()).value() ==
        sai::value{first.value().id()});
  CHECK(context.snapshot().read(targets.value()).value() ==
        sai::value{sai::node_list{first.value().id(), second.value().id()}});

  sai::browser foreign_host{graph_registry()};
  auto foreign_edit = foreign_host.current_scene().edit();
  auto foreign = foreign_edit.create_node("Transform");
  REQUIRE(foreign);
  REQUIRE(foreign_edit.commit());
  auto invalid = context.events(sai::event_time{61.0});
  auto denied = invalid.send(target.value(), foreign.value());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().revision() == 2);
}

TEST_CASE("a multi-hop route cascade preserves time and causal identity") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  auto third = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(third);
  auto first_value = author.field(first.value(), "translation");
  auto second_value = author.field(second.value(), "translation");
  auto third_value = author.field(third.value(), "translation");
  REQUIRE(first_value);
  REQUIRE(second_value);
  REQUIRE(third_value);
  REQUIRE(author.add_route(first_value.value(), second_value.value()));
  REQUIRE(author.add_route(second_value.value(), third_value.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{70.0});
  REQUIRE(events.send(first_value.value(), sai::vec3f{7, 0, 0}));
  auto cascade = events.commit();
  REQUIRE(cascade);
  REQUIRE(cascade.value().deliveries.size() == 3);
  CHECK(cascade.value().deliveries[0].time == sai::event_time{70.0});
  CHECK_FALSE(cascade.value().deliveries[0].cause);
  CHECK_FALSE(cascade.value().deliveries[0].route_index);
  CHECK(cascade.value().deliveries[1].target == second.value().id());
  CHECK(cascade.value().deliveries[1].cause == 0);
  CHECK(cascade.value().deliveries[1].route_index == 0);
  CHECK(cascade.value().deliveries[2].target == third.value().id());
  CHECK(cascade.value().deliveries[2].cause == 1);
  CHECK(cascade.value().deliveries[2].route_index == 1);
  REQUIRE(cascade.value().state_changes);
  CHECK(cascade.value().state_changes->changes.size() == 3);
  CHECK(context.snapshot().read(third_value.value()).value() ==
        sai::value{sai::vec3f{7, 0, 0}});
}

TEST_CASE("routed node events obey the destination accepted-node constraint") {
  auto source_value = test_field("value", sai::value_kind::node,
                                 sai::access_type::input_output,
                                 sai::node_id{});
  source_value.accepted_node_types = {"Box"};
  auto sink_value = test_field("value", sai::value_kind::node,
                               sai::access_type::input_output, sai::node_id{});
  sink_value.accepted_node_types = {"Shape"};

  sai::type_registry registry;
  REQUIRE(registry.define(test_node_type("Box", {})));
  REQUIRE(registry.define(test_node_type("Shape", {})));
  REQUIRE(registry.define(
      test_node_type("NodeSource", {std::move(source_value)})));
  REQUIRE(
      registry.define(test_node_type("NodeSink", {std::move(sink_value)})));

  sai::browser host{std::move(registry)};
  auto context = host.current_scene();
  auto author = context.edit();
  auto box = author.create_node("Box");
  auto source = author.create_node("NodeSource");
  auto sink = author.create_node("NodeSink");
  REQUIRE(box);
  REQUIRE(source);
  REQUIRE(sink);
  auto output = author.field(source.value(), "value");
  auto input = author.field(sink.value(), "value");
  REQUIRE(output);
  REQUIRE(input);
  REQUIRE(author.add_route(output.value(), input.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{69.0});
  REQUIRE(events.send(output.value(), box.value()));
  auto rejected = events.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::type_mismatch);
  CHECK(rejected.error().operation == "event_batch.commit");
  CHECK(rejected.error().node == sink.value().id());
  CHECK(rejected.error().field == "value");
  CHECK(context.snapshot().revision() == 1);
  CHECK(context.snapshot().read(input.value()).value() ==
        sai::value{sai::node_id{}});
}

TEST_CASE("a route cycle reaches quiescence by firing each route once") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  auto first_value = author.field(first.value(), "translation");
  auto second_value = author.field(second.value(), "translation");
  REQUIRE(first_value);
  REQUIRE(second_value);
  REQUIRE(author.add_route(first_value.value(), second_value.value()));
  REQUIRE(author.add_route(second_value.value(), first_value.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{71.0});
  REQUIRE(events.send(first_value.value(), sai::vec3f{1, 0, 0}));
  auto cascade = events.commit();
  REQUIRE(cascade);
  REQUIRE(cascade.value().deliveries.size() == 3);
  CHECK(cascade.value().deliveries[1].route_index == 0);
  CHECK(cascade.value().deliveries[2].route_index == 1);
  CHECK(cascade.value().deliveries[2].target == first.value().id());
  REQUIRE(cascade.value().state_changes);
  CHECK(cascade.value().state_changes->changes.size() == 2);
}

TEST_CASE("fan-in preserves the trace and exposes nonportable ordering") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto first = author.create_node("Transform");
  auto second = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(sink);
  auto first_value = author.field(first.value(), "translation");
  auto second_value = author.field(second.value(), "translation");
  auto sink_value = author.field(sink.value(), "translation");
  REQUIRE(first_value);
  REQUIRE(second_value);
  REQUIRE(sink_value);
  REQUIRE(author.add_route(first_value.value(), sink_value.value()));
  REQUIRE(author.add_route(second_value.value(), sink_value.value()));
  REQUIRE(author.commit());

  auto events = context.events(sai::event_time{80.0});
  REQUIRE(events.send(first_value.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(events.send(second_value.value(), sai::vec3f{2, 0, 0}));
  auto cascade = events.commit();
  REQUIRE(cascade);
  REQUIRE(cascade.value().deliveries.size() == 4);
  REQUIRE(cascade.value().portability_issues.size() == 1);
  CHECK(cascade.value().portability_issues[0].code ==
        sai::event_issue_code::nonportable_fan_in);
  CHECK(cascade.value().portability_issues[0].target == sink.value().id());
  CHECK(cascade.value().portability_issues[0].field == "translation");
  CHECK(cascade.value().portability_issues[0].delivery_indices ==
        std::vector<std::size_t>{2, 3});
  CHECK(context.snapshot().read(sink_value.value()).value() ==
        sai::value{sai::vec3f{2, 0, 0}});
}

TEST_CASE("duplicate event seeds poison the complete batch") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();
  const auto revision = context.snapshot().revision();

  auto events = context.events(sai::event_time{81.0});
  REQUIRE(events.send(translation, sai::vec3f{1, 0, 0}));
  auto duplicate = events.send(translation, sai::vec3f{2, 0, 0});
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::ambiguous_event_seed);
  REQUIRE_FALSE(events.commit());
  CHECK(context.snapshot().revision() == revision);
  CHECK(context.snapshot().read(translation).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("specialized containment mutation obeys the field lifecycle") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto parent = author.create_node("AccessNode");
  auto child = author.create_node("Transform");
  REQUIRE(parent);
  REQUIRE(child);
  REQUIRE(author.append(parent.value(), "initialChildren", child.value()));
  REQUIRE(author.append_root(parent.value()));
  REQUIRE(author.commit());

  auto late_initial = context.edit();
  auto denied_initial =
      late_initial.append(parent.value(), "initialChildren", child.value());
  REQUIRE_FALSE(denied_initial);
  CHECK(denied_initial.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(late_initial.commit());

  auto input = context.edit();
  auto denied_input =
      input.append(parent.value(), "receivedChildren", child.value());
  REQUIRE_FALSE(denied_input);
  CHECK(denied_input.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(input.commit());

  auto output = context.edit();
  auto denied_output =
      output.append(parent.value(), "observedChildren", child.value());
  REQUIRE_FALSE(denied_output);
  CHECK(denied_output.error().code == sai::error_code::access_denied);
  REQUIRE_FALSE(output.commit());
  CHECK(context.snapshot().revision() == 1);
}

TEST_CASE("routes connect output-capable ports to input-capable ports") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto output = author.field(source.value(), "worldTranslation");
  auto input = author.field(sink.value(), "set_translation");
  REQUIRE(output);
  REQUIRE(input);
  REQUIRE(author.add_route(output.value(), input.value()));
  REQUIRE(author.commit());

  auto invalid = context.edit();
  auto initial_source = invalid.field(source.value(), "initialLabel");
  auto initial_sink = invalid.field(sink.value(), "initialLabel");
  REQUIRE(initial_source);
  REQUIRE(initial_sink);
  auto denied = invalid.add_route(initial_source.value(), initial_sink.value());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_route);
  REQUIRE_FALSE(invalid.commit());
  CHECK(context.snapshot().routes().size() == 1);
}

TEST_CASE("duplicate route identity is rejected without publication") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto output = author.field(source.value(), "worldTranslation");
  auto input = author.field(sink.value(), "set_translation");
  REQUIRE(output);
  REQUIRE(input);
  REQUIRE(author.add_route(output.value(), input.value()));
  REQUIRE(author.commit());

  auto duplicate = context.edit();
  auto denied = duplicate.add_route(output.value(), input.value());
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_route);
  REQUIRE_FALSE(duplicate.commit());
  CHECK(context.snapshot().revision() == 1);
  CHECK(context.snapshot().routes().size() == 1);
}

TEST_CASE("change sets preserve authored operation order") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create_node("Transform");
  REQUIRE(transform);
  auto translation = edit.field(transform.value(), "translation");
  REQUIRE(translation);
  REQUIRE(edit.set(translation.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(edit.append_root(transform.value()));

  auto committed = edit.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 3);
  CHECK(committed.value().changes[0].kind == sai::change_kind::node_created);
  CHECK(committed.value().changes[1].kind == sai::change_kind::field_changed);
  CHECK(committed.value().changes[2].kind == sai::change_kind::root_inserted);
}

TEST_CASE("names and routes publish at the same revision as their nodes") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto source = edit.create_node("Transform");
  auto sink = edit.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_field = edit.field(source.value(), "translation");
  auto sink_field = edit.field(sink.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(sink_field);
  REQUIRE(edit.define_name("Source", source.value()));
  REQUIRE(edit.define_name("Sink", sink.value()));
  REQUIRE(edit.add_route(source_field.value(), sink_field.value()));
  REQUIRE(edit.append_root(source.value()));
  REQUIRE(edit.append_root(sink.value()));
  REQUIRE(edit.commit());

  auto snapshot = context.snapshot();
  REQUIRE(snapshot.named("Source"));
  REQUIRE(snapshot.named("Sink"));
  CHECK(snapshot.named("Source").value().id() == source.value().id());
  REQUIRE(snapshot.routes().size() == 1);
  CHECK(snapshot.routes()[0].source == source.value().id());
  CHECK(snapshot.routes()[0].source_field == "translation");
  CHECK(snapshot.routes()[0].sink == sink.value().id());
  CHECK(snapshot.routes()[0].sink_field == "translation");
}

TEST_CASE(
    "context exports are ordered aliases while ordinary lookup stays local") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto sibling = host.create_scene();

  auto source_edit = source.edit();
  auto source_node = source_edit.create_node("Transform");
  REQUIRE(source_node);
  REQUIRE(source_edit.define_name("Local", source_node.value()));
  REQUIRE(source_edit.export_node("PublicTransform", source_node.value()));
  REQUIRE(source_edit.commit());

  auto sibling_edit = sibling.edit();
  auto sibling_node = sibling_edit.create_node("Transform");
  REQUIRE(sibling_node);
  REQUIRE(sibling_edit.define_name("Local", sibling_node.value()));
  REQUIRE(sibling_edit.commit());

  const auto source_snapshot = source.snapshot();
  const auto sibling_snapshot = sibling.snapshot();
  CHECK(source_snapshot.named("Local").value().id() ==
        source_node.value().id());
  CHECK(sibling_snapshot.named("Local").value().id() ==
        sibling_node.value().id());
  REQUIRE_FALSE(sibling_snapshot.exported("PublicTransform"));
  REQUIRE(source_snapshot.exports().size() == 1);
  CHECK(source_snapshot.exports()[0].name == "PublicTransform");
  CHECK(source_snapshot.exports()[0].node == source_node.value().id());
  CHECK(source_snapshot.exported("PublicTransform").value().id() ==
        source_node.value().id());

  auto remove = source.edit();
  REQUIRE(remove.remove_export(source_snapshot.exports().front()));
  REQUIRE(remove.commit());
  CHECK(source.snapshot().exports().empty());
  REQUIRE_FALSE(source.snapshot().exported("PublicTransform"));
}

TEST_CASE("imports cross only explicit same-browser apertures") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());

  auto parent_edit = parent.edit();
  REQUIRE(parent_edit.import_node("Remote", source, "Shared"));
  REQUIRE(parent_edit.commit());
  const auto snapshot = parent.snapshot();
  REQUIRE(snapshot.imports().size() == 1);
  CHECK(snapshot.imports()[0].local_name == "Remote");
  CHECK(snapshot.imports()[0].source_generation == source.generation());
  CHECK(snapshot.imports()[0].exported_name == "Shared");
  CHECK(snapshot.imports()[0].target ==
        sai::semantic_node_id{source.generation(), shared.value().id()});
  auto imported = snapshot.imported("Remote");
  REQUIRE(imported);
  CHECK(imported.value().identity() == snapshot.imports()[0].target);
  CHECK(imported.value().local_name() == "Remote");

  sai::browser foreign_host{graph_registry()};
  auto foreign_source = foreign_host.current_scene();
  auto foreign_edit = foreign_source.edit();
  auto foreign_node = foreign_edit.create_node("Transform");
  REQUIRE(foreign_node);
  REQUIRE(foreign_edit.export_node("Foreign", foreign_node.value()));
  REQUIRE(foreign_edit.commit());
  auto denied_edit = parent.edit();
  auto denied = denied_edit.import_node("Denied", foreign_source, "Foreign");
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::invalid_context);
  REQUIRE_FALSE(denied_edit.commit());
  CHECK(parent.snapshot().imports().size() == 1);
}

TEST_CASE("DEF and import names share one atomic local collision domain") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());

  auto named_parent = host.create_scene();
  auto named_edit = named_parent.edit();
  auto local = named_edit.create_node("Transform");
  REQUIRE(local);
  REQUIRE(named_edit.define_name("Collision", local.value()));
  auto import_collision = named_edit.import_node("Collision", source, "Shared");
  REQUIRE_FALSE(import_collision);
  CHECK(import_collision.error().code == sai::error_code::duplicate_name);
  REQUIRE_FALSE(named_edit.commit());
  CHECK(named_parent.snapshot().revision() == 0);

  auto imported_parent = host.create_scene();
  auto imported_edit = imported_parent.edit();
  auto another_local = imported_edit.create_node("Transform");
  REQUIRE(another_local);
  REQUIRE(imported_edit.import_node("Collision", source, "Shared"));
  auto name_collision =
      imported_edit.define_name("Collision", another_local.value());
  REQUIRE_FALSE(name_collision);
  CHECK(name_collision.error().code == sai::error_code::duplicate_name);
  REQUIRE_FALSE(imported_edit.commit());
  CHECK(imported_parent.snapshot().revision() == 0);
}

TEST_CASE("source export remapping invalidates a staged import atomically") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto first = source_edit.create_node("Transform");
  auto second = source_edit.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(source_edit.export_node("Shared", first.value()));
  REQUIRE(source_edit.commit());

  auto staged = parent.edit();
  REQUIRE(staged.import_node("Remote", source, "Shared"));
  int callbacks = 0;
  auto observed = parent.observe([&](const sai::change_set &) { ++callbacks; });

  const auto source_before = source.snapshot();
  auto remap = source.edit();
  REQUIRE(remap.remove_export(source_before.exports().front()));
  REQUIRE(remap.export_node("Shared", second.value()));
  REQUIRE(remap.commit());

  auto rejected = staged.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::stale_aperture);
  CHECK(parent.snapshot().revision() == 0);
  CHECK(parent.snapshot().imports().empty());
  CHECK(parent.drain().delivered == 0);
  CHECK(callbacks == 0);
  CHECK(observed.active());
}

TEST_CASE("ordered imports are their own removal tokens") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto first = source_edit.create_node("Transform");
  auto second = source_edit.create_node("Transform");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(source_edit.export_node("First", first.value()));
  REQUIRE(source_edit.export_node("Second", second.value()));
  REQUIRE(source_edit.commit());

  auto imports = parent.edit();
  REQUIRE(imports.import_node("A", source, "First"));
  REQUIRE(imports.import_node("B", source, "Second"));
  REQUIRE(imports.commit());
  const auto before = parent.snapshot();
  REQUIRE(before.imports().size() == 2);
  CHECK(before.imports()[0].local_name == "A");
  CHECK(before.imports()[1].local_name == "B");

  auto remove = parent.edit();
  REQUIRE(remove.remove_import(before.imports().front()));
  auto committed = remove.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 1);
  CHECK(committed.value().changes[0].kind == sai::change_kind::import_removed);
  const auto after = parent.snapshot();
  REQUIRE(after.imports().size() == 1);
  CHECK(after.imports()[0].local_name == "B");
}

TEST_CASE(
    "imported typed and dynamic reads capture immutable source revisions") {
  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  auto translation = source_edit.field(shared.value(), "translation");
  REQUIRE(translation);
  REQUIRE(source_edit.set(translation.value(), sai::vec3f{1, 2, 3}));
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());
  auto parent_edit = parent.edit();
  REQUIRE(parent_edit.import_node("Remote", source, "Shared"));
  REQUIRE(parent_edit.commit());

  const auto before = parent.snapshot();
  auto imported = before.imported("Remote");
  REQUIRE(imported);
  CHECK(imported.value().source_revision() == source.snapshot().revision());
  auto described = before.describe(imported.value());
  REQUIRE(described);
  CHECK(described.value().name == "Transform");
  auto dynamic = before.field(imported.value(), "translation");
  REQUIRE(dynamic);
  auto typed = dynamic.value().as<sai::vec3f>();
  REQUIRE(typed);
  CHECK(before.read(dynamic.value()).value() ==
        sai::value{sai::vec3f{1, 2, 3}});
  CHECK(before.read(typed.value()).value() == sai::vec3f{1, 2, 3});

  auto source_update = source.edit();
  REQUIRE(source_update.set(translation.value(), sai::vec3f{4, 5, 6}));
  REQUIRE(source_update.commit());
  CHECK(before.read(typed.value()).value() == sai::vec3f{1, 2, 3});

  const auto after = parent.snapshot();
  auto current = after.imported("Remote");
  REQUIRE(current);
  CHECK(current.value().source_revision() == source.snapshot().revision());
  auto current_field = after.field(current.value(), "translation");
  REQUIRE(current_field);
  CHECK(after.read(current_field.value()).value() ==
        sai::value{sai::vec3f{4, 5, 6}});
}

TEST_CASE("imported authority is inspection-only and context checked") {
  static_assert(!accepts_imported_containment<sai::scene_edit>);
  static_assert(!accepts_imported_retained_write<sai::scene_edit>);
  static_assert(!accepts_imported_event_target<sai::event_batch>);
  static_assert(!std::convertible_to<sai::imported_node, sai::node>);

  sai::browser host{graph_registry()};
  auto source = host.current_scene();
  auto parent = host.create_scene();
  auto unrelated = host.create_scene();
  auto source_edit = source.edit();
  auto shared = source_edit.create_node("Transform");
  REQUIRE(shared);
  REQUIRE(source_edit.export_node("Shared", shared.value()));
  REQUIRE(source_edit.commit());
  auto parent_edit = parent.edit();
  REQUIRE(parent_edit.import_node("Remote", source, "Shared"));
  REQUIRE(parent_edit.commit());

  const auto old_parent_snapshot = parent.snapshot();
  auto imported = old_parent_snapshot.imported("Remote");
  REQUIRE(imported);
  auto field = old_parent_snapshot.field(imported.value(), "translation");
  REQUIRE(field);
  auto wrong_context = unrelated.snapshot().describe(imported.value());
  REQUIRE_FALSE(wrong_context);
  CHECK(wrong_context.error().code == sai::error_code::invalid_context);

  int parent_callbacks = 0;
  auto observed =
      parent.observe([&](const sai::change_set &) { ++parent_callbacks; });
  auto source_update = source.edit();
  auto source_field = source_update.field(shared.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(source_update.set(source_field.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(source_update.commit());
  CHECK(parent.drain().delivered == 0);
  CHECK(parent_callbacks == 0);
  CHECK(observed.active());
  CHECK(old_parent_snapshot.read(field.value()).value() ==
        sai::value{sai::vec3f{}});

  REQUIRE(host.replace_world(parent));
  auto current_parent_snapshot = parent.snapshot();
  REQUIRE_FALSE(current_parent_snapshot.imported("Remote"));
  auto stale = current_parent_snapshot.describe(imported.value());
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_handle);
  CHECK(old_parent_snapshot.read(field.value()).value() ==
        sai::value{sai::vec3f{}});
}

TEST_CASE("enumerated roots names and routes are their own removal tokens") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_field = author.field(source.value(), "translation");
  auto sink_field = author.field(sink.value(), "translation");
  REQUIRE(source_field);
  REQUIRE(sink_field);
  REQUIRE(author.append_root(source.value()));
  REQUIRE(author.append_root(sink.value()));
  REQUIRE(author.define_name("Source", source.value()));
  REQUIRE(author.add_route(source_field.value(), sink_field.value()));
  REQUIRE(author.commit());

  const auto before = context.snapshot();
  REQUIRE(before.roots().size() == 2);
  REQUIRE(before.routes().size() == 1);
  REQUIRE(before.lookup(before.roots().front()));
  CHECK(before.lookup(before.roots().front()).value().id() ==
        source.value().id());
  REQUIRE(before.names().size() == 1);
  CHECK(before.names().front().name == "Source");
  CHECK(before.names().front().node == source.value().id());

  auto missing = before.lookup(sai::node_id{999999});
  REQUIRE_FALSE(missing);
  CHECK(missing.error().code == sai::error_code::unknown_node);

  auto remove = context.edit();
  REQUIRE(remove.remove_root(0));
  REQUIRE(remove.undefine_name(before.names().front().name));
  REQUIRE(remove.remove_route(before.routes().front()));
  auto committed = remove.commit();
  REQUIRE(committed);
  REQUIRE(committed.value().changes.size() == 3);
  CHECK(committed.value().changes[0].kind == sai::change_kind::root_removed);
  CHECK(committed.value().changes[0].before == sai::value{source.value().id()});
  CHECK(committed.value().changes[0].index == 0);
  CHECK(committed.value().changes[1].kind == sai::change_kind::name_removed);
  CHECK(committed.value().changes[1].field == "Source");
  CHECK(committed.value().changes[2].kind == sai::change_kind::route_removed);

  const auto after = context.snapshot();
  REQUIRE(after.roots().size() == 1);
  CHECK(after.roots().front() == sink.value().id());
  CHECK_FALSE(after.named("Source"));
  CHECK(after.routes().empty());
}

TEST_CASE("optimistic edits conflict while old snapshots remain immutable") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());

  auto old_snapshot = context.snapshot();
  auto translation = old_snapshot.field(transform.value(), "translation");
  REQUIRE(translation);
  auto first = context.edit();
  auto competing = context.edit();
  REQUIRE(first.set(translation.value(), sai::vec3f{1, 0, 0}));
  REQUIRE(competing.set(translation.value(), sai::vec3f{2, 0, 0}));
  REQUIRE(first.commit());

  auto conflict = competing.commit();
  REQUIRE_FALSE(conflict);
  CHECK(conflict.error().code == sai::error_code::stale_revision);
  CHECK(conflict.error().base_revision == old_snapshot.revision());
  CHECK(conflict.error().current_revision == context.snapshot().revision());
  CHECK(old_snapshot.read(translation.value()).value() ==
        sai::value{sai::vec3f{}});
  CHECK(context.snapshot().read(translation.value()).value() ==
        sai::value{sai::vec3f{1, 0, 0}});
}

TEST_CASE(
    "world replacement invalidates handles without invalidating snapshots") {
  sai::browser host{graph_registry()};
  auto old_context = host.current_scene();
  auto author = old_context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto historical = old_context.snapshot();
  auto old_field = historical.field(transform.value(), "translation");
  REQUIRE(old_field);
  int retired_calls = 0;
  auto retired =
      old_context.observe([&](const sai::change_set &) { ++retired_calls; });
  CHECK(retired.active());

  auto replacement = host.create_scene();
  CHECK(replacement.generation() != old_context.generation());
  REQUIRE(host.replace_world(replacement));
  CHECK(host.current_scene().generation() == replacement.generation());
  CHECK_FALSE(retired.active());
  CHECK(old_context.drain().delivered == 0);
  CHECK(retired_calls == 0);

  auto foreign = replacement.snapshot().field(transform.value(), "translation");
  REQUIRE_FALSE(foreign);
  CHECK(foreign.error().code == sai::error_code::stale_handle);
  CHECK(historical.read(old_field.value()).value() == sai::value{sai::vec3f{}});

  auto stale_edit = old_context.edit();
  REQUIRE(stale_edit.set(old_field.value(), sai::vec3f{9, 9, 9}));
  auto stale_commit = stale_edit.commit();
  REQUIRE_FALSE(stale_commit);
  CHECK(stale_commit.error().code == sai::error_code::stale_handle);
}

TEST_CASE("a browser rejects an execution context from another browser") {
  sai::browser first{graph_registry()};
  sai::browser second{graph_registry()};
  auto foreign = second.create_scene();
  const auto original = first.current_scene().generation();

  auto rejected = first.replace_world(foreign);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_context);
  CHECK(first.current_scene().generation() == original);
}

static_assert(std::movable<sai::subscription>);
static_assert(!std::copy_constructible<sai::subscription>);

TEST_CASE("commit queues observation and drain permits a reentrant commit") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  std::vector<sai::revision_id> observed;
  auto subscription = context.observe([&](const sai::change_set &changes) {
    observed.push_back(changes.after_revision);
    if (observed.size() == 1) {
      auto nested = context.edit();
      REQUIRE(nested.set(translation, sai::vec3f{2, 0, 0}));
      REQUIRE(nested.commit());
    }
  });
  CHECK(subscription.active());

  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(edit.commit());
  CHECK(observed.empty());

  auto report = context.drain();
  CHECK(report.delivered == 2);
  CHECK(report.errors.empty());
  REQUIRE(observed.size() == 2);
  CHECK(observed[0] + 1 == observed[1]);
  CHECK(context.snapshot().read(translation).value() ==
        sai::value{sai::vec3f{2, 0, 0}});
}

TEST_CASE("subscription cancellation and destruction are callback-free") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  int calls = 0;
  auto cancelled = context.observe([&](const sai::change_set &) { ++calls; });
  cancelled.cancel();
  CHECK_FALSE(cancelled.active());
  {
    auto destroyed = context.observe([&](const sai::change_set &) { ++calls; });
    CHECK(destroyed.active());
  }

  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{3, 0, 0}));
  REQUIRE(edit.commit());
  CHECK(context.drain().delivered == 0);
  CHECK(calls == 0);
}

TEST_CASE("one throwing observer does not starve later observers") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  auto throwing = context.observe([](const sai::change_set &) {
    throw std::runtime_error("user callback");
  });
  int later_calls = 0;
  auto later = context.observe([&](const sai::change_set &) { ++later_calls; });
  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(edit.commit());

  auto report = context.drain();
  CHECK(report.delivered == 1);
  REQUIRE(report.errors.size() == 1);
  CHECK(report.errors[0].code == sai::error_code::callback_failed);
  CHECK(report.errors[0].operation == "execution_context.drain");
  CHECK(later_calls == 1);
  CHECK(throwing.active());
  CHECK(later.active());
}

TEST_CASE("dispatch does not copy user callables") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.append_root(transform.value()));
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();

  auto armed = std::make_shared<bool>(false);
  int guarded_calls = 0;
  sai::execution_context::observer guarded{
      copy_bomb_observer{armed, guarded_calls}};
  auto first = context.observe(std::move(guarded));
  int later_calls = 0;
  auto later = context.observe([&](const sai::change_set &) { ++later_calls; });
  *armed = true;

  auto edit = context.edit();
  REQUIRE(edit.set(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(edit.commit());
  sai::dispatch_report report;
  CHECK_NOTHROW(report = context.drain());
  CHECK(report.delivered == 2);
  CHECK(report.errors.empty());
  CHECK(guarded_calls == 1);
  CHECK(later_calls == 1);
  CHECK(first.active());
  CHECK(later.active());
}

TEST_CASE("field event observation is queued cancellable and access checked") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_value = author.field(source.value(), "translation");
  auto sink_value = author.field(sink.value(), "translation");
  auto input = author.field(sink.value(), "set_translation");
  REQUIRE(source_value);
  REQUIRE(sink_value);
  REQUIRE(input);
  REQUIRE(author.add_route(source_value.value(), sink_value.value()));
  REQUIRE(author.commit());

  std::vector<sai::event_delivery> observed;
  auto registered = context.observe(sink_value.value(),
                                    [&](const sai::event_delivery &delivery) {
                                      observed.push_back(delivery);
                                    });
  REQUIRE(registered);
  auto subscription = std::move(registered).value();
  auto denied =
      context.observe(input.value(), [](const sai::event_delivery &) {});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);

  auto events = context.events(sai::event_time{90.0});
  REQUIRE(events.send(source_value.value(), sai::vec3f{9, 0, 0}));
  REQUIRE(events.commit());
  CHECK(observed.empty());
  auto report = context.drain();
  CHECK(report.delivered == 1);
  CHECK(report.errors.empty());
  REQUIRE(observed.size() == 1);
  CHECK(observed[0].target == sink.value().id());
  CHECK(observed[0].time == sai::event_time{90.0});

  auto cancelled = context.observe(sink_value.value(),
                                   [&](const sai::event_delivery &delivery) {
                                     observed.push_back(delivery);
                                   });
  REQUIRE(cancelled);
  auto cancelled_subscription = std::move(cancelled).value();
  auto later = context.events(sai::event_time{91.0});
  REQUIRE(later.send(source_value.value(), sai::vec3f{10, 0, 0}));
  REQUIRE(later.commit());
  subscription.cancel();
  cancelled_subscription.cancel();
  CHECK(context.drain().delivered == 0);
  CHECK(observed.size() == 1);
}

TEST_CASE(
    "field observers isolate exceptions and permit a later reentrant batch") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto source = author.create_node("Transform");
  auto sink = author.create_node("Transform");
  REQUIRE(source);
  REQUIRE(sink);
  auto source_value = author.field(source.value(), "translation").value();
  auto sink_value = author.field(sink.value(), "translation").value();
  REQUIRE(author.add_route(source_value, sink_value));
  REQUIRE(author.commit());

  auto throwing = context.observe(sink_value, [](const sai::event_delivery &) {
    throw std::runtime_error("field observer");
  });
  REQUIRE(throwing);
  std::vector<sai::event_time> observed;
  auto reentrant =
      context.observe(sink_value, [&](const sai::event_delivery &delivery) {
        observed.push_back(delivery.time);
        if (observed.size() == 1) {
          auto nested = context.events(sai::event_time{101.0});
          REQUIRE(nested.send(source_value, sai::vec3f{2, 0, 0}));
          REQUIRE(nested.commit());
        }
      });
  REQUIRE(reentrant);

  auto initial = context.events(sai::event_time{100.0});
  REQUIRE(initial.send(source_value, sai::vec3f{1, 0, 0}));
  REQUIRE(initial.commit());
  auto report = context.drain();
  CHECK(report.delivered == 2);
  CHECK(report.errors.size() == 2);
  REQUIRE(observed.size() == 2);
  CHECK(observed[0] == sai::event_time{100.0});
  CHECK(observed[1] == sai::event_time{101.0});
}

TEST_CASE("event publication rejects timestamps that do not move forward") {
  sai::browser host{graph_registry()};
  auto context = host.current_scene();
  auto author = context.edit();
  auto transform = author.create_node("Transform");
  REQUIRE(transform);
  REQUIRE(author.commit());
  auto translation =
      context.snapshot().field(transform.value(), "translation").value();
  auto first = context.events(sai::event_time{110.0});
  REQUIRE(first.send(translation, sai::vec3f{1, 0, 0}));
  REQUIRE(first.commit());
  const auto revision = context.snapshot().revision();

  auto same_time = context.events(sai::event_time{110.0});
  REQUIRE(same_time.send(translation, sai::vec3f{2, 0, 0}));
  auto rejected = same_time.commit();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::event_time_regression);
  CHECK(context.snapshot().revision() == revision);
  CHECK(context.snapshot().read(translation).value() ==
        sai::value{sai::vec3f{1, 0, 0}});
}

TEST_CASE("cancelled and stale load completions are inert") {
  sai::browser host{graph_registry()};
  const auto original_generation = host.current_scene().generation();

  auto cancelled_ticket = host.begin_load();
  auto cancelled_candidate = host.create_scene();
  REQUIRE(host.cancel(cancelled_ticket));
  auto cancelled = host.complete_load(cancelled_ticket, cancelled_candidate);
  REQUIRE_FALSE(cancelled);
  CHECK(cancelled.error().code == sai::error_code::cancelled);
  CHECK(host.current_scene().generation() == original_generation);

  auto stale_ticket = host.begin_load();
  auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  auto stale_candidate = host.create_scene();
  auto stale = host.complete_load(stale_ticket, stale_candidate);
  REQUIRE_FALSE(stale);
  CHECK(stale.error().code == sai::error_code::stale_completion);
  CHECK(host.current_scene().generation() == replacement.generation());

  auto valid_ticket = host.begin_load();
  auto valid_candidate = host.create_scene();
  REQUIRE(host.complete_load(valid_ticket, valid_candidate));
  CHECK(host.current_scene().generation() == valid_candidate.generation());
}
