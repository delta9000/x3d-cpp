#include "x3d/sai/experimental/session.hpp"
#include "x3d/sai/experimental/image.hpp"

#include <algorithm>
#include <atomic>
#include <deque>
#include <list>
#include <mutex>
#include <unordered_set>

namespace x3d::sai::experimental {
namespace detail {
struct update_ticket_state {
  mutable std::mutex mutex;
  std::uint64_t id = 0;
  update_progress progress;
};
struct external_session_host_state {
  explicit external_session_host_state(execution_context scene)
      : context(std::move(scene)) {}
  execution_context context;
  mutable std::mutex mutex;
  bool processing = false;
  std::vector<std::weak_ptr<external_session::impl>> clients;
};
} // namespace detail

std::uint64_t update_ticket::id() const noexcept { return state_->id; }
update_progress update_ticket::progress() const {
  std::lock_guard lock(state_->mutex);
  return state_->progress;
}

namespace {
std::atomic<std::uint64_t> next_submission{1};
sai_error session_error(error_code code, const char *operation,
                        std::string message) {
  sai_error error;
  error.code = code;
  error.operation = operation;
  error.message = std::move(message);
  return error;
}
// Construct an empty MF by its exact kind without reading its current value.
// This also keeps clear available for writable, unreadable inputOnly fields.
result<value> empty_multi_value(value_kind kind, const char *operation) {
  std::optional<value> empty;
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    ([&] {
      using T = std::variant_alternative_t<I, value>;
      if constexpr (multi_field_sequence<T>)
        if (value_traits<T>::kind == kind) empty.emplace(T{});
    }(), ...);
  }(std::make_index_sequence<std::variant_size_v<value>>{});
  if (empty) return std::move(*empty);
  return failure(session_error(error_code::type_mismatch, operation,
                                "field is not a multi-valued field"));
}

using payload = std::variant<value, node, std::vector<node>>;
struct accepted_call {
  std::shared_ptr<detail::update_ticket_state> ticket;
  std::shared_ptr<update_callback> callback;
};
struct queued_write { dynamic_field target; payload data; };
struct submission {
  std::uint64_t sequence = 0;
  std::vector<queued_write> writes;
  std::vector<node_id> disposals;
  std::vector<accepted_call> calls;
};
struct queued_callback { std::shared_ptr<update_callback> callback; update_completion completion; };

bool same_field(const dynamic_field &a, const dynamic_field &b) {
  return a.node() == b.node() && a.name() == b.name();
}
result<void> send_payload(event_batch &batch, const queued_write &write) {
  return std::visit([&](const auto &data) -> result<void> {
    using T = std::decay_t<decltype(data)>;
    if constexpr (std::same_as<T, std::vector<node>>)
      return batch.send(write.target, std::span<const node>{data});
    else
      return batch.send(write.target, data);
  }, write.data);
}
void set_progress(const accepted_call &call, update_progress progress) {
  std::lock_guard lock(call.ticket->mutex);
  call.ticket->progress = std::move(progress);
}
} // namespace

struct external_session::impl {
  explicit impl(execution_context scene) : context(std::move(scene)) {}
  ~impl() {
    // Outstanding tickets remain inspectable after their session is destroyed.
    try {
      auto error = session_error(error_code::cancelled, "external_session.destroy",
                                 "session destroyed before queued work was processed");
      for (auto &call : buffered.calls)
        set_progress(call, {update_stage::failed, {}, error, {}});
      for (auto &item : ready)
        for (auto &call : item.calls)
          set_progress(call, {update_stage::failed, {}, error, item.sequence});
    } catch (...) {} // Destruction never calls user code or propagates failures.
  }

  execution_context context;
  std::weak_ptr<detail::external_session_host_state> host;
  mutable std::mutex mutex;
  bool gate = false;
  bool is_closed = false;
  bool processing = false;
  bool draining = false;
  std::uint64_t next_ticket = 1;
  submission buffered;
  std::list<submission> ready;
  std::list<queued_callback> callbacks;
  std::unordered_set<std::uint64_t> disposed;

  result<void> available(const char *operation) const {
    if (is_closed)
      return failure(session_error(error_code::disposed, operation,
                                    "external session is closed"));
    if (processing)
      return failure(session_error(error_code::operation_in_progress, operation,
                                    "this session is processing a submission"));
    return context.validate();
  }
  result<node_type_descriptor> node_descriptor(const node &owner,
                                               const char *operation) const {
    auto descriptor = context.snapshot().describe(owner);
    if (!descriptor) return failure(descriptor.error());
    if (disposed.contains(owner.id().value))
      return failure(session_error(error_code::disposed, operation,
                                    "node was disposed by this session"));
    return descriptor;
  }
  result<void> usable(const dynamic_field &target, const char *operation) const {
    auto batch = context.events(event_time{});
    auto writable = batch.writable(target, write_intent::event_input);
    if (!writable) return failure(writable.error());
    if (!*writable)
      return failure(session_error(error_code::access_denied, operation,
                                    "field does not accept external input"));
    return {};
  }
  result<void> validate_write(const queued_write &write) const {
    auto valid = usable(write.target, "external_session.set_value");
    if (!valid) return valid;
    // A scratch batch isolates each failed setter from previous accepted calls.
    auto batch = context.events(event_time{});
    return send_payload(batch, write);
  }
  const payload *pending_value(const dynamic_field &target) const {
    auto find = [&](const submission &item) -> const payload * {
      for (auto it = item.writes.rbegin(); it != item.writes.rend(); ++it)
        if (same_field(it->target, target)) return &it->data;
      return nullptr;
    };
    if (auto value = find(buffered)) return value;
    for (auto it = ready.rbegin(); it != ready.rend(); ++it)
      if (auto value = find(*it)) return value;
    return nullptr;
  }
  void mark_submitted(submission &item) {
    item.sequence = next_submission.fetch_add(1, std::memory_order_relaxed);
    for (auto &call : item.calls)
      set_progress(call, {update_stage::submitted, {}, {}, item.sequence});
  }
  accepted_call make_call(std::shared_ptr<update_callback> callback) {
    auto ticket = std::make_shared<detail::update_ticket_state>();
    ticket->id = next_ticket++;
    ticket->progress.stage = update_stage::accepted;
    return {std::move(ticket), std::move(callback)};
  }
  result<update_ticket> accept(queued_write write,
                               const std::shared_ptr<update_callback> &callback) {
    auto valid = validate_write(write);
    if (!valid) return failure(valid.error());
    auto call = make_call(callback);
    update_ticket ticket{call.ticket};
    if (gate) {
      // Complete all potentially allocating capacity growth before replacing
      // a buffered value: a failed acceptance leaves prior work unchanged.
      buffered.calls.reserve(buffered.calls.size() + 1);
      buffered.writes.reserve(buffered.writes.size() + 1);
      static_assert(std::is_nothrow_move_assignable_v<payload>);
      auto found = std::find_if(buffered.writes.begin(), buffered.writes.end(),
          [&](const queued_write &old) { return same_field(old.target, write.target); });
      if (found == buffered.writes.end()) buffered.writes.push_back(std::move(write));
      else found->data = std::move(write.data);
      buffered.calls.push_back(std::move(call));
    } else {
      submission item;
      item.writes.push_back(std::move(write));
      item.calls.push_back(std::move(call));
      ready.push_back(std::move(item));
      mark_submitted(ready.back());
    }
    return ticket;
  }
  void flush() {
    if (!buffered.calls.empty()) {
      // Allocate the queue node before moving or changing receipt state.
      ready.emplace_back();
      ready.back() = std::move(buffered);
      buffered = submission{};
      mark_submitted(ready.back());
    }
    gate = false;
  }
};

external_session::external_session(execution_context context)
    : impl_(std::make_shared<impl>(std::move(context))) {}
external_session::external_session(external_session &&) noexcept = default;
external_session &external_session::operator=(external_session &&) noexcept = default;
external_session::~external_session() = default;

result<void> external_session::begin_update() {
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.begin_update");
  if (!valid) return valid;
  impl_->gate = true;
  return {};
}
result<void> external_session::end_update() {
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.end_update");
  if (!valid) return valid;
  if (impl_->gate) impl_->flush();
  return {};
}
result<void> external_session::close() {
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  if (impl_->is_closed) return {};
  // Closing a retired context still releases its buffer: host processing then
  // reports stale_handle for every accepted call rather than losing tickets.
  if (impl_->processing)
    return failure(session_error(error_code::operation_in_progress,
        "external_session.close", "this session is processing a submission"));
  impl_->flush();
  impl_->is_closed = true;
  return {};
}
bool external_session::buffering() const {
  std::lock_guard lock(impl_->mutex); return impl_->gate;
}
bool external_session::closed() const {
  std::lock_guard lock(impl_->mutex); return impl_->is_closed;
}
std::size_t external_session::pending_submissions() const {
  std::lock_guard lock(impl_->mutex); return impl_->ready.size();
}
std::optional<std::uint64_t> external_session::next_submission_sequence() const {
  std::lock_guard lock(impl_->mutex);
  if (impl_->ready.empty()) return std::nullopt;
  return impl_->ready.front().sequence;
}
result<std::string> external_session::get_type_name(const node &owner) const {
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.get_type_name");
  if (!valid) return failure(valid.error());
  auto descriptor = impl_->node_descriptor(owner, "external_session.get_type_name");
  if (!descriptor) return failure(descriptor.error());
  return descriptor->name;
}
result<std::vector<field_descriptor>> external_session::get_field_definitions(
    const node &owner) const {
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.get_field_definitions");
  if (!valid) return failure(valid.error());
  auto descriptor = impl_->node_descriptor(owner, "external_session.get_field_definitions");
  if (!descriptor) return failure(descriptor.error());
  return std::move(descriptor->fields);
}
result<dynamic_field> external_session::get_field(
    const node &owner, const std::string &name) const {
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.get_field");
  if (!valid) return failure(valid.error());
  auto descriptor = impl_->node_descriptor(owner, "external_session.get_field");
  if (!descriptor) return failure(descriptor.error());
  // Reuse live lookup's inputOutput set_/_changed canonicalization after
  // validating this session's ownership and local disposal state.
  return owner.get_field(name);
}
result<bool> external_session::is_realized(const node &owner) const {
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.is_realized");
  if (!valid) return failure(valid.error());
  auto descriptor = impl_->node_descriptor(owner, "external_session.is_realized");
  if (!descriptor) return failure(descriptor.error());
  return impl_->context.snapshot().is_realized(owner);
}
result<value> external_session::get_value(const dynamic_field &source) const {
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.get_value");
  if (!valid) return failure(valid.error());
  auto read = impl_->context.snapshot().read(source);
  if (!read) return failure(read.error());
  return read;
}
result<std::size_t> external_session::get_size(
    const dynamic_field &source) const {
  auto read = get_value(source);
  if (!read) return failure(read.error());
  return std::visit([](const auto &items) -> result<std::size_t> {
    using T = std::decay_t<decltype(items)>;
    if constexpr (multi_field_sequence<T>) return items.size();
    else return failure(session_error(error_code::type_mismatch,
        "external_session.get_size", "field is not a multi-valued field"));
  }, *read);
}
result<value> external_session::get_one_value(
    const dynamic_field &source, std::size_t index) const {
  auto read = get_value(source);
  if (!read) return failure(read.error());
  return std::visit([&](const auto &items) -> result<value> {
    using T = std::decay_t<decltype(items)>;
    if constexpr (multi_field_sequence<T> && !std::same_as<T, node_list>) {
      if (index >= items.size())
        return failure(session_error(error_code::index_out_of_range,
            "external_session.get_one_value", "MF index is out of range"));
      return value{items[index]};
    } else return failure(session_error(error_code::type_mismatch,
        "external_session.get_one_value",
        "field is not a non-node MF field; use get_one_node for MFNode"));
  }, *read);
}
result<node> external_session::get_one_node(
    const dynamic_field &source, std::size_t index) const {
  auto state = impl_;
  std::lock_guard lock(state->mutex);
  auto valid = state->available("external_session.get_one_node");
  if (!valid) return failure(valid.error());
  auto read = state->context.snapshot().read_nodes(source);
  if (!read) return failure(read.error());
  if (index >= read->size())
    return failure(session_error(error_code::index_out_of_range,
        "external_session.get_one_node", "MFNode index is out of range"));
  return std::move((*read)[index]);
}
result<update_ticket> external_session::set_value(
    const dynamic_field &target, value data, update_callback callback) {
  // Keep an outer callback owner until after all locks have unwound, including
  // validation/allocator failures. Capture destruction may reenter the session.
  auto owned_callback = callback
      ? std::make_shared<update_callback>(std::move(callback)) : nullptr;
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.set_value");
  if (!valid) return failure(valid.error());
  return impl_->accept({target, payload{std::move(data)}}, owned_callback);
}
result<update_ticket> external_session::set_value(
    const dynamic_field &target, const node &data, update_callback callback) {
  // Keep an outer callback owner until after all locks have unwound, including
  // validation/allocator failures. Capture destruction may reenter the session.
  auto owned_callback = callback
      ? std::make_shared<update_callback>(std::move(callback)) : nullptr;
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.set_value");
  if (!valid) return failure(valid.error());
  return impl_->accept({target, payload{data}}, owned_callback);
}
result<update_ticket> external_session::set_value(
    const dynamic_field &target, std::span<const node> data, update_callback callback) {
  // Keep an outer callback owner until after all locks have unwound, including
  // validation/allocator failures. Capture destruction may reenter the session.
  auto owned_callback = callback
      ? std::make_shared<update_callback>(std::move(callback)) : nullptr;
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.set_value");
  if (!valid) return failure(valid.error());
  return impl_->accept({target, payload{std::vector<node>{data.begin(), data.end()}}},
                        owned_callback);
}
result<update_ticket> external_session::set_one_value(
    const dynamic_field &target, std::size_t index, value element,
    update_callback callback) {
  return mutate_multi_field(target, multi_mutation::set, index, &element,
                             nullptr, std::move(callback));
}
result<update_ticket> external_session::set_one_value(
    const dynamic_field &target, std::size_t index, const node &element,
    update_callback callback) {
  return mutate_multi_field(target, multi_mutation::set, index, nullptr,
                             &element, std::move(callback));
}
result<update_ticket> external_session::append(
    const dynamic_field &target, value element, update_callback callback) {
  return mutate_multi_field(target, multi_mutation::append, 0, &element,
                             nullptr, std::move(callback));
}
result<update_ticket> external_session::append(
    const dynamic_field &target, const node &element, update_callback callback) {
  return mutate_multi_field(target, multi_mutation::append, 0, nullptr,
                             &element, std::move(callback));
}
result<update_ticket> external_session::insert_value(
    const dynamic_field &target, std::size_t index, value element,
    update_callback callback) {
  return mutate_multi_field(target, multi_mutation::insert, index, &element,
                             nullptr, std::move(callback));
}
result<update_ticket> external_session::insert_value(
    const dynamic_field &target, std::size_t index, const node &element,
    update_callback callback) {
  return mutate_multi_field(target, multi_mutation::insert, index, nullptr,
                             &element, std::move(callback));
}
result<update_ticket> external_session::remove(
    const dynamic_field &target, std::size_t index, update_callback callback) {
  return mutate_multi_field(target, multi_mutation::remove, index, nullptr,
                             nullptr, std::move(callback));
}
result<update_ticket> external_session::clear(
    const dynamic_field &target, update_callback callback) {
  return mutate_multi_field(target, multi_mutation::clear, 0, nullptr,
                             nullptr, std::move(callback));
}
result<update_ticket> external_session::mutate_multi_field(
    const dynamic_field &target, multi_mutation mutation, std::size_t index,
    const value *element, const node *node_element, update_callback callback) {
  const char *operation = "external_session.set_one_value";
  switch (mutation) {
    case multi_mutation::set: break;
    case multi_mutation::append: operation = "external_session.append"; break;
    case multi_mutation::insert: operation = "external_session.insert_value"; break;
    case multi_mutation::remove: operation = "external_session.remove"; break;
    case multi_mutation::clear: operation = "external_session.clear"; break;
  }
  // Pin storage and keep callback ownership outside both locks. Even a rejected
  // call can destroy a capture that moves or destroys the owning session.
  auto state = impl_;
  auto owned_callback = callback
      ? std::make_shared<update_callback>(std::move(callback)) : nullptr;
  auto host = state->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(state->mutex);
  auto valid = state->available(operation);
  if (!valid) return failure(valid.error());
  valid = state->usable(target, operation);
  if (!valid) return failure(valid.error());
  auto empty = empty_multi_value(target.kind(), operation);
  if (!empty) return failure(empty.error());

  auto check_index = [&](std::size_t size) -> result<void> {
    if (mutation == multi_mutation::set || mutation == multi_mutation::remove) {
      if (index < size) return {};
    } else if (mutation != multi_mutation::insert || index <= size) return {};
    return failure(session_error(error_code::index_out_of_range, operation,
                                  "MF index is out of range"));
  };
  // Mutate only a local copy. Kernel admission of the complete candidate then
  // validates format, range, node scope/type/uniqueness without poisoning any
  // previously accepted call. Node ownership never passes through numeric IDs.
  if (target.kind() == value_kind::mf_node) {
    if (element)
      return failure(session_error(error_code::invalid_context, operation,
                                    "node elements require node handles"));
    std::vector<node> sequence;
    if (mutation != multi_mutation::clear) {
      if (const auto *pending = state->pending_value(target)) {
        sequence = std::get<std::vector<node>>(*pending);
      } else {
        auto read = state->context.snapshot().read_nodes(target);
        if (!read) return failure(read.error());
        sequence = std::move(*read);
      }
    }
    valid = check_index(sequence.size());
    if (!valid) return failure(valid.error());
    switch (mutation) {
      case multi_mutation::set: sequence[index] = *node_element; break;
      case multi_mutation::append: sequence.push_back(*node_element); break;
      case multi_mutation::insert:
        sequence.insert(sequence.begin() + static_cast<std::ptrdiff_t>(index),
                        *node_element); break;
      case multi_mutation::remove:
        sequence.erase(sequence.begin() + static_cast<std::ptrdiff_t>(index)); break;
      case multi_mutation::clear: break;
    }
    return state->accept({target, payload{std::move(sequence)}}, owned_callback);
  }
  if (node_element)
    return failure(session_error(error_code::type_mismatch, operation,
                                  "node element requires MFNode field"));
  value sequence = std::move(*empty);
  if (mutation != multi_mutation::clear) {
    if (const auto *pending = state->pending_value(target)) {
      sequence = std::get<value>(*pending);
    } else {
      auto read = state->context.snapshot().read(target);
      if (!read) return failure(read.error());
      sequence = std::move(*read);
    }
  }
  auto changed = std::visit([&](auto &items) -> result<void> {
    using T = std::decay_t<decltype(items)>;
    if constexpr (multi_field_sequence<T> && !std::same_as<T, node_list>) {
      using E = typename detail::multi_sequence_traits<T>::element_type;
      const auto *replacement = element ? std::get_if<E>(element) : nullptr;
      if (element && !replacement)
        return failure(session_error(error_code::type_mismatch, operation,
                                      "element kind does not match MF field"));
      auto checked = check_index(items.size());
      if (!checked) return checked;
      switch (mutation) {
        case multi_mutation::set:
          if constexpr (std::same_as<T, bool_list>) items.set(index, *replacement);
          else items[index] = *replacement;
          break;
        case multi_mutation::append: items.push_back(*replacement); break;
        case multi_mutation::insert:
          if constexpr (std::same_as<T, bool_list>) items.insert(index, *replacement);
          else items.insert(items.begin() + static_cast<std::ptrdiff_t>(index),
                            *replacement);
          break;
        case multi_mutation::remove:
          if constexpr (std::same_as<T, bool_list>) items.erase(index);
          else items.erase(items.begin() + static_cast<std::ptrdiff_t>(index));
          break;
        case multi_mutation::clear: break;
      }
      return {};
    } else return failure(session_error(error_code::type_mismatch, operation,
                                         "field is not a non-node MF field"));
  }, sequence);
  if (!changed) return failure(changed.error());
  return state->accept({target, payload{std::move(sequence)}}, owned_callback);
}
result<update_ticket> external_session::set_sub_image(
    const dynamic_field &target, const image &source, std::int32_t width,
    std::int32_t height, std::int32_t source_x, std::int32_t source_y,
    std::int32_t destination_x, std::int32_t destination_y,
    update_callback callback) {
  auto owned_callback = callback
      ? std::make_shared<update_callback>(std::move(callback)) : nullptr;
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.set_sub_image");
  if (!valid) return failure(valid.error());
  valid = impl_->usable(target, "external_session.set_sub_image");
  if (!valid) return failure(valid.error());
  if (target.kind() != value_kind::sf_image)
    return failure(session_error(error_code::type_mismatch,
        "external_session.set_sub_image", "subimage destination must be SFImage"));
  image destination;
  if (const auto *pending = impl_->pending_value(target)) {
    destination = std::get<image>(std::get<value>(*pending));
  } else {
    auto read = impl_->context.snapshot().read(target);
    if (!read) return failure(read.error());
    destination = std::get<image>(std::move(*read));
  }
  auto patched = with_sub_image(destination, source, width, height, source_x,
                                 source_y, destination_x, destination_y);
  if (!patched) return failure(patched.error());
  return impl_->accept({target, payload{value{std::move(*patched)}}}, owned_callback);
}
result<update_ticket> external_session::dispose_node(
    const node &owner, update_callback callback) {
  // Keep an outer callback owner until after all locks have unwound, including
  // validation/allocator failures. Capture destruction may reenter the session.
  auto owned_callback = callback
      ? std::make_shared<update_callback>(std::move(callback)) : nullptr;
  auto host = impl_->host.lock();
  std::unique_lock<std::mutex> order_lock;
  if (host) order_lock = std::unique_lock{host->mutex};
  std::lock_guard lock(impl_->mutex);
  auto valid = impl_->available("external_session.dispose_node");
  if (!valid) return failure(valid.error());
  auto descriptor = impl_->context.snapshot().describe(owner);
  if (!descriptor) return failure(descriptor.error());
  auto call = impl_->make_call(owned_callback);
  update_ticket ticket{call.ticket};
  if (impl_->gate) {
    impl_->buffered.calls.reserve(impl_->buffered.calls.size() + 1);
    impl_->buffered.disposals.reserve(impl_->buffered.disposals.size() + 1);
    if (std::find(impl_->buffered.disposals.begin(), impl_->buffered.disposals.end(),
                  owner.id()) == impl_->buffered.disposals.end())
      impl_->buffered.disposals.push_back(owner.id());
    impl_->buffered.calls.push_back(std::move(call));
  } else {
    submission item;
    item.disposals.push_back(owner.id());
    item.calls.push_back(std::move(call));
    impl_->ready.push_back(std::move(item));
    impl_->mark_submitted(impl_->ready.back());
  }
  return ticket;
}

result<session_process_report> external_session::process_next(event_time time) {
  if (!impl_->host.expired())
    return failure(session_error(error_code::unsupported_operation,
        "external_session.process_next", "managed clients are processed by their session host"));
  return process_submission(time);
}
result<session_process_report> external_session::process_submission(event_time time) {
  auto state = impl_; // Pin storage across arbitrary runtime/application callbacks.
  std::list<submission> in_flight;
  {
    std::lock_guard lock(state->mutex);
    if (state->processing)
      return failure(session_error(error_code::operation_in_progress,
          "external_session.process_next", "this session is already processing"));
    if (state->ready.empty()) return session_process_report{};
    in_flight.splice(in_flight.end(), state->ready, state->ready.begin());
    state->processing = true;
  }
  // Resource failures before publication put the same item back at the head.
  // There are no allocation/callback operations after a successful commit.
  struct processing_guard {
    impl &state;
    std::list<submission> &item;
    bool finalized = false;
    ~processing_guard() {
      if (!finalized) {
        std::lock_guard lock(state.mutex);
        state.ready.splice(state.ready.begin(), item);
        state.processing = false;
      }
    }
  } guard{*state, in_flight};
  auto &item = in_flight.front();
  session_process_report report;
  report.had_submission = true;
  report.completions.resize(item.calls.size());
  std::list<queued_callback> notifications;
  for (const auto &call : item.calls)
    if (call.callback) notifications.push_back({call.callback, {}});
  auto disposed_after = state->disposed;
  for (auto id : item.disposals) disposed_after.insert(id.value);
  auto events = item.writes.empty() ? std::shared_ptr<event_result>{}
                                  : std::make_shared<event_result>();
  std::optional<sai_error> error;
  auto valid = state->context.validate();
  if (!valid) error = valid.error();
  if (!error && !item.writes.empty()) {
    auto batch = state->context.events(time);
    for (const auto &write : item.writes) {
      auto accepted = send_payload(batch, write);
      if (!accepted) { error = accepted.error(); break; }
    }
    if (!error) {
      auto evaluated = batch.commit();
      if (!evaluated) error = evaluated.error();
      else {
        static_assert(std::is_nothrow_move_assignable_v<event_result>);
        *events = std::move(*evaluated);
      }
    }
  }
  if (error) events.reset();
  // Error copies can allocate, but failed kernel processing did not publish.
  // Prebuild them before changing any receipt or consuming the queue entry.
  std::vector<update_progress> progress;
  if (error) {
    progress.resize(item.calls.size());
    for (auto &entry : progress) entry = {update_stage::failed, {}, error, item.sequence};
  }
  auto notification = notifications.begin();
  for (std::size_t i = 0; i < item.calls.size(); ++i) {
    auto &call = item.calls[i];
    auto &completion = report.completions[i];
    completion.ticket_id = call.ticket->id;
    completion.submission_sequence = item.sequence;
    completion.events = events;
    completion.error = error;
    if (call.callback) {
      notification->completion = completion;
      ++notification;
    }
  }
  {
    std::lock_guard lock(state->mutex);
    if (!error) state->disposed.swap(disposed_after);
    for (std::size_t i = 0; i < item.calls.size(); ++i) {
      if (error) set_progress(item.calls[i], std::move(progress[i]));
      else set_progress(item.calls[i], {update_stage::processed,
          events ? std::optional{events->after_revision} : std::nullopt, {}, item.sequence});
    }
    state->callbacks.splice(state->callbacks.end(), notifications);
    state->processing = false;
    guard.finalized = true;
  }
  return report;
}

dispatch_report external_session::drain_completions() {
  auto state = impl_; // Pin storage across arbitrary runtime/application callbacks.
  dispatch_report report;
  {
    std::lock_guard lock(state->mutex);
    if (state->draining) { report.already_draining = true; return report; }
    state->draining = true;
  }
  struct draining_guard {
    impl &state;
    ~draining_guard() {
      std::lock_guard lock(state.mutex);
      state.draining = false;
    }
  } guard{*state};
  for (;;) {
    std::optional<queued_callback> notification;
    {
      std::lock_guard lock(state->mutex);
      if (state->callbacks.empty()) break;
      notification.emplace(std::move(state->callbacks.front()));
      state->callbacks.pop_front();
    }
    try {
      (*notification->callback)(notification->completion);
    } catch (const std::exception &error) {
      report.errors.push_back(session_error(error_code::callback_failed,
          "external_session.drain_completions", error.what()));
    } catch (...) {
      report.errors.push_back(session_error(error_code::callback_failed,
          "external_session.drain_completions", "callback threw a non-standard exception"));
    }
    ++report.delivered;
  }
  return report;
}
external_session_host::external_session_host(execution_context context)
    : state_(std::make_shared<detail::external_session_host_state>(std::move(context))) {}

result<external_session> external_session_host::create_session() {
  std::lock_guard lock(state_->mutex);
  auto valid = state_->context.validate();
  if (!valid) return failure(valid.error());
  auto client = std::make_shared<external_session::impl>(state_->context);
  client->host = state_;
  state_->clients.push_back(client);
  return external_session{std::move(client)};
}
std::size_t external_session_host::pending_submissions() const {
  std::vector<std::shared_ptr<external_session::impl>> retained;
  std::lock_guard order_lock(state_->mutex);
  retained.reserve(state_->clients.size());
  std::size_t count = 0;
  for (auto &registered : state_->clients)
    if (auto client = registered.lock()) {
      retained.push_back(client);
      std::lock_guard lock(client->mutex);
      count += client->ready.size();
    }
  return count;
}
result<session_process_report> external_session_host::process_next(event_time time) {
  std::shared_ptr<external_session::impl> selected;
  std::vector<std::shared_ptr<external_session::impl>> retained;
  std::uint64_t earliest = 0;
  {
    std::lock_guard order_lock(state_->mutex);
    if (state_->processing)
      return failure(session_error(error_code::operation_in_progress,
          "external_session_host.process_next", "this host is already processing"));
    std::erase_if(state_->clients, [](const auto &client) { return client.expired(); });
    retained.reserve(state_->clients.size());
    for (auto &registered : state_->clients)
      if (auto client = registered.lock()) {
        retained.push_back(client);
        std::lock_guard lock(client->mutex);
        if (!client->ready.empty() &&
            (!selected || client->ready.front().sequence < earliest)) {
          earliest = client->ready.front().sequence;
          selected = std::move(client);
        }
      }
    if (!selected) return session_process_report{};
    state_->processing = true;
  }
  struct host_guard {
    std::shared_ptr<detail::external_session_host_state> state;
    ~host_guard() {
      std::lock_guard lock(state->mutex);
      state->processing = false;
    }
  } guard{state_};
  external_session pinned{std::move(selected)};
  return pinned.process_submission(time);
}
} // namespace x3d::sai::experimental
