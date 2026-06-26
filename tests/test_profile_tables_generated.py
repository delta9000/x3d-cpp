"""Drift guard for the UOM-derived profile-fit tables.

`x3d validate`'s profile tables (tools/x3d-cli/*.gen.inc) are generated from the
UOM by scripts/gen_profile_tables.py and committed (golden). This test regenerates
them to a temp dir and asserts the committed copies match — so the tables can never
silently drift from the UOM (the regression that let the Interchange profile omit
the Interpolation component, falsely rejecting interpolator scenes).
"""

import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
GEN = REPO_ROOT / "scripts" / "gen_profile_tables.py"
COMMITTED = REPO_ROOT / "tools" / "x3d-cli"
FRAGMENTS = ["node_component_table.gen.inc", "profile_defs.gen.inc"]


def _regenerate(out_dir: Path) -> None:
    result = subprocess.run(
        [sys.executable, str(GEN), str(out_dir)],
        cwd=str(REPO_ROOT), capture_output=True, text=True,
    )
    assert result.returncode == 0, f"generator failed:\n{result.stdout}\n{result.stderr}"


def test_generated_profile_tables_match_committed(tmp_path):
    _regenerate(tmp_path)
    for frag in FRAGMENTS:
        regen = (tmp_path / frag).read_text()
        committed = (COMMITTED / frag).read_text()
        assert regen == committed, (
            f"{frag} is stale vs the UOM — run `mise run gen` and commit "
            f"tools/x3d-cli/{frag}"
        )


def test_interchange_includes_interpolation_and_environmental_effects(tmp_path):
    """Pin the specific spec facts whose omission was the original validate bug."""
    _regenerate(tmp_path)
    defs = (tmp_path / "profile_defs.gen.inc").read_text()
    interchange = next(l for l in defs.splitlines()
                       if l.startswith('{sdk::Profile::Interchange,'))
    # The X3D Interchange profile supports keyframe animation + environmental effects.
    assert '"Interpolation"' in interchange, "Interchange must include Interpolation"
    assert '"EnvironmentalEffects"' in interchange, \
        "Interchange must include EnvironmentalEffects (Background)"
