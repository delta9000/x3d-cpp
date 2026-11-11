#ifndef X3D_SAI_PROVIDER_BUDGET_HPP
#define X3D_SAI_PROVIDER_BUDGET_HPP

#include "x3d/sai/experimental/provider.hpp"

namespace x3d::sai::experimental::provider::detail {
// Logical admission accounting, not allocator/process memory accounting.
// Keeping a remaining budget avoids overflow in both products and sums.
class byte_budget {
public:
  explicit byte_budget(std::size_t available) : remaining_(available) {}
  bool add(std::size_t count, std::size_t width = 1) noexcept {
    if (width != 0 && count > remaining_ / width) return false;
    remaining_ -= count * width;
    return true;
  }
  std::size_t remaining() const noexcept { return remaining_; }
private:
  std::size_t remaining_;
};

inline bool account_dynamic(byte_budget &budget, const payload &value) {
  return std::visit([&](const auto &item) {
    using T = std::decay_t<decltype(item)>;
    if constexpr (std::same_as<T, image>) return budget.add(item.data.size());
    else if constexpr (std::same_as<T, bool_list>) return budget.add(item.size());
    else if constexpr (requires { typename T::value_type; item.size(); }) {
      if (!budget.add(item.size(), sizeof(typename T::value_type))) return false;
      if constexpr (std::same_as<T, image_list>) {
        for (const auto &element : item)
          if (!budget.add(element.data.size())) return false;
      } else if constexpr (std::same_as<T, string_list>) {
        for (const auto &element : item)
          if (!budget.add(element.size())) return false;
      }
      return true;
    } else return true; // Fixed-size values already occupy their payload object.
  }, value);
}
} // namespace x3d::sai::experimental::provider::detail
#endif
