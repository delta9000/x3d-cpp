import subprocess

from x3d_cpp_gen.backends.cpp_header import CppHeaderBackend, _STYLE_FILE


def test_format_batches_all_files_in_one_subprocess(monkeypatch):
    calls = []

    def fake_run(args, capture_output, text):
        calls.append((args, capture_output, text))
        return subprocess.CompletedProcess(args=args, returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)

    files = ["out/A.hpp", "out/A.cpp", "out/B.hpp"]
    CppHeaderBackend._format(files, "clang-format")

    assert calls == [
        (["clang-format", f"--style=file:{_STYLE_FILE}", "-i", *files], True, True),
    ]


def test_format_passes_an_explicit_style_rather_than_searching_upward(monkeypatch):
    """The style must never be left to clang-format's upward directory search.

    The golden gate regenerates into a temp dir outside the repo; an implicit
    search would find no .clang-format there and silently fall back to the
    built-in default, formatting the temp tree differently from the committed
    one and failing the gate forever.
    """
    calls = []

    def fake_run(args, capture_output, text):
        calls.append(args)
        return subprocess.CompletedProcess(args=args, returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)
    CppHeaderBackend._format(["out/A.hpp"], "clang-format")

    style_args = [a for a in calls[0] if a.startswith("--style=")]
    assert len(style_args) == 1, f"expected exactly one --style arg, got {calls[0]}"
    assert style_args[0].startswith("--style=file:") or style_args[0] == "--style=LLVM"


def test_format_stops_cleanly_when_clang_format_is_missing(monkeypatch, capsys):
    def fake_run(args, capture_output, text):
        raise FileNotFoundError

    monkeypatch.setattr(subprocess, "run", fake_run)

    result = CppHeaderBackend._format(["out/A.hpp"], "clang-format")

    assert result is None
    assert "skipping formatting" in capsys.readouterr().out


def test_format_skips_when_formatter_is_disabled(monkeypatch):
    called = False

    def fake_run(args, capture_output, text):
        nonlocal called
        called = True
        return subprocess.CompletedProcess(args=args, returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)

    assert CppHeaderBackend._format(["out/A.hpp"], None) is None
    assert not called


def test_format_skips_empty_batches(monkeypatch):
    called = False

    def fake_run(args, capture_output, text):
        nonlocal called
        called = True
        return subprocess.CompletedProcess(args=args, returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)

    CppHeaderBackend._format([], "clang-format")
    assert not called


def test_format_splits_large_batches_across_concurrent_processes(monkeypatch):
    """A full-tree batch is split into a few concurrent clang-format calls.

    Formatting dominates generation time; each file is formatted independently,
    so splitting keeps the output identical. Every file must be formatted
    exactly once, every call must carry the explicit style, and the number of
    processes stays bounded by the CPU count.
    """
    import os
    import threading

    lock = threading.Lock()
    calls = []

    def fake_run(args, capture_output, text):
        with lock:
            calls.append(args)
        return subprocess.CompletedProcess(args=args, returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)
    monkeypatch.setattr(os, "cpu_count", lambda: 4)

    files = [f"out/N{i}.hpp" for i in range(686)]
    CppHeaderBackend._format(files, "clang-format")

    assert 1 < len(calls) <= 4
    formatted = []
    for args in calls:
        assert args[:3] == ["clang-format", f"--style=file:{_STYLE_FILE}", "-i"]
        formatted += args[3:]
    assert sorted(formatted) == sorted(files)
