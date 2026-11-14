#ifndef X3D_SAI_EXPERIMENTAL_PROVIDER_HPP
#define X3D_SAI_EXPERIMENTAL_PROVIDER_HPP
#include "schema.hpp"
#include "user_data.hpp"
#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>

// The single experimental provider-neutral source contract. No ISO binding,
// profile conformance or stable ABI is claimed. Setup and live operations use
// the same owner-bearing identities; optional reference semantics stay separate.
namespace x3d::sai::experimental::provider {
class service;
class backend;

enum class object_kind { scene, context, node };
template <object_kind Kind> class handle {
public:
  handle() = default;
  bool expired() const noexcept { return owner_.expired(); }
  bool is_null() const noexcept { return scene_ == 0; }
  bool disposed() const noexcept { return disposed_; }
  void dispose() noexcept { disposed_ = true; }
  friend bool operator==(const handle &a, const handle &b) noexcept {
    return a.scene_identity_ == b.scene_identity_ && a.scene_ == b.scene_ && a.context_ == b.context_ && a.node_ == b.node_ &&
      !a.owner_.owner_before(b.owner_) && !b.owner_.owner_before(a.owner_);
  }
private:
  std::weak_ptr<const void> owner_;
  std::shared_ptr<const void> scene_identity_;
  std::uint64_t scene_ = 0, context_ = 0, node_ = 0;
  bool disposed_ = false;
  friend class service;
};
using scene = handle<object_kind::scene>;
using context = handle<object_kind::context>;
using node = handle<object_kind::node>;
struct named_node {
  std::string name;
  node target;
  friend bool operator==(const named_node &, const named_node &) = default;
};
// Only disengaged optionals mean X3D NULL. An engaged empty/disposed/expired
// node remains an invalid reference; it never silently becomes NULL.
using node_value = std::optional<node>;
using node_values = std::vector<node_value>;
template <class T> struct semantic_value { using type = T; };
template <> struct semantic_value<node_id> { using type = node_value; };
template <> struct semantic_value<node_list> { using type = node_values; };
template <class T> using semantic_value_t = typename semantic_value<T>::type;
namespace detail {
template <class T> struct project_value;
template <class... T> struct project_value<std::variant<T...>> {
  using type = std::variant<semantic_value_t<T>...>;
};
}
using payload = typename detail::project_value<experimental::value>::type;
std::optional<value_kind> kind_of(const payload &) noexcept;
struct field_info {
  std::string name;
  value_kind kind;
  access_type access;
  bool supported = true;
  friend bool operator==(const field_info &, const field_info &) = default;
};
class field {
public:
  field() = default;
  std::string_view name() const noexcept { return info_.name; }
  const field_info &descriptor() const noexcept { return info_; }
  bool expired() const noexcept { return node_.expired(); }
  bool is_null() const noexcept { return node_.is_null(); }
  bool disposed() const noexcept { return disposed_; }
  void dispose() noexcept { disposed_ = true; }
  friend bool operator==(const field &a, const field &b) noexcept {
    return a.node_ == b.node_ && a.info_.name == b.info_.name;
  }
private:
  node node_;
  field_info info_{};
  bool disposed_ = false;
  friend class service;
};
class activation {
public:
  activation() = default;
  friend bool operator==(const activation &, const activation &) = default;
private:
  scene scene_;
  std::uint64_t epoch_ = 0;
  friend class service;
};
class client {
public:
  client() = default;
private:
  activation activation_;
  std::uint64_t id_ = 0;
  friend class service;
};
enum class write_mode { replace, set_element, append, insert, remove, clear };
struct write {
  field target;
  payload value;
  write_mode mode = write_mode::replace;
  std::size_t index = 0;
};
enum class request_state { accepted, submitted, processed, cancelled, failed };
enum class failure_effect { none, partial_or_unknown };
enum class activation_state { setup, active, faulted, retired };
struct request_status {
  request_state state = request_state::accepted;
  std::uint64_t turn = 0;
  failure_effect effects = failure_effect::none;
  std::optional<sai_error> error;
  std::uint64_t submission = 0;
};
// A copyable owning receipt. Pending work does not depend on a caller keeping
// it alive. Snapshots are thread-safe, including after service destruction.
// Receipts never own backend scenes or request payloads.
class ticket {
public:
  ticket() = default;
  std::uint64_t id() const noexcept;
  result<request_status> status() const;
private:
  struct receipt;
  std::shared_ptr<receipt> receipt_;
  friend class service;
};
// Per-service admission budget, including held work; zero admits no work.
// Payload accounting includes each public write object, canonical field bytes,
// and logical dynamic payload bytes. Route requests account one admitted_route
// object and both canonical field names. It excludes allocator overhead,
// backend state, callback captures and consumer-retained receipts.
// Notifications separately account each stored notification object, canonical
// field bytes and logical dynamic value bytes. Detached dispatch deliveries are
// no longer queued. Counts are bounds on retained work, not total process
// memory.
struct resource_limits {
  std::size_t pending_requests = 1024;
  std::size_t payload_bytes = 16 * 1024 * 1024;
  std::size_t live_registrations = 1024;
  std::size_t queued_notifications = 4096;
  std::size_t notification_bytes = 16 * 1024 * 1024;
};
struct turn_report {
  std::uint64_t turn = 0;
  event_time time;
  std::optional<std::uint64_t> request;
  activation_state state = activation_state::active;
  failure_effect effects = failure_effect::none;
  std::optional<sai_error> error;
};
struct notification {
  field source;
  payload value;
  std::uint64_t activation_epoch = 0;
  std::uint64_t turn = 0;
  event_time time;
};
class subscription {
public:
  subscription() = default;
  subscription(subscription &&) noexcept = default;
  subscription &operator=(subscription &&other) noexcept;
  subscription(const subscription &) = delete;
  subscription &operator=(const subscription &) = delete;
  ~subscription() { cancel(); }
  // Cancellation and token destruction are safe on other threads; an already
  // executing callback is not interrupted. Callback capture cleanup stays on
  // the service thread at a later drain/close.
  void cancel() noexcept;
private:
  std::shared_ptr<std::atomic_bool> live_;
  explicit subscription(std::shared_ptr<std::atomic_bool> live) : live_(std::move(live)) {}
  friend class service;
};
enum class service_id {
  scene_authoring,
  host_turns,
  notifications,
  node_values,
  indexed_writes,
  user_data,
  reference_snapshots,
  reference_transactions,
  reference_checkpoints,
  scene_units,
  scene_metadata,
  live_routes
};
struct service_capability {
  service_id id;
  std::uint32_t version = 1;
  friend bool operator==(const service_capability &, const service_capability &) = default;
};
struct node_capability {
  std::string name;
  // Schema membership only. These facts never claim that the backend provides
  // the complete component at this level, or any complete X3D profile.
  std::string required_component;
  std::uint32_t minimum_declared_level = 0;
  friend bool operator==(const node_capability &, const node_capability &) = default;
};
struct capabilities {
  std::string backend_name;
  std::uint32_t contract_version = 1;
  std::vector<service_capability> services;
  std::vector<node_capability> nodes;
  // Implemented value services across the declared node slice; each canonical
  // field descriptor still reports actual per-field support and access.
  std::vector<value_kind> field_kinds;
  bool supports(service_id id, std::uint32_t version = 1) const noexcept {
    return std::any_of(services.begin(), services.end(), [&](const auto &s) {
      return s.id == id && s.version >= version;
    });
  }
};

// Backend authors implement this dispatch seam. These IDs and values are only
// backend storage addresses, never application authority. The frontend alone
// wraps/unwraps node values and controls admission, gates, receipts and events.
class backend {
public:
  using backend_value = experimental::value;
  struct address { std::uint64_t scene, context, node; std::string field; };
  struct name_binding { std::string name; std::uint64_t node; };
  struct admitted_write { address target; backend_value value; };
  struct admitted_route {
    address from, to;
  };
  struct backend_failure { sai_error error; failure_effect effects; };
  using backend_result = tl::expected<void, backend_failure>;
  virtual ~backend() = default;
  virtual capabilities supported() const = 0;
protected:
  virtual result<std::uint64_t> do_create_scene() = 0;
  // Optional scene-information seam. Units are explicit declarations here;
  // the frontend adds effective SI defaults. Storage stays backend-owned.
  virtual result<std::vector<unit_declaration>> do_units(std::uint64_t) const;
  virtual result<void> do_declare_unit(std::uint64_t, const unit_declaration &);
  virtual result<std::vector<metadata_entry>> do_metadata(std::uint64_t) const;
  virtual result<void> do_set_metadata(std::uint64_t, std::string_view,
                                       const std::optional<std::string> &);
  virtual result<std::uint64_t> do_create_node(std::uint64_t scene, std::string_view type) = 0;
  virtual result<void> do_append_root(const address &) = 0;
  virtual result<void> do_define_name(const address &, std::string_view) = 0;
  virtual result<std::vector<std::uint64_t>> do_roots(std::uint64_t scene) const = 0;
  virtual result<std::uint64_t> do_named(std::uint64_t scene, std::string_view) const = 0;
  // Optional discovery; default fails closed for existing backend subclasses.
  virtual result<std::vector<name_binding>> do_named_nodes(std::uint64_t scene) const;
  virtual result<std::string> do_type_name(const address &) const = 0;
  virtual result<std::vector<field_info>> do_fields(const address &) const = 0;
  virtual result<backend_value> do_read(const address &) const = 0;
  virtual result<void> do_author(const address &, const backend_value &) = 0;
  virtual result<void> do_add_route(const address &, const address &) = 0;
  // Optional live topology extension. Called at pump before do_turn; default
  // refuses without effects. Partial installation must report honest effects.
  // An exact duplicate of an installed route (same source node/field and sink
  // node/field) is redundant (ISO 19775-1 4.4.8.2): return success, no effect.
  virtual backend_result do_add_live_route(const address &, const address &);
  // Optional admission pre-check for enqueue_route, after the portable checks.
  // Refuse backend-specific restrictions (for example native kinds that share
  // a portable kind) here so they fail at enqueue, not at pump. The default
  // admits; do_add_live_route still checks again at installation.
  virtual result<void> do_check_live_route(const address &, const address &) const;
  virtual result<void> do_validate_configuration(std::uint64_t scene,
      const std::vector<admitted_write> &) const = 0;
  virtual result<void> do_activate(std::uint64_t scene) = 0;
  virtual backend_result do_turn(std::uint64_t scene, event_time,
                                 const std::vector<admitted_write> &) = 0;
  virtual void do_retire(std::uint64_t scene) noexcept = 0;
  void record_event(const address &where, backend_value value) {
    if (events_) events_(where, std::move(value));
  }
  // True when a live observer of the pumping activation watches this exact
  // field. A backend may skip reading and recording unobserved writes.
  bool observed(const address &where) const {
    return !observing_ || observing_(where);
  }
  static unexpected error(error_code, std::string_view operation, std::string_view message);
private:
  std::function<void(const address &, backend_value)> events_;
  std::function<bool(const address &)> observing_;
  friend class service;
};

// Owner-thread-affine. No hidden loop, clock, I/O, portable-owned global scene state,
// or application callbacks during backend execution. Handles retain authority,
// never backend storage. A scene's lexical context/node identity is independent
// of activation epoch and turn. This slice has one root lexical context per scene
// and explicitly excludes PROTO/IMPORT/Inline/Script/geospatial nodes.
class service final {
public:
  explicit service(std::unique_ptr<backend>, resource_limits = {});
  ~service();
  service(const service &) = delete;
  service &operator=(const service &) = delete;
  const capabilities &supported() const noexcept;
  result<scene> create_scene();
  // Effective angle/force/length/mass units in that stable order, including SI
  // defaults. These declarations never rescale canonical provider field values.
  result<std::vector<unit_declaration>> units(const context &) const;
  // Setup-only, before the first successful node creation; one declaration per
  // category. Failed declarations have no effect and do not poison the scene.
  result<void> declare_unit(const scene &, unit_declaration);
  // Owning, case-sensitive, insertion-ordered scene META. Missing and empty
  // values differ. Replacement keeps position; removal and reinsert append.
  result<std::vector<metadata_entry>> metadata(const scene &) const;
  result<std::optional<std::string>> metadata(const scene &, std::string_view key) const;
  // Setup-only immediate authoring, not live per-client buffered setMetaData.
  result<void> set_metadata(const scene &, std::string key,
                            std::optional<std::string> value);
  result<node> create_node(const context &, std::string_view type);
  result<void> append_root(const context &, const node &);
  result<void> define_name(const node &, std::string_view);
  result<context> root_context(const scene &) const;
  result<std::vector<node>> roots(const context &) const;
  result<node> named(const context &, std::string_view) const;
  // Owning snapshots in ascending name order, including detached named nodes.
  // Discovery is allowed in setup, active and faulted scenes.
  result<std::vector<named_node>> named_nodes(const context &) const;
  // Names in the node's lexical context; empty for an unnamed node. The current
  // setup contract permits at most one canonical DEF name per node.
  result<std::vector<std::string>> names(const node &) const;
  result<std::string> type_name(const node &) const;
  result<std::vector<field_info>> fields(const node &) const;
  result<field> get_field(const node &, std::string_view) const;
  result<payload> read(const field &) const;
  template <class Owner, class T>
  result<field> get_field(const node &n, const field_key<Owner, T> &key) const {
    auto type = type_name(n);
    if (!type) return failure(type.error());
    if (*type != Owner::x3d_name)
      return error(error_code::type_mismatch, "provider.field", "generated key owner differs from node type");
    auto f = get_field(n, key.name());
    if (!f) return failure(f.error());
    if (f->info_.kind != key.kind)
      return error(error_code::type_mismatch, "provider.field", "generated key kind differs from descriptor");
    if (f->info_.access != key.access())
      return error(error_code::invalid_descriptor, "provider.field", "generated key access differs from descriptor");
    return f;
  }
  template <class Owner, class T>
  result<semantic_value_t<T>> read(const node &n, const field_key<Owner, T> &key) const {
    auto f = get_field(n, key); if (!f) return failure(f.error());
    auto v = read(*f); if (!v) return failure(v.error());
    if (auto *typed = std::get_if<semantic_value_t<T>>(&*v)) return std::move(*typed);
    return error(error_code::type_mismatch, "provider.read", "backend payload differs from descriptor");
  }
  template <class Owner, class T>
  result<void> author(const node &n, const field_key<Owner, T> &key, semantic_value_t<T> v) {
    auto f = get_field(n, key); if (!f) return failure(f.error());
    return author(*f, payload{std::move(v)});
  }
  result<void> author(const field &, payload);
  result<user_data_value> user_data(const node &) const;
  result<user_data_value> user_data(const field &) const;
  result<void> set_user_data(const node &, user_data_value);
  result<void> set_user_data(const field &, user_data_value);
  result<void> add_route(const context &, const field &from, const field &to);
  result<activation> activate(const scene &);
  result<activation_state> state(const scene &) const;
  result<void> retire(const activation &);
  // Also retires a setup scene or a scene whose activation failed.
  result<void> retire(const scene &);
  result<client> connect(const activation &);
  result<void> begin_update(const client &);
  result<void> end_update(const client &);
  // Each call validates intrinsic values and authority. A held gate releases
  // ONE submission. Repeated Begin and unmatched End are no-ops. Within one
  // submission the last stateful whole-field write wins; every inputOnly
  // whole-field occurrence is preserved. Indexed MF edits aggregate in call
  // order into the latest pending MF occurrence, also for inputOnly. Release and execution validate the final
  // coupled candidate before backend entry. This is explicit SDK policy.
  // Resource limits count calls and owning payload bytes, including held work.
  result<ticket> enqueue(const client &, std::vector<write>);
  // Optional FIFO request. A released update gate installs all its routes
  // before evaluating its retained writes and ordered input occurrences.
  result<ticket> enqueue_route(const client &, const field &from,
                               const field &to);
  template <class Owner, class T>
  result<ticket> enqueue(const client &c, const node &n,
                         const field_key<Owner, T> &key, semantic_value_t<T> v) {
    auto f = get_field(n, key); if (!f) return failure(f.error());
    return enqueue(c, std::vector<write>{{*f, payload{std::move(v)}}});
  }
  // Cancelling a released ticket cancels its entire shared submission. Held
  // calls remain individually cancellable before release.
  result<void> cancel(const client &, const ticket &);
  result<void> disconnect(const client &);
  // At most one submitted batch per turn, FIFO within an activation. An empty
  // pump still advances behavior. Time is finite/nondecreasing per activation.
  // Known-no-effect failure fails only that submission. Partial/unknown effects
  // fault the activation. No portable rollback or automatic retry is promised.
  result<turn_report> pump(const activation &, event_time);
  result<subscription> observe(const activation &, const field &,
                               std::function<void(const notification &)>);
  template <class Callback>
    requires(!std::is_copy_constructible_v<std::decay_t<Callback>> &&
             std::constructible_from<std::decay_t<Callback>, Callback &&> &&
             std::is_invocable_r_v<void, std::decay_t<Callback> &, const notification &>)
  result<subscription> observe(const activation &a, const field &f, Callback &&callback) {
    auto owned = std::make_shared<std::decay_t<Callback>>(std::forward<Callback>(callback));
    return observe(a, f, std::function<void(const notification &)>{
      [owned = std::move(owned)](const notification &n) { std::invoke(*owned, n); }});
  }
  // Explicit callback drain. Cancellation/retirement discards queued notices.
  // A later fault does not revoke earlier successfully completed turn notices;
  // failed-turn output is discarded before this drain.
  // Callback exceptions are isolated and reported after other callbacks run.
  result<std::size_t> dispatch_notifications();
  // Explicit close is checked; a rejected close leaves the service untouched.
  // Destruction requires owner-thread teardown or a previously closed service.
  result<void> close();
private:
  using address = backend::address;
  using admitted_write = backend::admitted_write;
  using backend_result = backend::backend_result;
  using backend_failure = backend::backend_failure;
  using backend_value = backend::backend_value;
  struct queued_write;
  struct impl;
  static unexpected error(error_code, std::string_view, std::string_view);
  static void record_event(const std::weak_ptr<impl> &, const address &, backend_value);
  static bool observing(const std::weak_ptr<impl> &, const address &);
  result<backend_value> import_value(const field &, const payload &) const;
  result<payload> export_value(const address &, const backend_value &) const;
  result<std::vector<admitted_write>> prepare(const std::vector<queued_write> &) const;
  std::shared_ptr<impl> p_;
  result<void> check() const;
  template <object_kind K> result<void> validate(const handle<K> &) const;
  result<void> validate(const field &) const;
  result<void> validate(const activation &) const;
  result<void> validate(const client &) const;
  result<void> validate_value(const field &, const payload &, write_mode = write_mode::replace) const;
  template <object_kind K> handle<K> make(std::uint64_t s, std::uint64_t c = 1,
                                        std::uint64_t n = 0) const;
  static address where(const field &);
  static address where(const node &);
};
} // namespace x3d::sai::experimental::provider
#endif
