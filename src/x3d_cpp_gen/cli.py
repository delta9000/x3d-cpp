"""Command-line entry point for x3d-cpp-gen.

Runs correctly from ANY working directory: all bundled resources (the X3D UOM
XML and the Jinja templates) are resolved relative to the installed package via
importlib.resources / __file__, never relative to the current working directory.
"""

import argparse
import os
import subprocess
import sys
from importlib.resources import files
from pathlib import Path

from x3d_cpp_gen.parser import (
    parse_x3d_model, build_dependency_graph, parse_enum_definitions,
)
from x3d_cpp_gen.generator import (
    FIELD_TYPE_MAPPING, XS_TYPES, TEMPLATES_DIR,
    write_types_header, write_enums_header,
    write_reflection_header, write_node_factory,
    generate_cpp_bindings, generate_test_file,
    write_interface_registry, write_semantic_metadata_registry,
    write_sai_bindings,
)
from x3d_cpp_gen.model.version import SpecVersion

# Packaged default spec, resolved relative to the installed package (never CWD).
DEFAULT_SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")


def compile_and_run_test(test_file_path: str, output_dir: str,
                         compiler: str = "g++") -> bool:
    """Compile and run the generated smoke test. Fail closed.

    Returns True ONLY when the test actually compiled and ran green. Every path
    where verification did not happen returns False: reporting success when the
    thing under test was never built is exactly the failure this function used
    to have. The caller skips deliberately with --no-test; there is no
    succeed-without-testing path in here.
    """
    if not compiler:
        print("ERROR: no compiler configured, so the smoke test cannot run. "
              "Pass --no-test to skip deliberately.", file=sys.stderr)
        return False

    output_binary = os.path.join(output_dir, "test_exec")
    # Node sources + the smoke-test main now live under x3d/nodes/; the include
    # base stays output_dir so "x3d/core/..." and "x3d/nodes/..." resolve. Each
    # concrete node's vtable is emitted in the TU that defines its key function
    # (e.g. WorldInfo::nodeTypeName() in WorldInfo.cpp), so the smoke test must
    # LINK every generated .cpp -- linking test.cpp alone leaves every vtable/VTT
    # undefined. test.cpp is itself in x3d/nodes/, so the glob already includes
    # it (don't list it twice or main is multiply defined).
    nodes_dir = os.path.join(output_dir, "x3d", "nodes")
    test_main = os.path.join(nodes_dir, "test.cpp")
    if not os.path.exists(test_main):
        # Not environmental: generate_test_file() ran just before this, so the
        # main should exist. Its absence means the generator dropped it -- a
        # codegen regression, and never a reason to report success.
        print(f"ERROR: generated smoke-test main not found: {test_main}\n"
              "The generator was asked to produce it and did not; this is a "
              "codegen regression, not a missing tool.", file=sys.stderr)
        return False
    sources = sorted(str(p) for p in Path(nodes_dir).glob("*.cpp"))
    compile_command = [compiler, "-std=c++20", "-I", output_dir,
                       *sources, "-o", output_binary]

    try:
        compile_result = subprocess.run(compile_command, capture_output=True, text=True)
    except FileNotFoundError:
        print(f"ERROR: compiler '{compiler}' not found, so the smoke test could "
              f"not run. Pass --no-test to skip deliberately.", file=sys.stderr)
        return False

    if compile_result.returncode != 0:
        print("Compilation failed.", file=sys.stderr)
        if compile_result.stdout:
            print(compile_result.stdout)
        if compile_result.stderr:
            print(compile_result.stderr, file=sys.stderr)
        return False

    print("Compilation successful. Running test...")
    run_result = subprocess.run([output_binary], capture_output=True, text=True)
    if run_result.stdout:
        print(run_result.stdout)
    if run_result.returncode != 0:
        print("Test execution failed.", file=sys.stderr)
        if run_result.stderr:
            print(run_result.stderr, file=sys.stderr)
        return False
    return True


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="x3d-cpp-gen",
        description="Generate C++ bindings from the X3D Unified Object Model.",
    )
    parser.add_argument(
        "-s", "--spec", default=str(DEFAULT_SPEC),
        help="Path to the X3D UOM XML spec (default: packaged 4.0 model).",
    )
    parser.add_argument(
        "--spec-version", default=None,
        help="Override the X3D spec version (e.g. 4.1). By default the version "
             "is auto-detected from the UOM XML's <X3dUnifiedObjectModel> "
             "version attribute.",
    )
    parser.add_argument(
        "-o", "--out", default="generated_cpp_bindings",
        help="Output directory for generated headers (default: ./generated_cpp_bindings).",
    )
    parser.add_argument(
        "--templates", default=str(TEMPLATES_DIR),
        help="Jinja templates directory (default: packaged templates).",
    )
    parser.add_argument(
        "--clang-format", default=os.environ.get("CLANG_FORMAT", "clang-format"),
        help="clang-format executable used to format output "
             "(env CLANG_FORMAT; empty string to disable).",
    )
    parser.add_argument(
        "--compiler", default=os.environ.get("CXX", "g++"),
        help="C++ compiler for the smoke test (env CXX). Use --no-test to skip "
             "the smoke test; an empty value is an error, not a skip.",
    )
    parser.add_argument(
        "--no-test", action="store_true",
        help="Skip generating and compiling the smoke test.",
    )
    parser.add_argument(
        "--allow-unsupported-fields", action="store_true",
        help="Do not fail when the UOM spec contains a field whose type "
             "this generator doesn't support (default: fail closed).",
    )
    return parser


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)

    spec = Path(args.spec)
    out_dir = Path(args.out).resolve()
    templates_dir = Path(args.templates)

    if not spec.exists():
        print(f"ERROR: spec file not found: {spec}", file=sys.stderr)
        return 2
    if not templates_dir.exists():
        print(f"ERROR: templates dir not found: {templates_dir}", file=sys.stderr)
        return 2

    # Resolve the spec version: explicit override wins, else auto-detect from
    # the UOM XML root's version attribute. Nothing is hardcoded to 4.0.
    if args.spec_version:
        spec_version = SpecVersion(version=args.spec_version, spec_path=spec)
        print(f"Using X3D spec version {spec_version.version} (override).")
    else:
        try:
            spec_version = SpecVersion.detect(spec)
        except ValueError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            return 2
        print(f"Auto-detected X3D spec version {spec_version.version} "
              f"(from <X3dUnifiedObjectModel version=...> in {spec.name}).")

    # Generated node classes always live in x3d::nodes: the vocabulary
    # value/reflection types are x3d::core and the node classes are x3d::nodes.
    # The layout is fixed so the in-tree and installed include spellings
    # ("x3d/core/...", "x3d/nodes/...") are uniform. generate_cpp_bindings()
    # still takes a `namespace` parameter -- tests/test_version.py exercises it
    # directly -- but the CLI does not expose it, because a CLI option the tool
    # then overrides is a lie to the caller.
    namespace = "x3d::nodes"

    out_dir.mkdir(parents=True, exist_ok=True)
    core_dir = out_dir / "x3d" / "core"
    nodes_dir = out_dir / "x3d" / "nodes"
    core_dir.mkdir(parents=True, exist_ok=True)
    nodes_dir.mkdir(parents=True, exist_ok=True)

    nodes, skipped_fields = parse_x3d_model(str(spec), FIELD_TYPE_MAPPING, XS_TYPES)
    if skipped_fields and not args.allow_unsupported_fields:
        print(f"ERROR: {len(skipped_fields)} field(s) were skipped due to "
              f"unsupported types (this shrinks the generated API silently "
              f"unless you pass --allow-unsupported-fields):", file=sys.stderr)
        for node_name, field_name, raw_type in skipped_fields:
            print(f"  {node_name}.{field_name}: type={raw_type!r}", file=sys.stderr)
        return 1
    if not nodes:
        print(f"ERROR: no nodes parsed from {spec}", file=sys.stderr)
        return 1
    dependency_graph = build_dependency_graph(nodes)
    enum_defs = parse_enum_definitions(str(spec))

    write_types_header(str(core_dir))
    write_enums_header(str(core_dir), enum_defs)
    write_reflection_header(str(core_dir))
    generate_cpp_bindings(nodes, dependency_graph, str(out_dir),
                          templates_dir=templates_dir,
                          clang_format=args.clang_format,
                          enum_defs=enum_defs, namespace=namespace)
    write_node_factory(str(out_dir), nodes)
    write_interface_registry(str(out_dir), nodes, dependency_graph)
    write_semantic_metadata_registry(str(out_dir), nodes, dependency_graph,
                                     enum_defs, spec_version.version)
    write_sai_bindings(str(out_dir), nodes, dependency_graph, enum_defs,
                       spec_version.version, args.clang_format)

    if not args.no_test:
        test_file_path = generate_test_file(nodes, str(out_dir),
                                            templates_dir=templates_dir,
                                            enum_defs=enum_defs)
        ok = compile_and_run_test(test_file_path, str(out_dir),
                                  compiler=args.compiler)
        if not ok:
            return 1

    print(f"Done. Output written to {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
