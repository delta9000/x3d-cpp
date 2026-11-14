import importlib.util
from pathlib import Path

import pytest


@pytest.mark.parametrize("has_citation", [True, False])
def test_suggest_reports_incomplete_check_when_embedding_service_is_unavailable(
        monkeypatch, capsys, has_citation):
    scripts = Path(__file__).parent.parent / "scripts"
    monkeypatch.syspath_prepend(str(scripts))
    spec = importlib.util.spec_from_file_location("docs_drift", scripts / "docs_drift.py")
    drift = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(drift)

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
