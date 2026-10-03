"""Generate the experimental SAI bindings and semantic catalog from the UOM.

This generator is self-contained. The X3D Unified Object Model is published by
the Web3D Consortium; a copy is vendored under data/ and pinned by SHA-256, so
this project consumes the specification directly rather than depending on
x3d-cpp's generator.
"""

import argparse
import hashlib
import os
from pathlib import Path
import tempfile
from importlib.resources import files

from x3d_sai_gen.fieldtypes import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_sai_gen.format import format_files
from x3d_sai_gen.parser import (
    parse_x3d_model, build_dependency_graph, parse_enum_definitions,
)
from x3d_sai_gen.emit.reflection import gen_reflection_header
from x3d_sai_gen.emit.sai_bindings import (
    gen_sai_binding_index, gen_sai_bindings_catalog, gen_sai_node_binding,
)
from x3d_sai_gen.emit.semantic_metadata import (
    gen_semantic_metadata_header, gen_semantic_metadata_source,
)

SPEC_VERSION = "4.0"


def _data(name: str):
    return files("x3d_sai_gen.data") / name


def verify_uom() -> str:
    """Return the vendored UOM's SHA-256, raising if it drifted from the pin."""
    actual = hashlib.sha256(
        _data(f"X3dUnifiedObjectModel-{SPEC_VERSION}.xml").read_bytes()
    ).hexdigest()
    expected = _data(f"UOM-{SPEC_VERSION}.sha256").read_text().strip()
    if actual != expected:
        raise SystemExit(
            f"vendored UOM hash {actual} does not match pin {expected}")
    return actual


def _write(path: str, text: str) -> str:
    with open(path, "w") as handle:
        handle.write(text)
    return path


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default="generated_cpp_bindings")
    parser.add_argument("--clang-format", default="clang-format")
    parser.add_argument("--check-uom-only", action="store_true")
    parser.add_argument("--check", action="store_true",
                        help="Compare generated files without modifying the output directory.")
    args = parser.parse_args(argv)

    verify_uom()
    if args.check_uom_only:
        return 0

    if args.check:
        with tempfile.TemporaryDirectory(prefix="x3d-sai-gen-") as temporary:
            _generate(temporary, args.clang_format)
            return check_generated(Path(temporary), Path(args.out_dir))
    _generate(args.out_dir, args.clang_format)
    return 0


def check_generated(expected: Path, actual: Path) -> int:
    """Check both content and inventory; no git or staging-state dependency."""
    expected_files = {path.relative_to(expected) for path in expected.rglob("*")
                      if path.is_file()}
    actual_files = {path.relative_to(actual) for path in actual.rglob("*")
                    if path.is_file()}
    changed = sorted(expected_files ^ actual_files)
    changed += sorted(path for path in expected_files & actual_files
                      if (expected / path).read_bytes() != (actual / path).read_bytes())
    if changed:
        for path in changed:
            print(f"generated file differs: {path}")
        return 1
    print(f"Generated files match: {len(expected_files)} files")
    return 0


def _generate(out_dir: str, clang_format: str) -> None:
    uom = str(_data(f"X3dUnifiedObjectModel-{SPEC_VERSION}.xml"))
    nodes, skipped = parse_x3d_model(uom, FIELD_TYPE_MAPPING, XS_TYPES)
    if not nodes or skipped:
        raise SystemExit(f"UOM cannot produce a complete catalog: {len(skipped)} unsupported fields")
    graph = build_dependency_graph(nodes)
    enum_defs = parse_enum_definitions(uom)

    # Only the SAI bindings and catalogs are clang-formatted. The reflection
    # header and the semantic registry carry their emitter's own layout, and
    # reformatting them would diverge from the committed artifact.
    written = []
    to_format = []

    core_dir = os.path.join(out_dir, "x3d", "core")
    os.makedirs(core_dir, exist_ok=True)
    written.append(_write(
        os.path.join(core_dir, "X3DReflection.hpp"), gen_reflection_header()))

    nodes_dir = os.path.join(out_dir, "x3d", "nodes")
    os.makedirs(nodes_dir, exist_ok=True)
    written.append(_write(
        os.path.join(nodes_dir, "X3DSemanticMetadataRegistry.hpp"),
        gen_semantic_metadata_header()))
    metadata = gen_semantic_metadata_source(
        nodes, graph, enum_defs, SPEC_VERSION)
    written.append(_write(
        os.path.join(nodes_dir, "X3DSemanticMetadataRegistry.cpp"), metadata))

    root = os.path.join(out_dir, "x3d", "sai", "experimental")
    bindings_dir = os.path.join(root, "bindings")
    os.makedirs(bindings_dir, exist_ok=True)
    for name in sorted(nodes):
        to_format.append(_write(
            os.path.join(bindings_dir, f"{name}.hpp"),
            gen_sai_node_binding(nodes[name], nodes, graph, enum_defs)))

    # The fingerprint is read back out of the emitted catalog source so the
    # bindings and the registry cannot disagree about the model they describe.
    marker = "X3DSemanticMetadataRegistry::modelFingerprint()"
    fingerprint = metadata[metadata.index(marker):].split(
        'return "', 1)[1].split('"', 1)[0]
    to_format.append(_write(
        os.path.join(root, "X3DSAIBindings.hpp"),
        gen_sai_bindings_catalog(SPEC_VERSION, fingerprint)))
    to_format.append(_write(
        os.path.join(root, "X3DSAIBindingCatalog.hpp"),
        gen_sai_binding_index(nodes)))

    format_files(to_format, clang_format)
    print(f"Generated {len(written) + len(to_format)} files into "
          f"{out_dir}")


if __name__ == "__main__":
    raise SystemExit(main())
