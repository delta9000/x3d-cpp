"""Tests for the C1 declaration/definition split."""
from x3d_cpp_gen.emit.factory import (
    gen_node_factory_header, gen_node_factory_source,
)
from x3d_cpp_gen.parser import X3DNode


def _two_nodes():
    box = X3DNode(name="Box", fields=[]); box.is_abstract = False
    abs = X3DNode(name="X3DGeometryNode", fields=[]); abs.is_abstract = True
    return {"Box": box, "X3DGeometryNode": abs}


def test_factory_header_has_no_node_includes():
    hdr = gen_node_factory_header(_two_nodes())
    assert '#include "Box.hpp"' not in hdr
    assert "X3DNodeFactory" in hdr            # class still declared
    assert "create(" in hdr                   # entry point still declared
    assert "{ return std::make_shared" not in hdr   # registry body NOT inline


def test_factory_source_includes_concrete_nodes_and_defines_registry():
    src = gen_node_factory_source(_two_nodes())
    assert '#include "X3DNodeFactory.hpp"' in src
    assert '#include "Box.hpp"' in src                       # concrete included
    assert '#include "X3DGeometryNode.hpp"' not in src       # abstract excluded
    assert "X3DNodeFactory::registry()" in src               # out-of-line def
    assert "std::make_shared<Box>()" in src


import subprocess, sys
from pathlib import Path


def test_node_header_declares_fields_without_inline_body(tmp_path):
    out = tmp_path / "gen"
    out.mkdir()
    subprocess.run([sys.executable, "-m", "x3d_cpp_gen.cli",
                    "--out", str(out), "--no-test"], check=True)
    box_h = (out / "Box.hpp").read_text()
    box_c = (out / "Box.cpp").read_text()
    # Header DECLARES fields() but does NOT inline its FieldTable body.
    # (clang-format's default PointerAlignment: Right renders "FieldTable &".)
    assert "const FieldTable &fields() const override;" in box_h
    assert "static const FieldTable table" not in box_h
    # The out-of-line definition lives in the .cpp, class-qualified.
    assert "Box::fields()" in box_c
    assert "FieldTable" in box_c
    # Trivial accessors stay inline in the header.
    assert "getSize()" in box_h or "getDef" in box_h
