"""Public claims must survive contact with the code.

Two claims were literally false:

  "The SDK does no file IO"  -- the facade exports parseFile(), which opens an
  ifstream (include/x3d/sdk.hpp -> runtime/parse/X3DParse.cpp). sdk.hpp asserted
  it ~50 lines above the export.

  "every node and field is spec-correct by construction" -- generation from the
  UOM constrains declarations, types and defaults. It does not prove runtime
  semantics or eliminate UOM errata.
"""

from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

STRONG_IO_CLAIM_SITES = [
    "README.md",
    "include/x3d/sdk.hpp",
    "docs/wiki/architecture.md",
]


def test_no_unqualified_no_file_io_claim():
    """parseFile() opens a file; an unqualified 'no file IO' is false."""
    for rel in STRONG_IO_CLAIM_SITES:
        text = (REPO_ROOT / rel).read_text()
        for phrase in ("does **no** file IO", "does no file IO", "does NO file IO"):
            assert phrase not in text, (
                f"{rel} claims {phrase!r}, but the SDK exports parseFile() "
                f"(include/x3d/sdk.hpp -> runtime/parse/X3DParse.cpp opens an ifstream)"
            )


def test_parse_file_really_does_open_a_file():
    """Guard the guard: if this stops being true, the claim tests are moot."""
    assert "using x3d::codec::parseFile;" in (REPO_ROOT / "include/x3d/sdk.hpp").read_text()
    assert "std::ifstream" in (REPO_ROOT / "runtime/parse/X3DParse.cpp").read_text()


def test_no_spec_correct_by_construction_claim():
    """Generation from the UOM constrains declarations, not runtime semantics."""
    text = (REPO_ROOT / "README.md").read_text()
    assert "spec-correct by construction" not in text, (
        "generation from the UOM does not prove runtime semantics or eliminate "
        "UOM errata; behavioral conformance is tested separately"
    )


def test_readme_does_not_call_ci_manual_only():
    """ci.yml has a pull_request trigger; the README told readers to enable it."""
    text = (REPO_ROOT / "README.md").read_text()
    assert "re-enable the `push:` / `pull_request:` triggers" not in text
    workflow = (REPO_ROOT / ".github/workflows/ci.yml").read_text()
    assert "pull_request:" in workflow, (
        "the README now claims CI runs on every PR -- keep it true"
    )


def test_sdk_guide_does_not_claim_all_seams_experimental():
    """docs/wiki/seam-status.md declares six seams STABLE."""
    text = (REPO_ROOT / "docs/sdk/README.md").read_text()
    assert "All seams are **experimental**" not in text
    matrix = (REPO_ROOT / "docs/wiki/seam-status.md").read_text()
    assert "**STABLE**" in matrix, "the matrix no longer declares any seam STABLE"


def test_notice_and_readme_agree_on_corpus_gates():
    notice = (REPO_ROOT / "NOTICE").read_text()
    assert "those tests skip cleanly" not in notice, (
        "the corpus differential gates fail closed; only RAG/JDK skip cleanly"
    )
    assert "fail closed" in notice


def test_wiki_does_not_deny_renderers_are_in_the_repo():
    text = (REPO_ROOT / "docs/wiki/index.md").read_text()
    assert "not part of this repo" not in text, (
        "examples/cpu_raster and examples/poc_renderer are in the repo; "
        "they are out-of-SDK consumers, which is a different claim"
    )
    assert (REPO_ROOT / "examples/cpu_raster").is_dir()
    assert (REPO_ROOT / "examples/poc_renderer").is_dir()


def test_capability_matrix_mentions_the_moviedecoder_seam():
    """The matrix undersold shipped work: it omitted the seam entirely.

    Movie DECODE ships (two backends + x3d_movie_tests). Movie PLAYBACK is
    correctly still deferred -- the SDK never invokes the callback. Both facts
    belong in the matrix; previously neither did.
    """
    text = (REPO_ROOT / "docs/sdk/v1-capabilities.md").read_text()
    assert "MovieDecoder" in text, (
        "the MovieDecoder seam ships with two backends and a CI gate; the "
        "capability matrix must not omit it"
    )
    assert "MovieTexture frames" in text, (
        "MovieTexture frame playback is genuinely deferred and must stay listed"
    )
