from __future__ import annotations

import pytest

from scripts.classify_ci_changes import CLASSES, classify


def enabled_classes(paths: list[str]) -> set[str]:
    return {name for name, enabled in classify(paths).items() if enabled}


@pytest.mark.parametrize(
    ("paths", "expected"),
    [
        (["runtime/parse/X3DParse.hpp"], {"cpp", "assets"}),
        (["generated_cpp_bindings/x3d/nodes/Box.hpp"], {"cpp"}),
        (["src/x3d_cpp_gen/generator.py"], {"generator"}),
        (
            [
                "src/x3d_cpp_gen/generator.py",
                "generated_cpp_bindings/x3d/nodes/Box.hpp",
            ],
            {"generator", "cpp"},
        ),
        (["scripts/code_rag.py"], set()),
        (["docs/wiki/index.md"], {"docs"}),
        (
            ["CMakeLists.txt"],
            {"cpp", "audio", "quickjs", "assets", "fontmetrics"},
        ),
        ([], set(CLASSES)),
        (["unexpected/new-area/file.xyz"], set(CLASSES)),
    ],
)
def test_change_classes(paths: list[str], expected: set[str]) -> None:
    assert enabled_classes(paths) == expected


def test_classifier_always_returns_every_output_key() -> None:
    result = classify(["docs/wiki/index.md"])
    assert tuple(result) == CLASSES
    assert all(isinstance(value, bool) for value in result.values())
