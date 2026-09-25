def test_generated_node_header_is_namespaced(generated_tree):
    out = generated_tree
    appearance = (out / "x3d" / "nodes" / "Appearance.hpp").read_text()
    assert "#pragma once" in appearance
    assert '#include "x3d/core/X3Dtypes.hpp"' in appearance
    assert "namespace x3d::nodes {" in appearance
    assert "using namespace x3d::core;" in appearance
    # core header landed in the core subdir:
    assert (out / "x3d" / "core" / "X3Dtypes.hpp").exists()


def test_factory_and_registry_namespaced(generated_tree):
    out = generated_tree
    fac = (out / "x3d" / "nodes" / "X3DNodeFactory.hpp").read_text()
    assert "#pragma once" in fac
    assert "namespace x3d::nodes {" in fac
    assert "class X3DNode;" in fac  # forward-decl now lives inside x3d::nodes
