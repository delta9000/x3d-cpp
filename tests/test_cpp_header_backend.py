import subprocess

from x3d_cpp_gen.backends.cpp_header import CppHeaderBackend


def test_format_batches_all_files_in_one_subprocess(monkeypatch):
    calls = []

    def fake_run(args, capture_output, text):
        calls.append((args, capture_output, text))
        return subprocess.CompletedProcess(args=args, returncode=0)

    monkeypatch.setattr(subprocess, "run", fake_run)

    files = ["out/A.hpp", "out/A.cpp", "out/B.hpp"]
    CppHeaderBackend._format(files, "clang-format")

    assert calls == [
        (["clang-format", "-i", *files], True, True),
    ]


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
