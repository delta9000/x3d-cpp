"""The golden invariant is byte-exact, so the formatter must be pinned.

Without a committed .clang-format the emitter depends on whatever built-in
default the installed clang-format happens to use; without a version pin, a
distro upgrade silently reformats the golden tree.
"""

from pathlib import Path

from tests.conftest import PINNED_CLANG_FORMAT, clang_format_version

REPO_ROOT = Path(__file__).resolve().parent.parent
# The golden tree is BYTE-exact, so the contract is the FULL version, not the
# major: clang-format's default style can shift in any release. Keep this in
# sync with mise.toml [tools] and scripts/check_golden.sh.
EXPECTED_VERSION = "22.1.8"


def test_clang_format_config_is_committed():
    cfg = REPO_ROOT / ".clang-format"
    assert cfg.exists(), ".clang-format must be committed for byte-exact generation"
    assert "BasedOnStyle" in cfg.read_text()


def test_mise_pins_the_formatter_exactly():
    mise_toml = (REPO_ROOT / "mise.toml").read_text()
    assert f'clang-format = "{EXPECTED_VERSION}"' in mise_toml, (
        f"mise.toml [tools] must pin clang-format to exactly {EXPECTED_VERSION}"
    )


def test_emitter_passes_an_explicit_style_file():
    """clang-format must not be left to search upward for a style.

    The golden gate regenerates into a temp dir outside the repo, so an implicit
    search would find no .clang-format there and silently fall back to the
    built-in default -- formatting the temp tree differently from the committed
    one and breaking the gate forever.
    """
    src = (REPO_ROOT / "src/x3d_cpp_gen/backends/cpp_header.py").read_text()
    assert "--style=file:" in src, (
        "cpp_header.py must invoke clang-format with an explicit "
        "--style=file:<abs path>, not rely on an upward search"
    )


def test_conftest_pin_matches():
    """The golden fixture (conftest.py) resolves against the same pin."""
    assert PINNED_CLANG_FORMAT == EXPECTED_VERSION


def test_installed_formatter_matches_the_pin(pinned_clang_format):
    # The fixture already skips (locally) or fails (under CI) on a missing or
    # mismatched formatter, with install instructions; this states the contract.
    assert clang_format_version(pinned_clang_format) == EXPECTED_VERSION
