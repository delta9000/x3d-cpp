# Compile Topology Follow-up Design

**Date:** 2026-07-14
**Status:** Approved for autonomous implementation
**Scope:** Compile-contract dependencies, doctest entry point reuse, generated
node-factory compilation, and the boundary of the future compiled runtime

## Context

PR #54 made the generated node implementation a shared library and split the
behavior and public-header CI jobs. The split exposed an accidental dependency:
both `x3d_cpp_all_headers` and `x3d_header_isolation` consume the complete
`x3d_cpp::x3d_cpp` facade. That facade links `x3d_cpp_nodes`, so all 407
header-isolation translation units wait for the node shared library even though
they only need public include paths and C++20 usage requirements.

The same measurements identified two smaller sources of repeated work. Eight
doctest executables compile the identical `doctest_main.cpp`, and the generated
factory source includes all 262 concrete node headers in one translation unit.
The factory is therefore the longest generated-node compile and is invalidated
by any concrete node-header change.

The remaining large cost is architectural: parsing, codecs, events, scene
behavior, and extraction are still implementation-heavy headers. Moving those
definitions is valuable, but it changes binary boundaries and should not be
mixed into the graph and generator cleanup described here.

## Alternatives considered

1. **CMake-only correction.** Remove the contract dependency and move the
   install smoke. This is very low risk but leaves known duplicate and critical
   path work in place.
2. **Focused compile-topology follow-up.** Correct the contract graph,
   deduplicate doctest main, and distribute node creation into the existing
   per-node sources. This is the selected approach because every change is
   independently testable and measurable while remaining within build/codegen
   structure.
3. **Immediate runtime extraction.** Add the shared hand-written runtime and
   migrate large header bodies in the same branch. This offers the largest
   theoretical reduction but combines ABI, ownership, link, generator, and CI
   changes, making regressions and performance attribution unnecessarily hard.

## Decision

### Compile contracts are compile-only

`x3d_cpp_all_headers` and `x3d_header_isolation` will link privately to
`x3d_cpp::headers`, not `x3d_cpp::x3d_cpp`. They retain the exact include paths,
compile features, warnings, one-translation-unit-per-header structure, and CTest
coverage without a build or link edge to `x3d_cpp_nodes`.

The all-headers executable has an inert `main()` and needs no production
implementation symbols, so it remains an executable but links no project
runtime library. The installed-consumer smoke genuinely installs, links, and
runs the compiled package; it moves from the `compile-contract` CTest label to
`behavior`. The behavior aggregate already builds SDK consumers and therefore
the node shared library before that smoke runs.

Graph-shape tests will assert both positive and negative properties: the two
contract targets consume the headers layer, their Ninja inputs exclude the node
library, and the install smoke belongs only to behavior.

### Compile doctest main once per configuration

An internal static library named `x3d_doctest_main` will contain only
`runtime/test_support/doctest_main.cpp`. Every doctest-based executable will
link it instead of compiling the source directly. A static library is preferred
over an object library because it behaves as a normal reusable entry-point
provider and preserves per-target link composition. Global sanitizer compile
options still instrument it once in sanitizer configurations.

The target is created only when top-level tests are enabled and is classified
as internal. Tests will inspect generated compile commands and target linkage to
prove that the source is compiled exactly once and that all doctest suites
consume the shared test-main target.

### Distribute node creation into per-node sources

For each concrete node, the existing generated `<Node>.cpp` will define one
internal creator in `x3d::nodes::factory_detail`:

```cpp
std::shared_ptr<X3DNode> createBox() {
    return std::make_shared<Box>();
}
```

Abstract nodes emit no creator. `X3DNodeFactory.cpp` will include only its own
public header, forward-declare the concrete creator functions, and initialize
the registry with their function pointers. It will not include concrete node
headers or instantiate `make_shared` itself.

This keeps the public factory API unchanged, avoids static-registration and
initialization-order hazards, works for shared and static node-library modes,
and makes a node-header change rebuild only its normal unity batch rather than
the entire factory registry translation unit. Creator declarations stay in the
factory source so no private registry surface is installed as a public header.

Generator unit tests will pin concrete/abstract emission and the include-free
factory shape. The committed generated tree will be regenerated and the golden
drift checks will continue to enforce exact agreement.

## Compatibility

The installed CMake target names and public C++ APIs do not change. Consumers
continue linking `x3d_cpp::x3d_cpp`, `x3d_cpp::sdk`, or
`x3d_cpp::authoring`. `x3d_cpp::headers` remains the lower-level exported usage
requirements target introduced by PR #54; this change only uses it correctly
inside compile-only contracts.

Both default shared-node and explicit static-node configurations must build and
pass the installed-consumer smoke. No static initialization registry is added,
so archive dead-stripping cannot silently remove node registrations.

## Validation and performance acceptance

Correctness requires:

- focused Python graph/generator tests and the complete pytest suite;
- normal behavior and compile-contract aggregates;
- installed-consumer smoke through the behavior label;
- shared and static node-library configurations;
- CPU-raster/example consumer validation because it exercises the public SDK
  and factory through an out-of-tree-style consumer;
- sanitizer behavior tests if the generated factory change affects runtime
  construction.

Performance measurements use clean build directories with `CCACHE_DISABLE=1`,
the same compiler, Ninja generator, job count, and configuration before and
after. The branch records:

- absence of `x3d_cpp_nodes` from the header-contract Ninja dependency chain;
- node-runtime work removed from the header CI target;
- one `doctest_main.cpp` compile instead of eight;
- `X3DNodeFactory.cpp` compile time, preprocessing size, and object size before
  and after;
- complete contract and behavior target wall/user time where practical.

## Future phase: compiled hand-written runtime

The next architectural PR will introduce a shared `x3d_cpp_runtime` containing
measured, non-template implementations from parsing, codecs, events, scene
behavior, and extraction. It will preserve declarations and public types in
their current headers and keep templates, `constexpr` code, small accessors, and
generic callback seams inline.

Migration will proceed one subsystem at a time. Each wave must add link-level
coverage, pass behavior/header/install/example/sanitizer gates, and demonstrate
at least a 10% cumulative compiler-time improvement in the affected target group
or a similarly clear peak-memory reduction. The authoring footprint remains a
separate boundary; full runtime code must not leak into the slim authoring
surface. This is deliberately a separate PR so ABI ownership and performance
can be reviewed on their own evidence.
