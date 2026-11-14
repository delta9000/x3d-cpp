#ifndef X3D_SAI_EXPERIMENTAL_MATRIX_HPP
#define X3D_SAI_EXPERIMENTAL_MATRIX_HPP

#include "types.hpp"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <concepts>
#include <limits>
#include <type_traits>

namespace x3d::sai::experimental {

// Algebra on the existing owning SFMatrix values. Storage is row-major;
// vector orientation is explicit and independent of that storage order.
// All operations return new values and require no browser or compiled kernel.
// See docs/wiki/sai/reference/matrix.md for the source mapping and safety policy.
template <class T, std::size_t N>
concept supported_matrix =
    (std::same_as<T, float> || std::same_as<T, double>) && (N == 3 || N == 4);

template <class T, std::size_t N>
  requires supported_matrix<T, N>
using matrix_vector = std::conditional_t<
    N == 3, std::conditional_t<std::same_as<T, float>, vec3f, vec3d>,
    std::conditional_t<std::same_as<T, float>, vec4f, vec4d>>;

namespace detail {
inline unexpected matrix_error(error_code code, const char *operation,
                               const char *message) {
  sai_error error;
  error.code = code;
  error.operation = operation;
  error.message = message;
  return failure(std::move(error));
}

template <class T, std::size_t N>
bool matrix_finite(const matrix<T, N> &value) noexcept {
  return std::all_of(value.elements.begin(), value.elements.end(),
                     [](T component) { return std::isfinite(component); });
}

// Check before conversion: an out-of-range floating conversion must not be
// used as an overflow detector. Ordinary rounding/underflow is not clamping.
template <class T> bool matrix_narrow(long double value, T &output) noexcept {
  if (!std::isfinite(value) ||
      std::abs(value) > static_cast<long double>(std::numeric_limits<T>::max()))
    return false;
  output = static_cast<T>(value);
  return true;
}

// Evaluate one entry of left*right-I without hiding cancellation in rounded
// products. FMA recovers product rounding errors; TwoSum keeps a bounded
// nonoverlapping expansion for all 2*N product terms and the identity term.
// This helper is used only under round-to-nearest by matrix_inverse.
template <class T, std::size_t N>
bool matrix_identity_residual(const matrix<T, N> &left,
                              const matrix<T, N> &right, std::size_t row,
                              std::size_t column, long double &output) {
  std::array<long double, 2 * N + 1> expansion{};
  std::size_t size = 0;
  const auto accumulate = [&](long double term) {
    if (!std::isfinite(term))
      return false;
    std::size_t written = 0;
    for (std::size_t i = 0; i < size; ++i) {
      const auto addend = expansion[i];
      const auto sum = term + addend;
      if (!std::isfinite(sum))
        return false;
      const auto virtual_addend = sum - term;
      const auto error =
          (term - (sum - virtual_addend)) + (addend - virtual_addend);
      if (error != 0)
        expansion[written++] = error;
      term = sum;
    }
    if (term != 0 || written == 0)
      expansion[written++] = term;
    size = written;
    return true;
  };
  for (std::size_t k = 0; k < N; ++k) {
    const auto a = static_cast<long double>(left.elements[row * N + k]);
    const auto b = static_cast<long double>(right.elements[k * N + column]);
    const auto product = a * b;
    if (!std::isfinite(product) || !accumulate(product) ||
        !accumulate(std::fma(a, b, -product)))
      return false;
  }
  if (!accumulate(row == column ? -1.L : 0.L))
    return false;
  output = 0;
  for (std::size_t i = 0; i < size; ++i)
    output += expansion[i];
  return std::isfinite(output);
}

template <class T, std::size_t N>
result<matrix<T, N>> matrix_product(const matrix<T, N> &left,
                                    const matrix<T, N> &right,
                                    const char *operation) {
  if (!matrix_finite(left) || !matrix_finite(right))
    return matrix_error(error_code::invalid_value, operation,
                        "matrix components must be finite");
  matrix<T, N> output;
  for (std::size_t row = 0; row < N; ++row)
    for (std::size_t column = 0; column < N; ++column) {
      long double sum = 0;
      for (std::size_t k = 0; k < N; ++k) {
        sum += static_cast<long double>(left.elements[row * N + k]) *
               static_cast<long double>(right.elements[k * N + column]);
        if (!std::isfinite(sum))
          return matrix_error(error_code::invalid_value, operation,
                              "matrix product exceeded working precision");
      }
      if (!matrix_narrow(sum, output.elements[row * N + column]))
        return matrix_error(error_code::invalid_value, operation,
                            "matrix product is outside the component range");
    }
  return output;
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix_vector<T, N>>
matrix_vector_product(const matrix<T, N> &value,
                      const matrix_vector<T, N> &vector, bool column_vector,
                      const char *operation) {
  std::array<T, N> input{vector.x, vector.y, vector.z};
  if constexpr (N == 4)
    input[3] = vector.w;
  if (!matrix_finite(value) ||
      !std::all_of(input.begin(), input.end(),
                   [](T component) { return std::isfinite(component); }))
    return matrix_error(error_code::invalid_value, operation,
                        "matrix and vector components must be finite");
  std::array<T, N> output{};
  for (std::size_t i = 0; i < N; ++i) {
    long double sum = 0;
    for (std::size_t k = 0; k < N; ++k) {
      const auto index = column_vector ? i * N + k : k * N + i;
      sum += static_cast<long double>(value.elements[index]) *
             static_cast<long double>(input[k]);
      if (!std::isfinite(sum))
        return matrix_error(error_code::invalid_value, operation,
                            "vector product exceeded working precision");
    }
    if (!matrix_narrow(sum, output[i]))
      return matrix_error(error_code::invalid_value, operation,
                          "vector product is outside the component range");
  }
  if constexpr (N == 3)
    return matrix_vector<T, N>{output[0], output[1], output[2]};
  else
    return matrix_vector<T, N>{output[0], output[1], output[2], output[3]};
}
} // namespace detail

// These checked import/export functions change serialization order, never
// mathematical row/column coordinates or the vector convention.
template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_from_row_major(std::span<const T> components) {
  constexpr auto operation = "matrix_from_row_major";
  if (components.size() != N * N)
    return detail::matrix_error(
        error_code::invalid_value, operation,
        "component count must equal the matrix order squared");
  matrix<T, N> output;
  std::copy(components.begin(), components.end(), output.elements.begin());
  if (!detail::matrix_finite(output))
    return detail::matrix_error(error_code::invalid_value, operation,
                                "matrix components must be finite");
  return output;
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_from_column_major(std::span<const T> components) {
  constexpr auto operation = "matrix_from_column_major";
  if (components.size() != N * N)
    return detail::matrix_error(
        error_code::invalid_value, operation,
        "component count must equal the matrix order squared");
  matrix<T, N> output;
  for (std::size_t row = 0; row < N; ++row)
    for (std::size_t column = 0; column < N; ++column)
      output.elements[row * N + column] = components[column * N + row];
  if (!detail::matrix_finite(output))
    return detail::matrix_error(error_code::invalid_value, operation,
                                "matrix components must be finite");
  return output;
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<std::array<T, N * N>> matrix_to_row_major(const matrix<T, N> &value) {
  if (!detail::matrix_finite(value))
    return detail::matrix_error(error_code::invalid_value,
                                "matrix_to_row_major",
                                "matrix components must be finite");
  return value.elements;
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<std::array<T, N * N>> matrix_to_column_major(const matrix<T, N> &value) {
  if (!detail::matrix_finite(value))
    return detail::matrix_error(error_code::invalid_value,
                                "matrix_to_column_major",
                                "matrix components must be finite");
  std::array<T, N * N> output{};
  for (std::size_t row = 0; row < N; ++row)
    for (std::size_t column = 0; column < N; ++column)
      output[column * N + row] = value.elements[row * N + column];
  return output;
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<T> matrix_component(const matrix<T, N> &value, std::size_t row,
                           std::size_t column) {
  constexpr auto operation = "matrix_component";
  if (row >= N || column >= N)
    return detail::matrix_error(error_code::index_out_of_range, operation,
                                "matrix row or column is out of range");
  if (!detail::matrix_finite(value))
    return detail::matrix_error(error_code::invalid_value, operation,
                                "matrix components must be finite");
  return value.elements[row * N + column];
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> with_matrix_component(const matrix<T, N> &value,
                                           std::size_t row, std::size_t column,
                                           T component) {
  constexpr auto operation = "with_matrix_component";
  if (row >= N || column >= N)
    return detail::matrix_error(error_code::index_out_of_range, operation,
                                "matrix row or column is out of range");
  if (!detail::matrix_finite(value) || !std::isfinite(component))
    return detail::matrix_error(error_code::invalid_value, operation,
                                "matrix components must be finite");
  auto output = value;
  output.elements[row * N + column] = component;
  return output;
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_transpose(const matrix<T, N> &value) {
  if (!detail::matrix_finite(value))
    return detail::matrix_error(error_code::invalid_value, "matrix_transpose",
                                "matrix components must be finite");
  matrix<T, N> output;
  for (std::size_t row = 0; row < N; ++row)
    for (std::size_t column = 0; column < N; ++column)
      output.elements[row * N + column] = value.elements[column * N + row];
  return output;
}

// A * B. For column vectors B acts first; for row vectors A acts first.
template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_multiply(const matrix<T, N> &left,
                                     const matrix<T, N> &right) {
  return detail::matrix_product(left, right, "matrix_multiply");
}

// Pre-multiply the current matrix: left * current.
template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_multiply_left(const matrix<T, N> &current,
                                          const matrix<T, N> &left) {
  return detail::matrix_product(left, current, "matrix_multiply_left");
}

// Post-multiply the current matrix: current * right.
template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_multiply_right(const matrix<T, N> &current,
                                           const matrix<T, N> &right) {
  return detail::matrix_product(current, right, "matrix_multiply_right");
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix_vector<T, N>>
matrix_multiply_column_vector(const matrix<T, N> &value,
                              const matrix_vector<T, N> &vector) {
  return detail::matrix_vector_product(value, vector, true,
                                       "matrix_multiply_column_vector");
}

template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix_vector<T, N>>
matrix_multiply_row_vector(const matrix<T, N> &value,
                           const matrix_vector<T, N> &vector) {
  return detail::matrix_vector_product(value, vector, false,
                                       "matrix_multiply_row_vector");
}

// Scaled-partial-pivoted Gauss-Jordan elimination of A/max(abs(A)) in long
// double. Reject zero pivots, unrepresentable results, and a returned inverse
// whose two identity-product residuals exceed 64*N*epsilon(T). Conditioning
// alone does not reject exact inverses such as large translations. This is a
// proposal policy, not an ISO-prescribed tolerance or certified forward-error
// bound.
template <class T, std::size_t N>
  requires supported_matrix<T, N>
result<matrix<T, N>> matrix_inverse(const matrix<T, N> &value) {
  constexpr auto operation = "matrix_inverse";
  if (std::fegetround() != FE_TONEAREST)
    return detail::matrix_error(error_code::invalid_value, operation,
                                "inverse requires round-to-nearest arithmetic");
  if (!detail::matrix_finite(value))
    return detail::matrix_error(error_code::invalid_value, operation,
                                "matrix components must be finite");
  long double scale = 0;
  for (const auto component : value.elements)
    scale = std::max(scale, std::abs(static_cast<long double>(component)));
  if (scale == 0)
    return detail::matrix_error(error_code::invalid_value, operation,
                                "matrix is singular");

  std::array<std::array<long double, 2 * N>, N> work{};
  std::array<long double, N> row_scales{};
  for (std::size_t row = 0; row < N; ++row) {
    for (std::size_t column = 0; column < N; ++column) {
      work[row][column] =
          static_cast<long double>(value.elements[row * N + column]) / scale;
      row_scales[row] = std::max(row_scales[row], std::abs(work[row][column]));
    }
    if (row_scales[row] == 0)
      return detail::matrix_error(error_code::invalid_value, operation,
                                  "matrix has a zero row at working precision");
    work[row][N + row] = 1;
  }
  for (std::size_t column = 0; column < N; ++column) {
    auto pivot_row = column;
    for (std::size_t row = column + 1; row < N; ++row)
      if (std::abs(work[row][column]) / row_scales[row] >
          std::abs(work[pivot_row][column]) / row_scales[pivot_row])
        pivot_row = row;
    if (work[pivot_row][column] == 0)
      return detail::matrix_error(error_code::invalid_value, operation,
                                  "matrix is singular");
    if (pivot_row != column) {
      std::swap(work[pivot_row], work[column]);
      std::swap(row_scales[pivot_row], row_scales[column]);
    }
    const auto pivot = work[column][column];
    for (std::size_t k = 0; k < 2 * N; ++k) {
      work[column][k] /= pivot;
      if (!std::isfinite(work[column][k]))
        return detail::matrix_error(error_code::invalid_value, operation,
                                    "inverse exceeded working precision");
    }
    work[column][column] = 1;
    for (std::size_t row = 0; row < N; ++row) {
      if (row == column)
        continue;
      const auto factor = work[row][column];
      for (std::size_t k = 0; k < 2 * N; ++k) {
        work[row][k] -= factor * work[column][k];
        if (!std::isfinite(work[row][k]))
          return detail::matrix_error(error_code::invalid_value, operation,
                                      "inverse exceeded working precision");
      }
      work[row][column] = 0;
    }
  }
  matrix<T, N> output;
  for (std::size_t row = 0; row < N; ++row)
    for (std::size_t column = 0; column < N; ++column)
      if (!detail::matrix_narrow(work[row][N + column] / scale,
                                 output.elements[row * N + column]))
        return detail::matrix_error(error_code::invalid_value, operation,
                                    "inverse is outside the component range");
  // Validate the rounded, returned value, not merely the wider workspace.
  // Absolute infinity-norm residuals deliberately require an actual identity
  // approximation, rather than allowing errors proportional to cond(A).
  const auto tolerance =
      64.L * static_cast<long double>(N) * std::numeric_limits<T>::epsilon();
  for (std::size_t row = 0; row < N; ++row) {
    long double left_residual = 0, right_residual = 0;
    for (std::size_t column = 0; column < N; ++column) {
      long double left = 0, right = 0;
      if (!detail::matrix_identity_residual(value, output, row, column, left) ||
          !detail::matrix_identity_residual(output, value, row, column, right))
        return detail::matrix_error(
            error_code::invalid_value, operation,
            "inverse residual exceeded working precision");
      left_residual += std::abs(left);
      right_residual += std::abs(right);
    }
    if (left_residual > tolerance || right_residual > tolerance)
      return detail::matrix_error(
          error_code::invalid_value, operation,
          "rounded inverse failed identity residual validation");
  }
  return output;
}

} // namespace x3d::sai::experimental
#endif
