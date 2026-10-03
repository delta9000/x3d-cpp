#include "x3d/sai/experimental/kernel.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <deque>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace x3d::sai::experimental {

namespace {

// Authority comparisons must not borrow foreign owners inside internal locks.
template <class Control>
bool same_owner(const std::weak_ptr<Control> &candidate,
                const std::shared_ptr<Control> &owner) noexcept {
  return !candidate.owner_before(owner) && !owner.owner_before(candidate);
}

unexpected descriptor_error(error_code code, std::string operation,
                            std::string message) {
  sai_error error;
  error.code = code;
  error.operation = std::move(operation);
  error.message = std::move(message);
  return failure(std::move(error));
}

std::optional<value_kind> kind_of(const value &candidate) {
  return std::visit(
      []<class T>(const T &) -> std::optional<value_kind> {
        if constexpr (std::is_same_v<T, std::monostate>)
          return std::nullopt;
        else
          return value_traits<T>::kind;
      },
      candidate);
}

bool valid_image(const image &candidate) {
  if (candidate.width < 0 || candidate.height < 0 || candidate.components < 0 ||
      candidate.components > 4)
    return false;
  const auto bytes = static_cast<std::uint64_t>(candidate.width) *
                     static_cast<std::uint64_t>(candidate.height) *
                     static_cast<std::uint64_t>(candidate.components);
  return bytes <= std::numeric_limits<std::size_t>::max() &&
         bytes == candidate.data.size();
}

bool valid_color_component(float channel) {
  return channel >= 0.f && channel <= 1.f;
}

bool valid_color(const color3f &candidate) {
  return valid_color_component(candidate.r) &&
         valid_color_component(candidate.g) &&
         valid_color_component(candidate.b);
}

bool valid_color(const color4f &candidate) {
  return valid_color(color3f{candidate.r, candidate.g, candidate.b}) &&
         valid_color_component(candidate.a);
}

bool value_is_well_formed(const value &candidate) {
  return std::visit(
      []<class T>(const T &stored) {
        if constexpr (std::is_same_v<T, image>)
          return valid_image(stored);
        else if constexpr (std::is_same_v<T, image_list>)
          return std::ranges::all_of(stored, valid_image);
        else if constexpr (std::is_same_v<T, color3f> ||
                           std::is_same_v<T, color4f>)
          return valid_color(stored);
        else if constexpr (std::is_same_v<T, color3f_list> ||
                           std::is_same_v<T, color4f_list>)
          return std::ranges::all_of(
              stored, [](const auto &color) { return valid_color(color); });
        else
          return true;
      },
      candidate);
}

bool representation_equal(float left, float right) {
  return std::bit_cast<std::uint32_t>(left) ==
         std::bit_cast<std::uint32_t>(right);
}

bool representation_equal(double left, double right) {
  return std::bit_cast<std::uint64_t>(left) ==
         std::bit_cast<std::uint64_t>(right);
}

template <class T> bool representation_equal(const T &left, const T &right) {
  return left == right;
}

bool representation_equal(const time_value &left, const time_value &right) {
  return representation_equal(left.seconds, right.seconds);
}

#define X3D_SAI_COMPONENT_EQUAL(Type, ...)                                     \
  bool representation_equal(const Type &left, const Type &right) {             \
    return (__VA_ARGS__);                                                      \
  }
X3D_SAI_COMPONENT_EQUAL(color3f, representation_equal(left.r, right.r) &&
                                     representation_equal(left.g, right.g) &&
                                     representation_equal(left.b, right.b));
X3D_SAI_COMPONENT_EQUAL(color4f, representation_equal(left.r, right.r) &&
                                     representation_equal(left.g, right.g) &&
                                     representation_equal(left.b, right.b) &&
                                     representation_equal(left.a, right.a));
X3D_SAI_COMPONENT_EQUAL(rotation,
                        representation_equal(left.x, right.x) &&
                            representation_equal(left.y, right.y) &&
                            representation_equal(left.z, right.z) &&
                            representation_equal(left.angle, right.angle));
X3D_SAI_COMPONENT_EQUAL(vec2d, representation_equal(left.x, right.x) &&
                                   representation_equal(left.y, right.y));
X3D_SAI_COMPONENT_EQUAL(vec2f, representation_equal(left.x, right.x) &&
                                   representation_equal(left.y, right.y));
X3D_SAI_COMPONENT_EQUAL(vec3d, representation_equal(left.x, right.x) &&
                                   representation_equal(left.y, right.y) &&
                                   representation_equal(left.z, right.z));
X3D_SAI_COMPONENT_EQUAL(vec3f, representation_equal(left.x, right.x) &&
                                   representation_equal(left.y, right.y) &&
                                   representation_equal(left.z, right.z));
X3D_SAI_COMPONENT_EQUAL(vec4d, representation_equal(left.x, right.x) &&
                                   representation_equal(left.y, right.y) &&
                                   representation_equal(left.z, right.z) &&
                                   representation_equal(left.w, right.w));
X3D_SAI_COMPONENT_EQUAL(vec4f, representation_equal(left.x, right.x) &&
                                   representation_equal(left.y, right.y) &&
                                   representation_equal(left.z, right.z) &&
                                   representation_equal(left.w, right.w));
#undef X3D_SAI_COMPONENT_EQUAL

template <class T, std::size_t N>
bool representation_equal(const matrix<T, N> &left, const matrix<T, N> &right) {
  for (std::size_t index = 0; index < left.elements.size(); ++index) {
    if (!representation_equal(left.elements[index], right.elements[index]))
      return false;
  }
  return true;
}

template <class T, class Allocator>
bool representation_equal(const std::vector<T, Allocator> &left,
                          const std::vector<T, Allocator> &right) {
  if (left.size() != right.size())
    return false;
  for (std::size_t index = 0; index < left.size(); ++index) {
    if constexpr (std::is_same_v<T, bool>) {
      if (static_cast<bool>(left[index]) != static_cast<bool>(right[index]))
        return false;
    } else if (!representation_equal(left[index], right[index])) {
      return false;
    }
  }
  return true;
}

bool representation_equal(const image &left, const image &right) {
  return left.width == right.width && left.height == right.height &&
         left.components == right.components &&
         representation_equal(left.data, right.data);
}

bool representation_equal(const value &left, const value &right) {
  if (left.index() != right.index())
    return false;
  return std::visit(
      [&]<class T>(const T &stored) {
        return representation_equal(stored, std::get<T>(right));
      },
      left);
}

bool value_matches(value_kind kind, const value &candidate) {
  return kind_of(candidate) == kind && value_is_well_formed(candidate);
}

bool has_non_null_node_identity(const value &candidate) {
  if (const auto *single = std::get_if<node_id>(&candidate))
    return single->value != 0;
  if (const auto *many = std::get_if<node_list>(&candidate))
    return std::ranges::any_of(*many, [](node_id id) { return id.value != 0; });
  return false;
}

bool standard_unit_category(std::string_view category) {
  return category == "angle" || category == "force" || category == "length" ||
         category == "mass";
}

struct unit_basis {
  std::string_view category;
  int power;
};

std::optional<unit_basis> field_unit_basis(std::string_view category) {
  if (standard_unit_category(category))
    return unit_basis{category, 1};
  if (category == "angular_velocity")
    return unit_basis{"angle", 1};
  if (category == "acceleration" || category == "speed")
    return unit_basis{"length", 1};
  if (category == "area")
    return unit_basis{"length", 2};
  if (category == "volume")
    return unit_basis{"length", 3};
  return std::nullopt;
}

bool unit_scalable_kind(value_kind kind) {
  switch (kind) {
  case value_kind::sf_double:
  case value_kind::sf_float:
  case value_kind::sf_rotation:
  case value_kind::sf_vec2d:
  case value_kind::sf_vec2f:
  case value_kind::sf_vec3d:
  case value_kind::sf_vec3f:
  case value_kind::sf_vec4d:
  case value_kind::sf_vec4f:
  case value_kind::mf_double:
  case value_kind::mf_float:
  case value_kind::mf_rotation:
  case value_kind::mf_vec2d:
  case value_kind::mf_vec2f:
  case value_kind::mf_vec3d:
  case value_kind::mf_vec3f:
  case value_kind::mf_vec4d:
  case value_kind::mf_vec4f:
    return true;
  default:
    return false;
  }
}

} // namespace

bool same_representation(const value &left, const value &right) {
  return representation_equal(left, right);
}

result<void> type_registry::define(node_type_descriptor descriptor) {
  if (descriptor.name.empty()) {
    return descriptor_error(error_code::invalid_descriptor,
                            "type_registry.define", "type name is empty");
  }
  if (types_.contains(descriptor.name)) {
    return descriptor_error(error_code::duplicate_type, "type_registry.define",
                            "type is already defined: " + descriptor.name);
  }
  std::unordered_set<std::string> fields;
  for (const auto &field : descriptor.fields) {
    if (field.name.empty()) {
      return descriptor_error(error_code::invalid_descriptor,
                              "type_registry.define", "field name is empty");
    }
    if (!fields.insert(field.name).second) {
      return descriptor_error(error_code::duplicate_field,
                              "type_registry.define",
                              "field is already defined: " + field.name);
    }
    if (!value_matches(field.kind, field.default_value)) {
      return descriptor_error(
          error_code::invalid_descriptor, "type_registry.define",
          "default value violates field kind or representation: " + field.name);
    }
    if (has_non_null_node_identity(field.default_value)) {
      return descriptor_error(
          error_code::invalid_descriptor, "type_registry.define",
          "node default requires context-bearing authoring: " + field.name);
    }
    if (field.unit_category && (!field_unit_basis(*field.unit_category) ||
                                !unit_scalable_kind(field.kind))) {
      return descriptor_error(error_code::invalid_descriptor,
                              "type_registry.define",
                              "field unit category is unknown or incompatible "
                              "with its value kind: " +
                                  field.name);
    }
    if (field.containment && field.kind != value_kind::sf_node &&
        field.kind != value_kind::mf_node) {
      return descriptor_error(
          error_code::invalid_descriptor, "type_registry.define",
          "containment field is not node-valued: " + field.name);
    }
  }
  types_.emplace(descriptor.name, std::move(descriptor));
  // The fingerprint certifies the exact generated descriptor set. Any
  // successful public mutation turns this back into an ordinary registry.
  schema_fingerprint_.reset();
  return {};
}

const node_type_descriptor *
type_registry::find(const std::string &name) const noexcept {
  const auto found = types_.find(name);
  return found == types_.end() ? nullptr : &found->second;
}

std::vector<node_type_descriptor> type_registry::node_types() const {
  std::vector<node_type_descriptor> descriptors;
  descriptors.reserve(types_.size());
  for (const auto &entry : types_)
    descriptors.push_back(entry.second);
  std::ranges::sort(descriptors, {}, &node_type_descriptor::name);
  return descriptors;
}

namespace detail {

struct scene_state {
  struct node_record {
    std::string type_name;
    std::unordered_map<std::string, value> fields;
    std::optional<declaration_id> scope_owner;
    bool realized = false;
  };

  revision_id revision = 0;
  std::unordered_map<std::uint64_t, node_record> nodes;
  std::vector<declaration_id> declaration_order;
  std::unordered_map<std::uint64_t, declaration_descriptor> declarations;
  std::vector<unit_declaration> units;
  std::vector<metadata_entry> metadata;
  std::vector<node_id> roots;
  std::vector<name_binding> names;
  std::vector<export_binding> exports;
  std::vector<import_binding> imports;
  std::vector<std::weak_ptr<context_control>> import_sources;
  std::vector<route> routes;
  std::any runtime_state;
};

struct context_control {
  struct import_dependent_record {
    std::weak_ptr<context_control> importer;
    generation_id importer_generation = 0;
    std::string local_name;
    node_id target;
  };

  struct pending_change {
    change_set changes;
    std::vector<std::uint64_t> recipients;
  };
  struct pending_event {
    event_delivery delivery;
    std::vector<std::uint64_t> recipients;
  };
  using pending_notification = std::variant<pending_change, pending_event>;

  struct field_observer_record {
    node_id target;
    std::string field;
    std::shared_ptr<const execution_context::event_observer> callback;
  };

  mutable std::mutex mutex;
  std::weak_ptr<browser_control> owner;
  generation_id generation = 0;
  std::atomic_bool active{true};
  std::atomic_bool draining{false};
  type_registry registry;
  std::shared_ptr<const scene_state> state = std::make_shared<scene_state>();
  std::atomic<revision_id> published_revision{0};
  // Identity allocation belongs to the context, not the rollbackable scene.
  // Concurrent, abandoned, and rejected edits may consume IDs, but cannot
  // cause a later object to impersonate one of their leaked handles.
  std::atomic<std::uint64_t> next_node_id{1};
  std::atomic<std::uint64_t> next_declaration_id{1};
  std::uint64_t next_observer_id = 1;
  std::map<std::uint64_t, std::shared_ptr<const execution_context::observer>>
      observers;
  std::map<std::uint64_t, field_observer_record> event_observers;
  std::list<pending_notification> pending;
  std::optional<event_time> last_event_time;
  std::shared_ptr<const runtime_factory> factory;
  bool processing = false;
  std::vector<import_dependent_record> import_dependents;
};

struct browser_control {
  struct request_record {
    std::uint64_t world_epoch = 0;
    bool cancelled = false;
  };

  mutable std::mutex mutex;
  capability_set capabilities;
  type_registry registry;
  generation_id next_generation = 2;
  std::uint64_t world_epoch = 1;
  std::uint64_t next_request_id = 1;
  std::unordered_map<std::uint64_t, request_record> requests;
  std::shared_ptr<context_control> current;
};

} // namespace detail

namespace {

std::optional<std::uint64_t>
reserve_identity(std::atomic<std::uint64_t> &next) {
  auto candidate = next.load(std::memory_order_relaxed);
  while (candidate != std::numeric_limits<std::uint64_t>::max()) {
    if (next.compare_exchange_weak(candidate, candidate + 1,
                                   std::memory_order_relaxed))
      return candidate;
  }
  return std::nullopt;
}

unexpected edit_error(error_code code, const char *operation,
                      const detail::context_control &context,
                      revision_id base_revision, std::string message,
                      std::optional<node_id> node = std::nullopt,
                      std::string field = {}) {
  sai_error error;
  error.code = code;
  error.operation = operation;
  error.message = std::move(message);
  error.generation = context.generation;
  error.base_revision = base_revision;
  error.current_revision =
      context.published_revision.load(std::memory_order_acquire);
  error.node = node;
  error.field = std::move(field);
  return failure(std::move(error));
}

unexpected declaration_edit_error(error_code code, const char *operation,
                                  const detail::context_control &context,
                                  revision_id base_revision,
                                  std::string message, declaration_id target,
                                  std::string field = {}) {
  sai_error error;
  error.code = code;
  error.operation = operation;
  error.message = std::move(message);
  error.generation = context.generation;
  error.base_revision = base_revision;
  error.current_revision =
      context.published_revision.load(std::memory_order_acquire);
  error.declaration = target;
  error.field = std::move(field);
  return failure(std::move(error));
}

const field_descriptor *
find_descriptor(const detail::context_control &context,
                const detail::scene_state::node_record &record,
                const std::string &field) {
  const auto *type = context.registry.find(record.type_name);
  if (!type)
    return nullptr;
  for (const auto &descriptor : type->fields) {
    if (descriptor.name == field)
      return &descriptor;
  }
  return nullptr;
}

bool accepts_node_type(const detail::context_control &context,
                       const field_descriptor &field,
                       const detail::scene_state::node_record &candidate) {
  if (field.accepted_node_types.empty())
    return true;
  const auto *candidate_type = context.registry.find(candidate.type_name);
  if (!candidate_type)
    return false;
  return std::any_of(
      field.accepted_node_types.begin(), field.accepted_node_types.end(),
      [&](const std::string &accepted) {
        return accepted == candidate_type->name ||
               std::find(candidate_type->interfaces.begin(),
                         candidate_type->interfaces.end(),
                         accepted) != candidate_type->interfaces.end();
      });
}

bool same_node_scope(const detail::scene_state::node_record &owner,
                     const detail::scene_state::node_record &candidate) {
  return owner.scope_owner == candidate.scope_owner;
}

std::optional<error_code>
node_payload_issue(const detail::context_control &context,
                   const detail::scene_state &state,
                   const detail::scene_state::node_record &owner,
                   const field_descriptor &field, const value &payload) {
  const auto check = [&](node_id id) -> std::optional<error_code> {
    if (id.value == 0)
      return std::nullopt;
    const auto candidate = state.nodes.find(id.value);
    if (candidate == state.nodes.end() ||
        !accepts_node_type(context, field, candidate->second))
      return error_code::type_mismatch;
    if (!same_node_scope(owner, candidate->second))
      return error_code::invalid_context;
    return std::nullopt;
  };
  if (field.kind == value_kind::sf_node)
    return check(std::get<node_id>(payload));
  if (field.kind == value_kind::mf_node) {
    for (const node_id id : std::get<node_list>(payload)) {
      if (const auto issue = check(id))
        return issue;
    }
  }
  return std::nullopt;
}

result<void> validate_candidate_graph(const detail::context_control &context,
                                      const detail::scene_state &state,
                                      const char *operation,
                                      revision_id base_revision) {
  std::vector<std::uint64_t> node_ids;
  node_ids.reserve(state.nodes.size());
  for (const auto &[id, record] : state.nodes) {
    (void)record;
    node_ids.push_back(id);
  }
  std::sort(node_ids.begin(), node_ids.end());
  for (const auto id : node_ids) {
    const auto &record = state.nodes.at(id);
    const auto *type = context.registry.find(record.type_name);
    for (const auto &field : type->fields) {
      if (const auto issue = node_payload_issue(context, state, record, field,
                                                record.fields.at(field.name))) {
        return edit_error(*issue, operation, context, base_revision,
                          "node reference violates type or declaration scope",
                          node_id{id}, field.name);
      }
    }
  }
  std::unordered_map<std::uint64_t, int> colors;
  std::optional<std::pair<node_id, std::string>> cycle;
  std::function<bool(std::uint64_t)> visit = [&](std::uint64_t id) {
    if (id == 0 || colors[id] == 2)
      return true;
    if (colors[id] == 1)
      return false;
    colors[id] = 1;
    const auto &record = state.nodes.at(id);
    const auto *type = context.registry.find(record.type_name);
    for (const auto &field : type->fields) {
      if (!field.containment)
        continue;
      const auto &payload = record.fields.at(field.name);
      const auto follow = [&](node_id child) {
        if (visit(child.value))
          return true;
        if (!cycle)
          cycle.emplace(node_id{id}, field.name);
        return false;
      };
      if (field.kind == value_kind::sf_node) {
        if (!follow(std::get<node_id>(payload)))
          return false;
      } else if (field.kind == value_kind::mf_node) {
        for (node_id child : std::get<node_list>(payload)) {
          if (!follow(child))
            return false;
        }
      }
    }
    colors[id] = 2;
    return true;
  };
  for (const auto id : node_ids) {
    if (!visit(id))
      return edit_error(error_code::containment_cycle, operation, context,
                        base_revision, "containment graph contains a cycle",
                        cycle->first, cycle->second);
  }
  return {};
}

struct node_reference_blocker {
  std::string message;
  std::string field;
};

std::optional<node_reference_blocker>
find_node_reference(const detail::context_control &context,
                    const detail::scene_state &state, node_id target) {
  if (std::ranges::find(state.roots, target) != state.roots.end())
    return node_reference_blocker{"node is retained as a scene root", {}};

  std::vector<std::uint64_t> owner_ids;
  owner_ids.reserve(state.nodes.size());
  for (const auto &[id, record] : state.nodes) {
    (void)record;
    owner_ids.push_back(id);
  }
  std::ranges::sort(owner_ids);
  for (const auto owner_id : owner_ids) {
    const auto &record = state.nodes.at(owner_id);
    const auto *type = context.registry.find(record.type_name);
    if (!type)
      continue;
    for (const auto &descriptor : type->fields) {
      if (descriptor.kind != value_kind::sf_node &&
          descriptor.kind != value_kind::mf_node)
        continue;
      const auto stored = record.fields.find(descriptor.name);
      if (stored == record.fields.end())
        continue;
      bool references = false;
      if (descriptor.kind == value_kind::sf_node) {
        references = std::get<node_id>(stored->second) == target;
      } else {
        const auto &nodes = std::get<node_list>(stored->second);
        references = std::ranges::find(nodes, target) != nodes.end();
      }
      if (references)
        return node_reference_blocker{"node is retained by a node-valued field",
                                      descriptor.name};
    }
  }

  const auto named =
      std::ranges::find(state.names, target, &name_binding::node);
  if (named != state.names.end())
    return node_reference_blocker{"node is retained by a name", named->name};
  const auto exported =
      std::ranges::find(state.exports, target, &export_binding::node);
  if (exported != state.exports.end())
    return node_reference_blocker{"node is retained by an export",
                                  exported->name};
  const auto routed =
      std::ranges::find_if(state.routes, [&](const route &item) {
        return item.source == target || item.sink == target;
      });
  if (routed != state.routes.end())
    return node_reference_blocker{"node is retained by a route endpoint",
                                  routed->source_field + " -> " +
                                      routed->sink_field};
  return std::nullopt;
}

std::optional<std::string> find_live_import_reference(
    const detail::context_control &source, node_id target,
    std::vector<std::shared_ptr<detail::context_control>> &retained_importers) {
  for (const auto &dependent : source.import_dependents) {
    if (dependent.target != target)
      continue;
    // Allocate the slot before acquiring an owner. It then outlives the
    // caller's lock even when allocation of the returned name or diagnostic
    // fails.
    retained_importers.emplace_back();
    auto &importer = retained_importers.back();
    importer = dependent.importer.lock();
    if (importer && importer->active.load(std::memory_order_acquire) &&
        importer->generation == dependent.importer_generation)
      return dependent.local_name;
  }
  return std::nullopt;
}

std::optional<node_reference_blocker>
find_live_context_reference(const detail::context_control &context,
                            node_id target) {
  for (const auto &[observer_id, observer] : context.event_observers) {
    (void)observer_id;
    if (observer.target == target)
      return node_reference_blocker{
          "node is retained by an active field observer", observer.field};
  }
  for (const auto &notification : context.pending) {
    const auto *event =
        std::get_if<detail::context_control::pending_event>(&notification);
    if (event && event->delivery.target == target)
      return node_reference_blocker{
          "node is retained by an undrained event notification",
          event->delivery.field};
    if (event) {
      const value &payload = event->delivery.payload;
      const auto *single = std::get_if<node_id>(&payload);
      const auto *many = std::get_if<node_list>(&payload);
      if ((single && *single == target) ||
          (many && std::ranges::find(*many, target) != many->end()))
        return node_reference_blocker{
            "node is carried by an undrained event notification",
            event->delivery.field};
    }
  }
  return std::nullopt;
}

struct interface_validation_issue {
  error_code code = error_code::invalid_descriptor;
  std::string message;
  std::string field;
};

std::optional<interface_validation_issue> validate_declaration_interface(
    const std::vector<interface_field_descriptor> &interface,
    bool external = false, bool allow_authored_node_defaults = false) {
  std::unordered_set<std::string> names;
  for (const auto &field : interface) {
    if (field.name.empty())
      return interface_validation_issue{
          .message = "interface field name must not be empty", .field = {}};
    if (field.name == "metadata")
      return interface_validation_issue{
          .message = "prototype interface cannot redeclare metadata",
          .field = field.name};
    if (!names.insert(field.name).second)
      return interface_validation_issue{
          .code = error_code::duplicate_field,
          .message = "interface field name is already defined: " + field.name,
          .field = field.name};
    if (external && field.default_value)
      return interface_validation_issue{
          .message = "external interface fields cannot declare defaults",
          .field = field.name};
    if (field.default_value && (field.access == access_type::input_only ||
                                field.access == access_type::output_only))
      return interface_validation_issue{
          .message = "event interface field cannot declare a default value",
          .field = field.name};
    if (field.default_value && !value_matches(field.kind, *field.default_value))
      return interface_validation_issue{
          .message =
              "interface default value violates its field kind or representation",
          .field = field.name};
    if (!allow_authored_node_defaults && field.default_value &&
        has_non_null_node_identity(*field.default_value))
      return interface_validation_issue{
          .message =
              "node interface default requires context-bearing authoring",
          .field = field.name};
    if (field.unit_category && (!field_unit_basis(*field.unit_category) ||
                                !unit_scalable_kind(field.kind)))
      return interface_validation_issue{
          .message = "interface unit category is unknown or incompatible",
          .field = field.name};
    if (!field.accepted_node_types.empty() &&
        field.kind != value_kind::sf_node && field.kind != value_kind::mf_node)
      return interface_validation_issue{
          .message = "node-type constraints require an SFNode or MFNode field",
          .field = field.name};
  }
  return std::nullopt;
}

bool compatible_interface_field(const interface_field_descriptor &external,
                                const interface_field_descriptor &local) {
  return external.kind == local.kind && external.access == local.access &&
         (external.accepted_node_types.empty() ||
          external.accepted_node_types == local.accepted_node_types) &&
         (!external.unit_category ||
          external.unit_category == local.unit_category);
}

bool compatible_declaration_interface(
    const std::vector<interface_field_descriptor> &external,
    const std::vector<interface_field_descriptor> &local) {
  return std::ranges::all_of(external, [&](const auto &field) {
    const auto match =
        std::ranges::find(local, field.name, &interface_field_descriptor::name);
    return match != local.end() && compatible_interface_field(field, *match);
  });
}

std::optional<std::string>
validate_external_resolution(const detail::scene_state &state,
                             const external_declaration_descriptor &external) {
  if (external.load_state != external_load_state::resolved) {
    if (external.resolved_declaration)
      return "only a resolved external may retain a declaration target";
    return std::nullopt;
  }
  if (!external.resolved_declaration)
    return "resolved external declaration has no target";
  const auto target =
      state.declarations.find(external.resolved_declaration->value);
  if (target == state.declarations.end())
    return "resolved external declaration target does not exist";
  const auto *local =
      std::get_if<local_declaration_descriptor>(&target->second.payload);
  if (!local)
    return "resolved external declaration target is not a local declaration";
  if (!compatible_declaration_interface(external.interface, local->interface))
    return "resolved external interface is incompatible with its local target";
  return std::nullopt;
}

bool retained_write_allowed(access_type access, bool node_is_in_setup) {
  switch (access) {
  case access_type::initialize_only:
    return node_is_in_setup;
  case access_type::input_only:
  case access_type::output_only:
    return false;
  case access_type::input_output:
    return true;
  }
  return false;
}

bool field_is_readable(access_type access) {
  return access != access_type::input_only;
}

bool field_is_writable(access_type access, write_intent intent,
                       bool node_is_in_setup) {
  switch (intent) {
  case write_intent::initialize:
    return node_is_in_setup && (access == access_type::initialize_only ||
                                access == access_type::input_output);
  case write_intent::retained:
    return access == access_type::input_output;
  case write_intent::event_input:
    return access == access_type::input_only ||
           access == access_type::input_output;
  case write_intent::runtime_output:
    return access == access_type::output_only ||
           access == access_type::input_output;
  }
  return false;
}

// Traverse node-valued fields in descriptor/element order, including shared
// and non-containment references. NULL and cycles do not create transitions.
bool realize_nodes(const detail::context_control &context,
                   detail::scene_state &state, const value &roots,
                   std::vector<semantic_change> &changes) {
  std::vector<node_id> pending;
  const auto enqueue = [&](const value &payload) {
    if (const auto *single = std::get_if<node_id>(&payload))
      pending.push_back(*single);
    else if (const auto *many = std::get_if<node_list>(&payload))
      pending.insert(pending.end(), many->begin(), many->end());
  };
  enqueue(roots);
  bool changed = false;
  for (std::size_t index = 0; index < pending.size(); ++index) {
    const auto id = pending[index];
    if (id.value == 0)
      continue;
    auto &record = state.nodes.at(id.value);
    // Every reference mutation realizes its new targets. A realized node's
    // existing graph therefore needs no second traversal, even for cycles.
    if (record.realized)
      continue;
    record.realized = true;
    changed = true;
    changes.push_back(graph_change{.kind = change_kind::node_realized,
                                   .node = id,
                                   .field = {},
                                   .before = false,
                                   .after = true,
                                   .index = 0});
    for (const auto &field : context.registry.find(record.type_name)->fields)
      enqueue(record.fields.at(field.name));
  }
  return changed;
}

template <class T> bool scale_unit_value(T &, double) { return false; }

bool scale_unit_value(float &stored, double factor) {
  const double scaled = static_cast<double>(stored) * factor;
  if (std::isfinite(stored) &&
      (!std::isfinite(scaled) || scaled > std::numeric_limits<float>::max() ||
       scaled < -std::numeric_limits<float>::max()))
    return false;
  stored = static_cast<float>(scaled);
  return true;
}

bool scale_unit_value(double &stored, double factor) {
  const double scaled = stored * factor;
  if (std::isfinite(stored) && !std::isfinite(scaled))
    return false;
  stored = scaled;
  return true;
}

#define X3D_SAI_SCALE_VECTOR(Type, ...)                                        \
  bool scale_unit_value(Type &stored, double factor) { return __VA_ARGS__; }
X3D_SAI_SCALE_VECTOR(vec2f, scale_unit_value(stored.x, factor) &&
                                scale_unit_value(stored.y, factor));
X3D_SAI_SCALE_VECTOR(vec3f, scale_unit_value(stored.x, factor) &&
                                scale_unit_value(stored.y, factor) &&
                                scale_unit_value(stored.z, factor));
X3D_SAI_SCALE_VECTOR(vec4f, scale_unit_value(stored.x, factor) &&
                                scale_unit_value(stored.y, factor) &&
                                scale_unit_value(stored.z, factor) &&
                                scale_unit_value(stored.w, factor));
X3D_SAI_SCALE_VECTOR(vec2d, scale_unit_value(stored.x, factor) &&
                                scale_unit_value(stored.y, factor));
X3D_SAI_SCALE_VECTOR(vec3d, scale_unit_value(stored.x, factor) &&
                                scale_unit_value(stored.y, factor) &&
                                scale_unit_value(stored.z, factor));
X3D_SAI_SCALE_VECTOR(vec4d, scale_unit_value(stored.x, factor) &&
                                scale_unit_value(stored.y, factor) &&
                                scale_unit_value(stored.z, factor) &&
                                scale_unit_value(stored.w, factor));
#undef X3D_SAI_SCALE_VECTOR

bool scale_unit_value(rotation &stored, double factor) {
  return scale_unit_value(stored.angle, factor);
}

template <class T, class Allocator>
bool scale_unit_value(std::vector<T, Allocator> &stored, double factor) {
  T probe{};
  if (!scale_unit_value(probe, 1.0))
    return false;
  for (auto &element : stored) {
    if (!scale_unit_value(element, factor))
      return false;
  }
  return true;
}

std::optional<value> convert_unit_value(const detail::scene_state &state,
                                        const field_descriptor &descriptor,
                                        value payload, value_space source,
                                        value_space destination) {
  if (source == destination || !descriptor.unit_category)
    return payload;
  if (kind_of(payload) != descriptor.kind ||
      !unit_scalable_kind(descriptor.kind))
    return std::nullopt;
  const auto basis = field_unit_basis(*descriptor.unit_category);
  if (!basis)
    return std::nullopt;
  const auto declaration = std::find_if(
      state.units.begin(), state.units.end(), [&](const auto &candidate) {
        return candidate.category == basis->category;
      });
  if (declaration == state.units.end() || declaration->conversion_factor == 1.0)
    return payload;
  double factor = declaration->conversion_factor;
  if (basis->power == 2)
    factor *= declaration->conversion_factor;
  else if (basis->power == 3)
    factor *= declaration->conversion_factor * declaration->conversion_factor;
  if (!std::isfinite(factor) || factor == 0.0)
    return std::nullopt;
  if (source == value_space::canonical)
    factor = 1.0 / factor;
  if (!std::isfinite(factor) || factor == 0.0)
    return std::nullopt;
  const bool converted = std::visit(
      [&](auto &stored) { return scale_unit_value(stored, factor); }, payload);
  if (!converted)
    return std::nullopt;
  return payload;
}

bool route_source_allowed(access_type access) {
  return access == access_type::output_only ||
         access == access_type::input_output;
}

bool route_sink_allowed(access_type access) {
  return access == access_type::input_only ||
         access == access_type::input_output;
}

std::optional<value_kind> element_kind(value_kind sequence) {
  switch (sequence) {
  case value_kind::mf_bool:
    return value_kind::sf_bool;
  case value_kind::mf_color:
    return value_kind::sf_color;
  case value_kind::mf_color_rgba:
    return value_kind::sf_color_rgba;
  case value_kind::mf_double:
    return value_kind::sf_double;
  case value_kind::mf_float:
    return value_kind::sf_float;
  case value_kind::mf_image:
    return value_kind::sf_image;
  case value_kind::mf_int32:
    return value_kind::sf_int32;
  case value_kind::mf_matrix3d:
    return value_kind::sf_matrix3d;
  case value_kind::mf_matrix3f:
    return value_kind::sf_matrix3f;
  case value_kind::mf_matrix4d:
    return value_kind::sf_matrix4d;
  case value_kind::mf_matrix4f:
    return value_kind::sf_matrix4f;
  case value_kind::mf_node:
    return value_kind::sf_node;
  case value_kind::mf_rotation:
    return value_kind::sf_rotation;
  case value_kind::mf_string:
    return value_kind::sf_string;
  case value_kind::mf_time:
    return value_kind::sf_time;
  case value_kind::mf_vec2d:
    return value_kind::sf_vec2d;
  case value_kind::mf_vec2f:
    return value_kind::sf_vec2f;
  case value_kind::mf_vec3d:
    return value_kind::sf_vec3d;
  case value_kind::mf_vec3f:
    return value_kind::sf_vec3f;
  case value_kind::mf_vec4d:
    return value_kind::sf_vec4d;
  case value_kind::mf_vec4f:
    return value_kind::sf_vec4f;
  case value_kind::mf_enum:
    return value_kind::sf_enum;
  default:
    return std::nullopt;
  }
}

template <class T> struct is_std_vector : std::false_type {};
template <class T, class Allocator>
struct is_std_vector<std::vector<T, Allocator>> : std::true_type {};
template <class T>
inline constexpr bool is_std_vector_v = is_std_vector<T>::value;

} // namespace

struct detail::scene_edit_control {
  std::shared_ptr<detail::context_control> context;
  std::shared_ptr<const detail::scene_state> base;
  detail::scene_state staged;
  bool changed = false;
  std::optional<unexpected> poison;
  std::vector<semantic_change> changes;
  std::unordered_set<std::uint64_t> removed_nodes;
  bool committed = false;
};

struct event_batch::impl {
  std::shared_ptr<detail::context_control> context;
  std::shared_ptr<const detail::scene_state> base;
  event_time time;
  std::optional<unexpected> poison;
  std::vector<event_delivery> seeds;
  write_intent intent = write_intent::event_input;
  const char *send_operation() const noexcept {
    return intent == write_intent::runtime_output ? "runtime_step.emit"
                                                  : "event_batch.send";
  }
  bool committed = false;
};

struct detail::runtime_step_control {
  std::mutex mutex;
  event_time stamp;
  generation_id generation = 0;
  revision_id revision = 0;
  std::optional<event_delivery> incoming;
  std::unique_ptr<event_batch> outputs;
  bool active = true;
  bool ignored = false;
};

namespace {
unexpected expired_runtime_step(const detail::runtime_step_control &step,
                                const char *operation) {
  auto error = descriptor_error(error_code::stale_revision, operation,
                                "runtime callback has finished");
  error.value().generation = step.generation;
  error.value().base_revision = step.revision;
  return error;
}

// Declared before the publication lock: return paths release that lock before
// this guard clears the reservation. Callbacks never execute under it.
struct event_processing_guard {
  std::shared_ptr<detail::context_control> context;
  bool armed = false;
  ~event_processing_guard() {
    if (!armed)
      return;
    std::lock_guard lock(context->mutex);
    context->processing = false;
  }
};
} // namespace

event_time runtime_step::time() const noexcept { return control_->stamp; }
const event_delivery *runtime_step::input() const noexcept {
  return control_->incoming ? &*control_->incoming : nullptr;
}

result<value> runtime_step::read(const dynamic_field &source,
                                 value_space space) const {
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return expired_runtime_step(*control_, "runtime_step.read");
  const auto &batch = *control_->outputs->impl_;
  // Validate local storage directly: a full snapshot would borrow unrelated
  // imported hosts and release their resources while holding the step mutex.
  auto stored = scene_snapshot::read_storage(batch.context, batch.base, source,
                                             "runtime_step.read");
  if (!stored)
    return failure(stored.error());
  result<value> current{**stored};
  if (!control_->ignored && control_->incoming &&
      control_->incoming->target == source.node() &&
      control_->incoming->field == source.name())
    current = control_->incoming->payload;
  if (space == value_space::canonical)
    return current;
  const auto &owner = batch.base->nodes.at(source.node().value);
  const auto *descriptor =
      find_descriptor(*batch.context, owner, source.name());
  auto converted =
      convert_unit_value(*batch.base, *descriptor, std::move(*current),
                         value_space::canonical, space);
  if (!converted)
    return edit_error(
        error_code::invalid_value, "runtime_step.read", *batch.context,
        batch.base->revision,
        "field representation cannot carry the declared unit category",
        source.node(), source.name());
  return std::move(*converted);
}

result<void> runtime_step::emit(const dynamic_field &target, value payload,
                                value_space space) {
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return expired_runtime_step(*control_, "runtime_step.emit");
  return control_->outputs->send(target, std::move(payload), space);
}
result<void> runtime_step::emit(const dynamic_field &target,
                                const node &payload) {
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return expired_runtime_step(*control_, "runtime_step.emit");
  return control_->outputs->send(target, payload);
}
result<void> runtime_step::emit(const dynamic_field &target,
                                std::span<const node> payload) {
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return expired_runtime_step(*control_, "runtime_step.emit");
  return control_->outputs->send(target, payload);
}
result<void> runtime_step::ignore_input() {
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return expired_runtime_step(*control_, "runtime_step.ignore_input");
  if (!control_->incoming) {
    auto &batch = *control_->outputs->impl_;
    batch.poison =
        edit_error(error_code::invalid_context, "runtime_step.ignore_input",
                   *batch.context, batch.base->revision,
                   "prepare callback has no input to ignore");
    return *batch.poison;
  }
  control_->ignored = true;
  return {};
}

scene_edit::scene_edit(
    std::shared_ptr<detail::scene_edit_control> implementation)
    : impl_(std::move(implementation)) {}
scene_edit::scene_edit(scene_edit &&) noexcept = default;
scene_edit &scene_edit::operator=(scene_edit &&) noexcept = default;
scene_edit::~scene_edit() = default;

event_batch::event_batch(std::unique_ptr<impl> implementation)
    : impl_(std::move(implementation)) {}
event_batch::event_batch(event_batch &&) noexcept = default;
event_batch &event_batch::operator=(event_batch &&) noexcept = default;
event_batch::~event_batch() = default;

subscription::subscription(subscription &&other) noexcept
    : context_(std::move(other.context_)), observer_id_(other.observer_id_) {
  other.observer_id_ = 0;
}

subscription &subscription::operator=(subscription &&other) noexcept {
  if (this == &other)
    return *this;
  cancel();
  context_ = std::move(other.context_);
  observer_id_ = other.observer_id_;
  other.observer_id_ = 0;
  return *this;
}

subscription::~subscription() { cancel(); }

bool subscription::active() const noexcept {
  const auto context = context_.lock();
  if (!context || observer_id_ == 0)
    return false;
  std::lock_guard lock(context->mutex);
  return context->observers.contains(observer_id_) ||
         context->event_observers.contains(observer_id_);
}

void subscription::cancel() noexcept {
  const auto context = context_.lock();
  // Extracting map nodes never allocates. Keep their callbacks alive until
  // after unlocking and resetting the subscription: capture destructors may
  // reenter the context or even cancel this subscription again.
  decltype(detail::context_control::observers)::node_type retired_observer;
  decltype(detail::context_control::event_observers)::node_type retired_event;
  if (context && observer_id_ != 0) {
    std::lock_guard lock(context->mutex);
    retired_observer = context->observers.extract(observer_id_);
    retired_event = context->event_observers.extract(observer_id_);
  }
  observer_id_ = 0;
  context_.reset();
}

result<void> scene_edit::require_schema_fingerprint(std::string_view expected) {
  if (impl_->poison)
    return *impl_->poison;
  const auto actual = impl_->context->registry.schema_fingerprint();
  if (!actual || *actual != expected) {
    impl_->poison = edit_error(
        error_code::invalid_descriptor, "scene_edit.create_generated",
        *impl_->context, impl_->base->revision,
        "generated node binding does not match the registry schema");
    return *impl_->poison;
  }
  return {};
}

result<node> scene_edit::create_node(const std::string &type_name) {
  if (impl_->poison)
    return *impl_->poison;
  const auto *type = impl_->context->registry.find(type_name);
  if (!type) {
    impl_->poison = edit_error(
        error_code::unknown_type, "scene_edit.create_node", *impl_->context,
        impl_->base->revision, "unknown node type: " + type_name);
    return *impl_->poison;
  }
  if (type->abstract) {
    impl_->poison =
        edit_error(error_code::abstract_type, "scene_edit.create_node",
                   *impl_->context, impl_->base->revision,
                   "abstract node type cannot be instantiated: " + type_name);
    return *impl_->poison;
  }
  const auto reserved = reserve_identity(impl_->context->next_node_id);
  if (!reserved) {
    impl_->poison = edit_error(
        error_code::invalid_value, "scene_edit.create_node", *impl_->context,
        impl_->base->revision, "node identity space is exhausted");
    return *impl_->poison;
  }
  const node_id id{*reserved};
  detail::scene_state::node_record record;
  record.type_name = type_name;
  for (const auto &field : type->fields) {
    record.fields.emplace(field.name, field.default_value);
  }
  impl_->staged.nodes.emplace(id.value, std::move(record));
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::node_created,
                                        .node = id,
                                        .field = {},
                                        .before = std::monostate{},
                                        .after = type_name,
                                        .index = 0});
  return node{impl_->context, impl_->context->generation, id};
}

result<void> scene_edit::realize(const node &target) {
  if (impl_->poison)
    return *impl_->poison;
  if (impl_->committed) {
    return edit_error(error_code::stale_revision, "scene_edit.realize",
                      *impl_->context, impl_->base->revision,
                      "scene edit was already committed", target.id_);
  }
  if (target.context_.lock() != impl_->context ||
      target.generation_ != impl_->context->generation ||
      !impl_->staged.nodes.contains(target.id_.value)) {
    impl_->poison = edit_error(error_code::stale_handle, "scene_edit.realize",
                               *impl_->context, impl_->base->revision,
                               "invalid node handle", target.id_);
    return *impl_->poison;
  }
  if (realize_nodes(*impl_->context, impl_->staged, value{target.id_},
                    impl_->changes))
    impl_->changed = true;
  return {};
}

result<void> scene_edit::remove_node(const node &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  const auto found = impl_->staged.nodes.find(target.id_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.nodes.end()) {
    impl_->poison = edit_error(
        error_code::stale_handle, "scene_edit.remove_node", *impl_->context,
        impl_->base->revision, "invalid node handle", target.id_);
    return *impl_->poison;
  }
  if (found->second.scope_owner) {
    impl_->poison =
        edit_error(error_code::node_in_use, "scene_edit.remove_node",
                   *impl_->context, impl_->base->revision,
                   "node is owned by a local declaration", target.id_);
    return *impl_->poison;
  }
  if (const auto blocker =
          find_node_reference(*impl_->context, impl_->staged, target.id_)) {
    impl_->poison = edit_error(
        error_code::node_in_use, "scene_edit.remove_node", *impl_->context,
        impl_->base->revision, blocker->message, target.id_, blocker->field);
    return *impl_->poison;
  }
  std::vector<std::shared_ptr<detail::context_control>> retained_importers;
  {
    std::lock_guard lock(impl_->context->mutex);
    if (const auto blocker =
            find_live_context_reference(*impl_->context, target.id_)) {
      impl_->poison = edit_error(
          error_code::node_in_use, "scene_edit.remove_node", *impl_->context,
          impl_->base->revision, blocker->message, target.id_, blocker->field);
      return *impl_->poison;
    }
    if (const auto imported = find_live_import_reference(
            *impl_->context, target.id_, retained_importers)) {
      impl_->poison =
          edit_error(error_code::node_in_use, "scene_edit.remove_node",
                     *impl_->context, impl_->base->revision,
                     "node is retained by a committed import aperture",
                     target.id_, *imported);
      return *impl_->poison;
    }
  }

  impl_->staged.nodes.erase(found);
  impl_->removed_nodes.insert(target.id_.value);
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::node_removed,
                                        .node = target.id_,
                                        .field = {},
                                        .before = target.id_,
                                        .after = std::monostate{},
                                        .index = 0});
  return {};
}

result<void> scene_edit::append_root(const node &child) {
  if (impl_->poison)
    return *impl_->poison;
  const auto child_context = child.context_.lock();
  if (child_context != impl_->context ||
      child.generation_ != impl_->context->generation ||
      !impl_->staged.nodes.contains(child.id_.value)) {
    impl_->poison = edit_error(
        error_code::stale_handle, "scene_edit.append_root", *impl_->context,
        impl_->base->revision, "invalid child handle", child.id_);
    return *impl_->poison;
  }
  if (impl_->staged.nodes.at(child.id_.value).scope_owner) {
    impl_->poison = edit_error(
        error_code::invalid_context, "scene_edit.append_root", *impl_->context,
        impl_->base->revision,
        "a template-scoped node cannot become a scene root", child.id_);
    return *impl_->poison;
  }
  impl_->staged.roots.push_back(child.id_);
  impl_->changed = true;
  impl_->changes.push_back(
      graph_change{.kind = change_kind::root_inserted,
                   .node = child.id_,
                   .field = {},
                   .before = std::monostate{},
                   .after = child.id_,
                   .index = impl_->staged.roots.size() - 1});
  realize_nodes(*impl_->context, impl_->staged, value{child.id_},
                impl_->changes);
  return {};
}

result<void> scene_edit::remove_root(std::size_t index) {
  if (impl_->poison)
    return *impl_->poison;
  if (index >= impl_->staged.roots.size()) {
    impl_->poison = edit_error(
        error_code::unknown_node, "scene_edit.remove_root", *impl_->context,
        impl_->base->revision, "root index is out of range");
    return *impl_->poison;
  }
  const node_id removed = impl_->staged.roots[index];
  impl_->staged.roots.erase(impl_->staged.roots.begin() +
                            static_cast<std::ptrdiff_t>(index));
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::root_removed,
                                        .node = removed,
                                        .field = {},
                                        .before = removed,
                                        .after = std::monostate{},
                                        .index = index});
  return {};
}

result<void> scene_edit::append(const node &parent, const std::string &field,
                                const node &child) {
  if (impl_->poison)
    return *impl_->poison;
  const auto parent_context = parent.context_.lock();
  const auto child_context = child.context_.lock();
  auto parent_record = impl_->staged.nodes.find(parent.id_.value);
  if (parent_context != impl_->context || child_context != impl_->context ||
      parent.generation_ != impl_->context->generation ||
      child.generation_ != impl_->context->generation ||
      parent_record == impl_->staged.nodes.end() ||
      !impl_->staged.nodes.contains(child.id_.value)) {
    impl_->poison =
        edit_error(error_code::stale_handle, "scene_edit.append",
                   *impl_->context, impl_->base->revision,
                   "invalid parent or child handle", parent.id_, field);
    return *impl_->poison;
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, parent_record->second, field);
  if (!descriptor) {
    impl_->poison = edit_error(error_code::unknown_field, "scene_edit.append",
                               *impl_->context, impl_->base->revision,
                               "unknown field: " + field, parent.id_, field);
    return *impl_->poison;
  }
  if (!descriptor->containment || descriptor->kind != value_kind::mf_node) {
    impl_->poison =
        edit_error(error_code::type_mismatch, "scene_edit.append",
                   *impl_->context, impl_->base->revision,
                   "field is not a containment node list", parent.id_, field);
    return *impl_->poison;
  }
  if (!retained_write_allowed(descriptor->access,
                              !parent_record->second.realized)) {
    impl_->poison = edit_error(error_code::access_denied, "scene_edit.append",
                               *impl_->context, impl_->base->revision,
                               "containment field is not author-writable in "
                               "this lifecycle phase",
                               parent.id_, field);
    return *impl_->poison;
  }
  const auto child_record = impl_->staged.nodes.find(child.id_.value);
  if (child_record == impl_->staged.nodes.end() ||
      !accepts_node_type(*impl_->context, *descriptor, child_record->second)) {
    impl_->poison =
        edit_error(error_code::type_mismatch, "scene_edit.append",
                   *impl_->context, impl_->base->revision,
                   "child type is not accepted by the field descriptor",
                   parent.id_, field);
    return *impl_->poison;
  }
  if (!same_node_scope(parent_record->second, child_record->second)) {
    impl_->poison =
        edit_error(error_code::invalid_context, "scene_edit.append",
                   *impl_->context, impl_->base->revision,
                   "containment edges cannot cross scene or declaration scope",
                   parent.id_, field);
    return *impl_->poison;
  }
  auto &field_value = parent_record->second.fields.at(field);
  const value before = field_value;
  auto &children = std::get<node_list>(field_value);
  children.push_back(child.id_);
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::field_changed,
                                        .node = parent.id_,
                                        .field = field,
                                        .before = before,
                                        .after = field_value,
                                        .index = children.size() - 1});
  realize_nodes(*impl_->context, impl_->staged, value{child.id_},
                impl_->changes);
  return {};
}

result<void> scene_edit::declare_unit(unit_declaration declaration) {
  if (impl_->poison)
    return *impl_->poison;
  if (!impl_->staged.nodes.empty()) {
    impl_->poison = edit_error(
        error_code::access_denied, "scene_edit.declare_unit", *impl_->context,
        impl_->base->revision,
        "unit declarations are fixed before scene content is authored");
    return *impl_->poison;
  }
  if (declaration.category.empty() || declaration.name.empty() ||
      !standard_unit_category(declaration.category) ||
      !std::isfinite(declaration.conversion_factor) ||
      declaration.conversion_factor <= 0.0) {
    impl_->poison = edit_error(
        error_code::invalid_value, "scene_edit.declare_unit", *impl_->context,
        impl_->base->revision,
        "unit declarations require names and a finite positive factor");
    return *impl_->poison;
  }
  const auto duplicate = std::ranges::find(
      impl_->staged.units, declaration.category, &unit_declaration::category);
  if (duplicate != impl_->staged.units.end()) {
    impl_->poison = edit_error(
        error_code::invalid_value, "scene_edit.declare_unit", *impl_->context,
        impl_->base->revision, "unit category is already declared");
    return *impl_->poison;
  }
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::unit_declared,
                                        .node = {},
                                        .field = declaration.category,
                                        .before = declaration.name,
                                        .after = declaration.conversion_factor,
                                        .index = impl_->staged.units.size()});
  impl_->staged.units.push_back(std::move(declaration));
  return {};
}

result<void> scene_edit::set_metadata(std::string key,
                                      std::optional<std::string> data) {
  if (impl_->poison)
    return *impl_->poison;
  auto &entries = impl_->staged.metadata;
  const auto found = std::ranges::find(entries, key, &metadata_entry::key);
  const bool present = found != entries.end();
  std::optional<std::string> before;
  if (present)
    before = found->value;
  if (before == data)
    return {};

  impl_->changes.push_back(metadata_change{.key = key,
                                            .before = std::move(before),
                                            .after = data});
  if (data) {
    if (present) {
      found->value = std::move(*data);
    } else {
      try {
        entries.push_back(metadata_entry{std::move(key), std::move(*data)});
      } catch (...) {
        // Preserve the staged change trace if allocating a new entry fails.
        impl_->changes.pop_back();
        throw;
      }
    }
  } else {
    entries.erase(found);
  }
  impl_->changed = true;
  return {};
}

result<dynamic_field> scene_edit::field(const node &owner,
                                        const std::string &name) const {
  const auto owner_context = owner.context_.lock();
  const auto found = impl_->staged.nodes.find(owner.id_.value);
  if (owner_context != impl_->context ||
      owner.generation_ != impl_->context->generation ||
      found == impl_->staged.nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_edit.field",
                      *impl_->context, impl_->base->revision,
                      "invalid node handle", owner.id_, name);
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, name);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "scene_edit.field",
                      *impl_->context, impl_->base->revision,
                      "unknown field: " + name, owner.id_, name);
  }
  return dynamic_field{impl_->context,   impl_->context->generation,
                       owner.id_,        name,
                       descriptor->kind, descriptor->access};
}

result<bool> scene_edit::writable(const dynamic_field &target,
                                  write_intent intent) const {
  if (impl_->poison)
    return *impl_->poison;
  if (impl_->committed) {
    return edit_error(error_code::stale_revision, "scene_edit.writable",
                      *impl_->context, impl_->base->revision,
                      "scene edit was already committed", target.node(),
                      target.name());
  }
  {
    std::lock_guard lock(impl_->context->mutex);
    if (!impl_->context->active) {
      return edit_error(error_code::stale_handle, "scene_edit.writable",
                        *impl_->context, impl_->base->revision,
                        "execution context is no longer active", target.node(),
                        target.name());
    }
    if (impl_->context->published_revision.load(std::memory_order_acquire) !=
        impl_->base->revision) {
      return edit_error(error_code::stale_revision, "scene_edit.writable",
                        *impl_->context, impl_->base->revision,
                        "edit base revision is stale", target.node(),
                        target.name());
    }
  }
  const auto target_context = target.context_.lock();
  const auto found = impl_->staged.nodes.find(target.node_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_edit.writable",
                      *impl_->context, impl_->base->revision,
                      "invalid field handle", target.node_, target.name_);
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, target.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "scene_edit.writable",
                      *impl_->context, impl_->base->revision,
                      "unknown field: " + target.name_, target.node_,
                      target.name_);
  }
  if (intent == write_intent::event_input ||
      intent == write_intent::runtime_output)
    return false;
  return field_is_writable(descriptor->access, intent, !found->second.realized);
}

namespace {

result<std::shared_ptr<detail::scene_edit_control>>
active_multi_edit(const std::weak_ptr<detail::scene_edit_control> &weak,
                  const char *operation, node_id node,
                  const std::string &field) {
  auto edit = weak.lock();
  if (!edit) {
    sai_error error;
    error.code = error_code::stale_revision;
    error.operation = operation;
    error.message = "multi-field editor no longer has an owning transaction";
    error.node = node;
    error.field = field;
    return failure(std::move(error));
  }
  if (edit->poison)
    return *edit->poison;
  if (edit->committed) {
    return edit_error(error_code::stale_revision, operation, *edit->context,
                      edit->base->revision,
                      "multi-field editor is bound to a stale revision", node,
                      field);
  }
  std::lock_guard lock(edit->context->mutex);
  if (!edit->context->active) {
    return edit_error(error_code::stale_handle, operation, *edit->context,
                      edit->base->revision,
                      "multi-field editor belongs to a retired context", node,
                      field);
  }
  if (edit->context->published_revision.load(std::memory_order_acquire) !=
      edit->base->revision) {
    return edit_error(error_code::stale_revision, operation, *edit->context,
                      edit->base->revision,
                      "multi-field editor is bound to a stale revision", node,
                      field);
  }
  return edit;
}

result<std::shared_ptr<detail::scene_edit_control>>
writable_multi_edit(const std::weak_ptr<detail::scene_edit_control> &weak,
                    const char *operation, node_id node,
                    const std::string &field) {
  auto edit = active_multi_edit(weak, operation, node, field);
  if (!edit)
    return failure(edit.error());
  const auto owner = edit.value()->staged.nodes.find(node.value);
  const auto *descriptor =
      owner == edit.value()->staged.nodes.end()
          ? nullptr
          : find_descriptor(*edit.value()->context, owner->second, field);
  if (!descriptor ||
      !retained_write_allowed(descriptor->access, !owner->second.realized)) {
    edit.value()->poison = edit_error(
        descriptor ? error_code::access_denied : error_code::stale_handle,
        operation, *edit.value()->context, edit.value()->base->revision,
        "multi-field is not author-writable in this lifecycle phase", node,
        field);
    return *edit.value()->poison;
  }
  return edit;
}

value *multi_sequence(detail::scene_edit_control &edit,
                      const dynamic_field &target) {
  const auto found = edit.staged.nodes.find(target.node().value);
  if (found == edit.staged.nodes.end())
    return nullptr;
  const auto value_found = found->second.fields.find(target.name());
  return value_found == found->second.fields.end() ? nullptr
                                                   : &value_found->second;
}

std::size_t sequence_size(const value &sequence) {
  return std::visit(
      []<class T>(const T &stored) -> std::size_t {
        if constexpr (is_std_vector_v<T> || std::is_same_v<T, bool_list>)
          return stored.size();
        return 0;
      },
      sequence);
}

std::optional<value> sequence_element(const value &sequence,
                                      std::size_t index) {
  return std::visit(
      [index]<class T>(const T &stored) -> std::optional<value> {
        if constexpr (std::is_same_v<T, bool_list>) {
          if (index < stored.size())
            return value{stored[index]};
        } else if constexpr (is_std_vector_v<T>) {
          if (index < stored.size())
            return value{stored[index]};
        }
        return std::nullopt;
      },
      sequence);
}

enum class element_mutation { set, insert };

bool mutate_element(value &sequence, std::size_t index, const value &element,
                    element_mutation mutation) {
  return std::visit(
      [&](auto &stored) {
        using sequence_type = std::remove_cvref_t<decltype(stored)>;
        if constexpr (std::is_same_v<sequence_type, bool_list>) {
          if (!std::holds_alternative<bool>(element))
            return false;
          if (mutation == element_mutation::set) {
            if (index >= stored.size())
              return false;
            stored.set(index, std::get<bool>(element));
          } else {
            if (index > stored.size())
              return false;
            stored.insert(index, std::get<bool>(element));
          }
          return true;
        } else if constexpr (is_std_vector_v<sequence_type>) {
          using element_type = typename sequence_type::value_type;
          if (!std::holds_alternative<element_type>(element))
            return false;
          if (mutation == element_mutation::set) {
            if (index >= stored.size())
              return false;
            stored[index] = std::get<element_type>(element);
          } else {
            if (index > stored.size())
              return false;
            stored.insert(stored.begin() + static_cast<std::ptrdiff_t>(index),
                          std::get<element_type>(element));
          }
          return true;
        }
        return false;
      },
      sequence);
}

} // namespace

result<dynamic_multi_field_edit>
scene_edit::multi(const dynamic_field &target) {
  if (impl_->committed) {
    return edit_error(error_code::stale_revision, "scene_edit.multi",
                      *impl_->context, impl_->base->revision,
                      "transaction was already committed", target.node(),
                      target.name());
  }
  const auto target_context = target.context_.lock();
  const auto found = impl_->staged.nodes.find(target.node_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_edit.multi",
                      *impl_->context, impl_->base->revision,
                      "invalid multi-field handle", target.node_, target.name_);
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, target.name_);
  if (!descriptor || !element_kind(descriptor->kind)) {
    return edit_error(error_code::type_mismatch, "scene_edit.multi",
                      *impl_->context, impl_->base->revision,
                      "field is not a multi-field", target.node_, target.name_);
  }
  if (!retained_write_allowed(descriptor->access, !found->second.realized)) {
    return edit_error(error_code::access_denied, "scene_edit.multi",
                      *impl_->context, impl_->base->revision,
                      "multi-field is not author-writable", target.node_,
                      target.name_);
  }
  return dynamic_multi_field_edit{impl_, target};
}

result<std::size_t> dynamic_multi_field_edit::size() const {
  auto edit = active_multi_edit(edit_, "multi_field.size", target_.node(),
                                target_.name());
  if (!edit)
    return failure(edit.error());
  const auto *sequence = multi_sequence(*edit.value(), target_);
  if (!sequence)
    return edit_error(error_code::unknown_field, "multi_field.size",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field no longer exists", target_.node(),
                      target_.name());
  return sequence_size(*sequence);
}

result<value> dynamic_multi_field_edit::at(std::size_t index) const {
  auto edit = active_multi_edit(edit_, "multi_field.at", target_.node(),
                                target_.name());
  if (!edit)
    return failure(edit.error());
  const auto *sequence = multi_sequence(*edit.value(), target_);
  const auto element =
      sequence ? sequence_element(*sequence, index) : std::optional<value>{};
  if (!element)
    return edit_error(error_code::index_out_of_range, "multi_field.at",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field index is out of range", target_.node(),
                      target_.name());
  return *element;
}

result<node> dynamic_multi_field_edit::node_at(std::size_t index) const {
  auto edit = active_multi_edit(edit_, "multi_field.node_at", target_.node(),
                                target_.name());
  if (!edit)
    return failure(edit.error());
  if (target_.kind() != value_kind::mf_node) {
    return edit_error(error_code::type_mismatch, "multi_field.node_at",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field does not contain nodes", target_.node(),
                      target_.name());
  }
  const auto *sequence = multi_sequence(*edit.value(), target_);
  const auto element =
      sequence ? sequence_element(*sequence, index) : std::optional<value>{};
  if (!element) {
    return edit_error(error_code::index_out_of_range, "multi_field.node_at",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field index is out of range", target_.node(),
                      target_.name());
  }
  const node_id id = std::get<node_id>(*element);
  if (id.value == 0)
    return node{};
  if (!edit.value()->staged.nodes.contains(id.value)) {
    return edit_error(error_code::stale_handle, "multi_field.node_at",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field contains a dangling node identity", id,
                      target_.name());
  }
  return node{edit.value()->context, edit.value()->context->generation, id};
}

result<void> dynamic_multi_field_edit::set(std::size_t index, value element) {
  auto edit = writable_multi_edit(edit_, "multi_field.set", target_.node(),
                                  target_.name());
  if (!edit)
    return failure(edit.error());
  const auto expected = element_kind(target_.kind());
  if (target_.kind() == value_kind::mf_node) {
    edit.value()->poison =
        edit_error(error_code::invalid_context, "multi_field.set",
                   *edit.value()->context, edit.value()->base->revision,
                   "MFNode edits require context-bearing node handles",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  const bool matches_kind = expected && kind_of(element) == expected;
  if (!matches_kind || !value_is_well_formed(element)) {
    edit.value()->poison =
        edit_error(matches_kind ? error_code::invalid_value
                                : error_code::type_mismatch,
                   "multi_field.set",
                   *edit.value()->context, edit.value()->base->revision,
                   "element does not satisfy the multi-field descriptor",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  auto *sequence = multi_sequence(*edit.value(), target_);
  const auto before =
      sequence ? sequence_element(*sequence, index) : std::optional<value>{};
  if (!sequence || !before ||
      !mutate_element(*sequence, index, element, element_mutation::set)) {
    edit.value()->poison = edit_error(
        error_code::index_out_of_range, "multi_field.set",
        *edit.value()->context, edit.value()->base->revision,
        "multi-field index is out of range", target_.node(), target_.name());
    return *edit.value()->poison;
  }
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_element_set,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = *before,
                   .after = std::move(element),
                   .index = index});
  return {};
}

result<void> dynamic_multi_field_edit::set(std::size_t index,
                                           const node &element) {
  auto edit = writable_multi_edit(edit_, "multi_field.set_node", target_.node(),
                                  target_.name());
  if (!edit)
    return failure(edit.error());
  const bool null_element = element.is_null() && !element.disposed();
  const auto element_context = element.context_.lock();
  const auto candidate = edit.value()->staged.nodes.find(element.id_.value);
  const auto owner = edit.value()->staged.nodes.find(target_.node().value);
  const auto *descriptor = owner == edit.value()->staged.nodes.end()
                               ? nullptr
                               : find_descriptor(*edit.value()->context,
                                                 owner->second, target_.name());
  if (target_.kind() != value_kind::mf_node ||
      (!null_element &&
       (element_context != edit.value()->context ||
        element.generation_ != edit.value()->context->generation ||
        candidate == edit.value()->staged.nodes.end()))) {
    edit.value()->poison =
        edit_error(error_code::invalid_context, "multi_field.set_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "MFNode element is not a live handle from this context",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  if (!descriptor ||
      (!null_element && !accepts_node_type(*edit.value()->context, *descriptor,
                                           candidate->second))) {
    edit.value()->poison =
        edit_error(error_code::type_mismatch, "multi_field.set_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "node type is not accepted by the MFNode descriptor",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  if (!null_element && !same_node_scope(owner->second, candidate->second)) {
    edit.value()->poison =
        edit_error(error_code::invalid_context, "multi_field.set_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "MFNode element cannot cross scene or declaration scope",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  auto *sequence = multi_sequence(*edit.value(), target_);
  const auto before =
      sequence ? sequence_element(*sequence, index) : std::optional<value>{};
  const value authorized{element.id_};
  if (!sequence || !before ||
      !mutate_element(*sequence, index, authorized, element_mutation::set)) {
    edit.value()->poison = edit_error(
        error_code::index_out_of_range, "multi_field.set_node",
        *edit.value()->context, edit.value()->base->revision,
        "multi-field index is out of range", target_.node(), target_.name());
    return *edit.value()->poison;
  }
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_element_set,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = *before,
                   .after = authorized,
                   .index = index});
  realize_nodes(*edit.value()->context, edit.value()->staged, authorized,
                edit.value()->changes);
  return {};
}

result<void> dynamic_multi_field_edit::insert(std::size_t index,
                                              value element) {
  auto edit = writable_multi_edit(edit_, "multi_field.insert", target_.node(),
                                  target_.name());
  if (!edit)
    return failure(edit.error());
  const auto expected = element_kind(target_.kind());
  const bool matches_kind = expected && kind_of(element) == expected;
  if (target_.kind() == value_kind::mf_node || !matches_kind ||
      !value_is_well_formed(element)) {
    edit.value()->poison = edit_error(
        target_.kind() == value_kind::mf_node ? error_code::invalid_context
        : matches_kind ? error_code::invalid_value : error_code::type_mismatch,
        "multi_field.insert", *edit.value()->context,
        edit.value()->base->revision,
        "element is not authorizable for the multi-field", target_.node(),
        target_.name());
    return *edit.value()->poison;
  }
  auto *sequence = multi_sequence(*edit.value(), target_);
  if (!sequence ||
      !mutate_element(*sequence, index, element, element_mutation::insert)) {
    edit.value()->poison =
        edit_error(error_code::index_out_of_range, "multi_field.insert",
                   *edit.value()->context, edit.value()->base->revision,
                   "multi-field insertion index is out of range",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_inserted,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = std::monostate{},
                   .after = std::move(element),
                   .index = index});
  return {};
}

result<void> dynamic_multi_field_edit::insert(std::size_t index,
                                              const node &element) {
  auto edit = writable_multi_edit(edit_, "multi_field.insert_node",
                                  target_.node(), target_.name());
  if (!edit)
    return failure(edit.error());
  const bool null_element = element.is_null() && !element.disposed();
  const auto element_context = element.context_.lock();
  const auto candidate = edit.value()->staged.nodes.find(element.id_.value);
  const auto owner = edit.value()->staged.nodes.find(target_.node().value);
  const auto *descriptor = owner == edit.value()->staged.nodes.end()
                               ? nullptr
                               : find_descriptor(*edit.value()->context,
                                                 owner->second, target_.name());
  if (target_.kind() != value_kind::mf_node ||
      (!null_element &&
       (element_context != edit.value()->context ||
        element.generation_ != edit.value()->context->generation ||
        candidate == edit.value()->staged.nodes.end()))) {
    edit.value()->poison =
        edit_error(error_code::invalid_context, "multi_field.insert_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "MFNode element is not a live handle from this context",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  if (!descriptor ||
      (!null_element && !accepts_node_type(*edit.value()->context, *descriptor,
                                           candidate->second))) {
    edit.value()->poison =
        edit_error(error_code::type_mismatch, "multi_field.insert_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "node type is not accepted by the MFNode descriptor",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  if (!null_element && !same_node_scope(owner->second, candidate->second)) {
    edit.value()->poison =
        edit_error(error_code::invalid_context, "multi_field.insert_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "MFNode element cannot cross scene or declaration scope",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  auto *sequence = multi_sequence(*edit.value(), target_);
  const value authorized{element.id_};
  if (!sequence ||
      !mutate_element(*sequence, index, authorized, element_mutation::insert)) {
    edit.value()->poison =
        edit_error(error_code::index_out_of_range, "multi_field.insert_node",
                   *edit.value()->context, edit.value()->base->revision,
                   "multi-field insertion index is out of range",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_inserted,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = std::monostate{},
                   .after = authorized,
                   .index = index});
  realize_nodes(*edit.value()->context, edit.value()->staged, authorized,
                edit.value()->changes);
  return {};
}

result<void> dynamic_multi_field_edit::append(value element) {
  auto current_size = size();
  if (!current_size)
    return failure(current_size.error());
  return insert(current_size.value(), std::move(element));
}

result<void> dynamic_multi_field_edit::append(const node &element) {
  auto current_size = size();
  if (!current_size)
    return failure(current_size.error());
  return insert(current_size.value(), element);
}

result<void> dynamic_multi_field_edit::erase(std::size_t index) {
  auto edit = writable_multi_edit(edit_, "multi_field.erase", target_.node(),
                                  target_.name());
  if (!edit)
    return failure(edit.error());
  auto *sequence = multi_sequence(*edit.value(), target_);
  const auto before =
      sequence ? sequence_element(*sequence, index) : std::optional<value>{};
  bool erased = false;
  if (sequence && before) {
    erased = std::visit(
        [&](auto &stored) {
          using sequence_type = std::remove_cvref_t<decltype(stored)>;
          if constexpr (std::is_same_v<sequence_type, bool_list>) {
            stored.erase(index);
            return true;
          } else if constexpr (is_std_vector_v<sequence_type>) {
            stored.erase(stored.begin() + static_cast<std::ptrdiff_t>(index));
            return true;
          }
          return false;
        },
        *sequence);
  }
  if (!erased) {
    edit.value()->poison =
        edit_error(error_code::index_out_of_range, "multi_field.erase",
                   *edit.value()->context, edit.value()->base->revision,
                   "multi-field erase index is out of range", target_.node(),
                   target_.name());
    return *edit.value()->poison;
  }
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_erased,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = *before,
                   .after = std::monostate{},
                   .index = index});
  return {};
}

result<void> dynamic_multi_field_edit::clear() {
  auto edit = writable_multi_edit(edit_, "multi_field.clear", target_.node(),
                                  target_.name());
  if (!edit)
    return failure(edit.error());
  auto *sequence = multi_sequence(*edit.value(), target_);
  if (!sequence)
    return edit_error(error_code::unknown_field, "multi_field.clear",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field no longer exists", target_.node(),
                      target_.name());
  const value before = *sequence;
  std::visit(
      [](auto &stored) {
        using sequence_type = std::remove_cvref_t<decltype(stored)>;
        if constexpr (is_std_vector_v<sequence_type> ||
                      std::is_same_v<sequence_type, bool_list>)
          stored.clear();
      },
      *sequence);
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_cleared,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = before,
                   .after = *sequence,
                   .index = 0});
  return {};
}

result<void> dynamic_multi_field_edit::replace(value replacement) {
  auto edit = writable_multi_edit(edit_, "multi_field.replace", target_.node(),
                                  target_.name());
  if (!edit)
    return failure(edit.error());
  if (target_.kind() == value_kind::mf_node) {
    edit.value()->poison =
        edit_error(error_code::invalid_context, "multi_field.replace",
                   *edit.value()->context, edit.value()->base->revision,
                   "MFNode replacement requires context-bearing node handles",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  const bool matches_kind = kind_of(replacement) == target_.kind();
  if (!matches_kind || !value_is_well_formed(replacement)) {
    edit.value()->poison =
        edit_error(matches_kind ? error_code::invalid_value
                                : error_code::type_mismatch,
                   "multi_field.replace",
                   *edit.value()->context, edit.value()->base->revision,
                   "replacement does not satisfy the multi-field descriptor",
                   target_.node(), target_.name());
    return *edit.value()->poison;
  }
  auto *sequence = multi_sequence(*edit.value(), target_);
  if (!sequence)
    return edit_error(error_code::unknown_field, "multi_field.replace",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field no longer exists", target_.node(),
                      target_.name());
  const value before = *sequence;
  *sequence = std::move(replacement);
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_replaced,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = before,
                   .after = *sequence,
                   .index = 0});
  return {};
}

result<void>
dynamic_multi_field_edit::replace(std::span<const node> replacement) {
  auto edit = writable_multi_edit(edit_, "multi_field.replace_nodes",
                                  target_.node(), target_.name());
  if (!edit)
    return failure(edit.error());
  const auto owner = edit.value()->staged.nodes.find(target_.node().value);
  const auto *descriptor = owner == edit.value()->staged.nodes.end()
                               ? nullptr
                               : find_descriptor(*edit.value()->context,
                                                 owner->second, target_.name());
  if (target_.kind() != value_kind::mf_node || !descriptor) {
    edit.value()->poison = edit_error(
        error_code::type_mismatch, "multi_field.replace_nodes",
        *edit.value()->context, edit.value()->base->revision,
        "field is not an MFNode descriptor", target_.node(), target_.name());
    return *edit.value()->poison;
  }

  node_list authorized;
  authorized.reserve(replacement.size());
  for (const node &element : replacement) {
    if (element.is_null() && !element.disposed()) {
      authorized.push_back(node_id{});
      continue;
    }
    const auto element_context = element.context_.lock();
    const auto candidate = edit.value()->staged.nodes.find(element.id_.value);
    if (element_context != edit.value()->context ||
        element.generation_ != edit.value()->context->generation ||
        candidate == edit.value()->staged.nodes.end()) {
      edit.value()->poison =
          edit_error(error_code::invalid_context, "multi_field.replace_nodes",
                     *edit.value()->context, edit.value()->base->revision,
                     "MFNode range contains a handle from another context",
                     target_.node(), target_.name());
      return *edit.value()->poison;
    }
    if (!accepts_node_type(*edit.value()->context, *descriptor,
                           candidate->second)) {
      edit.value()->poison =
          edit_error(error_code::type_mismatch, "multi_field.replace_nodes",
                     *edit.value()->context, edit.value()->base->revision,
                     "MFNode range contains a type rejected by the descriptor",
                     target_.node(), target_.name());
      return *edit.value()->poison;
    }
    if (!same_node_scope(owner->second, candidate->second)) {
      edit.value()->poison =
          edit_error(error_code::invalid_context, "multi_field.replace_nodes",
                     *edit.value()->context, edit.value()->base->revision,
                     "MFNode range cannot cross scene or declaration scope",
                     target_.node(), target_.name());
      return *edit.value()->poison;
    }
    authorized.push_back(element.id_);
  }

  auto *sequence = multi_sequence(*edit.value(), target_);
  if (!sequence) {
    return edit_error(error_code::unknown_field, "multi_field.replace_nodes",
                      *edit.value()->context, edit.value()->base->revision,
                      "multi-field no longer exists", target_.node(),
                      target_.name());
  }
  const value before = *sequence;
  *sequence = std::move(authorized);
  edit.value()->changed = true;
  edit.value()->changes.push_back(
      graph_change{.kind = change_kind::multi_replaced,
                   .node = target_.node(),
                   .field = target_.name(),
                   .before = before,
                   .after = *sequence,
                   .index = 0});
  realize_nodes(*edit.value()->context, edit.value()->staged, *sequence,
                edit.value()->changes);
  return {};
}

result<void> scene_edit::set(const dynamic_field &target, value new_value) {
  return set_value(target, std::move(new_value), false);
}

result<void> scene_edit::set(const dynamic_field &target, value new_value,
                             value_space space) {
  return set_value(target, std::move(new_value), false, space);
}

result<void> scene_edit::set(const dynamic_field &target,
                             const node &new_value) {
  if (impl_->poison)
    return *impl_->poison;
  if (new_value.is_null() && !new_value.disposed())
    return set_value(target, value{node_id{}}, true);
  const auto value_context = new_value.context_.lock();
  if (value_context != impl_->context ||
      new_value.generation_ != impl_->context->generation ||
      !impl_->staged.nodes.contains(new_value.id_.value)) {
    impl_->poison =
        edit_error(error_code::invalid_context, "scene_edit.set",
                   *impl_->context, impl_->base->revision,
                   "node value belongs to another context", new_value.id_);
    return *impl_->poison;
  }
  return set_value(target, value{new_value.id_}, true);
}

result<void> scene_edit::set(const dynamic_field &target,
                             std::span<const node> new_value) {
  if (impl_->poison)
    return *impl_->poison;
  node_list ids;
  ids.reserve(new_value.size());
  for (const node &candidate : new_value) {
    if (candidate.is_null() && !candidate.disposed()) {
      ids.push_back(node_id{});
      continue;
    }
    const auto value_context = candidate.context_.lock();
    if (value_context != impl_->context ||
        candidate.generation_ != impl_->context->generation ||
        !impl_->staged.nodes.contains(candidate.id_.value)) {
      impl_->poison =
          edit_error(error_code::invalid_context, "scene_edit.set",
                     *impl_->context, impl_->base->revision,
                     "node range contains another context", candidate.id_);
      return *impl_->poison;
    }
    ids.push_back(candidate.id_);
  }
  return set_value(target, value{std::move(ids)}, true);
}

result<void> scene_edit::set_value(const dynamic_field &target, value new_value,
                                   bool node_authority_checked,
                                   value_space space) {
  if (impl_->poison)
    return *impl_->poison;
  auto found = impl_->staged.nodes.find(target.node_.value);
  if (!same_owner(target.context_, impl_->context) ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.nodes.end()) {
    impl_->poison =
        edit_error(error_code::stale_handle, "scene_edit.set", *impl_->context,
                   impl_->base->revision, "invalid field handle", target.node_,
                   target.name_);
    return *impl_->poison;
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, target.name_);
  if (!descriptor) {
    impl_->poison =
        edit_error(error_code::unknown_field, "scene_edit.set", *impl_->context,
                   impl_->base->revision, "unknown field: " + target.name_,
                   target.node_, target.name_);
    return *impl_->poison;
  }
  if (!retained_write_allowed(descriptor->access, !found->second.realized)) {
    impl_->poison = edit_error(error_code::access_denied, "scene_edit.set",
                               *impl_->context, impl_->base->revision,
                               "field is not author-writable in this lifecycle "
                               "phase",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if ((descriptor->kind == value_kind::sf_node ||
       descriptor->kind == value_kind::mf_node) &&
      !node_authority_checked) {
    impl_->poison =
        edit_error(error_code::invalid_context, "scene_edit.set",
                   *impl_->context, impl_->base->revision,
                   "node-valued writes require context-bearing node handles",
                   target.node_, target.name_);
    return *impl_->poison;
  }
  if (kind_of(new_value) != descriptor->kind) {
    impl_->poison = edit_error(error_code::type_mismatch, "scene_edit.set",
                               *impl_->context, impl_->base->revision,
                               "value does not match field descriptor",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if (space != value_space::canonical) {
    auto canonical =
        convert_unit_value(impl_->staged, *descriptor, std::move(new_value),
                           space, value_space::canonical);
    if (!canonical) {
      impl_->poison = edit_error(
          error_code::invalid_value, "scene_edit.set", *impl_->context,
          impl_->base->revision,
          "field representation cannot carry the declared unit category",
          target.node_, target.name_);
      return *impl_->poison;
    }
    new_value = std::move(*canonical);
  }
  if (!value_is_well_formed(new_value)) {
    impl_->poison = edit_error(error_code::invalid_value, "scene_edit.set",
                               *impl_->context, impl_->base->revision,
                               "value violates field representation invariants",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if (node_authority_checked && descriptor->kind == value_kind::sf_node &&
      std::get<node_id>(new_value).value != 0) {
    const auto id = std::get<node_id>(new_value);
    const auto candidate = impl_->staged.nodes.find(id.value);
    if (candidate == impl_->staged.nodes.end() ||
        !accepts_node_type(*impl_->context, *descriptor, candidate->second)) {
      impl_->poison =
          edit_error(error_code::type_mismatch, "scene_edit.set",
                     *impl_->context, impl_->base->revision,
                     "node type is not accepted by the field descriptor",
                     target.node_, target.name_);
      return *impl_->poison;
    }
    if (id.value != 0 &&
        candidate->second.scope_owner != found->second.scope_owner) {
      impl_->poison = edit_error(
          error_code::invalid_context, "scene_edit.set", *impl_->context,
          impl_->base->revision,
          "node-valued fields cannot cross scene or declaration scope",
          target.node_, target.name_);
      return *impl_->poison;
    }
  }
  if (node_authority_checked && descriptor->kind == value_kind::mf_node) {
    for (const auto id : std::get<node_list>(new_value)) {
      if (id.value == 0)
        continue;
      const auto candidate = impl_->staged.nodes.find(id.value);
      if (candidate == impl_->staged.nodes.end() ||
          !accepts_node_type(*impl_->context, *descriptor, candidate->second)) {
        impl_->poison = edit_error(
            error_code::type_mismatch, "scene_edit.set", *impl_->context,
            impl_->base->revision,
            "node range contains a type not accepted by the field descriptor",
            target.node_, target.name_);
        return *impl_->poison;
      }
      if (candidate->second.scope_owner != found->second.scope_owner) {
        impl_->poison =
            edit_error(error_code::invalid_context, "scene_edit.set",
                       *impl_->context, impl_->base->revision,
                       "node range cannot cross scene or declaration scope",
                       target.node_, target.name_);
        return *impl_->poison;
      }
    }
  }
  auto &stored = found->second.fields.at(target.name_);
  const value before = stored;
  stored = std::move(new_value);
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::field_changed,
                                        .node = target.node_,
                                        .field = target.name_,
                                        .before = before,
                                        .after = stored,
                                        .index = 0});
  realize_nodes(*impl_->context, impl_->staged, stored, impl_->changes);
  return {};
}

result<void> scene_edit::define_name(const std::string &name,
                                     const node &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  if (name.empty()) {
    impl_->poison = edit_error(
        error_code::invalid_name, "scene_edit.define_name", *impl_->context,
        impl_->base->revision, "name is empty", target.id_);
    return *impl_->poison;
  }
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      !impl_->staged.nodes.contains(target.id_.value)) {
    impl_->poison = edit_error(
        error_code::stale_handle, "scene_edit.define_name", *impl_->context,
        impl_->base->revision, "invalid node handle", target.id_);
    return *impl_->poison;
  }
  if (impl_->staged.nodes.at(target.id_.value).scope_owner) {
    impl_->poison = edit_error(
        error_code::invalid_context, "scene_edit.define_name", *impl_->context,
        impl_->base->revision,
        "a template-scoped node cannot have a scene name", target.id_);
    return *impl_->poison;
  }
  const auto existing = std::find_if(
      impl_->staged.names.begin(), impl_->staged.names.end(),
      [&](const name_binding &binding) { return binding.name == name; });
  const auto imported =
      std::find_if(impl_->staged.imports.begin(), impl_->staged.imports.end(),
                   [&](const import_binding &binding) {
                     return binding.local_name == name;
                   });
  if (existing != impl_->staged.names.end() ||
      imported != impl_->staged.imports.end()) {
    impl_->poison = edit_error(
        error_code::duplicate_name, "scene_edit.define_name", *impl_->context,
        impl_->base->revision, "name is already defined: " + name, target.id_);
    return *impl_->poison;
  }
  impl_->staged.names.push_back(name_binding{.name = name, .node = target.id_});
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::name_defined,
                                        .node = target.id_,
                                        .field = name,
                                        .before = std::monostate{},
                                        .after = name,
                                        .index = 0});
  return {};
}

result<void> scene_edit::undefine_name(const std::string &name) {
  if (impl_->poison)
    return *impl_->poison;
  const auto found = std::find_if(
      impl_->staged.names.begin(), impl_->staged.names.end(),
      [&](const name_binding &binding) { return binding.name == name; });
  if (found == impl_->staged.names.end()) {
    impl_->poison = edit_error(
        error_code::invalid_name, "scene_edit.undefine_name", *impl_->context,
        impl_->base->revision, "name is not defined: " + name);
    return *impl_->poison;
  }
  const node_id removed = found->node;
  impl_->staged.names.erase(found);
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::name_removed,
                                        .node = removed,
                                        .field = name,
                                        .before = name,
                                        .after = std::monostate{},
                                        .index = 0});
  return {};
}

result<void> scene_edit::export_node(const std::string &name,
                                     const node &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  if (name.empty()) {
    impl_->poison = edit_error(
        error_code::invalid_name, "scene_edit.export_node", *impl_->context,
        impl_->base->revision, "export name is empty", target.id_);
    return *impl_->poison;
  }
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      !impl_->staged.nodes.contains(target.id_.value)) {
    impl_->poison = edit_error(
        error_code::stale_handle, "scene_edit.export_node", *impl_->context,
        impl_->base->revision, "invalid local node handle", target.id_);
    return *impl_->poison;
  }
  if (impl_->staged.nodes.at(target.id_.value).scope_owner) {
    impl_->poison =
        edit_error(error_code::invalid_context, "scene_edit.export_node",
                   *impl_->context, impl_->base->revision,
                   "a template-scoped node cannot be exported", target.id_);
    return *impl_->poison;
  }
  const auto existing = std::find_if(
      impl_->staged.exports.begin(), impl_->staged.exports.end(),
      [&](const export_binding &binding) { return binding.name == name; });
  if (existing != impl_->staged.exports.end()) {
    impl_->poison =
        edit_error(error_code::duplicate_name, "scene_edit.export_node",
                   *impl_->context, impl_->base->revision,
                   "export name is already defined: " + name, target.id_);
    return *impl_->poison;
  }
  impl_->staged.exports.push_back(
      export_binding{.name = name, .node = target.id_});
  impl_->changed = true;
  impl_->changes.push_back(
      graph_change{.kind = change_kind::export_added,
                   .node = target.id_,
                   .field = name,
                   .before = std::monostate{},
                   .after = name,
                   .index = impl_->staged.exports.size() - 1});
  return {};
}

result<void> scene_edit::remove_export(const export_binding &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto found = std::find(impl_->staged.exports.begin(),
                               impl_->staged.exports.end(), target);
  if (found == impl_->staged.exports.end()) {
    impl_->poison =
        edit_error(error_code::invalid_name, "scene_edit.remove_export",
                   *impl_->context, impl_->base->revision,
                   "export is not present in this context", target.node);
    return *impl_->poison;
  }
  const std::size_t index = static_cast<std::size_t>(
      std::distance(impl_->staged.exports.begin(), found));
  impl_->staged.exports.erase(found);
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::export_removed,
                                        .node = target.node,
                                        .field = target.name,
                                        .before = target.name,
                                        .after = std::monostate{},
                                        .index = index});
  return {};
}

result<void> scene_edit::import_node(const std::string &local_name,
                                     const execution_context &source,
                                     const std::string &exported_name) {
  if (impl_->poison)
    return *impl_->poison;
  if (local_name.empty() || exported_name.empty()) {
    impl_->poison = edit_error(
        error_code::invalid_name, "scene_edit.import_node", *impl_->context,
        impl_->base->revision, "import and export names must not be empty");
    return *impl_->poison;
  }
  const auto local = std::find_if(
      impl_->staged.names.begin(), impl_->staged.names.end(),
      [&](const name_binding &binding) { return binding.name == local_name; });
  const auto imported =
      std::find_if(impl_->staged.imports.begin(), impl_->staged.imports.end(),
                   [&](const import_binding &binding) {
                     return binding.local_name == local_name;
                   });
  if (local != impl_->staged.names.end() ||
      imported != impl_->staged.imports.end()) {
    impl_->poison = edit_error(
        error_code::duplicate_name, "scene_edit.import_node", *impl_->context,
        impl_->base->revision, "local name is already defined: " + local_name);
    return *impl_->poison;
  }
  if (!source.control_ || source.control_ == impl_->context ||
      source.control_->owner.lock() != impl_->context->owner.lock()) {
    impl_->poison = edit_error(
        error_code::invalid_context, "scene_edit.import_node", *impl_->context,
        impl_->base->revision,
        "source context must be a distinct context in the same browser");
    return *impl_->poison;
  }

  std::lock_guard source_lock(source.control_->mutex);
  if (!source.control_->active) {
    impl_->poison = edit_error(
        error_code::stale_handle, "scene_edit.import_node", *impl_->context,
        impl_->base->revision, "source context is no longer active");
    return *impl_->poison;
  }
  const auto exported = std::find_if(source.control_->state->exports.begin(),
                                     source.control_->state->exports.end(),
                                     [&](const export_binding &binding) {
                                       return binding.name == exported_name;
                                     });
  if (exported == source.control_->state->exports.end()) {
    impl_->poison =
        edit_error(error_code::invalid_name, "scene_edit.import_node",
                   *impl_->context, impl_->base->revision,
                   "source export is not defined: " + exported_name);
    return *impl_->poison;
  }
  const import_binding binding{
      .local_name = local_name,
      .source_generation = source.control_->generation,
      .exported_name = exported_name,
      .target = semantic_node_id{source.control_->generation, exported->node}};
  impl_->staged.imports.push_back(binding);
  impl_->staged.import_sources.push_back(source.control_);
  impl_->changed = true;
  impl_->changes.push_back(
      graph_change{.kind = change_kind::import_added,
                   .node = exported->node,
                   .field = local_name,
                   .before = std::monostate{},
                   .after = exported_name,
                   .index = impl_->staged.imports.size() - 1});
  return {};
}

result<void> scene_edit::remove_import(const import_binding &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto found = std::find(impl_->staged.imports.begin(),
                               impl_->staged.imports.end(), target);
  if (found == impl_->staged.imports.end()) {
    impl_->poison = edit_error(
        error_code::invalid_name, "scene_edit.remove_import", *impl_->context,
        impl_->base->revision, "import is not present in this context",
        target.target.local);
    return *impl_->poison;
  }
  const std::size_t index = static_cast<std::size_t>(
      std::distance(impl_->staged.imports.begin(), found));
  impl_->staged.imports.erase(found);
  impl_->staged.import_sources.erase(impl_->staged.import_sources.begin() +
                                     static_cast<std::ptrdiff_t>(index));
  impl_->changed = true;
  impl_->changes.push_back(graph_change{.kind = change_kind::import_removed,
                                        .node = target.target.local,
                                        .field = target.local_name,
                                        .before = target.exported_name,
                                        .after = std::monostate{},
                                        .index = index});
  return {};
}

result<void> scene_edit::add_route(const dynamic_field &source,
                                   const dynamic_field &sink) {
  if (impl_->poison)
    return *impl_->poison;
  const auto source_context = source.context_.lock();
  const auto sink_context = sink.context_.lock();
  if (source_context != impl_->context || sink_context != impl_->context ||
      source.generation_ != impl_->context->generation ||
      sink.generation_ != impl_->context->generation ||
      !impl_->staged.nodes.contains(source.node_.value) ||
      !impl_->staged.nodes.contains(sink.node_.value)) {
    impl_->poison = edit_error(error_code::stale_handle, "scene_edit.add_route",
                               *impl_->context, impl_->base->revision,
                               "invalid route endpoint");
    return *impl_->poison;
  }
  if (source.kind_ != sink.kind_ || !route_source_allowed(source.access_) ||
      !route_sink_allowed(sink.access_)) {
    impl_->poison =
        edit_error(error_code::invalid_route, "scene_edit.add_route",
                   *impl_->context, impl_->base->revision,
                   "route endpoint types or access are incompatible");
    return *impl_->poison;
  }
  if (impl_->staged.nodes.at(source.node_.value).scope_owner ||
      impl_->staged.nodes.at(sink.node_.value).scope_owner) {
    impl_->poison =
        edit_error(error_code::invalid_context, "scene_edit.add_route",
                   *impl_->context, impl_->base->revision,
                   "template-scoped nodes cannot be scene route endpoints");
    return *impl_->poison;
  }
  route added{.source = source.node_,
              .source_field = source.name_,
              .sink = sink.node_,
              .sink_field = sink.name_};
  if (std::find(impl_->staged.routes.begin(), impl_->staged.routes.end(),
                added) != impl_->staged.routes.end()) {
    impl_->poison = edit_error(
        error_code::invalid_route, "scene_edit.add_route", *impl_->context,
        impl_->base->revision, "route is already present in this scene");
    return *impl_->poison;
  }
  impl_->staged.routes.push_back(added);
  impl_->changed = true;
  impl_->changes.push_back(
      graph_change{.kind = change_kind::route_added,
                   .node = source.node_,
                   .field = source.name_ + " -> " + sink.name_,
                   .before = std::monostate{},
                   .after = sink.node_,
                   .index = impl_->staged.routes.size() - 1});
  return {};
}

result<void> scene_edit::remove_route(const route &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto found = std::find(impl_->staged.routes.begin(),
                               impl_->staged.routes.end(), target);
  if (found == impl_->staged.routes.end()) {
    impl_->poison = edit_error(
        error_code::invalid_route, "scene_edit.remove_route", *impl_->context,
        impl_->base->revision, "route is not present in this scene");
    return *impl_->poison;
  }
  const std::size_t index = static_cast<std::size_t>(
      std::distance(impl_->staged.routes.begin(), found));
  impl_->staged.routes.erase(found);
  impl_->changed = true;
  impl_->changes.push_back(
      graph_change{.kind = change_kind::route_removed,
                   .node = target.source,
                   .field = target.source_field + " -> " + target.sink_field,
                   .before = target.sink,
                   .after = std::monostate{},
                   .index = index});
  return {};
}

result<void> scene_edit::claim_declaration_nodes(declaration_id owner,
                                                 std::span<const node_id> roots,
                                                 const char *operation) {
  std::vector<node_id> pending{roots.begin(), roots.end()};
  std::unordered_set<std::uint64_t> closure;
  while (!pending.empty()) {
    const node_id current = pending.back();
    pending.pop_back();
    if (!closure.insert(current.value).second)
      continue;
    const auto found = impl_->staged.nodes.find(current.value);
    if (found == impl_->staged.nodes.end()) {
      impl_->poison =
          edit_error(error_code::invalid_descriptor, operation, *impl_->context,
                     impl_->base->revision,
                     "template closure contains a dangling node", current);
      return *impl_->poison;
    }
    if (found->second.scope_owner && found->second.scope_owner != owner) {
      impl_->poison = edit_error(
          error_code::node_in_use, operation, *impl_->context,
          impl_->base->revision,
          "template node is already owned by another declaration", current);
      return *impl_->poison;
    }
    for (const auto &[field_name, stored] : found->second.fields) {
      (void)field_name;
      if (const auto *single = std::get_if<node_id>(&stored)) {
        if (single->value != 0)
          pending.push_back(*single);
      } else if (const auto *many = std::get_if<node_list>(&stored)) {
        for (node_id child : *many)
          if (child.value != 0)
            pending.push_back(child);
      }
    }
  }

  std::unordered_map<std::uint64_t, int> colors;
  std::function<bool(std::uint64_t)> acyclic = [&](std::uint64_t current) {
    if (colors[current] == 1)
      return false;
    if (colors[current] == 2)
      return true;
    colors[current] = 1;
    const auto &record = impl_->staged.nodes.at(current);
    const auto *type = impl_->context->registry.find(record.type_name);
    for (const field_descriptor &field : type->fields) {
      if (!field.containment)
        continue;
      const value &stored = record.fields.at(field.name);
      if (const auto *single = std::get_if<node_id>(&stored)) {
        if (single->value != 0 && !acyclic(single->value))
          return false;
      } else if (const auto *many = std::get_if<node_list>(&stored)) {
        for (node_id child : *many) {
          if (child.value != 0 && !acyclic(child.value))
            return false;
        }
      }
    }
    colors[current] = 2;
    return true;
  };
  if (std::ranges::any_of(closure,
                          [&](std::uint64_t node) { return !acyclic(node); })) {
    impl_->poison = edit_error(error_code::containment_cycle, operation,
                               *impl_->context, impl_->base->revision,
                               "template body contains a containment cycle");
    return *impl_->poison;
  }

  const auto in_closure = [&](node_id candidate) {
    return closure.contains(candidate.value);
  };
  if (std::ranges::any_of(impl_->staged.roots, in_closure) ||
      std::ranges::any_of(impl_->staged.names,
                          [&](const name_binding &binding) {
                            return in_closure(binding.node);
                          }) ||
      std::ranges::any_of(impl_->staged.exports,
                          [&](const export_binding &binding) {
                            return in_closure(binding.node);
                          }) ||
      std::ranges::any_of(impl_->staged.routes, [&](const route &candidate) {
        return in_closure(candidate.source) || in_closure(candidate.sink);
      })) {
    impl_->poison =
        edit_error(error_code::node_in_use, operation, *impl_->context,
                   impl_->base->revision,
                   "template closure is already referenced by the live scene");
    return *impl_->poison;
  }
  for (const auto &[id, record] : impl_->staged.nodes) {
    if (closure.contains(id) || record.scope_owner == owner)
      continue;
    for (const auto &[field_name, stored] : record.fields) {
      bool crosses = false;
      if (const auto *single = std::get_if<node_id>(&stored))
        crosses = single->value != 0 && in_closure(*single);
      else if (const auto *many = std::get_if<node_list>(&stored))
        crosses = std::ranges::any_of(*many, in_closure);
      if (crosses) {
        impl_->poison =
            edit_error(error_code::node_in_use, operation, *impl_->context,
                       impl_->base->revision,
                       "template closure is retained by an ordinary scene node",
                       node_id{id}, field_name);
        return *impl_->poison;
      }
    }
  }

  for (const auto id : closure)
    impl_->staged.nodes.at(id).scope_owner = owner;
  return {};
}

result<declaration>
scene_edit::add_local_declaration(local_declaration_descriptor descriptor) {
  if (!descriptor.body_roots.empty()) {
    impl_->poison = edit_error(
        error_code::invalid_context, "scene_edit.add_local_declaration",
        *impl_->context, impl_->base->revision,
        "template bodies require context-bearing node handles");
    return *impl_->poison;
  }
  return add_local_declaration(std::move(descriptor), {});
}

result<declaration>
scene_edit::add_local_declaration(local_declaration_descriptor descriptor,
                                  std::span<const node> body_roots) {
  if (impl_->poison)
    return *impl_->poison;
  if (descriptor.name.empty()) {
    impl_->poison =
        edit_error(error_code::invalid_name, "scene_edit.add_local_declaration",
                   *impl_->context, impl_->base->revision,
                   "declaration name must not be empty");
    return *impl_->poison;
  }
  if (!descriptor.body_roots.empty()) {
    impl_->poison = edit_error(
        error_code::invalid_context, "scene_edit.add_local_declaration",
        *impl_->context, impl_->base->revision,
        "body root IDs are inspection data; author with node handles");
    return *impl_->poison;
  }
  if (const auto issue = validate_declaration_interface(descriptor.interface)) {
    impl_->poison = edit_error(issue->code, "scene_edit.add_local_declaration",
                               *impl_->context, impl_->base->revision,
                               issue->message, std::nullopt, issue->field);
    return *impl_->poison;
  }
  const auto duplicate = std::ranges::find_if(
      impl_->staged.declaration_order, [&](declaration_id candidate) {
        return experimental::name(impl_->staged.declarations.at(
                   candidate.value)) == descriptor.name;
      });
  if (duplicate != impl_->staged.declaration_order.end()) {
    impl_->poison = edit_error(
        error_code::duplicate_name, "scene_edit.add_local_declaration",
        *impl_->context, impl_->base->revision,
        "declaration name is already defined: " + descriptor.name);
    return *impl_->poison;
  }

  std::vector<node_id> authored_roots;
  authored_roots.reserve(body_roots.size());
  for (const node &root : body_roots) {
    const auto root_context = root.context_.lock();
    if (root_context != impl_->context ||
        root.generation_ != impl_->context->generation ||
        !impl_->staged.nodes.contains(root.id_.value)) {
      impl_->poison = edit_error(
          error_code::invalid_context, "scene_edit.add_local_declaration",
          *impl_->context, impl_->base->revision,
          "template body root belongs to another context", root.id_);
      return *impl_->poison;
    }
    authored_roots.push_back(root.id_);
  }

  const auto reserved = reserve_identity(impl_->context->next_declaration_id);
  if (!reserved) {
    impl_->poison = edit_error(error_code::invalid_value,
                               "scene_edit.add_local_declaration",
                               *impl_->context, impl_->base->revision,
                               "declaration identity space is exhausted");
    return *impl_->poison;
  }
  const declaration_id id{*reserved};
  if (auto claimed = claim_declaration_nodes(
          id, authored_roots, "scene_edit.add_local_declaration");
      !claimed)
    return failure(claimed.error());
  descriptor.body_roots = std::move(authored_roots);
  declaration_descriptor authored{.id = id, .payload = std::move(descriptor)};
  impl_->staged.declaration_order.push_back(id);
  impl_->staged.declarations.emplace(id.value, authored);
  impl_->changes.push_back(
      declaration_change{.kind = declaration_change_kind::added,
                         .declaration = id,
                         .before = std::nullopt,
                         .after = std::move(authored)});
  impl_->changed = true;
  return declaration{impl_->context, impl_->context->generation, id};
}

result<declaration> scene_edit::add_external_declaration(
    external_declaration_descriptor descriptor) {
  if (impl_->poison)
    return *impl_->poison;
  if (descriptor.name.empty()) {
    impl_->poison =
        edit_error(error_code::invalid_name,
                   "scene_edit.add_external_declaration", *impl_->context,
                   impl_->base->revision, "declaration name must not be empty");
    return *impl_->poison;
  }
  if (descriptor.urls.empty() ||
      std::ranges::any_of(descriptor.urls,
                          [](const std::string &url) { return url.empty(); }) ||
      descriptor.load_state != external_load_state::unresolved ||
      descriptor.resolved_declaration) {
    impl_->poison = edit_error(
        error_code::invalid_descriptor, "scene_edit.add_external_declaration",
        *impl_->context, impl_->base->revision,
        "new external declaration must be unresolved with at least one URL");
    return *impl_->poison;
  }
  if (const auto issue =
          validate_declaration_interface(descriptor.interface, true)) {
    impl_->poison = edit_error(
        issue->code, "scene_edit.add_external_declaration", *impl_->context,
        impl_->base->revision, issue->message, std::nullopt, issue->field);
    return *impl_->poison;
  }
  const auto duplicate = std::ranges::find_if(
      impl_->staged.declaration_order, [&](declaration_id candidate) {
        return experimental::name(impl_->staged.declarations.at(
                   candidate.value)) == descriptor.name;
      });
  if (duplicate != impl_->staged.declaration_order.end()) {
    impl_->poison = edit_error(
        error_code::duplicate_name, "scene_edit.add_external_declaration",
        *impl_->context, impl_->base->revision,
        "declaration name is already defined: " + descriptor.name);
    return *impl_->poison;
  }
  const auto reserved = reserve_identity(impl_->context->next_declaration_id);
  if (!reserved) {
    impl_->poison = edit_error(error_code::invalid_value,
                               "scene_edit.add_external_declaration",
                               *impl_->context, impl_->base->revision,
                               "declaration identity space is exhausted");
    return *impl_->poison;
  }
  const declaration_id id{*reserved};
  declaration_descriptor authored{.id = id, .payload = std::move(descriptor)};
  impl_->staged.declaration_order.push_back(id);
  impl_->staged.declarations.emplace(id.value, authored);
  impl_->changes.push_back(
      declaration_change{.kind = declaration_change_kind::added,
                         .declaration = id,
                         .before = std::nullopt,
                         .after = std::move(authored)});
  impl_->changed = true;
  return declaration{impl_->context, impl_->context->generation, id};
}

result<void> scene_edit::rename_declaration(const declaration &target,
                                            std::string new_name) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  const auto found = impl_->staged.declarations.find(target.id_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.declarations.end()) {
    impl_->poison = declaration_edit_error(
        error_code::stale_handle, "scene_edit.rename_declaration",
        *impl_->context, impl_->base->revision, "invalid declaration handle",
        target.id_);
    return *impl_->poison;
  }
  if (new_name.empty()) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_name, "scene_edit.rename_declaration",
        *impl_->context, impl_->base->revision,
        "declaration name must not be empty", target.id_);
    return *impl_->poison;
  }
  const auto duplicate = std::ranges::find_if(
      impl_->staged.declaration_order, [&](declaration_id candidate) {
        return candidate != target.id_ &&
               experimental::name(
                   impl_->staged.declarations.at(candidate.value)) == new_name;
      });
  if (duplicate != impl_->staged.declaration_order.end()) {
    impl_->poison = declaration_edit_error(
        error_code::duplicate_name, "scene_edit.rename_declaration",
        *impl_->context, impl_->base->revision,
        "declaration name is already defined: " + new_name, target.id_);
    return *impl_->poison;
  }
  const declaration_descriptor before = found->second;
  std::visit([&](auto &payload) { payload.name = std::move(new_name); },
             found->second.payload);
  impl_->changes.push_back(
      declaration_change{.kind = declaration_change_kind::renamed,
                         .declaration = target.id_,
                         .before = before,
                         .after = found->second});
  impl_->changed = true;
  return {};
}

void scene_edit::stage_declaration_update(declaration_id id,
                                          declaration_payload replacement) {
  auto &authored = impl_->staged.declarations.at(id.value);
  const declaration_descriptor before = authored;
  authored.payload = std::move(replacement);
  impl_->changes.push_back(
      declaration_change{.kind = declaration_change_kind::updated,
                         .declaration = id,
                         .before = before,
                         .after = authored});
  impl_->changed = true;
}

result<void> scene_edit::update_declaration(const declaration &target,
                                            declaration_payload replacement) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  const auto found = impl_->staged.declarations.find(target.id_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.declarations.end()) {
    impl_->poison = declaration_edit_error(
        error_code::stale_handle, "scene_edit.update_declaration",
        *impl_->context, impl_->base->revision, "invalid declaration handle",
        target.id_);
    return *impl_->poison;
  }
  if (replacement.index() != found->second.payload.index()) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_descriptor, "scene_edit.update_declaration",
        *impl_->context, impl_->base->revision,
        "declaration kind cannot change during update", target.id_);
    return *impl_->poison;
  }
  const std::string &replacement_name = std::visit(
      [](const auto &payload) -> const std::string & { return payload.name; },
      replacement);
  if (replacement_name != experimental::name(found->second)) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_name, "scene_edit.update_declaration",
        *impl_->context, impl_->base->revision,
        "rename intent must use rename_declaration", target.id_);
    return *impl_->poison;
  }
  const auto &interface = std::visit(
      [](const auto &payload)
          -> const std::vector<interface_field_descriptor> & {
        return payload.interface;
      },
      replacement);
  if (const auto issue = validate_declaration_interface(
          interface,
          std::holds_alternative<external_declaration_descriptor>(replacement),
          std::holds_alternative<local_declaration_descriptor>(replacement))) {
    impl_->poison = declaration_edit_error(
        issue->code, "scene_edit.update_declaration", *impl_->context,
        impl_->base->revision, issue->message, target.id_, issue->field);
    return *impl_->poison;
  }
  if (const auto *local =
          std::get_if<local_declaration_descriptor>(&replacement)) {
    const auto &current =
        std::get<local_declaration_descriptor>(found->second.payload);
    if (local->body_roots != current.body_roots) {
      impl_->poison = declaration_edit_error(
          error_code::invalid_context, "scene_edit.update_declaration",
          *impl_->context, impl_->base->revision,
          "template body changes require context-bearing node handles",
          target.id_);
      return *impl_->poison;
    }
    for (const auto &field : local->interface) {
      if (!field.default_value ||
          !has_non_null_node_identity(*field.default_value))
        continue;
      const auto existing = std::ranges::find_if(
          current.interface, [&](const interface_field_descriptor &candidate) {
            return candidate.name == field.name;
          });
      bool preserved = existing != current.interface.end() &&
                       existing->kind == field.kind &&
                       existing->default_value &&
                       *existing->default_value == *field.default_value;
      if (preserved) {
        const auto validate_id = [&](node_id id) {
          if (!id.value)
            return true;
          const auto candidate = impl_->staged.nodes.find(id.value);
          if (candidate == impl_->staged.nodes.end() ||
              candidate->second.scope_owner != target.id_)
            return false;
          field_descriptor constraint;
          constraint.accepted_node_types = field.accepted_node_types;
          return accepts_node_type(*impl_->context, constraint,
                                   candidate->second);
        };
        if (field.kind == value_kind::sf_node)
          preserved = validate_id(std::get<node_id>(*field.default_value));
        else
          preserved = std::ranges::all_of(
              std::get<node_list>(*field.default_value), validate_id);
      }
      if (!preserved) {
        impl_->poison = declaration_edit_error(
            error_code::invalid_descriptor, "scene_edit.update_declaration",
            *impl_->context, impl_->base->revision,
            "node default changes require context-bearing node handles",
            target.id_, field.name);
        return *impl_->poison;
      }
    }
  }
  if (const auto *external =
          std::get_if<external_declaration_descriptor>(&replacement);
      external) {
    const auto &current =
        std::get<external_declaration_descriptor>(found->second.payload);
    if (external->resolved_declaration &&
        (current.load_state != external_load_state::resolved ||
         current.resolved_declaration != external->resolved_declaration)) {
      impl_->poison = declaration_edit_error(
          error_code::invalid_descriptor, "scene_edit.update_declaration",
          *impl_->context, impl_->base->revision,
          "resolved target changes require resolve_external_declaration",
          target.id_);
      return *impl_->poison;
    }
    if (external->urls.empty() ||
        std::ranges::any_of(external->urls, [](const std::string &url) {
          return url.empty();
        })) {
      impl_->poison = declaration_edit_error(
          error_code::invalid_descriptor, "scene_edit.update_declaration",
          *impl_->context, impl_->base->revision,
          "external declaration requires at least one non-empty URL",
          target.id_);
      return *impl_->poison;
    }
    detail::scene_state candidate_state = impl_->staged;
    candidate_state.declarations.at(target.id_.value).payload = replacement;
    if (const auto issue =
            validate_external_resolution(candidate_state, *external)) {
      impl_->poison = declaration_edit_error(
          error_code::invalid_descriptor, "scene_edit.update_declaration",
          *impl_->context, impl_->base->revision, *issue, target.id_);
      return *impl_->poison;
    }
  }
  if (const auto *local =
          std::get_if<local_declaration_descriptor>(&replacement)) {
    const auto retained = std::ranges::find_if(
        impl_->staged.declaration_order, [&](declaration_id candidate) {
          const auto *external = std::get_if<external_declaration_descriptor>(
              &impl_->staged.declarations.at(candidate.value).payload);
          return external &&
                 external->load_state == external_load_state::resolved &&
                 external->resolved_declaration == target.id_ &&
                 !compatible_declaration_interface(external->interface,
                                                   local->interface);
        });
    if (retained != impl_->staged.declaration_order.end()) {
      impl_->poison = declaration_edit_error(
          error_code::declaration_in_use, "scene_edit.update_declaration",
          *impl_->context, impl_->base->revision,
          "local declaration is retained by a resolved external declaration",
          target.id_);
      return *impl_->poison;
    }
  }
  stage_declaration_update(target.id_, std::move(replacement));
  return {};
}

result<void>
scene_edit::resolve_external_declaration(const declaration &external,
                                         const declaration &local) {
  if (impl_->poison)
    return *impl_->poison;
  const auto external_found =
      impl_->staged.declarations.find(external.id_.value);
  const auto local_found = impl_->staged.declarations.find(local.id_.value);
  if (external.context_.lock() != impl_->context ||
      external.generation_ != impl_->context->generation ||
      external_found == impl_->staged.declarations.end() ||
      local.context_.lock() != impl_->context ||
      local.generation_ != impl_->context->generation ||
      local_found == impl_->staged.declarations.end()) {
    impl_->poison = declaration_edit_error(
        error_code::stale_handle, "scene_edit.resolve_external_declaration",
        *impl_->context, impl_->base->revision, "invalid declaration handle",
        external.id_);
    return *impl_->poison;
  }
  auto *external_payload = std::get_if<external_declaration_descriptor>(
      &external_found->second.payload);
  const auto *local_payload =
      std::get_if<local_declaration_descriptor>(&local_found->second.payload);
  if (!external_payload || !local_payload) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_descriptor,
        "scene_edit.resolve_external_declaration", *impl_->context,
        impl_->base->revision,
        "resolution requires an external and a local declaration",
        external.id_);
    return *impl_->poison;
  }
  if (!compatible_declaration_interface(external_payload->interface,
                                        local_payload->interface)) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_descriptor,
        "scene_edit.resolve_external_declaration", *impl_->context,
        impl_->base->revision,
        "resolved external interface is incompatible with its local target",
        external.id_);
    return *impl_->poison;
  }
  auto resolved = *external_payload;
  resolved.load_state = external_load_state::resolved;
  resolved.diagnostic.clear();
  resolved.resolved_declaration = local.id_;
  stage_declaration_update(external.id_, std::move(resolved));
  return {};
}

result<void>
scene_edit::set_declaration_default(const declaration &target,
                                    std::string_view field_name,
                                    const std::optional<node> &authored) {
  if (impl_->poison)
    return *impl_->poison;
  node_id id{};
  if (authored) {
    const auto context = authored->context_.lock();
    if (authored->disposed_ || context != impl_->context ||
        authored->generation_ != impl_->context->generation ||
        !impl_->staged.nodes.contains(authored->id_.value)) {
      impl_->poison = declaration_edit_error(
          error_code::invalid_context, "scene_edit.set_declaration_default",
          *impl_->context, impl_->base->revision,
          "node default requires a live node handle in this context",
          target.id_, std::string{field_name});
      return *impl_->poison;
    }
    id = authored->id_;
  }
  return set_declaration_default_ids(target, field_name, value{id});
}

result<void> scene_edit::set_declaration_default(
    const declaration &target, std::string_view field_name,
    std::span<const std::optional<node>> authored) {
  if (impl_->poison)
    return *impl_->poison;
  node_list ids;
  ids.reserve(authored.size());
  for (const auto &candidate : authored) {
    if (!candidate) {
      ids.push_back(node_id{});
      continue;
    }
    const auto context = candidate->context_.lock();
    if (candidate->disposed_ || context != impl_->context ||
        candidate->generation_ != impl_->context->generation ||
        !impl_->staged.nodes.contains(candidate->id_.value)) {
      impl_->poison = declaration_edit_error(
          error_code::invalid_context, "scene_edit.set_declaration_default",
          *impl_->context, impl_->base->revision,
          "node default requires live node handles in this context", target.id_,
          std::string{field_name});
      return *impl_->poison;
    }
    ids.push_back(candidate->id_);
  }
  return set_declaration_default_ids(target, field_name, value{std::move(ids)});
}

result<void> scene_edit::set_declaration_default_ids(
    const declaration &target, std::string_view field_name, value authored) {
  constexpr const char *operation = "scene_edit.set_declaration_default";
  const auto context = target.context_.lock();
  const auto found = impl_->staged.declarations.find(target.id_.value);
  if (context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.declarations.end()) {
    impl_->poison = declaration_edit_error(
        error_code::stale_handle, operation, *impl_->context,
        impl_->base->revision, "invalid declaration handle", target.id_);
    return *impl_->poison;
  }
  auto *local =
      std::get_if<local_declaration_descriptor>(&found->second.payload);
  if (!local) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_descriptor, operation, *impl_->context,
        impl_->base->revision, "node defaults require a local declaration",
        target.id_);
    return *impl_->poison;
  }
  const auto field = std::ranges::find_if(
      local->interface, [&](const interface_field_descriptor &candidate) {
        return candidate.name == field_name;
      });
  if (field == local->interface.end()) {
    impl_->poison = declaration_edit_error(
        error_code::unknown_field, operation, *impl_->context,
        impl_->base->revision, "unknown interface field", target.id_,
        std::string{field_name});
    return *impl_->poison;
  }
  if (field->kind != kind_of(authored) ||
      (field->access != access_type::initialize_only &&
       field->access != access_type::input_output)) {
    impl_->poison = declaration_edit_error(
        error_code::invalid_descriptor, operation, *impl_->context,
        impl_->base->revision, "field cannot receive this node default",
        target.id_, std::string{field_name});
    return *impl_->poison;
  }
  node_list roots;
  if (const auto *single = std::get_if<node_id>(&authored)) {
    if (single->value)
      roots.push_back(*single);
  } else {
    for (node_id id : std::get<node_list>(authored))
      if (id.value)
        roots.push_back(id);
  }
  field_descriptor constraint;
  constraint.accepted_node_types = field->accepted_node_types;
  for (node_id id : roots) {
    const auto candidate = impl_->staged.nodes.find(id.value);
    if (candidate == impl_->staged.nodes.end() ||
        !accepts_node_type(*impl_->context, constraint, candidate->second)) {
      impl_->poison = declaration_edit_error(
          error_code::type_mismatch, operation, *impl_->context,
          impl_->base->revision, "node type is not accepted by the field",
          target.id_, std::string{field_name});
      return *impl_->poison;
    }
  }
  if (auto claimed = claim_declaration_nodes(target.id_, roots, operation);
      !claimed)
    return failure(claimed.error());
  const declaration_descriptor before = found->second;
  field->default_value = std::move(authored);
  impl_->changes.push_back(
      declaration_change{.kind = declaration_change_kind::updated,
                         .declaration = target.id_,
                         .before = before,
                         .after = found->second});
  impl_->changed = true;
  return {};
}

result<void> scene_edit::remove_declaration(const declaration &target) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  const auto found = impl_->staged.declarations.find(target.id_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->staged.declarations.end()) {
    impl_->poison = declaration_edit_error(
        error_code::stale_handle, "scene_edit.remove_declaration",
        *impl_->context, impl_->base->revision, "invalid declaration handle",
        target.id_);
    return *impl_->poison;
  }
  const auto retained = std::ranges::find_if(
      impl_->staged.declaration_order, [&](declaration_id candidate) {
        const auto *external = std::get_if<external_declaration_descriptor>(
            &impl_->staged.declarations.at(candidate.value).payload);
        return external &&
               external->load_state == external_load_state::resolved &&
               external->resolved_declaration == target.id_;
      });
  if (retained != impl_->staged.declaration_order.end()) {
    impl_->poison = declaration_edit_error(
        error_code::declaration_in_use, "scene_edit.remove_declaration",
        *impl_->context, impl_->base->revision,
        "local declaration is retained by a resolved external declaration",
        target.id_);
    return *impl_->poison;
  }
  const declaration_descriptor before = found->second;
  for (auto &[node, record] : impl_->staged.nodes) {
    (void)node;
    if (record.scope_owner == target.id_)
      record.scope_owner.reset();
  }
  const auto ordered =
      std::ranges::find(impl_->staged.declaration_order, target.id_);
  impl_->staged.declaration_order.erase(ordered);
  impl_->staged.declarations.erase(found);
  impl_->changes.push_back(
      declaration_change{.kind = declaration_change_kind::removed,
                         .declaration = target.id_,
                         .before = before,
                         .after = std::nullopt});
  impl_->changed = true;
  return {};
}

namespace {
struct edit_admission {
  std::vector<std::shared_ptr<detail::context_control>> import_sources;
  std::vector<std::shared_ptr<detail::context_control>> dependency_sources;
  std::vector<std::shared_ptr<detail::context_control>> lock_order;
  std::vector<std::shared_ptr<detail::context_control>> retained_importers;
  // Release locks before any pinned context can run its resource destructors.
  std::vector<std::unique_lock<std::mutex>> locks;
};

result<edit_admission> validate_edit(const detail::scene_edit_control &edit,
                                     const char *operation) {
  if (edit.poison)
    return *edit.poison;
  if (edit.committed) {
    return edit_error(error_code::poisoned_edit, operation, *edit.context,
                      edit.base->revision, "scene edit was already committed");
  }

  for (declaration_id id : edit.staged.declaration_order) {
    const auto *external = std::get_if<external_declaration_descriptor>(
        &edit.staged.declarations.at(id.value).payload);
    if (!external)
      continue;
    if (const auto issue =
            validate_external_resolution(edit.staged, *external)) {
      return declaration_edit_error(error_code::invalid_descriptor, operation,
                                    *edit.context, edit.base->revision, *issue,
                                    id);
    }
  }

  if (auto valid = validate_candidate_graph(*edit.context, edit.staged,
                                            operation, edit.base->revision);
      !valid)
    return failure(valid.error());

  edit_admission admission;
  auto &import_sources = admission.import_sources;
  import_sources.reserve(edit.staged.import_sources.size());
  for (const auto &weak_source : edit.staged.import_sources) {
    auto source = weak_source.lock();
    if (!source) {
      return edit_error(error_code::stale_aperture, operation, *edit.context,
                        edit.base->revision,
                        "import source context no longer exists");
    }
    import_sources.push_back(std::move(source));
  }
  auto &dependency_sources = admission.dependency_sources;
  dependency_sources = import_sources;
  dependency_sources.reserve(dependency_sources.size() +
                             edit.base->import_sources.size());
  for (const auto &weak_source : edit.base->import_sources) {
    if (auto source = weak_source.lock())
      dependency_sources.push_back(std::move(source));
  }
  auto &lock_order = admission.lock_order;
  lock_order = dependency_sources;
  lock_order.push_back(edit.context);
  std::sort(lock_order.begin(), lock_order.end(),
            [](const auto &left, const auto &right) {
              if (left->generation != right->generation)
                return left->generation < right->generation;
              return left.get() < right.get();
            });
  lock_order.erase(std::unique(lock_order.begin(), lock_order.end()),
                   lock_order.end());
  auto &locks = admission.locks;
  locks.reserve(lock_order.size());
  for (const auto &context : lock_order)
    locks.emplace_back(context->mutex);

  if (edit.context->processing) {
    return edit_error(error_code::operation_in_progress, operation,
                      *edit.context, edit.base->revision,
                      "runtime evaluation is in progress");
  }
  if (!edit.context->active) {
    return edit_error(error_code::stale_handle, operation, *edit.context,
                      edit.base->revision,
                      "execution context is no longer active");
  }
  if (edit.context->state->revision != edit.base->revision) {
    return edit_error(error_code::stale_revision, operation, *edit.context,
                      edit.base->revision, "edit base revision is stale");
  }
  for (const auto &[id, staged] : edit.staged.nodes) {
    if (!staged.scope_owner)
      continue;
    const auto base = edit.base->nodes.find(id);
    if (base != edit.base->nodes.end() &&
        base->second.scope_owner == staged.scope_owner)
      continue;
    if (const auto blocker =
            find_live_context_reference(*edit.context, node_id{id})) {
      return edit_error(error_code::node_in_use, operation, *edit.context,
                        edit.base->revision, blocker->message, node_id{id},
                        blocker->field);
    }
    if (const auto imported = find_live_import_reference(
            *edit.context, node_id{id}, admission.retained_importers)) {
      return edit_error(error_code::node_in_use, operation, *edit.context,
                        edit.base->revision,
                        "node acquired a committed import aperture while its "
                        "template claim was staged",
                        node_id{id}, *imported);
    }
  }
  for (const auto removed : edit.removed_nodes) {
    if (const auto blocker =
            find_live_context_reference(*edit.context, node_id{removed})) {
      return edit_error(error_code::node_in_use, operation, *edit.context,
                        edit.base->revision, blocker->message, node_id{removed},
                        blocker->field);
    }
    if (const auto imported = find_live_import_reference(
            *edit.context, node_id{removed}, admission.retained_importers)) {
      return edit_error(error_code::node_in_use, operation, *edit.context,
                        edit.base->revision,
                        "node acquired a committed import aperture while its "
                        "removal was staged",
                        node_id{removed}, *imported);
    }
  }
  for (std::size_t index = 0; index < edit.staged.imports.size(); ++index) {
    const import_binding &binding = edit.staged.imports[index];
    const auto &source = import_sources[index];
    if (!source->active || source->generation != binding.source_generation) {
      return edit_error(error_code::stale_aperture, operation, *edit.context,
                        edit.base->revision,
                        "import source generation is stale",
                        binding.target.local);
    }
    const auto exported = std::find_if(
        source->state->exports.begin(), source->state->exports.end(),
        [&](const export_binding &candidate) {
          return candidate.name == binding.exported_name;
        });
    if (exported == source->state->exports.end() ||
        exported->node != binding.target.local) {
      return edit_error(error_code::stale_aperture, operation, *edit.context,
                        edit.base->revision,
                        "source export changed before import publication",
                        binding.target.local);
    }
  }
  return admission;
}
} // namespace

result<void> scene_edit::validate() const {
  auto checked = validate_edit(*impl_, "scene_edit.validate");
  if (!checked)
    return failure(checked.error());
  return {};
}

result<change_set> scene_edit::commit() {
  static_assert(std::is_nothrow_move_constructible_v<change_set>);
  auto checked = validate_edit(*impl_, "scene_edit.commit");
  if (!checked)
    return failure(checked.error());
  const auto &import_sources = checked->import_sources;
  const auto &dependency_sources = checked->dependency_sources;
  const auto &lock_order = checked->lock_order;
  if (!impl_->changed) {
    impl_->committed = true;
    return change_set{.before_revision = impl_->base->revision,
                      .after_revision = impl_->base->revision,
                      .changes = {}};
  }
  using dependents = decltype(detail::context_control::import_dependents);
  std::vector<std::pair<std::shared_ptr<detail::context_control>, dependents>>
      dependency_updates;
  for (const auto &source : lock_order) {
    if (std::ranges::find(dependency_sources, source) ==
        dependency_sources.end())
      continue;
    auto updated = source->import_dependents;
    std::erase_if(updated, [&](const auto &dependent) {
      return same_owner(dependent.importer, impl_->context) &&
             dependent.importer_generation == impl_->context->generation;
    });
    for (std::size_t index = 0; index < impl_->staged.imports.size(); ++index) {
      if (import_sources[index] != source)
        continue;
      const auto &binding = impl_->staged.imports[index];
      updated.push_back(detail::context_control::import_dependent_record{
          .importer = impl_->context,
          .importer_generation = impl_->context->generation,
          .local_name = binding.local_name,
          .target = binding.target.local});
    }
    dependency_updates.emplace_back(source, std::move(updated));
  }
  const revision_id before = impl_->base->revision;
  change_set committed{.before_revision = before,
                       .after_revision = before + 1,
                       .changes = impl_->changes};
  decltype(impl_->context->pending) notifications;
  if (!impl_->context->observers.empty()) {
    std::vector<std::uint64_t> recipients;
    recipients.reserve(impl_->context->observers.size());
    for (const auto &[observer_id, callback] : impl_->context->observers) {
      (void)callback;
      recipients.push_back(observer_id);
    }
    notifications.push_back(detail::context_control::pending_change{
        .changes = committed, .recipients = std::move(recipients)});
  }
  static_assert(std::is_nothrow_move_constructible_v<detail::scene_state>);
  auto published =
      std::make_shared<detail::scene_state>(std::move(impl_->staged));
  published->revision = before + 1;
  // Everything below is non-allocating. Exceptions above leave visible state,
  // import retention, the notification queue and revision entirely unchanged.
  for (auto &[source, updated] : dependency_updates)
    source->import_dependents.swap(updated);
  impl_->context->state = published;
  impl_->context->published_revision.store(published->revision,
                                           std::memory_order_release);
  impl_->context->pending.splice(impl_->context->pending.end(), notifications);
  impl_->committed = true;
  return committed;
}

result<void> event_batch::send(const dynamic_field &target, value payload) {
  return send_value(target, std::move(payload), false);
}

result<bool> event_batch::writable(const dynamic_field &target,
                                   write_intent intent) const {
  if (impl_->poison)
    return *impl_->poison;
  if (impl_->committed) {
    return edit_error(error_code::stale_revision, "event_batch.writable",
                      *impl_->context, impl_->base->revision,
                      "event batch was already committed", target.node(),
                      target.name());
  }
  {
    std::lock_guard lock(impl_->context->mutex);
    if (!impl_->context->active) {
      return edit_error(error_code::stale_handle, "event_batch.writable",
                        *impl_->context, impl_->base->revision,
                        "execution context is no longer active", target.node(),
                        target.name());
    }
    if (impl_->context->published_revision.load(std::memory_order_acquire) !=
        impl_->base->revision) {
      return edit_error(error_code::stale_revision, "event_batch.writable",
                        *impl_->context, impl_->base->revision,
                        "event batch base revision is stale", target.node(),
                        target.name());
    }
  }
  const auto target_context = target.context_.lock();
  const auto found = impl_->base->nodes.find(target.node_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->base->nodes.end()) {
    return edit_error(error_code::stale_handle, "event_batch.writable",
                      *impl_->context, impl_->base->revision,
                      "invalid field handle", target.node_, target.name_);
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, target.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "event_batch.writable",
                      *impl_->context, impl_->base->revision,
                      "unknown field: " + target.name_, target.node_,
                      target.name_);
  }
  if (found->second.scope_owner)
    return false;
  if (intent != impl_->intent)
    return false;
  return field_is_writable(descriptor->access, intent, false);
}

result<void> event_batch::send(const dynamic_field &target, value payload,
                               value_space space) {
  return send_value(target, std::move(payload), false, space);
}

result<void> event_batch::send(const dynamic_field &target,
                               const node &payload) {
  if (impl_->poison)
    return *impl_->poison;
  if (payload.is_null() && !payload.disposed())
    return send_value(target, value{node_id{}}, true);
  if (!same_owner(payload.context_, impl_->context) ||
      payload.generation_ != impl_->context->generation ||
      !impl_->base->nodes.contains(payload.id_.value)) {
    impl_->poison =
        edit_error(error_code::invalid_context, impl_->send_operation(),
                   *impl_->context, impl_->base->revision,
                   "node event belongs to another context", payload.id_);
    return *impl_->poison;
  }
  return send_value(target, value{payload.id_}, true);
}

result<void> event_batch::send(const dynamic_field &target,
                               std::span<const node> payload) {
  if (impl_->poison)
    return *impl_->poison;
  node_list ids;
  ids.reserve(payload.size());
  for (const node &candidate : payload) {
    if (candidate.is_null() && !candidate.disposed()) {
      ids.push_back(node_id{});
      continue;
    }
    if (!same_owner(candidate.context_, impl_->context) ||
        candidate.generation_ != impl_->context->generation ||
        !impl_->base->nodes.contains(candidate.id_.value)) {
      impl_->poison =
          edit_error(error_code::invalid_context, impl_->send_operation(),
                     *impl_->context, impl_->base->revision,
                     "node event range crosses a context", candidate.id_);
      return *impl_->poison;
    }
    ids.push_back(candidate.id_);
  }
  return send_value(target, value{std::move(ids)}, true);
}

result<void> event_batch::send_value(const dynamic_field &target, value payload,
                                     bool node_authority_checked,
                                     value_space space) {
  if (impl_->poison)
    return *impl_->poison;
  const auto found = impl_->base->nodes.find(target.node_.value);
  if (!same_owner(target.context_, impl_->context) ||
      target.generation_ != impl_->context->generation ||
      found == impl_->base->nodes.end()) {
    impl_->poison =
        edit_error(error_code::stale_handle, impl_->send_operation(),
                   *impl_->context, impl_->base->revision,
                   "invalid event destination", target.node_, target.name_);
    return *impl_->poison;
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, target.name_);
  if (!descriptor) {
    impl_->poison =
        edit_error(error_code::unknown_field, impl_->send_operation(),
                   *impl_->context, impl_->base->revision,
                   "unknown event destination", target.node_, target.name_);
    return *impl_->poison;
  }
  if (found->second.scope_owner) {
    impl_->poison =
        edit_error(error_code::invalid_context, impl_->send_operation(),
                   *impl_->context, impl_->base->revision,
                   "template-scoped nodes cannot receive live events",
                   target.node_, target.name_);
    return *impl_->poison;
  }
  if (!field_is_writable(descriptor->access, impl_->intent, false)) {
    impl_->poison = edit_error(
        error_code::access_denied, impl_->send_operation(), *impl_->context,
        impl_->base->revision, "field does not accept this event intent",
        target.node_, target.name_);
    return *impl_->poison;
  }
  if ((descriptor->kind == value_kind::sf_node ||
       descriptor->kind == value_kind::mf_node) &&
      !node_authority_checked) {
    impl_->poison =
        edit_error(error_code::invalid_context, impl_->send_operation(),
                   *impl_->context, impl_->base->revision,
                   "node-valued events require context-bearing node handles",
                   target.node_, target.name_);
    return *impl_->poison;
  }
  if (kind_of(payload) != descriptor->kind) {
    impl_->poison = edit_error(
        error_code::type_mismatch, impl_->send_operation(), *impl_->context,
        impl_->base->revision, "event payload does not match field descriptor",
        target.node_, target.name_);
    return *impl_->poison;
  }
  if (space != value_space::canonical) {
    auto canonical =
        convert_unit_value(*impl_->base, *descriptor, std::move(payload), space,
                           value_space::canonical);
    if (!canonical) {
      impl_->poison = edit_error(
          error_code::invalid_value, impl_->send_operation(), *impl_->context,
          impl_->base->revision,
          "field representation cannot carry the declared unit category",
          target.node_, target.name_);
      return *impl_->poison;
    }
    payload = std::move(*canonical);
  }
  if (!value_is_well_formed(payload)) {
    impl_->poison = edit_error(
        error_code::invalid_value, impl_->send_operation(), *impl_->context,
        impl_->base->revision, "event violates field representation invariants",
        target.node_, target.name_);
    return *impl_->poison;
  }
  if (const auto issue = node_payload_issue(
          *impl_->context, *impl_->base, found->second, *descriptor, payload)) {
    impl_->poison = edit_error(*issue, impl_->send_operation(), *impl_->context,
                               impl_->base->revision,
                               "node event violates type or declaration scope",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  const auto duplicate = std::find_if(impl_->seeds.begin(), impl_->seeds.end(),
                                      [&](const event_delivery &seed) {
                                        return seed.target == target.node_ &&
                                               seed.field == target.name_;
                                      });
  if (duplicate != impl_->seeds.end()) {
    if (impl_->intent == write_intent::runtime_output)
      return {}; // validation precedes per-field loop suppression
    impl_->poison =
        edit_error(error_code::ambiguous_event_seed, impl_->send_operation(),
                   *impl_->context, impl_->base->revision,
                   "one event batch cannot seed the same field more than once",
                   target.node_, target.name_);
    return *impl_->poison;
  }
  impl_->seeds.push_back(event_delivery{.time = impl_->time,
                                        .target = target.node_,
                                        .field = target.name_,
                                        .payload = std::move(payload),
                                        .cause = std::nullopt,
                                        .route_index = std::nullopt});
  return {};
}

result<event_result> event_batch::commit() {
  static_assert(std::is_nothrow_move_constructible_v<event_result>);
  if (impl_->poison)
    return *impl_->poison;
  if (impl_->committed)
    return edit_error(error_code::poisoned_edit, "event_batch.commit",
                      *impl_->context, impl_->base->revision,
                      "event batch was already committed");
  impl_->committed = true;
  event_processing_guard reservation{impl_->context};
  std::optional<runtime_frame> frame;
  std::shared_ptr<const runtime_factory> factory;
  {
    std::lock_guard lock(impl_->context->mutex);
    if (impl_->context->processing)
      return edit_error(error_code::operation_in_progress, "event_batch.commit",
                        *impl_->context, impl_->base->revision,
                        "runtime evaluation is in progress");
    if (!impl_->context->active)
      return edit_error(error_code::stale_handle, "event_batch.commit",
                        *impl_->context, impl_->base->revision,
                        "execution context is no longer active");
    if (impl_->context->state->revision != impl_->base->revision)
      return edit_error(error_code::stale_revision, "event_batch.commit",
                        *impl_->context, impl_->base->revision,
                        "event batch base revision is stale");
    factory = impl_->context->factory;
    if ((!impl_->seeds.empty() || factory) && impl_->context->last_event_time &&
        !(impl_->context->last_event_time->seconds < impl_->time.seconds))
      return edit_error(error_code::event_time_regression, "event_batch.commit",
                        *impl_->context, impl_->base->revision,
                        "event time must strictly advance");
    impl_->context->processing = true;
    reservation.armed = true;
  }

  if (factory) {
    try {
      auto created = (*factory)(scene_snapshot{impl_->context, impl_->base},
                                impl_->time);
      if (!created)
        return failure(created.error());
      frame.emplace(std::move(*created));
      if (!frame->implementation_)
        return edit_error(error_code::callback_failed, "event_batch.commit",
                          *impl_->context, impl_->base->revision,
                          "runtime factory returned a moved-from frame");
    } catch (const std::exception &exception) {
      return edit_error(error_code::callback_failed, "event_batch.commit",
                        *impl_->context, impl_->base->revision, exception.what());
    } catch (...) {
      return edit_error(error_code::callback_failed, "event_batch.commit",
                        *impl_->context, impl_->base->revision,
                        "runtime factory threw a non-standard exception");
    }
  }
  auto staged = std::make_shared<detail::scene_state>(*impl_->base);
  std::deque<event_delivery> work;
  for (auto &seed : impl_->seeds)
    work.push_back(std::move(seed));
  std::vector<event_delivery> deliveries;
  std::vector<semantic_change> changes;
  std::vector<bool> route_fired(staged->routes.size(), false);
  std::set<std::pair<std::uint64_t, std::string>> output_fired;
  std::vector<std::size_t> output_indices;
  struct runtime_response {
    std::vector<event_delivery> outputs;
    bool ignored = false;
  };
  const auto evaluate =
      [&](std::optional<event_delivery> input) -> result<runtime_response> {
    if (!frame)
      return runtime_response{};
    auto validator = std::make_unique<impl>();
    validator->context = impl_->context;
    validator->base = staged;
    validator->time = impl_->time;
    validator->intent = write_intent::runtime_output;
    auto scoped = std::make_shared<detail::runtime_step_control>();
    scoped->stamp = impl_->time;
    scoped->generation = impl_->context->generation;
    scoped->revision = impl_->base->revision;
    scoped->incoming = std::move(input);
    scoped->outputs =
        std::unique_ptr<event_batch>{new event_batch{std::move(validator)}};
    runtime_step step{scoped};
    result<void> processed;
    try {
      processed = frame->implementation_->process(step);
    } catch (const std::exception &exception) {
      processed =
          edit_error(error_code::callback_failed, "event_batch.commit",
                     *impl_->context, impl_->base->revision, exception.what());
    } catch (...) {
      processed =
          edit_error(error_code::callback_failed, "event_batch.commit",
                     *impl_->context, impl_->base->revision,
                     "runtime processor threw a non-standard exception");
    }
    std::lock_guard step_lock(scoped->mutex);
    scoped->active = false;
    auto poison = std::move(scoped->outputs->impl_->poison);
    runtime_response response{std::move(scoped->outputs->impl_->seeds),
                              scoped->ignored};
    // Retained steps own only immutable input facts, never the working state
    // or a strong context reference that could form a processor ownership
    // cycle.
    scoped->outputs.reset();
    if (poison)
      return *poison;
    if (!processed)
      return failure(processed.error());
    return response;
  };
  const auto queue_outputs = [&](runtime_response &response,
                                 std::optional<std::size_t> cause) {
    for (auto &output : response.outputs) {
      output.origin = event_origin::runtime;
      output.cause = cause;
      work.push_back(std::move(output));
    }
  };
  if (frame) {
    auto prepared = evaluate(std::nullopt);
    if (!prepared)
      return failure(prepared.error());
    queue_outputs(*prepared, std::nullopt);
  }
  while (!work.empty()) {
    const event_delivery incoming = std::move(work.front());
    work.pop_front();
    const auto key = std::pair{incoming.target.value, incoming.field};
    // Validation already happened in emit; suppress repeated output before
    // assigning ledger indices so causal links never need renumbering.
    if (incoming.origin == event_origin::runtime && output_fired.contains(key))
      continue;
    const std::size_t index = deliveries.size();
    deliveries.push_back(incoming);
    auto &record = staged->nodes.at(incoming.target.value);
    const auto *descriptor =
        find_descriptor(*impl_->context, record, incoming.field);
    realize_nodes(*impl_->context, *staged, value{incoming.target}, changes);
    runtime_response response;
    if (incoming.origin != event_origin::runtime) {
      auto evaluated = evaluate(incoming);
      if (!evaluated)
        return failure(evaluated.error());
      response = std::move(*evaluated);
      deliveries[index].ignored = response.ignored;
    }
    if (!response.ignored) {
      // Native outputs and implicit inputOutput echoes share the ISO limit.
      // Repeated native outputs are dropped only after send validation.
      realize_nodes(*impl_->context, *staged, incoming.payload, changes);
      if (descriptor->access == access_type::input_output ||
          incoming.origin == event_origin::runtime) {
        auto &stored = record.fields.at(incoming.field);
        if (!same_representation(stored, incoming.payload)) {
          const value before = stored;
          stored = incoming.payload;
          changes.push_back(graph_change{.kind = change_kind::field_changed,
                                         .node = incoming.target,
                                         .field = incoming.field,
                                         .before = before,
                                         .after = stored,
                                         .index = 0});
        }
      }
      const bool replaced_echo =
          std::any_of(response.outputs.begin(), response.outputs.end(),
                      [&](const event_delivery &output) {
                        return output.target == incoming.target &&
                               output.field == incoming.field;
                      });
      if (descriptor->access != access_type::input_only && !replaced_echo &&
          output_fired.insert(key).second) {
        output_indices.push_back(index);
        for (std::size_t route_index = 0; route_index < staged->routes.size();
             ++route_index) {
          if (route_fired[route_index])
            continue;
          const route &candidate = staged->routes[route_index];
          if (candidate.source != incoming.target ||
              candidate.source_field != incoming.field)
            continue;
          const auto &sink = staged->nodes.at(candidate.sink.value);
          const auto *sink_descriptor =
              find_descriptor(*impl_->context, sink, candidate.sink_field);
          if (const auto issue =
                  node_payload_issue(*impl_->context, *staged, sink,
                                     *sink_descriptor, incoming.payload))
            return edit_error(
                *issue, "event_batch.commit", *impl_->context,
                impl_->base->revision,
                "routed node event violates type or declaration scope",
                candidate.sink, candidate.sink_field);
          route_fired[route_index] = true;
          work.push_back(event_delivery{.time = impl_->time,
                                        .target = candidate.sink,
                                        .field = candidate.sink_field,
                                        .payload = incoming.payload,
                                        .cause = index,
                                        .route_index = route_index,
                                        .origin = event_origin::route});
        }
      }
    }
    queue_outputs(response, index);
  }
  if (auto valid =
          validate_candidate_graph(*impl_->context, *staged,
                                   "event_batch.commit", impl_->base->revision);
      !valid)
    return failure(valid.error());
  if (frame) {
    try {
      auto finished = frame->implementation_->finish();
      if (!finished)
        return failure(finished.error());
      if (finished->has_value() || impl_->base->runtime_state.has_value()) {
        changes.push_back(runtime_state_change{impl_->base->runtime_state,
                                               *finished});
        staged->runtime_state = std::move(*finished);
      }
    } catch (const std::exception &exception) {
      return edit_error(error_code::callback_failed, "event_batch.commit",
                        *impl_->context, impl_->base->revision, exception.what());
    } catch (...) {
      return edit_error(error_code::callback_failed, "event_batch.commit",
                        *impl_->context, impl_->base->revision,
                        "runtime finalization threw a non-standard exception");
    }
  }
  std::vector<event_issue> portability_issues;
  std::map<std::pair<std::uint64_t, std::string>, std::size_t> first_delivery;
  std::map<std::pair<std::uint64_t, std::string>, std::size_t> issue_by_target;
  for (std::size_t index = 0; index < deliveries.size(); ++index) {
    if (deliveries[index].ignored ||
        deliveries[index].origin == event_origin::runtime)
      continue;
    const auto key =
        std::pair{deliveries[index].target.value, deliveries[index].field};
    const auto [found, inserted] = first_delivery.emplace(key, index);
    if (inserted || same_representation(deliveries[found->second].payload,
                                        deliveries[index].payload))
      continue;
    const auto issue = issue_by_target.find(key);
    if (issue == issue_by_target.end()) {
      issue_by_target.emplace(key, portability_issues.size());
      portability_issues.push_back(
          event_issue{.code = event_issue_code::nonportable_fan_in,
                      .target = deliveries[index].target,
                      .field = deliveries[index].field,
                      .delivery_indices = {found->second, index}});
    } else {
      portability_issues[issue->second].delivery_indices.push_back(index);
    }
  }
  event_result result{.time = impl_->time,
                      .before_revision = impl_->base->revision,
                      .after_revision = impl_->base->revision,
                      .deliveries = std::move(deliveries),
                      .state_changes = std::nullopt,
                      .portability_issues = std::move(portability_issues)};
  if (!changes.empty()) {
    staged->revision = impl_->base->revision + 1;
    result.after_revision = staged->revision;
    result.state_changes.emplace(
        change_set{.before_revision = impl_->base->revision,
                   .after_revision = staged->revision,
                   .changes = std::move(changes)});
  }
  {
    std::lock_guard lock(impl_->context->mutex);
    decltype(impl_->context->pending) notifications;
    for (const auto index : output_indices) {
      const auto &delivery = result.deliveries[index];
      std::vector<std::uint64_t> recipients;
      for (const auto &[observer_id, observer] :
           impl_->context->event_observers)
        if (observer.target == delivery.target &&
            observer.field == delivery.field)
          recipients.push_back(observer_id);
      if (!recipients.empty())
        notifications.push_back(detail::context_control::pending_event{
            .delivery = delivery, .recipients = std::move(recipients)});
    }
    if (result.state_changes && !impl_->context->observers.empty()) {
      std::vector<std::uint64_t> recipients;
      for (const auto &[observer_id, callback] : impl_->context->observers) {
        (void)callback;
        recipients.push_back(observer_id);
      }
      notifications.push_back(detail::context_control::pending_change{
          .changes = *result.state_changes,
          .recipients = std::move(recipients)});
    }
    if (result.state_changes) {
      impl_->context->state = staged;
      impl_->context->published_revision.store(staged->revision,
                                               std::memory_order_release);
    }
    impl_->context->pending.splice(impl_->context->pending.end(),
                                   notifications);
    if (!result.deliveries.empty() || factory)
      impl_->context->last_event_time = impl_->time;
  }
  return result;
}

scene_snapshot::scene_snapshot(std::shared_ptr<detail::context_control> context,
                               std::shared_ptr<const detail::scene_state> state)
    : context_(std::move(context)), state_(std::move(state)) {
  import_contexts_.reserve(state_->imports.size());
  import_states_.reserve(state_->imports.size());
  for (std::size_t index = 0; index < state_->imports.size(); ++index) {
    const auto source = state_->import_sources[index].lock();
    if (!source) {
      import_contexts_.push_back({});
      import_states_.push_back({});
      continue;
    }
    std::lock_guard lock(source->mutex);
    if (!source->active ||
        source->generation != state_->imports[index].source_generation) {
      import_contexts_.push_back({});
      import_states_.push_back({});
      continue;
    }
    import_contexts_.push_back(source);
    import_states_.push_back(source->state);
  }
}

const std::any &scene_snapshot::runtime_state_storage() const noexcept {
  return state_->runtime_state;
}

revision_id scene_snapshot::revision() const noexcept {
  return state_->revision;
}

const std::vector<unit_declaration> &scene_snapshot::units() const noexcept {
  return state_->units;
}

std::vector<node_type_descriptor> scene_snapshot::node_types() const {
  return context_->registry.node_types();
}

const std::vector<metadata_entry> &scene_snapshot::metadata() const noexcept {
  return state_->metadata;
}

std::optional<std::string> scene_snapshot::metadata(std::string_view key) const {
  const auto found = std::ranges::find(state_->metadata, key, &metadata_entry::key);
  if (found == state_->metadata.end())
    return std::nullopt;
  return found->value;
}

const std::vector<node_id> &scene_snapshot::roots() const noexcept {
  return state_->roots;
}

std::vector<occurrence> scene_snapshot::occurrences() const {
  std::vector<occurrence> result;
  std::function<void(node_id, std::optional<std::size_t>, std::string,
                     std::size_t, std::vector<std::size_t>)>
      walk = [&](node_id id, std::optional<std::size_t> parent,
                 std::string container, std::size_t index,
                 std::vector<std::size_t> path) {
        const std::size_t occurrence_index = result.size();
        result.push_back(occurrence{.node = id,
                                    .parent_occurrence = parent,
                                    .container_field = std::move(container),
                                    .index = index,
                                    .path = path});
        const auto &record = state_->nodes.at(id.value);
        const auto *type = context_->registry.find(record.type_name);
        for (const auto &descriptor : type->fields) {
          if (!descriptor.containment)
            continue;
          const auto &field_value = record.fields.at(descriptor.name);
          if (descriptor.kind == value_kind::sf_node) {
            const node_id child = std::get<node_id>(field_value);
            if (child.value != 0) {
              auto child_path = path;
              child_path.push_back(0);
              walk(child, occurrence_index, descriptor.name, 0,
                   std::move(child_path));
            }
          } else if (descriptor.kind == value_kind::mf_node) {
            const auto &children = std::get<node_list>(field_value);
            for (std::size_t child_index = 0; child_index < children.size();
                 ++child_index) {
              if (children[child_index].value == 0)
                continue;
              auto child_path = path;
              child_path.push_back(child_index);
              walk(children[child_index], occurrence_index, descriptor.name,
                   child_index, std::move(child_path));
            }
          }
        }
      };
  for (std::size_t root_index = 0; root_index < state_->roots.size();
       ++root_index) {
    walk(state_->roots[root_index], std::nullopt, {}, root_index, {root_index});
  }
  return result;
}

result<dynamic_field> scene_snapshot::field(const node &owner,
                                            const std::string &name) const {
  const auto owner_context = owner.context_.lock();
  const auto found = state_->nodes.find(owner.id_.value);
  if (owner_context != context_ || owner.generation_ != context_->generation ||
      found == state_->nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_snapshot.field",
                      *context_, state_->revision, "invalid node handle",
                      owner.id_, name);
  }
  const auto *descriptor = find_descriptor(*context_, found->second, name);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "scene_snapshot.field",
                      *context_, state_->revision, "unknown field: " + name,
                      owner.id_, name);
  }
  return dynamic_field{context_, context_->generation, owner.id_,
                       name,     descriptor->kind,     descriptor->access};
}

result<node> scene_snapshot::lookup(node_id id) const {
  if (!state_->nodes.contains(id.value)) {
    return edit_error(error_code::unknown_node, "scene_snapshot.lookup",
                      *context_, state_->revision,
                      "node identity is not present in this snapshot", id);
  }
  return node{context_, context_->generation, id};
}

const std::vector<declaration_id> &
scene_snapshot::declarations() const noexcept {
  return state_->declaration_order;
}

result<declaration> scene_snapshot::declaration_at(declaration_id id) const {
  if (!state_->declarations.contains(id.value)) {
    return edit_error(error_code::invalid_descriptor,
                      "scene_snapshot.declaration_at", *context_,
                      state_->revision,
                      "declaration identity is not present in this snapshot");
  }
  return declaration{context_, context_->generation, id};
}

result<declaration>
scene_snapshot::declaration_named(std::string_view requested_name) const {
  const auto found = std::ranges::find_if(
      state_->declaration_order, [&](declaration_id candidate) {
        return experimental::name(state_->declarations.at(candidate.value)) ==
               requested_name;
      });
  if (found == state_->declaration_order.end()) {
    return edit_error(
        error_code::invalid_name, "scene_snapshot.declaration_named", *context_,
        state_->revision,
        "declaration name is not defined: " + std::string{requested_name});
  }
  return declaration{context_, context_->generation, *found};
}

result<void> scene_snapshot::require_node_type(
    const node &owner, std::string_view type_name,
    std::optional<std::string_view> schema_fingerprint) const {
  auto descriptor = describe(owner);
  if (!descriptor)
    return failure(descriptor.error());
  if (descriptor->name != type_name &&
      std::ranges::find(descriptor->interfaces, type_name) ==
          descriptor->interfaces.end()) {
    return edit_error(error_code::type_mismatch, "scene_snapshot.as", *context_,
                      state_->revision,
                      "node does not implement the requested type", owner.id_);
  }
  if (schema_fingerprint) {
    const auto actual = context_->registry.schema_fingerprint();
    if (!actual || *actual != *schema_fingerprint) {
      return edit_error(
          error_code::invalid_descriptor, "scene_snapshot.as", *context_,
          state_->revision,
          "generated node binding does not match the registry schema",
          owner.id_);
    }
  }
  return {};
}

result<bool> scene_snapshot::is_realized(const node &owner) const {
  const auto found = state_->nodes.find(owner.id_.value);
  if (owner.context_.lock() != context_ ||
      owner.generation_ != context_->generation ||
      found == state_->nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_snapshot.is_realized",
                      *context_, state_->revision, "invalid node handle",
                      owner.id_);
  }
  return found->second.realized;
}

result<node_type_descriptor> scene_snapshot::describe(const node &owner) const {
  const auto owner_context = owner.context_.lock();
  const auto found = state_->nodes.find(owner.id_.value);
  if (owner_context != context_ || owner.generation_ != context_->generation ||
      found == state_->nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_snapshot.describe",
                      *context_, state_->revision, "invalid node handle",
                      owner.id_);
  }
  const auto *descriptor = context_->registry.find(found->second.type_name);
  if (!descriptor) {
    return edit_error(error_code::unknown_type, "scene_snapshot.describe",
                      *context_, state_->revision,
                      "node type is absent from the context registry",
                      owner.id_);
  }
  return *descriptor;
}

result<declaration_descriptor>
scene_snapshot::describe(const declaration &owner) const {
  const auto owner_context = owner.context_.lock();
  const auto found = state_->declarations.find(owner.id_.value);
  if (owner_context != context_ || owner.generation_ != context_->generation ||
      found == state_->declarations.end()) {
    return declaration_edit_error(
        error_code::stale_handle, "scene_snapshot.describe_declaration",
        *context_, state_->revision, "invalid declaration handle", owner.id_);
  }
  return found->second;
}

result<node_type_descriptor>
scene_snapshot::describe(const imported_node &owner) const {
  const auto importer = owner.importer_.lock();
  const auto source = owner.source_.lock();
  if (importer != context_ ||
      owner.importer_generation_ != context_->generation) {
    return edit_error(
        error_code::invalid_context, "scene_snapshot.describe_import",
        *context_, state_->revision,
        "imported node belongs to another snapshot context", owner.id_);
  }
  if (!source || source->generation != owner.source_generation_) {
    return edit_error(
        error_code::stale_handle, "scene_snapshot.describe_import", *context_,
        state_->revision, "invalid imported node handle", owner.id_);
  }
  const auto binding =
      std::find_if(state_->imports.begin(), state_->imports.end(),
                   [&](const import_binding &candidate) {
                     return candidate.local_name == owner.local_name_ &&
                            candidate.target == owner.identity();
                   });
  if (binding == state_->imports.end()) {
    return edit_error(
        error_code::invalid_context, "scene_snapshot.describe_import",
        *context_, state_->revision,
        "imported node does not belong to this snapshot", owner.id_);
  }
  const auto index =
      static_cast<std::size_t>(std::distance(state_->imports.begin(), binding));
  const auto &source_state = import_states_[index];
  if (!source_state || source_state->revision != owner.source_revision_) {
    return edit_error(
        error_code::stale_handle, "scene_snapshot.describe_import", *context_,
        state_->revision, "imported node source revision is unavailable",
        owner.id_);
  }
  const auto found = source_state->nodes.find(owner.id_.value);
  if (found == source_state->nodes.end()) {
    return edit_error(error_code::unknown_node,
                      "scene_snapshot.describe_import", *context_,
                      state_->revision, "imported node is absent", owner.id_);
  }
  const auto *descriptor = source->registry.find(found->second.type_name);
  if (!descriptor) {
    return edit_error(
        error_code::unknown_type, "scene_snapshot.describe_import", *context_,
        state_->revision, "imported node type is absent", owner.id_);
  }
  return *descriptor;
}

result<dynamic_imported_field>
scene_snapshot::field(const imported_node &owner,
                      const std::string &name) const {
  auto described = describe(owner);
  if (!described)
    return failure(described.error());
  const auto descriptor = std::find_if(described.value().fields.begin(),
                                       described.value().fields.end(),
                                       [&](const field_descriptor &candidate) {
                                         return candidate.name == name;
                                       });
  if (descriptor == described.value().fields.end()) {
    return edit_error(error_code::unknown_field, "scene_snapshot.field_import",
                      *context_, state_->revision,
                      "unknown imported field: " + name, owner.id_, name);
  }
  return dynamic_imported_field{context_,
                                context_->generation,
                                owner.source_,
                                owner.source_generation_,
                                owner.source_revision_,
                                owner.id_,
                                name,
                                descriptor->kind,
                                descriptor->access};
}

result<bool> scene_snapshot::readable(const dynamic_field &source) const {
  const auto source_context = source.context_.lock();
  const auto found = state_->nodes.find(source.node_.value);
  if (source_context != context_ ||
      source.generation_ != context_->generation ||
      found == state_->nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_snapshot.readable",
                      *context_, state_->revision, "invalid field handle",
                      source.node_, source.name_);
  }
  const auto *descriptor =
      find_descriptor(*context_, found->second, source.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "scene_snapshot.readable",
                      *context_, state_->revision,
                      "unknown field: " + source.name_, source.node_,
                      source.name_);
  }
  return field_is_readable(descriptor->access);
}

result<std::shared_ptr<const value>>
scene_snapshot::read_storage(const dynamic_field &source,
                             const char *operation) const {
  return read_storage(context_, state_, source, operation);
}

result<std::shared_ptr<const value>> scene_snapshot::read_storage(
    const std::shared_ptr<detail::context_control> &context,
    const std::shared_ptr<const detail::scene_state> &state,
    const dynamic_field &source, const char *operation) {
  const auto found = state->nodes.find(source.node_.value);
  if (!same_owner(source.context_, context) ||
      source.generation_ != context->generation ||
      found == state->nodes.end()) {
    return edit_error(error_code::stale_handle, operation, *context,
                      state->revision, "invalid field handle", source.node_,
                      source.name_);
  }
  const auto *descriptor =
      find_descriptor(*context, found->second, source.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, operation, *context,
                      state->revision, "unknown field: " + source.name_,
                      source.node_, source.name_);
  }
  if (!field_is_readable(descriptor->access)) {
    return edit_error(error_code::access_denied, operation, *context,
                      state->revision, "inputOnly field is not readable",
                      source.node_, source.name_);
  }
  return std::shared_ptr<const value>{state,
                                      &found->second.fields.at(source.name_)};
}

result<std::shared_ptr<const value>>
scene_snapshot::read_view(const dynamic_field &source) const {
  return read_storage(source, "scene_snapshot.read_view");
}

result<value> scene_snapshot::read(const dynamic_field &source) const {
  auto stored = read_storage(source, "scene_snapshot.read");
  if (!stored)
    return failure(stored.error());
  return **stored;
}

result<node> scene_snapshot::read_node(const dynamic_field &source) const {
  auto stored = read_storage(source, "scene_snapshot.read_node");
  if (!stored)
    return failure(stored.error());
  const auto *id = std::get_if<node_id>(stored->get());
  if (!id) {
    return edit_error(error_code::type_mismatch, "scene_snapshot.read_node",
                      *context_, state_->revision, "field is not SFNode",
                      source.node_, source.name_);
  }
  return id->value == 0 ? result<node>{node{}} : lookup(*id);
}

result<std::vector<node>>
scene_snapshot::read_nodes(const dynamic_field &source) const {
  auto stored = read_storage(source, "scene_snapshot.read_nodes");
  if (!stored)
    return failure(stored.error());
  const auto *ids = std::get_if<node_list>(stored->get());
  if (!ids) {
    return edit_error(error_code::type_mismatch, "scene_snapshot.read_nodes",
                      *context_, state_->revision, "field is not MFNode",
                      source.node_, source.name_);
  }
  std::vector<node> nodes;
  nodes.reserve(ids->size());
  for (const auto id : *ids) {
    if (id.value == 0) {
      nodes.emplace_back();
      continue;
    }
    auto handle = lookup(id);
    if (!handle)
      return failure(handle.error());
    nodes.push_back(std::move(handle).value());
  }
  return nodes;
}

result<value> scene_snapshot::read(const dynamic_field &source,
                                   value_space space) const {
  auto stored = read(source);
  if (!stored || space == value_space::canonical)
    return stored;
  const auto found = state_->nodes.find(source.node_.value);
  const auto *descriptor =
      found == state_->nodes.end()
          ? nullptr
          : find_descriptor(*context_, found->second, source.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "scene_snapshot.read",
                      *context_, state_->revision, "unknown unit-bearing field",
                      source.node_, source.name_);
  }
  auto authored =
      convert_unit_value(*state_, *descriptor, std::move(stored).value(),
                         value_space::canonical, space);
  if (!authored) {
    return edit_error(
        error_code::invalid_value, "scene_snapshot.read", *context_,
        state_->revision,
        "field representation cannot carry the declared unit category",
        source.node_, source.name_);
  }
  return std::move(*authored);
}

result<value> scene_snapshot::read(const dynamic_imported_field &field) const {
  return read(field, value_space::canonical);
}

result<std::shared_ptr<const value>>
scene_snapshot::read_storage(const dynamic_imported_field &field,
                             const char *operation) const {
  const auto importer = field.importer_.lock();
  const auto source = field.source_.lock();
  if (importer != context_ ||
      field.importer_generation_ != context_->generation) {
    return edit_error(error_code::invalid_context, operation, *context_,
                      state_->revision,
                      "imported field belongs to another snapshot context",
                      field.node_, field.name_);
  }
  if (!source || source->generation != field.source_generation_) {
    return edit_error(error_code::stale_handle, operation, *context_,
                      state_->revision, "invalid imported field handle",
                      field.node_, field.name_);
  }
  std::optional<std::size_t> binding_index;
  for (std::size_t index = 0; index < state_->imports.size(); ++index) {
    if (state_->imports[index].target == field.node() &&
        state_->import_sources[index].lock() == source) {
      binding_index = index;
      break;
    }
  }
  if (!binding_index) {
    return edit_error(error_code::invalid_context, operation, *context_,
                      state_->revision,
                      "imported field does not belong to this snapshot",
                      field.node_, field.name_);
  }
  const auto &source_state = import_states_[*binding_index];
  if (!source_state || source_state->revision != field.source_revision_) {
    return edit_error(error_code::stale_handle, operation, *context_,
                      state_->revision,
                      "imported field source revision is unavailable",
                      field.node_, field.name_);
  }
  const auto found = source_state->nodes.find(field.node_.value);
  if (found == source_state->nodes.end()) {
    return edit_error(error_code::unknown_node, operation, *context_,
                      state_->revision, "imported field owner is absent",
                      field.node_, field.name_);
  }
  const auto *descriptor = find_descriptor(*source, found->second, field.name_);
  if (!descriptor || descriptor->kind != field.kind_ ||
      descriptor->access != field.access_) {
    return edit_error(error_code::unknown_field, operation, *context_,
                      state_->revision, "imported field descriptor changed",
                      field.node_, field.name_);
  }
  if (descriptor->access == access_type::input_only) {
    return edit_error(
        error_code::access_denied, operation, *context_, state_->revision,
        "inputOnly imported field is not readable", field.node_, field.name_);
  }
  return std::shared_ptr<const value>{source_state,
                                      &found->second.fields.at(field.name_)};
}

result<std::shared_ptr<const value>>
scene_snapshot::read_view(const dynamic_imported_field &field) const {
  return read_storage(field, "scene_snapshot.read_view_import");
}

result<value> scene_snapshot::read(const dynamic_imported_field &field,
                                   value_space space) const {
  auto view = read_storage(field, "scene_snapshot.read_import");
  if (!view)
    return failure(view.error());
  value stored = **view;
  if (space == value_space::canonical)
    return stored;
  // The checked view above guarantees this source and binding are pinned.
  const auto source = field.source_.lock();
  std::size_t index = 0;
  while (index < state_->imports.size() &&
         (state_->imports[index].target != field.node() ||
          import_contexts_[index] != source))
    ++index;
  if (index == state_->imports.size()) {
    return edit_error(error_code::invalid_context, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "imported field source state is unavailable", field.node_,
                      field.name_);
  }
  const auto &source_state = import_states_[index];
  const auto &record = source_state->nodes.at(field.node_.value);
  const auto *descriptor = find_descriptor(*source, record, field.name_);
  auto authored =
      convert_unit_value(*source_state, *descriptor, std::move(stored),
                         value_space::canonical, space);
  if (!authored) {
    return edit_error(
        error_code::invalid_value, "scene_snapshot.read_import", *context_,
        state_->revision,
        "imported field representation cannot carry its unit category",
        field.node_, field.name_);
  }
  return std::move(*authored);
}

result<node> scene_snapshot::named(const std::string &name) const {
  const auto found = std::find_if(
      state_->names.begin(), state_->names.end(),
      [&](const name_binding &binding) { return binding.name == name; });
  if (found == state_->names.end()) {
    return edit_error(error_code::invalid_name, "scene_snapshot.named",
                      *context_, state_->revision,
                      "name is not defined: " + name);
  }
  return node{context_, context_->generation, found->node};
}

const std::vector<name_binding> &scene_snapshot::names() const noexcept {
  return state_->names;
}

result<node> scene_snapshot::exported(const std::string &name) const {
  const auto found = std::find_if(
      state_->exports.begin(), state_->exports.end(),
      [&](const export_binding &binding) { return binding.name == name; });
  if (found == state_->exports.end()) {
    return edit_error(error_code::invalid_name, "scene_snapshot.exported",
                      *context_, state_->revision,
                      "export is not defined: " + name);
  }
  return node{context_, context_->generation, found->node};
}

const std::vector<export_binding> &scene_snapshot::exports() const noexcept {
  return state_->exports;
}

result<imported_node>
scene_snapshot::imported(const std::string &local_name) const {
  const auto found =
      std::find_if(state_->imports.begin(), state_->imports.end(),
                   [&](const import_binding &binding) {
                     return binding.local_name == local_name;
                   });
  if (found == state_->imports.end()) {
    return edit_error(error_code::invalid_name, "scene_snapshot.imported",
                      *context_, state_->revision,
                      "import is not defined: " + local_name);
  }
  const auto index =
      static_cast<std::size_t>(std::distance(state_->imports.begin(), found));
  const auto source = state_->import_sources[index].lock();
  const auto &source_state = import_states_[index];
  if (!source || source->generation != found->source_generation ||
      !source_state) {
    return edit_error(error_code::stale_handle, "scene_snapshot.imported",
                      *context_, state_->revision,
                      "import source generation is stale", found->target.local);
  }
  const auto exported =
      std::find_if(source_state->exports.begin(), source_state->exports.end(),
                   [&](const export_binding &candidate) {
                     return candidate.name == found->exported_name;
                   });
  if (exported == source_state->exports.end() ||
      exported->node != found->target.local) {
    return edit_error(error_code::stale_aperture, "scene_snapshot.imported",
                      *context_, state_->revision,
                      "captured source export no longer resolves this import",
                      found->target.local);
  }
  return imported_node{context_,
                       context_->generation,
                       source,
                       found->source_generation,
                       source_state->revision,
                       found->target.local,
                       found->local_name};
}

const std::vector<import_binding> &scene_snapshot::imports() const noexcept {
  return state_->imports;
}

const std::vector<route> &scene_snapshot::routes() const noexcept {
  return state_->routes;
}

execution_context::execution_context(
    std::shared_ptr<detail::context_control> control)
    : control_(std::move(control)) {}

generation_id execution_context::generation() const noexcept {
  return control_->generation;
}

scene_snapshot execution_context::snapshot() const {
  std::shared_ptr<const detail::scene_state> state;
  {
    std::lock_guard lock(control_->mutex);
    state = control_->state;
  }
  return scene_snapshot{control_, std::move(state)};
}

scene_edit execution_context::edit() const {
  std::lock_guard lock(control_->mutex);
  auto implementation = std::make_shared<detail::scene_edit_control>();
  implementation->context = control_;
  implementation->base = control_->state;
  implementation->staged = *control_->state;
  return scene_edit{std::move(implementation)};
}

event_batch execution_context::events(event_time time) const {
  std::lock_guard lock(control_->mutex);
  auto implementation = std::make_unique<event_batch::impl>();
  implementation->context = control_;
  implementation->base = control_->state;
  implementation->time = time;
  if (!std::isfinite(time.seconds)) {
    implementation->poison = edit_error(
        error_code::invalid_descriptor, "execution_context.events", *control_,
        control_->state->revision, "event time must be finite");
  }
  return event_batch{std::move(implementation)};
}

subscription execution_context::observe(observer callback) const {
  if (!callback)
    return {};
  // Retain an owner outside the lock even if registration allocation fails.
  const auto owned_callback =
      std::make_shared<const observer>(std::move(callback));
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return {};
  const std::uint64_t observer_id = control_->next_observer_id++;
  control_->observers.emplace(observer_id, owned_callback);
  return subscription{control_, observer_id};
}

result<subscription> execution_context::observe(const dynamic_field &source,
                                                event_observer callback) const {
  if (!callback) {
    return edit_error(
        error_code::invalid_descriptor, "execution_context.observe_event",
        *control_, control_->published_revision.load(std::memory_order_acquire),
        "event observer is empty", source.node_, source.name_);
  }
  const auto owned_callback =
      std::make_shared<const event_observer>(std::move(callback));
  std::lock_guard lock(control_->mutex);
  if (!control_->active) {
    return edit_error(
        error_code::stale_handle, "execution_context.observe_event", *control_,
        control_->state->revision, "execution context is no longer active",
        source.node_, source.name_);
  }
  if (!same_owner(source.context_, control_) ||
      source.generation_ != control_->generation) {
    return edit_error(error_code::invalid_context,
                      "execution_context.observe_event", *control_,
                      control_->state->revision,
                      "field belongs to another execution context",
                      source.node_, source.name_);
  }
  const auto found = control_->state->nodes.find(source.node_.value);
  if (found == control_->state->nodes.end()) {
    return edit_error(error_code::unknown_node,
                      "execution_context.observe_event", *control_,
                      control_->state->revision, "field owner does not exist",
                      source.node_, source.name_);
  }
  const auto *descriptor =
      find_descriptor(*control_, found->second, source.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field,
                      "execution_context.observe_event", *control_,
                      control_->state->revision, "field does not exist",
                      source.node_, source.name_);
  }
  if (!route_source_allowed(descriptor->access)) {
    return edit_error(
        error_code::access_denied, "execution_context.observe_event", *control_,
        control_->state->revision,
        "only outputOnly and inputOutput fields emit observable events",
        source.node_, source.name_);
  }
  const std::uint64_t observer_id = control_->next_observer_id++;
  control_->event_observers.emplace(
      observer_id, detail::context_control::field_observer_record{
                       .target = source.node_,
                       .field = source.name_,
                       .callback = owned_callback});
  return subscription{control_, observer_id};
}

dispatch_report execution_context::drain() const {
  dispatch_report report;
  if (control_->draining.exchange(true, std::memory_order_acquire)) {
    report.already_draining = true;
    return report;
  }
  struct drain_guard {
    std::atomic_bool &draining;
    ~drain_guard() { draining.store(false, std::memory_order_release); }
  } guard{control_->draining};
  for (;;) {
    detail::context_control::pending_notification pending;
    {
      std::lock_guard lock(control_->mutex);
      if (control_->pending.empty())
        break;
      pending = std::move(control_->pending.front());
      control_->pending.pop_front();
    }
    std::visit(
        [&](auto &notification) {
          for (std::uint64_t observer_id : notification.recipients) {
            try {
              if constexpr (std::same_as<
                                std::remove_cvref_t<decltype(notification)>,
                                detail::context_control::pending_change>) {
                std::shared_ptr<const observer> callback;
                {
                  std::lock_guard lock(control_->mutex);
                  const auto found = control_->observers.find(observer_id);
                  if (found == control_->observers.end())
                    continue;
                  callback = found->second;
                }
                (*callback)(notification.changes);
              } else {
                std::shared_ptr<const event_observer> callback;
                {
                  std::lock_guard lock(control_->mutex);
                  const auto found =
                      control_->event_observers.find(observer_id);
                  if (found == control_->event_observers.end())
                    continue;
                  callback = found->second.callback;
                }
                (*callback)(notification.delivery);
              }
              ++report.delivered;
            } catch (const std::exception &exception) {
              sai_error error;
              error.code = error_code::callback_failed;
              error.operation = "execution_context.drain";
              error.message = exception.what();
              error.generation = control_->generation;
              error.current_revision =
                  control_->published_revision.load(std::memory_order_acquire);
              report.errors.push_back(std::move(error));
            } catch (...) {
              sai_error error;
              error.code = error_code::callback_failed;
              error.operation = "execution_context.drain";
              error.message = "observer threw a non-standard exception";
              error.generation = control_->generation;
              error.current_revision =
                  control_->published_revision.load(std::memory_order_acquire);
              report.errors.push_back(std::move(error));
            }
          }
        },
        pending);
  }
  return report;
}

browser::browser(type_registry registry)
    : control_(std::make_shared<detail::browser_control>()) {
  control_->registry = registry;
  auto context = std::make_shared<detail::context_control>();
  context->owner = control_;
  context->generation = 1;
  context->registry = std::move(registry);
  control_->current = std::move(context);
}

capability_set browser::capabilities() const noexcept {
  return control_->capabilities;
}

execution_context browser::current_scene() const {
  std::lock_guard lock(control_->mutex);
  return execution_context{control_->current};
}

execution_context browser::create_scene() {
  std::lock_guard lock(control_->mutex);
  auto context = std::make_shared<detail::context_control>();
  context->owner = control_;
  context->generation = control_->next_generation++;
  context->registry = control_->registry;
  return execution_context{std::move(context)};
}

result<void> browser::set_runtime_processor(const execution_context &scene,
                                            runtime_processor processor) {
  runtime_factory factory;
  if (processor) {
    auto shared = std::make_shared<const runtime_processor>(std::move(processor));
    factory = [shared = std::move(shared)](const scene_snapshot &, event_time)
        -> result<runtime_frame> { return runtime_frame{shared}; };
  }
  return install_runtime_factory(scene, std::move(factory),
                                  "browser.set_runtime_processor");
}

result<void> browser::set_runtime_factory(const execution_context &scene,
                                          runtime_factory factory) {
  return install_runtime_factory(scene, std::move(factory),
                                  "browser.set_runtime_factory");
}

result<void> browser::install_runtime_factory(const execution_context &scene,
                                              runtime_factory factory,
                                              const char *operation) {
  if (!scene.control_ || scene.control_->owner.lock() != control_)
    return descriptor_error(error_code::invalid_context, operation,
                            "scene belongs to another browser");
  auto installed = factory
      ? std::make_shared<const runtime_factory>(std::move(factory))
      : std::shared_ptr<const runtime_factory>{};
  std::shared_ptr<const runtime_factory> retired;
  {
    std::lock_guard lock(scene.control_->mutex);
    if (!scene.control_->active)
      return edit_error(error_code::stale_handle, operation, *scene.control_,
                        scene.control_->state->revision, "scene is retired");
    if (scene.control_->processing)
      return edit_error(error_code::operation_in_progress, operation,
                        *scene.control_, scene.control_->state->revision,
                        "runtime evaluation is in progress");
    retired = std::exchange(scene.control_->factory, std::move(installed));
  }
  return {}; // callable destruction is outside the context lock
}

namespace {
struct retired_context_callbacks {
  std::shared_ptr<detail::context_control> context;
  decltype(detail::context_control::observers) changes;
  decltype(detail::context_control::event_observers) fields;
  decltype(detail::context_control::pending) pending;
  std::shared_ptr<const runtime_factory> factory;
};

// The caller holds the browser lock through ticket validation and publication.
// It destroys retired callables after releasing that lock.
result<void> replace_world_locked(
    const std::shared_ptr<detail::browser_control> &browser,
    const std::shared_ptr<detail::context_control> &replacement,
    retired_context_callbacks &retired) {
  if (!replacement)
    return descriptor_error(error_code::stale_handle, "browser.replace_world",
                            "replacement context is empty");
  if (!same_owner(replacement->owner, browser))
    return descriptor_error(error_code::invalid_context,
                            "browser.replace_world",
                            "replacement context belongs to another browser");
  if (replacement == browser->current)
    return {};
  // Keep the retired context alive until both of its locks are released.
  const auto current = browser->current;
  std::scoped_lock context_locks(replacement->mutex, current->mutex);
  if (!replacement->active)
    return edit_error(error_code::stale_handle, "browser.replace_world",
                      *replacement, replacement->state->revision,
                      "replacement context is stale");
  if (replacement->processing || current->processing)
    return edit_error(error_code::operation_in_progress,
                      "browser.replace_world", *current,
                      current->state->revision,
                      "world replacement conflicts with runtime evaluation");
  current->active = false;
  retired.changes.swap(current->observers);
  retired.fields.swap(current->event_observers);
  retired.pending.swap(current->pending);
  retired.factory.swap(current->factory);
  retired.context = current;
  browser->current = replacement;
  ++browser->world_epoch;
  return {};
}
} // namespace

result<void> browser::replace_world(const execution_context &replacement) {
  retired_context_callbacks retired;
  std::lock_guard browser_lock(control_->mutex);
  return replace_world_locked(control_, replacement.control_, retired);
}

load_ticket browser::begin_load() {
  std::lock_guard lock(control_->mutex);
  const std::uint64_t request_id = control_->next_request_id++;
  control_->requests.emplace(
      request_id,
      detail::browser_control::request_record{
          .world_epoch = control_->world_epoch, .cancelled = false});
  return load_ticket{control_, request_id, control_->world_epoch};
}

result<void> browser::cancel(const load_ticket &ticket) {
  const auto owner = ticket.browser_.lock();
  if (owner != control_) {
    sai_error error;
    error.code = error_code::stale_completion;
    error.operation = "browser.cancel";
    error.message = "load ticket belongs to another browser";
    return failure(std::move(error));
  }
  std::lock_guard lock(control_->mutex);
  const auto found = control_->requests.find(ticket.request_id_);
  if (found == control_->requests.end() ||
      found->second.world_epoch != control_->world_epoch) {
    sai_error error;
    error.code = error_code::stale_completion;
    error.operation = "browser.cancel";
    error.message = "load ticket is stale";
    return failure(std::move(error));
  }
  found->second.cancelled = true;
  return {};
}

result<void> browser::complete_load(const load_ticket &ticket,
                                    const execution_context &replacement) {
  const auto owner = ticket.browser_.lock();
  if (owner != control_) {
    sai_error error;
    error.code = error_code::stale_completion;
    error.operation = "browser.complete_load";
    error.message = "load ticket belongs to another browser";
    return failure(std::move(error));
  }
  retired_context_callbacks retired;
  std::lock_guard lock(control_->mutex);
  const auto found = control_->requests.find(ticket.request_id_);
  if (found == control_->requests.end()) {
    sai_error error;
    error.code = error_code::stale_completion;
    error.operation = "browser.complete_load";
    error.message = "load ticket is unknown or already completed";
    return failure(std::move(error));
  }
  if (found->second.cancelled) {
    sai_error error;
    error.code = error_code::cancelled;
    error.operation = "browser.complete_load";
    error.message = "load request was cancelled";
    return failure(std::move(error));
  }
  if (found->second.world_epoch != control_->world_epoch ||
      ticket.world_epoch_ != control_->world_epoch) {
    sai_error error;
    error.code = error_code::stale_completion;
    error.operation = "browser.complete_load";
    error.message = "browser world changed after the load began";
    return failure(std::move(error));
  }
  auto published =
      replace_world_locked(control_, replacement.control_, retired);
  if (!published)
    return published;
  control_->requests.erase(found);
  return {};
}

} // namespace x3d::sai::experimental
