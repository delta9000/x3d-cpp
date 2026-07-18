#!/usr/bin/env python3
"""Validate and render the C++ SAI service and semantic-invariant registers.

The registers deliberately distinguish three claims:

* an ISO service exists and has defined observable semantics;
* x3d-cpp exposes some, all, or none of that service today;
* executable tests falsify the semantic claim.

Normal validation is a consistency/drift gate. ``--strict`` is the opt-in
convergence gate: every service must be implemented and every claim must have
an executable (non-``planned:``) test.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
from pathlib import Path
import re
import sys
from typing import Any

import yaml


SUPPORT_STATES = {"implemented", "partial", "planned", "unsupported"}
PROFILES = {"authoring", "inspection", "live", "browser-adapter"}
EVIDENCE_STRENGTHS = {
    "normative",
    "recurrent_intent",
    "framework_proposal",
    "cpp_policy",
}
SERVICE_REQUIRED = {
    "id": str,
    "clause": str,
    "profiles": list,
    "public_symbol": str,
    "support": str,
    "errors": str,
    "buffering": str,
    "events": str,
    "tests": list,
    "preserves": list,
}
INVARIANT_REQUIRED = {
    "id": str,
    "title": str,
    "axiom": str,
    "evidence_strength": str,
    "statement": str,
    "profiles": list,
    "preserving_operations": list,
    "falsification_tests": list,
    "property_generators": list,
    "adversarial_sequences": list,
    "normative_references": list,
    "evidence_references": list,
    "review_marker": str,
}


def _load_yaml(path: Path) -> dict[str, Any]:
    value = yaml.safe_load(path.read_text())
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected a mapping at document root")
    return value


def load_registries(conformance_dir: Path) -> tuple[dict[str, Any], dict[str, Any]]:
    return (
        _load_yaml(conformance_dir / "sai-services.yaml"),
        _load_yaml(conformance_dir / "sai-invariants.yaml"),
    )


def load_catalog(conformance_dir: Path) -> dict[str, Any]:
    return _load_yaml(conformance_dir / "sai-service-catalog.yaml")


def catalog_services(catalog_doc: dict[str, Any]) -> tuple[dict[str, str], list[str]]:
    errors: list[str] = []
    if catalog_doc.get("schema_version") != 1:
        errors.append("catalog: schema_version must be 1")
    for key in ("standard", "source_url", "reviewed"):
        if not isinstance(catalog_doc.get(key), str) or not catalog_doc[key].strip():
            errors.append(f"catalog: {key} must not be empty")
    groups = catalog_doc.get("groups")
    if not isinstance(groups, list) or not groups:
        return {}, errors + ["catalog: groups must be a non-empty list"]
    result: dict[str, str] = {}
    for index, group in enumerate(groups):
        if not isinstance(group, dict):
            errors.append(f"catalog group[{index}]: expected a mapping")
            continue
        name = group.get("name", f"group[{index}]")
        services = group.get("services")
        if not isinstance(services, dict) or not services:
            errors.append(f"catalog {name}: services must be a non-empty mapping")
            continue
        for clause, service_name in services.items():
            if not isinstance(clause, str) or not re.fullmatch(r"6(?:\.\d+){2,3}", clause):
                errors.append(f"catalog {name}: invalid service clause {clause}")
                continue
            if not isinstance(service_name, str) or not service_name.strip():
                errors.append(f"catalog {clause}: service name must not be empty")
                continue
            if clause in result:
                errors.append(f"catalog: duplicate service clause {clause}")
            result[clause] = service_name
    return result, errors


def catalog_cpp_binding(
    catalog_doc: dict[str, Any],
) -> tuple[dict[str, str], str, list[str]]:
    """Return independently audited C++-binding-only semantic obligations."""
    binding = catalog_doc.get("cpp_binding")
    if not isinstance(binding, dict):
        return {}, "missing", ["catalog: cpp_binding must be a mapping"]
    errors: list[str] = []
    for key in ("standard", "source_url", "reviewed"):
        if not isinstance(binding.get(key), str) or not binding[key].strip():
            errors.append(f"catalog cpp_binding: {key} must not be empty")
    audit_status = binding.get("audit_status")
    if audit_status not in {"incomplete", "complete"}:
        errors.append("catalog cpp_binding: audit_status must be incomplete or complete")
        audit_status = "invalid"
    obligations = binding.get("obligations")
    if not isinstance(obligations, dict) or not obligations:
        return {}, audit_status, errors + [
            "catalog cpp_binding: obligations must be a non-empty mapping"
        ]
    result: dict[str, str] = {}
    for obligation_id, statement in obligations.items():
        if not isinstance(obligation_id, str) or not re.fullmatch(
            r"CPP-[A-Z0-9.\-]+", obligation_id
        ):
            errors.append(
                f"catalog cpp_binding: invalid obligation id {obligation_id}"
            )
            continue
        if not isinstance(statement, str) or not statement.strip():
            errors.append(
                f"catalog cpp_binding {obligation_id}: statement must not be empty"
            )
            continue
        result[obligation_id] = statement
    return result, audit_status, errors


def _nonempty_strings(value: Any) -> bool:
    return isinstance(value, list) and bool(value) and all(
        isinstance(item, str) and bool(item.strip()) for item in value
    )


def statement_review_marker(statement: str) -> str:
    """Bind human review evidence to the exact normative invariant prose."""
    digest = hashlib.sha256(statement.encode("utf-8")).hexdigest()[:12]
    return f"reviewed:sha256:{digest}"


def _validate_records(
    records: Any, required: dict[str, type], kind: str
) -> list[str]:
    if not isinstance(records, list):
        return [f"{kind}s: expected a list"]
    errors: list[str] = []
    seen: set[str] = set()
    for index, record in enumerate(records):
        if not isinstance(record, dict):
            errors.append(f"{kind}[{index}]: expected a mapping")
            continue
        record_id = record.get("id", f"{kind}[{index}]")
        if isinstance(record_id, str) and record_id in seen:
            errors.append(f"duplicate {kind} id {record_id}")
        elif isinstance(record_id, str):
            seen.add(record_id)
        for key, expected_type in required.items():
            value = record.get(key)
            if not isinstance(value, expected_type):
                errors.append(f"{record_id}: {key} must be {expected_type.__name__}")
            elif expected_type is str and not value.strip():
                errors.append(f"{record_id}: {key} must not be empty")
            elif expected_type is list and not _nonempty_strings(value):
                errors.append(f"{record_id}: {key} must be a non-empty string list")
            elif expected_type is list and len(value) != len(set(value)):
                errors.append(f"{record_id}: duplicate {key}")
    return errors


def validate_registries(
    services_doc: dict[str, Any],
    invariants_doc: dict[str, Any],
    *,
    strict: bool = False,
    known_tests: set[str] | None = None,
    catalog_doc: dict[str, Any] | None = None,
) -> list[str]:
    errors: list[str] = []
    if services_doc.get("schema_version") != 1:
        errors.append("services: schema_version must be 1")
    if invariants_doc.get("schema_version") != 1:
        errors.append("invariants: schema_version must be 1")

    services = services_doc.get("services", [])
    invariants = invariants_doc.get("invariants", [])
    axioms = invariants_doc.get("axioms", [])
    errors.extend(_validate_records(services, SERVICE_REQUIRED, "service"))
    errors.extend(_validate_records(invariants, INVARIANT_REQUIRED, "invariant"))

    catalog: dict[str, str] = {}
    if catalog_doc is not None:
        catalog, catalog_errors = catalog_services(catalog_doc)
        errors.extend(catalog_errors)
        binding_catalog, binding_audit_status, binding_errors = catalog_cpp_binding(
            catalog_doc
        )
        errors.extend(binding_errors)
    else:
        binding_catalog, binding_audit_status = {}, "missing"

    if not isinstance(axioms, list) or not axioms:
        errors.append("axioms: expected a non-empty list")
        axioms = []
    axiom_ids: set[str] = set()
    for index, axiom in enumerate(axioms):
        if not isinstance(axiom, dict):
            errors.append(f"axiom[{index}]: expected a mapping")
            continue
        axiom_id = axiom.get("id")
        if not isinstance(axiom_id, str) or not axiom_id:
            errors.append(f"axiom[{index}]: id must not be empty")
        elif axiom_id in axiom_ids:
            errors.append(f"duplicate axiom id {axiom_id}")
        else:
            axiom_ids.add(axiom_id)
        for key in ("title", "statement"):
            if not isinstance(axiom.get(key), str) or not axiom[key].strip():
                errors.append(f"{axiom_id or f'axiom[{index}]'}: {key} must not be empty")

    invariant_ids = {
        item.get("id") for item in invariants if isinstance(item, dict) and isinstance(item.get("id"), str)
    }
    if isinstance(invariants, list):
        for invariant in invariants:
            if not isinstance(invariant, dict):
                continue
            invariant_id = invariant.get("id", "invariant")
            axiom = invariant.get("axiom")
            if isinstance(axiom, str) and axiom not in axiom_ids:
                errors.append(f"{invariant_id}: unknown axiom {axiom}")
            evidence_strength = invariant.get("evidence_strength")
            if (
                isinstance(evidence_strength, str)
                and evidence_strength not in EVIDENCE_STRENGTHS
            ):
                errors.append(
                    f"{invariant_id}: evidence_strength must be one of "
                    f"{sorted(EVIDENCE_STRENGTHS)}"
                )
            statement = invariant.get("statement")
            review_marker = invariant.get("review_marker")
            if (
                isinstance(statement, str)
                and isinstance(review_marker, str)
                and review_marker != statement_review_marker(statement)
            ):
                errors.append(
                    f"{invariant_id}: review_marker does not match statement; "
                    "review the prose and update its digest"
                )
            if known_tests is not None:
                for test in invariant.get("falsification_tests", []):
                    if (
                        isinstance(test, str)
                        and not test.startswith("planned:")
                        and test not in known_tests
                    ):
                        errors.append(f"{invariant_id}: test {test} is an unknown CTest")
            if strict and not _has_executable_test(invariant.get("falsification_tests")):
                errors.append(f"{invariant_id}: has no executable falsification test")

    preserved_ids: set[str] = set()
    if isinstance(services, list):
        for service in services:
            if not isinstance(service, dict):
                continue
            service_id = service.get("id", "service")
            support = service.get("support")
            if isinstance(support, str) and support not in SUPPORT_STATES:
                errors.append(
                    f"{service_id}: support must be one of {sorted(SUPPORT_STATES)}"
                )
            profiles = service.get("profiles", [])
            unknown_profiles = (
                {profile for profile in profiles if isinstance(profile, str)} - PROFILES
                if isinstance(profiles, list)
                else set()
            )
            if unknown_profiles:
                errors.append(
                    f"{service_id}: profiles contains unknown values {sorted(unknown_profiles)}"
                )
            binding_obligations = service.get("binding_obligations", [])
            if binding_obligations and not _nonempty_strings(binding_obligations):
                errors.append(
                    f"{service_id}: binding_obligations must be a non-empty string list"
                )
            elif isinstance(binding_obligations, list):
                if len(binding_obligations) != len(set(binding_obligations)):
                    errors.append(f"{service_id}: duplicate binding_obligations")
                for obligation_id in binding_obligations:
                    if obligation_id not in binding_catalog:
                        errors.append(
                            f"{service_id}: unknown C++ binding obligation {obligation_id}"
                        )
            for invariant_id in service.get("preserves", []):
                if not isinstance(invariant_id, str):
                    continue
                if invariant_id not in invariant_ids:
                    errors.append(f"{service_id}: unknown preserved invariant {invariant_id}")
                else:
                    preserved_ids.add(invariant_id)
            if known_tests is not None:
                for test in service.get("tests", []):
                    if (
                        isinstance(test, str)
                        and not test.startswith("planned:")
                        and test not in known_tests
                    ):
                        errors.append(f"{service_id}: test {test} is an unknown CTest")
            if strict and support != "implemented":
                errors.append(f"{service_id}: support={support}, expected implemented")
            public_symbol = service.get("public_symbol")
            if strict and (
                not isinstance(public_symbol, str)
                or public_symbol.startswith("planned:")
            ):
                errors.append(f"{service_id}: has no public symbol")
            if strict and not _has_executable_test(service.get("tests")):
                errors.append(f"{service_id}: has no executable test")
    for invariant_id in sorted(invariant_ids - preserved_ids):
        errors.append(f"{invariant_id}: has no preserving service")
    if strict and catalog:
        mapped = _mapped_catalog_clauses(services, set(catalog))
        for clause in sorted(set(catalog) - mapped, key=_clause_key):
            errors.append(f"{clause} {catalog[clause]}: unmapped ISO service")
        mapped_binding = _mapped_binding_obligations(services, set(binding_catalog))
        for obligation_id in sorted(set(binding_catalog) - mapped_binding):
            errors.append(
                f"{obligation_id} {binding_catalog[obligation_id]}: "
                "unmapped C++ binding obligation"
            )
        if binding_audit_status != "complete":
            errors.append(
                "C++ binding catalog audit is incomplete; strict conformance fails closed"
            )
    return errors


def _has_executable_test(tests: Any) -> bool:
    return isinstance(tests, list) and any(
        isinstance(test, str) and test and not test.startswith("planned:")
        for test in tests
    )


def _clause_key(clause: str) -> tuple[int, ...]:
    return tuple(int(part) for part in clause.split("."))


def _mapped_catalog_clauses(services: Any, catalog: set[str]) -> set[str]:
    if not isinstance(services, list):
        return set()
    mapped: set[str] = set()
    for service in services:
        if not isinstance(service, dict) or not isinstance(service.get("clause"), str):
            continue
        mapped.update(
            clause
            for clause in re.findall(r"\b6(?:\.\d+){2,3}\b", service["clause"])
            if clause in catalog
        )
    return mapped


def _mapped_binding_obligations(services: Any, catalog: set[str]) -> set[str]:
    if not isinstance(services, list):
        return set()
    return {
        obligation_id
        for service in services
        if isinstance(service, dict)
        for obligation_id in service.get("binding_obligations", [])
        if isinstance(obligation_id, str) and obligation_id in catalog
    }


def render_baseline(
    services_doc: dict[str, Any],
    invariants_doc: dict[str, Any],
    *,
    catalog_doc: dict[str, Any] | None = None,
) -> str:
    services = sorted(services_doc["services"], key=lambda item: item["id"])
    invariants = sorted(invariants_doc["invariants"], key=lambda item: item["id"])
    preserving_services: dict[str, list[str]] = {
        invariant["id"]: [] for invariant in invariants
    }
    for service in services:
        for invariant_id in service["preserves"]:
            preserving_services.setdefault(invariant_id, []).append(service["id"])
    counts = Counter(item["support"] for item in services)
    profile_counts = Counter(
        profile for service in services for profile in service["profiles"]
    )
    profile_support = Counter(
        (profile, service["support"])
        for service in services
        for profile in service["profiles"]
    )
    catalog, _ = catalog_services(catalog_doc) if catalog_doc is not None else ({}, [])
    if catalog_doc is not None:
        binding_catalog, binding_audit_status, _ = catalog_cpp_binding(catalog_doc)
    else:
        binding_catalog, binding_audit_status = {}, "missing"
    mapped_catalog = _mapped_catalog_clauses(services, set(catalog))
    mapped_binding = _mapped_binding_obligations(services, set(binding_catalog))
    count_text = ", ".join(
        f"{counts[state]} {state}"
        for state in ("implemented", "partial", "planned", "unsupported")
        if counts[state]
    )
    lines = [
        "# C++ SAI convergence baseline",
        "",
        "_Generated by `scripts/sai_conformance.py` from the two SAI YAML registers; do not edit._",
        "",
        f"**Services:** {len(services)} total — {count_text}.",
        "**Profiles:** "
        + ", ".join(f"{profile}: {profile_counts[profile]}" for profile in sorted(PROFILES))
        + ".",
        f"**Semantic invariants:** {len(invariants)} under {len(invariants_doc['axioms'])} axioms.",
        (
            f"**Normative catalog:** {len(mapped_catalog)}/{len(catalog)} mapped."
            if catalog
            else "**Normative catalog:** not supplied."
        ),
        (
            f"**C++ binding obligations:** {len(mapped_binding)}/{len(binding_catalog)} "
            f"mapped (audit {binding_audit_status})."
            if binding_catalog
            else "**C++ binding obligations:** catalog missing."
        ),
        "",
        "Normal validation checks that claims are complete and cross-linked. The opt-in",
        "strict gate also requires every service to be implemented and every claim to have",
        "an executable falsification test.",
        "",
        "## Profile × support",
        "",
        "| Profile | Implemented | Partial | Planned | Unsupported |",
        "|---|---:|---:|---:|---:|",
    ]
    for profile in sorted(PROFILES):
        lines.append(
            f"| {profile} | {profile_support[(profile, 'implemented')]} | "
            f"{profile_support[(profile, 'partial')]} | {profile_support[(profile, 'planned')]} | "
            f"{profile_support[(profile, 'unsupported')]} |"
        )
    lines.extend([
        "",
        "## Service map",
        "",
        "| Service | Clause | Profiles | Support | Public C++ surface | Tests | C++ obligations | Buffering | Events | Errors | Preserves |",
        "|---|---|---|---|---|---:|---|---|---|---|---|",
    ])
    for service in services:
        executable = sum(
            not test.startswith("planned:") for test in service["tests"]
        )
        lines.append(
            f"| `{service['id']}` | {service['clause']} | {', '.join(service['profiles'])} | "
            f"{service['support']} | `{service['public_symbol']}` | {executable}/{len(service['tests'])} | "
            f"{', '.join(f'`{item}`' for item in service.get('binding_obligations', [])) or '—'} | "
            f"{service['buffering']} | {service['events']} | {service['errors']} | "
            f"{', '.join(f'`{item}`' for item in service['preserves'])} |"
        )
    if catalog:
        lines.extend([
            "",
            "## Unmapped normative services",
            "",
            "These catalog entries have no clause-linked row in `sai-services.yaml`.",
            "They are reported by the normal gate and fail the opt-in strict gate.",
            "",
            "| Clause | Service |",
            "|---|---|",
        ])
        for clause in sorted(set(catalog) - mapped_catalog, key=_clause_key):
            lines.append(f"| {clause} | `{catalog[clause]}` |")
    if binding_catalog:
        lines.extend([
            "",
            "## Unmapped C++ binding obligations",
            "",
            "These language-binding semantics are additional to the abstract SAI catalog.",
            "The strict gate also fails while the binding audit itself remains incomplete.",
            "",
            "| Obligation | Semantic requirement |",
            "|---|---|",
        ])
        for obligation_id in sorted(set(binding_catalog) - mapped_binding):
            lines.append(
                f"| `{obligation_id}` | {binding_catalog[obligation_id]} |"
            )
    lines.extend(
        [
            "",
            "## Semantic invariant map",
            "",
            "| Invariant | Axiom | Evidence | Review marker | Preserving services | Executable tests |",
            "|---|---|---|---|---:|---:|",
        ]
    )
    for invariant in invariants:
        executable = sum(
            not test.startswith("planned:")
            for test in invariant["falsification_tests"]
        )
        lines.append(
            f"| `{invariant['id']}` — {invariant['title']} | `{invariant['axiom']}` "
            f"| {invariant['evidence_strength']} | `{invariant['review_marker']}` | "
            f"{len(preserving_services[invariant['id']])} | "
            f"{executable}/{len(invariant['falsification_tests'])} |"
        )
    return "\n".join(lines) + "\n"


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def known_ctests(repo: Path) -> set[str]:
    """Return literal CTest names declared by this project's CMake source."""
    cmake = (repo / "CMakeLists.txt").read_text()
    return set(
        re.findall(r"add_test\s*\(\s*NAME\s+([A-Za-z0-9_]+)", cmake)
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("generate", "check", "strict"))
    args = parser.parse_args(argv)
    conf = _repo_root() / "docs" / "conformance"
    try:
        services, invariants = load_registries(conf)
        catalog = load_catalog(conf)
        errors = validate_registries(
            services,
            invariants,
            strict=args.command == "strict",
            known_tests=known_ctests(_repo_root()),
            catalog_doc=catalog,
        )
    except (OSError, ValueError, yaml.YAMLError) as exc:
        print(f"SAI CONFORMANCE: {exc}", file=sys.stderr)
        return 1
    if errors:
        print("SAI CONFORMANCE ERRORS:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    rendered = render_baseline(services, invariants, catalog_doc=catalog)
    baseline = conf / "SAI-BASELINE.md"
    if args.command == "generate":
        baseline.write_text(rendered)
        print(f"wrote {baseline.relative_to(_repo_root())}")
        return 0
    if not baseline.exists() or baseline.read_text() != rendered:
        print("SAI CONFORMANCE: generated baseline drift; run `mise run sai-baseline`", file=sys.stderr)
        return 1
    print(
        f"SAI conformance OK: {len(services['services'])} services, "
        f"{len(invariants['invariants'])} invariants"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
