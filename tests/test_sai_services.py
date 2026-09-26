"""Tests for the SAI service register validator (scripts/check_sai_services.py).

Drives the schema + evidence checks with small inline fixtures, then runs the
validator over the real register — it must pass, so a broken
`docs/conformance/sai-services.yaml` fails `uv run pytest`.
"""

import importlib.util
import pathlib
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]
_spec = importlib.util.spec_from_file_location(
    "check_sai_services", REPO / "scripts" / "check_sai_services.py"
)
css = importlib.util.module_from_spec(_spec)
sys.modules["check_sai_services"] = css
_spec.loader.exec_module(css)


def _register(rows, defaults=None):
    return {"schema_version": css.SCHEMA_VERSION,
            "defaults": defaults if defaults is not None else {"query": {"kind": "query"}},
            "services": rows}


def _row(**over):
    row = {"kind": "query", "id": "SVC-X", "clause": "6.1",
           "public_symbol": "x3d::foo", "support": "partial",
           "tests": ["x3d_sai_context"], "preserves": ["INV-1"]}
    row.update(over)
    return row


# --- schema ----------------------------------------------------------------
def test_valid_register_has_no_errors():
    reg = _register([_row()])
    assert css.check_schema(reg, "<<: *query\n") == []


def test_missing_required_key():
    reg = _register([_row()])
    del reg["services"][0]["clause"]
    errs = css.check_schema(reg, "<<: *query\n")
    assert any("clause" in e for e in errs)


def test_bad_support_value():
    errs = css.check_schema(_register([_row(support="mostly")]), "<<: *query\n")
    assert any("support" in e for e in errs)


def test_bad_kind_value():
    errs = css.check_schema(_register([_row(kind="wat")]), "<<: *query\n")
    assert any("kind" in e for e in errs)


def test_duplicate_ids_rejected():
    reg = _register([_row(), _row()])
    errs = css.check_schema(reg, "<<: *query\n")
    assert any("duplicate id" in e for e in errs)


def test_undefined_anchor_rejected():
    reg = _register([_row()], defaults={})
    errs = css.check_schema(reg, "<<: *query\n")
    assert any("undefined anchors" in e for e in errs)


def test_unused_anchor_rejected():
    reg = _register([_row()], defaults={"query": {"kind": "query"},
                                        "command": {"kind": "command"}})
    errs = css.check_schema(reg, "<<: *query\n")
    assert any("unused anchors" in e for e in errs)


def test_empty_tests_rejected():
    errs = css.check_schema(_register([_row(tests=[])]), "<<: *query\n")
    assert any("tests" in e for e in errs)


# --- evidence --------------------------------------------------------------
def test_unknown_evidence_flagged():
    errs = css.check_evidence(_register([_row(tests=["x3d_sai_context", "nope"])]),
                              {"x3d_sai_context"})
    assert any("nope" in e for e in errs)


def test_known_evidence_accepted():
    assert css.check_evidence(_register([_row(tests=["x3d_sai_context"])]),
                              {"x3d_sai_context"}) == []


def test_evidence_comes_from_cmake_and_doctest(tmp_path):
    (tmp_path / "cmake").mkdir()
    (tmp_path / "cmake" / "t.cmake").write_text("add_test(NAME x3d_alpha COMMAND x)\n")
    (tmp_path / "runtime" / "tests").mkdir(parents=True)
    (tmp_path / "runtime" / "tests" / "t.cpp").write_text(
        'TEST_CASE("beta_test") {}\n')
    evidence = css.collect_evidence(tmp_path)
    assert "x3d_alpha" in evidence
    assert "beta_test" in evidence


# --- real register ---------------------------------------------------------
def test_real_register_is_clean():
    assert css.run(REPO) == [], "docs/conformance/sai-services.yaml is invalid"
