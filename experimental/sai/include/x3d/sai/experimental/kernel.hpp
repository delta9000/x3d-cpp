#ifndef X3D_SAI_EXPERIMENTAL_KERNEL_HPP
#define X3D_SAI_EXPERIMENTAL_KERNEL_HPP

#include "types.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::sai::experimental {

namespace detail {
struct browser_control;
struct context_control;
struct scene_edit_control;
struct scene_state;
} // namespace detail

class execution_context;
template <class Tag> class typed_node;

template <class Owner, class T> class field_key {
public:
  constexpr std::string_view name() const noexcept { return name_; }
  static constexpr value_kind kind = value_traits<T>::kind;
  constexpr access_type access() const noexcept { return access_; }

private:
  constexpr field_key(std::string_view name, access_type access)
      : name_(name), access_(access) {}

  std::string_view name_;
  access_type access_;
  friend Owner;
  template <class> friend class typed_node;
};

struct field_key_descriptor {
  std::string_view name;
  value_kind kind = value_kind::sf_string;
  access_type access = access_type::input_output;
};

struct node_key_descriptor {
  std::string_view name;
  std::string_view schema_fingerprint;
  std::span<const field_key_descriptor> fields;
};

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
  bool disposed() const noexcept { return disposed_; }
  void dispose() noexcept {
    context_.reset();
    disposed_ = true;
  }

private:
  node(std::weak_ptr<detail::context_control> context, generation_id generation,
       node_id id)
      : context_(std::move(context)), generation_(generation), id_(id) {}

  std::weak_ptr<detail::context_control> context_;
  generation_id generation_ = 0;
  node_id id_;
  bool disposed_ = false;
  friend class scene_edit;
  friend class dynamic_multi_field_edit;
  friend class scene_snapshot;
  friend class event_batch;
  template <class> friend class typed_node;
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
  revision_id source_revision() const noexcept { return source_revision_; }
  const std::string &local_name() const noexcept { return local_name_; }

private:
  imported_node(std::weak_ptr<detail::context_control> importer,
                generation_id importer_generation,
                std::weak_ptr<detail::context_control> source,
                generation_id source_generation, revision_id source_revision,
                node_id id, std::string local_name)
      : importer_(std::move(importer)),
        importer_generation_(importer_generation), source_(std::move(source)),
        source_generation_(source_generation),
        source_revision_(source_revision), id_(id),
        local_name_(std::move(local_name)) {}

  std::weak_ptr<detail::context_control> importer_;
  generation_id importer_generation_ = 0;
  std::weak_ptr<detail::context_control> source_;
  generation_id source_generation_ = 0;
  revision_id source_revision_ = 0;
  node_id id_;
  std::string local_name_;
  friend class scene_snapshot;
};

template <class T> class imported_field;

class dynamic_imported_field {
public:
  semantic_node_id node() const noexcept {
    return semantic_node_id{source_generation_, node_};
  }
  const std::string &name() const noexcept { return name_; }
  value_kind kind() const noexcept { return kind_; }
  access_type access() const noexcept { return access_; }
  revision_id source_revision() const noexcept { return source_revision_; }

  template <class T> result<imported_field<T>> as() const;

private:
  dynamic_imported_field(std::weak_ptr<detail::context_control> importer,
                         generation_id importer_generation,
                         std::weak_ptr<detail::context_control> source,
                         generation_id source_generation,
                         revision_id source_revision, node_id node,
                         std::string name, value_kind kind, access_type access)
      : importer_(std::move(importer)),
        importer_generation_(importer_generation), source_(std::move(source)),
        source_generation_(source_generation),
        source_revision_(source_revision), node_(node), name_(std::move(name)),
        kind_(kind), access_(access) {}

  std::weak_ptr<detail::context_control> importer_;
  generation_id importer_generation_ = 0;
  std::weak_ptr<detail::context_control> source_;
  generation_id source_generation_ = 0;
  revision_id source_revision_ = 0;
  node_id node_;
  std::string name_;
  value_kind kind_ = value_kind::sf_string;
  access_type access_ = access_type::input_output;
  friend class scene_snapshot;
  template <class T> friend class imported_field;
};

template <class T> class imported_field {
public:
  semantic_node_id node() const noexcept { return dynamic_.node(); }
  const std::string &name() const noexcept { return dynamic_.name(); }

private:
  explicit imported_field(dynamic_imported_field dynamic)
      : dynamic_(std::move(dynamic)) {}
  dynamic_imported_field dynamic_;
  friend class dynamic_imported_field;
  friend class scene_snapshot;
};

template <class T>
result<imported_field<T>> dynamic_imported_field::as() const {
  if (kind_ == value_traits<T>::kind)
    return imported_field<T>{*this};
  sai_error error;
  error.code = error_code::type_mismatch;
  error.operation = "dynamic_imported_field.as";
  error.message = "requested typed view does not match the field descriptor";
  error.generation = source_generation_;
  error.node = node_;
  error.field = name_;
  return failure(std::move(error));
}

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
  value_kind kind_ = value_kind::sf_string;
  access_type access_ = access_type::input_output;
  friend class scene_edit;
  friend class dynamic_multi_field_edit;
  friend class scene_snapshot;
  friend class event_batch;
  friend class execution_context;
  template <class T> friend class field;
  template <class> friend class typed_node;
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
  template <class Tag> friend class typed_node;
};

template <class Tag> class typed_node {
public:
  node_id id() const noexcept { return dynamic_.id(); }
  const node &dynamic() const noexcept { return dynamic_; }
  bool disposed() const noexcept { return dynamic_.disposed(); }
  void dispose() noexcept { dynamic_.dispose(); }

  template <class T>
  experimental::field<T> field(const field_key<Tag, T> &key) const noexcept {
    return experimental::field<T>{dynamic_field{
        dynamic_.context_, dynamic_.generation_, dynamic_.id_,
        std::string{key.name_}, value_traits<T>::kind, key.access_}};
  }

private:
  explicit typed_node(node dynamic) : dynamic_(std::move(dynamic)) {}

  node dynamic_;
  friend class scene_edit;
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
  return failure(std::move(error));
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
  node_removed,
  unit_declared,
  field_changed,
  multi_element_set,
  multi_inserted,
  multi_erased,
  multi_cleared,
  multi_replaced,
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

class dynamic_multi_field_edit {
public:
  result<std::size_t> size() const;
  result<value> at(std::size_t index) const;
  result<node> node_at(std::size_t index) const;
  result<void> set(std::size_t index, value element);
  result<void> set(std::size_t index, const node &element);
  result<void> insert(std::size_t index, value element);
  result<void> insert(std::size_t index, const node &element);
  result<void> erase(std::size_t index);
  result<void> append(value element);
  result<void> append(const node &element);
  result<void> clear();
  result<void> replace(value sequence);
  result<void> replace(std::span<const node> sequence);

private:
  dynamic_multi_field_edit(std::weak_ptr<detail::scene_edit_control> edit,
                           dynamic_field target)
      : edit_(std::move(edit)), target_(std::move(target)) {}

  std::weak_ptr<detail::scene_edit_control> edit_;
  dynamic_field target_;
  friend class scene_edit;
};

namespace detail {
template <class Sequence> struct multi_sequence_traits;
template <class Element, class Allocator>
struct multi_sequence_traits<std::vector<Element, Allocator>> {
  using element_type = Element;
};
template <> struct multi_sequence_traits<bool_list> {
  using element_type = bool;
};
} // namespace detail

template <class Sequence>
concept multi_field_sequence = requires {
  typename detail::multi_sequence_traits<Sequence>::element_type;
  value_traits<Sequence>::kind;
};

template <multi_field_sequence Sequence> class multi_field_edit {
public:
  using element_type =
      typename detail::multi_sequence_traits<Sequence>::element_type;

  result<std::size_t> size() const { return dynamic_.size(); }
  result<element_type> at(std::size_t index) const {
    auto element = dynamic_.at(index);
    if (!element)
      return failure(element.error());
    return std::get<element_type>(std::move(element).value());
  }
  result<void> set(std::size_t index, element_type element) {
    return dynamic_.set(index, value{std::move(element)});
  }
  result<void> insert(std::size_t index, element_type element) {
    return dynamic_.insert(index, value{std::move(element)});
  }
  result<void> erase(std::size_t index) { return dynamic_.erase(index); }
  result<void> append(element_type element) {
    return dynamic_.append(value{std::move(element)});
  }
  result<void> clear() { return dynamic_.clear(); }
  result<void> replace(Sequence sequence) {
    return dynamic_.replace(value{std::move(sequence)});
  }

private:
  explicit multi_field_edit(dynamic_multi_field_edit dynamic)
      : dynamic_(std::move(dynamic)) {}
  dynamic_multi_field_edit dynamic_;
  friend class scene_edit;
};

template <> class multi_field_edit<node_list> {
public:
  result<std::size_t> size() const { return dynamic_.size(); }
  result<node> at(std::size_t index) const { return dynamic_.node_at(index); }
  result<void> set(std::size_t index, const node &element) {
    return dynamic_.set(index, element);
  }
  result<void> insert(std::size_t index, const node &element) {
    return dynamic_.insert(index, element);
  }
  result<void> erase(std::size_t index) { return dynamic_.erase(index); }
  result<void> append(const node &element) { return dynamic_.append(element); }
  result<void> clear() { return dynamic_.clear(); }
  result<void> replace(std::span<const node> sequence) {
    return dynamic_.replace(sequence);
  }

private:
  explicit multi_field_edit(dynamic_multi_field_edit dynamic)
      : dynamic_(std::move(dynamic)) {}
  dynamic_multi_field_edit dynamic_;
  friend class scene_edit;
};

class scene_edit {
public:
  scene_edit(scene_edit &&) noexcept;
  scene_edit &operator=(scene_edit &&) noexcept;
  ~scene_edit();
  scene_edit(const scene_edit &) = delete;
  scene_edit &operator=(const scene_edit &) = delete;

  result<node> create_node(const std::string &type_name);
  result<void> remove_node(const node &target);
  template <class Tag> result<typed_node<Tag>> create() {
    if constexpr (requires { std::string_view{Tag::schema_fingerprint}; }) {
      auto verified = require_schema_fingerprint(Tag::schema_fingerprint);
      if (!verified)
        return failure(verified.error());
    }
    return create_node(std::string{Tag::x3d_name}).transform([](node created) {
      return typed_node<Tag>{std::move(created)};
    });
  }
  result<void> append_root(const node &child);
  template <class Tag> result<void> append_root(const typed_node<Tag> &child) {
    return append_root(child.dynamic());
  }
  result<void> remove_root(std::size_t index);
  result<void> append(const node &parent, const std::string &field,
                      const node &child);
  result<void> declare_unit(unit_declaration declaration);
  result<dynamic_field> field(const node &owner, const std::string &name) const;
  result<bool> writable(const dynamic_field &target, write_intent intent) const;
  result<dynamic_multi_field_edit> multi(const dynamic_field &target);
  template <multi_field_sequence Sequence>
  result<multi_field_edit<Sequence>>
  multi(const experimental::field<Sequence> &target) {
    auto dynamic = multi(target.dynamic_);
    if (!dynamic)
      return failure(dynamic.error());
    return multi_field_edit<Sequence>{std::move(dynamic).value()};
  }
  result<void> set(const dynamic_field &target, value new_value);
  result<void> set(const dynamic_field &target, value new_value,
                   value_space space);
  result<void> set(const dynamic_field &target, const node &new_value);
  result<void> set(const dynamic_field &target,
                   std::span<const node> new_value);
  template <class T>
    requires(!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<void> set(const experimental::field<T> &target, T new_value) {
    return set(target.dynamic_, value{std::move(new_value)});
  }
  template <class T>
    requires(!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<void> set(const experimental::field<T> &target, T new_value,
                   value_space space) {
    return set(target.dynamic_, value{std::move(new_value)}, space);
  }
  result<void> set(const experimental::field<node_id> &target,
                   const node &new_value) {
    return set(target.dynamic_, new_value);
  }
  template <class Tag>
  result<void> set(const experimental::field<node_id> &target,
                   const typed_node<Tag> &new_value) {
    return set(target.dynamic_, new_value.dynamic());
  }
  result<void> set(const experimental::field<node_list> &target,
                   std::span<const node> new_value) {
    return set(target.dynamic_, new_value);
  }
  template <std::ranges::input_range Range>
    requires requires(std::ranges::range_reference_t<Range> item) {
      { item.dynamic() } -> std::same_as<const node &>;
    }
  result<void> set(const experimental::field<node_list> &target,
                   const Range &new_value) {
    std::vector<node> dynamic_nodes;
    if constexpr (std::ranges::sized_range<Range>)
      dynamic_nodes.reserve(std::ranges::size(new_value));
    for (const auto &typed : new_value)
      dynamic_nodes.push_back(typed.dynamic());
    return set(target.dynamic_, std::span<const node>{dynamic_nodes});
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
  result<void> require_schema_fingerprint(std::string_view expected);
  result<void> set_value(const dynamic_field &target, value new_value,
                         bool node_authority_checked);
  explicit scene_edit(
      std::shared_ptr<detail::scene_edit_control> implementation);
  std::shared_ptr<detail::scene_edit_control> impl_;
  friend class execution_context;
};

class scene_snapshot {
public:
  revision_id revision() const noexcept;
  const std::vector<unit_declaration> &units() const noexcept;
  const std::vector<node_id> &roots() const noexcept;
  std::vector<occurrence> occurrences() const;
  result<node> lookup(node_id id) const;
  result<node_type_descriptor> describe(const node &owner) const;
  result<node_type_descriptor> describe(const imported_node &owner) const;
  result<dynamic_field> field(const node &owner, const std::string &name) const;
  result<dynamic_imported_field> field(const imported_node &owner,
                                       const std::string &name) const;
  result<bool> readable(const dynamic_field &source) const;
  result<value> read(const dynamic_field &source) const;
  result<value> read(const dynamic_field &source, value_space space) const;
  result<value> read(const dynamic_imported_field &source) const;
  result<value> read(const dynamic_imported_field &source,
                     value_space space) const;
  template <class T>
  result<T> read(const experimental::field<T> &source) const {
    auto dynamic = read(source.dynamic_);
    if (!dynamic)
      return failure(dynamic.error());
    auto payload = std::move(dynamic).value();
    if (auto *typed = std::get_if<T>(&payload))
      return std::move(*typed);
    return typed_read_mismatch(source.dynamic_, "scene_snapshot.read_typed");
  }
  template <class T>
  result<T> read(const experimental::field<T> &source,
                 value_space space) const {
    auto dynamic = read(source.dynamic_, space);
    if (!dynamic)
      return failure(dynamic.error());
    auto payload = std::move(dynamic).value();
    if (auto *typed = std::get_if<T>(&payload))
      return std::move(*typed);
    return typed_read_mismatch(source.dynamic_, "scene_snapshot.read_typed");
  }
  template <class T>
  result<T> read(const experimental::imported_field<T> &source) const {
    auto dynamic = read(source.dynamic_);
    if (!dynamic)
      return failure(dynamic.error());
    auto payload = std::move(dynamic).value();
    if (auto *typed = std::get_if<T>(&payload))
      return std::move(*typed);
    return typed_read_mismatch(source.dynamic_,
                               "scene_snapshot.read_imported_typed");
  }
  template <class T>
  result<T> read(const experimental::imported_field<T> &source,
                 value_space space) const {
    auto dynamic = read(source.dynamic_, space);
    if (!dynamic)
      return failure(dynamic.error());
    auto payload = std::move(dynamic).value();
    if (auto *typed = std::get_if<T>(&payload))
      return std::move(*typed);
    return typed_read_mismatch(source.dynamic_,
                               "scene_snapshot.read_imported_typed");
  }
  result<node> named(const std::string &name) const;
  const std::vector<name_binding> &names() const noexcept;
  result<node> exported(const std::string &name) const;
  const std::vector<export_binding> &exports() const noexcept;
  result<imported_node> imported(const std::string &local_name) const;
  const std::vector<import_binding> &imports() const noexcept;
  const std::vector<route> &routes() const noexcept;

private:
  static unexpected typed_read_mismatch(const dynamic_field &source,
                                        std::string operation) {
    sai_error error;
    error.code = error_code::type_mismatch;
    error.operation = std::move(operation);
    error.message = "typed field representation does not match its handle";
    error.generation = source.generation_;
    error.node = source.node_;
    error.field = source.name_;
    return failure(std::move(error));
  }
  static unexpected typed_read_mismatch(const dynamic_imported_field &source,
                                        std::string operation) {
    sai_error error;
    error.code = error_code::type_mismatch;
    error.operation = std::move(operation);
    error.message =
        "typed imported field representation does not match its handle";
    error.generation = source.source_generation_;
    error.node = source.node_;
    error.field = source.name_;
    return failure(std::move(error));
  }
  scene_snapshot(std::shared_ptr<detail::context_control> context,
                 std::shared_ptr<const detail::scene_state> state);

  std::shared_ptr<detail::context_control> context_;
  std::shared_ptr<const detail::scene_state> state_;
  std::vector<std::shared_ptr<detail::context_control>> import_contexts_;
  std::vector<std::shared_ptr<const detail::scene_state>> import_states_;
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
  result<bool> writable(const dynamic_field &target, write_intent intent) const;
  result<void> send(const dynamic_field &target, value payload,
                    value_space space);
  result<void> send(const dynamic_field &target, const node &payload);
  result<void> send(const dynamic_field &target, std::span<const node> payload);
  template <class T>
    requires(!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<void> send(const experimental::field<T> &target, T payload) {
    return send(target.dynamic_, value{std::move(payload)});
  }
  template <class T>
    requires(!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<void> send(const experimental::field<T> &target, T payload,
                    value_space space) {
    return send(target.dynamic_, value{std::move(payload)}, space);
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
