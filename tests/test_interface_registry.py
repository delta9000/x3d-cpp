# tests/test_interface_registry.py
"""Codegen tests for the interface registry emitter."""
from importlib.resources import files
import pytest

from x3d_cpp_gen.parser import (
    parse_x3d_model, build_dependency_graph,
)
from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_cpp_gen.emit.registry import (
    interface_ids, interfaces_of,
    gen_interface_registry_header, gen_interface_registry_source,
)

SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")


@pytest.fixture(scope="module")
def model():
    nodes = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    graph = build_dependency_graph(nodes)
    return nodes, graph


def test_enum_contains_core_interfaces(model):
    nodes, _ = model
    ids = interface_ids(nodes)
    for name in ("X3DTimeDependentNode", "X3DSensorNode", "X3DChildNode",
                 "X3DInterpolatorNode", "X3DGroupingNode", "X3DBoundedObject"):
        assert name in ids, f"{name} missing from InterfaceId enum"
    # The doc's "~30" was wrong: the real count is ~70+.
    assert len(ids) > 60


def test_multi_interface_node(model):
    nodes, graph = model
    # TimeSensor implements BOTH X3DTimeDependentNode and X3DSensorNode.
    ifaces = interfaces_of("TimeSensor", nodes, graph)
    assert "X3DTimeDependentNode" in ifaces
    assert "X3DSensorNode" in ifaces


def test_transitive_closure(model):
    nodes, graph = model
    # MovieTexture reaches X3DChildNode only transitively (via the sound /
    # time-dependent chain) — the closure must include it, not just direct bases.
    ifaces = interfaces_of("MovieTexture", nodes, graph)
    assert "X3DUrlObject" in ifaces
    assert "X3DTimeDependentNode" in ifaces
    assert "X3DChildNode" in ifaces


def test_header_and_source_render(model):
    nodes, graph = model
    hdr = gen_interface_registry_header(nodes)
    src = gen_interface_registry_source(nodes, graph)
    assert "enum class InterfaceId" in hdr
    assert "X3DInterfaceRegistry" in hdr
    assert "nodeImplements" in hdr
    # The TimeSensor row must list both its interfaces in the source table.
    assert "TimeSensor" in src
    assert "InterfaceId::X3DTimeDependentNode" in src
    assert "InterfaceId::X3DSensorNode" in src
