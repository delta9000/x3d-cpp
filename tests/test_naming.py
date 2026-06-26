"""Naming / identifier-sanitization helpers."""

import pytest

from x3d_cpp_gen.generator import pascal
from x3d_cpp_gen.parser import sanitize_field_name, CPP_RESERVED_KEYWORDS
# emit.naming is the single canonicalizer; parser/generator must re-export it.
from x3d_cpp_gen.emit import naming as naming_mod


def test_naming_is_single_source_of_truth():
    # The functions exposed via parser/generator must BE the ones in emit.naming.
    assert sanitize_field_name is naming_mod.sanitize_field_name
    assert pascal is naming_mod.pascal
    assert CPP_RESERVED_KEYWORDS is naming_mod.CPP_RESERVED_KEYWORDS


def test_pascal_capitalizes_first_only():
    # Must NOT lowercase the tail (camelCase-safe), unlike str.capitalize().
    assert pascal("bboxCenter") == "BboxCenter"


@pytest.mark.parametrize(
    "raw,expected",
    [
        ("bboxCenter", "BboxCenter"),
        ("size", "Size"),
        ("URL", "URL"),
        ("x", "X"),
    ],
)
def test_pascal_examples(raw, expected):
    assert pascal(raw) == expected


def test_pascal_empty_string():
    assert pascal("") == ""


def test_sanitize_replaces_hyphen_with_underscore():
    assert sanitize_field_name("some-field") == "some_field"
    assert sanitize_field_name("a-b-c") == "a_b_c"


def test_sanitize_suffixes_reserved_keyword():
    assert sanitize_field_name("class") == "class_"
    assert sanitize_field_name("template") == "template_"
    assert sanitize_field_name("new") == "new_"


def test_sanitize_leaves_normal_names_unchanged():
    assert sanitize_field_name("bboxCenter") == "bboxCenter"
    assert sanitize_field_name("size") == "size"


def test_sanitize_none_is_safe():
    # _parse_fields may pass field.get('name', '') but guard against None too.
    assert sanitize_field_name(None) == ""


@pytest.mark.parametrize("kw", sorted(CPP_RESERVED_KEYWORDS))
def test_every_reserved_keyword_is_suffixed(kw):
    sanitized = sanitize_field_name(kw)
    assert sanitized == f"{kw}_"
    assert sanitized not in CPP_RESERVED_KEYWORDS
