from __future__ import annotations

import json
from pathlib import Path

import pytest

from scripts.benchmark_build import (
    parse_ninja_log,
    parse_time_output,
    prepare_paths,
)


def test_parse_time_output_reads_wall_user_and_system_seconds() -> None:
    assert parse_time_output(
        "__X3D_BUILD_TIME__ wall=80.65 user=1081.60 system=22.75\n"
    ) == {
        "wall_s": 80.65,
        "user_s": 1081.60,
        "system_s": 22.75,
    }


def test_parse_time_output_rejects_missing_measurement() -> None:
    with pytest.raises(ValueError, match="timing measurement"):
        parse_time_output("compiler output without the timing sentinel")


def test_parse_ninja_log_groups_object_work_by_target(tmp_path: Path) -> None:
    build_dir = tmp_path / "build"
    ninja_log = """\
# ninja log v5
0\t1250\t0\tCMakeFiles/x3d_cpp_nodes.dir/generated/Box.cpp.o\tabc
50\t900\t0\tCMakeFiles/x3d_cpp_nodes.dir/generated/Sphere.cpp.o\tdef
100\t510\t0\tCMakeFiles/x3d_parse_tests.dir/runtime/parse/tests.cpp.o\tghi
0\t2000\t0\tx3d_parse_tests\tjkl
"""

    assert parse_ninja_log(ninja_log, build_dir) == {
        "x3d_cpp_nodes": {"objects": 2, "elapsed_sum_s": 2.1},
        "x3d_parse_tests": {"objects": 1, "elapsed_sum_s": 0.41},
    }


def test_parse_ninja_log_uses_latest_record_for_rebuilt_object(
    tmp_path: Path,
) -> None:
    build_dir = tmp_path / "build"
    ninja_log = """\
# ninja log v5
0\t1000\t0\tCMakeFiles/x3d_cpp_nodes.dir/generated/Box.cpp.o\told
20\t220\t0\tCMakeFiles/x3d_cpp_nodes.dir/generated/Box.cpp.o\tnew
"""

    assert parse_ninja_log(ninja_log, build_dir) == {
        "x3d_cpp_nodes": {"objects": 1, "elapsed_sum_s": 0.2},
    }


def test_prepare_paths_rejects_build_directory_inside_source(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()

    with pytest.raises(ValueError, match="outside the source tree"):
        prepare_paths(
            source=source,
            build_dir=source / "build-benchmark",
            output=source / "build-benchmarks" / "result.json",
            reuse=False,
        )


def test_prepare_paths_rejects_existing_build_directory_without_reuse(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    build_dir = tmp_path / "build"
    build_dir.mkdir()

    with pytest.raises(ValueError, match="--reuse"):
        prepare_paths(
            source=source,
            build_dir=build_dir,
            output=source / "build-benchmarks" / "result.json",
            reuse=False,
        )


def test_prepare_paths_accepts_safe_reuse_and_creates_output_parent(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    output = source / "build-benchmarks" / "result.json"

    paths = prepare_paths(
        source=source,
        build_dir=build_dir,
        output=output,
        reuse=True,
    )

    assert paths.source == source.resolve()
    assert paths.build_dir == build_dir.resolve()
    assert paths.output == output.resolve()
    assert output.parent.is_dir()


def test_benchmark_result_shape_matches_machine_readable_contract() -> None:
    result = {
        "configure_wall_s": 0.35,
        "build_wall_s": 80.65,
        "build_user_s": 1081.60,
        "test_wall_s": 28.88,
        "targets": {
            "x3d_cpp_nodes": {"objects": 43, "elapsed_sum_s": 202.3}
        },
    }

    assert json.loads(json.dumps(result)) == result
