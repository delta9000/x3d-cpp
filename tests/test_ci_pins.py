"""CI must be reproducible: pinned actions, pinned uv, tested main.

A floating action tag is mutable -- the owner can repoint @v4 at any commit, so
a green run does not describe what runs next time. And setup-uv does not read
mise.toml, so pinning uv there alone leaves CI floating: the two drift apart
silently, which is worse than both floating.
"""

import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = REPO_ROOT / ".github/workflows/ci.yml"


def test_every_action_is_pinned_to_a_sha():
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "scripts/check_action_pins.py")],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr


def test_ci_runs_on_pushes_to_main():
    """Without this, main's actual head is never tested and the badge lies."""
    text = WORKFLOW.read_text()
    on_block = text.split("\non:", 1)[1].split("\npermissions:", 1)[0]
    assert "push:" in on_block, "ci.yml must run on pushes to main"
    assert "branches: [main]" in on_block


def test_uv_is_pinned_in_both_mise_and_ci():
    """setup-uv does not read mise.toml -- pinning one site is not enough."""
    mise = (REPO_ROOT / "mise.toml").read_text()
    m = re.search(r'uv\s*=\s*"([^"]+)"', mise)
    assert m, "mise.toml [tools] must pin uv"
    pinned = m.group(1)
    assert pinned != "latest", "uv must be pinned to a version, not 'latest'"

    text = WORKFLOW.read_text()
    setup_uv_steps = text.count("astral-sh/setup-uv@")
    assert setup_uv_steps > 0, "expected setup-uv steps in ci.yml"
    versions = re.findall(r'version:\s*"([^"]+)"\s*#\s*keep in sync with mise\.toml', text)
    assert len(versions) == setup_uv_steps, (
        f"{setup_uv_steps} setup-uv steps but {len(versions)} pinned `version:` "
        f"inputs; setup-uv installs latest when version is omitted"
    )
    assert set(versions) == {pinned}, (
        f"ci.yml pins uv {set(versions)} but mise.toml pins {pinned}"
    )
