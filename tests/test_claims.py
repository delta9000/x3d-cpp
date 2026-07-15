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
