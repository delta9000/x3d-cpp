"""Shared fixtures.

`pinned_clang_format` resolves the formatter the golden tests must use. The
golden tree is byte-exact, so only the pinned clang-format can reproduce it.
Resolution matches the generator CLI and scripts/check_golden.sh: $CLANG_FORMAT
if set, else `clang-format` on PATH.

Outside CI, a missing or wrong-version formatter SKIPS the golden tests with
install instructions, so a stock checkout isn't red for an environment reason.
Under CI (`CI` set, as GitHub Actions does) it FAILS instead, so the gate can
never pass by skipping.
"""

import os
import re
import shutil
import subprocess

import pytest

# Keep in sync with mise.toml [tools] and scripts/check_golden.sh
# (test_formatter_pin.py asserts the mise pin).
PINNED_CLANG_FORMAT = "22.1.8"

_INSTALL_HINT = (
    f"install the pin with `mise install clang-format@{PINNED_CLANG_FORMAT}`, "
    f"or `uvx --from clang-format=={PINNED_CLANG_FORMAT} clang-format` and "
    f"point CLANG_FORMAT at it"
)


def clang_format_version(exe: str):
    """The full x.y.z version of `exe`, or None if it cannot be run/parsed."""
    try:
        out = subprocess.run([exe, "--version"], capture_output=True,
                             text=True, timeout=30).stdout
    except (OSError, subprocess.SubprocessError):
        return None
    m = re.search(r"version (\d+\.\d+\.\d+)", out)
    return m.group(1) if m else None


def resolve_pinned_clang_format():
    """(path, problem): the formatter to use, or why none is usable."""
    exe = os.environ.get("CLANG_FORMAT") or "clang-format"
    path = shutil.which(exe)
    if path is None:
        return None, f"clang-format not found ({exe!r}); {_INSTALL_HINT}"
    version = clang_format_version(path)
    if version != PINNED_CLANG_FORMAT:
        return None, (f"clang-format {version or 'unknown version'} at {path} "
                      f"!= pinned {PINNED_CLANG_FORMAT}; the golden tree is "
                      f"byte-exact, so {_INSTALL_HINT}")
    return path, None


@pytest.fixture(scope="session")
def pinned_clang_format():
    path, problem = resolve_pinned_clang_format()
    if problem:
        if os.environ.get("CI"):
            pytest.fail(problem)
        pytest.skip(problem)
    return path


@pytest.fixture(scope="session")
def generated_tree(tmp_path_factory):
    """The full generated source tree, generated ONCE per test session.

    Regenerating the tree (clang-format dominates) took ~17 s per test and was
    repeated by several tests; read-only tests share this one instead. The
    pinned formatter is used when available, so golden comparisons can use this
    tree too (they also request `pinned_clang_format`, which skips or fails
    otherwise); failing that, the environment's formatter.
    """
    import sys
    from pathlib import Path

    out = Path(tmp_path_factory.mktemp("generated")) / "gen"
    env = dict(os.environ)
    pinned, _problem = resolve_pinned_clang_format()
    if pinned:
        env["CLANG_FORMAT"] = pinned
    result = subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", "--out", str(out), "--no-test"],
        cwd=str(Path(__file__).resolve().parent.parent),
        env=env, capture_output=True, text=True,
    )
    assert result.returncode == 0, (
        f"regeneration failed (exit {result.returncode}):\n"
        f"{result.stdout}\n{result.stderr}")
    return out
