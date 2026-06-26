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
