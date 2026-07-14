#!/usr/bin/env python3
"""Run reproducible, uncached CMake build benchmarks.

The build directory is deliberately required to live outside the source tree so
benchmark artifacts cannot contaminate a checkout. Measurement JSON belongs in
the source tree's ignored ``build-benchmarks`` directory.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shlex
import socket
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Sequence


TIME_SENTINEL = "__X3D_BUILD_TIME__"
TIME_FORMAT = f"{TIME_SENTINEL} wall=%e user=%U system=%S"
TIME_PATTERN = re.compile(
    rf"^{re.escape(TIME_SENTINEL)}\s+"
    r"wall=(?P<wall>[0-9]+(?:\.[0-9]+)?)\s+"
    r"user=(?P<user>[0-9]+(?:\.[0-9]+)?)\s+"
    r"system=(?P<system>[0-9]+(?:\.[0-9]+)?)$",
    re.MULTILINE,
)
OBJECT_PATH_PATTERN = re.compile(
    r"(?:^|/)CMakeFiles/(?P<target>[^/]+)\.dir/.+\.(?:o|obj)$"
)


@dataclass(frozen=True)
class BenchmarkPaths:
    source: Path
    build_dir: Path
    output: Path


def _is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
    except ValueError:
        return False
    return True


def prepare_paths(
    *, source: Path, build_dir: Path, output: Path, reuse: bool
) -> BenchmarkPaths:
    """Validate benchmark paths without deleting or replacing existing data."""

    source = source.resolve()
    build_dir = build_dir.resolve()
    output = output.resolve()

    if not source.is_dir():
        raise ValueError(f"source directory does not exist: {source}")
    if _is_relative_to(build_dir, source):
        raise ValueError(
            f"benchmark build directory must be outside the source tree: {build_dir}"
        )
    if build_dir.exists() and not build_dir.is_dir():
        raise ValueError(f"benchmark build path is not a directory: {build_dir}")
    if build_dir.exists() and not reuse:
        raise ValueError(
            f"benchmark build directory already exists; pass --reuse to use it: "
            f"{build_dir}"
        )

    benchmark_output_dir = (source / "build-benchmarks").resolve()
    if not _is_relative_to(output, benchmark_output_dir):
        raise ValueError(
            f"benchmark output must be under {benchmark_output_dir}: {output}"
        )
    if output.exists() and not reuse:
        raise ValueError(
            f"benchmark output already exists; pass --reuse to replace it: {output}"
        )

    build_dir.parent.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    return BenchmarkPaths(source=source, build_dir=build_dir, output=output)


def parse_time_output(text: str) -> dict[str, float]:
    """Parse the stable sentinel emitted by GNU ``time``."""

    match = TIME_PATTERN.search(text)
    if match is None:
        raise ValueError("timing measurement is missing or malformed")
    return {
        "wall_s": float(match.group("wall")),
        "user_s": float(match.group("user")),
        "system_s": float(match.group("system")),
    }


def parse_ninja_log(text: str, build_dir: Path) -> dict[str, dict[str, int | float]]:
    """Summarize the most recent Ninja object record for each output by target."""

    build_dir = build_dir.resolve()
    latest_by_output: dict[str, tuple[int, int]] = {}
    for raw_line in text.splitlines():
        if not raw_line or raw_line.startswith("#"):
            continue
        fields = raw_line.split("\t")
        if len(fields) < 5:
            continue
        try:
            start_ms = int(fields[0])
            end_ms = int(fields[1])
        except ValueError:
            continue
        output_path = fields[3].replace("\\", "/")
        if Path(output_path).is_absolute():
            try:
                output_path = str(Path(output_path).resolve().relative_to(build_dir))
            except ValueError:
                continue
        if OBJECT_PATH_PATTERN.search(output_path) is None:
            continue
        latest_by_output[output_path] = (start_ms, end_ms)

    totals: dict[str, dict[str, int | float]] = {}
    for output_path, (start_ms, end_ms) in latest_by_output.items():
        match = OBJECT_PATH_PATTERN.search(output_path)
        assert match is not None
        target = match.group("target")
        target_totals = totals.setdefault(
            target, {"objects": 0, "elapsed_sum_s": 0.0}
        )
        target_totals["objects"] = int(target_totals["objects"]) + 1
        elapsed_s = max(0, end_ms - start_ms) / 1000.0
        target_totals["elapsed_sum_s"] = round(
            float(target_totals["elapsed_sum_s"]) + elapsed_s, 3
        )
    return dict(sorted(totals.items()))


def _run_timed(
    command: Sequence[str], *, cwd: Path, env: dict[str, str]
) -> dict[str, float]:
    with tempfile.NamedTemporaryFile(
        mode="w", prefix="x3d-build-time-", suffix=".txt", delete=False
    ) as timing_file:
        timing_path = Path(timing_file.name)
    timed_command = [
        "/usr/bin/time",
        "-f",
        TIME_FORMAT,
        "-o",
        str(timing_path),
        *command,
    ]
    print(f"+ {shlex.join(command)}", flush=True)
    try:
        subprocess.run(timed_command, cwd=cwd, env=env, check=True)
        return parse_time_output(timing_path.read_text(encoding="utf-8"))
    finally:
        timing_path.unlink(missing_ok=True)


def _command_version(command: Sequence[str]) -> str:
    try:
        result = subprocess.run(
            command,
            check=True,
            capture_output=True,
            text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return "unknown"
    output = result.stdout.strip() or result.stderr.strip()
    return output.splitlines()[0] if output else "unknown"


def _cmake_cache_value(cache_text: str, name: str) -> str | None:
    match = re.search(rf"^{re.escape(name)}:[^=]+=(.*)$", cache_text, re.MULTILINE)
    return match.group(1).strip() if match else None


def _metadata(paths: BenchmarkPaths, *, preset: str, jobs: int) -> dict[str, object]:
    cache_path = paths.build_dir / "CMakeCache.txt"
    cache_text = cache_path.read_text(encoding="utf-8", errors="replace")
    compiler = _cmake_cache_value(cache_text, "CMAKE_CXX_COMPILER") or "unknown"
    compiler_id = _cmake_cache_value(cache_text, "CMAKE_CXX_COMPILER_ID") or "unknown"
    compiler_version = (
        _cmake_cache_value(cache_text, "CMAKE_CXX_COMPILER_VERSION") or "unknown"
    )
    try:
        revision = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=paths.source,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        revision = "unknown"

    return {
        "timestamp_utc": datetime.now(UTC).isoformat(),
        "hostname": socket.gethostname(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python": platform.python_version(),
        "cmake": _command_version(["cmake", "--version"]),
        "ninja": _command_version(["ninja", "--version"]),
        "compiler": compiler,
        "compiler_id": compiler_id,
        "compiler_version": compiler_version,
        "compiler_banner": _command_version([compiler, "--version"])
        if compiler != "unknown"
        else "unknown",
        "git_revision": revision,
        "preset": preset,
        "jobs": jobs,
        "source": str(paths.source),
        "build_dir": str(paths.build_dir),
        "ccache_disabled": True,
    }


def run_benchmark(
    paths: BenchmarkPaths, *, preset: str, jobs: int
) -> dict[str, object]:
    env = os.environ.copy()
    env["CCACHE_DISABLE"] = "1"

    configure = _run_timed(
        [
            "cmake",
            "--preset",
            preset,
            "-S",
            str(paths.source),
            "-B",
            str(paths.build_dir),
        ],
        cwd=paths.source,
        env=env,
    )
    build = _run_timed(
        ["cmake", "--build", str(paths.build_dir), "--parallel", str(jobs)],
        cwd=paths.source,
        env=env,
    )
    test = _run_timed(
        [
            "ctest",
            "--test-dir",
            str(paths.build_dir),
            "--output-on-failure",
            "--parallel",
            str(jobs),
        ],
        cwd=paths.source,
        env=env,
    )

    ninja_log_path = paths.build_dir / ".ninja_log"
    ninja_log = (
        ninja_log_path.read_text(encoding="utf-8", errors="replace")
        if ninja_log_path.exists()
        else ""
    )
    return {
        "configure_wall_s": configure["wall_s"],
        "configure_user_s": configure["user_s"],
        "configure_system_s": configure["system_s"],
        "build_wall_s": build["wall_s"],
        "build_user_s": build["user_s"],
        "build_system_s": build["system_s"],
        "test_wall_s": test["wall_s"],
        "test_user_s": test["user_s"],
        "test_system_s": test["system_s"],
        "targets": parse_ninja_log(ninja_log, paths.build_dir),
        "metadata": _metadata(paths, preset=preset, jobs=jobs),
    }


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Measure an uncached CMake configure, build, and CTest run."
    )
    parser.add_argument("--source", type=Path, default=Path.cwd())
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--preset", choices=("dev", "ci", "san"), required=True)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--reuse",
        action="store_true",
        help="allow an existing build directory and replace existing output JSON",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    if args.jobs < 1:
        raise SystemExit("--jobs must be at least 1")
    source = args.source.resolve()
    output = args.output if args.output.is_absolute() else source / args.output
    try:
        paths = prepare_paths(
            source=source,
            build_dir=args.build_dir,
            output=output,
            reuse=args.reuse,
        )
    except ValueError as error:
        raise SystemExit(str(error)) from error

    result = run_benchmark(paths, preset=args.preset, jobs=args.jobs)
    paths.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"benchmark JSON: {paths.output}")
    for stage in ("configure", "build", "test"):
        print(
            f"{stage}: wall={result[f'{stage}_wall_s']:.2f}s "
            f"user={result[f'{stage}_user_s']:.2f}s "
            f"system={result[f'{stage}_system_s']:.2f}s"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
