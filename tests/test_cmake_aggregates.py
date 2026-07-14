from __future__ import annotations

import json
import re
import shlex
import subprocess
from pathlib import Path

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]
PRESETS = json.loads((REPO_ROOT / "CMakePresets.json").read_text())
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
DEFAULT_DOCTEST_TARGETS = {
    "x3d_geometry_scene_tests",
    "x3d_codecs_tests",
    "x3d_parse_tests",
    "x3d_extract_tests",
    "x3d_events_tests",
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
    commands: dict[str, Path] = {}
    testfile = (build_dir / "CTestTestfile.cmake").read_text()
    for line in testfile.splitlines():
        if not line.startswith("add_test(") or not line.endswith(")"):
            continue
        arguments = shlex.split(line[len("add_test(") : -1])
        if len(arguments) >= 2:
            name = arguments[0]
            bracket_quote = re.fullmatch(r"\[(=*)\[(.*)\]\1\]", name)
            if bracket_quote:
                name = bracket_quote.group(2)
            commands[name] = Path(arguments[1])
    return commands


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


def ninja_targets(build_dir: Path) -> set[str]:
    output = run_checked("ninja", "-C", str(build_dir), "-t", "targets", "all")
    return {line.split(":", 1)[0] for line in output.splitlines() if ":" in line}


def node_library_artifacts(build_dir: Path) -> set[str]:
    return {
        Path(target).name
        for target in ninja_targets(build_dir)
        if "libx3d_cpp_nodes" in Path(target).name
    }


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


def test_ctest_commands_accept_cmake_test_name_quoting_styles(
    tmp_path: Path,
) -> None:
    # CMake versions vary between bare, normal-quoted, and bracket-quoted test
    # names. All forms describe the same generated test command syntax.
    (tmp_path / "CTestTestfile.cmake").write_text(
        'add_test(x3d_cpp_all_headers "/build/x3d_cpp_all_headers")\n'
        'add_test("x3d_header_isolation" "/usr/bin/cmake" "--build")\n'
        'add_test([=[x3d_install_embed_smoke]=] "bash" "verify.sh")\n'
    )

    assert ctest_commands(tmp_path) == {
        "x3d_cpp_all_headers": Path("/build/x3d_cpp_all_headers"),
        "x3d_header_isolation": Path("/usr/bin/cmake"),
        "x3d_install_embed_smoke": Path("bash"),
    }


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
    assert labels_for(tests["x3d_install_embed_smoke"]) == {"behavior"}

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


def test_external_corpus_gates_exist_but_are_not_default_build_inputs(
    configured_ci: Path,
) -> None:
    gates = {"x3d_cli_gate", "x3d_canon_gate"}
    assert gates <= ninja_targets(configured_ci)
    assert not gates.intersection(query_target_inputs(configured_ci, "all"))


def test_generated_node_runtime_is_shared_by_default_and_excludes_test_main(
    configured_ci: Path,
) -> None:
    cache = (configured_ci / "CMakeCache.txt").read_text(encoding="utf-8")
    assert "X3D_CPP_SHARED_NODES:BOOL=ON" in cache

    artifacts = node_library_artifacts(configured_ci)
    assert "libx3d_cpp_nodes.so" in artifacts
    assert "libx3d_cpp_nodes.a" not in artifacts

    compile_commands = json.loads(
        (configured_ci / "compile_commands.json").read_text(encoding="utf-8")
    )
    compiled_sources = {Path(entry["file"]).resolve() for entry in compile_commands}
    assert (
        REPO_ROOT / "generated_cpp_bindings" / "x3d" / "nodes" / "test.cpp"
    ).resolve() not in compiled_sources


def test_generated_node_runtime_has_explicit_static_opt_out(tmp_path: Path) -> None:
    build_dir = tmp_path / "static-nodes"
    run_checked(
        "cmake",
        "--preset",
        "ci",
        "-B",
        str(build_dir),
        "-DX3D_CPP_SHARED_NODES=OFF",
    )

    cache = (build_dir / "CMakeCache.txt").read_text(encoding="utf-8")
    assert "X3D_CPP_SHARED_NODES:BOOL=OFF" in cache
    artifacts = node_library_artifacts(build_dir)
    assert "libx3d_cpp_nodes.a" in artifacts
    assert "libx3d_cpp_nodes.so" not in artifacts


def test_doctest_main_is_compiled_once_and_reused(
    configured_ci: Path,
) -> None:
    compile_commands = json.loads(
        (configured_ci / "compile_commands.json").read_text(encoding="utf-8")
    )
    test_main = (
        REPO_ROOT / "runtime" / "test_support" / "doctest_main.cpp"
    ).resolve()
    main_compiles = [
        entry
        for entry in compile_commands
        if Path(entry["file"]).resolve() == test_main
    ]

    assert len(main_compiles) == 1
    for target in DEFAULT_DOCTEST_TARGETS:
        query = run_checked(
            "ninja", "-C", str(configured_ci), "-t", "query", target
        )
        assert "libx3d_doctest_main.a" in query, target


def test_sanitizer_preset_and_drivers_are_debug_behavior_only() -> None:
    configure = next(
        preset for preset in PRESETS["configurePresets"] if preset["name"] == "san"
    )
    cache = configure["cacheVariables"]
    assert cache["CMAKE_BUILD_TYPE"] == "Debug"
    assert cache["X3D_CPP_SAN"] == "ON"
    assert cache["X3D_CPP_PER_HEADER_CHECKS"] == "OFF"

    expected_build = (
        "cmake --build --preset san --target x3d_sanitizer_tests"
    )
    expected_test = "ctest --preset san -L behavior --output-on-failure"
    assert expected_build in (REPO_ROOT / "mise.toml").read_text()
    assert expected_build in (REPO_ROOT / ".github/workflows/ci.yml").read_text()
    assert expected_test in (REPO_ROOT / "mise.toml").read_text()
    assert expected_test in (REPO_ROOT / ".github/workflows/ci.yml").read_text()


def test_sanitizer_configures_with_minimal_debug_information(
    tmp_path: Path,
) -> None:
    build_dir = tmp_path / "san"
    run_checked("cmake", "--preset", "san", "-B", str(build_dir))
    compile_commands = json.loads(
        (build_dir / "compile_commands.json").read_text(encoding="utf-8")
    )
    node_command = next(
        entry["command"]
        for entry in compile_commands
        if "x3d_cpp_nodes.dir" in entry["command"]
    )
    flags = shlex.split(node_command)

    assert "-fsanitize=address,undefined" in flags
    assert "-g" in flags
    assert "-g1" in flags
    assert flags.index("-g1") > flags.index("-g")


def test_fuzz_preset_instruments_compiled_runtime_layers(tmp_path: Path) -> None:
    build_dir = tmp_path / "fuzz"
    run_checked("cmake", "--preset", "fuzz", "-B", str(build_dir))
    compile_commands = json.loads(
        (build_dir / "compile_commands.json").read_text(encoding="utf-8")
    )

    for target in (
        "x3d_cpp_nodes.dir",
        "x3d_cpp_authoring_runtime.dir",
        "x3d_cpp_runtime.dir",
    ):
        command = next(
            entry["command"]
            for entry in compile_commands
            if target in entry["command"]
        )
        flags = shlex.split(command)
        assert "-fsanitize=fuzzer-no-link,address,undefined" in flags, target
        assert "-fno-omit-frame-pointer" in flags, target

    harness_command = next(
        entry["command"]
        for entry in compile_commands
        if "x3d_parse_fuzz.dir" in entry["command"]
    )
    assert "-fsanitize=fuzzer,address,undefined" in shlex.split(harness_command)
