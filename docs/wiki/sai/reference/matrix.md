---
title: SAI matrix algebra
summary: Reference for the header-only checked matrix helpers, their conventions, numerical policy and source mapping.
tags: [sai, reference, values, math]
updated: 2026-11-14
related:
  - field-values.md
---

# SAI matrix algebra

Header: `<x3d/sai/experimental/matrix.hpp>`, namespace
`x3d::sai::experimental`. Header-only checked algebra over the owning
`matrix3f`, `matrix3d`, `matrix4f` and `matrix4d` values.

- Free functions returning `result<T>`.
- No compiled kernel or provider needed.
- Arguments are never changed; a failure produces no candidate result.
- The helpers do not bypass field access, buffering or events. Assign the
  result yourself or publish it through an ordinary field write.

Rules for matrix values stored in fields are in
[Field values and validity](field-values.md).

This is a partial implementation of the SAI matrix-service obligations.
Transform set/get is open (see [Source mapping](#source-mapping)).

## Conventions

| Convention | Rule |
|---|---|
| Storage | Row-major. `elements[r*N+c]` is always mathematical entry `(r,c)` |
| Import and export | Explicit row-major and column-major conversion; converting serialization order does not transpose the mathematical matrix |
| Vector orientation | Separate from storage order. Changing a transform from row- to column-vector action needs a mathematical transpose |
| Row-vector translation | Lives in the last row |
| Column vectors | `A*B` applies `B` first |

## Functions

| Function | Returns |
|---|---|
| `matrix_from_row_major<T,N>(span)`, `matrix_from_column_major<T,N>(span)` | `result<matrix<T,N>>`; needs exactly `N*N` components |
| `matrix_to_row_major(m)`, `matrix_to_column_major(m)` | Owning `result<std::array<T,N*N>>` |
| `matrix_component(m, row, column)` | `result<T>` |
| `with_matrix_component(m, row, column, value)` | New matrix |
| `matrix_transpose(m)`, `matrix_inverse(m)` | New matrix |
| `matrix_multiply(A, B)` | `A*B` |
| `matrix_multiply_left(current, left)` | `left*current` |
| `matrix_multiply_right(current, right)` | `current*right` |
| `matrix_multiply_column_vector(M, v)` | `M*v`: `out[i] = sum(M[i,k]*v[k])` |
| `matrix_multiply_row_vector(M, v)` | `v*M`: `out[j] = sum(v[k]*M[k,j])` |

`matrix_vector<T,N>` names the matching owning `vec3f`, `vec3d`, `vec4f` or
`vec4d` type.

## Shape rules

- Only `float`/`double` and orders 3 and 4.
- Mixed scalar types, mismatched orders and mismatched vector types fail
  overload resolution. There is no implicit precision conversion or dimension
  padding.
- For a 4D homogeneous point use `w=1`; for a direction use `w=0`. The returned
  `w` is preserved as computed. The helper neither assumes an affine transform
  nor divides by `w`.

## Example

```cpp
namespace sai = x3d::sai::experimental;
const sai::matrix4d translation{{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    10, 20, 30, 1}};
const sai::matrix4d scale{{
    2, 0, 0, 0,
    0, 3, 0, 0,
    0, 0, 4, 0,
    0, 0, 0, 1}};
const sai::vec4d point{1, 2, 3, 1};
const auto first_translate = sai::matrix_multiply_right(translation, scale);
if (first_translate) {
  const auto transformed =
      sai::matrix_multiply_row_vector(*first_translate, point);
  // On success: transformed == vec4d{22, 66, 132, 1}.
}
```

Reversing the matrix product gives `{12,26,42,1}` for that point.

## Errors

| Code | Cause |
|---|---|
| `index_out_of_range` | Invalid row or column |
| `invalid_value` | Wrong component count, non-finite component, rejected inverse, arithmetic range error |

Every error includes the public operation name and a diagnostic message.
Branch on the code, not the wording.

## Numerical policy

The source service tables list no errors and prescribe no numerical algorithm,
singularity tolerance or special-value handling. The following are safety
choices of this SDK.

### Inputs

Every helper requires finite input components, including component reads and
exports. A replacement component must be finite. Raw matrix construction and
generic field storage are unchanged; this arithmetic policy is not imposed on
the kernel's matrix fields.

### Products

Products accumulate in `long double`, then range-check before conversion to the
requested precision. Non-finite intermediates and results beyond the
destination's finite range fail. Ordinary rounding, including subnormal
rounding, follows the platform floating-point environment. Values are never
clamped or silently normalized.

### Inversion

| Step | Detail |
|---|---|
| Scaling | Divide the input by its largest absolute component |
| Elimination | Row-scaled partial-pivoted Gauss-Jordan in `long double` |
| Immediate failure | All-zero matrix, zero pivot, non-finite intermediate |
| Residual check | After conversion to the returned precision, both `norm_inf(A*inverse-I)` and `norm_inf(inverse*A-I)` must be at most `64*N*numeric_limits<T>::epsilon()` |
| Residual arithmetic | `long double`, keeping each product's rounding error with FMA and summing a bounded error-free TwoSum expansion |
| Rounding mode | Requires `FE_TONEAREST`; other modes are rejected without changing the environment |

A failed check is an error. There is no pseudoinverse, diagonal perturbation,
identity fallback, normalization repair, unchecked-inversion option or
tolerance override.

Consequences:

- The check tests the actual returned inverse and does not reject on condition
  number alone. Exactly representable large translations and diagonal inverses
  can succeed with large condition numbers. Tiny and huge uniformly scaled
  identities succeed when their inverses fit the result type.
- An invertible matrix can be rejected when its rounded inverse does not
  approximate both identity products closely enough. Large translations
  combined with inexact rotations or scales can expose this, especially at
  `float` precision; `double` may suit better.
- `long double` precision and range are platform-dependent. Residual checks are
  not certified forward-error bounds, and successful results do not promise
  exact arithmetic or cross-platform bitwise identity.
- Fast-math and unsafe reassociation are not supported.

## Source mapping

Sources:

- [Abstract SAI, 6.12](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#MatrixServices):
  3×3 and 4×4 matrix services, both input precisions, transform set/get,
  inverse, transpose and multiplication on both sides
- [Published C++ binding, 7.10](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#MatrixServices)
  and [Annex A.5.4–A.5.5](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/abstracts.html#Matrix3):
  identity initialization, component access, named left/right and row/column
  multiplication, transform operations
- [Architecture, 5.3.8–5.3.11](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html#SFMatrix3dAndMFMatrix3d):
  row-major SFMatrix field values and identity defaults

| Source obligation | Mapping | Scope |
|---|---|---|
| Identity initialization, C++ 7.10 | `matrix3f{}`, `matrix3d{}`, `matrix4f{}`, `matrix4d{}` | All four default to identity; assignment resets a value |
| Component get/set, C++ 7.10 | `matrix_component`, `with_matrix_component` | Checked zero-based indices; functional update |
| Inverse, abstract 6.12.4 | `matrix_inverse` | Finite, numerically accepted matrices |
| Transpose, abstract 6.12.5 | `matrix_transpose` | All finite supported values |
| Both multiplication directions, abstract 6.12.6 | `matrix_multiply_left`, `matrix_multiply_right` | Same scalar type and order |
| Both vector directions, abstract 6.12.7 | `matrix_multiply_row_vector`, `matrix_multiply_column_vector` | 3×3 with vec3, 4×4 with explicit vec4 |
| Transform set/get, abstract 6.12.2–6.12.3 | Open | Component access is not transform composition or decomposition |

### Where the SDK made a choice

1. Abstract 6.12.1 describes service matrices as column-major, and the
   architectural SFMatrix fields are row-major. The SDK extends the row-major
   owning field values and provides explicit column-major import and export.
   It does not claim its in-memory layout is the abstract service's layout.
2. The published C++ text declares `set(int row, int column)` without a value
   and lists `Matrix3` results for some Matrix4 algebra and vector operations.
   The SDK takes a value argument and returns the matching matrix or vector
   type.
3. Abstract inverse, transpose and multiplication operate in place. These
   helpers return new owning values. Binary32 results keep their field
   precision.

### Open items

- No transform-parameter composition or decomposition. The header does not
  guess a unique rotation, scale orientation, centre, reflection convention or
  shear decomposition.
- The implicit homogeneous coordinate and perspective-division behaviour of
  the source's 4×4/SFVec3 vector signatures. The explicit vec4 operation is
  complete linear algebra but is not presented as full coverage of that
  signature.

## Evidence

- `sai/tests/matrix_operations_test.cpp` (CTest `sai_matrix_operations`)
  covers all four matrix types: independently calculated nonsymmetric products
  and inverses, row swaps, noncommuting translations and scales, row and
  column vector action, full homogeneous `w` arithmetic, storage conversions,
  signed zero, wrong shapes and indices, precision preservation, the required
  rounding mode, and an exact-rational regression for multiplication error
  hidden by cancellation.
- Singular, inaccurate rounded-inverse, non-finite and out-of-range inputs
  have negative tests.
- Exact large translations, representable inverses of ill-conditioned matrices
  and extreme uniformly scaled identities have positive tests.

The test is standalone and needs no compiled SAI kernel. From `sai/`:

```sh
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude -Ithird_party \
  tests/matrix_operations_test.cpp third_party/doctest/doctest_main.cpp \
  -o /tmp/sai-matrix-operations-test
/tmp/sai-matrix-operations-test
```
