#ifndef X3D_SAI_EXPERIMENTAL_TYPES_HPP
#define X3D_SAI_EXPERIMENTAL_TYPES_HPP

#include <array>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace x3d::sai::experimental {

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
              "the experimental SAI requires IEEE-754 binary32 float");
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
              "the experimental SAI requires IEEE-754 binary64 double");

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

struct vec2d {
  double x = 0;
  double y = 0;
  friend bool operator==(const vec2d &, const vec2d &) = default;
};
struct vec2f {
  float x = 0;
  float y = 0;
  friend bool operator==(const vec2f &, const vec2f &) = default;
};
struct vec3d {
  double x = 0;
  double y = 0;
  double z = 0;
  friend bool operator==(const vec3d &, const vec3d &) = default;
};
struct vec4d {
  double x = 0;
  double y = 0;
  double z = 0;
  double w = 0;
  friend bool operator==(const vec4d &, const vec4d &) = default;
};
struct vec4f {
  float x = 0;
  float y = 0;
  float z = 0;
  float w = 0;
  friend bool operator==(const vec4f &, const vec4f &) = default;
};
struct color3f {
  float r = 0;
  float g = 0;
  float b = 0;
  friend bool operator==(const color3f &, const color3f &) = default;
};
struct color4f {
  float r = 0;
  float g = 0;
  float b = 0;
  float a = 0;
  friend bool operator==(const color4f &, const color4f &) = default;
};
struct rotation {
  float x = 0;
  float y = 0;
  float z = 1;
  float angle = 0;
  friend bool operator==(const rotation &, const rotation &) = default;
};

template <class T, std::size_t N> struct matrix {
  std::array<T, N * N> elements{};
  constexpr matrix() {
    for (std::size_t diagonal = 0; diagonal < N; ++diagonal)
      elements[diagonal * N + diagonal] = T{1};
  }
  constexpr explicit matrix(std::array<T, N * N> values)
      : elements(std::move(values)) {}
  T &at(std::size_t row, std::size_t column) {
    if (row >= N || column >= N)
      throw std::out_of_range("matrix index is out of range");
    return elements.at(row * N + column);
  }
  const T &at(std::size_t row, std::size_t column) const {
    if (row >= N || column >= N)
      throw std::out_of_range("matrix index is out of range");
    return elements.at(row * N + column);
  }
  friend bool operator==(const matrix &, const matrix &) = default;
};

using matrix3d = matrix<double, 3>;
using matrix3f = matrix<float, 3>;
using matrix4d = matrix<double, 4>;
using matrix4f = matrix<float, 4>;

struct image {
  std::int32_t width = 0;
  std::int32_t height = 0;
  std::int32_t components = 0;
  std::vector<std::uint8_t> data;
  friend bool operator==(const image &, const image &) = default;
};

struct time_value {
  double seconds = 0;
  friend bool operator==(time_value, time_value) = default;
};

struct enum_value {
  std::string token;
  friend bool operator==(const enum_value &, const enum_value &) = default;
};

using node_list = std::vector<node_id>;

class bool_list {
public:
  class const_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;
    using value_type = bool;
    using difference_type = std::ptrdiff_t;
    using reference = bool;

    const_iterator() = default;
    bool operator*() const noexcept { return *current_ != 0; }
    const_iterator &operator++() noexcept {
      ++current_;
      return *this;
    }
    const_iterator operator++(int) noexcept {
      auto previous = *this;
      ++*this;
      return previous;
    }
    friend bool operator==(const const_iterator &,
                           const const_iterator &) = default;

  private:
    explicit const_iterator(std::vector<std::uint8_t>::const_iterator current)
        : current_(current) {}
    std::vector<std::uint8_t>::const_iterator current_;
    friend class bool_list;
  };

  bool_list() = default;
  bool_list(std::initializer_list<bool> values) {
    reserve(values.size());
    for (const bool value : values)
      push_back(value);
  }

  std::size_t size() const noexcept { return values_.size(); }
  bool empty() const noexcept { return values_.empty(); }
  bool at(std::size_t index) const { return values_.at(index) != 0; }
  bool operator[](std::size_t index) const noexcept {
    return values_[index] != 0;
  }
  void set(std::size_t index, bool value) {
    values_.at(index) = static_cast<std::uint8_t>(value);
  }
  void reserve(std::size_t capacity) { values_.reserve(capacity); }
  void push_back(bool value) {
    values_.push_back(static_cast<std::uint8_t>(value));
  }
  void insert(std::size_t index, bool value) {
    if (index > values_.size())
      throw std::out_of_range("Boolean insertion index is out of range");
    values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(index),
                   static_cast<std::uint8_t>(value));
  }
  void erase(std::size_t index) {
    if (index >= values_.size())
      throw std::out_of_range("Boolean erase index is out of range");
    values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(index));
  }
  void clear() noexcept { values_.clear(); }
  const_iterator begin() const noexcept {
    return const_iterator{values_.begin()};
  }
  const_iterator end() const noexcept { return const_iterator{values_.end()}; }
  friend bool operator==(const bool_list &, const bool_list &) = default;

private:
  std::vector<std::uint8_t> values_;
};
using color3f_list = std::vector<color3f>;
using color4f_list = std::vector<color4f>;
using double_list = std::vector<double>;
using float_list = std::vector<float>;
using image_list = std::vector<image>;
using int32_list = std::vector<std::int32_t>;
using matrix3d_list = std::vector<matrix3d>;
using matrix3f_list = std::vector<matrix3f>;
using matrix4d_list = std::vector<matrix4d>;
using matrix4f_list = std::vector<matrix4f>;
using rotation_list = std::vector<rotation>;
using string_list = std::vector<std::string>;
using time_list = std::vector<time_value>;
using vec2d_list = std::vector<vec2d>;
using vec2f_list = std::vector<vec2f>;
using vec3d_list = std::vector<vec3d>;
using vec3f_list = std::vector<vec3f>;
using vec4d_list = std::vector<vec4d>;
using vec4f_list = std::vector<vec4f>;
using enum_list = std::vector<enum_value>;

enum class value_kind {
  sf_bool,
  sf_color,
  sf_color_rgba,
  sf_double,
  sf_float,
  sf_image,
  sf_int32,
  sf_matrix3d,
  sf_matrix3f,
  sf_matrix4d,
  sf_matrix4f,
  sf_node,
  sf_rotation,
  sf_string,
  sf_time,
  sf_vec2d,
  sf_vec2f,
  sf_vec3d,
  sf_vec3f,
  sf_vec4d,
  sf_vec4f,
  mf_bool,
  mf_color,
  mf_color_rgba,
  mf_double,
  mf_float,
  mf_image,
  mf_int32,
  mf_matrix3d,
  mf_matrix3f,
  mf_matrix4d,
  mf_matrix4f,
  mf_node,
  mf_rotation,
  mf_string,
  mf_time,
  mf_vec2d,
  mf_vec2f,
  mf_vec3d,
  mf_vec3f,
  mf_vec4d,
  mf_vec4f,
  sf_enum,
  mf_enum,

  // Transitional spellings retained while the experimental call sites move
  // to exact ISO field kinds.
  boolean = sf_bool,
  int32 = sf_int32,
  number = sf_double,
  string = sf_string,
  vec3f = sf_vec3f,
  node = sf_node,
  node_list = mf_node,
};

using value = std::variant<
    std::monostate, bool, color3f, color4f, double, float, image, std::int32_t,
    matrix3d, matrix3f, matrix4d, matrix4f, node_id, rotation, std::string,
    time_value, vec2d, vec2f, vec3d, vec3f, vec4d, vec4f, bool_list,
    color3f_list, color4f_list, double_list, float_list, image_list, int32_list,
    matrix3d_list, matrix3f_list, matrix4d_list, matrix4f_list, node_list,
    rotation_list, string_list, time_list, vec2d_list, vec2f_list, vec3d_list,
    vec3f_list, vec4d_list, vec4f_list, enum_value, enum_list>;

// Numerical operator== remains available on the vocabulary types. This
// explicit relation is the SAI storage/event equivalence: it preserves IEEE
// payload bits, so signed zeros differ and identical NaN payloads match.
bool same_representation(const value &left, const value &right);

enum class access_type {
  initialize_only,
  input_only,
  output_only,
  input_output,
};

enum class default_source {
  field_type,
  schema,
};

template <class T> struct value_traits;
#define X3D_SAI_VALUE_TRAIT(Type, Kind)                                        \
  template <> struct value_traits<Type> {                                      \
    static constexpr value_kind kind = value_kind::Kind;                       \
  }
X3D_SAI_VALUE_TRAIT(bool, sf_bool);
X3D_SAI_VALUE_TRAIT(color3f, sf_color);
X3D_SAI_VALUE_TRAIT(color4f, sf_color_rgba);
X3D_SAI_VALUE_TRAIT(double, sf_double);
X3D_SAI_VALUE_TRAIT(float, sf_float);
X3D_SAI_VALUE_TRAIT(image, sf_image);
X3D_SAI_VALUE_TRAIT(std::int32_t, sf_int32);
X3D_SAI_VALUE_TRAIT(matrix3d, sf_matrix3d);
X3D_SAI_VALUE_TRAIT(matrix3f, sf_matrix3f);
X3D_SAI_VALUE_TRAIT(matrix4d, sf_matrix4d);
X3D_SAI_VALUE_TRAIT(matrix4f, sf_matrix4f);
X3D_SAI_VALUE_TRAIT(node_id, sf_node);
X3D_SAI_VALUE_TRAIT(rotation, sf_rotation);
X3D_SAI_VALUE_TRAIT(std::string, sf_string);
X3D_SAI_VALUE_TRAIT(time_value, sf_time);
X3D_SAI_VALUE_TRAIT(vec2d, sf_vec2d);
X3D_SAI_VALUE_TRAIT(vec2f, sf_vec2f);
X3D_SAI_VALUE_TRAIT(vec3d, sf_vec3d);
X3D_SAI_VALUE_TRAIT(vec3f, sf_vec3f);
X3D_SAI_VALUE_TRAIT(vec4d, sf_vec4d);
X3D_SAI_VALUE_TRAIT(vec4f, sf_vec4f);
X3D_SAI_VALUE_TRAIT(bool_list, mf_bool);
X3D_SAI_VALUE_TRAIT(color3f_list, mf_color);
X3D_SAI_VALUE_TRAIT(color4f_list, mf_color_rgba);
X3D_SAI_VALUE_TRAIT(double_list, mf_double);
X3D_SAI_VALUE_TRAIT(float_list, mf_float);
X3D_SAI_VALUE_TRAIT(image_list, mf_image);
X3D_SAI_VALUE_TRAIT(int32_list, mf_int32);
X3D_SAI_VALUE_TRAIT(matrix3d_list, mf_matrix3d);
X3D_SAI_VALUE_TRAIT(matrix3f_list, mf_matrix3f);
X3D_SAI_VALUE_TRAIT(matrix4d_list, mf_matrix4d);
X3D_SAI_VALUE_TRAIT(matrix4f_list, mf_matrix4f);
X3D_SAI_VALUE_TRAIT(node_list, mf_node);
X3D_SAI_VALUE_TRAIT(rotation_list, mf_rotation);
X3D_SAI_VALUE_TRAIT(string_list, mf_string);
X3D_SAI_VALUE_TRAIT(time_list, mf_time);
X3D_SAI_VALUE_TRAIT(vec2d_list, mf_vec2d);
X3D_SAI_VALUE_TRAIT(vec2f_list, mf_vec2f);
X3D_SAI_VALUE_TRAIT(vec3d_list, mf_vec3d);
X3D_SAI_VALUE_TRAIT(vec3f_list, mf_vec3f);
X3D_SAI_VALUE_TRAIT(vec4d_list, mf_vec4d);
X3D_SAI_VALUE_TRAIT(vec4f_list, mf_vec4f);
X3D_SAI_VALUE_TRAIT(enum_value, sf_enum);
X3D_SAI_VALUE_TRAIT(enum_list, mf_enum);
#undef X3D_SAI_VALUE_TRAIT

enum class error_code {
  duplicate_type,
  duplicate_field,
  invalid_descriptor,
  unknown_type,
  unknown_node,
  unknown_field,
  type_mismatch,
  invalid_value,
  index_out_of_range,
  access_denied,
  invalid_name,
  duplicate_name,
  invalid_context,
  invalid_route,
  containment_cycle,
  stale_handle,
  stale_revision,
  stale_aperture,
  poisoned_edit,
  callback_failed,
  ambiguous_event_seed,
  event_time_regression,
  cancelled,
  stale_completion,
  unsupported_field_type,
  abstract_type,
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
  default_source default_origin = default_source::field_type;
  bool containment = false;
  std::vector<std::string> accepted_node_types;
  std::optional<std::string> unit_category;
};

struct node_type_descriptor {
  std::string name;
  std::vector<field_descriptor> fields;
  std::string component;
  int component_level = 0;
  std::vector<std::string> interfaces;
  bool abstract = false;
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
