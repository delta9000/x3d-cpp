"""compute_default_expr: X3D field default -> C++ initializer expression."""

from dataclasses import dataclass
from typing import Optional

import pytest

from x3d_cpp_gen.generator import compute_default_expr, tokenize_mfstring


@dataclass
class FakeField:
    """Minimal stand-in matching the attributes compute_default_expr reads."""

    type: str
    default: Optional[str] = None


def expr(type_, default):
    return compute_default_expr(FakeField(type=type_, default=default))


def test_sfbool_true():
    assert expr("SFBool", "true") == "true"


def test_sfbool_false_and_casing():
    assert expr("SFBool", "false") == "false"
    assert expr("SFBool", "TRUE") == "true"
    assert expr("SFBool", "garbage") == "false"


def test_sfvec3f_two_two_two():
    assert expr("SFVec3f", "2 2 2") == "SFVec3f{2, 2, 2}"


def test_sfvec3f_arity_padding():
    # Under-specified default is zero-padded, never read out of bounds.
    assert expr("SFVec3f", "1") == "SFVec3f{1, 0.0f, 0.0f}"


def test_sfvec3d_uses_double_zero_pad():
    assert expr("SFVec3d", "1 2") == "SFVec3d{1, 2, 0.0}"


def test_mfstring_quoted():
    assert expr("MFString", '"a b" "c"') == 'std::vector<std::string>{"a b", "c"}'


def test_mfstring_empty_default_is_empty_vector():
    # Empty string is not None, so it flows through; tokenizing "" yields no
    # tokens -> an empty vector literal.
    assert expr("MFString", "") == "std::vector<std::string>{}"
    assert expr("MFString", '""') == 'std::vector<std::string>{""}'


def test_sfstring_quoting_and_escaping():
    assert expr("SFString", "hello") == '"hello"'
    assert expr("SFString", 'a"b') == '"a\\"b"'


def test_sfint32_and_sffloat():
    assert expr("SFInt32", "42") == "42"
    assert expr("SFFloat", "3.5") == "3.5"


def test_sfnode_is_nullptr():
    assert expr("SFNode", "anything") == "nullptr"
    assert expr("std::shared_ptr<X3DNode>", "anything") == "nullptr"


def test_no_default_returns_none():
    assert expr("SFVec3f", None) is None
    assert expr("SFBool", None) is None


def test_mfint32_list():
    assert expr("MFInt32", "1 2 3") == "std::vector<int>{1, 2, 3}"


def test_mfvec3f_single_element():
    assert expr("MFVec3f", "1 2 3") == "std::vector<SFVec3f>{SFVec3f{1, 2, 3}}"


# tokenize_mfstring (quote-aware splitting)

def test_tokenize_quoted_multiword():
    assert tokenize_mfstring('"a b" "c"') == ["a b", "c"]


def test_tokenize_empty():
    assert tokenize_mfstring("") == []
    assert tokenize_mfstring(None) == []


def test_tokenize_unquoted_multiword_fallback():
    assert tokenize_mfstring("a b c") == ["a", "b", "c"]


def test_tokenize_single_quoted():
    assert tokenize_mfstring('"CENTER"') == ["CENTER"]
