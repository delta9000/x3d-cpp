"""Generate private schema tags for the instance-free SAI metadata catalog.

These tags are implementation details, not x3d-cpp runtime reflection types.
The SAI catalog needs no runtime nodes, callbacks, field tables, or visitors.
"""

from x3d_cpp_gen.model.types import X3DType, TypeRegistry


def gen_reflection_header() -> str:
    """Render the private metadata vocabulary, never a runtime reflection API."""
    tags = list(dict.fromkeys(TypeRegistry.runtime_tag(member) for member in X3DType))
    # Legacy storage tags remain recognizable so the adapter can fail closed.
    tags += [tag for tag in ("SFEnum", "MFEnum") if tag not in tags]
    enumerators = "\n".join(f"    {tag}," for tag in tags)
    return f"""// metadata_types.hpp
// Auto-generated private vocabulary for the SAI metadata bridge.
#pragma once

namespace x3d::sai::experimental::detail {{

enum class X3DFieldType {{
{enumerators}
}};

enum class AccessType {{
    InitializeOnly,
    InputOnly,
    OutputOnly,
    InputOutput,
}};

}} // namespace x3d::sai::experimental::detail
"""
