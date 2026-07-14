#!/usr/bin/env python3
"""Classify changed paths into CI execution groups."""

from __future__ import annotations

import argparse
import re
import sys
from collections.abc import Iterable

CLASSES = ("audio", "quickjs", "assets", "fontmetrics", "cpp", "generator", "docs")

BUILD_SENSITIVE_PATTERNS = (
    r"^CMakeLists\.txt$",
    r"^CMakePresets\.json$",
    r"^cmake/",
    r"^mise\.toml$",
    r"^vcpkg\.json$",
    r"^\.github/workflows/ci\.yml$",
)

CPP_PATTERNS = (
    r"^generated_cpp_bindings/",
    r"^include/",
    r"^runtime/",
    r"^examples/",
    r"^third_party/",
    r"^tools/",
    r"^tests/cmake/",
    r"^tests/.*\.(?:c|cc|cpp|cxx|h|hh|hpp)$",
    r"^scripts/(?:authoring-footprint|validate-examples|verify_install_embed)\.sh$",
)

GENERATOR_PATTERNS = (
    r"^src/x3d_cpp_gen/",
    r"^tests/fixtures/",
    r"^(?:pyproject\.toml|uv\.lock)$",
)

DOCS_PATTERNS = (r"^docs/", r"\.md$", r"^mkdocs\.yml$")

TOOLING_PATTERNS = (
    r"^scripts/.*\.(?:py|sh)$",
    r"^tests/.*\.py$",
    r"^src/.*\.py$",
    r"^\.git(?:ignore|attributes)$",
    r"^(?:LICENSE|NOTICE)$",
)

SEAM_PATTERNS = {
    "audio": (r"^runtime/sound/",),
    "quickjs": (r"^runtime/script/",),
    "assets": (
        r"^runtime/io/(?:curl|s3)/",
        r"^runtime/io/tests/asset_resolver",
        r"^runtime/extract/AssetResolver\.(?:hpp|cpp)$",
        r"^runtime/parse/(?:X3DParse|X3DProtoResolver|JsonReader|ClassicVrmlReader|Vrml97Dialect)\.(?:hpp|cpp)$",
        r"^runtime/InlineExpand\.(?:hpp|cpp)$",
        r"^runtime/ext/(?:ExtResolver|ExternalGeometry)\.(?:hpp|cpp)$",
        r"^runtime/extract/TextureExtract\.(?:hpp|cpp)$",
    ),
    "fontmetrics": (
        r"^runtime/io/(?:stbtt|freetype)/",
        r"^runtime/io/tests/font_metrics",
        r"^runtime/extract/FontMetrics\.(?:hpp|cpp)$",
        r"^runtime/extract/(?:TextLayout|TextExtract|MeshBuilder|RenderItem)\.(?:hpp|cpp)$",
    ),
}


def _matches(path: str, patterns: Iterable[str]) -> bool:
    return any(re.search(pattern, path) for pattern in patterns)


def _enable_all(result: dict[str, bool]) -> None:
    result.update(dict.fromkeys(CLASSES, True))


def classify(paths: Iterable[str]) -> dict[str, bool]:
    """Return every CI class for newline-style repository-relative paths.

    Empty or unrecognized inputs deliberately enable every class. A new path
    must never silently skip a potentially relevant build until it is added to
    one of the explicit pattern tables above.
    """

    normalized = [path.strip().removeprefix("./") for path in paths if path.strip()]
    result = dict.fromkeys(CLASSES, False)
    if not normalized:
        _enable_all(result)
        return result

    for path in normalized:
        recognized = False

        if _matches(path, BUILD_SENSITIVE_PATTERNS):
            recognized = True
            result["cpp"] = True
            for seam in SEAM_PATTERNS:
                result[seam] = True

        if _matches(path, CPP_PATTERNS):
            recognized = True
            result["cpp"] = True

        if _matches(path, GENERATOR_PATTERNS):
            recognized = True
            result["generator"] = True

        if _matches(path, DOCS_PATTERNS):
            recognized = True
            result["docs"] = True

        if _matches(path, TOOLING_PATTERNS):
            recognized = True

        for seam, patterns in SEAM_PATTERNS.items():
            if _matches(path, patterns):
                recognized = True
                result[seam] = True

        if not recognized:
            _enable_all(result)

    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Emit key=true|false CI classes for paths read from stdin."
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="enable every class (for manual workflow dispatch)",
    )
    args = parser.parse_args(argv)

    result = (
        dict.fromkeys(CLASSES, True)
        if args.all
        else classify(line for line in sys.stdin)
    )
    for name, enabled in result.items():
        print(f"{name}={'true' if enabled else 'false'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
