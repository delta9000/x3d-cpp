"""The citable conformance-report artifact: a deterministic, per-version scorecard
over a corpus, binding each file to its own version's (errata-corrected) oracle.
This is the moat capstone — provenance (manifest hashes) + clean% + residual
classified by code, reproducible and machine-readable."""
import json

from x3d_cpp_gen.conformance.report import build_report, format_markdown

CLEAN_40 = """<X3D profile='Immersive' version='4.0'><Scene>
  <Shape><Box size='1 2 3'/></Shape></Scene></X3D>"""

BAD_NODE_40 = """<X3D profile='Immersive' version='4.0'><Scene>
  <Shape><NotARealNode/></Shape></Scene></X3D>"""

# a real 3.0 erratum case: route into Viewpoint.orientation — clean ONLY because
# the errata overlay corrects orientation to inputOutput
ERRATA_30 = """<X3D version='3.0'><Scene>
  <OrientationInterpolator DEF='OI'/><Viewpoint DEF='V'/>
  <ROUTE fromNode='OI' fromField='value_changed' toNode='V' toField='orientation'/>
</Scene></X3D>"""


def _corpus(tmp_path, files):
    for name, body in files.items():
        (tmp_path / name).write_text(body)
    return str(tmp_path)


def test_totals_and_clean_pct(tmp_path):
    root = _corpus(tmp_path, {"clean.x3d": CLEAN_40, "bad.x3d": BAD_NODE_40})
    r = build_report(root)
    assert r["totals"]["validated"] == 2
    assert r["totals"]["structurallyClean"] == 1
    assert r["totals"]["cleanPct"] == 50.0


def test_per_version_scorecard_and_provenance(tmp_path):
    root = _corpus(tmp_path, {"clean.x3d": CLEAN_40, "bad.x3d": BAD_NODE_40})
    v40 = build_report(root)["perVersion"]["4.0"]
    assert v40["validated"] == 2 and v40["clean"] == 1
    # provenance ties the scorecard to the exact UOM extraction (the moat anchor)
    assert v40["provenance"]["uomManifestHash"]
    assert v40["provenance"]["nodeCount"] > 0


def test_findings_classified_by_code(tmp_path):
    root = _corpus(tmp_path, {"clean.x3d": CLEAN_40, "bad.x3d": BAD_NODE_40})
    fbc = build_report(root)["findingsByCode"]
    assert fbc["NODE_UNKNOWN_FOR_VERSION"]["count"] == 1
    assert fbc["NODE_UNKNOWN_FOR_VERSION"]["files"] == 1
    # unknown-node names surfaced for transparency
    assert ["NotARealNode", 1] in [list(x) for x in build_report(root)["topUnknownNodes"]]


def test_errata_applied_section_lists_cited_corrections(tmp_path):
    root = _corpus(tmp_path, {"v30.x3d": ERRATA_30})
    r = build_report(root)
    # the 3.0 file is clean only because the orientation erratum fired
    assert r["perVersion"]["3.0"]["clean"] == 1
    applied = r["errataApplied"]["3.0"]
    e = next(x for x in applied if x["field"] == "orientation")
    assert e["from"] == "initializeOnly" and e["to"] == "inputOutput"
    assert e["corrected_in"] and e["evidence"]


def test_report_is_deterministic(tmp_path):
    root = _corpus(tmp_path, {"a.x3d": CLEAN_40, "b.x3d": BAD_NODE_40, "c.x3d": ERRATA_30})
    a = json.dumps(build_report(root), sort_keys=True)
    b = json.dumps(build_report(root), sort_keys=True)
    assert a == b


def test_markdown_renders_headline_numbers(tmp_path):
    root = _corpus(tmp_path, {"clean.x3d": CLEAN_40, "bad.x3d": BAD_NODE_40})
    md = format_markdown(build_report(root))
    assert "Conformance" in md and "4.0" in md and "%" in md
