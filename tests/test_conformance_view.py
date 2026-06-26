"""Tests for the conformance-view generator (scripts/conformance_view.py).

Drives the pure extractors + join with small inline fixtures, validates the
findings schema, and runs one smoke test over the real repo (generation must
succeed with no unresolved findings).
"""

import importlib.util
import pathlib
import sys

import pytest

REPO = pathlib.Path(__file__).resolve().parents[1]
_spec = importlib.util.spec_from_file_location(
    "conformance_view", REPO / "scripts" / "conformance_view.py"
)
cv = importlib.util.module_from_spec(_spec)
sys.modules["conformance_view"] = cv  # so @dataclass can resolve the module
_spec.loader.exec_module(cv)


# --- node header parsing ---------------------------------------------------
def test_parse_node_header_concrete():
    text = """
    class Box : public virtual X3DGeometryNode {
      static std::string componentName() { return "Geometry3D"; }
      static int componentLevel() { return 1; }
    };
    """
    n = cv.parse_node_header("Box", text)
    assert n.name == "Box"
    assert n.component == "Geometry3D"
    assert n.level == 1
    assert n.abstract is False


def test_parse_node_header_abstract_has_no_component():
    # Abstract base types carry no componentName(); they are not concrete nodes.
    text = "class X3DGeometryNode : public virtual X3DNode {\n};\n"
    n = cv.parse_node_header("X3DGeometryNode", text)
    assert n.abstract is True


# --- registry table parsing ------------------------------------------------
REGISTRY = """
const std::unordered_map<std::string, std::vector<InterfaceId>>& table() {
    static const std::unordered_map<std::string, std::vector<InterfaceId>> t = {
        {"Box", {InterfaceId::X3DGeometryNode, InterfaceId::X3DNode}},
        {"TimeSensor", {InterfaceId::X3DChildNode, InterfaceId::X3DSensorNode, InterfaceId::X3DTimeDependentNode}},
        {"SplinePositionInterpolator", {InterfaceId::X3DChildNode, InterfaceId::X3DInterpolatorNode, InterfaceId::X3DNode}},
    };
    return t;
}
"""


def test_parse_registry_maps_node_to_interfaces():
    table = cv.parse_registry_table(REGISTRY)
    assert table["Box"] == {"X3DGeometryNode", "X3DNode"}
    assert "X3DTimeDependentNode" in table["TimeSensor"]
    assert "X3DInterpolatorNode" in table["SplinePositionInterpolator"]


def test_behavioral_classification():
    table = cv.parse_registry_table(REGISTRY)
    assert cv.is_behavioral(table["TimeSensor"]) is True
    assert cv.is_behavioral(table["SplinePositionInterpolator"]) is True
    assert cv.is_behavioral(table["Box"]) is False


# --- System-wiring detection ----------------------------------------------
def test_detect_system_targets_concrete_and_template():
    text = """
    void attach(X3DNode *node) {
      if (auto *n = dynamic_cast<TimeSensor *>(node)) { ... }
    }
    systems.push_back(std::make_shared<SplineInterpolatorSystem<SplinePositionInterpolator, SFVec3f>>());
    systems.push_back(std::make_shared<InterpolatorSystem<ScalarInterpolator, float>>(...));
    """
    targets = cv.detect_system_targets(text)
    assert "TimeSensor" in targets
    assert "SplinePositionInterpolator" in targets
    assert "ScalarInterpolator" in targets
    # NodeT is a template parameter, not a concrete node — must be excluded.
    assert "NodeT" not in targets
    assert "X3DNode" not in targets


def test_detect_system_targets_string_compare_in_system_file():
    # ViewDependentSystem wires by nodeTypeName() string compare, not dynamic_cast.
    text = '''
    void attach(X3DNode *node) {
      const std::string t = node->nodeTypeName();
      if (t == "LOD") {}
      else if (t == "ProximitySensor" || node->nodeTypeName() == "VisibilitySensor") {}
    }
    '''
    targets = cv.detect_system_targets(text, is_system_file=True)
    assert {"ProximitySensor", "VisibilitySensor", "LOD"} <= targets
    # The same string-compare in a NON-system file is ignored (codec/reader noise).
    assert "ProximitySensor" not in cv.detect_system_targets(text, is_system_file=False)


# --- findings schema -------------------------------------------------------
def test_validate_finding_accepts_good():
    f = {
        "id": "INTERP-01",
        "interfaces": ["X3DInterpolatorNode"],
        "nodes": ["SplinePositionInterpolator"],
        "severity": "critical",
        "status": "closed",
        "summary": "ok",
    }
    assert cv.validate_finding(f) == []


@pytest.mark.parametrize(
    "mutate",
    [
        lambda f: f.pop("id"),
        lambda f: f.update(severity="catastrophic"),
        lambda f: f.update(status="wip"),
        lambda f: f.pop("summary"),
    ],
)
def test_validate_finding_rejects_bad(mutate):
    f = {
        "id": "X",
        "interfaces": [],
        "nodes": ["Box"],
        "severity": "minor",
        "status": "open",
        "summary": "s",
    }
    mutate(f)
    assert cv.validate_finding(f) != []


# --- findings loading robustness -------------------------------------------
def test_load_findings_rejects_non_list(tmp_path):
    bad = tmp_path / "findings.yaml"
    bad.write_text("key: value\n")  # a mapping, not a list
    with pytest.raises(ValueError):
        cv.load_findings(bad)


def test_load_findings_empty_ok(tmp_path):
    empty = tmp_path / "findings.yaml"
    empty.write_text("")
    assert cv.load_findings(empty) == []


# --- behaves classification join ------------------------------------------
def test_classify_behaves_non_behavioral_is_na():
    assert cv.classify_behaves(behavioral=False, wired=False, findings=[]) == "n/a"


def test_classify_behaves_inert_when_behavioral_unwired():
    assert cv.classify_behaves(behavioral=True, wired=False, findings=[]) == "inert"


def test_classify_behaves_partial_when_open_finding():
    f = [{"status": "open"}]
    assert cv.classify_behaves(behavioral=True, wired=True, findings=f) == "partial"


def test_classify_behaves_conformant_when_wired_no_open():
    f = [{"status": "closed"}]
    assert cv.classify_behaves(behavioral=True, wired=True, findings=f) == "conformant"


def test_classify_behaves_closed_finding_implies_effective_wired():
    # Heuristic missed the System, but a closed finding asserts it's done.
    assert (
        cv.classify_behaves(behavioral=True, wired=False, findings=[{"status": "closed"}])
        == "conformant"
    )


# --- profile membership ----------------------------------------------------
def test_node_profiles():
    profiles = {
        "Interchange": {"Geometry3D": 2, "Interpolation": 2},
        "Full": {"Geometry3D": 4, "Interpolation": 4},
    }
    # Geometry3D level-1 node is in both.
    assert set(cv.node_profiles("Geometry3D", 1, profiles)) == {"Interchange", "Full"}
    # A level-3 Geometry3D node exceeds Interchange's level-2 cap -> Full only.
    assert cv.node_profiles("Geometry3D", 3, profiles) == ["Full"]
    # A component absent from all profiles -> none.
    assert cv.node_profiles("Nurbs", 1, profiles) == []


# --- referential integrity -------------------------------------------------
def test_unresolved_findings_flagged():
    nodes = {"Box": cv.NodeFact("Box", "Geometry3D", 1, False)}
    findings = [
        {"id": "GOOD", "nodes": ["Box"], "interfaces": [], "status": "open",
         "severity": "minor", "summary": "s"},
        {"id": "STALE", "nodes": ["NoSuchNode"], "interfaces": [], "status": "open",
         "severity": "minor", "summary": "s"},
    ]
    unresolved = cv.unresolved_findings(findings, nodes)
    ids = {u["id"] for u in unresolved}
    assert "STALE" in ids
    assert "GOOD" not in ids


# --- real-repo smoke -------------------------------------------------------
def test_real_repo_generates_clean():
    model = cv.build_model(REPO)
    assert model["summary"]["component_count"] > 20
    # Every committed finding must resolve to a real node/interface.
    assert model["unresolved_findings"] == [], (
        "stale findings reference unknown nodes/interfaces: "
        f"{[u['id'] for u in model['unresolved_findings']]}"
    )
    # The interpolation cluster we just shipped should read as present + behaving.
    interp = next(c for c in model["components"] if c["name"] == "Interpolation")
    names = {n["name"] for n in interp["nodes"]}
    assert "SplinePositionInterpolator" in names
