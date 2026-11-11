#ifndef X3D_SAI_EXPERIMENTAL_USER_DATA_HPP
#define X3D_SAI_EXPERIMENTAL_USER_DATA_HPP

#include "types.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace x3d::sai::experimental {

// Proposal-owned user data, not the published binding's CObject spelling.
// Copies share ownership; reads expose const payloads. Payload construction and
// destruction never occur under an internal kernel lock. Destructors must obey
// the C++ noexcept contract; throwing from one terminates the process.
class user_data_value {
public:
  user_data_value() noexcept = default;

  template <class T, class... Args>
    requires std::is_object_v<T> && (!std::is_array_v<T>) &&
             std::same_as<T, std::remove_cv_t<T>> &&
             std::is_nothrow_destructible_v<T> &&
             std::constructible_from<T, Args...>
  static user_data_value make(Args &&...args) {
    return user_data_value{std::make_shared<const T>(
                               std::forward<Args>(args)...),
                           typeid(T)};
  }

  bool has_value() const noexcept { return static_cast<bool>(value_); }

  template <class T>
    requires std::is_object_v<T> && (!std::is_array_v<T>) &&
             std::same_as<T, std::remove_cv_t<T>>
  result<std::shared_ptr<const T>> as() const {
    if (!value_)
      return std::shared_ptr<const T>{};
    if (*type_ == typeid(T))
      return std::static_pointer_cast<const T>(value_);
    sai_error error;
    error.code = error_code::type_mismatch;
    error.operation = "user_data_value.as";
    error.message = "user data does not contain the requested exact type";
    return failure(std::move(error));
  }

  void swap(user_data_value &other) noexcept {
    value_.swap(other.value_);
    std::swap(type_, other.type_);
  }

private:
  user_data_value(std::shared_ptr<const void> value, const std::type_info &type)
      : value_(std::move(value)), type_(&type) {}
  std::shared_ptr<const void> value_;
  const std::type_info *type_ = nullptr;
};


} // namespace x3d::sai::experimental

#endif
