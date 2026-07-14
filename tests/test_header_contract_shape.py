from __future__ import annotations

import json
import re
import subprocess
import tomllib
from pathlib import Path

import pytest
import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = yaml.safe_load(
    (REPO_ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
)
MISE = tomllib.loads((REPO_ROOT / "mise.toml").read_text(encoding="utf-8"))


def run_checked(*command: str) -> str:
    result = subprocess.run(
        command,
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


@pytest.fixture(scope="module")
def configured_ci(tmp_path_factory: pytest.TempPathFactory) -> Path:
    build_dir = tmp_path_factory.mktemp("header-contract")
    run_checked("cmake", "--preset", "ci", "-B", str(build_dir))
    return build_dir


def contract_headers() -> list[tuple[str, Path, Path]]:
    groups = [
        (
            "generated",
            REPO_ROOT / "generated_cpp_bindings",
            REPO_ROOT / "generated_cpp_bindings" / "x3d",
        ),
        ("runtime", REPO_ROOT / "runtime", REPO_ROOT / "runtime"),
        (
            "codecs",
            REPO_ROOT / "runtime" / "codecs",
            REPO_ROOT / "runtime" / "codecs",
        ),
        (
            "events",
            REPO_ROOT / "runtime" / "events",
            REPO_ROOT / "runtime" / "events",
        ),
        (
            "parse",
            REPO_ROOT / "runtime" / "parse",
            REPO_ROOT / "runtime" / "parse",
        ),
    ]
    headers: list[tuple[str, Path, Path]] = []
    for group, include_root, search_root in groups:
        candidates = (
            search_root.rglob("*.hpp")
            if group == "generated"
            else search_root.glob("*.hpp")
        )
        headers.extend((group, include_root, path) for path in sorted(candidates))
    return headers


def isolation_source_path(
    build_dir: Path, group: str, include_root: Path, header: Path
) -> Path:
    relative = header.relative_to(include_root).as_posix()
    safe_name = re.sub(r"[^A-Za-z0-9_]", "_", relative)
    return build_dir / "header_isolation" / group / f"{safe_name}.cpp"


def job_run_lines(job: dict[str, object]) -> list[str]:
    return [
        line.strip()
        for step in job["steps"]
        for line in step.get("run", "").splitlines()
        if line.strip()
    ]


def test_header_contract_is_one_ctest_and_one_ninja_target(
    configured_ci: Path,
) -> None:
    ctest_output = run_checked("ctest", "--test-dir", str(configured_ci), "-N")
    test_names = re.findall(r"Test\s+#\d+:\s+(\S+)", ctest_output)
    assert test_names.count("x3d_header_isolation") == 1
    assert not any(name.startswith("compile_") for name in test_names)

    target_output = run_checked(
        "ninja", "-C", str(configured_ci), "-t", "targets", "all"
    )
    target_names = [line.partition(":")[0] for line in target_output.splitlines()]
    assert target_names.count("x3d_header_isolation") == 1
    assert not any(name.startswith("x3d_syntax_") for name in target_names)


def test_header_contract_keeps_one_translation_unit_per_header(
    configured_ci: Path,
) -> None:
    expected_sources = {
        isolation_source_path(configured_ci, group, include_root, header).resolve()
        for group, include_root, header in contract_headers()
    }
    compile_commands = json.loads(
        (configured_ci / "compile_commands.json").read_text(encoding="utf-8")
    )
    actual_sources = {
        Path(entry["file"]).resolve()
        for entry in compile_commands
        if Path(entry["file"]).resolve().is_relative_to(
            (configured_ci / "header_isolation").resolve()
        )
    }

    assert actual_sources == expected_sources
    assert len(actual_sources) == len(contract_headers())


def test_header_contract_graph_does_not_build_compiled_runtimes(
    configured_ci: Path,
) -> None:
    commands = run_checked(
        "ninja",
        "-C",
        str(configured_ci),
        "-t",
        "commands",
        "x3d_compile_contracts",
    )

    for target in (
        "x3d_cpp_nodes",
        "x3d_cpp_authoring_runtime",
        "x3d_cpp_runtime",
    ):
        assert f"{target}.dir" not in commands
        assert f"lib{target}" not in commands


def test_ci_behavior_and_header_contract_jobs_are_scoped() -> None:
    jobs = WORKFLOW["jobs"]
    behavior = jobs["cpp"]
    contracts = jobs["cpp-headers"]

    assert behavior["name"] == "C++ build + ctest (gcc, PR fast gate)"
    assert contracts["name"] == "C++ header compile contracts (gcc, PR gate)"
    for job in (behavior, contracts):
        assert job["needs"] == "changes"
        assert job["if"] == "needs.changes.outputs.cpp == 'true'"

    behavior_lines = job_run_lines(behavior)
    contract_lines = job_run_lines(contracts)
    assert "cmake --preset ci" in behavior_lines
    assert (
        "cmake --build --preset ci --target x3d_behavior_tests"
        in behavior_lines
    )
    assert (
        'ctest --preset ci -L behavior --output-on-failure -j "$(nproc)"'
        in behavior_lines
    )
    assert "cmake --preset ci" in contract_lines
    assert (
        "cmake --build --preset ci --target x3d_compile_contracts"
        in contract_lines
    )
    assert (
        'ctest --preset ci -L compile-contract --output-on-failure -j "$(nproc)"'
        in contract_lines
    )

    build_lines = [
        line
        for job in (behavior, contracts)
        for line in job_run_lines(job)
        if line.startswith("cmake --build")
    ]
    assert all("--target" in line for line in build_lines)


def test_cpp_job_ccache_namespaces_are_separate_and_bounded() -> None:
    jobs = WORKFLOW["jobs"]
    expected_key_prefixes = {
        "cpp": "ccache-behavior-",
        "cpp-san": "ccache-san-",
        "cpp-headers": "ccache-headers-",
    }
    for job_name, prefix in expected_key_prefixes.items():
        job = jobs[job_name]
        cache_step = next(step for step in job["steps"] if "with" in step)
        assert cache_step["with"]["key"].startswith(prefix)
        assert "ccache --max-size=750M" in job_run_lines(job)


def test_build_ci_mirrors_both_aggregates_in_order() -> None:
    lines = [
        line.strip()
        for line in MISE["tasks"]["build-ci"]["run"].splitlines()
        if line.strip()
    ]
    assert lines == [
        "cmake --preset ci",
        "cmake --build --preset ci --target x3d_behavior_tests",
        'ctest --preset ci -L behavior --output-on-failure -j "$(nproc)"',
        "cmake --build --preset ci --target x3d_compile_contracts",
        'ctest --preset ci -L compile-contract --output-on-failure -j "$(nproc)"',
    ]


def test_compiled_parser_headers_do_not_export_implementation_dependencies() -> None:
    forbidden_includes = {
        "X3DParse.hpp": {
            "ClassicVrmlReader.hpp",
            "Inflate.hpp",
            "JsonReader.hpp",
            "NodeBuilder.hpp",
            "PathConfine.hpp",
            "Vrml97Dialect.hpp",
            "Vrml97Reader.hpp",
            "VrmlTokenizer.hpp",
            "X3DProtoExpand.hpp",
            "X3DRangeValidate.hpp",
            "XmlReaderAdapter.hpp",
        },
        "NodeBuilder.hpp": {
            "FieldValueIO.hpp",
            "VrmlTokenizer.hpp",
            "X3DRuntime.hpp",
            "x3d/nodes/X3DNodeFactory.hpp",
        },
        "ClassicVrmlReader.hpp": {
            "DynamicField.hpp",
            "FieldAliases.hpp",
            "FieldValueIO.hpp",
            "NodeBuilder.hpp",
            "RecursionLimits.hpp",
            "x3d/nodes/X3DNodeFactory.hpp",
        },
        "JsonReader.hpp": {
            "DynamicField.hpp",
            "FieldAliases.hpp",
            "FieldValueIO.hpp",
            "JsonLite.hpp",
            "NodeBuilder.hpp",
            "x3d/nodes/Script.hpp",
        },
        "Vrml97Reader.hpp": {
            "Vrml97Dialect.hpp",
            "X3DRuntime.hpp",
        },
    }

    for header_name, forbidden in forbidden_includes.items():
        text = (REPO_ROOT / "runtime" / "parse" / header_name).read_text(
            encoding="utf-8"
        )
        includes = set(re.findall(r'^#include\s+[<"]([^>"]+)[>"]', text, re.M))
        assert includes.isdisjoint(forbidden), (
            header_name,
            sorted(includes & forbidden),
        )
