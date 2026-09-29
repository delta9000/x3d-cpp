"""Drift guard for the generated profile-fit tables.

`x3d validate`'s profile tables (tools/x3d-cli/*.gen.inc) combine UOM node metadata
with normative profile levels in docs/conformance/profiles.yaml. This test
regenerates them to a temp dir and asserts the committed copies match, while pinning
key profile facts that previously drifted from the published tables.
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


def test_primary_profile_tables_use_published_component_levels(tmp_path):
    _regenerate(tmp_path)
    defs = (tmp_path / "profile_defs.gen.inc").read_text()
    rows = {
        name: next(line for line in defs.splitlines()
                   if line.startswith(f"{{sdk::Profile::{name},"))
        for name in ("Interchange", "Interactive", "Immersive")
    }
    assert '"Networking", 1' in rows["Interchange"]
    assert '"EnvironmentalSensor", 1' in rows["Interactive"]
    assert '"Navigation", 1' in rows["Interactive"]
    assert '"EnvironmentalEffects", 1' in rows["Interactive"]
    assert '"Lighting", 2' in rows["Immersive"]
    assert '"EnvironmentalEffects", 2' in rows["Immersive"]
    assert '"CubeMapTexturing"' not in rows["Immersive"]
