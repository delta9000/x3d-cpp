from jinja2 import Environment, FileSystemLoader
import os
from pathlib import Path
from typing import Dict
from x3d_cpp_gen.parser import X3DNode
from x3d_cpp_gen.backends.cpp_header import CppHeaderBackend, TEMPLATES_DIR

# --- Re-exports: these responsibilities now live in dedicated layers, but the
# historical generator.* import surface is preserved for the CLI and tests. ---
from x3d_cpp_gen.emit.naming import pascal  # noqa: F401
from x3d_cpp_gen.emit.naming import cpp_str as _cpp_str  # noqa: F401
from x3d_cpp_gen.emit.defaults import (  # noqa: F401
    compute_default_expr, tokenize_mfstring, cpp_string_literal,
)

# Mapping from X3D field types to C++ types
FIELD_TYPE_MAPPING = {
    # Single-value types
    "SFBool": "bool",
    "SFColor": "SFColor",
    "SFColorRGBA": "SFColorRGBA",
    "SFDouble": "double",
    "SFFloat": "float",
    "SFImage": "SFImage",
    "SFInt32": "int",
    "SFMatrix3d": "SFMatrix3d",
    "SFMatrix3f": "SFMatrix3f",
    "SFMatrix4d": "SFMatrix4d",
    "SFMatrix4f": "SFMatrix4f",
    "SFNode": "std::shared_ptr<X3DNode>",
    "SFRotation": "SFRotation",
    "SFString": "std::string",
    "SFTime": "double",
    "SFVec2d": "SFVec2d",
    "SFVec2f": "SFVec2f",
    "SFVec3d": "SFVec3d",
    "SFVec3f": "SFVec3f",
    "SFVec4d": "SFVec4d",
    "SFVec4f": "SFVec4f",
    # Multi-value types
    "MFBool": "std::vector<bool>",
    "MFColor": "std::vector<SFColor>",
    "MFColorRGBA": "std::vector<SFColorRGBA>",
    "MFDouble": "std::vector<double>",
    "MFFloat": "std::vector<float>",
    "MFImage": "std::vector<SFImage>",
    "MFInt32": "std::vector<int>",
    "MFMatrix3d": "std::vector<SFMatrix3d>",
    "MFMatrix3f": "std::vector<SFMatrix3f>",
    "MFMatrix4d": "std::vector<SFMatrix4d>",
    "MFMatrix4f": "std::vector<SFMatrix4f>",
    "MFNode": "std::vector<std::shared_ptr<X3DNode>>",
    "MFRotation": "std::vector<SFRotation>",
    "MFString": "std::vector<std::string>",
    "MFTime": "std::vector<double>",
    "MFVec2d": "std::vector<SFVec2d>",
    "MFVec2f": "std::vector<SFVec2f>",
    "MFVec3d": "std::vector<SFVec3d>",
    "MFVec3f": "std::vector<SFVec3f>",
    "MFVec4d": "std::vector<SFVec4d>",
    "MFVec4f": "std::vector<SFVec4f>",
}

XS_TYPES = {
    "xs:NMTOKEN": ("xs_nmtoken","std::string"),
}

# Special structs to generate
SPECIAL_STRUCTS = [
    "SFVec2d", "SFVec2f", "SFVec3d", "SFVec3f", "SFVec4d", "SFVec4f",
    "SFColor", "SFColorRGBA",
    "SFRotation", "SFMatrix3d", "SFMatrix3f", "SFMatrix4d", "SFMatrix4f", "SFImage"
]

# Coverage assertion: every SPECIAL_STRUCTS entry (except SFImage's documented gap)
# must have a corresponding _STRUCT_ARITY entry in defaults.py, or default_expr_for
# will silently emit invalid C++ struct literals for fields with spec defaults.
# This assertion fires at import time, before any code generation can proceed.
def _assert_struct_arity_coverage():
    from x3d_cpp_gen.emit.defaults import struct_arity_names
    covered = struct_arity_names()
    expected_gap = {"SFImage"}
    missing = [name for name in SPECIAL_STRUCTS
               if name not in covered and name not in expected_gap]
    assert not missing, (
        f"SPECIAL_STRUCTS entries with no _STRUCT_ARITY coverage: {missing}. "
        f"A field of this type with a spec default will silently emit an "
        f"unbraced token list instead of a valid struct literal."
    )

_assert_struct_arity_coverage()

def generate_special_structs() -> str:
    """Generates C++ struct definitions for special field types."""
    structs = {
        "SFVec2d": """
    struct SFVec2d {
        double x, y;
    };
""",
        "SFVec2f": """
    struct SFVec2f {
        float x, y;
    };
""",
        "SFVec3d": """
    struct SFVec3d {
        double x, y, z;
    };
""",
        "SFVec3f": """
    struct SFVec3f {
        float x, y, z;
    };
""",
        "SFVec4d": """
    struct SFVec4d {
        double x, y, z, w;
    };
""",
        "SFVec4f": """
    struct SFVec4f {
        float x, y, z, w;
    };
""",
        "SFColor": """
    struct SFColor {
        float r, g, b;
    };
""",
        "SFColorRGBA": """
    struct SFColorRGBA {
        float r, g, b, a;
    };
""",
        "SFRotation": """
    struct SFRotation {
        float x, y, z, angle;
    };
""",
        "SFMatrix3d": """
    struct SFMatrix3d {
        double matrix[3][3];
    };
""",
        "SFMatrix3f": """
    struct SFMatrix3f {
        float matrix[3][3];
    };
""",
        "SFMatrix4d": """
    struct SFMatrix4d {
        double matrix[4][4];
    };
""",
        "SFMatrix4f": """
    struct SFMatrix4f {
        float matrix[4][4];
    };
""",
        "SFImage": """
    struct SFImage {
        int width, height, numComponents;
        std::vector<unsigned char> data;
    };
"""
    }
    structs_code = "// Special structs\n"
    for struct in SPECIAL_STRUCTS:
        structs_code += structs.get(struct, f"// Struct for {struct} not defined.\n")
    structs_code += "\n"
    return structs_code

def gen_types_header() -> str:
    """Generates the header file for the X3D types."""
    header = """#pragma once

#include <vector>
#include <memory>
#include <string>

namespace x3d::nodes { class X3DNode; }

namespace x3d::core {

"""

    # Generate special structs
    header += generate_special_structs()

    # Generate the type definitions. Skip tautological self-typedefs
    # (typedef X X;) which are ill-formed for struct names and redundant.
    # SFNode/MFNode point at the nodes-namespace X3DNode.
    for field_type, cpp_type in FIELD_TYPE_MAPPING.items():
        if cpp_type == field_type:
            continue
        if field_type == "SFNode":
            header += "using SFNode = std::shared_ptr<nodes::X3DNode>;\n"
            continue
        if field_type == "MFNode":
            header += "using MFNode = std::vector<std::shared_ptr<nodes::X3DNode>>;\n"
            continue
        header += f"typedef {cpp_type} {field_type};\n"

    for item in XS_TYPES:
        if XS_TYPES[item][1] == XS_TYPES[item][0]:
            continue
        header += f"typedef {XS_TYPES[item][1]} {XS_TYPES[item][0]};\n"

    header += "\n"
    header += "\n} // namespace x3d::core\n"
    return header

def write_types_header(output_dir: str) -> None:
    """Writes the X3D types header file to the output directory."""
    types_header = gen_types_header()
    output_file = os.path.join(output_dir, "X3Dtypes.hpp")
    with open(output_file, 'w') as f:
        f.write(types_header)
    print(f"Generated X3D types header file at {output_file}")


def gen_enums_header(enum_defs) -> str:
    """Generate X3Denums.hpp: one ``enum class`` per bounded SimpleType.

    ``enum_defs`` maps SimpleType name -> ``EnumDef``. Emitted in sorted order
    for deterministic output. Always emits a valid (possibly empty) header so
    generated node headers can unconditionally ``#include "X3Denums.hpp"``.
    """
    lines = [
        "#pragma once",
        "",
        "// Auto-generated C++ 'enum class' definitions for the bounded X3D",
        "// SimpleType enumerations (closed vocabularies) in the UOM spec.",
        "",
        "#include <string>",
        "",
        "namespace x3d::core {",
        "",
    ]
    for name in sorted(enum_defs):
        ed = enum_defs[name]
        if ed.description:
            lines.append(f"// {ed.cpp_name}: {ed.description}")
        lines.append(f"enum class {ed.cpp_name} {{")
        for m in ed.members:
            lines.append(f"    {m.cpp_name},  // \"{m.value}\"")
        lines.append("};")
        lines.append("")

    # Per-enum token <-> value conversions. These let node-agnostic codecs
    # serialize/parse an enum-class field by its X3D token (the raw spec value)
    # without compile-time knowledge of the concrete enum type: each node's
    # reflection FieldInfo binds these via string-form thunks. The X3D token is
    # the enumeration's raw spec spelling (quotes/whitespace stripped).
    lines.append("// X3D-token <-> enum-value conversions for the bounded enums above.")
    for name in sorted(enum_defs):
        ed = enum_defs[name]
        cpp = ed.cpp_name
        # to_string: enum value -> X3D token string.
        lines.append(f"inline std::string to_string({cpp} value) {{")
        lines.append("    switch (value) {")
        for m in ed.members:
            token = _enum_token(m.value)
            lines.append(f"    case {cpp}::{m.cpp_name}: return {_cpp_str_literal(token)};")
        lines.append("    }")
        lines.append("    return \"\";")
        lines.append("}")
        lines.append("")
        # from_string: X3D token string -> enum value (returns false if unknown).
        lines.append(
            f"inline bool from_string(const std::string& token, {cpp}& out) {{")
        for m in ed.members:
            token = _enum_token(m.value)
            lines.append(
                f"    if (token == {_cpp_str_literal(token)}) {{ out = {cpp}::{m.cpp_name}; return true; }}")
        lines.append("    return false;")
        lines.append("}")
        lines.append("")

    lines.append("} // namespace x3d::core")
    lines.append("")
    return "\n".join(lines)


def _enum_token(raw: str) -> str:
    """The X3D wire token for an enumeration value: raw spec spelling, with any
    surrounding quotes and whitespace stripped (some UOM values are quoted)."""
    return (raw or "").strip().strip('"')


def _cpp_str_literal(s: str) -> str:
    """A fully-quoted C++ string literal for ``s``."""
    return f'"{_cpp_str(s)}"'


def write_enums_header(output_dir: str, enum_defs) -> None:
    """Write X3Denums.hpp to the output directory."""
    output_file = os.path.join(output_dir, "X3Denums.hpp")
    with open(output_file, 'w') as f:
        f.write(gen_enums_header(enum_defs))
    print(f"Generated X3D enums header file at {output_file}")

def write_reflection_header(output_dir: str) -> None:
    """Write the runtime reflection support header X3DReflection.hpp."""
    from x3d_cpp_gen.emit.reflection import gen_reflection_header
    output_file = os.path.join(output_dir, "X3DReflection.hpp")
    with open(output_file, 'w') as f:
        f.write(gen_reflection_header())
    print(f"Generated X3D reflection header file at {output_file}")


def write_node_factory(output_dir: str, nodes: Dict[str, X3DNode]) -> None:
    """Write X3DNodeFactory.hpp (declarations) + X3DNodeFactory.cpp (registry)."""
    from x3d_cpp_gen.emit.factory import (
        gen_node_factory_header, gen_node_factory_source,
    )
    nodes_dir = os.path.join(output_dir, "x3d", "nodes")
    os.makedirs(nodes_dir, exist_ok=True)
    hdr = os.path.join(nodes_dir, "X3DNodeFactory.hpp")
    with open(hdr, "w") as f:
        f.write(gen_node_factory_header(nodes))
    print(f"Generated X3D node factory header file at {hdr}")
    src = os.path.join(nodes_dir, "X3DNodeFactory.cpp")
    with open(src, "w") as f:
        f.write(gen_node_factory_source(nodes))
    print(f"Generated X3D node factory source file at {src}")


def write_interface_registry(output_dir: str, nodes: Dict[str, X3DNode],
                             dependency_graph) -> None:
    """Write X3DInterfaceRegistry.hpp/.cpp (the interface membership tables)."""
    from x3d_cpp_gen.emit.registry import (
        gen_interface_registry_header, gen_interface_registry_source,
    )
    nodes_dir = os.path.join(output_dir, "x3d", "nodes")
    os.makedirs(nodes_dir, exist_ok=True)
    hdr = os.path.join(nodes_dir, "X3DInterfaceRegistry.hpp")
    with open(hdr, "w") as f:
        f.write(gen_interface_registry_header(nodes))
    print(f"Generated X3D interface registry header at {hdr}")
    src = os.path.join(nodes_dir, "X3DInterfaceRegistry.cpp")
    with open(src, "w") as f:
        f.write(gen_interface_registry_source(nodes, dependency_graph))
    print(f"Generated X3D interface registry source at {src}")


def generate_cpp_bindings(nodes: Dict[str, X3DNode], dependency_graph,
                          output_dir: str, templates_dir=None,
                          clang_format: str = "clang-format",
                          enum_defs=None, namespace: str = "") -> None:
    """Thin orchestrator: delegate per-node header emission to the backend.

    All type decisions now live in emit.descriptors and the backend renders the
    thin template; this function just wires the backend up with the requested
    templates dir / clang-format / enum definitions and runs it.

    ``namespace`` (default empty) optionally wraps each emitted class in a C++
    namespace. Empty keeps output byte-identical to the 4.0 golden.
    """
    backend = CppHeaderBackend(templates_dir=templates_dir,
                               clang_format=clang_format, enum_defs=enum_defs,
                               namespace=namespace)
    backend.emit(nodes, dependency_graph, output_dir)



def _assertable_default_fields(node, enum_defs=None):
    """Fields of ``node`` that are readable AND have a spec default value.

    For each such field a default-value assertion can be emitted: the generated
    node exposes a static ``getDefault<Name>()`` (return type identical to the
    field's getter) whenever the field has a default and is not an inputOnly
    event, so the test compares ``instance.get<Name>()`` against
    ``Node::getDefault<Name>()``. This drives the assertions off the SAME
    descriptor pipeline the headers are generated from, so they can never drift.
    """
    from x3d_cpp_gen.emit.descriptors import build_descriptors
    from x3d_cpp_gen.parser import get_own_fields

    # Only the node's OWN fields get a getDefault<Name>()/get<Name>() emitted on
    # the class itself (inherited ones are reachable via base classes but the
    # static default lives on the declaring base). Asserting on own fields keeps
    # every emitted call resolvable on the concrete type.
    out = []
    for d in build_descriptors(get_own_fields(node), enum_defs):
        # getDefault<Name>() is emitted iff: has a default and is not an event.
        # We compare via the getter, so the field must also be readable.
        if d.has_default and not d.is_event and d.is_readable:
            out.append({
                "name_pascal": d.name_pascal,
                "getter_name": d.getter_name,
                "x3d_name": d.x3d_name,
            })
    return out


def generate_test_file(nodes: Dict[str, X3DNode], output_dir: str,
                       templates_dir=None, enum_defs=None) -> str:
    """Generates a value-asserting smoke test for the X3D nodes.

    For every concrete node the generated test instantiates a default-constructed
    instance and ASSERTS that each readable field with a spec default returns
    exactly that default (comparing the field's getter against the node's static
    ``getDefault<Name>()``). It additionally pins a handful of well-known nodes
    (e.g. Box: size=={2,2,2}, solid==true) with explicit literal assertions.
    """
    templates_dir = str(templates_dir) if templates_dir else str(TEMPLATES_DIR)
    env = Environment(loader=FileSystemLoader(templates_dir))
    test_template = env.get_template('test_template.cpp.jinja')

    # Only include concrete (instantiable) nodes in the test file
    concrete = [node for node in nodes.values() if not node.is_abstract]
    concrete_names = {n.name for n in concrete}

    nodes_ctx = [
        {
            "name": node.name,
            "default_fields": _assertable_default_fields(node, enum_defs),
        }
        for node in concrete
    ]

    # Explicit "well-known" literal pins. Only emitted for nodes that are
    # actually present + concrete in this spec, so the test always compiles.
    well_known = [
        wk for wk in _WELL_KNOWN_PINS if wk["node"] in concrete_names
    ]

    # Render the test file
    test_code = test_template.render(
        concrete_nodes=concrete,
        nodes_ctx=nodes_ctx,
        well_known=well_known,
    )
    nodes_dir = os.path.join(output_dir, "x3d", "nodes")
    os.makedirs(nodes_dir, exist_ok=True)
    test_file_path = os.path.join(nodes_dir, "test.cpp")

    with open(test_file_path, 'w') as f:
        f.write(test_code)
    print(f"Generated {test_file_path}")
    return test_file_path


# Hand-picked spec defaults pinned with explicit literal assertions, on top of
# the generic getter-vs-getDefault checks emitted for every node. Each `expr`
# is a C++ boolean expression evaluated against a default-constructed `inst`.
_WELL_KNOWN_PINS = [
    {"node": "Box", "desc": "Box.size default == {2,2,2}",
     "expr": "inst.getSize().x == 2 && inst.getSize().y == 2 && "
             "inst.getSize().z == 2"},
    {"node": "Box", "desc": "Box.solid default == true",
     "expr": "inst.getSolid() == true"},
    {"node": "Sphere", "desc": "Sphere.radius default == 1",
     "expr": "inst.getRadius() == 1.0f"},
    {"node": "Cone", "desc": "Cone.bottomRadius default == 1",
     "expr": "inst.getBottomRadius() == 1.0f"},
    {"node": "Cone", "desc": "Cone.height default == 2",
     "expr": "inst.getHeight() == 2.0f"},
    {"node": "Cylinder", "desc": "Cylinder.radius default == 1",
     "expr": "inst.getRadius() == 1.0f"},
    {"node": "Material", "desc": "Material.transparency default == 0",
     "expr": "inst.getTransparency() == 0.0f"},
]
