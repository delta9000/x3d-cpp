from pathlib import Path
from x3d_cpp_gen.conformance.manifest import extract_manifest, Manifest

UOM_40 = Path("src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml")

def test_extract_40_manifest_shape():
    m = extract_manifest(str(UOM_40))
    assert isinstance(m, Manifest)
    assert m.uom_version == "4.0"
    # node-count sanity (4.0 has ~260 concrete nodes)
    assert len(m.nodes) >= 255
    # a known node carries its component, containerField, and a field default
    box = m.nodes["Box"]
    assert box["component"]["name"] == "Geometry3D"
    assert box["containerField"] == "geometry"
    assert box["fields"]["size"]["type"] == "SFVec3f"
    assert box["fields"]["size"]["default"] == "2 2 2"
    # provenance + a stable content hash are present
    assert m.uom_manifest_hash and len(m.uom_manifest_hash) == 64

def test_manifest_hash_is_deterministic():
    a = extract_manifest(str(UOM_40)).uom_manifest_hash
    b = extract_manifest(str(UOM_40)).uom_manifest_hash
    assert a == b
