"""emit.defaults: enum-keyed default-literal generation (moved out of Jinja)."""

import pytest

from x3d_cpp_gen.emit.defaults import (
    default_expr_for,
    cpp_string_literal,
    tokenize_mfstring,
)
from x3d_cpp_gen.model.types import X3DType


# default_expr_for keys on the X3DType enum, not on raw type-name strings.

def test_sfbool_true_false_casing():
    assert default_expr_for(X3DType.SFBool, "true") == "true"
    assert default_expr_for(X3DType.SFBool, "TRUE") == "true"
    assert default_expr_for(X3DType.SFBool, "false") == "false"
    assert default_expr_for(X3DType.SFBool, "garbage") == "false"


def test_sfvec3f_and_arity_padding():
    assert default_expr_for(X3DType.SFVec3f, "2 2 2") == "SFVec3f{2, 2, 2}"
    assert default_expr_for(X3DType.SFVec3f, "1") == "SFVec3f{1, 0.0f, 0.0f}"


def test_sfvec3d_double_zero_pad():
    assert default_expr_for(X3DType.SFVec3d, "1 2") == "SFVec3d{1, 2, 0.0}"


def test_sfcolor_distinct_from_vec3f():
    # SFColor is its own struct even though it is three floats.
    assert default_expr_for(X3DType.SFColor, "1 0 0") == "SFColor{1, 0, 0}"


def test_sfmatrix4f_identity_is_row_braced():
    # SFMatrix4f wraps a nested `float matrix[4][4]` member, unlike the flat
    # vector/colour/rotation structs -- eliding braces around the flat
    # 16-value list (or wrapping it in just one extra brace) is legal C++ but
    # trips -Werror=missing-braces under Clang even though GCC accepts both
    # forms silently. Each row needs its own brace, verified directly against
    # both compilers with -Wmissing-braces -Werror.
    identity = "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
    assert (default_expr_for(X3DType.SFMatrix4f, identity)
            == "SFMatrix4f{{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}}")


def test_sfmatrix3f_and_double_variants_row_braced():
    identity3 = "1 0 0 0 1 0 0 0 1"
    assert (default_expr_for(X3DType.SFMatrix3f, identity3)
            == "SFMatrix3f{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}")
    assert (default_expr_for(X3DType.SFMatrix3d, identity3)
            == "SFMatrix3d{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}")
    identity4 = "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
    assert (default_expr_for(X3DType.SFMatrix4d, identity4)
            == "SFMatrix4d{{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}}")


def test_sfmatrix4f_arity_padding_still_row_braced():
    # Arity guard (zero-pad short defaults) still applies; the row chunking
    # doesn't change based on how many values were actually supplied.
    z = "0.0f"
    row0 = "{1, " + z + ", " + z + ", " + z + "}"
    other_row = "{" + ", ".join([z] * 4) + "}"
    expected = "SFMatrix4f{{" + ", ".join([row0] + [other_row] * 3) + "}}"
    assert default_expr_for(X3DType.SFMatrix4f, "1") == expected


def test_flat_structs_stay_single_braced_not_double():
    # Regression guard: only the matrix types get the extra brace. A flat
    # struct double-braced would itself be a -Wmissing-braces mismatch the
    # other way (too many braces around a scalar member list).
    assert default_expr_for(X3DType.SFRotation, "0 1 0 0") == "SFRotation{0, 1, 0, 0}"
    assert default_expr_for(X3DType.SFVec4d, "1 2 3 4") == "SFVec4d{1, 2, 3, 4}"


def test_sfstring_quoting_and_escaping():
    assert default_expr_for(X3DType.SFString, "hi") == '"hi"'
    assert default_expr_for(X3DType.SFString, 'a"b') == '"a\\"b"'


def test_sfnode_nullptr():
    assert default_expr_for(X3DType.SFNode, "anything") == "nullptr"


def test_no_default_is_none():
    assert default_expr_for(X3DType.SFVec3f, None) is None
    assert default_expr_for(X3DType.SFBool, None) is None


def test_mf_scalar_and_struct():
    assert default_expr_for(X3DType.MFInt32, "1 2 3") == "std::vector<int>{1, 2, 3}"
    assert (default_expr_for(X3DType.MFVec3f, "1 2 3")
            == "std::vector<SFVec3f>{SFVec3f{1, 2, 3}}")


def test_mfstring_quoted_and_empty():
    assert (default_expr_for(X3DType.MFString, '"a b" "c"')
            == 'std::vector<std::string>{"a b", "c"}')
    assert default_expr_for(X3DType.MFString, "") == "std::vector<std::string>{}"


def test_xs_nmtoken_quoted_like_string():
    assert default_expr_for(X3DType.XS_NMTOKEN, "MIDDLE") == '"MIDDLE"'


def test_mfnode_value_initializes():
    # MFNode has no element-literal form -> value-init (None).
    assert default_expr_for(X3DType.MFNode, "x") is None


# Moved string helpers.

def test_cpp_string_literal_escapes():
    assert cpp_string_literal('a"b\\c') == 'a\\"b\\\\c'


def test_tokenize_mfstring_quote_aware():
    assert tokenize_mfstring('"a b" "c"') == ["a b", "c"]
    assert tokenize_mfstring("a b c") == ["a", "b", "c"]
    assert tokenize_mfstring("") == []
    assert tokenize_mfstring(None) == []
