import json
from pathlib import Path
from x3d_cpp_gen.conformance.manifest import extract_manifest
from x3d_cpp_gen.conformance.meta_validator import (
    validate_manifest, assert_field_types_mapped,
)

MANIFEST_DIR = Path("src/x3d_cpp_gen/conformance/manifests")
# All six genuine ISO/Web3D UOMs (offline-pinned). 3.x sourced from web3d.org
# (provenance headers preserved); 4.0 ships with the package; 4.1 is a fixture.
UOM = {
    "3.0": "tests/fixtures/uom/X3dUnifiedObjectModel-3.0.xml",
    "3.1": "tests/fixtures/uom/X3dUnifiedObjectModel-3.1.xml",
    "3.2": "tests/fixtures/uom/X3dUnifiedObjectModel-3.2.xml",
    "3.3": "tests/fixtures/uom/X3dUnifiedObjectModel-3.3.xml",
    "4.0": "src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml",
    "4.1": "tests/fixtures/X3dUnifiedObjectModel-4.1.xml",
}

def test_committed_manifests_match_fresh_extraction():
    # The committed JSON must equal a fresh extraction (so a UOM change that is
    # not re-committed fails loudly — a golden-style gate for the criteria).
    for ver, uom in UOM.items():
        committed = (MANIFEST_DIR / f"x3d-{ver}.json").read_text()
        fresh = extract_manifest(uom).to_json()
        assert committed == fresh, f"committed x3d-{ver}.json drifted from the UOM"

def test_every_committed_manifest_passes_the_meta_validator():
    # The moat's future-proofing across the FULL version range, zero extractor edits:
    # every one of the six genuine UOMs extracts + passes the admission gates.
    for ver, uom in UOM.items():
        m = extract_manifest(uom)
        validate_manifest(m)             # node-count floor + shape
        assert_field_types_mapped(uom)   # strict mode: no unmapped field type

def test_41_has_eight_more_nodes_than_40():
    # The manifest extracts ALL UOM node types (concrete + abstract). 4.1 adds the
    # 6 new concrete nodes (EnvironmentLight, FontLibrary, HAnimPose, InlineGeometry,
    # RenderedTexture, Tangent) PLUS 2 new abstract node types (X3DTangentNode,
    # X3DUrlOutputObject) — confirmed against the authoritative UOM, so the real
    # extracted delta is 8.
    n40 = json.loads(extract_manifest(UOM["4.0"]).to_json())["provenance"]["nodeCount"]
    n41 = json.loads(extract_manifest(UOM["4.1"]).to_json())["provenance"]["nodeCount"]
    assert n41 - n40 == 8
