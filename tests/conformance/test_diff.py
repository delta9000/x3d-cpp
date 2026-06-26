from x3d_cpp_gen.conformance.manifest import extract_manifest
from x3d_cpp_gen.conformance.diff import manifest_diff

UOM_40 = "src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml"
UOM_41 = "tests/fixtures/X3dUnifiedObjectModel-4.1.xml"

def test_40_to_41_ripple():
    m41 = extract_manifest(UOM_41)
    d = manifest_diff(extract_manifest(UOM_40), m41)
    # The +6 concrete-node ripple. The authoritative UOM also adds the abstract
    # base/interface types X3DTangentNode and X3DUrlOutputObject; those are not
    # ConcreteNodes, so filter to concrete additions for the structural delta.
    concrete_added = [n for n in d["nodes_added"] if not m41.nodes[n]["abstract"]]
    assert sorted(concrete_added) == sorted(
        ["EnvironmentLight", "FontLibrary", "HAnimPose", "InlineGeometry",
         "RenderedTexture", "Tangent"]
    )
    assert d["nodes_removed"] == []
    # defaults_changed / fields_added are present (possibly empty) and well-formed
    assert isinstance(d["defaults_changed"], list)
    assert isinstance(d["fields_added"], list)
