"""The generator must not report success when verification never happened.

The project thesis is fail-closed: a gate with no inputs must never green.
Skipping is legitimate only when the caller explicitly asked for it (--no-test).
"""

import subprocess
import sys

from x3d_cpp_gen.cli import compile_and_run_test


def test_missing_compiler_fails_closed(tmp_path):
    """A compiler that is not on PATH must FAIL, not silently pass."""
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    (tmp_path / "x3d" / "nodes" / "test.cpp").write_text("int main() { return 0; }\n")
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path),
        compiler="definitely-not-a-real-compiler-xyz",
    )
    assert ok is False, "a missing compiler must fail closed"


def test_missing_generated_main_fails_closed(tmp_path):
    """A missing test.cpp is a codegen regression -- never report success."""
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path), compiler="g++",
    )
    assert ok is False, "a dropped smoke-test main must fail closed"


def test_empty_compiler_fails_closed(tmp_path):
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    (tmp_path / "x3d" / "nodes" / "test.cpp").write_text("int main() { return 0; }\n")
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path), compiler="",
    )
    assert ok is False, "--compiler '' must fail closed; use --no-test to skip"


def test_no_test_still_skips_and_succeeds(tmp_path):
    """--no-test is the ONE sanctioned way to skip. It has 5 dependents."""
    result = subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", "--no-test", "--out", str(tmp_path)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stderr
    assert not (tmp_path / "x3d" / "nodes" / "test.cpp").exists()
