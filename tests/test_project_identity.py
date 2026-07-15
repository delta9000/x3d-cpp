"""x3d-cpp (the product) and x3d-cpp-gen (the Python generator) are distinct.

The repo was simultaneously x3d-cpp, x3d-cpp-gen, a "header-only binding"
project and a compiled runtime SDK. The settled model:

    product / repository        x3d-cpp
    Python package + command    x3d-cpp-gen
    CMake project + namespace   x3d_cpp

Surviving 'x3d-cpp-gen' occurrences must refer to the Python package, the
executable, or the generator subsystem -- never to the whole runtime.
"""

import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def test_readme_opens_with_the_product_name():
    first = (REPO_ROOT / "README.md").read_text().splitlines()[0].strip()
    assert first == "# x3d-cpp", f"README opens with {first!r}"


def test_cmake_description_is_not_header_only():
    """The package installs compiled shared libraries; it is not header-only."""
    text = (REPO_ROOT / "CMakeLists.txt").read_text()
    preamble = text.split("LANGUAGES")[0]
    assert "Header-only" not in preamble, (
        "CMake still describes the project as header-only, but it installs "
        "compiled shared libraries (x3d_cpp_runtime, x3d_cpp_nodes)"
    )


def test_mkdocs_points_at_the_real_repository():
    text = (REPO_ROOT / "mkdocs.yml").read_text()
    assert "https://github.com/delta9000/x3d-cpp" in text
    assert "sandbrookvt" not in text, "wrong repository owner"
    header = text.split("nav:")[0]
    assert "x3d-cpp-gen" not in header, (
        "the wiki is the product's knowledge home, not the generator's"
    )


def test_notice_uses_the_product_name():
    first = (REPO_ROOT / "NOTICE").read_text().splitlines()[0].strip()
    assert first == "x3d-cpp", f"NOTICE opens with {first!r}"


def test_readme_cmake_floor_matches_cmake_minimum_required():
    """The README's stated floor must match what CMake actually declares.

    These drifted once already: PROJECT_IS_TOP_LEVEL (3.21+) was in use while
    cmake_minimum_required said 3.20, where it silently evaluates empty and
    X3D_CPP_BUILD_TESTS / X3D_CPP_BUILD_EXAMPLES default OFF.
    """
    cmake = (REPO_ROOT / "CMakeLists.txt").read_text()
    m = re.search(r"cmake_minimum_required\(VERSION (\d+\.\d+)", cmake)
    assert m, "could not find cmake_minimum_required"
    declared = m.group(1)

    readme = (REPO_ROOT / "README.md").read_text()
    r = re.search(r"CMake (\d+\.\d+)\+", readme)
    assert r, "README does not state a CMake floor"
    assert r.group(1) == declared, (
        f"README says CMake {r.group(1)}+ but CMakeLists declares {declared}"
    )

    if "PROJECT_IS_TOP_LEVEL" in cmake:
        assert tuple(int(x) for x in declared.split(".")) >= (3, 21), (
            f"PROJECT_IS_TOP_LEVEL requires CMake 3.21 but the floor is {declared}; "
            f"below 3.21 it silently evaluates empty"
        )


def test_ext_urn_is_not_renamed_by_an_identity_sweep():
    """The ext-firewall URN is a WIRE FORMAT constant, not a product name.

    "urn:x3d-cpp-gen:ext:ExternalGeometry" appears inside real X3D scene files
    as an <ExternProtoDeclare url=...>. Renaming it to match the product would
    break every scene that uses the ext firewall, and the parser would stop
    intercepting it (ADR-0001). It is deliberately excluded from the
    x3d-cpp-gen -> x3d-cpp rename, and must stay excluded.
    """
    resolver = (REPO_ROOT / "runtime/ext/ExtResolver.hpp").read_text()
    assert '"urn:x3d-cpp-gen:ext:ExternalGeometry"' in resolver, (
        "the ext URN changed; it is a wire-format constant baked into scene "
        "files and must not be renamed by a product-identity sweep"
    )
