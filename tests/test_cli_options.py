"""The generator CLI must not advertise options it ignores.

--namespace was parsed and then overwritten by a hardcoded "x3d::nodes", so
passing it silently did nothing. An ignored compatibility option is worse than
no option: it tells the caller a lie the tool cannot honour.

Note this is about the CLI surface only. generate_cpp_bindings(namespace=...)
remains a real, exercised parameter -- see tests/test_version.py.
"""

import subprocess
import sys


def _run(*args):
    return subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", *args],
        capture_output=True, text=True,
    )


def test_namespace_option_is_rejected():
    """--namespace must be gone, not silently ignored."""
    result = _run("--namespace", "foo::bar", "--out", "/tmp/should-not-be-written")
    assert result.returncode != 0, (
        "--namespace was accepted; an option the generator ignores must not exist"
    )
    assert "unrecognized arguments" in result.stderr, result.stderr


def test_namespace_absent_from_help():
    result = _run("--help")
    assert result.returncode == 0, result.stderr
    assert "--namespace" not in result.stdout
