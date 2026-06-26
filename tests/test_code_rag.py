import importlib.util, pathlib, subprocess, sys

# Load scripts/code_rag.py as a module (it lives outside the package).
_SPEC = importlib.util.spec_from_file_location(
    "code_rag", pathlib.Path(__file__).parent.parent / "scripts" / "code_rag.py")
code_rag = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(code_rag)


SAMPLE = '''\
// File summary line one.
// File summary line two.
#ifndef X_HPP
#define X_HPP
namespace x3d::runtime {

// Computes the thing.
class BoundsSystem {
public:
  // Build all bounds.
  void buildBounds(const Scene &s) {
    do_work();
  }
  int localCount() const { return n_; }
private:
  int n_ = 0;
};

// A pure data record.
struct RenderItem {
  int id;
  float xyz[3];
};

// Free helper.
inline int clampIndex(int i) {
  return i < 0 ? 0 : i;
}

} // namespace
#endif
'''


def test_iter_symbols_methods_struct_and_free_function():
    syms = code_rag.iter_symbols(SAMPLE, file="X.hpp")
    by_name = {s.symbol: s for s in syms}
    assert "BoundsSystem::buildBounds" in by_name
    assert by_name["BoundsSystem::buildBounds"].kind == "method"
    assert by_name["BoundsSystem::buildBounds"].cls == "BoundsSystem"
    assert "BoundsSystem::localCount" in by_name
    assert by_name["BoundsSystem::localCount"].kind == "method"
    assert by_name["BoundsSystem::localCount"].cls == "BoundsSystem"
    # RenderItem has no method bodies -> emitted as a single struct symbol.
    assert "RenderItem" in by_name
    assert by_name["RenderItem"].kind == "struct"
    assert by_name["RenderItem"].cls == ""
    # Free function at namespace scope.
    assert "clampIndex" in by_name
    assert by_name["clampIndex"].kind == "function"
    assert by_name["clampIndex"].cls == ""
    # BoundsSystem itself is NOT emitted as a struct symbol (it has method bodies).
    assert "BoundsSystem" not in by_name
    # Full text span is captured.
    assert "do_work();" in by_name["BoundsSystem::buildBounds"].text


def test_file_header_and_leading_comment_and_embed_text():
    assert "File summary line one." in code_rag.file_header(SAMPLE)
    assert "File summary line two." in code_rag.file_header(SAMPLE)
    assert "#ifndef" not in code_rag.file_header(SAMPLE)

    syms = {s.symbol: s for s in code_rag.iter_symbols(SAMPLE, file="X.hpp")}
    bb = syms["BoundsSystem::buildBounds"]
    lc = code_rag.leading_comment(SAMPLE, bb.start_line)
    assert "Build all bounds." in lc

    et = code_rag.embed_text(bb, SAMPLE)
    assert "File summary line one." in et          # file header folded in
    assert "BoundsSystem::buildBounds" in et       # symbol id present
    assert "Build all bounds." in et               # leading doc-comment
    assert "do_work();" in et                       # body present


def test_scope_tags_and_exclusions(tmp_path):
    # Build a miniature repo tree.
    (tmp_path / "runtime/scene").mkdir(parents=True)
    (tmp_path / "runtime/scene/BoundsSystem.hpp").write_text("// x\n")
    (tmp_path / "runtime/scene/tests").mkdir()
    (tmp_path / "runtime/scene/tests/bounds_test.cpp").write_text("// t\n")
    (tmp_path / "runtime/script/vendor/duktape").mkdir(parents=True)
    (tmp_path / "runtime/script/vendor/duktape/duktape.h").write_text("// vendor\n")
    (tmp_path / "runtime/parse").mkdir(parents=True)
    (tmp_path / "runtime/parse/tinfl.h").write_text("// vendor\n")
    (tmp_path / "include/x3d").mkdir(parents=True)
    (tmp_path / "include/x3d/sdk.hpp").write_text("// sdk\n")
    (tmp_path / "third_party/glad").mkdir(parents=True)
    (tmp_path / "third_party/glad/glad.c").write_text("// glad\n")
    (tmp_path / "generated_cpp_bindings").mkdir()
    (tmp_path / "generated_cpp_bindings/Shape.cpp").write_text("// gen\n")

    got = {str(p.relative_to(tmp_path)): tag for p, tag in code_rag.scope(tmp_path)}
    assert got["runtime/scene/BoundsSystem.hpp"] == "runtime"
    assert got["include/x3d/sdk.hpp"] == "runtime"
    assert got["runtime/scene/tests/bounds_test.cpp"] == "test"
    assert got["generated_cpp_bindings/Shape.cpp"] == "generated"
    # vendored / third-party fully excluded
    assert not any("vendor" in k or "tinfl" in k or "third_party" in k for k in got)
    # `only` filter
    only_rt = {str(p.relative_to(tmp_path)) for p, _ in code_rag.scope(tmp_path, only={"runtime"})}
    assert "runtime/scene/tests/bounds_test.cpp" not in only_rt
    assert "runtime/scene/BoundsSystem.hpp" in only_rt


def test_embed_input_roles():
    q = code_rag._embed_input(["how are bounds computed"], "query")
    assert q[0].startswith("Instruct: ")
    assert "Query: how are bounds computed" in q[0]
    d = code_rag._embed_input(["raw body text"], "document")
    assert d == ["raw body text"]


def test_chunks_for_symbol_small_and_oversize():
    syms = {s.symbol: s for s in code_rag.iter_symbols(SAMPLE, file="X.hpp")}
    bb = syms["BoundsSystem::buildBounds"]
    small = code_rag.chunks_for_symbol(bb, SAMPLE, "runtime")
    assert len(small) == 1
    assert small[0]["payload"]["symbol"] == "BoundsSystem::buildBounds"
    assert small[0]["payload"]["source"] == "runtime"
    assert small[0]["payload"]["text"] == bb.text  # full source in payload

    # Oversize: synthesize a symbol whose text exceeds SPLIT_CHARS.
    big = code_rag.Symbol("Big.hpp", "huge", "function", "", 1, 999,
                          "void huge(){\n" + ("  step();\n" * 4000) + "}\n")
    chunks = code_rag.chunks_for_symbol(big, big.text, "runtime")
    assert len(chunks) > 1
    assert all(c["payload"]["symbol"] == "huge" for c in chunks)   # all same symbol
    assert len({c["payload"]["win"] for c in chunks}) == len(chunks)  # distinct windows
    assert all(len(c["embed"]) <= code_rag.WINDOW_CHARS + 4000 for c in chunks)


def test_point_id_deterministic_and_distinct():
    a = code_rag._point_id("X.hpp", "A::b", 10, 0)
    assert a == code_rag._point_id("X.hpp", "A::b", 10, 0)    # stable
    assert a != code_rag._point_id("X.hpp", "A::b", 10, 1)    # window distinct
    assert a != code_rag._point_id("Y.hpp", "A::b", 10, 0)    # file distinct
    assert isinstance(a, int) and a > 0
    # Overload-collision regression: same file+symbol+win but different start_line
    # must produce different ids (the original bug was these collided).
    b = code_rag._point_id("X.hpp", "A::b", 20, 0)
    assert a != b, "overloaded symbols at different start_line must have distinct ids"


def test_points_from_chunks_shape():
    syms = {s.symbol: s for s in code_rag.iter_symbols(SAMPLE, file="X.hpp")}
    chunks = code_rag.chunks_for_symbol(syms["clampIndex"], SAMPLE, "runtime")
    vecs = [[0.0] * code_rag.DIM for _ in chunks]
    pts = code_rag._points_from_chunks(chunks, vecs)
    assert len(pts) == len(chunks)
    p = pts[0]
    assert set(p) == {"id", "vector", "payload"}
    assert p["payload"]["symbol"] == "clampIndex"
    assert len(p["vector"]) == code_rag.DIM
    assert p["id"] == code_rag._point_id("X.hpp", "clampIndex", syms["clampIndex"].start_line, 0)


SCRIPT = str(pathlib.Path(__file__).parent.parent / "scripts" / "code_rag.py")


def test_cli_usage_when_no_args():
    r = subprocess.run([sys.executable, SCRIPT], capture_output=True, text=True)
    assert r.returncode != 0
    assert "usage" in (r.stdout + r.stderr).lower()


def test_cli_typo_guard_for_symbols():
    # `symbols Foo` (note the trailing arg) is a typo'd subcommand -> fail loud.
    r = subprocess.run([sys.executable, SCRIPT, "symbols", "Foo"],
                       capture_output=True, text=True)
    assert r.returncode != 0
    assert "did you mean" in (r.stdout + r.stderr).lower()
    assert "symbol" in (r.stdout + r.stderr).lower()


def test_cli_help():
    r = subprocess.run([sys.executable, SCRIPT, "--help"], capture_output=True, text=True)
    assert r.returncode == 0
    assert "WHICH ONE" in r.stdout


def test_dedup_by_symbol_keeps_best_and_limits():
    results = [
        {"score": 0.40, "payload": {"file": "A.hpp", "symbol": "f", "win": 0}},
        {"score": 0.90, "payload": {"file": "A.hpp", "symbol": "f", "win": 1}},
        {"score": 0.50, "payload": {"file": "B.hpp", "symbol": "g", "win": 0}},
        {"score": 0.10, "payload": {"file": "C.hpp", "symbol": "h", "win": 0}},
    ]
    out = code_rag._dedup_by_symbol(results, k=2)
    assert len(out) == 2
    # 'f' collapses to its best (0.90) and ranks first.
    assert out[0]["payload"]["symbol"] == "f" and out[0]["score"] == 0.90
    assert out[1]["payload"]["symbol"] == "g"
