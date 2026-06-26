from x3d_cpp_gen.conformance.node_floor import node_floor_map, X3D_VERSIONS, version_key
from x3d_cpp_gen.conformance.version_resolve import resolve_version


def _r(xml):
    return resolve_version(xml.encode("utf-8"))


def test_declared_version_wins():
    res = _r('<X3D version="4.0" profile="Immersive"><Scene/></X3D>')
    assert (res.version, res.stamp, res.declared) == ("4.0", "VERSION_DECLARED", True)


def test_xsd_citation_inferred():
    xml = ('<X3D profile="Immersive" '
           'xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance" '
           'xsd:noNamespaceSchemaLocation='
           '"http://www.web3d.org/specifications/x3d-3.3.xsd"><Scene/></X3D>')
    res = _r(xml)
    assert (res.version, res.stamp, res.declared) == ("3.3", "VERSION_INFERRED_XSD_CITED", False)


def test_node_floor_inferred():
    # No version, no citation, no profile — but a node forces a floor.
    # CADAssembly is a 3.1-added node (absent in 3.0).
    res = _r('<X3D><Scene><CADAssembly/></Scene></X3D>')
    assert res.stamp == "VERSION_INFERRED_NODE_FORCED"
    assert res.version == "3.1"
    assert res.declared is False


def test_profile_floor_inferred():
    # No version, no citation, no recognized node — only a profile.
    res = _r('<X3D profile="Immersive"><Scene/></X3D>')
    assert (res.version, res.stamp) == ("3.0", "VERSION_INFERRED_PROFILE_FLOOR")


def test_bare_floor_inferred():
    res = _r('<X3D><Scene/></X3D>')
    assert (res.version, res.stamp) == ("3.0", "VERSION_INFERRED_BARE_FLOOR")


def test_future_version_not_clamped():
    res = _r('<X3D version="4.2"><Scene/></X3D>')
    assert res.version == "4.2"  # forward-compat: never floor a high version


def test_x3d_versions_ascending():
    assert X3D_VERSIONS == ["3.0", "3.1", "3.2", "3.3", "4.0", "4.1"]
    assert version_key("3.10") > version_key("3.3")  # numeric, not lexicographic


def test_node_floor_box_is_3_0():
    # Box is a Core/Geometry3D primitive present since X3D 3.0.
    assert node_floor_map()["Box"] == "3.0"


def test_node_floor_map_values_are_valid_versions():
    fm = node_floor_map()
    assert fm  # non-empty
    assert set(fm.values()) <= set(X3D_VERSIONS)
    # A 4.x-era node must floor above 3.0 (sanity that not everything collapses to 3.0).
    assert any(v != "3.0" for v in fm.values())


def test_xsd_citation_substring_not_matched():
    # A trailing-garbage URL must not false-positive (word-boundary on the ext).
    xml = ('<X3D xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance" '
           'xsd:noNamespaceSchemaLocation="x3d-3.3.xsdEXTRA"><Scene/></X3D>')
    res = _r(xml)
    assert res.stamp != "VERSION_INFERRED_XSD_CITED"  # falls through to bare floor
    assert res.version == "3.0"


def test_sweep_infers_unversioned(tmp_path):
    from x3d_cpp_gen.conformance.sweep import sweep
    # One declared 4.0 file + one unversioned file with a bare floor.
    (tmp_path / "a.x3d").write_text('<X3D version="4.0"><Scene><Box/></Scene></X3D>')
    (tmp_path / "b.x3d").write_text('<X3D><Scene><Box/></Scene></X3D>')
    counts = sweep(str(tmp_path))
    assert counts["validated"] == 2          # both bound to an oracle now
    assert counts["skipped_no_manifest"] == 0
    assert counts["inferred"] == 1           # only b.x3d was inferred


def test_report_records_inference(tmp_path):
    from x3d_cpp_gen.conformance.report import build_report
    (tmp_path / "declared.x3d").write_text('<X3D version="4.0"><Scene><Box/></Scene></X3D>')
    (tmp_path / "bare.x3d").write_text('<X3D><Scene><Box/></Scene></X3D>')
    rpt = build_report(str(tmp_path))
    assert rpt["totals"]["validated"] == 2
    assert rpt["totals"].get("skippedNoManifest", 0) == 0
    inf = rpt["versionResolution"]
    assert inf["declared"] == 1
    assert inf["inferred"] == 1
    assert inf["byStamp"]["VERSION_INFERRED_BARE_FLOOR"] == 1
    assert inf["bareFloorLowTrust"] == 1
    # The bare file is bound to the 3.0 oracle, not skipped.
    assert "3.0" in rpt["perVersion"]
