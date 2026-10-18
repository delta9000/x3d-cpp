#include "x3d/sai/experimental/provider.hpp"
#include "provider_budget.hpp"
#include "x3d/sai/experimental/image.hpp"
#include "x3d/sai/experimental/value_constraints.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <map>
#include <list>
#include <mutex>
#include <set>
#include <tuple>

namespace x3d::sai::experimental::provider {
namespace {
const std::shared_ptr<const void> &thread_identity() {
  // Retaining the token prevents an unrelated thread from inheriting authority
  // when the OS recycles a thread ID after the creating thread exits.
  thread_local const std::shared_ptr<const void> token = std::make_shared<const int>(0);
  return token;
}
}
namespace {
template <class T> bool intrinsic_value(const T &v) {
  if constexpr (std::same_as<T, rotation>) return experimental::detail::valid_rotation_axis(v);
  else if constexpr (std::same_as<T, image>) return experimental::detail::image_layout_valid(v);
  else if constexpr (std::same_as<T, color3f> || std::same_as<T, color4f>) {
    const auto channel = [](float c) { return c >= 0 && c <= 1; };
    const bool rgb = channel(v.r) && channel(v.g) && channel(v.b);
    if constexpr (std::same_as<T, color4f>) return rgb && channel(v.a);
    else return rgb;
  } else if constexpr (requires { typename T::value_type; v.begin(); v.end(); })
    return std::all_of(v.begin(), v.end(), [](const auto &element) { return intrinsic_value(element); });
  else return true;
}
bool intrinsic_payload(const payload &v) {
  return std::visit([](const auto &value) { return intrinsic_value(value); }, v);
}
// Narrow source-backed constraints for the currently advertised geometry slice.
// ISO/IEC 19775-1:2023, Geometry3D 13.3.1 and Grouping 10.2.2:
// https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/components/geometry3D.html#Box
// https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/components/grouping.html#Boundingboxes
// These checks are not a complete generated/node constraint audit. In
// particular, an arbitrary author-declared field with the same name does not
// acquire a built-in-node constraint. No current value is read before a repair.
template <class TypeLookup>
result<void> validate_scoped_value(std::string_view field,
    const payload &candidate, std::string_view operation, TypeLookup lookup) {
  const auto *size = std::get_if<vec3f>(&candidate);
  const bool dimension = size && (field == "size" || field == "bboxSize");
  const bool curve = field == "key" || field == "keyValue" ||
      field == "set_fraction" || field == "value_changed";
  if (!dimension && !curve) return {};
  auto type = lookup();
  if (!type) return failure(type.error());
  // Finite interpolation payloads are the bounded adapter proposal's policy,
  // not a blanket restriction on generic SFFloat/SFVec3f/MF values. Ordinary
  // Transform translation/center/scale/bboxCenter retain their supplied bits.
  if (*type == "PositionInterpolator" && curve) {
    const auto finite_vector = [](const vec3f &v) {
      return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    const bool finite = std::visit([&](const auto &v) {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::same_as<T,float>) return std::isfinite(v);
      else if constexpr (std::same_as<T,vec3f>) return finite_vector(v);
      else if constexpr (std::same_as<T,float_list>)
        return std::all_of(v.begin(),v.end(),[](float x) { return std::isfinite(x); });
      else if constexpr (std::same_as<T,vec3f_list>)
        return std::all_of(v.begin(),v.end(),finite_vector);
      else return true; // Exact field/element kind is validated independently.
    },candidate);
    if (finite) return {};
    sai_error error; error.code=error_code::invalid_value;
    error.operation=operation; error.field=field;
    error.message="bounded PositionInterpolator policy requires finite curve/input/output values";
    return failure(std::move(error));
  }
  if (!dimension) return {};
  const bool box = *type == "Box" && field == "size";
  const bool bounds = (*type == "Transform" || *type == "Shape") && field == "bboxSize";
  if (!box && !bounds) return {};
  const bool finite = std::isfinite(size->x) && std::isfinite(size->y) && std::isfinite(size->z);
  const bool valid = finite && (box
      ? size->x > 0 && size->y > 0 && size->z > 0
      : (*size == vec3f{-1, -1, -1} || (size->x >= 0 && size->y >= 0 && size->z >= 0)));
  if (valid) return {};
  sai_error error; error.code = error_code::invalid_value;
  error.operation = operation; error.field = field;
  error.message = box ? "Box.size components must be finite and greater than zero"
      : "bboxSize must be finite and componentwise nonnegative or exactly (-1,-1,-1)";
  return failure(std::move(error));
}
}
std::optional<value_kind> kind_of(const payload &v) noexcept {
  return std::visit([](const auto &item) -> std::optional<value_kind> {
    using T = std::decay_t<decltype(item)>;
    if constexpr (std::same_as<T, std::monostate>) return std::nullopt;
    else if constexpr (std::same_as<T, node_value>) return value_kind::sf_node;
    else if constexpr (std::same_as<T, node_values>) return value_kind::mf_node;
    else return value_traits<T>::kind;
  }, v);
}
subscription &subscription::operator=(subscription &&other) noexcept {
  if (this != &other) { cancel(); live_ = std::move(other.live_); }
  return *this;
}
void subscription::cancel() noexcept { if (live_) live_->store(false); live_.reset(); }
struct ticket::receipt {
  struct outcome {
    mutable std::mutex mutex;
    request_status status;
  };
  mutable std::mutex mutex;
  std::shared_ptr<outcome> shared_outcome;
  request_status status;
  std::weak_ptr<const void> authority;
  std::uint64_t id = 0, client = 0;
  // Owner-thread-only link. Cleared before removing the live FIFO entry.
  // It never owns that entry or any backend/payload storage.
  void *pending = nullptr;
};
std::uint64_t ticket::id() const noexcept { return receipt_ ? receipt_->id : 0; }
result<request_status> ticket::status() const {
  if (!receipt_) {
    sai_error e; e.code = error_code::stale_handle; e.operation = "provider.ticket";
    e.message = "receipt is empty"; return failure(std::move(e));
  }
  std::shared_ptr<receipt::outcome> outcome;
  {
    std::lock_guard lock(receipt_->mutex);
    outcome = receipt_->shared_outcome;
    if (!outcome) return receipt_->status;
  }
  std::lock_guard lock(outcome->mutex);
  return outcome->status;
}
struct service::queued_write {
  address target;
  backend_value value;
  access_type access;
  value_kind kind;
  write_mode mode;
  std::size_t index;
};
struct service::impl {
  struct request;
  using request_queue = std::list<request>;
  struct request {
    std::vector<std::shared_ptr<ticket::receipt>> receipts;
    std::vector<queued_write> writes;
    std::size_t bytes = 0;
    request_queue *queue = nullptr;
    request_queue::iterator position;
  };
  struct scene_state {
    std::shared_ptr<const void> identity = std::make_shared<const int>(0);
    activation_state state = activation_state::setup;
    bool content_started = false;
    std::uint64_t epoch = 0, turn = 0;
    std::optional<double> time;
    request_queue submitted;
  };
  struct client_state {
    activation world;
    bool holding = false;
    request_queue accepted;
  };
  struct observer {
    activation world;
    field source;
    std::shared_ptr<std::atomic_bool> live;
    std::shared_ptr<const std::function<void(const notification &)>> callback;
  };
  struct notice { notification event; std::weak_ptr<observer> target; std::size_t bytes = 0; };
  impl(std::unique_ptr<backend> value, resource_limits budget)
      : implementation(std::move(value)), facts(implementation->supported()), limits(budget) {}
  std::unique_ptr<backend> implementation;
  capabilities facts;
  using metadata_key = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::string>;
  std::map<metadata_key, user_data_value> metadata;
  std::shared_ptr<const void> authority = std::make_shared<int>(0);
  std::shared_ptr<const void> owner = thread_identity();
  const resource_limits limits;
  std::size_t pending_requests = 0, pending_bytes = 0, notification_bytes = 0;
  bool closed = false, pumping = false, dispatching = false;
  std::uint64_t next_epoch = 1, next_client = 1, next_request = 1;
  std::uint64_t pumping_scene = 0;
  event_time pumping_time;
  std::optional<sai_error> capture_error;
  std::map<std::uint64_t, scene_state> scenes;
  std::map<std::uint64_t, client_state> clients;
  std::vector<std::shared_ptr<observer>> observers;
  std::deque<notice> notices;

  template <class Predicate> void discard_notices(Predicate predicate) {
    std::erase_if(notices, [&](const auto &notice) {
      if (!predicate(notice)) return false;
      notification_bytes -= notice.bytes;
      return true;
    });
  }
  // Publish registry cleanup before returning owning captures to the caller.
  // Their destruction is deferred until that caller's final state access.
  std::vector<std::shared_ptr<observer>> detach_cancelled() {
    discard_notices([](const auto &notice) {
      const auto target = notice.target.lock();
      return !target || !target->live->load();
    });
    std::vector<std::shared_ptr<observer>> retained, released;
    retained.reserve(observers.size());
    released.reserve(observers.size());
    for (auto &entry : observers)
      (entry->live->load() ? retained : released).push_back(std::move(entry));
    observers = std::move(retained);
    return released;
  }
  void finish(request &r, request_status status) {
    auto &first = *r.receipts.front();
    if (first.shared_outcome) {
      std::lock_guard lock(first.shared_outcome->mutex);
      status.submission = first.shared_outcome->status.submission;
      first.shared_outcome->status = std::move(status);
    } else {
      std::lock_guard lock(first.mutex);
      status.submission = first.status.submission;
      first.status = std::move(status);
    }
    for (auto &receipt : r.receipts) receipt->pending = nullptr;
    pending_requests -= r.receipts.size();
    pending_bytes -= r.bytes;
    r.queue->erase(r.position);
  }
  void cancel_all(request_queue &queue) {
    while (!queue.empty()) finish(queue.front(), {request_state::cancelled, 0, failure_effect::none, {}});
  }
  void cancel_scene(std::uint64_t scene) {
    cancel_all(scenes.at(scene).submitted);
    for (auto &[id, c] : clients)
      if (c.world.scene_.scene_ == scene) cancel_all(c.accepted);
  }
};
service::service(std::unique_ptr<backend> implementation, resource_limits limits) {
  if (!implementation) throw std::invalid_argument("provider backend must not be null");
  p_ = std::make_shared<impl>(std::move(implementation), limits);
  if (!p_->facts.supports(service_id::user_data))
    p_->facts.services.push_back({service_id::user_data, 1});
  if (!p_->facts.supports(service_id::indexed_writes))
    p_->facts.services.push_back({service_id::indexed_writes, 1});
  p_->implementation->events_ = [weak = std::weak_ptr<impl>{p_}](
      const address &where, backend_value value) {
    record_event(weak, where, std::move(value));
  };
}
service::~service() {
  if (p_ && !p_->closed) {
    try { if (!close()) std::terminate(); } catch (...) { std::terminate(); }
  }
}
const capabilities &service::supported() const noexcept { return p_->facts; }
unexpected backend::error(error_code code, std::string_view op, std::string_view message) {
  sai_error e; e.code = code; e.operation = op; e.message = message; return failure(std::move(e));
}
unexpected service::error(error_code code, std::string_view op, std::string_view message) {
  sai_error e; e.code = code; e.operation = op; e.message = message; return failure(std::move(e));
}
result<std::vector<unit_declaration>> backend::do_units(std::uint64_t) const {
  return error(error_code::unsupported_operation, "provider.units", "backend does not provide scene units");
}
result<void> backend::do_declare_unit(std::uint64_t, const unit_declaration &) {
  return error(error_code::unsupported_operation, "provider.declare_unit", "backend does not provide scene units");
}
result<std::vector<metadata_entry>> backend::do_metadata(std::uint64_t) const {
  return error(error_code::unsupported_operation, "provider.metadata", "backend does not provide scene metadata");
}
result<void> backend::do_set_metadata(std::uint64_t, std::string_view,
                                      const std::optional<std::string> &) {
  return error(error_code::unsupported_operation, "provider.set_metadata", "backend does not provide scene metadata");
}
result<std::vector<backend::name_binding>> backend::do_named_nodes(std::uint64_t) const {
  return error(error_code::unsupported_operation, "provider.named_nodes", "backend does not provide named-node enumeration");
}
result<void> service::check() const {
  if (p_->owner != thread_identity())
    return error(error_code::wrong_thread, "provider", "service belongs to another thread");
  if (p_->closed) return error(error_code::stale_handle, "provider", "service is closed");
  if (p_->pumping) return error(error_code::operation_in_progress, "provider", "native turn is in progress");
  return {};
}
template <object_kind K> result<void> service::validate(const handle<K> &h) const {
  if (auto ok = check(); !ok) return ok;
  if (h.disposed_) return error(error_code::disposed, "provider.handle", "wrapper is disposed");
  if (!h.scene_ || h.owner_.expired())
    return error(error_code::stale_handle, "provider.handle", "handle authority has expired");
  if (h.owner_.owner_before(p_->authority) || p_->authority.owner_before(h.owner_))
    return error(error_code::invalid_context, "provider.handle", "foreign service authority");
  auto found = p_->scenes.find(h.scene_);
  if (found == p_->scenes.end() || found->second.identity != h.scene_identity_ ||
      found->second.state == activation_state::retired)
    return error(error_code::stale_handle, "provider.handle", "scene has retired");
  if (h.context_ != 1 || (K == object_kind::node && !h.node_))
    return error(error_code::invalid_context, "provider.handle", "invalid lexical identity");
  return {};
}
result<void> service::validate(const field &f) const {
  if (auto ok = check(); !ok) return ok;
  if (f.disposed_) return error(error_code::disposed, "provider.field", "wrapper is disposed");
  auto descriptors = fields(f.node_);
  if (!descriptors) return failure(descriptors.error());
  const auto found = std::find_if(descriptors->begin(), descriptors->end(), [&](const auto &info) { return info.name == f.info_.name; });
  if (found == descriptors->end()) return error(error_code::unknown_field, "provider.field", "canonical field no longer exists");
  if (*found != f.info_) return error(error_code::invalid_descriptor, "provider.field", "canonical field descriptor changed after handle acquisition");
  return {};
}
result<void> service::validate(const activation &a) const {
  if (auto ok = validate(a.scene_); !ok) return ok;
  const auto &s = p_->scenes.at(a.scene_.scene_);
  if (!a.epoch_ || a.epoch_ != s.epoch || s.state == activation_state::setup)
    return error(error_code::stale_handle, "provider.activation", "activation has expired");
  return {};
}
result<void> service::validate(const client &c) const {
  if (auto ok = validate(c.activation_); !ok) return ok;
  auto found = p_->clients.find(c.id_);
  if (found == p_->clients.end() || !(found->second.world == c.activation_))
    return error(error_code::stale_handle, "provider.client", "client is disconnected");
  return {};
}
template <object_kind K> handle<K> service::make(std::uint64_t s, std::uint64_t c, std::uint64_t n) const {
  handle<K> result; result.owner_ = p_->authority; result.scene_ = s;
  result.scene_identity_ = p_->scenes.at(s).identity;
  result.context_ = c; result.node_ = n; return result;
}
service::address service::where(const node &n) { return {n.scene_, n.context_, n.node_, {}}; }
service::address service::where(const field &f) { auto a = where(f.node_); a.field = f.info_.name; return a; }
namespace {
template <class T> struct sequence_traits { static constexpr bool sequence = false; };
template <class T, class A> struct sequence_traits<std::vector<T, A>> {
  static constexpr bool sequence = true;
  using element = T;
};
template <> struct sequence_traits<bool_list> {
  static constexpr bool sequence = true;
  using element = bool;
};
result<backend::backend_value> empty_sequence(value_kind kind) {
  std::optional<backend::backend_value> empty;
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    ([&] {
      using T = std::variant_alternative_t<I, backend::backend_value>;
      if constexpr (sequence_traits<T>::sequence)
        if (value_traits<T>::kind == kind) empty.emplace(T{});
    }(), ...);
  }(std::make_index_sequence<std::variant_size_v<backend::backend_value>>{});
  if (empty) return std::move(*empty);
  sai_error error; error.code = error_code::type_mismatch;
  error.operation = "provider.clear"; error.message = "field is not an MF kind";
  return failure(std::move(error));
}
bool valid_unit(const unit_declaration &unit) {
  return (unit.category == "angle" || unit.category == "force" ||
          unit.category == "length" || unit.category == "mass") &&
      !unit.name.empty() && unit.name.find_first_of(" \t\r\n\f\v") == std::string::npos &&
      std::isfinite(unit.conversion_factor) && unit.conversion_factor > 0.0;
}
bool valid_name(std::string_view name) {
  const auto letter = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
  return !name.empty() && letter(name.front()) && std::all_of(name.begin() + 1, name.end(), [&](char c) {
    return letter(c) || (c >= '0' && c <= '9');
  });
}
}
result<scene> service::create_scene() {
  if (auto ok = check(); !ok) return failure(ok.error());
  if (!p_->facts.supports(service_id::scene_authoring))
    return error(error_code::unsupported_operation, "provider.create_scene", "backend does not provide scene authoring");
  auto id = p_->implementation->do_create_scene();
  if (!id) return failure(id.error());
  if (!*id || p_->scenes.contains(*id))
    return error(error_code::invalid_context, "provider.create_scene", "backend reused scene identity");
  p_->scenes.emplace(*id, impl::scene_state{});
  return make<object_kind::scene>(*id);
}
result<std::vector<unit_declaration>> service::units(const context &context) const {
  if (auto ok = validate(context); !ok) return failure(ok.error());
  if (!p_->facts.supports(service_id::scene_units))
    return error(error_code::unsupported_operation, "provider.units", "backend does not provide scene units");
  auto declared = p_->implementation->do_units(context.scene_);
  if (!declared) return failure(declared.error());
  std::vector<unit_declaration> effective{{"angle", "radian", 1.0},
      {"force", "newton", 1.0}, {"length", "metre", 1.0}, {"mass", "kilogram", 1.0}};
  std::set<std::string> categories;
  for (const auto &unit : *declared) {
    if (!valid_unit(unit) || !categories.insert(unit.category).second)
      return error(error_code::invalid_descriptor, "provider.units", "backend returned invalid or duplicate units");
    auto found = std::find_if(effective.begin(), effective.end(), [&](const auto &u) {
      return u.category == unit.category;
    });
    if (found == effective.end())
      return error(error_code::invalid_descriptor, "provider.units", "backend returned an unknown unit category");
    *found = unit;
  }
  return effective;
}
result<void> service::declare_unit(const scene &scene, unit_declaration declaration) {
  if (auto ok = validate(scene); !ok) return ok;
  if (!p_->facts.supports(service_id::scene_units))
    return error(error_code::unsupported_operation, "provider.declare_unit", "backend does not provide scene units");
  const auto &state = p_->scenes.at(scene.scene_);
  if (state.state != activation_state::setup || state.content_started)
    return error(error_code::access_denied, "provider.declare_unit", "units are fixed before scene content is authored");
  const auto &category = declaration.category;
  if (!valid_unit(declaration))
    return error(error_code::invalid_value, "provider.declare_unit", "unit requires a standard category, non-whitespace name and finite positive factor");
  auto declared = p_->implementation->do_units(scene.scene_);
  if (!declared) return failure(declared.error());
  if (std::any_of(declared->begin(), declared->end(), [&](const auto &u) { return u.category == category; }))
    return error(error_code::invalid_value, "provider.declare_unit", "unit category is already declared");
  return p_->implementation->do_declare_unit(scene.scene_, declaration);
}
result<std::vector<metadata_entry>> service::metadata(const scene &scene) const {
  if (auto ok = validate(scene); !ok) return failure(ok.error());
  if (!p_->facts.supports(service_id::scene_metadata))
    return error(error_code::unsupported_operation, "provider.metadata", "backend does not provide scene metadata");
  return p_->implementation->do_metadata(scene.scene_);
}
result<std::optional<std::string>> service::metadata(const scene &scene, std::string_view key) const {
  auto entries = metadata(scene);
  if (!entries) return failure(entries.error());
  auto found = std::find_if(entries->begin(), entries->end(), [&](const auto &entry) { return entry.key == key; });
  if (found == entries->end()) return std::optional<std::string>{};
  return std::optional<std::string>{std::move(found->value)};
}
result<void> service::set_metadata(const scene &scene, std::string key,
                                   std::optional<std::string> value) {
  if (auto ok = validate(scene); !ok) return ok;
  if (!p_->facts.supports(service_id::scene_metadata))
    return error(error_code::unsupported_operation, "provider.set_metadata", "backend does not provide scene metadata");
  if (p_->scenes.at(scene.scene_).state != activation_state::setup)
    return error(error_code::access_denied, "provider.set_metadata", "scene metadata authoring is setup-only");
  return p_->implementation->do_set_metadata(scene.scene_, key, value);
}
result<node> service::create_node(const context &context, std::string_view type) {
  if (auto ok = validate(context); !ok) return failure(ok.error());
  if (!p_->facts.supports(service_id::scene_authoring))
    return error(error_code::unsupported_operation, "provider.create_node", "backend does not provide scene authoring");
  if (p_->scenes.at(context.scene_).state != activation_state::setup)
    return error(error_code::access_denied, "provider.create_node", "node topology is setup-only");
  auto id = p_->implementation->do_create_node(context.scene_, type);
  if (!id) return failure(id.error());
  if (!*id) return error(error_code::invalid_descriptor, "provider.create_node", "backend returned a null node identity");
  p_->scenes.at(context.scene_).content_started = true;
  return make<object_kind::node>(context.scene_, context.context_, *id);
}
result<void> service::append_root(const context &context, const node &node) {
  if (auto ok = validate(context); !ok) return ok;
  if (auto ok = validate(node); !ok) return ok;
  if (context.scene_ != node.scene_ || context.context_ != node.context_)
    return error(error_code::invalid_context, "provider.append_root", "root crosses lexical context");
  if (p_->scenes.at(context.scene_).state != activation_state::setup)
    return error(error_code::access_denied, "provider.append_root", "root topology is setup-only");
  return p_->implementation->do_append_root(where(node));
}
result<void> service::define_name(const node &node, std::string_view name) {
  if (auto ok = validate(node); !ok) return ok;
  if (!valid_name(name)) return error(error_code::invalid_name, "provider.define_name", "name is outside the bounded ASCII DEF grammar");
  if (p_->scenes.at(node.scene_).state != activation_state::setup)
    return error(error_code::access_denied, "provider.define_name", "names are setup-only");
  return p_->implementation->do_define_name(where(node), name);
}
result<service::backend_value> service::import_value(const field &target, const payload &value) const {
  return std::visit([&](const auto &v) -> result<backend_value> {
    using T = std::decay_t<decltype(v)>;
    const auto unwrap = [&](const node_value &slot) -> result<node_id> {
      if (!slot) return node_id{};
      if (auto valid = validate(*slot); !valid) return failure(valid.error());
      if (slot->scene_ != target.node_.scene_ || slot->context_ != target.node_.context_)
        return error(error_code::invalid_context, "provider.value", "node value crosses lexical context");
      return node_id{slot->node_};
    };
    if constexpr (std::same_as<T, node_value>) {
      auto id = unwrap(v); if (!id) return failure(id.error());
      return backend_value{*id};
    } else if constexpr (std::same_as<T, node_values>) {
      node_list ids; ids.reserve(v.size());
      for (const auto &slot : v) {
        auto id = unwrap(slot); if (!id) return failure(id.error());
        ids.push_back(*id);
      }
      return backend_value{std::move(ids)};
    } else return backend_value{v};
  }, value);
}
result<payload> service::export_value(const address &target, const backend_value &value) const {
  return std::visit([&](const auto &v) -> result<payload> {
    using T = std::decay_t<decltype(v)>;
    const auto wrap = [&](node_id id) -> result<node_value> {
      if (!id.value) return node_value{};
      address reference{target.scene, target.context, id.value, {}};
      auto type = p_->implementation->do_type_name(reference);
      if (!type) return failure(type.error());
      return node_value{make<object_kind::node>(target.scene, target.context, id.value)};
    };
    if constexpr (std::same_as<T, node_id>) {
      auto reference = wrap(v); if (!reference) return failure(reference.error());
      return payload{*reference};
    } else if constexpr (std::same_as<T, node_list>) {
      node_values references; references.reserve(v.size());
      for (const auto id : v) {
        auto reference = wrap(id); if (!reference) return failure(reference.error());
        references.push_back(std::move(*reference));
      }
      return payload{std::move(references)};
    } else return payload{v};
  }, value);
}
result<std::vector<service::admitted_write>> service::prepare(const std::vector<queued_write> &writes) const {
  std::vector<admitted_write> candidates;
  std::map<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::string>, std::size_t> latest;
  for (const auto &write : writes) {
    const auto key = std::tuple{write.target.scene, write.target.context, write.target.node, write.target.field};
    const auto previous = latest.find(key);
    backend_value candidate;
    if (write.mode == write_mode::replace) candidate = write.value;
    else if (write.mode == write_mode::clear) {
      auto empty = empty_sequence(write.kind);
      if (!empty) return failure(empty.error());
      candidate = std::move(*empty);
    } else {
      if (previous != latest.end()) candidate = candidates[previous->second].value;
      else {
        if (write.access == access_type::input_only)
          return error(error_code::access_denied, "provider.index", "inputOnly MF mutation needs a pending whole-field base");
        auto observed = p_->implementation->do_read(write.target);
        if (!observed) return failure(observed.error());
        candidate = std::move(*observed);
      }
      auto modified = std::visit([&](auto &sequence) -> result<void> {
        using T = std::decay_t<decltype(sequence)>;
        if constexpr (!sequence_traits<T>::sequence)
          return error(error_code::type_mismatch, "provider.index", "backend base is not an MF value");
        else {
          using element = typename sequence_traits<T>::element;
          const auto size = sequence.size();
          const auto index = write.mode == write_mode::append ? size : write.index;
          const bool insert = write.mode == write_mode::append || write.mode == write_mode::insert;
          if (index > size || (!insert && index == size))
            return error(error_code::index_out_of_range, "provider.index", "index is outside the pending MF candidate");
          if (write.mode == write_mode::remove) {
            if constexpr (std::same_as<T, bool_list>) sequence.erase(index);
            else sequence.erase(sequence.begin() + static_cast<std::ptrdiff_t>(index));
            return {};
          }
          const auto *value = std::get_if<element>(&write.value);
          if (!value) return error(error_code::type_mismatch, "provider.index", "element type differs from MF candidate");
          if constexpr (std::same_as<T, bool_list>) {
            if (insert) sequence.insert(index, *value); else sequence.set(index, *value);
          } else {
            if (insert) sequence.insert(sequence.begin() + static_cast<std::ptrdiff_t>(index), *value);
            else sequence[index] = *value;
          }
          return {};
        }
      }, candidate);
      if (!modified) return failure(modified.error());
    }
    if ((write.access != access_type::input_only || write.mode != write_mode::replace) && previous != latest.end())
      candidates[previous->second].value = std::move(candidate);
    else {
      latest[key] = candidates.size();
      candidates.push_back({write.target, std::move(candidate)});
    }
  }
  return candidates;
}
result<user_data_value> service::user_data(const node &node) const {
  if (auto ok = validate(node); !ok) return failure(ok.error());
  const auto found = p_->metadata.find({node.scene_, node.context_, node.node_, {}});
  return found == p_->metadata.end() ? user_data_value{} : found->second;
}
result<user_data_value> service::user_data(const field &field) const {
  if (auto ok = validate(field); !ok) return failure(ok.error());
  const auto &node = field.node_;
  const auto found = p_->metadata.find({node.scene_, node.context_, node.node_, field.info_.name});
  return found == p_->metadata.end() ? user_data_value{} : found->second;
}
result<void> service::set_user_data(const node &node, user_data_value value) {
  if (auto ok = validate(node); !ok) return ok;
  auto state = p_;
  const impl::metadata_key key{node.scene_, node.context_, node.node_, {}};
  auto found = state->metadata.find(key);
  if (found == state->metadata.end()) {
    if (value.has_value()) state->metadata.emplace(key, std::move(value));
  } else {
    found->second.swap(value);
    if (!found->second.has_value()) state->metadata.erase(found);
  }
  value = {}; // Release displaced payload now, not at the caller full-expression end.
  return {};
}
result<void> service::set_user_data(const field &field, user_data_value value) {
  if (auto ok = validate(field); !ok) return ok;
  auto state = p_;
  const auto &node = field.node_;
  const impl::metadata_key key{node.scene_, node.context_, node.node_, field.info_.name};
  auto found = state->metadata.find(key);
  if (found == state->metadata.end()) {
    if (value.has_value()) state->metadata.emplace(key, std::move(value));
  } else {
    found->second.swap(value);
    if (!found->second.has_value()) state->metadata.erase(found);
  }
  value = {}; // Reentrant cleanup observes the published replacement.
  return {};
}

result<context> service::root_context(const scene &s) const {
  if (auto ok = validate(s); !ok) return failure(ok.error());
  return make<object_kind::context>(s.scene_);
}
result<std::vector<node>> service::roots(const context &c) const {
  if (auto ok = validate(c); !ok) return failure(ok.error());
  auto ids = p_->implementation->do_roots(c.scene_); if (!ids) return failure(ids.error());
  std::vector<node> result;
  for (auto id : *ids) {
    if (!id) return error(error_code::invalid_descriptor, "provider.roots", "backend returned a null root identity");
    result.push_back(make<object_kind::node>(c.scene_, c.context_, id));
  }
  return result;
}
result<node> service::named(const context &c, std::string_view name) const {
  if (auto ok = validate(c); !ok) return failure(ok.error());
  auto id = p_->implementation->do_named(c.scene_, name); if (!id) return failure(id.error());
  if (!*id) return error(error_code::invalid_descriptor, "provider.named", "backend returned a null named identity");
  return make<object_kind::node>(c.scene_, c.context_, *id);
}
result<std::vector<named_node>> service::named_nodes(const context &c) const {
  if (auto ok = validate(c); !ok) return failure(ok.error());
  auto bindings = p_->implementation->do_named_nodes(c.scene_);
  if (!bindings) return failure(bindings.error());
  std::sort(bindings->begin(), bindings->end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });
  std::vector<named_node> result;
  for (const auto &binding : *bindings) {
    if (!binding.node || !valid_name(binding.name) ||
        (!result.empty() && result.back().name == binding.name))
      return error(error_code::invalid_descriptor, "provider.named_nodes", "backend returned an invalid or duplicate name binding");
    result.push_back({binding.name, make<object_kind::node>(c.scene_, c.context_, binding.node)});
  }
  return result;
}
result<std::vector<std::string>> service::names(const node &n) const {
  if (auto ok = validate(n); !ok) return failure(ok.error());
  auto bindings = named_nodes(make<object_kind::context>(n.scene_, n.context_));
  if (!bindings) return failure(bindings.error());
  std::vector<std::string> result;
  for (const auto &binding : *bindings)
    if (binding.target == n) result.push_back(binding.name);
  return result;
}
result<std::string> service::type_name(const node &n) const {
  if (auto ok = validate(n); !ok) return failure(ok.error());
  return p_->implementation->do_type_name(where(n));
}
result<std::vector<field_info>> service::fields(const node &n) const {
  if (auto ok = validate(n); !ok) return failure(ok.error());
  auto descriptors = p_->implementation->do_fields(where(n));
  if (!descriptors) return descriptors;
  std::set<std::string> names;
  for (const auto &field : *descriptors) {
    if (field.name.empty() || !names.insert(field.name).second ||
        static_cast<unsigned>(field.kind) > static_cast<unsigned>(value_kind::mf_vec4f) ||
        static_cast<unsigned>(field.access) > static_cast<unsigned>(access_type::input_output))
      return error(error_code::invalid_descriptor, "provider.fields", "backend field descriptors are not canonical and unique");
  }
  return descriptors;
}
result<field> service::get_field(const node &n, std::string_view name) const {
  auto descriptors = fields(n); if (!descriptors) return failure(descriptors.error());
  auto found = std::find_if(descriptors->begin(), descriptors->end(),
                           [&](const auto &info) { return info.name == name; });
  if (found == descriptors->end())
    found = std::find_if(descriptors->begin(), descriptors->end(), [&](const auto &info) {
      return info.access == access_type::input_output &&
        (name == "set_" + info.name || name == info.name + "_changed");
    });
  if (found != descriptors->end()) {
    field result; result.node_ = n; result.info_ = *found; return result;
  }
  return error(error_code::unknown_field, "provider.field", "unknown field or alias");
}
result<payload> service::read(const field &f) const {
  if (auto ok = validate(f); !ok) return failure(ok.error());
  if (f.info_.access == access_type::input_only)
    return error(error_code::access_denied, "provider.read", "inputOnly field is not readable");
  if (!f.info_.supported) return error(error_code::unsupported_field_type, "provider.read", "backend does not support this field service");
  auto raw = p_->implementation->do_read(where(f));
  if (!raw) return failure(raw.error());
  auto projected = export_value(where(f), *raw);
  if (!projected) return projected;
  if (kind_of(*projected) != f.info_.kind)
    return error(error_code::type_mismatch, "provider.read", "backend value differs from descriptor");
  if (!intrinsic_payload(*projected))
    return error(error_code::invalid_value, "provider.read", "backend payload violates intrinsic representation constraints");
  if (auto valid = validate_scoped_value(f.info_.name, *projected, "provider.read",
      [&] { return p_->implementation->do_type_name(where(f.node_)); }); !valid)
    return failure(valid.error());
  return projected;
}
result<void> service::validate_value(const field &f, const payload &value, write_mode mode) const {
  if (auto ok = validate(f); !ok) return ok;
  if (!f.info_.supported) return error(error_code::unsupported_field_type, "provider.value", "backend does not support this field service");
  if (static_cast<unsigned>(mode) > static_cast<unsigned>(write_mode::clear))
    return error(error_code::invalid_descriptor, "provider.value", "unknown write operation");
  auto expected = f.info_.kind;
  if (mode != write_mode::replace) {
    if (static_cast<int>(expected) < static_cast<int>(value_kind::mf_bool))
      return error(error_code::type_mismatch, "provider.value", "indexed operations require an MF field");
    if (mode == write_mode::remove || mode == write_mode::clear) return {};
    expected = static_cast<value_kind>(static_cast<int>(expected) - static_cast<int>(value_kind::mf_bool));
  }
  if (kind_of(value) != expected) return error(error_code::type_mismatch, "provider.value", "payload kind differs from field or element");
  if (!intrinsic_payload(value)) return error(error_code::invalid_value, "provider.value", "payload violates intrinsic representation constraints");
  if (auto valid = validate_scoped_value(f.info_.name, value, "provider.value",
      [&] { return p_->implementation->do_type_name(where(f.node_)); }); !valid)
    return valid;
  auto raw = import_value(f, value);
  if (!raw) return failure(raw.error());
  return {};
}
result<void> service::author(const field &f, payload value) {
  if (auto ok = validate_value(f, value); !ok) return ok;
  if (p_->scenes.at(f.node_.scene_).state != activation_state::setup ||
      (f.info_.access != access_type::initialize_only && f.info_.access != access_type::input_output))
    return error(error_code::access_denied, "provider.author", "authoring requires setup and an authorable field");
  auto raw = import_value(f, value);
  if (!raw) return failure(raw.error());
  return p_->implementation->do_author(where(f), *raw);
}
result<void> service::add_route(const context &c, const field &from, const field &to) {
  if (auto ok = validate(c); !ok) return ok;
  if (auto ok = validate(from); !ok) return ok;
  if (auto ok = validate(to); !ok) return ok;
  if (from.node_.scene_ != c.scene_ || to.node_.scene_ != c.scene_ ||
      from.node_.context_ != c.context_ || to.node_.context_ != c.context_)
    return error(error_code::invalid_context, "provider.route", "route crosses lexical context");
  if (p_->scenes.at(c.scene_).state != activation_state::setup)
    return error(error_code::access_denied, "provider.route", "route topology is setup-only in this slice");
  const bool output = from.info_.access == access_type::output_only || from.info_.access == access_type::input_output;
  const bool input = to.info_.access == access_type::input_only || to.info_.access == access_type::input_output;
  if (!output || !input || from.info_.kind != to.info_.kind)
    return error(error_code::invalid_route, "provider.route", "route access or kind is incompatible");
  if (!from.info_.supported || !to.info_.supported)
    return error(error_code::unsupported_field_type, "provider.route", "backend does not support this route field service");
  return p_->implementation->do_add_route(where(from), where(to));
}
result<activation> service::activate(const scene &s) {
  if (auto ok = validate(s); !ok) return failure(ok.error());
  if (!p_->facts.supports(service_id::host_turns))
    return error(error_code::unsupported_operation, "provider.activate", "backend does not provide host-driven activation");
  auto &state = p_->scenes.at(s.scene_);
  if (state.state != activation_state::setup) return error(error_code::access_denied, "provider.activate", "scene is not in setup");
  if (auto valid = p_->implementation->do_validate_configuration(s.scene_, {}); !valid)
    return failure(valid.error());
  if (p_->next_epoch == std::numeric_limits<std::uint64_t>::max())
    return error(error_code::resource_limit, "provider.activate", "activation identity exhausted");
  state.epoch = p_->next_epoch++;
  result<void> activated;
  try { activated = p_->implementation->do_activate(s.scene_); }
  catch (const std::exception &e) { activated = error(error_code::callback_failed, "provider.activate", e.what()); }
  catch (...) { activated = error(error_code::callback_failed, "provider.activate", "backend activation threw"); }
  // A failed activation may already own partially initialized native state.
  // Never present it as pristine setup or retry its construction automatically.
  if (!activated) { state.state = activation_state::faulted; return failure(activated.error()); }
  state.state = activation_state::active;
  activation a; a.scene_ = s; a.epoch_ = state.epoch; return a;
}
result<activation_state> service::state(const scene &s) const {
  if (auto ok = check(); !ok) return failure(ok.error());
  if (s.disposed_) return error(error_code::disposed, "provider.state", "wrapper is disposed");
  if (s.owner_.expired() || !s.scene_identity_)
    return error(error_code::stale_handle, "provider.state", "scene authority has expired or is empty");
  if (s.owner_.owner_before(p_->authority) || p_->authority.owner_before(s.owner_))
    return error(error_code::invalid_context, "provider.state", "foreign scene authority");
  const auto found = p_->scenes.find(s.scene_);
  return found == p_->scenes.end() || found->second.identity != s.scene_identity_
      ? activation_state::retired : found->second.state;
}
result<void> service::retire(const activation &a) {
  if (auto ok = validate(a); !ok) return ok;
  return retire(a.scene_);
}
result<void> service::retire(const scene &s) {
  if (auto ok = validate(s); !ok) return ok;
  auto state = p_;
  const auto sid = s.scene_;
  std::vector<user_data_value> released;
  released.reserve(std::count_if(state->metadata.begin(), state->metadata.end(),
      [sid](const auto &entry) { return std::get<0>(entry.first) == sid; }));
  state->scenes.at(sid).state = activation_state::retired;
  state->cancel_scene(sid);
  std::erase_if(state->clients, [sid](const auto &entry) {
    return entry.second.world.scene_.scene_ == sid;
  });
  for (auto &o : state->observers) if (o->world.scene_.scene_ == sid) *o->live = false;
  state->implementation->do_retire(sid);
  state->scenes.erase(sid);
  for (auto it = state->metadata.begin(); it != state->metadata.end();) {
    if (std::get<0>(it->first) == sid) {
      released.push_back(std::move(it->second));
      it = state->metadata.erase(it);
    } else ++it;
  }
  state->discard_notices([sid](const auto &notice) {
    return notice.event.source.node_.scene_ == sid;
  });
  auto released_observers = state->detach_cancelled();
  return {}; // Owning user captures are released only after published teardown.
}
result<client> service::connect(const activation &a) {
  if (auto ok = validate(a); !ok) return failure(ok.error());
  if (p_->scenes.at(a.scene_.scene_).state != activation_state::active)
    return error(error_code::access_denied, "provider.connect", "activation is faulted");
  if (p_->next_client == std::numeric_limits<std::uint64_t>::max())
    return error(error_code::resource_limit, "provider.connect", "client identity exhausted");
  client c; c.activation_ = a; c.id_ = p_->next_client++;
  p_->clients.emplace(c.id_, impl::client_state{a, false, {}}); return c;
}
result<void> service::begin_update(const client &c) {
  if (auto ok = validate(c); !ok) return ok;
  if (p_->scenes.at(c.activation_.scene_.scene_).state != activation_state::active)
    return error(error_code::access_denied, "provider.begin_update", "activation is faulted");
  auto &s = p_->clients.at(c.id_);
  s.holding = true; return {};
}
result<void> service::end_update(const client &c) {
  if (auto ok = validate(c); !ok) return ok;
  if (p_->scenes.at(c.activation_.scene_.scene_).state != activation_state::active)
    return error(error_code::access_denied, "provider.end_update", "activation is faulted");
  auto &client = p_->clients.at(c.id_);
  if (!client.holding) return {};
  if (client.accepted.empty()) { client.holding = false; return {}; }
  // Assemble before changing live ownership. All per-call receipts move into
  // one FIFO submission and receive its shared processing/cancellation result.
  std::vector<queued_write> writes;
  std::vector<std::shared_ptr<ticket::receipt>> receipts;
  std::size_t bytes = 0;
  for (const auto &r : client.accepted) {
    writes.insert(writes.end(), r.writes.begin(), r.writes.end());
    receipts.insert(receipts.end(), r.receipts.begin(), r.receipts.end());
    bytes += r.bytes;
  }
  const auto submission = receipts.front()->id;
  auto outcome = std::make_shared<ticket::receipt::outcome>();
  outcome->status = {request_state::submitted, 0, failure_effect::none, {}, submission};
  auto prepared = prepare(writes);
  result<void> valid;
  if (!prepared) valid = failure(prepared.error());
  else valid = p_->implementation->do_validate_configuration(c.activation_.scene_.scene_, *prepared);
  if (!valid) {
    client.holding = false;
    outcome->status = {request_state::failed, 0, failure_effect::none, valid.error(), submission};
    for (auto &receipt : receipts) {
      std::lock_guard lock(receipt->mutex);
      receipt->shared_outcome = outcome;
    }
    // Publish one shared failure before physically reclaiming each accepted call.
    while (!client.accepted.empty()) {
      auto &request = client.accepted.front();
      for (auto &receipt : request.receipts) receipt->pending = nullptr;
      p_->pending_requests -= request.receipts.size();
      p_->pending_bytes -= request.bytes;
      client.accepted.pop_front();
    }
    return valid;
  }
  auto &submitted = p_->scenes.at(c.activation_.scene_.scene_).submitted;
  submitted.push_back({std::move(receipts), std::move(writes), bytes, &submitted, {}});
  auto &released = submitted.back();
  released.position = std::prev(submitted.end());
  for (auto &receipt : released.receipts) {
    std::lock_guard lock(receipt->mutex);
    receipt->shared_outcome = outcome;
    receipt->pending = &released;
  }
  client.holding = false;
  client.accepted.clear(); // Accounting belongs to the new released submission.
  return {};
}
result<ticket> service::enqueue(const client &c, std::vector<write> writes) {
  if (auto ok = validate(c); !ok) return failure(ok.error());
  if (p_->scenes.at(c.activation_.scene_.scene_).state != activation_state::active)
    return error(error_code::access_denied, "provider.enqueue", "activation is faulted");
  if (writes.empty()) return error(error_code::invalid_value, "provider.enqueue", "batch must contain a write");
  if (p_->pending_requests >= p_->limits.pending_requests)
    return error(error_code::resource_limit, "provider.enqueue", "request admission capacity exhausted");
  if (p_->next_request == std::numeric_limits<std::uint64_t>::max())
    return error(error_code::resource_limit, "provider.enqueue", "request correlation identity exhausted");
  const auto available = p_->limits.payload_bytes - p_->pending_bytes;
  detail::byte_budget budget(available);
  // Account before copying payloads into service-owned storage or mutating a
  // queue/counter. Empty values still pay for their owning write and field name.
  if (!budget.add(writes.size(), sizeof(write)))
    return error(error_code::resource_limit, "provider.enqueue", "payload admission capacity exhausted");
  for (const auto &w : writes)
    if (!budget.add(w.target.info_.name.size()) || !detail::account_dynamic(budget, w.value))
      return error(error_code::resource_limit, "provider.enqueue", "payload admission capacity exhausted");
  std::vector<queued_write> admitted;
  admitted.reserve(writes.size());
  for (const auto &w : writes) {
    if (auto ok = validate_value(w.target, w.value, w.mode); !ok) return failure(ok.error());
    if (w.target.node_.scene_ != c.activation_.scene_.scene_)
      return error(error_code::invalid_context, "provider.enqueue", "write crosses scene authority");
    if (w.target.info_.access != access_type::input_only && w.target.info_.access != access_type::input_output)
      return error(error_code::access_denied, "provider.enqueue", "field is not a live input");
    auto raw = import_value(w.target, w.value);
    if (!raw) return failure(raw.error());
    admitted.push_back({where(w.target), std::move(*raw), w.target.info_.access, w.target.info_.kind, w.mode, w.index});
  }
  auto &owner = p_->clients.at(c.id_);
  // Intrinsic/index validation is per call. Coupled constraints can be repaired
  // by subsequent held calls and are checked only at release/execution.
  std::vector<queued_write> candidate;
  if (owner.holding)
    for (const auto &pending : owner.accepted)
      candidate.insert(candidate.end(), pending.writes.begin(), pending.writes.end());
  candidate.insert(candidate.end(), admitted.begin(), admitted.end());
  auto prepared = prepare(candidate);
  if (!prepared) return failure(prepared.error());
  if (!owner.holding)
    if (auto valid = p_->implementation->do_validate_configuration(c.activation_.scene_.scene_, *prepared); !valid)
      return failure(valid.error());
  ticket result;
  result.receipt_ = std::make_shared<ticket::receipt>();
  auto &receipt = *result.receipt_;
  receipt.authority = p_->authority;
  receipt.id = p_->next_request;
  receipt.client = c.id_;
  receipt.status.state = owner.holding ? request_state::accepted : request_state::submitted;
  receipt.status.submission = owner.holding ? 0 : receipt.id;
  auto &queue = owner.holding ? owner.accepted : p_->scenes.at(c.activation_.scene_.scene_).submitted;
  const auto bytes = available - budget.remaining();
  queue.push_back({{result.receipt_}, std::move(admitted), bytes, &queue, {}});
  auto &r = queue.back();
  r.position = std::prev(queue.end());
  receipt.pending = &r;
  ++p_->next_request;
  ++p_->pending_requests;
  p_->pending_bytes += bytes; // Proven <= limits.payload_bytes by byte_budget.
  return result;
}
result<void> service::cancel(const client &c, const ticket &t) {
  if (auto ok = validate(c); !ok) return ok;
  if (!t.receipt_ || t.receipt_->authority.lock() != p_->authority ||
      t.receipt_->client != c.id_)
    return error(error_code::invalid_context, "provider.cancel", "receipt belongs to another client or service");
  if (!t.receipt_->pending)
    return error(error_code::access_denied, "provider.cancel", "request is already terminal");
  auto &r = *static_cast<impl::request *>(t.receipt_->pending);
  p_->finish(r, {request_state::cancelled, 0, failure_effect::none, {}});
  return {};
}
result<void> service::disconnect(const client &c) {
  if (auto ok = validate(c); !ok) return ok;
  auto &owner = p_->clients.at(c.id_);
  p_->cancel_all(owner.accepted);
  auto &submitted = p_->scenes.at(c.activation_.scene_.scene_).submitted;
  for (auto it = submitted.begin(); it != submitted.end();) {
    auto &r = *it++;
    if (r.receipts.front()->client == c.id_) p_->finish(r, {request_state::cancelled, 0, failure_effect::none, {}});
  }
  p_->clients.erase(c.id_);
  return {};
}
result<turn_report> service::pump(const activation &a, event_time time) {
  if (auto ok = validate(a); !ok) return failure(ok.error());
  const auto sid = a.scene_.scene_;
  auto &s = p_->scenes.at(sid);
  if (s.state != activation_state::active)
    return error(error_code::access_denied, "provider.pump", "faulted activation must be retired, not retried");
  if (!std::isfinite(time.seconds)) return error(error_code::invalid_value, "provider.pump", "host time must be finite");
  if (s.time && time.seconds < *s.time) return error(error_code::event_time_regression, "provider.pump", "host time cannot decrease");
  auto state = p_;
  auto released_observers = state->detach_cancelled();
  const bool observed = std::any_of(state->observers.begin(), state->observers.end(), [&](const auto &o) {
    return o->world == a;
  });
  // Known exhausted capacity is rejected before backend entry and leaves the
  // queued submission available for retry after the host drains notifications.
  if (observed && (state->notices.size() >= state->limits.queued_notifications ||
      state->limits.notification_bytes - state->notification_bytes < sizeof(notification)))
    return error(error_code::resource_limit, "provider.pump", "notification queue needs draining before another observed turn");
  impl::request *request = s.submitted.empty() ? nullptr : &s.submitted.front();
  const std::optional<std::uint64_t> request_id = request ?
      std::optional{request->receipts.front()->id} : std::nullopt;
  if (s.turn == std::numeric_limits<std::uint64_t>::max())
    return error(error_code::resource_limit, "provider.pump", "logical turn identity exhausted");
  ++s.turn; s.time = time.seconds;
  turn_report report; report.turn = s.turn; report.time = time; report.request = request_id;
  p_->pumping = true; p_->pumping_scene = sid; p_->pumping_time = time;
  p_->capture_error.reset();
  const auto notices_before = p_->notices.size();
  backend_result done;
  bool entered_turn = false;
  try {
    const std::vector<queued_write> empty;
    auto writes = prepare(request ? request->writes : empty);
    if (!writes) done = tl::make_unexpected(backend_failure{writes.error(), failure_effect::none});
    else if (auto valid = p_->implementation->do_validate_configuration(sid, *writes); !valid)
      done = tl::make_unexpected(backend_failure{valid.error(), failure_effect::none});
    else { entered_turn = true; done = p_->implementation->do_turn(sid, time, *writes); }
  }
  catch (const std::exception &e) { done = tl::make_unexpected(backend_failure{error(error_code::callback_failed, "provider.pump", e.what()).value(), entered_turn ? failure_effect::partial_or_unknown : failure_effect::none}); }
  catch (...) { done = tl::make_unexpected(backend_failure{error(error_code::callback_failed, "provider.pump", "backend threw").value(), entered_turn ? failure_effect::partial_or_unknown : failure_effect::none}); }
  if (done && p_->capture_error)
    done = tl::make_unexpected(backend_failure{*p_->capture_error, failure_effect::partial_or_unknown});
  p_->pumping = false; p_->pumping_scene = 0;
  if (!done) {
    report.effects = done.error().effects; report.error = done.error().error;
    if (report.effects == failure_effect::partial_or_unknown) s.state = activation_state::faulted;
    report.state = s.state;
    while (p_->notices.size() > notices_before) {
      p_->notification_bytes -= p_->notices.back().bytes;
      p_->notices.pop_back();
    }
    if (request) p_->finish(*request, {request_state::failed, s.turn, report.effects, report.error});
    if (s.state == activation_state::faulted) p_->cancel_scene(sid);
  } else if (request) p_->finish(*request, {request_state::processed, s.turn, failure_effect::none, {}});
  return report;
}
result<subscription> service::observe(const activation &a, const field &f, std::function<void(const notification &)> callback) {
  if (auto ok = validate(a); !ok) return failure(ok.error());
  if (auto ok = validate(f); !ok) return failure(ok.error());
  if (a.scene_.scene_ != f.node_.scene_) return error(error_code::invalid_context, "provider.observe", "field belongs to another scene");
  if (p_->scenes.at(a.scene_.scene_).state != activation_state::active || f.info_.access == access_type::input_only || f.info_.access == access_type::initialize_only)
    return error(error_code::access_denied, "provider.observe", "field is not an observable output");
  if (!f.info_.supported) return error(error_code::unsupported_field_type, "provider.observe", "backend does not support this field service");
  if (!p_->facts.supports(service_id::notifications))
    return error(error_code::unsupported_operation, "provider.observe", "backend does not provide event observation");
  if (!callback) return error(error_code::invalid_value, "provider.observe", "callback is empty");
  auto state = p_;
  auto released = state->detach_cancelled();
  if (state->observers.size() >= state->limits.live_registrations)
    return error(error_code::resource_limit, "provider.observe", "live registration capacity exhausted");
  auto o = std::make_shared<impl::observer>(); o->world = a; o->source = f;
  o->live = std::make_shared<std::atomic_bool>(true); o->callback = std::make_shared<const std::function<void(const notification &)>>(std::move(callback));
  p_->observers.push_back(o); return subscription{o->live};
}
void service::record_event(const std::weak_ptr<impl> &weak, const address &addr, backend_value raw) {
  const auto state = weak.lock();
  if (!state || state->owner != thread_identity() || state->closed || !state->pumping || state->capture_error || addr.scene != state->pumping_scene) return;
  const auto projected = std::visit([&](const auto &value) -> result<payload> {
    using T = std::decay_t<decltype(value)>;
    const auto wrap = [&](node_id id) -> result<node_value> {
      if (!id.value) return node_value{};
      auto type = state->implementation->do_type_name({addr.scene, addr.context, id.value, {}});
      if (!type) return failure(type.error());
      node n; n.owner_ = state->authority; n.scene_ = addr.scene;
      n.scene_identity_ = state->scenes.at(addr.scene).identity;
      n.context_ = addr.context; n.node_ = id.value; return node_value{n};
    };
    if constexpr (std::same_as<T, node_id>) {
      auto wrapped = wrap(value); if (!wrapped) return failure(wrapped.error());
      return payload{std::move(*wrapped)};
    } else if constexpr (std::same_as<T, node_list>) {
      node_values values; values.reserve(value.size());
      for (auto id : value) {
        auto wrapped = wrap(id); if (!wrapped) return failure(wrapped.error());
        values.push_back(std::move(*wrapped));
      }
      return payload{std::move(values)};
    } else return payload{value};
  }, raw);
  if (!projected) { state->capture_error = projected.error(); return; }
  if (!intrinsic_payload(*projected)) {
    state->capture_error = error(error_code::invalid_value, "provider.event", "backend event violates intrinsic representation constraints").value();
    return;
  }
  if (auto valid = validate_scoped_value(addr.field, *projected, "provider.event",
      [&] { return state->implementation->do_type_name(addr); }); !valid) {
    state->capture_error = valid.error(); return;
  }
  const auto &s = state->scenes.at(addr.scene);
  for (const auto &o : state->observers) {
    if (!*o->live || o->world.epoch_ != s.epoch) continue;
    const auto source = where(o->source);
    if (source.scene == addr.scene && source.context == addr.context && source.node == addr.node && source.field == addr.field) {
      if (kind_of(*projected) != o->source.info_.kind) {
        state->capture_error = error(error_code::type_mismatch, "provider.event", "backend event payload differs from canonical field descriptor").value();
        return;
      }
      detail::byte_budget budget(state->limits.notification_bytes - state->notification_bytes);
      if (state->notices.size() >= state->limits.queued_notifications ||
          !budget.add(1, sizeof(notification)) || !budget.add(o->source.info_.name.size()) ||
          !detail::account_dynamic(budget, *projected)) {
        state->capture_error = error(error_code::resource_limit, "provider.event", "notification admission capacity exhausted during backend execution").value();
        return;
      }
      const auto bytes = state->limits.notification_bytes - state->notification_bytes - budget.remaining();
      state->notices.push_back({notification{o->source, *projected, s.epoch, s.turn, state->pumping_time}, o, bytes});
      state->notification_bytes += bytes;
    }
  }
}
result<std::size_t> service::dispatch_notifications() {
  if (auto ok = check(); !ok) return failure(ok.error());
  auto state = p_;
  if (state->dispatching) return error(error_code::operation_in_progress, "provider.dispatch", "recursive dispatch is not supported");
  // Pin detached delivery/state. An application callback may close or destroy
  // the service, so there is intentionally no access through this from here.
  auto notices = std::move(state->notices); state->notices.clear();
  state->dispatching = true;
  state->notification_bytes = 0;
  std::size_t delivered = 0; bool threw = false;
  for (auto &n : notices) {
    auto o = n.target.lock(); if (!o || !o->live->load() || state->closed) continue;
    const auto found = state->scenes.find(o->world.scene_.scene_);
    if (found == state->scenes.end() ||
        (found->second.state != activation_state::active &&
         found->second.state != activation_state::faulted)) continue;
    try { (*o->callback)(n.event); ++delivered; } catch (...) { threw = true; }
  }
  state->dispatching = false;
  // Publish the retained set before releasing callbacks. Capture destructors
  // are application code too and may reenter or destroy the service.
  auto released = state->detach_cancelled();
  released.clear();
  if (threw) return error(error_code::callback_failed, "provider.dispatch", "one or more application callbacks threw; other callbacks were drained");
  return delivered;
}

result<void> service::close() {
  if (!p_) return {};
  if (p_->owner != thread_identity())
    return error(error_code::wrong_thread, "provider.close", "service belongs to another thread");
  if (p_->pumping)
    return error(error_code::operation_in_progress, "provider.close", "backend turn is in progress");
  if (p_->closed) return {};
  auto state = p_;
  // A deque move may allocate its replacement storage. Finish that before
  // publishing closed state so failure leaves the service available to retry.
  auto notices = std::move(state->notices);
  state->closed = true;
  for (auto &[id, s] : state->scenes) state->cancel_all(s.submitted);
  for (auto &[id, c] : state->clients) state->cancel_all(c.accepted);
  state->clients.clear();
  state->authority.reset();
  for (auto &o : state->observers) o->live->store(false);
  auto metadata = std::move(state->metadata);
  auto observers = std::move(state->observers);
  state->notices.clear(); state->observers.clear();
  state->notification_bytes = 0;
  for (auto &[id, s] : state->scenes) if (s.state != activation_state::retired) {
    s.state = activation_state::retired; state->implementation->do_retire(id);
  }
  state->scenes.clear();
  state->implementation->events_ = {};
  state->implementation.reset();
  // Only after the final use of this may owning captures be released. Their
  // destructors can call close(), query handles, or destroy their owner.
  return {};
}

} // namespace x3d::sai::experimental::provider
