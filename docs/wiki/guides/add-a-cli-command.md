---
title: Add a CLI Command
summary: Step-by-step recipe for adding a new subcommand to the x3d CLI, including the dispatch table, handler pattern, arg parsing, oracle/gate wiring, and CLI test.
tags: [guide, cli, commands, extensibility]
updated: 2026-06-20
related:
  - ../subsystems/cli-suite.md
  - gate-system.md
---

# Add a CLI Command

This guide walks through every change needed to ship a new `x3d <cmd>` subcommand. All five existing commands (`convert`, `validate`, `extract`, `canonicalize`, `sim`) follow the same pattern; this guide uses `cmdExtract` and `cmdSim` as concrete examples throughout.

The single file that matters for the dispatch layer is `tools/x3d_cli.cpp`. Helper headers live in `tools/x3d-cli/`.

---

## 1. Understand the shape

```
x3d <subcommand> [args]
x3d <subcommand> --help
```

`main` in `tools/x3d_cli.cpp` reads `argv[1]` as the subcommand name, builds a tail `std::vector<std::string>` from `argv[2..]`, and dispatches into a static table:

```cpp
static const SubCmd kCmds[] = {
    {"convert",      "Convert an X3D scene between encodings",        cmdConvert},
    {"validate",     "Validate an X3D scene and report diagnostics",  cmdValidate},
    {"extract",      "Export X3D geometry to a binary STL file",      cmdExtract},
    {"canonicalize", "Emit the X3D Canonical Form (X3DC14N)",         cmdCanonicalize},
    {"sim",          "Headless behavior simulation + field-change trace", cmdSim},
};
```

Each `SubCmd` entry is `{name, brief, run}` where `run` is `std::function<int(const std::vector<std::string>&)>`. Adding a command = adding one entry here and one handler function.

---

## 2. Write the handler

### Naming convention

Name the function `cmdYourName` (camelCase, `cmd` prefix). Keep it in the anonymous namespace at the top of `tools/x3d_cli.cpp`.

### Skeleton

```cpp
// ─── cmdYourName ──────────────────────────────────────────────────────────────

int cmdYourName(const std::vector<std::string> &args) {
    // x3d yourname <in> [--flag] [-o <out>]
    static const char *kUsage =
        "usage: x3d yourname <input> [--flag] [-o <output>]\n"
        "\n"
        "  One paragraph describing what this does.\n"
        "\n"
        "  Exit codes:\n"
        "    0  ok\n"
        "    1  usage error\n"
        "    2  parse / IO failure\n"
        "    3  issues found (only for commands that audit/report)\n";

    if (args.empty()) {
        std::cerr << kUsage;
        return 1;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        std::cout << kUsage;
        return 0;
    }

    std::string inPath;
    std::string outPath;
    bool flag = false;

    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string &a = args[static_cast<size_t>(i)];
        if (a == "-o") {
            if (i + 1 >= static_cast<int>(args.size())) {
                std::cerr << "error: -o requires an argument\n" << kUsage;
                return 1;
            }
            outPath = args[static_cast<size_t>(++i)];
        } else if (a == "--flag") {
            flag = true;
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "error: unknown option: " << a << "\n" << kUsage;
            return 1;
        } else if (inPath.empty()) {
            inPath = a;
        } else {
            std::cerr << "error: unexpected argument: " << a << "\n" << kUsage;
            return 1;
        }
    }

    if (inPath.empty()) {
        std::cerr << "error: missing input file\n" << kUsage;
        return 1;
    }

    // Parse.
    sdk::X3DDocument doc;
    try {
        doc = sdk::parseFile(inPath);
    } catch (const std::exception &e) {
        std::cerr << "error: failed to parse '" << inPath << "': " << e.what() << "\n";
        return 2;
    }
    surfaceWarnings(doc);   // surfaces rangeWarnings + protoWarnings to stderr (non-fatal)

    // ... your work here ...

    return 0;
}
```

### Gotchas

- **Always handle `--help` before touching the arg list.** If `args` is empty, print usage to `stderr` and return `1` (usage error, not parse error). If `args[0]` is `--help`/`-h`, print to `stdout` and return `0`.
- **Unknown `-` prefixed flags must return `1`**, not `2`. Unknown options are a *usage* error. The `!a.empty() && a[0] == '-'` guard in the loop handles this.
- **Positional arg overflow returns `1`.** If a second positional arrives when `inPath` is already set, that is a usage error.
- **Parse failure returns `2`.** All `sdk::parseFile` exceptions, IO errors, and serialization failures return `2`.
- **Do not swallow exceptions silently.** Every `try` must have a matching `catch (const std::exception &e)` that emits to `stderr` with an `"error: "` prefix.
- **`surfaceWarnings`** is a file-local helper (already in scope) that emits `doc.rangeWarnings` and `doc.protoWarnings` to `stderr` as non-fatal notices. Call it after `parseFile` if your command uses the parsed document.
- **Stdout vs file.** The `writeOutput(outPath, content)` helper (already in scope) writes to a file if `outPath` is non-empty, or to `stdout` otherwise. Use it for text output; for binary output (like `cmdExtract`'s STL), write directly to `std::cout` or `std::ofstream` as `cmdExtract` does.

---

## 3. Register in `kCmds[]`

Add one line to the `kCmds[]` array near the bottom of the anonymous namespace in `tools/x3d_cli.cpp`:

```cpp
static const SubCmd kCmds[] = {
    {"convert",      "...", cmdConvert},
    {"validate",     "...", cmdValidate},
    {"extract",      "...", cmdExtract},
    {"canonicalize", "...", cmdCanonicalize},
    {"sim",          "...", cmdSim},
    {"yourname",     "One-line brief shown in x3d --help", cmdYourName},  // <-- add this
};
```

Also add a matching line to the `kTopUsage` string literal directly below:

```cpp
static const char *kTopUsage =
    ...
    "  yourname     One-line brief shown in x3d --help\n"
    ...
```

That is the only change needed for dispatch. The `main` function iterates `kCmds` at runtime — no other wiring required.

---

## 4. Exit-code convention

Every command must use exactly this set:

| Code | Meaning |
|---|---|
| `0` | Success (including zero-result scenes — e.g. an empty STL is still exit 0) |
| `1` | Usage error — wrong flags, missing required arg, unknown option |
| `2` | Parse / IO failure — `sdk::parseFile` threw, file not found, serialization failed |
| `3` | Issues found — used by `validate` (diagnostics present); only for audit/report commands |

Use `2` for runtime failures you cannot control (bad file). Use `1` for things the caller got wrong (missing argument). Never return non-zero for a successfully-processed empty result.

---

## 5. Add a CLI test

All CLI behavior is tested in `tools/tests/x3d_cli_test.sh`. Each command gets a block of checks in this script. The test is invoked by the `x3d_cli_test` CTest target with:

```
$1 = path to the x3d binary
$2 = path to tools/x3d-cli/fixtures/
$3 = path to tools/x3d-cli/goldens/ (optional; used for golden-trace regression)
```

### Mandatory checks for every new command

Add a block like this:

```bash
# ════════════════════════════════════════════════════════════════════════════
# yourname: <what it does>
# ════════════════════════════════════════════════════════════════════════════

FIXTURE_YOURNAME="$FIXTURES/yourname-fixture.x3d"

# N+0. yourname --help exits 0
ec=$("$CLI" yourname --help >/dev/null 2>&1; echo $?)
check "yourname --help exits 0" "$ec" "0"

# N+1. yourname no args → exit 1
ec=$("$CLI" yourname >/dev/null 2>&1; echo $?)
check "yourname no args exits 1" "$ec" "1"

# N+2. yourname missing file → exit 2
ec=$("$CLI" yourname /nonexistent/nope.x3d >/dev/null 2>&1; echo $?)
check "yourname missing file exits 2" "$ec" "2"

# N+3. yourname unknown option → exit 1
ec=$("$CLI" yourname "$FIXTURE_YOURNAME" --unknown-flag >/dev/null 2>&1; echo $?)
check "yourname unknown option exits 1" "$ec" "1"

# N+4. yourname on fixture → exits 0 and produces expected output
out=$("$CLI" yourname "$FIXTURE_YOURNAME" 2>/dev/null || true)
ec=$("$CLI" yourname "$FIXTURE_YOURNAME" >/dev/null 2>&1; echo $?)
check "yourname fixture exits 0" "$ec" "0"
if [[ "$out" == *"expected content"* ]]; then
    echo "ok:   yourname output contains expected content"
else
    echo "FAIL: yourname output missing expected content (got: $out)"
    failures=$(( failures + 1 ))
fi
```

Use the `check` helper (already defined at the top of the script) for exit-code assertions. Use an inline `if` block for content assertions. **Never use `set -e`** — the script already avoids it because many sub-commands intentionally exit non-zero.

### Add the fixture

Drop a minimal `.x3d` file under `tools/x3d-cli/fixtures/` that exercises your command. The fixtures are small in-repo files (no corpus dependency). If your command produces golden output, commit the golden alongside the fixture.

### Build and run the test

```bash
mise run build          # compiles the x3d binary and the x3d_cli_test target
ctest --preset dev -R x3d_cli_test -V   # runs just the CLI test
```

Or run the full suite:

```bash
mise run build && ctest --preset dev    # all tests
```

---

## 6. Wire an oracle or gate (if needed)

Simple commands (those that just parse+emit) get automatic coverage from the existing `x3d_cli_gate` convert round-trip (it tests every file in the corpus subset for re-parseability). For commands that need deeper regression protection, wire an oracle:

### Self-oracle pattern (`cmdExtract`)

`cmdExtract` exports to binary STL, then the CTest target `extract_oracle_test` re-imports via `StlReader` and asserts triangle count and AABB within tolerance. The self-oracle lives in `tools/x3d-cli/extract_oracle_test.cpp`. Copy that pattern for commands with a natural round-trip (export → reimport → verify).

### Differential / golden-trace pattern (`cmdSim`)

`cmdSim` commits golden JSON traces under `tools/x3d-cli/goldens/` (e.g. `sim-anim.trace.json`). The CLI test (test 41 in `x3d_cli_test.sh`) asserts the current trace is byte-identical to the golden. To update a golden after an intentional behavioral change:

```bash
./build/x3d sim fixtures/sim-anim.x3d --fps 4 --ticks 8 \
    --watch Mover.translation --json \
    > tools/x3d-cli/goldens/sim-anim.trace.json
git add tools/x3d-cli/goldens/sim-anim.trace.json
git commit -m "chore(golden): update sim-anim trace — <reason>"
```

### Differential / differential-gate pattern (`cmdValidate`, `cmdConvert`, `cmdCanonicalize`)

These three are covered by the differential CLI gate (`x3d_cli_gate` and `x3d_canon_gate`). A new command that produces deterministic output over a large corpus is a candidate for the same pattern — see `tools/x3d-cli/cli_gate.cpp` and the [Gate System](gate-system.md) guide. The steps are:

1. Add your command's output format to `cli_gate.cpp` (or write a new gate binary).
2. Run `mise run cli-gate-baseline` to lock the initial baseline.
3. Commit the baseline TSV under `tools/x3d-cli/goldens/`.
4. `mise run cli-gate-regression` (part of `mise run ci`) will enforce it from then on.

---

## 7. Update `kTopUsage` and check `--help`

Verify the full top-level help text is coherent after your addition:

```bash
./build/x3d --help
./build/x3d yourname --help
```

The first must list your command and its brief. The second must print your `kUsage` string and exit `0`.

---

## 8. Run `mise run ci` before committing

```bash
mise run ci
```

This runs (in order): `mise run test` → `mise run golden` → `mise run conformance-gate` → `mise run coverage-gate` → `mise run doc-ctest-gate` → `mise run build` (which includes `ctest`) → `mise run cli-gate-regression`. The CLI test (`x3d_cli_test`) is part of the CTest suite inside `mise run build`.

Also run `mise run docs-build` if you edited any wiki pages, since the DOCS gate checks internal link integrity independently.

---

## Summary checklist

```
[ ] cmdYourName() written in tools/x3d_cli.cpp (anonymous namespace)
[ ] kCmds[] entry added (name + brief + handler)
[ ] kTopUsage string updated with matching line
[ ] Exit codes: 0/1/2 (and 3 if audit command)
[ ] Fixture added to tools/x3d-cli/fixtures/
[ ] CLI test block added to tools/tests/x3d_cli_test.sh (help/no-args/missing-file/unknown-flag/functional)
[ ] Oracle or golden committed if needed
[ ] mise run build && ctest passes
[ ] mise run ci passes
[ ] mise run docs-build passes (if wiki edited)
```
