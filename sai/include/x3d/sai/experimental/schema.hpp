#ifndef X3D_SAI_EXPERIMENTAL_SCHEMA_HPP
#define X3D_SAI_EXPERIMENTAL_SCHEMA_HPP

#include "types.hpp"

#include <span>
#include <string_view>

namespace x3d::sai::experimental {

// Shared generated-schema vocabulary. These keys carry field facts, never
// node/provider authority, and require neither concrete kernel handles nor
// reference-kernel linkage. Kernel typed_node remains a compatible consumer.
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

} // namespace x3d::sai::experimental
#endif
