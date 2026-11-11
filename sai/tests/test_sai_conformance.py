"""Contract tests for the machine-readable C++ SAI convergence registers."""

import importlib.util
import hashlib
import pathlib
import sys

import pytest


REPO = pathlib.Path(__file__).resolve().parents[1]
_spec = importlib.util.spec_from_file_location(
    "sai_conformance", REPO / "scripts" / "sai_conformance.py"
)
sai = importlib.util.module_from_spec(_spec)
sys.modules["sai_conformance"] = sai
_spec.loader.exec_module(sai)


def service(**overrides):
    value = {
        "id": "SVC-FIELD-READ",
        "clause": "ISO/IEC 19775-2:2015, 6.7.4",
        "profiles": ["inspection"],
        "public_symbol": "x3d::X3DField::getValue",
        "support": "partial",
        "errors": "typed result; no exceptions for ordinary absence",
        "buffering": "immediate read of committed state",
        "events": "none",
        "tests": ["x3d_sai_field_read"],
        "preserves": ["INV-ID-1"],
    }
    value.update(overrides)
    return value


def invariant(**overrides):
    statement = "Repeated lookup of a live object preserves identity."
    value = {
        "id": "INV-ID-1",
        "title": "Stable identity",
        "axiom": "AX-1",
        "evidence_strength": "normative",
        "statement": statement,
        "falsification_tests": ["planned:sai_identity_repeated_lookup"],
        "profiles": ["inspection"],
        "preserving_operations": ["node lookup"],
        "property_generators": ["planned:identity_graphs"],
        "adversarial_sequences": ["planned:alias_churn"],
        "normative_references": ["ISO/IEC 19775-2:2015, 5.3"],
        "evidence_references": ["mailing-list:identity"],
        "review_marker": f"reviewed:sha256:{hashlib.sha256(statement.encode()).hexdigest()[:12]}",
    }
    value.update(overrides)
    return value


def registries():
    services = {"schema_version": 1, "services": [service()]}
    invariants = {
        "schema_version": 1,
        "axioms": [
            {
                "id": "AX-1",
                "title": "Identity is observable",
                "statement": "The API preserves observable object identity.",
            }
        ],
        "invariants": [invariant()],
    }
    return services, invariants


def test_registry_contract_accepts_well_formed_cross_links():
    services, invariants = registries()
    assert sai.validate_registries(services, invariants) == []


@pytest.mark.parametrize(
    ("key", "bad_value"),
    [
        ("support", "mostly"),
        ("profiles", []),
        ("public_symbol", ""),
        ("tests", "x3d_sai_field_read"),
        ("preserves", []),
    ],
)
def test_service_schema_rejects_ambiguous_or_missing_contract_data(key, bad_value):
    services, invariants = registries()
    services["services"][0][key] = bad_value
    errors = sai.validate_registries(services, invariants)
    assert any("SVC-FIELD-READ" in error and key in error for error in errors)


@pytest.mark.parametrize(
    ("key", "bad_value"),
    [("profiles", [{}]), ("tests", [42]), ("preserves", [[]])],
)
def test_schema_reports_malformed_list_items_without_crashing(key, bad_value):
    services, invariants = registries()
    services["services"][0][key] = bad_value
    errors = sai.validate_registries(
        services, invariants, known_tests={"x3d_sai_field_read"}
    )
    assert any("SVC-FIELD-READ" in error and key in error for error in errors)


def test_schema_reports_unhashable_semantic_values_without_crashing():
    services, invariants = registries()
    services["services"][0]["support"] = []
    invariants["invariants"][0]["axiom"] = []
    invariants["invariants"][0]["evidence_strength"] = []
    errors = sai.validate_registries(services, invariants)
    assert any("support" in error for error in errors)
    assert any("axiom" in error for error in errors)
    assert any("evidence_strength" in error for error in errors)


def test_registry_rejects_duplicate_ids_and_dangling_invariant_links():
    services, invariants = registries()
    services["services"].append(service())
    services["services"][0]["preserves"].append("INV-NOT-REGISTERED")
    errors = sai.validate_registries(services, invariants)
    assert any("duplicate service id SVC-FIELD-READ" in error for error in errors)
    assert any("INV-NOT-REGISTERED" in error for error in errors)


def test_registry_rejects_duplicate_tests_and_invariant_links():
    services, invariants = registries()
    services["services"][0]["tests"] *= 2
    services["services"][0]["preserves"] *= 2
    errors = sai.validate_registries(services, invariants)
    assert any("SVC-FIELD-READ: duplicate tests" in error for error in errors)
    assert any("SVC-FIELD-READ: duplicate preserves" in error for error in errors)


def test_registry_rejects_unknown_axiom_and_evidence_strength():
    services, invariants = registries()
    invariants["invariants"][0]["axiom"] = "AX-MISSING"
    invariants["invariants"][0]["evidence_strength"] = "strong vibes"
    errors = sai.validate_registries(services, invariants)
    assert any("AX-MISSING" in error for error in errors)
    assert any("evidence_strength" in error for error in errors)


def test_registry_requires_review_marker_to_track_exact_invariant_statement():
    services, invariants = registries()
    invariants["invariants"][0]["statement"] += " Changed without review."
    errors = sai.validate_registries(services, invariants)
    assert any("review_marker does not match statement" in error for error in errors)


def test_registry_rejects_unknown_profiles_and_unpreserved_invariants():
    services, invariants = registries()
    services["services"][0]["profiles"] = ["rendering"]
    invariants["invariants"].append(
        invariant(id="INV-ID-ORPHAN", title="Unmapped semantic claim")
    )
    errors = sai.validate_registries(services, invariants)
    assert any("profiles" in error and "rendering" in error for error in errors)
    assert any("INV-ID-ORPHAN" in error and "no preserving service" in error for error in errors)


def test_strict_mode_turns_unimplemented_services_and_planned_tests_into_gaps():
    services, invariants = registries()
    services["services"][0]["support"] = "planned"
    services["services"][0]["tests"] = ["planned:x3d_sai_field_read"]
    errors = sai.validate_registries(services, invariants, strict=True)
    assert any("support=planned" in error for error in errors)
    assert any("has no executable test" in error for error in errors)


def test_strict_mode_rejects_placeholder_public_symbol_even_if_marked_implemented():
    services, invariants = registries()
    services["services"][0]["support"] = "implemented"
    services["services"][0]["public_symbol"] = "planned:x3d::sai::field::read"
    errors = sai.validate_registries(services, invariants, strict=True)
    assert any("has no public symbol" in error for error in errors)


def test_strict_mode_rejects_mixed_executable_and_planned_evidence():
    services, invariants = registries()
    services["services"][0]["support"] = "implemented"
    services["services"][0]["tests"] = ["sai_field_read", "planned:sai_field_error"]
    invariants["invariants"][0]["falsification_tests"] = [
        "sai_identity", "planned:sai_stale_identity"
    ]
    assert sai.validate_registries(services, invariants) == []
    errors = sai.validate_registries(services, invariants, strict=True)
    assert "SVC-FIELD-READ: unresolved service test planned:sai_field_error" in errors
    assert "INV-ID-1: unresolved falsification test planned:sai_stale_identity" in errors


def test_repository_gate_rejects_unknown_executable_test_mapping():
    services, invariants = registries()
    errors = sai.validate_registries(
        services, invariants, known_tests={"some_other_ctest"}
    )
    assert any("x3d_sai_field_read" in error and "unknown CTest" in error for error in errors)


def test_repository_gate_rejects_unknown_invariant_test_mapping():
    services, invariants = registries()
    invariants["invariants"][0]["falsification_tests"] = ["typo_ctest"]
    errors = sai.validate_registries(
        services, invariants, known_tests={"x3d_sai_field_read"}
    )
    assert any("typo_ctest" in error and "unknown CTest" in error for error in errors)


def test_strict_gate_uses_independent_iso_catalog_as_completeness_oracle():
    services, invariants = registries()
    catalog = {
        "schema_version": 1,
        "standard": "ISO/IEC 19775-2:2015",
        "source_url": "https://example.test/services",
        "reviewed": "2026-07-18",
        "cpp_binding": {
            "standard": "ISO/IEC 19777-4",
            "source_url": "https://example.test/cpp-binding",
            "reviewed": "2026-07-18",
            "audit_status": "complete",
            "obligations": {"CPP-4.3.3.1": "identity-bound user data"},
        },
        "groups": [
            {
                "name": "fixture",
                "services": {"6.7.4": "getName", "6.7.5": "getValue"},
            }
        ],
    }
    normal = sai.validate_registries(services, invariants, catalog_doc=catalog)
    strict = sai.validate_registries(
        services, invariants, catalog_doc=catalog, strict=True
    )
    assert not any("unmapped ISO service" in error for error in normal)
    assert any("6.7.5 getValue: unmapped ISO service" in error for error in strict)
    assert any(
        "CPP-4.3.3.1 identity-bound user data: unmapped C++ binding obligation"
        in error
        for error in strict
    )
    report = sai.render_baseline(services, invariants, catalog_doc=catalog)
    assert "1/2 mapped" in report
    assert "0/1 mapped (audit complete)" in report


def test_strict_gate_fails_closed_while_cpp_binding_audit_is_incomplete():
    services, invariants = registries()
    catalog = {
        "schema_version": 1,
        "standard": "ISO/IEC 19775-2:2015",
        "source_url": "https://example.test/services",
        "reviewed": "2026-07-18",
        "groups": [{"name": "fixture", "services": {"6.7.4": "getName"}}],
        "cpp_binding": {
            "standard": "ISO/IEC 19777-4",
            "source_url": "https://example.test/cpp-binding",
            "reviewed": "2026-07-18",
            "audit_status": "incomplete",
            "obligations": {"CPP-4.3.3.1": "identity-bound user data"},
        },
    }
    errors = sai.validate_registries(
        services, invariants, catalog_doc=catalog, strict=True
    )
    assert any("C++ binding catalog audit is incomplete" in error for error in errors)


def test_historical_cpp_inventory_cannot_be_promoted_to_published_audit():
    catalog = sai.load_catalog(REPO / "docs" / "conformance")
    catalog["cpp_binding"]["audit_status"] = "complete"
    _, _, errors = sai.catalog_cpp_binding(catalog)
    assert any("complete audit requires reconciled published clauses" in e for e in errors)


def test_published_review_keeps_historical_ids_and_explicit_assessments():
    catalog = sai.load_catalog(REPO / "docs" / "conformance")
    binding = catalog["cpp_binding"]
    assert set(binding["published_review"]) == set(binding["obligations"])
    assert binding["published_review"]["CPP-4.3.1.2-CALLBACK-ROBUSTNESS"]["clause"] == "4.3.1.3"
    assert binding["audit_status"] == "incomplete"
    assert binding["current_published_standard"]["clause_mapping_status"] == "partial"
    binding["published_review"]["CPP-NOT-IN-CATALOG"] = {"status": "complete"}
    _, _, errors = sai.catalog_cpp_binding(catalog)
    assert any("unknown published review ID" in e for e in errors)
    assert any("invalid published review status" in e for e in errors)


@pytest.mark.parametrize("remove_reviews", [False, True])
def test_complete_published_audit_needs_coverage_and_resolved_decisions(remove_reviews):
    catalog = sai.load_catalog(REPO / "docs" / "conformance")
    binding = catalog["cpp_binding"]
    binding["audit_status"] = "complete"
    binding["source_status"] = "official_web3d_designated_IS_copy"
    binding["current_published_standard"]["clause_mapping_status"] = "complete"
    if remove_reviews:
        binding["published_review"] = {}
    _, _, errors = sai.catalog_cpp_binding(catalog)
    expected = "coverage for every obligation" if remove_reviews else "unresolved published decision"
    assert any(expected in error for error in errors)


def test_baseline_report_is_deterministic_and_exposes_gaps():
    services, invariants = registries()
    services["services"].append(
        service(
            id="SVC-WORLD-REPLACE",
            support="planned",
            public_symbol="planned:x3d::Browser::replaceWorld",
            tests=["planned:x3d_sai_replace_world"],
        )
    )
    first = sai.render_baseline(services, invariants)
    second = sai.render_baseline(services, invariants)
    assert first == second
    assert "1 partial" in first
    assert "1 planned" in first
    assert "inspection: 2" in first
    assert "SVC-WORLD-REPLACE" in first
    assert "INV-ID-1" in first
    assert "Preserving services" in first


def test_committed_registries_are_valid_and_report_is_current():
    services, invariants = sai.load_registries(REPO / "docs" / "conformance")
    catalog = sai.load_catalog(REPO / "docs" / "conformance")
    assert sai.validate_registries(
        services, invariants, catalog_doc=catalog, known_tests=sai.known_ctests(REPO)
    ) == []
    expected = sai.render_baseline(services, invariants, catalog_doc=catalog)
    committed = (REPO / "docs" / "conformance" / "SAI-BASELINE.md").read_text()
    assert committed == expected


def test_repository_health_does_not_claim_semantic_completion():
    services, invariants = sai.load_registries(REPO / "docs" / "conformance")
    catalog = sai.load_catalog(REPO / "docs" / "conformance")
    by_id = {row["id"]: row for row in services["services"]}
    assert by_id["SVC-NATIVE-ABI-BOUNDARY"]["support"] == "planned"
    assert by_id["SVC-BROWSER-IMPORT-DOCUMENT"]["support"] == "planned"
    assert "not an ISO abstract service" in by_id["SVC-NATIVE-ABI-BOUNDARY"]["clause"]
    errors = sai.validate_registries(
        services, invariants, catalog_doc=catalog,
        known_tests=sai.known_ctests(REPO), strict=True,
    )
    assert any("unmapped ISO service" in error for error in errors)
    assert any("C++ binding catalog audit is incomplete" in error for error in errors)
    assert any("INV-RT-2" in error for error in errors)


def test_ci_requires_register_health_without_claiming_full_conformance():
    import tomllib

    # The SAI tasks live in the enclosing x3d-cpp checkout's mise.toml.
    tasks = tomllib.loads((REPO.parent / "mise.toml").read_text())["tasks"]
    steps = tasks["sai-ci"]["run"].splitlines()
    assert "sai-ci" in tasks["ci"]["depends"]
    assert "mise run sai-conformance-gate" in steps
    assert "mise run sai-invariants" in steps
    assert "mise run sai-conformance" not in steps
    assert "--strict" in tasks["sai-conformance"]["run"]


def test_published_2015_catalog_includes_browser_option_service():
    catalog = sai.load_catalog(REPO / "docs" / "conformance")
    clauses, errors = sai.catalog_services(catalog)
    assert errors == []
    assert clauses["6.3.26"] == "setBrowserOption"
    assert len(clauses) == 91
