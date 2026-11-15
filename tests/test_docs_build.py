"""Exercise documentation safeguards with the production MkDocs configuration.

Run with `uv run --group docs pytest tests/test_docs_build.py`.
"""
import re
import subprocess
import sys
from pathlib import Path

import pytest
import yaml


pytest.importorskip("mkdocs", reason="requires the docs dependency group")
pytest.importorskip("material", reason="requires the docs dependency group")

REPO = Path(__file__).resolve().parent.parent


def build_docs(tmp_path, pages):
    docs = tmp_path / "docs"
    docs.mkdir()
    for name, content in pages.items():
        (docs / name).write_text(content, encoding="utf-8")
    config = tmp_path / "mkdocs.yml"
    config.write_text(yaml.safe_dump({
        "INHERIT": str(REPO / "mkdocs.yml"),
        "docs_dir": str(docs),
        "site_dir": str(tmp_path / "site"),
        "nav": [{"Home": "index.md"}],
    }), encoding="utf-8")
    return subprocess.run(
        [sys.executable, "-m", "mkdocs", "build", "--strict", "-f", str(config)],
        capture_output=True, text=True, timeout=30,
    )


@pytest.mark.parametrize("pages,diagnostic", [
    ({"index.md": "# Home\n[Missing heading](#missing-heading)\n"},
     "no such anchor"),
    ({"index.md": "# Home\n", "orphan.md": "# Orphan\n"}, "orphan.md"),
])
def test_strict_docs_build_rejects_missing_anchors_and_orphans(tmp_path, pages, diagnostic):
    result = build_docs(tmp_path, pages)
    output = result.stdout + result.stderr
    assert result.returncode != 0, output
    assert "WARNING" in output and diagnostic in output, output


def test_docs_build_emits_a_mermaid_diagram(tmp_path):
    result = build_docs(tmp_path, {
        "index.md": "# Home\n\n```mermaid\nflowchart TB\n  A --> B\n```\n",
    })
    assert result.returncode == 0, result.stdout + result.stderr
    html = (tmp_path / "site" / "index.html").read_text(encoding="utf-8")
    assert re.search(r'<pre\b[^>]*class="mermaid"', html)
    assert "flowchart TB" in html
