"""The errata overlay: cited, version-keyed corrections applied ON TOP of the
extracted manifest at load time. Keeps the committed manifest JSON (and its
uomManifestHash) byte-faithful to the UOM while letting validation use an oracle
corrected for known UOM errata. Self-disabling: an erratum whose `from` guard no
longer matches (the source UOM got fixed) applies nothing."""
import json
from pathlib import Path

from x3d_cpp_gen.conformance.errata import ERRATA, apply_errata, errata_for
from x3d_cpp_gen.conformance.validate import validate_document
from x3d_cpp_gen.conformance.version_resolve import load_manifest

_MANIFEST_DIR = Path("src/x3d_cpp_gen/conformance/manifests")


def test_3_0_viewpoint_orientation_corrected_to_inputoutput():
    m = load_manifest("3.0")
    assert m.nodes["Viewpoint"]["fields"]["orientation"]["accessType"] == "inputOutput"
    assert m.nodes["GeoViewpoint"]["fields"]["orientation"]["accessType"] == "inputOutput"


def test_errata_does_not_touch_unrelated_orientation_fields():
    # Extrusion.orientation is a genuinely-inputOnly MFRotation (spine orientation),
    # not the Viewpoint erratum — the type+from guard must leave it alone.
    m = load_manifest("3.0")
    assert m.nodes["Extrusion"]["fields"]["orientation"]["accessType"] == "inputOnly"


def test_committed_3_0_json_on_disk_stays_faithful_to_the_uom():
    # the overlay is in-memory only; the committed criteria file is unchanged
    raw = json.loads((_MANIFEST_DIR / "x3d-3.0.json").read_text())
    assert raw["nodes"]["Viewpoint"]["fields"]["orientation"]["accessType"] == "initializeOnly"


def test_3_1_plus_manifests_get_no_orientation_errata_applied():
    # the source UOM is already correct from 3.1 on, so the from-guard misses → no-op
    applied = errata_for("3.1")
    assert not [e for e in applied if e["field"] == "orientation"]


def test_apply_errata_is_self_disabling_when_guard_misses():
    # feeding an already-corrected nodes dict applies nothing (proves it can't
    # silently overwrite a future-fixed value)
    nodes = {"Viewpoint": {"fields": {"orientation": {"accessType": "inputOutput",
                                                       "type": "SFRotation"}}}}
    applied = apply_errata("3.0", nodes)
    assert applied == []
    assert nodes["Viewpoint"]["fields"]["orientation"]["accessType"] == "inputOutput"


def test_every_erratum_carries_evidence_and_a_correction_citation():
    for e in ERRATA:
        assert e.get("reason") and e.get("evidence") and e.get("corrected_in")


def test_3_0_route_into_viewpoint_orientation_no_longer_flagged():
    # the real corpus pattern: OrientationInterpolator.value_changed -> Viewpoint.orientation
    doc = """<X3D version='3.0'><Scene>
      <OrientationInterpolator DEF='OI'/><Viewpoint DEF='V'/>
      <ROUTE fromNode='OI' fromField='value_changed' toNode='V' toField='orientation'/>
      <ROUTE fromNode='OI' fromField='value_changed' toNode='V' toField='set_orientation'/>
    </Scene></X3D>"""
    codes = {f.code for f in validate_document(doc, load_manifest("3.0"))}
    assert "ROUTE_ACCESS_ILLEGAL" not in codes
