#ifndef X3D_SAI_EXPERIMENTAL_KERNEL_HPP
#define X3D_SAI_EXPERIMENTAL_KERNEL_HPP

#include "types.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace x3d::sai::experimental {

namespace detail {
struct browser_control;
struct context_control;
struct scene_state;
} // namespace detail

class execution_context;

class load_ticket {
public:
  std::uint64_t request_id() const noexcept { return request_id_; }
  std::uint64_t world_epoch() const noexcept { return world_epoch_; }

private:
  load_ticket(std::weak_ptr<detail::browser_control> browser,
              std::uint64_t request_id, std::uint64_t world_epoch)
      : browser_(std::move(browser)), request_id_(request_id),
        world_epoch_(world_epoch) {}

  std::weak_ptr<detail::browser_control> browser_;
  std::uint64_t request_id_ = 0;
  std::uint64_t world_epoch_ = 0;
  friend class browser;
};

struct capability_set {
  bool authoring = true;
  bool inspection = true;
  bool live = false;
  bool async_loading = false;
  bool rendering = false;
};

class node {
public:
  node_id id() const noexcept { return id_; }
  generation_id generation() const noexcept { return generation_; }

private:
  node(std::weak_ptr<detail::context_control> context, generation_id generation,
       node_id id)
      : context_(std::move(context)), generation_(generation), id_(id) {}

  std::weak_ptr<detail::context_control> context_;
  generation_id generation_ = 0;
  node_id id_;
  friend class scene_edit;
  friend class scene_snapshot;
  friend class event_batch;
};

struct semantic_node_id {
  generation_id generation = 0;
  node_id local;
  friend bool operator==(semantic_node_id, semantic_node_id) = default;
};

class imported_node {
public:
  semantic_node_id identity() const noexcept {
    return semantic_node_id{source_generation_, id_};
  }
  generation_id importer_generation() const noexcept {
    return importer_generation_;
  }
  const std::string &local_name() const noexcept { return local_name_; }

private:
  imported_node(std::weak_ptr<detail::context_control> importer,
                generation_id importer_generation,
                std::weak_ptr<detail::context_control> source,
                generation_id source_generation, node_id id,
                std::string local_name)
      : importer_(std::move(importer)),
        importer_generation_(importer_generation), source_(std::move(source)),
        source_generation_(source_generation), id_(id),
        local_name_(std::move(local_name)) {}

  std::weak_ptr<detail::context_control> importer_;
  generation_id importer_generation_ = 0;
  std::weak_ptr<detail::context_control> source_;
  generation_id source_generation_ = 0;
  node_id id_;
  std::string local_name_;
  friend class scene_snapshot;
};

template <class T> class field;

class dynamic_field {
public:
  node_id node() const noexcept { return node_; }
  const std::string &name() const noexcept { return name_; }
  value_kind kind() const noexcept { return kind_; }
  access_type access() const noexcept { return access_; }

  template <class T> result<field<T>> as() const;

private:
  dynamic_field(std::weak_ptr<detail::context_control> context,
                generation_id generation, node_id node, std::string name,
                value_kind kind, access_type access)
      : context_(std::move(context)), generation_(generation), node_(node),
        name_(std::move(name)), kind_(kind), access_(access) {}

  std::weak_ptr<detail::context_control> context_;
  generation_id generation_ = 0;
  node_id node_;
  std::string name_;
  value_kind kind_ = value_kind::string;
  access_type access_ = access_type::input_output;
  friend class scene_edit;
  friend class scene_snapshot;
  friend class event_batch;
  friend class execution_context;
  template <class T> friend class field;
};

template <class T> class field {
public:
  node_id node() const noexcept { return dynamic_.node(); }
  const std::string &name() const noexcept { return dynamic_.name(); }

private:
  explicit field(dynamic_field dynamic) : dynamic_(std::move(dynamic)) {}
  dynamic_field dynamic_;
  friend class dynamic_field;
  friend class scene_edit;
  friend class scene_snapshot;
  friend class event_batch;
};

template <class T> result<field<T>> dynamic_field::as() const {
  if (kind_ == value_traits<T>::kind)
    return field<T>{*this};
  sai_error error;
  error.code = error_code::type_mismatch;
  error.operation = "dynamic_field.as";
  error.message = "requested typed view does not match the field descriptor";
  error.generation = generation_;
  error.node = node_;
  error.field = name_;
  return error;
}

struct occurrence {
  node_id node;
  std::optional<std::size_t> parent_occurrence;
  std::string container_field;
  std::size_t index = 0;
  std::vector<std::size_t> path;
};

enum class change_kind {
  node_created,
  field_changed,
  root_inserted,
  root_removed,
  name_defined,
  name_removed,
  export_added,
  export_removed,
  import_added,
  import_removed,
  route_added,
  route_removed,
};

struct change {
  change_kind kind = change_kind::field_changed;
  node_id node;
  std::string field;
  value before;
  value after;
  std::size_t index = 0;
};

struct change_set {
  revision_id before_revision = 0;
  revision_id after_revision = 0;
  std::vector<change> changes;
};

struct event_delivery {
  event_time time;
  node_id target;
  std::string field;
  value payload;
  std::optional<std::size_t> cause;
  std::optional<std::size_t> route_index;
};

enum class event_issue_code { nonportable_fan_in };

struct event_issue {
  event_issue_code code = event_issue_code::nonportable_fan_in;
  node_id target;
  std::string field;
  std::vector<std::size_t> delivery_indices;
};

struct event_result {
  event_time time;
  revision_id before_revision = 0;
  revision_id after_revision = 0;
  std::vector<event_delivery> deliveries;
  std::optional<change_set> state_changes;
  std::vector<event_issue> portability_issues;
};

struct route {
  node_id source;
  std::string source_field;
  node_id sink;
  std::string sink_field;
  friend bool operator==(const route &, const route &) = default;
};

struct name_binding {
  std::string name;
  node_id node;
  friend bool operator==(const name_binding &, const name_binding &) = default;
};

struct export_binding {
  std::string name;
  node_id node;
  friend bool operator==(const export_binding &,
                         const export_binding &) = default;
};

struct import_binding {
  std::string local_name;
  generation_id source_generation = 0;
  std::string exported_name;
  semantic_node_id target;
  friend bool operator==(const import_binding &,
                         const import_binding &) = default;
};

class subscription {
public:
  subscription() = default;
  subscription(subscription &&other) noexcept;
  subscription &operator=(subscription &&other) noexcept;
  ~subscription();
  subscription(const subscription &) = delete;
  subscription &operator=(const subscription &) = delete;

  bool active() const noexcept;
  void cancel() noexcept;

private:
  subscription(std::weak_ptr<detail::context_control> context,
               std::uint64_t observer_id)
      : context_(std::move(context)), observer_id_(observer_id) {}

  std::weak_ptr<detail::context_control> context_;
  std::uint64_t observer_id_ = 0;
  friend class execution_context;
};

struct dispatch_report {
  std::size_t delivered = 0;
  std::vector<sai_error> errors;
};

class scene_edit {
public:
  scene_edit(scene_edit &&) noexcept;
  scene_edit &operator=(scene_edit &&) noexcept;
  ~scene_edit();
  scene_edit(const scene_edit &) = delete;
  scene_edit &operator=(const scene_edit &) = delete;

  result<node> create_node(const std::string &type_name);
  result<void> append_root(const node &child);
  result<void> remove_root(std::size_t index);
  result<void> append(const node &parent, const std::string &field,
                      const node &child);
  result<dynamic_field> field(const node &owner, const std::string &name) const;
  result<void> set(const dynamic_field &target, value new_value);
  result<void> set(const dynamic_field &target, const node &new_value);
  result<void> set(const dynamic_field &target,
                   std::span<const node> new_value);
  template <class T>
    requires(!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<void> set(const experimental::field<T> &target, T new_value) {
    return set(target.dynamic_, value{std::move(new_value)});
  }
  result<void> set(const experimental::field<node_id> &target,
                   const node &new_value) {
    return set(target.dynamic_, new_value);
  }
  result<void> set(const experimental::field<node_list> &target,
                   std::span<const node> new_value) {
    return set(target.dynamic_, new_value);
  }
  result<void> define_name(const std::string &name, const node &target);
  result<void> undefine_name(const std::string &name);
  result<void> export_node(const std::string &name, const node &target);
  result<void> remove_export(const export_binding &target);
  result<void> import_node(const std::string &local_name,
                           const execution_context &source,
                           const std::string &exported_name);
  result<void> remove_import(const import_binding &target);
  result<void> add_route(const dynamic_field &source,
                         const dynamic_field &sink);
  result<void> remove_route(const route &target);
  result<change_set> commit();

private:
  result<void> set_value(const dynamic_field &target, value new_value,
                         bool node_authority_checked);
  struct impl;
  explicit scene_edit(std::unique_ptr<impl> implementation);
  std::unique_ptr<impl> impl_;
  friend class execution_context;
};

class scene_snapshot {
public:
  revision_id revision() const noexcept;
  const std::vector<node_id> &roots() const noexcept;
  std::vector<occurrence> occurrences() const;
  result<node> lookup(node_id id) const;
  result<node_type_descriptor> describe(const node &owner) const;
  result<dynamic_field> field(const node &owner, const std::string &name) const;
  result<value> read(const dynamic_field &source) const;
  template <class T>
  result<T> read(const experimental::field<T> &source) const {
    auto dynamic = read(source.dynamic_);
    if (!dynamic)
      return dynamic.error();
    return std::get<T>(std::move(dynamic).value());
  }
  result<node> named(const std::string &name) const;
  const std::vector<name_binding> &names() const noexcept;
  result<node> exported(const std::string &name) const;
  const std::vector<export_binding> &exports() const noexcept;
  result<imported_node> imported(const std::string &local_name) const;
  const std::vector<import_binding> &imports() const noexcept;
  const std::vector<route> &routes() const noexcept;

private:
  scene_snapshot(std::shared_ptr<detail::context_control> context,
                 std::shared_ptr<const detail::scene_state> state);

  std::shared_ptr<detail::context_control> context_;
  std::shared_ptr<const detail::scene_state> state_;
  friend class execution_context;
};

class event_batch {
public:
  event_batch(event_batch &&) noexcept;
  event_batch &operator=(event_batch &&) noexcept;
  ~event_batch();
  event_batch(const event_batch &) = delete;
  event_batch &operator=(const event_batch &) = delete;

  result<void> send(const dynamic_field &target, value payload);
  result<void> send(const dynamic_field &target, const node &payload);
  result<void> send(const dynamic_field &target, std::span<const node> payload);
  template <class T>
    requires(!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<void> send(const experimental::field<T> &target, T payload) {
    return send(target.dynamic_, value{std::move(payload)});
  }
  result<void> send(const experimental::field<node_id> &target,
                    const node &payload) {
    return send(target.dynamic_, payload);
  }
  result<void> send(const experimental::field<node_list> &target,
                    std::span<const node> payload) {
    return send(target.dynamic_, payload);
  }
  result<event_result> commit();

private:
  result<void> send_value(const dynamic_field &target, value payload,
                          bool node_authority_checked);
  struct impl;
  explicit event_batch(std::unique_ptr<impl> implementation);
  std::unique_ptr<impl> impl_;
  friend class execution_context;
};

class execution_context {
public:
  using observer = std::function<void(const change_set &)>;
  using event_observer = std::function<void(const event_delivery &)>;

  generation_id generation() const noexcept;
  scene_snapshot snapshot() const;
  scene_edit edit() const;
  event_batch events(event_time time) const;
  subscription observe(observer callback) const;
  result<subscription> observe(const dynamic_field &source,
                               event_observer callback) const;
  dispatch_report drain() const;

private:
  explicit execution_context(std::shared_ptr<detail::context_control> control);
  std::shared_ptr<detail::context_control> control_;
  friend class browser;
  friend class scene_edit;
};

class browser {
public:
  explicit browser(type_registry registry);

  capability_set capabilities() const noexcept;
  execution_context current_scene() const;
  execution_context create_scene();
  result<void> replace_world(const execution_context &replacement);
  load_ticket begin_load();
  result<void> cancel(const load_ticket &ticket);
  result<void> complete_load(const load_ticket &ticket,
                             const execution_context &replacement);

private:
  std::shared_ptr<detail::browser_control> control_;
};

} // namespace x3d::sai::experimental

#endif
