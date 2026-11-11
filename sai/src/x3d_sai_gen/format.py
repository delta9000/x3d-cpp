"""Batch clang-format over emitted headers.

The generated C++ tree is a byte-exact artifact, so its formatting is part of
the contract. The style is pinned by the repository's .clang-format rather than
clang-format's built-in default, which is version-dependent.
"""

import subprocess
from pathlib import Path
from typing import List, Optional

_STYLE_FILE = Path(__file__).resolve().parents[2] / ".clang-format"


def format_files(output_files: List[str],
                 clang_format: Optional[str] = "clang-format") -> None:
    """Run clang-format once over all emitted files, if requested.

    If clang-format is unavailable, warn and skip formatting for the whole
    batch so generation still completes. A skipped batch will fail the
    determinism gate, which is the intended signal.
    """
    if not clang_format or not output_files:
        return
    style_arg = (f"--style=file:{_STYLE_FILE}" if _STYLE_FILE.exists()
                 else "--style=LLVM")
    try:
        result = subprocess.run(
            [clang_format, style_arg, "-i", *output_files],
            capture_output=True, text=True)
        if result.returncode != 0:
            print(f"WARNING: {clang_format} failed on {len(output_files)} "
                  f"files (exit {result.returncode}); batch formatting did "
                  f"not complete successfully.\n{result.stderr}")
    except FileNotFoundError:
        print(f"WARNING: {clang_format} not found; emitted files are "
              f"unformatted.")
