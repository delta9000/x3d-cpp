#ifndef X3D_SAI_EXPERIMENTAL_SESSION_HPP
#define X3D_SAI_EXPERIMENTAL_SESSION_HPP

#include "kernel.hpp"

namespace x3d::sai::experimental {

namespace detail {
struct update_ticket_state;
struct external_session_host_state;
}

// Proposal vocabulary, not names of ISO services. "Submitted" means released
// to this session's browser-input FIFO; only "processed" guarantees that the
// reference kernel has evaluated the event cascade and published scene state.
enum class update_stage { accepted, submitted, processed, failed };

struct update_progress {
  update_stage stage = update_stage::accepted;
  std::optional<revision_id> scene_revision;
  std::optional<sai_error> error;
  std::optional<std::uint64_t> submission_sequence;
};

class update_ticket {
public:
  std::uint64_t id() const noexcept;
  update_progress progress() const;
private:
  explicit update_ticket(std::shared_ptr<detail::update_ticket_state> state)
      : state_(std::move(state)) {}
  std::shared_ptr<detail::update_ticket_state> state_;
  friend class external_session;
};

struct update_completion {
  std::uint64_t ticket_id = 0;
  // Coalesced calls share the complete whole-field/cascade result. A successful
  // disposal-only submission has no event result and does not mutate the scene.
  std::shared_ptr<const event_result> events;
  std::optional<sai_error> error;
  std::uint64_t submission_sequence = 0;
};
using update_callback = std::function<void(const update_completion &)>;

struct session_process_report {
  bool had_submission = false;
  std::vector<update_completion> completions;
};

// A bounded external-SAI field/updateControl/dispose implementation. This is
// separate from the snapshot/editor transaction extension, not a replacement
// for it. It supports live inputOnly/inputOutput writes; initializeOnly
// construction remains a scene_edit extension. It does not implement browser
// loadURL/replaceWorld service ordering.
//
// begin_update is a per-client, non-nesting gate. Reads always consult committed
// scene state. Gated writes to a field form one whole-field event; indexed
// writes aggregate and the last write to an index wins. Whole-field/SF last
// write wins is an explicit reference-provider policy. Without a gate every
// call gets a separate FIFO submission and event, even for equal values.
//
// The host calls process_next with its clock at processing time (no earlier
// than the submission's end_update/set_value request). Times must strictly
// advance across all sessions and direct event batches sharing the context.
// This class neither guesses a clock nor schedules background processing.
// While this session is processing, its ordinary services reject reentrant
// calls with operation_in_progress; receipt polling and draining remain safe.
// Runtime failures are terminal for that submission, leave scene state intact,
// and do not poison later submissions. Graph/event observers are dispatched by
// execution_context::drain; completion callbacks ONLY by drain_completions.
//
// close flushes the gate and rejects new calls; process_next and completion
// draining remain available until the FIFO is empty. Hosts must finish this
// drain before destroying the session. Destruction fails outstanding tickets
// with cancelled and never calls user code. Separate sessions are independent;
// disposing a node invalidates only this session's node services, not scene
// edges, previously acquired fields, kernel handles, or another client's access.
class external_session {
public:
  explicit external_session(execution_context context);
  external_session(external_session &&) noexcept;
  external_session &operator=(external_session &&) noexcept;
  ~external_session();
  external_session(const external_session &) = delete;
  external_session &operator=(const external_session &) = delete;

  result<void> begin_update();
  result<void> end_update();
  result<void> close();
  bool buffering() const;
  bool closed() const;
  std::size_t pending_submissions() const;
  std::optional<std::uint64_t> next_submission_sequence() const;

  result<std::string> get_type_name(const node &) const;
  result<std::vector<field_descriptor>> get_field_definitions(const node &) const;
  result<dynamic_field> get_field(const node &, const std::string &) const;
  result<bool> is_realized(const node &) const;

  result<value> get_value(const dynamic_field &source) const;
  template <class T> result<T> get_value(const field<T> &source) const {
    auto read = get_value(source.dynamic());
    if (!read) return failure(read.error());
    return std::get<T>(std::move(*read));
  }

  // MF reads count logical elements and always consult committed scene state.
  // get_one_value(dynamic_field) rejects MFNode; get_one_node and the typed
  // MFNode overload return the owner-bearing handles required by node setters.
  result<std::size_t> get_size(const dynamic_field &) const;
  template <multi_field_sequence Sequence>
  result<std::size_t> get_size(const field<Sequence> &source) const {
    return get_size(source.dynamic());
  }
  result<value> get_one_value(const dynamic_field &, std::size_t) const;
  result<node> get_one_node(const dynamic_field &, std::size_t) const;
  template <multi_field_sequence Sequence>
    requires (!std::same_as<Sequence, node_list>)
  result<typename detail::multi_sequence_traits<Sequence>::element_type>
  get_one_value(const field<Sequence> &source, std::size_t index) const {
    auto read = get_one_value(source.dynamic(), index);
    if (!read) return failure(read.error());
    using element_type =
        typename detail::multi_sequence_traits<Sequence>::element_type;
    return std::get<element_type>(std::move(*read));
  }
  result<node> get_one_value(const field<node_list> &source,
                             std::size_t index) const {
    return get_one_node(source.dynamic(), index);
  }

  result<update_ticket> set_value(const dynamic_field &, value,
                                  update_callback = {});
  result<update_ticket> set_value(const dynamic_field &, const node &,
                                  update_callback = {});
  result<update_ticket> set_value(const dynamic_field &, std::span<const node>,
                                  update_callback = {});
  result<update_ticket> set_value(const dynamic_field &target, std::nullopt_t,
                                  update_callback callback = {}) {
    return set_value(target, node{}, std::move(callback));
  }
  template <class T>
    requires (!std::same_as<T, node_id> && !std::same_as<T, node_list>)
  result<update_ticket> set_value(const field<T> &target, T payload,
                                  update_callback callback = {}) {
    return set_value(target.dynamic(), value{std::move(payload)},
                     std::move(callback));
  }
  result<update_ticket> set_value(const field<node_id> &target,
                                  const node &payload,
                                  update_callback callback = {}) {
    return set_value(target.dynamic(), payload, std::move(callback));
  }
  result<update_ticket> set_value(const field<node_id> &target,
                                  std::nullopt_t,
                                  update_callback callback = {}) {
    return set_value(target.dynamic(), node{}, std::move(callback));
  }
  result<update_ticket> set_value(const field<node_list> &target,
                                  std::span<const node> payload,
                                  update_callback callback = {}) {
    return set_value(target.dynamic(), payload, std::move(callback));
  }

  // Indexed operations preserve length and reject an out-of-bounds index.
  // For inputOnly MF fields a preceding pending whole-field write is required
  // as the base, since reading committed inputOnly state is not permitted.
  result<update_ticket> set_one_value(const dynamic_field &, std::size_t,
                                      value, update_callback = {});
  result<update_ticket> set_one_value(const dynamic_field &, std::size_t,
                                      const node &, update_callback = {});
  template <multi_field_sequence Sequence>
    requires (!std::same_as<Sequence, node_list>)
  result<update_ticket> set_one_value(
      const field<Sequence> &target, std::size_t index,
      typename detail::multi_sequence_traits<Sequence>::element_type element,
      update_callback callback = {}) {
    return set_one_value(target.dynamic(), index, value{std::move(element)},
                         std::move(callback));
  }
  result<update_ticket> set_one_value(const field<node_list> &target,
                                      std::size_t index, const node &element,
                                      update_callback callback = {}) {
    return set_one_value(target.dynamic(), index, element, std::move(callback));
  }
  // Structural MF mutations use the latest pending value, or committed state
  // when no value is pending. Insert accepts index == size; remove requires
  // index < size. clear needs no readable base and can seed an inputOnly MF.
  // Each accepted call produces one whole-field event (one per field per gate).
  result<update_ticket> append(const dynamic_field &, value,
                                update_callback = {});
  result<update_ticket> append(const dynamic_field &, const node &,
                                update_callback = {});
  result<update_ticket> insert_value(const dynamic_field &, std::size_t, value,
                                      update_callback = {});
  result<update_ticket> insert_value(const dynamic_field &, std::size_t,
                                      const node &, update_callback = {});
  result<update_ticket> remove(const dynamic_field &, std::size_t,
                                update_callback = {});
  result<update_ticket> clear(const dynamic_field &, update_callback = {});
  template <multi_field_sequence Sequence>
    requires (!std::same_as<Sequence, node_list>)
  result<update_ticket> append(
      const field<Sequence> &target,
      typename detail::multi_sequence_traits<Sequence>::element_type element,
      update_callback callback = {}) {
    return append(target.dynamic(), value{std::move(element)},
                  std::move(callback));
  }
  result<update_ticket> append(const field<node_list> &target,
                                const node &element,
                                update_callback callback = {}) {
    return append(target.dynamic(), element, std::move(callback));
  }
  template <multi_field_sequence Sequence>
    requires (!std::same_as<Sequence, node_list>)
  result<update_ticket> insert_value(
      const field<Sequence> &target, std::size_t index,
      typename detail::multi_sequence_traits<Sequence>::element_type element,
      update_callback callback = {}) {
    return insert_value(target.dynamic(), index, value{std::move(element)},
                        std::move(callback));
  }
  result<update_ticket> insert_value(const field<node_list> &target,
                                      std::size_t index, const node &element,
                                      update_callback callback = {}) {
    return insert_value(target.dynamic(), index, element, std::move(callback));
  }
  template <multi_field_sequence Sequence>
  result<update_ticket> remove(const field<Sequence> &target, std::size_t index,
                                update_callback callback = {}) {
    return remove(target.dynamic(), index, std::move(callback));
  }
  template <multi_field_sequence Sequence>
  result<update_ticket> clear(const field<Sequence> &target,
                               update_callback callback = {}) {
    return clear(target.dynamic(), std::move(callback));
  }

  // Patches the latest pending image, or committed image when no update is
  // pending. Pixel format is exact and clipping never resizes the destination.
  result<update_ticket> set_sub_image(
      const dynamic_field &, const image &source, std::int32_t width,
      std::int32_t height, std::int32_t source_x, std::int32_t source_y,
      std::int32_t destination_x, std::int32_t destination_y,
      update_callback = {});
  result<update_ticket> set_sub_image(
      const field<image> &target, const image &source, std::int32_t width,
      std::int32_t height, std::int32_t source_x, std::int32_t source_y,
      std::int32_t destination_x, std::int32_t destination_y,
      update_callback callback = {}) {
    return set_sub_image(target.dynamic(), source, width, height, source_x,
                         source_y, destination_x, destination_y,
                         std::move(callback));
  }
  result<update_ticket> dispose_node(const node &, update_callback = {});

  result<session_process_report> process_next(event_time);
  dispatch_report drain_completions();

private:
  struct impl;
  explicit external_session(std::shared_ptr<impl> implementation)
      : impl_(std::move(implementation)) {}
  enum class multi_mutation { set, append, insert, remove, clear };
  result<update_ticket> mutate_multi_field(
      const dynamic_field &, multi_mutation, std::size_t,
      const value *, const node *, update_callback);
  result<session_process_report> process_submission(event_time);
  std::shared_ptr<impl> impl_;
  friend class external_session_host;
  friend struct detail::external_session_host_state;
};

// A host-controlled scheduler for clients of exactly one execution context.
// Creation establishes context authority; arbitrary/foreign client registration
// is intentionally not exposed. Releases are ordered across clients, including
// ungated calls. process_next chooses the earliest released submission and
// executes without scheduler locks held. It never drains application callbacks.
// Closed clients remain drainable; destroyed clients are pruned. Direct client
// processing is disabled while its host exists, preventing FIFO bypass. If the
// host is destroyed first, surviving clients revert to the lower-level API.
class external_session_host {
public:
  explicit external_session_host(execution_context context);
  result<external_session> create_session();
  result<session_process_report> process_next(event_time);
  std::size_t pending_submissions() const;
private:
  std::shared_ptr<detail::external_session_host_state> state_;
};

} // namespace x3d::sai::experimental
#endif
