#ifndef X3D_SAI_EXPERIMENTAL_TYPES_HPP
#define X3D_SAI_EXPERIMENTAL_TYPES_HPP

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace x3d::sai::experimental {

using generation_id = std::uint64_t;
using revision_id = std::uint64_t;

struct event_time {
  double seconds = 0;
  friend bool operator==(event_time, event_time) = default;
  friend auto operator<=>(event_time, event_time) = default;
};

struct node_id {
  std::uint64_t value = 0;
  friend bool operator==(node_id, node_id) = default;
};

struct vec3f {
  float x = 0;
  float y = 0;
  float z = 0;
  friend bool operator==(const vec3f &, const vec3f &) = default;
};

using node_list = std::vector<node_id>;
using value = std::variant<std::monostate, bool, std::int32_t, double,
                           std::string, vec3f, node_id, node_list>;

enum class value_kind {
  boolean,
  int32,
  number,
  string,
  vec3f,
  node,
  node_list,
};

enum class access_type {
  initialize_only,
  input_only,
  output_only,
  input_output,
};

template <class T> struct value_traits;
template <> struct value_traits<bool> {
  static constexpr value_kind kind = value_kind::boolean;
};
template <> struct value_traits<std::int32_t> {
  static constexpr value_kind kind = value_kind::int32;
};
template <> struct value_traits<double> {
  static constexpr value_kind kind = value_kind::number;
};
template <> struct value_traits<std::string> {
  static constexpr value_kind kind = value_kind::string;
};
template <> struct value_traits<vec3f> {
  static constexpr value_kind kind = value_kind::vec3f;
};
template <> struct value_traits<node_id> {
  static constexpr value_kind kind = value_kind::node;
};
template <> struct value_traits<node_list> {
  static constexpr value_kind kind = value_kind::node_list;
};

enum class error_code {
  duplicate_type,
  duplicate_field,
  invalid_descriptor,
  unknown_type,
  unknown_node,
  unknown_field,
  type_mismatch,
  access_denied,
  invalid_name,
  duplicate_name,
  invalid_context,
  invalid_route,
  containment_cycle,
  stale_handle,
  stale_revision,
  poisoned_edit,
  callback_failed,
  ambiguous_event_seed,
  event_time_regression,
  cancelled,
  stale_completion,
};

struct sai_error {
  error_code code = error_code::invalid_descriptor;
  std::string operation;
  std::string message;
  generation_id generation = 0;
  revision_id base_revision = 0;
  revision_id current_revision = 0;
  std::optional<node_id> node;
  std::string field;
};

template <class T> class [[nodiscard]] result {
public:
  result(T value) : storage_(std::move(value)) {}
  result(sai_error error) : storage_(std::move(error)) {}

  explicit operator bool() const noexcept {
    return std::holds_alternative<T>(storage_);
  }

  T &value() & {
    if (!*this)
      throw std::logic_error("result has no value");
    return std::get<T>(storage_);
  }
  const T &value() const & {
    if (!*this)
      throw std::logic_error("result has no value");
    return std::get<T>(storage_);
  }
  T &&value() && {
    if (!*this)
      throw std::logic_error("result has no value");
    return std::get<T>(std::move(storage_));
  }

  sai_error &error() & {
    if (*this)
      throw std::logic_error("result has no error");
    return std::get<sai_error>(storage_);
  }
  const sai_error &error() const & {
    if (*this)
      throw std::logic_error("result has no error");
    return std::get<sai_error>(storage_);
  }

private:
  std::variant<T, sai_error> storage_;
};

template <> class [[nodiscard]] result<void> {
public:
  result() = default;
  result(sai_error error) : error_(std::move(error)) {}

  explicit operator bool() const noexcept { return !error_.has_value(); }
  void value() const {
    if (!*this)
      throw std::logic_error("result has no value");
  }
  sai_error &error() & {
    if (*this)
      throw std::logic_error("result has no error");
    return *error_;
  }
  const sai_error &error() const & {
    if (*this)
      throw std::logic_error("result has no error");
    return *error_;
  }

private:
  std::optional<sai_error> error_;
};

struct field_descriptor {
  std::string name;
  value_kind kind = value_kind::string;
  access_type access = access_type::input_output;
  value default_value;
  bool containment = false;
};

struct node_type_descriptor {
  std::string name;
  std::vector<field_descriptor> fields;
};

class type_registry {
public:
  result<void> define(node_type_descriptor descriptor);
  const node_type_descriptor *find(const std::string &name) const noexcept;

private:
  std::unordered_map<std::string, node_type_descriptor> types_;
};

} // namespace x3d::sai::experimental

#endif
