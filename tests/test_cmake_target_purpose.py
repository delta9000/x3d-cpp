from __future__ import annotations

import subprocess
from pathlib import Path

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]
FIXTURE = REPO_ROOT / "tests" / "cmake" / "target_purpose"


def configure_fixture(tmp_path: Path, test_case: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            "cmake",
            "-S",
            str(FIXTURE),
            "-B",
            str(tmp_path / test_case),
            "-G",
            "Ninja",
            f"-DX3D_CPP_SOURCE_DIR={REPO_ROOT}",
            f"-DTEST_CASE={test_case}",
        ],
        capture_output=True,
        text=True,
    )


def combined_output(result: subprocess.CompletedProcess[str]) -> str:
    return result.stdout + result.stderr


def test_valid_classification_and_suite_registration_configure(
    tmp_path: Path,
) -> None:
    result = configure_fixture(tmp_path, "valid")

    assert result.returncode == 0, combined_output(result)


@pytest.mark.parametrize(
    ("test_case", "message"),
    [
        (
            "unclassified",
            "repository-owned target 'unclassified' has no X3D_TARGET_PURPOSE",
        ),
        ("duplicate", "target 'classified' already has X3D_TARGET_PURPOSE"),
        ("invalid", "invalid X3D target purpose 'accidental'"),
        (
            "missing-suite-target",
            "cannot register nonexistent test target 'does_not_exist'",
        ),
    ],
)
def test_invalid_target_metadata_fails_with_actionable_message(
    tmp_path: Path, test_case: str, message: str
) -> None:
    result = configure_fixture(tmp_path, test_case)

    assert result.returncode != 0
    assert message in combined_output(result)
    if test_case == "unclassified":
        assert (
            "repository-owned target 'unclassified_second' has no "
            "X3D_TARGET_PURPOSE"
        ) in combined_output(result)
