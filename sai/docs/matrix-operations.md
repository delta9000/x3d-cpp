# Owning matrix algebra: supported subset and conventions

[`matrix.hpp`](../include/x3d/sai/experimental/matrix.hpp) adds header-only,
checked algebra to the existing owning `matrix3f`, `matrix3d`, `matrix4f`, and
`matrix4d` values. It uses modern free functions returning `result<T>`, requires
no compiled kernel or provider, and never changes its arguments. The returned
matrix can be assigned by the caller or published through an ordinary field
write; these helpers do not bypass field access, buffering, or events.

This is a partial implementation of the SAI matrix-service obligations, not a
claim of full clause 6.12 or published C++ interface conformance.

## Source mapping and unresolved discrepancies

The sources used are:

- [Abstract SAI, 6.12](https://www.web3d.org/documents/specifications/19775-2/V3.3/Part02/servRef.html#MatrixServices):
  3×3 and 4×4 matrix services, both input precisions, transform set/get,
  inverse, transpose, and multiplication on both sides
- [Published C++ binding, 7.10](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/functions.html#MatrixServices)
  and [Annex A.5.4–A.5.5](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/abstracts.html#Matrix3):
  identity initialization, component access, named left/right and row/column
  multiplication, and transform operations
- [Architecture, 5.3.8–5.3.11](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html#SFMatrix3dAndMFMatrix3d):
  row-major SFMatrix field values and identity defaults

Where this SDK's choices differ from the sources, they are listed here rather
than silently resolved:

1. Abstract 6.12.1 describes service matrices as column-major; the architectural
   SFMatrix fields are row-major. This SDK extends the existing row-major
   owning field values and provides explicit column-major import/export. It
   does not claim its in-memory layout is the abstract service's layout.
2. Storage order and vector orientation are separate choices. `elements[r*N+c]`
   always denotes mathematical entry `(r,c)`. Converting serialization order
   does not transpose that mathematical matrix. Changing a transform from row-
   to column-vector action requires a mathematical transpose, not merely a
   storage conversion.
3. The published C++ text declares `set(int row, int column)` without a value
   and lists `Matrix3` results for some Matrix4 algebra and vector operations,
   which reads like an editorial slip worth reporting. This SDK takes a value
   argument and returns the matching matrix or vector type.
4. Abstract inverse, transpose, and matrix multiplication operate in place;
   these modern helpers return new owning values. No partial mutation occurs on
   failure. Both binary32 and binary64 values are supported; binary32 results
   intentionally retain their field precision rather than claiming all storage
   is double precision.

| Source obligation | Implemented mapping | Scope |
| --- | --- | --- |
| Identity initialization, C++ 7.10 | Existing `matrix3f{}`, `matrix3d{}`, `matrix4f{}`, `matrix4d{}` | All four default to identity; assignment resets a value |
| Component get/set, C++ 7.10 | `matrix_component`, `with_matrix_component` | Checked zero-based indices; functional update |
| Inverse, abstract 6.12.4 | `matrix_inverse` | Finite, numerically accepted matrices; explicit failure policy below |
| Transpose, abstract 6.12.5 | `matrix_transpose` | All finite supported values |
| Both multiplication directions, abstract 6.12.6 | `matrix_multiply_left`, `matrix_multiply_right` | Exact same scalar type and order |
| Both vector directions, abstract 6.12.7 | `matrix_multiply_row_vector`, `matrix_multiply_column_vector` | 3×3 with vec3, 4×4 with explicit vec4 |
| Transform set/get, abstract 6.12.2–6.12.3 | Open | Component access is not transform composition/decomposition |

There is no transform-parameter composition/decomposition API in this header.
In particular, it does not guess a unique rotation, scale orientation, center,
reflection convention, or shear decomposition. The implicit homogeneous
coordinate and perspective-division behavior of the source's 4×4/SFVec3 vector
signatures also remains open. The explicit vec4 operation is complete linear
algebra but is not presented as full coverage of that source signature.

## API and shape rules

Include `x3d/sai/experimental/matrix.hpp`; all names are in
`x3d::sai::experimental`.

- `matrix_from_row_major<T,N>(span)` and
  `matrix_from_column_major<T,N>(span)` require exactly `N*N` components and
  return `result<matrix<T,N>>`
- `matrix_to_row_major(value)` and `matrix_to_column_major(value)` return
  owning `result<std::array<T,N*N>>` values
- `matrix_component(value,row,column)` returns `result<T>`;
  `with_matrix_component(value,row,column,component)` returns a new matrix
- `matrix_transpose(value)` and `matrix_inverse(value)` return new matrices
- `matrix_multiply(A,B)` returns `A*B`
- `matrix_multiply_left(current,left)` returns `left*current`
- `matrix_multiply_right(current,right)` returns `current*right`
- `matrix_multiply_column_vector(M,v)` returns `M*v`, with component
  `out[i] = sum(M[i,k]*v[k])`
- `matrix_multiply_row_vector(M,v)` returns `v*M`, with component
  `out[j] = sum(v[k]*M[k,j])`

`matrix_vector<T,N>` names the corresponding existing owning `vec3f`, `vec3d`,
`vec4f`, or `vec4d` type. Only float/double and orders 3/4 are accepted. Mixed
scalar types, mismatched orders, and mismatched vector types fail overload
resolution. There is no implicit precision conversion or dimension padding.
For a 4D homogeneous point use `w=1`; for a direction use `w=0`. The returned
`w` is preserved as computed; the helper neither assumes an affine transform
nor divides by `w`.

For example, row-vector translation lives in the last row, as described by the
architectural field type reference:

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

Reversing the matrix product gives `{12,26,42,1}` for that point. For column
vectors, `A*B` applies `B` first. These rules do not depend on import/export
storage order.

## Explicit numerical and error policy

The source service tables list no errors and do not prescribe a numerical
algorithm, singularity tolerance, or special-value handling. The following are
safety choices of this SDK, not added ISO requirements:

- Every helper requires all input components to be finite, including component
  reads and exports. A replacement component must also be finite. Raw matrix
  construction and generic field storage are unchanged; the header does not
  impose this arithmetic policy on the kernel's matrix fields
- Invalid indices return `error_code::index_out_of_range`. Incorrect component
  counts, nonfinite components, rejected inverses, and arithmetic range errors
  return `error_code::invalid_value`. Every error includes the public operation
  name and a diagnostic message; callers should branch on the code rather than
  depend on diagnostic wording
- Products accumulate in `long double`, then range-check before conversion to
  the requested float/double precision. Nonfinite intermediate results or
  results exceeding the destination's finite range fail. Ordinary rounding,
  including subnormal rounding/underflow, follows the platform floating-point
  environment; values are never clamped or silently normalized
- Inversion divides the input by its largest absolute component, then uses
  row-scaled partial-pivoted Gauss-Jordan elimination in `long double`. An all-zero matrix,
  a zero elimination pivot, or nonfinite elimination intermediate fails
- After conversion to the returned component precision, check both identity
  products in `long double`, retaining each product’s rounding error with FMA
  and summing a bounded error-free TwoSum expansion. Ordinary dot products can
  hide material errors during cancellation, so they are not used for this
  validation. Require both absolute infinity-norm residuals
  `norm_inf(A*inverse-I)` and `norm_inf(inverse*A-I)` to be at most
  `64*N*numeric_limits<T>::epsilon()`. A zero pivot, an unrepresentable inverse,
  or a failed residual check produces an error, not a pseudoinverse, diagonal
  perturbation, identity fallback, or normalization repair
- This checks the actual returned inverse rather than rejecting on condition
  number alone. Exactly representable large translations and diagonal inverses
  can succeed even when their condition numbers are large. Tiny and huge
  uniformly scaled identities also remain admissible when their inverses fit
  the result type. Conversely, an invertible matrix can be rejected when its
  rounded inverse does not approximate both identity products closely enough.
  Large translations combined with nonexact rotations/scales can expose such
  rounding, especially at float precision; double may be more suitable
- Inversion requires `FE_TONEAREST` rounding and rejects other rounding modes
  without changing the environment. The expansion arithmetic assumes standard
  floating-point semantics; unsafe reassociation/fast-math compilation is not
  supported
- `long double` precision/range is platform-dependent, including the residual
  evaluation. Nonfinite residual intermediates fail. Residual checks are not
  certified forward-error bounds; successful results do not promise exact
  arithmetic or cross-platform bitwise identity. The API does not offer an
  unchecked-inversion or tolerance-override escape hatch

Inputs are `const` and returned objects own their storage. Failure leaves the
input untouched and produces no candidate result to publish. These guarantees
also cover multiplying a value by itself and retaining results after the input
is changed.

## Verification

[`matrix_operations_test.cpp`](../tests/matrix_operations_test.cpp) covers all
four supported matrix types. Evidence includes independently calculated
nonsymmetric products and inverses, row swaps, noncommuting translations and
scales, row/column vector action, full homogeneous `w` arithmetic, explicit
storage conversions, signed zero, wrong shapes/indices, and float/double
precision preservation, required rounding mode, and an exact-rational regression
for multiplication error hidden by cancellation. Singular, inaccurate rounded-inverse, nonfinite and out-of-range
inputs have negative tests. Exact large translations and representable inverses
of ill-conditioned matrices have positive tests. Extreme uniformly scaled identities test that
inversion is scale-aware. Deterministic well-conditioned matrices exercise both
inverse identities using an independent scalar oracle, the transpose-product
identity, the inverse-product identity, and double inversion.

The test is standalone and needs no compiled SAI kernel:

```sh
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude -Ithird_party \
  tests/matrix_operations_test.cpp third_party/doctest/doctest_main.cpp \
  -o /tmp/sai-matrix-operations-test
/tmp/sai-matrix-operations-test
```

These tests establish this finite algebra subset and SDK policy, not the
still-open transform services or a complete browser/provider implementation.
