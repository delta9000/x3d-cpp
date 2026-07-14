from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_AGGREGATE_TARGETS = {
    "x3d_cli_gate",
    "x3d_canon_gate",
    "x3d_header_isolation",
    "x3d_parse_fuzz",
    "x3d_example_01_load_validate_convert",
    "x3d_example_02_extract_render_feed",
    "x3d_example_03_attach_behavior_tick",
    "x3d_assetresolver_swap",
    "x3d_quickjs_swap",
    "x3d_sound_swaptest",
}


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
    build_dir = tmp_path_factory.mktemp("cmake-aggregates")
    run_checked("cmake", "--preset", "ci", "-B", str(build_dir))
    return build_dir


def ctest_model(build_dir: Path) -> dict[str, object]:
    return json.loads(
        run_checked(
            "ctest", "--test-dir", str(build_dir), "--show-only=json-v1"
        )
    )


def labels_for(test: dict[str, object]) -> set[str]:
    for prop in test.get("properties", []):
        if prop["name"] == "LABELS":
            return set(prop["value"])
    return set()


def ctest_commands(build_dir: Path) -> dict[str, Path]:
    testfile = (build_dir / "CTestTestfile.cmake").read_text()
    return {
        name: Path(command)
        for name, command in re.findall(
            r'^add_test\("([^"]+)" "([^"]+)"', testfile, re.MULTILINE
        )
    }


def query_target_inputs(build_dir: Path, target: str) -> set[str]:
    output = run_checked(
        "ninja", "-C", str(build_dir), "-t", "query", target
    )
    inputs: set[str] = set()
    in_inputs = False
    for line in output.splitlines():
        if line.startswith("  input:"):
            in_inputs = True
        elif line.startswith("  outputs:"):
            in_inputs = False
        elif in_inputs and line.startswith("    "):
            dependency = line.strip().removeprefix("|| ")
            if dependency == "x3d":
                # The logical x3d_cli target has OUTPUT_NAME x3d, which is the
                # artifact CMake places on aggregate dependency edges.
                dependency = "x3d_cli"
            if dependency.startswith("x3d_"):
                inputs.add(dependency)
    return inputs


def normal_ctest_executable_targets(build_dir: Path) -> set[str]:
    targets: set[str] = set()
    for command in ctest_commands(build_dir).values():
        if command.parent.resolve() != build_dir.resolve():
            continue
        target = command.name
        if target == "x3d_cpp_all_headers" or target.startswith("x3d_example_"):
            continue
        targets.add(target)
    # x3d_cli_test is shell-driven, but its owning production target must still
    # be built by the behavior/sanitizer aggregate.
    targets.add("x3d_cli")
    return targets


def test_behavior_and_sanitizer_aggregates_cover_normal_ctest_executables(
    configured_ci: Path,
) -> None:
    expected = normal_ctest_executable_targets(configured_ci)
    behavior = query_target_inputs(configured_ci, "x3d_behavior_tests")
    sanitizer = query_target_inputs(configured_ci, "x3d_sanitizer_tests")

    assert behavior == expected
    assert sanitizer == expected
    assert not behavior.intersection(FORBIDDEN_AGGREGATE_TARGETS)
    assert not sanitizer.intersection(FORBIDDEN_AGGREGATE_TARGETS)


def test_compile_contract_aggregate_and_ctest_labels_are_complete(
    configured_ci: Path,
) -> None:
    compile_targets = query_target_inputs(configured_ci, "x3d_compile_contracts")
    assert compile_targets == {"x3d_cpp_all_headers", "x3d_header_isolation"}

    tests = {test["name"]: test for test in ctest_model(configured_ci)["tests"]}
    assert labels_for(tests["x3d_cpp_all_headers"]) == {"compile-contract"}
    assert labels_for(tests["x3d_header_isolation"]) == {"compile-contract"}
    assert labels_for(tests["x3d_install_embed_smoke"]) == {"compile-contract"}

    commands = ctest_commands(configured_ci)
    for test_name, test in tests.items():
        command = commands[test_name]
        is_normal_executable = (
            command.parent.resolve() == configured_ci.resolve()
            and command.name in normal_ctest_executable_targets(configured_ci)
        )
        if is_normal_executable or test_name in {
            "x3d_cli_test",
            "x3d_corpus_tools_cli_test",
        }:
            assert "behavior" in labels_for(test), test_name
