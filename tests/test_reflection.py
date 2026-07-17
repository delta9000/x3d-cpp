"""emit.reflection: the generated X3DReflection.hpp support header."""

from x3d_cpp_gen.emit.reflection import gen_reflection_header


def test_reflection_header_declares_parse_enum_tokens():
    header = gen_reflection_header()
    assert "parseEnumTokens" in header
    assert "std::vector<std::string> parseEnumTokens" in header


def test_reflection_header_still_declares_field_table_type():
    # Characterization check: this new helper must not replace/break the
    # existing FieldTable/FieldInfo declarations already in this header.
    header = gen_reflection_header()
    assert "FieldTable" in header
    assert "FieldInfo" in header
