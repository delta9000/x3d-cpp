#include "x3d/sai/experimental/kernel.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
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

bool value_matches(value_kind kind, const value &candidate) {
  switch (kind) {
  case value_kind::boolean:
    return std::holds_alternative<bool>(candidate);
  case value_kind::int32:
    return std::holds_alternative<std::int32_t>(candidate);
  case value_kind::number:
    return std::holds_alternative<double>(candidate);
  case value_kind::string:
    return std::holds_alternative<std::string>(candidate);
  case value_kind::vec3f:
    return std::holds_alternative<vec3f>(candidate);
  case value_kind::node:
    return std::holds_alternative<node_id>(candidate);
  case value_kind::node_list:
    return std::holds_alternative<node_list>(candidate);
  }
  return false;
}

} // namespace

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

value_kind kind_of(const value &field_value) {
  switch (field_value.index()) {
  case 1:
    return value_kind::boolean;
  case 2:
    return value_kind::int32;
  case 3:
    return value_kind::number;
  case 4:
    return value_kind::string;
  case 5:
    return value_kind::vec3f;
  case 6:
    return value_kind::node;
  case 7:
    return value_kind::node_list;
  default:
    return value_kind::string;
  }
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
  if (existing != impl_->staged.names.end()) {
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

  std::lock_guard lock(impl_->context->mutex);
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
    if (inserted ||
        deliveries[found->second].payload == deliveries[index].payload)
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
    if (stored == delivery.payload)
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
    : context_(std::move(context)), state_(std::move(state)) {}

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
  std::lock_guard lock(control_->mutex);
  return scene_snapshot{control_, control_->state};
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
