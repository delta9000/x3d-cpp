from x3d_cpp_gen.generator import SPECIAL_STRUCTS, gen_types_header, gen_enums_header
from x3d_cpp_gen.emit.reflection import gen_reflection_header

def test_types_header_wraps_core_and_forward_decls_x3dnode():
    h = gen_types_header()
    assert "#pragma once" in h
    assert "namespace x3d::nodes { class X3DNode; }" in h
    assert "namespace x3d::core {" in h
    assert "} // namespace x3d::core" in h
    # SFNode points at the nodes-namespace X3DNode:
    assert "using SFNode = std::shared_ptr<nodes::X3DNode>;" in h

def test_every_special_struct_has_defaulted_equality():
    # Equality is vocabulary, not math (ADR-0012 keeps arithmetic out of core,
    # but consumers should not each reinvent member-wise comparison). C++20
    # defaulted operator== also synthesizes operator!=.
    h = " ".join(gen_types_header().split())
    for struct in SPECIAL_STRUCTS:
        assert f"bool operator==(const {struct}&) const = default;" in h, struct

def test_enums_and_reflection_wrap_core():
    assert "namespace x3d::core {" in gen_enums_header({})
    assert "namespace x3d::core {" in gen_reflection_header()
