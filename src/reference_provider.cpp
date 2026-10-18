#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/metadata.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <type_traits>

namespace x3d::sai::experimental {
namespace {

// The descriptor still exposes unsupported fields so discovery does not pretend
// that the selected generated node type has a smaller schema than it really has.
bool supported_field(const field_descriptor &field) noexcept {
  return field.name != "addChildren" && field.name != "removeChildren";
}

bool valid_curve(const float_list &keys, const vec3f_list &positions) noexcept {
  if (keys.empty() || keys.size() != positions.size() ||
      !std::all_of(keys.begin(), keys.end(), [](float v) { return std::isfinite(v); }) ||
      !std::all_of(positions.begin(), positions.end(), [](const vec3f &v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
      }))
    return false;
  for (std::size_t i = 1; i < keys.size(); ++i) {
    const auto &a = positions[i - 1];
    const auto &b = positions[i];
    if (!(keys[i] > keys[i - 1]) || !std::isfinite(keys[i] - keys[i - 1]) ||
        !std::isfinite(b.x - a.x) || !std::isfinite(b.y - a.y) || !std::isfinite(b.z - a.z))
      return false;
  }
  return true;
}

class reference_backend final : public provider::backend {
  struct curve_fields {
    node_id owner;
    dynamic_field keys, positions, output;
  };
  struct scene_storage {
    explicit scene_storage(type_registry registry)
        : host(std::move(registry)), scene(host.create_scene()) {}
    browser host;
    execution_context scene;
    std::vector<experimental::node> nodes;
    // Backend-owned setup roots preserve these kernel node identities while
    // delaying kernel root realization until activation. The frontend owns no
    // scene graph or alternative representation of this manifest.
    std::vector<experimental::node> prepared_roots;
    bool activated = false;
    // Cancel internal captures before destroying the kernel context/browser.
    std::vector<experimental::subscription> captures;
  };
  type_registry registry_;
  std::map<std::uint64_t, std::unique_ptr<scene_storage>> scenes_;
  std::uint64_t next_scene_ = 1;

public:
  explicit reference_backend(type_registry registry) : registry_(std::move(registry)) {}

  provider::capabilities supported() const override {
    provider::capabilities result;
    result.backend_name = "reference-kernel";
    result.services = {{provider::service_id::scene_authoring},
                       {provider::service_id::host_turns},
                       {provider::service_id::notifications},
                       {provider::service_id::node_values},
                       {provider::service_id::indexed_writes},
                       {provider::service_id::user_data},
                       {provider::service_id::scene_units},
                       {provider::service_id::scene_metadata}};
    for (const auto &type : registry_.node_types()) {
      if (type.abstract) continue;
      result.nodes.push_back({type.name, type.component,
                             static_cast<std::uint32_t>(type.component_level)});
      for (const auto &field : type.fields)
        if (supported_field(field) &&
            std::find(result.field_kinds.begin(), result.field_kinds.end(), field.kind) ==
                result.field_kinds.end())
          result.field_kinds.push_back(field.kind);
    }
    return result;
  }

private:
  result<scene_storage *> storage(std::uint64_t id) const {
    const auto found = scenes_.find(id);
    if (found == scenes_.end())
      return error(error_code::stale_handle, "reference_provider.scene",
                   "scene storage has been retired");
    return found->second.get();
  }

  result<std::vector<unit_declaration>> do_units(std::uint64_t id) const override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    return (*stored)->scene.snapshot().units();
  }
  result<void> do_declare_unit(std::uint64_t id, const unit_declaration &unit) override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    auto edit = (*stored)->scene.edit();
    if (auto ok = edit.declare_unit(unit); !ok) return ok;
    auto committed = edit.commit();
    if (!committed) return failure(committed.error());
    return {};
  }
  result<std::vector<metadata_entry>> do_metadata(std::uint64_t id) const override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    return (*stored)->scene.snapshot().metadata();
  }
  result<void> do_set_metadata(std::uint64_t id, std::string_view key,
                                const std::optional<std::string> &value) override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    auto edit = (*stored)->scene.edit();
    if (auto ok = edit.set_metadata(std::string(key), value); !ok) return ok;
    auto committed = edit.commit();
    if (!committed) return failure(committed.error());
    return {};
  }

  result<experimental::node> lookup_node(const address &target) const {
    auto stored = storage(target.scene);
    if (!stored) return failure(stored.error());
    if (target.context != 1)
      return error(error_code::invalid_context, "reference_provider.node",
                   "only the root lexical context is supported");
    return (*stored)->scene.snapshot().lookup(node_id{target.node});
  }

  result<dynamic_field> lookup(const address &target) const {
    auto owner = lookup_node(target);
    if (!owner) return failure(owner.error());
    return storage(target.scene).value()->scene.snapshot().field(*owner, target.field);
  }

  // Raw IDs cross only the private dispatch seam. Reacquire kernel authority
  // before authoring or sending any SFNode/MFNode payload.
  template <class Sink>
  result<void> write_value(scene_storage &stored, Sink &sink,
                            const dynamic_field &field, const value &payload) const {
    const auto snapshot = stored.scene.snapshot();
    const auto apply = [&](const auto &v) -> result<void> {
      if constexpr (std::same_as<Sink, scene_edit>) return sink.set(field, v);
      else return sink.send(field, v);
    };
    if (const auto *id = std::get_if<node_id>(&payload)) {
      if (id->value == 0) return apply(std::nullopt);
      auto owner = snapshot.lookup(*id);
      if (!owner) return failure(owner.error());
      return apply(*owner);
    }
    if (const auto *ids = std::get_if<node_list>(&payload)) {
      std::vector<experimental::node> nodes;
      nodes.reserve(ids->size());
      for (const auto id : *ids) {
        if (id.value == 0) { nodes.emplace_back(); continue; }
        auto owner = snapshot.lookup(id);
        if (!owner) return failure(owner.error());
        nodes.push_back(*owner);
      }
      return apply(std::span<const experimental::node>{nodes});
    }
    return apply(payload);
  }

  result<std::uint64_t> do_create_scene() override {
    const auto id = next_scene_++;
    scenes_.emplace(id, std::make_unique<scene_storage>(registry_));
    return id;
  }

  result<std::uint64_t> do_create_node(std::uint64_t id, std::string_view type) override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    auto edit = (*stored)->scene.edit();
    auto created = edit.create_node(std::string(type));
    if (!created) return failure(created.error());
    // Complete owner tracking allocation before publishing the kernel node.
    auto &nodes = (*stored)->nodes;
    if (nodes.size() == nodes.capacity())
      nodes.reserve(nodes.size() < nodes.max_size() / 2
                        ? std::max<std::size_t>(1, nodes.size() * 2)
                        : nodes.size() + 1);
    static_assert(std::is_nothrow_copy_constructible_v<experimental::node>);
    if (auto committed = edit.commit(); !committed) return failure(committed.error());
    (*stored)->nodes.push_back(*created);
    return created->id().value;
  }

  result<void> do_append_root(const address &target) override {
    auto owner = lookup_node(target);
    if (!owner) return failure(owner.error());
    storage(target.scene).value()->prepared_roots.push_back(*owner);
    return {};
  }

  result<void> do_define_name(const address &target, std::string_view name) override {
    auto owner = lookup_node(target);
    if (!owner) return failure(owner.error());
    auto stored = storage(target.scene).value();
    // Keep the contract's one canonical DEF name per node.
    for (const auto &binding : stored->scene.snapshot().names())
      if (binding.node == owner->id())
        return error(error_code::duplicate_name, "reference_provider.define_name",
                     "node is already named");
    auto edit = stored->scene.edit();
    if (auto defined = edit.define_name(std::string(name), *owner); !defined) return defined;
    if (auto committed = edit.commit(); !committed) return failure(committed.error());
    return {};
  }

  result<std::vector<std::uint64_t>> do_roots(std::uint64_t id) const override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    std::vector<std::uint64_t> roots;
    if (!(*stored)->activated) {
      for (const auto &root : (*stored)->prepared_roots) roots.push_back(root.id().value);
    } else {
      const auto snapshot = (*stored)->scene.snapshot();
      for (const auto root : snapshot.roots()) roots.push_back(root.value);
    }
    return roots;
  }

  result<std::uint64_t> do_named(std::uint64_t id, std::string_view name) const override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    auto found = (*stored)->scene.snapshot().named(std::string(name));
    if (!found)
      return error(error_code::unknown_node, "reference_provider.named",
                   "named node does not exist in this scene");
    return found->id().value;
  }

  result<std::vector<name_binding>> do_named_nodes(std::uint64_t id) const override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    std::vector<name_binding> result;
    const auto snapshot = (*stored)->scene.snapshot();
    for (const auto &binding : snapshot.names())
      result.push_back({binding.name, binding.node.value});
    return result;
  }

  result<std::string> do_type_name(const address &target) const override {
    auto owner = lookup_node(target);
    if (!owner) return failure(owner.error());
    return owner->get_type_name();
  }

  result<std::vector<provider::field_info>> do_fields(const address &target) const override {
    auto owner = lookup_node(target);
    if (!owner) return failure(owner.error());
    auto descriptor = storage(target.scene).value()->scene.snapshot().describe(*owner);
    if (!descriptor) return failure(descriptor.error());
    std::vector<provider::field_info> fields;
    for (const auto &field : descriptor->fields)
      fields.push_back({field.name, field.kind, field.access, supported_field(field)});
    return fields;
  }

  result<backend_value> do_read(const address &target) const override {
    auto field = lookup(target);
    if (!field) return failure(field.error());
    return storage(target.scene).value()->scene.snapshot().read(*field);
  }

  result<void> do_author(const address &target, const backend_value &payload) override {
    auto field = lookup(target);
    if (!field) return failure(field.error());
    auto stored = storage(target.scene).value();
    auto edit = stored->scene.construction_edit();
    if (auto written = write_value(*stored, edit, *field, payload); !written) return written;
    if (target.field == "keyValue") {
      // ISO 19775-1 19.3.1: reading before inputs returns the first keyValue,
      // or the field default when empty. Initialize real kernel storage in the
      // same setup edit, without generating an event or inventing host time.
      // https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#X3DInterpolatorNode
      auto owner = lookup_node(target);
      if (!owner) return failure(owner.error());
      auto output = edit.field(*owner, "value_changed");
      if (!output) return failure(output.error());
      const auto &positions = std::get<vec3f_list>(payload);
      const auto initial = positions.empty() ? vec3f{} : positions.front();
      if (auto initialized = edit.initialize_output(*output, value{initial}); !initialized)
        return initialized;
    }
    if (auto committed = edit.commit(); !committed) return failure(committed.error());
    return {};
  }

  result<void> do_add_route(const address &source, const address &target) override {
    auto from = lookup(source);
    auto to = lookup(target);
    if (!from) return failure(from.error());
    if (!to) return failure(to.error());
    auto edit = storage(source.scene).value()->scene.edit();
    if (auto added = edit.add_route(*from, *to); !added) return added;
    if (auto committed = edit.commit(); !committed) return failure(committed.error());
    return {};
  }

  result<std::vector<curve_fields>> curves(const scene_storage &stored) const {
    const auto snapshot = stored.scene.snapshot();
    std::vector<curve_fields> result;
    for (const auto &owner : stored.nodes) {
      auto type = snapshot.describe(owner);
      if (!type) return failure(type.error());
      if (type->name != "PositionInterpolator") continue;
      auto keys = snapshot.field(owner, "key");
      auto positions = snapshot.field(owner, "keyValue");
      auto output = snapshot.field(owner, "value_changed");
      if (!keys) return failure(keys.error());
      if (!positions) return failure(positions.error());
      if (!output) return failure(output.error());
      result.push_back({owner.id(), *keys, *positions, *output});
    }
    return result;
  }

  result<void> do_validate_configuration(
      std::uint64_t id, const std::vector<admitted_write> &writes) const override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    // Validate the final admitted node graph without publishing an edit. Live
    // gate releases may repair a transient cycle across several retained
    // writes, so validate the complete candidate rather than each prefix.
    auto candidate = (*stored)->scene.edit();
    for (const auto &write : writes) {
      if (!std::holds_alternative<node_id>(write.value) &&
          !std::holds_alternative<node_list>(write.value))
        continue;
      auto field = lookup(write.target);
      if (!field) return failure(field.error());
      if (field->access() != access_type::input_output) continue;
      if (auto written = write_value(**stored, candidate, *field, write.value); !written)
        return written;
    }
    if (auto valid = candidate.validate(); !valid) return valid;
    auto curve_list = curves(**stored);
    if (!curve_list) return failure(curve_list.error());
    const auto snapshot = (*stored)->scene.snapshot();
    for (const auto &curve : *curve_list) {
      auto keys_value = snapshot.read(curve.keys);
      auto positions_value = snapshot.read(curve.positions);
      if (!keys_value) return failure(keys_value.error());
      if (!positions_value) return failure(positions_value.error());
      auto keys = std::get<float_list>(std::move(*keys_value));
      auto positions = std::get<vec3f_list>(std::move(*positions_value));
      for (const auto &write : writes) {
        if (write.target.node != curve.owner.value) continue;
        if (write.target.field == "key") keys = std::get<float_list>(write.value);
        else if (write.target.field == "keyValue") positions = std::get<vec3f_list>(write.value);
      }
      if (!valid_curve(keys, positions))
        return error(error_code::invalid_value, "reference_provider.validate_configuration",
                     "the bounded interpolation policy requires matching nonempty finite arrays, "
                     "strictly increasing keys and finite float spans");
    }
    return {};
  }

  result<void> do_activate(std::uint64_t id) override {
    auto stored = storage(id);
    if (!stored) return failure(stored.error());
    auto curve_list = curves(**stored);
    if (!curve_list) return failure(curve_list.error());
    auto installed = (*stored)->host.set_runtime_processor(
        (*stored)->scene, [curves = std::move(*curve_list)](runtime_step &step) -> result<void> {
          const auto *incoming = step.input();
          if (!incoming || incoming->field != "set_fraction") return {};
          const auto curve = std::find_if(curves.begin(), curves.end(), [&](const auto &c) {
            return c.owner == incoming->target;
          });
          if (curve == curves.end()) return {};
          auto keys_value = step.read(curve->keys);
          auto positions_value = step.read(curve->positions);
          if (!keys_value) return failure(keys_value.error());
          if (!positions_value) return failure(positions_value.error());
          const auto &keys = std::get<float_list>(*keys_value);
          const auto &positions = std::get<vec3f_list>(*positions_value);
          const float fraction = std::get<float>(incoming->payload);
          // ROUTEs may also alter curve fields during a turn. Revalidate the
          // staged values before indexing, even after frontend admission.
          if (!std::isfinite(fraction) || !valid_curve(keys, positions))
            return error(error_code::invalid_value, "reference_provider.interpolate",
                         "the staged curve violates the bounded interpolation policy");
          vec3f interpolated;
          if (fraction <= keys.front()) interpolated = positions.front();
          else if (fraction >= keys.back()) interpolated = positions.back();
          else {
            const auto high = static_cast<std::size_t>(
                std::upper_bound(keys.begin(), keys.end(), fraction) - keys.begin());
            const auto low = high - 1;
            const float weight = static_cast<float>((static_cast<double>(fraction) - keys[low]) /
                (static_cast<double>(keys[high]) - keys[low]));
            const auto &a = positions[low];
            const auto &b = positions[high];
            interpolated = {std::lerp(a.x, b.x, weight), std::lerp(a.y, b.y, weight),
                            std::lerp(a.z, b.z, weight)};
          }
          return step.emit(curve->output, value{interpolated});
        });
    if (!installed) return installed;
    auto edit = (*stored)->scene.edit();
    for (const auto &root : (*stored)->prepared_roots)
      if (auto added = edit.append_root(root); !added) return added;
    if (auto committed = edit.commit(); !committed) return failure(committed.error());
    if (auto activated = (*stored)->host.replace_world((*stored)->scene); !activated) return activated;
    (*stored)->activated = true;
    const auto snapshot = (*stored)->scene.snapshot();
    for (const auto &owner : (*stored)->nodes) {
      auto descriptor = snapshot.describe(owner);
      if (!descriptor) return failure(descriptor.error());
      for (const auto &field : descriptor->fields) {
        if (!supported_field(field) ||
            (field.access != access_type::input_output && field.access != access_type::output_only))
          continue;
        auto source = snapshot.field(owner, field.name);
        if (!source) return failure(source.error());
        auto capture = (*stored)->scene.observe(*source, [this, id](const event_delivery &delivery) {
          record_event({id, 1, delivery.target.value, delivery.field}, delivery.payload);
        });
        if (!capture) return failure(capture.error());
        (*stored)->captures.push_back(std::move(*capture));
      }
    }
    return {};
  }

  backend_result do_turn(std::uint64_t id, event_time time,
                         const std::vector<admitted_write> &writes) override {
    auto fail = [](sai_error error) -> backend_result {
      return tl::unexpected{backend_failure{std::move(error), provider::failure_effect::none}};
    };
    auto stored = storage(id);
    if (!stored) return fail(stored.error());
    auto batch = (*stored)->scene.logical_turn(time);
    // Explicit bounded policy: install every retained inputOutput candidate
    // before processing inputOnly behavior. Preserve occurrence order within
    // both groups; never reset output/ROUTE guards between input occurrences.
    // The supported runtime has no independent startup/tick behavior that reads
    // retained fields before these seeds are installed.
    for (const bool stateful : {true, false}) {
      for (const auto &write : writes) {
        auto field = lookup(write.target);
        if (!field) return fail(field.error());
        if ((field->access() == access_type::input_output) != stateful) continue;
        if (auto sent = write_value(**stored, batch, *field, write.value); !sent)
          return fail(sent.error());
      }
    }
    if (auto committed = batch.commit(); !committed) return fail(committed.error());
    const auto captured = (*stored)->scene.drain();
    if (!captured.errors.empty())
      return tl::unexpected{backend_failure{captured.errors.front(),
                                           provider::failure_effect::partial_or_unknown}};
    return {};
  }

  void do_retire(std::uint64_t id) noexcept override { scenes_.erase(id); }
};
} // namespace

result<std::unique_ptr<provider::service>>
make_reference_provider(provider::resource_limits limits) {
  constexpr std::array<std::string_view, 4> types{"PositionInterpolator", "Transform", "Shape", "Box"};
  auto registry = generated_type_registry(types);
  if (!registry) return failure(registry.error());
  return std::make_unique<provider::service>(
      std::make_unique<reference_backend>(std::move(*registry)), limits);
}
} // namespace x3d::sai::experimental
