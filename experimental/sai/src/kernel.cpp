#include "x3d/sai/experimental/kernel.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace x3d::sai::experimental {

namespace {

sai_error descriptor_error(error_code code, std::string operation,
                           std::string message) {
  sai_error error;
  error.code = code;
  error.operation = std::move(operation);
  error.message = std::move(message);
  return error;
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

bool value_is_well_formed(const value &candidate) {
  return std::visit(
      []<class T>(const T &stored) {
        if constexpr (std::is_same_v<T, image>)
          return valid_image(stored);
        else if constexpr (std::is_same_v<T, image_list>)
          return std::ranges::all_of(stored, valid_image);
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
          "default value does not match field kind: " + field.name);
    }
    if (field.containment && field.kind != value_kind::node &&
        field.kind != value_kind::node_list) {
      return descriptor_error(
          error_code::invalid_descriptor, "type_registry.define",
          "containment field is not node-valued: " + field.name);
    }
  }
  types_.emplace(descriptor.name, std::move(descriptor));
  return {};
}

const node_type_descriptor *
type_registry::find(const std::string &name) const noexcept {
  const auto found = types_.find(name);
  return found == types_.end() ? nullptr : &found->second;
}

namespace detail {

struct scene_state {
  struct node_record {
    std::string type_name;
    std::unordered_map<std::string, value> fields;
  };

  revision_id revision = 0;
  std::uint64_t next_node_id = 1;
  std::unordered_map<std::uint64_t, node_record> nodes;
  std::vector<node_id> roots;
  std::vector<name_binding> names;
  std::vector<export_binding> exports;
  std::vector<import_binding> imports;
  std::vector<std::weak_ptr<context_control>> import_sources;
  std::vector<route> routes;
};

struct context_control {
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
  bool active = true;
  type_registry registry;
  std::shared_ptr<const scene_state> state = std::make_shared<scene_state>();
  std::atomic<revision_id> published_revision{0};
  std::uint64_t next_observer_id = 1;
  std::map<std::uint64_t, std::shared_ptr<const execution_context::observer>>
      observers;
  std::map<std::uint64_t, field_observer_record> event_observers;
  std::deque<pending_notification> pending;
  std::optional<event_time> last_event_time;
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

sai_error edit_error(error_code code, const char *operation,
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
  return error;
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

bool accepts_node_payload(const detail::context_control &context,
                          const detail::scene_state &state,
                          const field_descriptor &field, const value &payload) {
  if (field.kind == value_kind::node) {
    const auto candidate = state.nodes.find(std::get<node_id>(payload).value);
    return candidate != state.nodes.end() &&
           accepts_node_type(context, field, candidate->second);
  }
  if (field.kind == value_kind::node_list) {
    return std::ranges::all_of(
        std::get<node_list>(payload), [&](const node_id id) {
          const auto candidate = state.nodes.find(id.value);
          return candidate != state.nodes.end() &&
                 accepts_node_type(context, field, candidate->second);
        });
  }
  return true;
}

bool retained_write_allowed(access_type access, bool node_is_being_created) {
  switch (access) {
  case access_type::initialize_only:
    return node_is_being_created;
  case access_type::input_only:
  case access_type::output_only:
    return false;
  case access_type::input_output:
    return true;
  }
  return false;
}

bool route_source_allowed(access_type access) {
  return access == access_type::output_only ||
         access == access_type::input_output;
}

bool route_sink_allowed(access_type access) {
  return access == access_type::input_only ||
         access == access_type::input_output;
}

} // namespace

struct scene_edit::impl {
  std::shared_ptr<detail::context_control> context;
  std::shared_ptr<const detail::scene_state> base;
  detail::scene_state staged;
  bool changed = false;
  std::optional<sai_error> poison;
  std::vector<change> changes;
  std::unordered_set<std::uint64_t> created_nodes;
};

struct event_batch::impl {
  std::shared_ptr<detail::context_control> context;
  std::shared_ptr<const detail::scene_state> base;
  event_time time;
  std::optional<sai_error> poison;
  std::vector<event_delivery> seeds;
  bool committed = false;
};

scene_edit::scene_edit(std::unique_ptr<impl> implementation)
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
  if (context && observer_id_ != 0) {
    std::lock_guard lock(context->mutex);
    context->observers.erase(observer_id_);
    context->event_observers.erase(observer_id_);
  }
  observer_id_ = 0;
  context_.reset();
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
  const node_id id{impl_->staged.next_node_id++};
  detail::scene_state::node_record record;
  record.type_name = type_name;
  for (const auto &field : type->fields) {
    record.fields.emplace(field.name, field.default_value);
  }
  impl_->staged.nodes.emplace(id.value, std::move(record));
  impl_->created_nodes.insert(id.value);
  impl_->changed = true;
  impl_->changes.push_back(change{.kind = change_kind::node_created,
                                  .node = id,
                                  .field = {},
                                  .before = std::monostate{},
                                  .after = type_name,
                                  .index = 0});
  return node{impl_->context, impl_->context->generation, id};
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
  impl_->staged.roots.push_back(child.id_);
  impl_->changed = true;
  impl_->changes.push_back(change{.kind = change_kind::root_inserted,
                                  .node = child.id_,
                                  .field = {},
                                  .before = std::monostate{},
                                  .after = child.id_,
                                  .index = impl_->staged.roots.size() - 1});
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
  impl_->changes.push_back(change{.kind = change_kind::root_removed,
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
  if (!descriptor->containment || descriptor->kind != value_kind::node_list) {
    impl_->poison =
        edit_error(error_code::type_mismatch, "scene_edit.append",
                   *impl_->context, impl_->base->revision,
                   "field is not a containment node list", parent.id_, field);
    return *impl_->poison;
  }
  if (!retained_write_allowed(descriptor->access, impl_->created_nodes.contains(
                                                      parent.id_.value))) {
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
  auto &field_value = parent_record->second.fields.at(field);
  const value before = field_value;
  auto &children = std::get<node_list>(field_value);
  children.push_back(child.id_);
  impl_->changed = true;
  impl_->changes.push_back(change{.kind = change_kind::field_changed,
                                  .node = parent.id_,
                                  .field = field,
                                  .before = before,
                                  .after = field_value,
                                  .index = children.size() - 1});
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

result<void> scene_edit::set(const dynamic_field &target, value new_value) {
  return set_value(target, std::move(new_value), false);
}

result<void> scene_edit::set(const dynamic_field &target,
                             const node &new_value) {
  if (impl_->poison)
    return *impl_->poison;
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
                                   bool node_authority_checked) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  auto found = impl_->staged.nodes.find(target.node_.value);
  if (target_context != impl_->context ||
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
  if (!retained_write_allowed(descriptor->access, impl_->created_nodes.contains(
                                                      target.node_.value))) {
    impl_->poison = edit_error(error_code::access_denied, "scene_edit.set",
                               *impl_->context, impl_->base->revision,
                               "field is not author-writable in this lifecycle "
                               "phase",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if ((descriptor->kind == value_kind::node ||
       descriptor->kind == value_kind::node_list) &&
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
  if (!value_is_well_formed(new_value)) {
    impl_->poison = edit_error(error_code::invalid_value, "scene_edit.set",
                               *impl_->context, impl_->base->revision,
                               "value violates field representation invariants",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if (node_authority_checked && descriptor->kind == value_kind::node) {
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
  }
  if (node_authority_checked && descriptor->kind == value_kind::node_list) {
    for (const auto id : std::get<node_list>(new_value)) {
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
    }
  }
  auto &stored = found->second.fields.at(target.name_);
  const value before = stored;
  stored = std::move(new_value);
  impl_->changed = true;
  impl_->changes.push_back(change{.kind = change_kind::field_changed,
                                  .node = target.node_,
                                  .field = target.name_,
                                  .before = before,
                                  .after = stored,
                                  .index = 0});
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
  impl_->changes.push_back(change{.kind = change_kind::name_defined,
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
  impl_->changes.push_back(change{.kind = change_kind::name_removed,
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
  impl_->changes.push_back(change{.kind = change_kind::export_added,
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
  impl_->changes.push_back(change{.kind = change_kind::export_removed,
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
  impl_->changes.push_back(change{.kind = change_kind::import_added,
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
  impl_->changes.push_back(change{.kind = change_kind::import_removed,
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
  impl_->changes.push_back(change{.kind = change_kind::route_added,
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
      change{.kind = change_kind::route_removed,
             .node = target.source,
             .field = target.source_field + " -> " + target.sink_field,
             .before = target.sink,
             .after = std::monostate{},
             .index = index});
  return {};
}

result<change_set> scene_edit::commit() {
  if (impl_->poison)
    return *impl_->poison;

  std::unordered_map<std::uint64_t, int> colors;
  std::function<bool(std::uint64_t)> visit = [&](std::uint64_t id) {
    if (colors[id] == 1)
      return false;
    if (colors[id] == 2)
      return true;
    colors[id] = 1;
    const auto &record = impl_->staged.nodes.at(id);
    const auto *type = impl_->context->registry.find(record.type_name);
    for (const auto &descriptor : type->fields) {
      if (!descriptor.containment)
        continue;
      const auto &field_value = record.fields.at(descriptor.name);
      if (descriptor.kind == value_kind::node) {
        const node_id child = std::get<node_id>(field_value);
        if (child.value != 0 &&
            (!impl_->staged.nodes.contains(child.value) || !visit(child.value)))
          return false;
      } else if (descriptor.kind == value_kind::node_list) {
        for (node_id child : std::get<node_list>(field_value)) {
          if (!impl_->staged.nodes.contains(child.value) || !visit(child.value))
            return false;
        }
      }
    }
    colors[id] = 2;
    return true;
  };
  for (const auto &[id, record] : impl_->staged.nodes) {
    (void)record;
    if (!visit(id)) {
      return edit_error(error_code::containment_cycle, "scene_edit.commit",
                        *impl_->context, impl_->base->revision,
                        "containment graph contains a cycle or dangling node");
    }
  }

  std::vector<std::shared_ptr<detail::context_control>> import_sources;
  import_sources.reserve(impl_->staged.import_sources.size());
  for (const auto &weak_source : impl_->staged.import_sources) {
    auto source = weak_source.lock();
    if (!source) {
      return edit_error(error_code::stale_aperture, "scene_edit.commit",
                        *impl_->context, impl_->base->revision,
                        "import source context no longer exists");
    }
    import_sources.push_back(std::move(source));
  }
  std::vector<std::shared_ptr<detail::context_control>> lock_order =
      import_sources;
  lock_order.push_back(impl_->context);
  std::sort(lock_order.begin(), lock_order.end(),
            [](const auto &left, const auto &right) {
              if (left->generation != right->generation)
                return left->generation < right->generation;
              return left.get() < right.get();
            });
  lock_order.erase(std::unique(lock_order.begin(), lock_order.end()),
                   lock_order.end());
  std::vector<std::unique_lock<std::mutex>> locks;
  locks.reserve(lock_order.size());
  for (const auto &context : lock_order)
    locks.emplace_back(context->mutex);

  if (!impl_->context->active) {
    return edit_error(error_code::stale_handle, "scene_edit.commit",
                      *impl_->context, impl_->base->revision,
                      "execution context is no longer active");
  }
  if (impl_->context->state->revision != impl_->base->revision) {
    return edit_error(error_code::stale_revision, "scene_edit.commit",
                      *impl_->context, impl_->base->revision,
                      "edit base revision is stale");
  }
  for (std::size_t index = 0; index < impl_->staged.imports.size(); ++index) {
    const import_binding &binding = impl_->staged.imports[index];
    const auto &source = import_sources[index];
    if (!source->active || source->generation != binding.source_generation) {
      return edit_error(error_code::stale_aperture, "scene_edit.commit",
                        *impl_->context, impl_->base->revision,
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
      return edit_error(error_code::stale_aperture, "scene_edit.commit",
                        *impl_->context, impl_->base->revision,
                        "source export changed before import publication",
                        binding.target.local);
    }
  }
  if (!impl_->changed) {
    return change_set{.before_revision = impl_->base->revision,
                      .after_revision = impl_->base->revision,
                      .changes = {}};
  }
  const revision_id before = impl_->base->revision;
  impl_->staged.revision = before + 1;
  auto published =
      std::make_shared<detail::scene_state>(std::move(impl_->staged));
  impl_->context->state = published;
  impl_->context->published_revision.store(published->revision,
                                           std::memory_order_release);
  change_set committed{.before_revision = before,
                       .after_revision = published->revision,
                       .changes = std::move(impl_->changes)};
  if (!impl_->context->observers.empty()) {
    std::vector<std::uint64_t> recipients;
    recipients.reserve(impl_->context->observers.size());
    for (const auto &[observer_id, callback] : impl_->context->observers) {
      (void)callback;
      recipients.push_back(observer_id);
    }
    impl_->context->pending.push_back(detail::context_control::pending_change{
        .changes = committed, .recipients = std::move(recipients)});
  }
  return committed;
}

result<void> event_batch::send(const dynamic_field &target, value payload) {
  return send_value(target, std::move(payload), false);
}

result<void> event_batch::send(const dynamic_field &target,
                               const node &payload) {
  if (impl_->poison)
    return *impl_->poison;
  const auto value_context = payload.context_.lock();
  if (value_context != impl_->context ||
      payload.generation_ != impl_->context->generation ||
      !impl_->base->nodes.contains(payload.id_.value)) {
    impl_->poison =
        edit_error(error_code::invalid_context, "event_batch.send",
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
    const auto value_context = candidate.context_.lock();
    if (value_context != impl_->context ||
        candidate.generation_ != impl_->context->generation ||
        !impl_->base->nodes.contains(candidate.id_.value)) {
      impl_->poison =
          edit_error(error_code::invalid_context, "event_batch.send",
                     *impl_->context, impl_->base->revision,
                     "node event range crosses a context", candidate.id_);
      return *impl_->poison;
    }
    ids.push_back(candidate.id_);
  }
  return send_value(target, value{std::move(ids)}, true);
}

result<void> event_batch::send_value(const dynamic_field &target, value payload,
                                     bool node_authority_checked) {
  if (impl_->poison)
    return *impl_->poison;
  const auto target_context = target.context_.lock();
  const auto found = impl_->base->nodes.find(target.node_.value);
  if (target_context != impl_->context ||
      target.generation_ != impl_->context->generation ||
      found == impl_->base->nodes.end()) {
    impl_->poison =
        edit_error(error_code::stale_handle, "event_batch.send",
                   *impl_->context, impl_->base->revision,
                   "invalid event destination", target.node_, target.name_);
    return *impl_->poison;
  }
  const auto *descriptor =
      find_descriptor(*impl_->context, found->second, target.name_);
  if (!descriptor) {
    impl_->poison =
        edit_error(error_code::unknown_field, "event_batch.send",
                   *impl_->context, impl_->base->revision,
                   "unknown event destination", target.node_, target.name_);
    return *impl_->poison;
  }
  if (descriptor->access != access_type::input_only &&
      descriptor->access != access_type::input_output) {
    impl_->poison = edit_error(error_code::access_denied, "event_batch.send",
                               *impl_->context, impl_->base->revision,
                               "field does not accept external event intent",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if ((descriptor->kind == value_kind::node ||
       descriptor->kind == value_kind::node_list) &&
      !node_authority_checked) {
    impl_->poison =
        edit_error(error_code::invalid_context, "event_batch.send",
                   *impl_->context, impl_->base->revision,
                   "node-valued events require context-bearing node handles",
                   target.node_, target.name_);
    return *impl_->poison;
  }
  if (kind_of(payload) != descriptor->kind) {
    impl_->poison = edit_error(error_code::type_mismatch, "event_batch.send",
                               *impl_->context, impl_->base->revision,
                               "event payload does not match field descriptor",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if (!value_is_well_formed(payload)) {
    impl_->poison = edit_error(error_code::invalid_value, "event_batch.send",
                               *impl_->context, impl_->base->revision,
                               "event violates field representation invariants",
                               target.node_, target.name_);
    return *impl_->poison;
  }
  if (node_authority_checked && descriptor->kind == value_kind::node) {
    const auto id = std::get<node_id>(payload);
    const auto candidate = impl_->base->nodes.find(id.value);
    if (candidate == impl_->base->nodes.end() ||
        !accepts_node_type(*impl_->context, *descriptor, candidate->second)) {
      impl_->poison =
          edit_error(error_code::type_mismatch, "event_batch.send",
                     *impl_->context, impl_->base->revision,
                     "node event type is not accepted by the field descriptor",
                     target.node_, target.name_);
      return *impl_->poison;
    }
  }
  if (node_authority_checked && descriptor->kind == value_kind::node_list) {
    for (const auto id : std::get<node_list>(payload)) {
      const auto candidate = impl_->base->nodes.find(id.value);
      if (candidate == impl_->base->nodes.end() ||
          !accepts_node_type(*impl_->context, *descriptor, candidate->second)) {
        impl_->poison = edit_error(
            error_code::type_mismatch, "event_batch.send", *impl_->context,
            impl_->base->revision,
            "node event range contains a type not accepted by the field "
            "descriptor",
            target.node_, target.name_);
        return *impl_->poison;
      }
    }
  }
  const auto duplicate = std::find_if(impl_->seeds.begin(), impl_->seeds.end(),
                                      [&](const event_delivery &seed) {
                                        return seed.target == target.node_ &&
                                               seed.field == target.name_;
                                      });
  if (duplicate != impl_->seeds.end()) {
    impl_->poison =
        edit_error(error_code::ambiguous_event_seed, "event_batch.send",
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
  if (impl_->poison)
    return *impl_->poison;
  if (impl_->committed) {
    return edit_error(error_code::poisoned_edit, "event_batch.commit",
                      *impl_->context, impl_->base->revision,
                      "event batch was already committed");
  }
  impl_->committed = true;
  std::lock_guard lock(impl_->context->mutex);
  if (!impl_->context->active) {
    return edit_error(error_code::stale_handle, "event_batch.commit",
                      *impl_->context, impl_->base->revision,
                      "execution context is no longer active");
  }
  if (impl_->context->state->revision != impl_->base->revision) {
    return edit_error(error_code::stale_revision, "event_batch.commit",
                      *impl_->context, impl_->base->revision,
                      "event batch base revision is stale");
  }
  if (!impl_->seeds.empty() && impl_->context->last_event_time &&
      !(impl_->context->last_event_time->seconds < impl_->time.seconds)) {
    return edit_error(error_code::event_time_regression, "event_batch.commit",
                      *impl_->context, impl_->base->revision,
                      "event time must strictly advance");
  }
  detail::scene_state staged = *impl_->base;
  std::vector<event_delivery> deliveries = std::move(impl_->seeds);
  std::vector<bool> route_fired(staged.routes.size(), false);
  for (std::size_t delivery_index = 0; delivery_index < deliveries.size();
       ++delivery_index) {
    const node_id source_target = deliveries[delivery_index].target;
    const std::string source_field = deliveries[delivery_index].field;
    const value source_payload = deliveries[delivery_index].payload;
    for (std::size_t route_index = 0; route_index < staged.routes.size();
         ++route_index) {
      if (route_fired[route_index])
        continue;
      const route &candidate = staged.routes[route_index];
      if (candidate.source != source_target ||
          candidate.source_field != source_field)
        continue;
      const auto sink = staged.nodes.find(candidate.sink.value);
      const auto *sink_descriptor =
          sink == staged.nodes.end()
              ? nullptr
              : find_descriptor(*impl_->context, sink->second,
                                candidate.sink_field);
      if (!sink_descriptor ||
          !accepts_node_payload(*impl_->context, staged, *sink_descriptor,
                                source_payload)) {
        return edit_error(
            error_code::type_mismatch, "event_batch.commit", *impl_->context,
            impl_->base->revision,
            "routed node event type is not accepted by the destination field "
            "descriptor",
            candidate.sink, candidate.sink_field);
      }
      route_fired[route_index] = true;
      deliveries.push_back(event_delivery{.time = impl_->time,
                                          .target = candidate.sink,
                                          .field = candidate.sink_field,
                                          .payload = source_payload,
                                          .cause = delivery_index,
                                          .route_index = route_index});
    }
  }
  std::vector<event_issue> portability_issues;
  std::map<std::pair<std::uint64_t, std::string>, std::size_t> first_delivery;
  std::map<std::pair<std::uint64_t, std::string>, std::size_t> issue_by_target;
  for (std::size_t index = 0; index < deliveries.size(); ++index) {
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
  std::vector<change> changes;
  for (const event_delivery &delivery : deliveries) {
    auto found = staged.nodes.find(delivery.target.value);
    const auto *descriptor =
        find_descriptor(*impl_->context, found->second, delivery.field);
    if (descriptor->access != access_type::input_output)
      continue;
    auto &stored = found->second.fields.at(delivery.field);
    if (same_representation(stored, delivery.payload))
      continue;
    const value before = stored;
    stored = delivery.payload;
    changes.push_back(change{.kind = change_kind::field_changed,
                             .node = delivery.target,
                             .field = delivery.field,
                             .before = before,
                             .after = stored,
                             .index = 0});
  }

  event_result result{.time = impl_->time,
                      .before_revision = impl_->base->revision,
                      .after_revision = impl_->base->revision,
                      .deliveries = std::move(deliveries),
                      .state_changes = std::nullopt,
                      .portability_issues = std::move(portability_issues)};
  if (!result.deliveries.empty()) {
    impl_->context->last_event_time = impl_->time;
    for (const event_delivery &delivery : result.deliveries) {
      std::vector<std::uint64_t> recipients;
      for (const auto &[observer_id, observer] :
           impl_->context->event_observers) {
        if (observer.target == delivery.target &&
            observer.field == delivery.field)
          recipients.push_back(observer_id);
      }
      if (!recipients.empty()) {
        impl_->context->pending.push_back(
            detail::context_control::pending_event{
                .delivery = delivery, .recipients = std::move(recipients)});
      }
    }
  }
  if (changes.empty())
    return result;

  staged.revision = impl_->base->revision + 1;
  auto published = std::make_shared<detail::scene_state>(std::move(staged));
  impl_->context->state = published;
  impl_->context->published_revision.store(published->revision,
                                           std::memory_order_release);
  change_set committed{.before_revision = impl_->base->revision,
                       .after_revision = published->revision,
                       .changes = std::move(changes)};
  result.after_revision = published->revision;
  result.state_changes = committed;
  if (!impl_->context->observers.empty()) {
    std::vector<std::uint64_t> recipients;
    recipients.reserve(impl_->context->observers.size());
    for (const auto &[observer_id, callback] : impl_->context->observers) {
      (void)callback;
      recipients.push_back(observer_id);
    }
    impl_->context->pending.push_back(detail::context_control::pending_change{
        .changes = committed, .recipients = std::move(recipients)});
  }
  return result;
}

scene_snapshot::scene_snapshot(std::shared_ptr<detail::context_control> context,
                               std::shared_ptr<const detail::scene_state> state)
    : context_(std::move(context)), state_(std::move(state)) {
  import_states_.reserve(state_->imports.size());
  for (std::size_t index = 0; index < state_->imports.size(); ++index) {
    const auto source = state_->import_sources[index].lock();
    if (!source) {
      import_states_.push_back({});
      continue;
    }
    std::lock_guard lock(source->mutex);
    if (!source->active ||
        source->generation != state_->imports[index].source_generation) {
      import_states_.push_back({});
      continue;
    }
    import_states_.push_back(source->state);
  }
}

revision_id scene_snapshot::revision() const noexcept {
  return state_->revision;
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
          if (descriptor.kind == value_kind::node) {
            const node_id child = std::get<node_id>(field_value);
            if (child.value != 0) {
              auto child_path = path;
              child_path.push_back(0);
              walk(child, occurrence_index, descriptor.name, 0,
                   std::move(child_path));
            }
          } else if (descriptor.kind == value_kind::node_list) {
            const auto &children = std::get<node_list>(field_value);
            for (std::size_t child_index = 0; child_index < children.size();
                 ++child_index) {
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
    return described.error();
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

result<value> scene_snapshot::read(const dynamic_field &source) const {
  const auto source_context = source.context_.lock();
  const auto found = state_->nodes.find(source.node_.value);
  if (source_context != context_ ||
      source.generation_ != context_->generation ||
      found == state_->nodes.end()) {
    return edit_error(error_code::stale_handle, "scene_snapshot.read",
                      *context_, state_->revision, "invalid field handle",
                      source.node_, source.name_);
  }
  const auto *descriptor =
      find_descriptor(*context_, found->second, source.name_);
  if (!descriptor) {
    return edit_error(error_code::unknown_field, "scene_snapshot.read",
                      *context_, state_->revision,
                      "unknown field: " + source.name_, source.node_,
                      source.name_);
  }
  if (descriptor->access == access_type::input_only) {
    return edit_error(error_code::access_denied, "scene_snapshot.read",
                      *context_, state_->revision,
                      "inputOnly field is not readable", source.node_,
                      source.name_);
  }
  return found->second.fields.at(source.name_);
}

result<value> scene_snapshot::read(const dynamic_imported_field &field) const {
  const auto importer = field.importer_.lock();
  const auto source = field.source_.lock();
  if (importer != context_ ||
      field.importer_generation_ != context_->generation) {
    return edit_error(error_code::invalid_context, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "imported field belongs to another snapshot context",
                      field.node_, field.name_);
  }
  if (!source || source->generation != field.source_generation_) {
    return edit_error(error_code::stale_handle, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "invalid imported field handle", field.node_,
                      field.name_);
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
    return edit_error(error_code::invalid_context, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "imported field does not belong to this snapshot",
                      field.node_, field.name_);
  }
  const auto &source_state = import_states_[*binding_index];
  if (!source_state || source_state->revision != field.source_revision_) {
    return edit_error(error_code::stale_handle, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "imported field source revision is unavailable",
                      field.node_, field.name_);
  }
  const auto found = source_state->nodes.find(field.node_.value);
  if (found == source_state->nodes.end()) {
    return edit_error(error_code::unknown_node, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "imported field owner is absent", field.node_,
                      field.name_);
  }
  const auto *descriptor = find_descriptor(*source, found->second, field.name_);
  if (!descriptor || descriptor->kind != field.kind_ ||
      descriptor->access != field.access_) {
    return edit_error(error_code::unknown_field, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "imported field descriptor changed", field.node_,
                      field.name_);
  }
  if (descriptor->access == access_type::input_only) {
    return edit_error(error_code::access_denied, "scene_snapshot.read_import",
                      *context_, state_->revision,
                      "inputOnly imported field is not readable", field.node_,
                      field.name_);
  }
  return found->second.fields.at(field.name_);
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
  auto implementation = std::make_unique<scene_edit::impl>();
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
  std::lock_guard lock(control_->mutex);
  if (!control_->active)
    return {};
  const std::uint64_t observer_id = control_->next_observer_id++;
  control_->observers.emplace(
      observer_id, std::make_shared<const observer>(std::move(callback)));
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
  std::lock_guard lock(control_->mutex);
  if (!control_->active) {
    return edit_error(
        error_code::stale_handle, "execution_context.observe_event", *control_,
        control_->state->revision, "execution context is no longer active",
        source.node_, source.name_);
  }
  if (source.context_.lock() != control_ ||
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
                       .callback = std::make_shared<const event_observer>(
                           std::move(callback))});
  return subscription{control_, observer_id};
}

dispatch_report execution_context::drain() const {
  dispatch_report report;
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

result<void> browser::replace_world(const execution_context &replacement) {
  std::lock_guard browser_lock(control_->mutex);
  if (!replacement.control_) {
    sai_error error;
    error.code = error_code::stale_handle;
    error.operation = "browser.replace_world";
    error.message = "replacement context is empty";
    return error;
  }
  if (replacement.control_->owner.lock() != control_) {
    sai_error error;
    error.code = error_code::invalid_context;
    error.operation = "browser.replace_world";
    error.message = "replacement context belongs to another browser";
    return error;
  }
  if (replacement.control_ == control_->current)
    return {};
  {
    std::lock_guard replacement_lock(replacement.control_->mutex);
    if (!replacement.control_->active) {
      return edit_error(error_code::stale_handle, "browser.replace_world",
                        *replacement.control_,
                        replacement.control_->state->revision,
                        "replacement context is stale");
    }
  }
  {
    std::lock_guard old_lock(control_->current->mutex);
    control_->current->active = false;
    control_->current->observers.clear();
    control_->current->event_observers.clear();
    control_->current->pending.clear();
  }
  control_->current = replacement.control_;
  ++control_->world_epoch;
  return {};
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
    return error;
  }
  std::lock_guard lock(control_->mutex);
  const auto found = control_->requests.find(ticket.request_id_);
  if (found == control_->requests.end() ||
      found->second.world_epoch != control_->world_epoch) {
    sai_error error;
    error.code = error_code::stale_completion;
    error.operation = "browser.cancel";
    error.message = "load ticket is stale";
    return error;
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
    return error;
  }
  {
    std::lock_guard lock(control_->mutex);
    const auto found = control_->requests.find(ticket.request_id_);
    if (found == control_->requests.end()) {
      sai_error error;
      error.code = error_code::stale_completion;
      error.operation = "browser.complete_load";
      error.message = "load ticket is unknown or already completed";
      return error;
    }
    if (found->second.cancelled) {
      sai_error error;
      error.code = error_code::cancelled;
      error.operation = "browser.complete_load";
      error.message = "load request was cancelled";
      return error;
    }
    if (found->second.world_epoch != control_->world_epoch ||
        ticket.world_epoch_ != control_->world_epoch) {
      sai_error error;
      error.code = error_code::stale_completion;
      error.operation = "browser.complete_load";
      error.message = "browser world changed after the load began";
      return error;
    }
    control_->requests.erase(found);
  }
  return replace_world(replacement);
}

} // namespace x3d::sai::experimental
