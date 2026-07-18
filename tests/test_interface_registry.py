# tests/test_interface_registry.py
"""Codegen tests for the interface registry emitter."""
from importlib.resources import files
import copy

import pytest

from x3d_cpp_gen.parser import (
    parse_x3d_model, build_dependency_graph,
)
from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_cpp_gen.emit.registry import (
    interface_ids, interfaces_of,
    gen_interface_registry_header, gen_interface_registry_source,
)
from x3d_cpp_gen.emit.semantic_metadata import (
    gen_semantic_metadata_header, gen_semantic_metadata_source,
)

SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")


@pytest.fixture(scope="module")
def model():
    nodes, _skipped = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
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


def test_semantic_metadata_is_an_owning_schema_catalog(model):
    nodes, graph = model
    hdr = gen_semantic_metadata_header()
    src = gen_semantic_metadata_source(nodes, graph, spec_version="4.0")

    assert "struct SemanticFieldDescriptor" in hdr
    assert "std::optional<std::string> defaultValue" in hdr
    assert "std::vector<std::string> acceptableNodeTypes" in hdr
    assert "std::optional<std::string> unitCategory" in hdr
    assert "struct SemanticNodeDescriptor" in hdr
    assert "std::vector<SemanticFieldDescriptor> fields" in hdr
    assert "X3DSemanticMetadataRegistry" in hdr
    assert "specificationVersion" in hdr
    assert 'return "4.0"' in src
    assert "modelFingerprint" in hdr
    assert "generatorVersion" in hdr
    fingerprint_marker = "X3DSemanticMetadataRegistry::modelFingerprint()"

    def extract_fingerprint(generated):
        return generated[generated.index(fingerprint_marker):].split(
            'return "', 1)[1].split('"', 1)[0]

    fingerprint = extract_fingerprint(src)
    assert len(fingerprint) == 64
    assert all(character in "0123456789abcdef" for character in fingerprint)
    changed_nodes = copy.deepcopy(nodes)
    translation = next(
        field for field in changed_nodes["Transform"].fields
        if field.x3d_name == "translation")
    translation.default = "1 2 3"
    changed = gen_semantic_metadata_source(
        changed_nodes, graph, spec_version="4.0")
    assert extract_fingerprint(changed) != fingerprint
    assert 'X3DSemanticMetadataRegistry::generatorVersion()' in src
    assert "unitCategoriesComplete" in hdr

    transform = src.index('SemanticNodeDescriptor{\n            "Transform"')
    center = src.index('"center"', transform)
    children = src.index('"children"', transform)
    translation = src.index('"translation"', transform)
    assert center < children < translation
    assert '"Grouping", 1' in src[transform:]
    assert '"children", X3DFieldType::MFNode, AccessType::InputOutput' in src
    assert 'std::vector<std::string>{"X3DChildNode"}' in src
