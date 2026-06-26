# tests/conformance/test_prose_anchors.py
import json
from pathlib import Path

import pytest

from x3d_cpp_gen.conformance import prose_anchors as pa


def _write_manifest(dir_: Path, ver: str, nodes: dict):
    (dir_ / f"x3d-{ver}.json").write_text(json.dumps({"nodes": nodes, "provenance": {}}))


def test_union_node_set_merges_versions_and_abstract(tmp_path):
    _write_manifest(tmp_path, "4.0", {"Cone": {"abstract": False}})
    _write_manifest(tmp_path, "4.1", {"Cone": {"abstract": False},
                                      "Tangent": {"abstract": False},
                                      "X3DNode": {"abstract": True}})
    u = pa.union_node_set(manifest_dir=tmp_path)
    assert set(u) == {"Cone", "Tangent", "X3DNode"}
    assert u["Cone"]["versions"] == ["x3d-4.0", "x3d-4.1"]
    assert u["Tangent"]["versions"] == ["x3d-4.1"]
    assert u["X3DNode"]["abstract"] is True
    assert u["Cone"]["abstract"] is False


def test_prose_corpus_hash_is_stable_and_skips_index(tmp_path):
    (tmp_path / "a.md").write_text("# A\nbody\n")
    (tmp_path / "INDEX.md").write_text("ignored\n")
    h1 = pa.prose_corpus_hash(prose_dir=tmp_path)
    h2 = pa.prose_corpus_hash(prose_dir=tmp_path)
    assert h1 == h2 and len(h1) == 64
    (tmp_path / "a.md").write_text("# A\nCHANGED\n")
    assert pa.prose_corpus_hash(prose_dir=tmp_path) != h1


def test_parse_prose_headings_extracts_anchors(tmp_path):
    (tmp_path / "geometry3D.md").write_text(
        '<!-- source: https://example.org/geometry3D.html -->\n'
        '# 13 Geometry3D component\n'
        '## <span id="Box"></span> 13.3.1 Box\n'
        'box prose\n'
        '## <span id="Cone"></span> 13.3.2 Cone\n'
        'cone prose\n'
        '## <span id="X3DGeometryNode"></span> 13.3.0 *X3DGeometryNode*\n'
        '## <span id="NurbsSet"></span> **27.4.8 NurbsSet**\n'
    )
    cands = pa.parse_prose_headings(prose_dir=tmp_path)
    assert set(cands) == {"Box", "Cone", "X3DGeometryNode", "NurbsSet"}
    cone = cands["Cone"][0]
    assert cone["file"] == "geometry3D.md"
    assert cone["sourceUrl"] == "https://example.org/geometry3D.html"
    assert cone["section"] == "13.3.2"
    assert cone["depth"] == 2
    assert cone["level"] == 2
    assert cone["title"] == "Cone"
    # emphasis (*...*) stripped from abstract-type titles
    assert cands["X3DGeometryNode"][0]["title"] == "X3DGeometryNode"
    # markdown bold (**) wrapping the WHOLE "number title" must not defeat parsing
    nurbs = cands["NurbsSet"][0]
    assert nurbs["section"] == "27.4.8"
    assert nurbs["title"] == "NurbsSet"


def _prose(tmp_path, name="c.md", body=""):
    (tmp_path / name).write_text("<!-- source: https://ex.org/c.html -->\n" + body)


def test_build_anchor_map_classifies(tmp_path):
    mdir = tmp_path / "manifests"; mdir.mkdir()
    pdir = tmp_path / "prose"; pdir.mkdir()
    _write_manifest(mdir, "4.1", {
        "Cone": {"abstract": False},        # clean exact match
        "Script": {"abstract": False},      # ambiguous (two files)
        "fontstyle": {"abstract": False},   # fuzzy: prose anchor is "FontStyle"
        "Tangent": {"abstract": False},     # unanchored (no prose)
    })
    _prose(pdir, "geometry3D.md",
           "## <span id=\"Cone\"></span> 13.3.2 Cone\nbody\n")
    _prose(pdir, "core.md",
           "## <span id=\"Script\"></span> 7.2.9 Script\nshallow\n")
    _prose(pdir, "scripting.md",
           "## <span id=\"Script\"></span> 29.4.1 Script\ndeep\n"
           "## <span id=\"FontStyle\"></span> 16.4.2 FontStyle\nfs\n")
    art = pa.build_anchor_map(prose_dir=pdir, manifest_dir=mdir)
    assert art["anchors"]["Cone"]["file"] == "geometry3D.md"
    assert art["anchors"]["Cone"]["fragment"] == "Cone"
    # ambiguity resolved to the deeper section, others recorded
    assert art["anchors"]["Script"]["section"] == "29.4.1"
    assert art["anchors"]["Script"]["ambiguous"] == [{"file": "core.md", "section": "7.2.9"}]
    # fuzzy (case-only) match is NOT a clean anchor
    assert "fontstyle" not in art["anchors"]
    assert art["fuzzy"]["fontstyle"]["matchedAnchor"] == "FontStyle"
    assert art["unanchored"] == ["Tangent"]
    assert art["provenance"]["nodeCount"] == 4
    assert art["provenance"]["anchoredCount"] == 2


def test_coverage_buckets_for_version(tmp_path):
    mdir = tmp_path / "manifests"; mdir.mkdir()
    pdir = tmp_path / "prose"; pdir.mkdir()
    _write_manifest(mdir, "4.1", {
        "Cone": {"abstract": False, "component": {"name": "Geometry3D", "level": 1}},
        "Tangent": {"abstract": False, "component": {"name": "Rendering", "level": 1}},
    })
    _prose(pdir, "geometry3D.md", "## <span id=\"Cone\"></span> 13.3.2 Cone\nb\n"
                                  "## <span id=\"Newish\"></span> 13.3.9 Newish\nx\n")
    cov = pa.coverage("4.1", prose_dir=pdir, manifest_dir=mdir)
    assert cov["version"] == "4.1"
    assert cov["anchored"] == ["Cone"]
    assert cov["unanchored"] == [{"node": "Tangent", "abstract": False,
                                  "component": "Rendering"}]
    # prose anchor matching no manifest node (version-newer / mis-named)
    assert "Newish" in cov["proseOnly"]


# Statements (prose lives in core/concepts, not node-reference sections) + 4.1-new
# nodes absent from the 4.0 prose mirror. A node leaving/joining this set must be a
# CONSCIOUS recommit — this gate fails otherwise.
EXPECTED_UNANCHORED = {
    "EXPORT", "EnvironmentLight", "ExternProtoDeclare", "FontLibrary", "HAnimPose",
    "IMPORT", "IS", "InlineGeometry", "ProtoBody", "ProtoDeclare", "ProtoInstance",
    "ProtoInterface", "ROUTE", "RenderedTexture", "Scene", "SceneGraphStructureStatement",
    "Tangent", "X3D", "X3DStatement", "X3DTangentNode", "X3DUrlOutputObject",
    "component", "connect", "field", "fieldValue", "head", "meta", "unit",
}


def test_committed_unanchored_is_calibrated():
    art = pa.load_anchors()
    committed = set(art["unanchored"]) | set(art["fuzzy"])
    assert committed == EXPECTED_UNANCHORED, (
        "prose coverage changed; reconcile EXPECTED_UNANCHORED and recommit the artifact")


def test_section_text_slices_until_next_same_or_higher_heading(tmp_path):
    (tmp_path / "g.md").write_text(
        "## <span id=\"Cone\"></span> 13.3.2 Cone\n"
        "line one\n### <span id=\"Sub\"></span> 13.3.2.1 Sub\nsub line\n"
        "## <span id=\"Sphere\"></span> 13.3.3 Sphere\nnope\n"
    )
    anchor = {"file": "g.md", "fragment": "Cone"}
    txt = pa.section_text(anchor, prose_dir=tmp_path)
    assert "Cone" in txt and "line one" in txt and "sub line" in txt
    assert "Sphere" not in txt and "nope" not in txt


def test_section_text_returns_none_when_mirror_absent(tmp_path):
    assert pa.section_text({"file": "missing.md", "fragment": "Cone"},
                           prose_dir=tmp_path) is None


def test_committed_artifact_schema_is_valid():
    art = pa.load_anchors()
    assert set(art) >= {"provenance", "anchors", "fuzzy", "unanchored"}
    for name, a in art["anchors"].items():
        assert a["fragment"] == name
        assert a["file"].endswith(".md")
        assert a["sourceUrl"].startswith("http")
        assert a["section"] and a["section"][0].isdigit()  # no empty/corrupt section


@pytest.mark.skipif(not pa.PROSE_DIR.exists(),
                    reason="prose mirror not present (external x3d-render)")
def test_committed_anchor_map_matches_fresh_build():
    fresh = pa.dumps_artifact(pa.build_anchor_map())
    assert pa.ANCHORS_PATH.read_text() == fresh, (
        "prose_anchors.json drifted from a fresh build; "
        "rerun `python -m x3d_cpp_gen.conformance.prose_anchors build` and recommit")
