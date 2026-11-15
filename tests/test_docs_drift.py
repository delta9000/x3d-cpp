import importlib.util
from pathlib import Path

import pytest


@pytest.fixture
def drift(monkeypatch):
    scripts = Path(__file__).parent.parent / "scripts"
    monkeypatch.syspath_prepend(str(scripts))
    spec = importlib.util.spec_from_file_location("docs_drift", scripts / "docs_drift.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.mark.parametrize("has_citation", [True, False])
def test_suggest_reports_incomplete_check_when_embedding_service_is_unavailable(
        drift, monkeypatch, capsys, has_citation):
    paths = ["runtime/Widget.cpp", "runtime/Other.cpp"]
    monkeypatch.setattr(drift, "resolve_rev", lambda rev: "working")
    monkeypatch.setattr(drift, "changed_paths", lambda rev: (paths, [], set()))
    monkeypatch.setattr(drift, "changed_new_lines", lambda *args: set())
    monkeypatch.setattr(drift, "changed_symbols", lambda *args: [])
    docs = {"docs/wiki/widgets.md": "See runtime/Widget.cpp."} if has_citation else {}
    monkeypatch.setattr(drift, "_load_docs", lambda: docs)

    calls = []

    def unavailable(*args):
        calls.append(args)
        raise RuntimeError("curl http://localhost:8080/v1/embeddings: connection refused")

    monkeypatch.setattr(drift, "_curl", unavailable)
    drift.suggest("working")

    output = capsys.readouterr().out
    assert "semantic check unavailable" in output
    assert "connection refused" in output
    assert "results are incomplete" in output
    assert "no literal citations; semantic check unavailable" in output
    assert "no doc references this change" not in output
    assert "advisory only (exit 0)" in output
    assert len(calls) == 1  # Do not retry an unavailable service for every file.
    if has_citation:
        assert "CITES  docs/wiki/widgets.md" in output


@pytest.mark.parametrize(("changed_path", "expected_page"), [
    ("sai/include/x3d/sai/experimental/value_constraints.hpp",
     "docs/wiki/sai/reference/field-values.md"),
    ("sai/src/metadata.cpp", "docs/wiki/sai/reference/generated-fields.md"),
    ("sai/examples/provider_scene.cpp", "docs/wiki/sai/tutorials/first-scene.md"),
    ("adapters/sai_provider/src/sai_provider.cpp",
     "docs/wiki/sai/reference/native-provider.md"),
    ("adapters/sai_provider/include/x3d/sai_provider.hpp",
     "docs/wiki/sai/reference/native-provider.md"),
])
def test_sai_changes_suggest_their_existing_documentation(
        drift, monkeypatch, capsys, changed_path, expected_page):
    ignored_paths = [
        "sai/third_party/tl/expected.hpp",
        "sai/generated_cpp_bindings/public/x3d/sai/experimental/X3DSAIBindings.hpp",
        "generated_cpp_bindings/X3DNode.hpp",
        "adapters/sai_provider/vendor/example.hpp",
    ]

    def changed_files(*args):
        if args == ("diff", "--name-only", "HEAD"):
            return "\n".join([changed_path, *ignored_paths])
        assert args == ("ls-files", "--others", "--exclude-standard")
        return ""

    monkeypatch.setattr(drift, "_git", changed_files)
    monkeypatch.setattr(drift, "changed_new_lines", lambda *args: set())
    monkeypatch.setattr(drift, "changed_symbols", lambda *args: [])
    monkeypatch.setattr(drift, "_semantic", lambda *args: [])

    drift.suggest("working")

    output = capsys.readouterr().out
    assert changed_path in output
    assert f"CITES  {expected_page}" in output
    assert "nothing to check" not in output
    for path in ignored_paths:
        assert path not in output
