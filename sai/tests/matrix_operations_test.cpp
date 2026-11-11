#include "doctest/doctest.h"
#include "x3d/sai/experimental/matrix.hpp"

#include <cmath>
#include <limits>
#include <random>
#include <type_traits>

namespace sai = x3d::sai::experimental;

namespace {
template <class A, class B>
concept multiplicable = requires(A a, B b) { sai::matrix_multiply(a, b); };
template <class A, class V>
concept vector_multiplicable =
    requires(A a, V v) { sai::matrix_multiply_column_vector(a, v); };
static_assert(multiplicable<sai::matrix3f, sai::matrix3f>);
static_assert(!multiplicable<sai::matrix3f, sai::matrix4f>);
static_assert(!multiplicable<sai::matrix3f, sai::matrix3d>);
static_assert(!multiplicable<sai::matrix<int, 3>, sai::matrix<int, 3>>);
static_assert(!multiplicable<sai::matrix<float, 2>, sai::matrix<float, 2>>);
static_assert(vector_multiplicable<sai::matrix3f, sai::vec3f>);
static_assert(vector_multiplicable<sai::matrix4d, sai::vec4d>);
static_assert(!vector_multiplicable<sai::matrix4f, sai::vec3f>);
static_assert(!vector_multiplicable<sai::matrix3d, sai::vec3f>);
static_assert(std::same_as<sai::matrix_vector<float, 3>, sai::vec3f>);
static_assert(std::same_as<sai::matrix_vector<double, 4>, sai::vec4d>);

template <class T> constexpr long double tolerance() {
  return 128.L * std::numeric_limits<T>::epsilon();
}

template <class T, std::size_t N>
void check_close(const sai::matrix<T, N> &actual,
                 const sai::matrix<T, N> &expected) {
  for (std::size_t i = 0; i < N * N; ++i)
    CHECK(std::abs(static_cast<long double>(actual.elements[i]) -
                   expected.elements[i]) <=
          tolerance<T>() * std::max(1.L, std::abs(static_cast<long double>(
                                             expected.elements[i]))));
}

// A separate scalar oracle, independent of the production multiply routine.
template <class T, std::size_t N>
void check_inverse_identity(const sai::matrix<T, N> &matrix,
                            const sai::matrix<T, N> &inverse) {
  for (std::size_t i = 0; i < N; ++i)
    for (std::size_t j = 0; j < N; ++j) {
      long double left = 0, right = 0;
      for (std::size_t k = 0; k < N; ++k) {
        left += static_cast<long double>(matrix.elements[i * N + k]) *
                inverse.elements[k * N + j];
        right += static_cast<long double>(inverse.elements[i * N + k]) *
                 matrix.elements[k * N + j];
      }
      const auto expected = i == j ? 1.L : 0.L;
      CHECK(std::abs(left - expected) <= tolerance<T>());
      CHECK(std::abs(right - expected) <= tolerance<T>());
    }
}

template <class T, std::size_t N> void check_scaled_inverses() {
  const std::array<T, 6> scales{std::numeric_limits<T>::min(),
                                std::numeric_limits<T>::min() / T{2},
                                std::numeric_limits<T>::max(),
                                std::numeric_limits<T>::max() / T{2},
                                T{1},
                                T{0.5}};
  for (const auto scale : scales) {
    sai::matrix<T, N> value;
    for (std::size_t i = 0; i < N; ++i)
      value.elements[i * N + i] = scale;
    const auto inverse = sai::matrix_inverse(value);
    INFO("uniform scale: ", scale, ", order: ", N);
    REQUIRE(inverse);
    check_inverse_identity(value, *inverse);
    for (std::size_t i = 0; i < N; ++i) {
      const auto expected = 1.L / static_cast<long double>(scale);
      CHECK(std::abs(static_cast<long double>(inverse->elements[i * N + i]) /
                         expected -
                     1.L) <= tolerance<T>());
    }
  }
  sai::matrix<T, N> too_small;
  for (std::size_t i = 0; i < N; ++i)
    too_small.elements[i * N + i] = std::numeric_limits<T>::denorm_min();
  const auto overflow = sai::matrix_inverse(too_small);
  REQUIRE_FALSE(overflow);
  CHECK(overflow.error().code == sai::error_code::invalid_value);
}

template <class T, std::size_t N> void check_algebra_identities() {
  std::mt19937 generator{37119};
  for (int example = 0; example < 40; ++example) {
    sai::matrix<T, N> a, b;
    for (std::size_t row = 0; row < N; ++row)
      for (std::size_t column = 0; column < N; ++column) {
        a.elements[row * N + column] =
            T(static_cast<int>(generator() % 17) - 8) / T{16} +
            (row == column ? T{4} : T{0});
        b.elements[row * N + column] =
            T(static_cast<int>(generator() % 17) - 8) / T{16} +
            (row == column ? T{4} : T{0});
      }
    const auto inverse = sai::matrix_inverse(a);
    REQUIRE(inverse);
    check_inverse_identity(a, *inverse);
    check_close(sai::matrix_multiply(a, sai::matrix<T, N>{}).value(), a);
    check_close(sai::matrix_multiply(sai::matrix<T, N>{}, a).value(), a);
    const auto product = sai::matrix_multiply(a, b).value();
    const auto transposed_product = sai::matrix_transpose(product).value();
    const auto reverse_transposes =
        sai::matrix_multiply(sai::matrix_transpose(b).value(),
                             sai::matrix_transpose(a).value())
            .value();
    check_close(transposed_product, reverse_transposes);
    const auto product_inverse = sai::matrix_inverse(product);
    REQUIRE(product_inverse);
    check_close(
        *product_inverse,
        sai::matrix_multiply(sai::matrix_inverse(b).value(), *inverse).value());
    check_close(sai::matrix_inverse(*inverse).value(), a);
    // Exact nonduplicate dependent rows, not just the easy zero/duplicate case.
    auto dependent = a;
    for (std::size_t column = 0; column < N; ++column) {
      dependent.elements[(N - 1) * N + column] =
          a.elements[column] + T{2} * a.elements[N + column];
      if constexpr (N == 4)
        dependent.elements[(N - 1) * N + column] -=
            T{3} * a.elements[2 * N + column];
    }
    CHECK_FALSE(sai::matrix_inverse(dependent));
  }
}
} // namespace

TEST_CASE_TEMPLATE(
    "matrix components and storage orders are explicit owning values", T, float,
    double) {
  const std::array<T, 9> rows{1, 2, 3, 4, 5, 6, 7, 8, 9};
  const std::array<T, 9> columns{1, 4, 7, 2, 5, 8, 3, 6, 9};
  const auto value = sai::matrix_from_row_major<T, 3>(rows).value();
  CHECK(value.elements == rows);
  CHECK(sai::matrix_to_row_major(value).value() == rows);
  CHECK(sai::matrix_to_column_major(value).value() == columns);
  CHECK(sai::matrix_from_column_major<T, 3>(columns).value() == value);
  CHECK(sai::matrix_component(value, 0, 1).value() == T{2});
  CHECK(sai::matrix_component(value, 1, 0).value() == T{4});
  const auto changed = sai::with_matrix_component(value, 0, 1, T{-7}).value();
  CHECK(changed.elements[1] == T{-7});
  CHECK(value.elements == rows);
  CHECK(sai::matrix_component(value, 3, 0).error().code ==
        sai::error_code::index_out_of_range);
  CHECK(sai::matrix_component(value, 0, 3).error().operation ==
        "matrix_component");
  CHECK(sai::with_matrix_component(value, 0, 3, T{0}).error().code ==
        sai::error_code::index_out_of_range);
  CHECK_FALSE(
      sai::matrix_component(value, std::numeric_limits<std::size_t>::max(), 0));
  CHECK_FALSE(
      sai::matrix_from_row_major<T, 3>(std::span<const T>(rows).first(8)));
  CHECK_FALSE(
      sai::matrix_from_column_major<T, 3>(std::span<const T>(rows).first(8)));
  const std::array<T, 10> oversized{};
  CHECK_FALSE(sai::matrix_from_row_major<T, 3>(oversized));
  CHECK_FALSE(sai::matrix_from_column_major<T, 3>(oversized));
  CHECK_FALSE(sai::matrix_from_row_major<T, 4>(rows));

  const std::array<T, 16> rows4{1, 2,  3,  4,  5,  6,  7,  8,
                                9, 10, 11, 12, 13, 14, 15, 16};
  const std::array<T, 16> columns4{1, 5, 9,  13, 2, 6, 10, 14,
                                   3, 7, 11, 15, 4, 8, 12, 16};
  const auto value4 = sai::matrix_from_column_major<T, 4>(columns4).value();
  CHECK(value4.elements == rows4);
  CHECK(sai::matrix_to_column_major(value4).value() == columns4);
  CHECK(sai::matrix_from_row_major<T, 4>(rows4).value() == value4);
  CHECK(sai::matrix_component(value4, 3, 2).value() == T{15});
  CHECK_FALSE(sai::with_matrix_component(value4, 4, 0, T{0}));
  CHECK_FALSE(sai::matrix_component(value4, 0, 4));
}

TEST_CASE_TEMPLATE(
    "matrix transpose moves nonsymmetric components and preserves signed zero",
    T, float, double) {
  const sai::matrix<T, 3> original{{1, 2, 3, 4, 5, 6, 7, 8, 9}};
  const sai::matrix<T, 3> expected{{1, 4, 7, 2, 5, 8, 3, 6, 9}};
  CHECK(sai::matrix_transpose(original).value() == expected);
  CHECK(sai::matrix_transpose(expected).value() == original);
  const sai::matrix<T, 4> original4{
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};
  const sai::matrix<T, 4> expected4{
      {1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16}};
  CHECK(sai::matrix_transpose(original4).value() == expected4);
  CHECK(sai::matrix_transpose(expected4).value() == original4);
  auto signed_zero = original;
  signed_zero.elements[1] = -T{0};
  CHECK(std::signbit(sai::matrix_transpose(signed_zero)->elements[3]));
  CHECK(std::signbit(sai::matrix_to_column_major(signed_zero)->at(3)));
}

TEST_CASE_TEMPLATE(
    "matrix products and both vector orientations match hand arithmetic", T,
    float, double) {
  const sai::matrix<T, 3> a{{1, 2, 3, 0, 1, 4, 5, 6, 0}};
  const sai::matrix<T, 3> b{{2, -1, 0, 3, 0, 1, 4, 2, 1}};
  const sai::matrix<T, 3> ab{{20, 5, 5, 19, 8, 5, 28, -5, 6}};
  const sai::matrix<T, 3> ba{{2, 3, 2, 8, 12, 9, 9, 16, 20}};
  CHECK(sai::matrix_multiply(a, b).value() == ab);
  CHECK(sai::matrix_multiply(b, a).value() == ba);
  CHECK(sai::matrix_multiply_right(a, b).value() == ab);
  CHECK(sai::matrix_multiply_left(a, b).value() == ba);
  const sai::matrix_vector<T, 3> v{2, -1, 3};
  CHECK(sai::matrix_multiply_column_vector(a, v).value() ==
        sai::matrix_vector<T, 3>{9, 11, 4});
  CHECK(sai::matrix_multiply_row_vector(a, v).value() ==
        sai::matrix_vector<T, 3>{17, 21, 2});
  CHECK(sai::matrix_multiply_column_vector(sai::matrix_transpose(a).value(), v)
            .value() == sai::matrix_multiply_row_vector(a, v).value());
  CHECK(a.elements[1] == T{2});
  CHECK(b.elements[1] == T{-1});

  const sai::matrix<T, 4> a4{
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};
  const sai::matrix_vector<T, 4> v4{2, -1, 3, 4};
  CHECK(sai::matrix_multiply_column_vector(a4, v4).value() ==
        sai::matrix_vector<T, 4>{25, 57, 89, 121});
  CHECK(sai::matrix_multiply_row_vector(a4, v4).value() ==
        sai::matrix_vector<T, 4>{76, 84, 92, 100});
}

TEST_CASE_TEMPLATE(
    "noncommuting affine matrices distinguish pre post row and column action",
    T, float, double) {
  const sai::matrix<T, 4> translation{
      {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 20, 30, 1}};
  const sai::matrix<T, 4> scale{
      {2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 1}};
  const sai::matrix_vector<T, 4> point{1, 2, 3, 1};
  const auto ts = sai::matrix_multiply_right(translation, scale).value();
  const auto st = sai::matrix_multiply_left(translation, scale).value();
  CHECK(sai::matrix_multiply_row_vector(ts, point).value() ==
        sai::matrix_vector<T, 4>{22, 66, 132, 1});
  CHECK(sai::matrix_multiply_row_vector(st, point).value() ==
        sai::matrix_vector<T, 4>{12, 26, 42, 1});
  const auto column_translation = sai::matrix_transpose(translation).value();
  const auto column_ts =
      sai::matrix_multiply(column_translation, scale).value();
  const auto column_st =
      sai::matrix_multiply(scale, column_translation).value();
  CHECK(sai::matrix_multiply_column_vector(column_ts, point).value() ==
        sai::matrix_vector<T, 4>{12, 26, 42, 1});
  CHECK(sai::matrix_multiply_column_vector(column_st, point).value() ==
        sai::matrix_vector<T, 4>{22, 66, 132, 1});
  const sai::matrix_vector<T, 4> direction{1, 2, 3, 0};
  CHECK(sai::matrix_multiply_row_vector(translation, direction).value() ==
        direction);
  CHECK(sai::matrix_multiply_row_vector(translation,
                                        sai::matrix_vector<T, 4>{1, 2, 3, 2})
            .value() == sai::matrix_vector<T, 4>{21, 42, 63, 2});

  // The same convention holds for 2D homogeneous transforms in a 3x3 value.
  const sai::matrix<T, 3> translation2{{1, 0, 0, 0, 1, 0, 10, 20, 1}};
  const sai::matrix<T, 3> scale2{{2, 0, 0, 0, 3, 0, 0, 0, 1}};
  CHECK(sai::matrix_multiply_row_vector(
            sai::matrix_multiply(translation2, scale2).value(),
            sai::matrix_vector<T, 3>{1, 2, 1})
            .value() == sai::matrix_vector<T, 3>{22, 66, 1});
  CHECK(sai::matrix_multiply_row_vector(
            sai::matrix_multiply(scale2, translation2).value(),
            sai::matrix_vector<T, 3>{1, 2, 1})
            .value() == sai::matrix_vector<T, 3>{12, 26, 1});
}

TEST_CASE_TEMPLATE(
    "matrix inverses match known nonsymmetric answers and require pivoting", T,
    float, double) {
  const sai::matrix<T, 3> a{{1, 2, 3, 0, 1, 4, 5, 6, 0}};
  const sai::matrix<T, 3> expected{{-24, 18, 5, 20, -15, -4, -5, 4, 1}};
  const auto inverse = sai::matrix_inverse(a);
  REQUIRE(inverse);
  check_close(*inverse, expected);
  check_inverse_identity(a, *inverse);
  const sai::matrix<T, 4> a4{{1, 2, 3, 4, 0, 1, 5, 6, 0, 0, 1, 7, 0, 0, 0, 1}};
  const sai::matrix<T, 4> expected4{
      {1, -2, 7, -41, 0, 1, -5, 29, 0, 0, 1, -7, 0, 0, 0, 1}};
  const auto inverse4 = sai::matrix_inverse(a4);
  REQUIRE(inverse4);
  check_close(*inverse4, expected4);
  check_inverse_identity(a4, *inverse4);
  const sai::matrix<T, 3> swap{{0, 1, 0, 1, 0, 0, 0, 0, 1}};
  CHECK(sai::matrix_inverse(swap).value() == swap);
  const sai::matrix<T, 4> swap4{
      {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0}};
  CHECK(sai::matrix_inverse(swap4).value() == swap4);
}

TEST_CASE_TEMPLATE("inverse validation is scale aware for extreme identities",
                   T, float, double) {
  check_scaled_inverses<T, 3>();
  check_scaled_inverses<T, 4>();
}

TEST_CASE_TEMPLATE("inverse rejects singular matrices and inaccurate rounded "
                   "inverses unchanged",
                   T, float, double) {
  const sai::matrix<T, 3> duplicate{{1, 2, 3, 4, 5, 6, 1, 2, 3}};
  const auto rejected = sai::matrix_inverse(duplicate);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_value);
  CHECK(rejected.error().operation == "matrix_inverse");
  CHECK_FALSE(rejected.error().message.empty());
  CHECK_FALSE(sai::matrix_inverse(sai::matrix<T, 3>{std::array<T, 9>{}}));
  CHECK_FALSE(sai::matrix_inverse(sai::matrix<T, 4>{std::array<T, 16>{}}));
  const auto delta = std::nextafter(T{1}, T{2});
  const sai::matrix<T, 3> almost_dependent{{1, 1, 0, 1, delta, 0, 0, 0, 1}};
  const auto exact_ill_conditioned = sai::matrix_inverse(almost_dependent);
  REQUIRE(exact_ill_conditioned);
  check_inverse_identity(almost_dependent, *exact_ill_conditioned);
  auto ill_scaled = sai::matrix<T, 4>{};
  ill_scaled.elements[0] = std::numeric_limits<T>::epsilon() / T{16};
  const auto ill_scaled_inverse = sai::matrix_inverse(ill_scaled);
  REQUIRE(ill_scaled_inverse);
  check_inverse_identity(ill_scaled, *ill_scaled_inverse);
  CHECK(ill_scaled.elements[0] == std::numeric_limits<T>::epsilon() / T{16});
  CHECK(duplicate.elements == std::array<T, 9>{1, 2, 3, 4, 5, 6, 1, 2, 3});
  const auto epsilon = std::numeric_limits<T>::epsilon();
  const sai::matrix<T, 3> poor_rounded_inverse{
      {1, 1, 1, 1, T{1} + epsilon, 1, 1, 1, T{1} + T{3} * epsilon}};
  const auto poor_result = sai::matrix_inverse(poor_rounded_inverse);
  REQUIRE_FALSE(poor_result);
  CHECK(poor_result.error().code == sai::error_code::invalid_value);
  CHECK(poor_result.error().operation == "matrix_inverse");
}

TEST_CASE_TEMPLATE(
    "inverse accepts exact large translations regardless of condition number",
    T, float, double) {
  for (const auto distance :
       {T{1000000}, std::ldexp(T{1}, std::same_as<T, float> ? 24 : 40)}) {
    const sai::matrix<T, 4> a{{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, distance,
                               -T{2} * distance, T{3} * distance, 1}};
    const auto inverse = sai::matrix_inverse(a);
    REQUIRE(inverse);
    CHECK(inverse->elements[12] == -distance);
    CHECK(inverse->elements[13] == T{2} * distance);
    CHECK(inverse->elements[14] == -T{3} * distance);
    check_inverse_identity(a, *inverse);
    const auto column = sai::matrix_transpose(a).value();
    const auto column_inverse = sai::matrix_inverse(column);
    REQUIRE(column_inverse);
    CHECK(*column_inverse == sai::matrix_transpose(*inverse).value());
    check_inverse_identity(column, *column_inverse);
    const sai::matrix<T, 3> a2{
        {1, 0, 0, 0, 1, 0, distance, -T{2} * distance, 1}};
    const auto inverse2 = sai::matrix_inverse(a2);
    REQUIRE(inverse2);
    check_inverse_identity(a2, *inverse2);
  }
}

TEST_CASE_TEMPLATE(
    "matrix algebra satisfies independent inverse and transpose identities", T,
    float, double) {
  check_algebra_identities<T, 3>();
  check_algebra_identities<T, 4>();
}

TEST_CASE_TEMPLATE(
    "matrix helpers reject nonfinite inputs and out-of-range outputs", T, float,
    double) {
  for (const auto invalid :
       {std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(),
        std::numeric_limits<T>::quiet_NaN()}) {
    auto a = sai::matrix<T, 3>{};
    a.elements[5] = invalid;
    CHECK_FALSE(sai::matrix_component(a, 0, 0));
    CHECK_FALSE(sai::with_matrix_component(a, 1, 2, T{0}));
    CHECK_FALSE(sai::with_matrix_component(sai::matrix<T, 3>{}, 0, 0, invalid));
    CHECK_FALSE(sai::matrix_to_row_major(a));
    CHECK_FALSE(sai::matrix_to_column_major(a));
    CHECK_FALSE(sai::matrix_from_row_major<T, 3>(a.elements));
    CHECK_FALSE(sai::matrix_from_column_major<T, 3>(a.elements));
    CHECK_FALSE(sai::matrix_transpose(a));
    CHECK_FALSE(sai::matrix_inverse(a));
    CHECK_FALSE(sai::matrix_multiply(a, sai::matrix<T, 3>{}));
    CHECK_FALSE(sai::matrix_multiply(sai::matrix<T, 3>{}, a));
    CHECK(sai::matrix_multiply_left(a, a).error().operation ==
          "matrix_multiply_left");
    CHECK(sai::matrix_multiply_right(a, a).error().operation ==
          "matrix_multiply_right");
    CHECK_FALSE(sai::matrix_multiply_row_vector(a, sai::matrix_vector<T, 3>{}));
    CHECK_FALSE(
        sai::matrix_multiply_column_vector(a, sai::matrix_vector<T, 3>{}));
    CHECK_FALSE(sai::matrix_multiply_row_vector(
        sai::matrix<T, 3>{}, sai::matrix_vector<T, 3>{invalid, 0, 0}));
    CHECK_FALSE(sai::matrix_multiply_column_vector(
        sai::matrix<T, 4>{}, sai::matrix_vector<T, 4>{0, 0, 0, invalid}));
  }
  auto large = sai::matrix<T, 4>{};
  large.elements[0] = std::numeric_limits<T>::max();
  auto twice = sai::matrix<T, 4>{};
  twice.elements[0] = T{2};
  const auto overflow = sai::matrix_multiply(large, twice);
  REQUIRE_FALSE(overflow);
  CHECK(overflow.error().code == sai::error_code::invalid_value);
  CHECK_FALSE(sai::matrix_multiply_column_vector(
      large, sai::matrix_vector<T, 4>{2, 0, 0, 1}));
  CHECK_FALSE(sai::matrix_multiply_row_vector(
      large, sai::matrix_vector<T, 4>{2, 0, 0, 1}));
  CHECK(large.elements[0] == std::numeric_limits<T>::max());
}

TEST_CASE("double matrix helpers never narrow through float") {
  const double distinct = 1.0 + std::ldexp(1.0, -40);
  auto a = sai::matrix4d{};
  a.elements[0] = distinct;
  CHECK(sai::matrix_multiply(a, sai::matrix4d{})->elements[0] == distinct);
  CHECK(sai::matrix_multiply_column_vector(a, sai::vec4d{1, 0, 0, 0})->x ==
        distinct);
  CHECK(sai::matrix_inverse(a)->elements[0] == 1.0 / distinct);
  CHECK(sai::matrix_inverse(a)->elements[0] != 1.0);
  auto f = sai::matrix3f{};
  f.elements[0] = 3.F;
  CHECK(sai::matrix_inverse(f)->elements[0] == 1.F / 3.F);
}

TEST_CASE("inverse residual validation retains product rounding errors under "
          "cancellation") {
  const sai::matrix4d value{{1, 8192, 0, 0, 0, -17592186044415., 32768, 0, 0,
                             -536870912., 1, 0, 0, 2748779069440., -5120, 1}};
  // Ordinary long-double dot products miss the returned candidate's exact
  // (inverse*A-I)[2,1] = 1/16384, independently verified with rational
  // arithmetic. Reject that inaccurate inverse even though the true inverse is
  // representable.
  const auto inverse = sai::matrix_inverse(value);
  REQUIRE_FALSE(inverse);
  CHECK(inverse.error().code == sai::error_code::invalid_value);
  CHECK(inverse.error().operation == "matrix_inverse");
}

TEST_CASE("inverse validates its required floating point rounding mode") {
  const auto previous = std::fegetround();
  if (std::fesetround(FE_DOWNWARD) == 0) {
    const auto inverse = sai::matrix_inverse(sai::matrix4d{});
    const auto restored = std::fesetround(previous);
    REQUIRE(restored == 0);
    REQUIRE_FALSE(inverse);
    CHECK(inverse.error().code == sai::error_code::invalid_value);
  }
}
