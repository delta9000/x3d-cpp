import subprocess, sys, pathlib


def test_generated_node_header_is_namespaced(tmp_path):
    out = tmp_path / "gen"
    subprocess.run([sys.executable, "-m", "x3d_cpp_gen.cli",
                    "--out", str(out)], check=True)
    appearance = (out / "x3d" / "nodes" / "Appearance.hpp").read_text()
    assert "#pragma once" in appearance
    assert '#include "x3d/core/X3Dtypes.hpp"' in appearance
    assert "namespace x3d::nodes {" in appearance
    assert "using namespace x3d::core;" in appearance
    # core header landed in the core subdir:
    assert (out / "x3d" / "core" / "X3Dtypes.hpp").exists()
