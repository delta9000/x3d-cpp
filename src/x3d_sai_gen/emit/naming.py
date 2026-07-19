"""The single naming canonicalizer.

One source of truth for turning X3D field names into valid C++ identifiers and
for the PascalCase accessor-name transform. Both the parser and the emit/backend
layers import from here so the rules can never drift apart.
"""

# C++ reserved keywords. A field whose sanitized name collides with one of these
# is suffixed with '_' so generated members/accessors stay well-formed C++.
CPP_RESERVED_KEYWORDS = {
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
    "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
    "char", "char16_t", "char32_t", "class", "compl", "concept", "const",
    "constexpr", "const_cast", "continue", "co_await", "co_return", "co_yield",
    "decltype", "default", "delete", "do", "double", "dynamic_cast", "else",
    "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
    "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
    "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
    "protected", "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast",
    "struct", "switch", "template", "this", "thread_local", "throw", "true",
    "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
    "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
}


def sanitize_field_name(field_name: str) -> str:
    """Canonical field-name sanitizer.

    Replaces '-' with '_' AND suffixes C++ reserved keywords with '_' so that
    generated member/accessor names are always valid C++ identifiers.
    """
    name = (field_name or "").replace("-", "_")
    if name in CPP_RESERVED_KEYWORDS:
        return f"{name}_"
    return name


def pascal(s: str) -> str:
    """Capitalize only the first character, preserving the rest (camelCase safe).

    Unlike Jinja's `capitalize`, this does NOT lowercase the tail, so
    'bboxCenter' -> 'BboxCenter' (yielding getBboxCenter, not getBboxcenter).
    """
    return (s[:1].upper() + s[1:]) if s else s


def cpp_str(s: str) -> str:
    """Escape a Python string into the body of a C++ double-quoted literal.

    Escapes backslash, double-quote, newline, tab, and carriage return -- the
    union of what this generator's two previously-separate, inconsistent
    escaping implementations covered (emit.defaults.cpp_string_literal used
    to escape all five; generator._cpp_str used to escape only backslash and
    quote). One shared implementation now backs every quoted interpolation
    site, including Jinja templates via the registered ``cpp_str`` filter.
    """
    if s is None:
        return ""
    out = []
    for ch in s:
        if ch == '\\':
            out.append('\\\\')
        elif ch == '"':
            out.append('\\"')
        elif ch == '\n':
            out.append('\\n')
        elif ch == '\t':
            out.append('\\t')
        elif ch == '\r':
            out.append('\\r')
        else:
            out.append(ch)
    return ''.join(out)
