"""The published Python package must describe itself, and carry its licenses.

pyproject publishes x3d-cpp-gen (a code generator) but used the C++ runtime's
README as its PyPI description, so a user who installed a generator saw a
gallery, renderer examples and CMake targets they did not install. The package
also bundles UOM data, so LICENSE and NOTICE must ship with it.
"""

import shutil
import subprocess
import tarfile
import zipfile
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent


def test_generator_readme_exists_and_is_scoped():
    readme = REPO_ROOT / "README.generator.md"
    assert readme.exists(), "the Python package needs its own README"
    text = readme.read_text()
    assert "x3d-cpp-gen" in text
    # It must not drag in the runtime's front-page material.
    for leaked in ("## Gallery", "cpu_raster", "target_link_libraries"):
        assert leaked not in text, (
            f"{leaked!r} is runtime documentation; a PyPI user installed a "
            f"code generator and did not install that"
        )


def test_pyproject_points_at_the_generator_readme():
    text = (REPO_ROOT / "pyproject.toml").read_text()
    assert 'readme = "README.generator.md"' in text


@pytest.mark.slow
def test_sdist_and_wheel_carry_license_and_notice(tmp_path):
    # uv is a standalone binary (installed via mise here), NOT an importable
    # Python module -- `python -m uv` fails with "No module named uv". Invoke the
    # executable, and say so plainly if it is absent.
    uv = shutil.which("uv")
    if uv is None:
        pytest.skip("uv not on PATH; run `mise install` (build artifacts need uv)")
    subprocess.run([uv, "build", "--out-dir", str(tmp_path)],
                   cwd=REPO_ROOT, check=True)
    sdist = next(tmp_path.glob("*.tar.gz"))
    wheel = next(tmp_path.glob("*.whl"))

    with tarfile.open(sdist) as tf:
        names = tf.getnames()
    assert any(n.endswith("/LICENSE") for n in names), f"LICENSE missing from sdist: {names}"
    assert any(n.endswith("/NOTICE") for n in names), f"NOTICE missing from sdist: {names}"
    assert any(n.endswith("/README.generator.md") for n in names), names

    # In the wheel they belong in .dist-info/licenses/ -- hatchling's default
    # license-files globs (LICEN[CS]E*, NOTICE*, ...) put them there, which is
    # where packaging tooling looks. No force-include needed; adding one only
    # duplicates them into a second, non-standard path.
    with zipfile.ZipFile(wheel) as zf:
        names = zf.namelist()
    assert any(n.endswith(".dist-info/licenses/LICENSE") for n in names), (
        f"LICENSE missing from wheel dist-info/licenses/: {names}"
    )
    assert any(n.endswith(".dist-info/licenses/NOTICE") for n in names), (
        f"NOTICE missing from wheel dist-info/licenses/: {names}"
    )
