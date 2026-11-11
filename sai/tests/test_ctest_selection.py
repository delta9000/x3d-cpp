"""The CTest selection gate must distinguish selection from success."""
import importlib.util
from pathlib import Path
import subprocess
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("selection", ROOT / "scripts/check_ctest_selection.py")
selection = importlib.util.module_from_spec(spec)
spec.loader.exec_module(selection)


@pytest.mark.parametrize(("count", "fails"), [(0, True), (1, False), (7, False)])
def test_selector_gate_rejects_zero_matches(count, fails):
    commands = []
    def run(command, **kwargs):
        commands.append(command)
        return SimpleNamespace(returncode=0, stdout=f"[doctest] unskipped test cases passing the current filters: {count}\n")
    selected, errors = selection.selection_errors([
        {"name": "filtered", "command": ["test-binary", "--test-case=something"]},
        {"name": "package", "command": ["cmake", "-P", "consumer.cmake"]},
    ], run)
    assert selected == 1
    assert bool(errors) == fails
    assert commands == [["test-binary", "--test-case=something", "--count", "--no-colors"]]


def test_selector_gate_does_not_turn_query_failure_into_evidence():
    tests = [{"name": "bad", "command": ["test-binary", "--test-case=missing"]}]
    for result in (SimpleNamespace(returncode=1, stdout=""), SimpleNamespace(returncode=0, stdout="")):
        _, errors = selection.selection_errors(tests, lambda *a, **k: result)
        assert errors == ["bad: doctest count failed or was not reported"]
    def timeout(*a, **k):
        raise subprocess.TimeoutExpired("test-binary", 30)
    _, errors = selection.selection_errors(tests, timeout)
    assert "cannot query selection" in errors[0]
