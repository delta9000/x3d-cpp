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


# -- field additions: fields the standard defines but the vendored UOM omits -----

from x3d_cpp_gen.conformance.errata import (
    FIELD_ADDITIONS, additions_for, apply_field_additions_manifest,
    apply_field_additions_model,
)


def test_4_0_waveshaper_curve_is_present_in_the_loaded_oracle():
    f = load_manifest("4.0").nodes["WaveShaper"]["fields"]["curve"]
    assert (f["type"], f["accessType"]) == ("MFFloat", "inputOutput")


def test_committed_4_0_json_stays_faithful_to_the_uom_without_curve():
    # the overlay is in-memory only: the committed manifest (and its hash) is pure UOM
    raw = json.loads((_MANIFEST_DIR / "x3d-4.0.json").read_text())
    assert "curve" not in raw["nodes"]["WaveShaper"]["fields"]
    assert "oversample" in raw["nodes"]["WaveShaper"]["fields"]


def test_field_additions_do_not_apply_to_other_versions():
    assert not additions_for("3.3")
    m = load_manifest("3.3")
    assert "WaveShaper" not in m.nodes or "curve" not in m.nodes["WaveShaper"]["fields"]


def test_field_additions_are_self_disabling_when_the_uom_gains_the_field():
    nodes = {"WaveShaper": {"fields": {"curve": {"type": "MFFloat",
                                                  "accessType": "inputOutput",
                                                  "default": "SENTINEL"}}}}
    assert apply_field_additions_manifest("4.0", nodes) == []
    assert nodes["WaveShaper"]["fields"]["curve"]["default"] == "SENTINEL"
    assert apply_field_additions_manifest("4.0", {}) == []          # node absent: no-op


def test_every_field_addition_cites_the_standard():
    for a in FIELD_ADDITIONS:
        assert a.get("source") and a.get("source_url") and a.get("reason") and a.get("evidence")
        assert "19775-1" in a["source"] and a["source_url"].startswith("https://www.web3d.org/")
        assert a["description"].startswith("The ")     # the standard's wording, not a paraphrase


def test_waveshaper_curve_added_to_the_generator_model_and_only_there():
    from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES
    from x3d_cpp_gen.parser import parse_x3d_model

    nodes, _ = parse_x3d_model("src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml",
                               FIELD_TYPE_MAPPING, XS_TYPES)
    assert not any(f.name == "curve" for f in nodes["WaveShaper"].fields)   # pure parse
    applied = apply_field_additions_model("4.0", nodes)
    assert [(a["node"], a["field"]) for a in applied] == [("WaveShaper", "curve")]
    curve = next(f for f in nodes["WaveShaper"].fields if f.name == "curve")
    assert (curve.type, curve.accessType, curve.x3d_name) == ("MFFloat", "inputOutput", "curve")
    assert curve.inherited_from is None
    assert apply_field_additions_model("4.0", nodes) == []                  # second pass: no-op
    assert apply_field_additions_model("4.1", nodes) == []                  # other version: no-op
