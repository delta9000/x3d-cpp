from __future__ import annotations

import json
import subprocess
from pathlib import Path

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]
AUTHORING_RUNTIME_SOURCES = {
    (REPO_ROOT / "runtime" / "codecs" / name).resolve()
    for name in (
        "FieldValueIO.cpp",
        "XmlWriter.cpp",
        "JsonWriter.cpp",
        "VrmlWriter.cpp",
        "CanonicalXmlWriter.cpp",
    )
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


def ninja_targets(build_dir: Path) -> set[str]:
    output = run_checked("ninja", "-C", str(build_dir), "-t", "targets", "all")
    return {line.split(":", 1)[0] for line in output.splitlines() if ":" in line}


def library_artifacts(build_dir: Path, stem: str) -> set[str]:
    return {
        Path(target).name
        for target in ninja_targets(build_dir)
        if stem in Path(target).name
    }


@pytest.fixture(scope="module")
def configured_ci(tmp_path_factory: pytest.TempPathFactory) -> Path:
    build_dir = tmp_path_factory.mktemp("runtime-topology-shared")
    run_checked("cmake", "--preset", "ci", "-B", str(build_dir))
    return build_dir


def test_compiled_runtime_layers_are_shared_by_default(
    configured_ci: Path,
) -> None:
    for stem in (
        "libx3d_cpp_nodes",
        "libx3d_cpp_authoring_runtime",
        "libx3d_cpp_runtime",
    ):
        artifacts = library_artifacts(configured_ci, stem)
        assert f"{stem}.so" in artifacts
        assert f"{stem}.a" not in artifacts


def test_compiled_runtime_layers_follow_static_node_opt_out(
    tmp_path: Path,
) -> None:
    build_dir = tmp_path / "runtime-topology-static"
    run_checked(
        "cmake",
        "--preset",
        "ci",
        "-B",
        str(build_dir),
        "-DX3D_CPP_SHARED_NODES=OFF",
    )

    for stem in (
        "libx3d_cpp_nodes",
        "libx3d_cpp_authoring_runtime",
        "libx3d_cpp_runtime",
    ):
        artifacts = library_artifacts(build_dir, stem)
        assert f"{stem}.a" in artifacts
        assert f"{stem}.so" not in artifacts


def test_facades_link_only_their_owned_runtime_layer(
    configured_ci: Path,
) -> None:
    sdk_commands = run_checked(
        "ninja", "-C", str(configured_ci), "-t", "commands", "x3d_sdk_facade"
    )
    authoring_commands = run_checked(
        "ninja",
        "-C",
        str(configured_ci),
        "-t",
        "commands",
        "x3d_authoring_link_contract",
    )

    assert "libx3d_cpp_runtime.so" in sdk_commands
    assert "libx3d_cpp_authoring_runtime.so" in authoring_commands
    assert "libx3d_cpp_runtime.so" not in authoring_commands


def test_authoring_implementations_compile_once_in_owned_layer(
    configured_ci: Path,
) -> None:
    commands = json.loads(
        (configured_ci / "compile_commands.json").read_text(encoding="utf-8")
    )
    entries = [
        entry
        for entry in commands
        if Path(entry["file"]).resolve() in AUTHORING_RUNTIME_SOURCES
    ]

    assert {Path(entry["file"]).resolve() for entry in entries} == (
        AUTHORING_RUNTIME_SOURCES
    )
    assert len(entries) == len(AUTHORING_RUNTIME_SOURCES)
    assert all("x3d_cpp_authoring_runtime.dir" in entry["command"] for entry in entries)
