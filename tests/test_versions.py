"""Version tags on generated nodes and fields (ADR-0077)."""
from x3d_cpp_gen.emit.versions import VersionFloors, parse_version


def test_parse_version():
    assert parse_version("4.1") == (4, 1)
    assert parse_version("3") == (3, 0)


def test_node_floors_come_from_the_earliest_manifest():
    f = VersionFloors("4.1")
    assert f.node("Box") == (3, 0)
    assert f.node("PhysicalMaterial") == (4, 0)
    assert f.node("EnvironmentLight") == (4, 1)
    assert f.node("Tangent") == (4, 1)


def test_field_floors_are_per_node():
    f = VersionFloors("4.1")
    assert f.field("IndexedFaceSet", "coord") == (3, 0)
    assert f.field("IndexedFaceSet", "tangent") == (4, 1)
    assert f.field("OscillatorSource", "periodicWave") == (4, 1)
    assert f.field("TextureProjectorParallel", "upVector") == (4, 1)
    assert f.field("TextureProjector", "upVector") == (4, 0)


def test_names_no_manifest_defines_get_the_target_version():
    f = VersionFloors("4.1")
    assert f.node("NotARealNode") == (4, 1)
    assert f.field("Box", "notARealField") == (4, 1)


def test_an_older_target_ignores_newer_manifests():
    f = VersionFloors("4.0")
    assert "EnvironmentLight" not in f.nodes
    assert f.node("EnvironmentLight") == (4, 0)
