"""Generate lightweight typed bindings for the experimental C++ SAI."""

from x3d_cpp_gen import __version__
from x3d_cpp_gen.emit.naming import cpp_str, sanitize_field_name
from x3d_cpp_gen.emit.semantic_fields import resolved_node_fields
from x3d_cpp_gen.model.types import X3DType


_NS = "::x3d::sai::experimental::"

SAI_VALUE_TYPE_MAPPING = {
    X3DType.SFBool: "bool",
    X3DType.SFColor: f"{_NS}color3f",
    X3DType.SFColorRGBA: f"{_NS}color4f",
    X3DType.SFDouble: "double",
    X3DType.SFFloat: "float",
    X3DType.SFImage: f"{_NS}image",
    X3DType.SFInt32: "std::int32_t",
    X3DType.SFMatrix3d: f"{_NS}matrix3d",
    X3DType.SFMatrix3f: f"{_NS}matrix3f",
    X3DType.SFMatrix4d: f"{_NS}matrix4d",
    X3DType.SFMatrix4f: f"{_NS}matrix4f",
    X3DType.SFNode: f"{_NS}node_id",
    X3DType.SFRotation: f"{_NS}rotation",
    X3DType.SFString: "std::string",
    X3DType.SFTime: f"{_NS}time_value",
    X3DType.SFVec2d: f"{_NS}vec2d",
    X3DType.SFVec2f: f"{_NS}vec2f",
    X3DType.SFVec3d: f"{_NS}vec3d",
    X3DType.SFVec3f: f"{_NS}vec3f",
    X3DType.SFVec4d: f"{_NS}vec4d",
    X3DType.SFVec4f: f"{_NS}vec4f",
    X3DType.MFBool: f"{_NS}bool_list",
    X3DType.MFColor: f"{_NS}color3f_list",
    X3DType.MFColorRGBA: f"{_NS}color4f_list",
    X3DType.MFDouble: f"{_NS}double_list",
    X3DType.MFFloat: f"{_NS}float_list",
    X3DType.MFImage: f"{_NS}image_list",
    X3DType.MFInt32: f"{_NS}int32_list",
    X3DType.MFMatrix3d: f"{_NS}matrix3d_list",
    X3DType.MFMatrix3f: f"{_NS}matrix3f_list",
    X3DType.MFMatrix4d: f"{_NS}matrix4d_list",
    X3DType.MFMatrix4f: f"{_NS}matrix4f_list",
    X3DType.MFNode: f"{_NS}node_list",
    X3DType.MFRotation: f"{_NS}rotation_list",
    X3DType.MFString: f"{_NS}string_list",
    X3DType.MFTime: f"{_NS}time_list",
    X3DType.MFVec2d: f"{_NS}vec2d_list",
    X3DType.MFVec2f: f"{_NS}vec2f_list",
    X3DType.MFVec3d: f"{_NS}vec3d_list",
    X3DType.MFVec3f: f"{_NS}vec3f_list",
    X3DType.MFVec4d: f"{_NS}vec4d_list",
    X3DType.MFVec4f: f"{_NS}vec4f_list",
    # UOM schema attributes use xs:NMTOKEN, but its executable SAI/runtime
    # representation is SFString (see X3DType.runtime_tag).
    X3DType.XS_NMTOKEN: "std::string",
}

_ACCESS = {
    "initializeOnly": "initialize_only",
    "inputOnly": "input_only",
    "outputOnly": "output_only",
    "inputOutput": "input_output",
}


def sai_value_type(kind):
    try:
        return SAI_VALUE_TYPE_MAPPING[kind]
    except KeyError as error:
        raise ValueError(f"unsupported SAI field type: {kind!r}") from error


def _descriptor_value_type(descriptor):
    if descriptor.is_enum:
        return f"{_NS}{'enum_list' if descriptor.runtime_field_type == 'MFEnum' else 'enum_value'}"
    return sai_value_type(descriptor.x3d_type)


def gen_sai_node_binding(node, nodes, graph, enum_defs=None):
    descriptors = resolved_node_fields(node, nodes, graph, enum_defs)
    identifiers = set()
    rendered = []
    for descriptor in descriptors:
        identifier = sanitize_field_name(descriptor.x3d_name)
        if identifier in identifiers:
            raise ValueError(
                f"duplicate SAI field identifier {identifier!r} on {node.name}"
            )
        identifiers.add(identifier)
        rendered.append((descriptor, identifier, _descriptor_value_type(descriptor)))

    lines = [
        "#pragma once",
        "",
        "// Auto-generated experimental SAI schema binding.",
        '#include "x3d/sai/experimental/X3DSAIBindings.hpp"',
        '#include "x3d/sai/experimental/kernel.hpp"',
        "",
        "namespace x3d::sai::experimental::bindings {",
        "",
        f"struct {node.name} {{",
        f'  static constexpr std::string_view x3d_name = "{cpp_str(node.name)}";',
        "  static constexpr std::string_view schema_fingerprint = model_fingerprint;",
    ]
    for descriptor, identifier, value_type in rendered:
        access = _ACCESS[descriptor.access_type]
        lines.extend(
            [
                f"  inline static constexpr field_key<{node.name}, {value_type}>",
                f"      {identifier}{{\"{cpp_str(descriptor.x3d_name)}\",",
                f"                   access_type::{access}}};",
            ]
        )
    lines.extend(
        [
            f"  inline static constexpr std::array<field_key_descriptor, {len(rendered)}>",
            "      field_keys{{",
        ]
    )
    for _descriptor, identifier, _value_type in rendered:
        lines.append(
            f"          {{{identifier}.name(), {identifier}.kind, {identifier}.access()}},"
        )
    lines.extend(["      }};", ""])
    lines.extend(["};", "", "} // namespace x3d::sai::experimental::bindings", ""])
    return "\n".join(lines)


def gen_sai_binding_index(nodes):
    names = sorted(nodes)
    lines = [
        "#pragma once",
        "",
        "// Auto-generated exhaustive experimental SAI key catalog.",
    ]
    lines.extend(
        f'#include "x3d/sai/experimental/bindings/{name}.hpp"' for name in names
    )
    lines.extend(
        [
            "",
            "namespace x3d::sai::experimental::bindings {",
            "",
            f"inline constexpr std::array<node_key_descriptor, {len(names)}>",
            "    node_keys{{",
        ]
    )
    for name in names:
        lines.extend(
            [
                f"        {{{name}::x3d_name, {name}::schema_fingerprint,",
                f"          std::span<const field_key_descriptor>{{{name}::field_keys}}}},",
            ]
        )
    lines.extend(
        [
            "    }};",
            "",
            "} // namespace x3d::sai::experimental::bindings",
            "",
        ]
    )
    return "\n".join(lines)


def gen_sai_bindings_catalog(spec_version, model_fingerprint):
    return "\n".join(
        [
            "#pragma once",
            "",
            "// Auto-generated experimental SAI binding provenance.",
            "#include <string_view>",
            "",
            "namespace x3d::sai::experimental::bindings {",
            f'inline constexpr std::string_view specification_version = "{cpp_str(spec_version)}";',
            f'inline constexpr std::string_view model_fingerprint = "{cpp_str(model_fingerprint)}";',
            f'inline constexpr std::string_view generator_version = "{cpp_str(__version__)}";',
            "} // namespace x3d::sai::experimental::bindings",
            "",
        ]
    )
