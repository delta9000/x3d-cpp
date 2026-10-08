# Security & Correctness Audit — x3d-cpp runtime

Date: 2026-10-08
Scope: parse front door, path confinement, gzip, IO backends (curl/S3/file),
recursion caps, script backends, JSON/XML parsers, CLI tooling.
Baseline commit: `a490af4` (branch `fix/integer-trigger-listener-lifetime`).

Overall: the codebase is unusually well-hardened (SEC-1…SEC-6 markers, thorough
comments). One real security hole and a handful of smaller issues found.

---

## HIGH

### 1. SEC-3 path traversal regression: `localFileInlineResolver` never confines
**File:** `runtime/parse/X3DParse.cpp:330-361`

The PROTO resolver was fixed to use `confineLocalIncludePath()` (~line 305), but
the **default Inline resolver** still builds its path the old way:

```cpp
std::string path = url;
if (!baseUrl.empty() && !path.empty() && path.front() != '/')
  path = baseUrl + "/" + path;   // no confineLocalIncludePath, no canonicalization
```

Only a leading-`/` check exists. `<Inline url='"../../../../etc/passwd"'/>`
resolves outside the confinement root and `parseFile()` (which opens whatever it
is given) reads it; contents leak through parse errors/warnings or partially
spliced nodes. This directly contradicts the comment in `PathConfine.hpp:5-8`,
which claims both default resolvers were fixed. Existing tests
(`runtime/parse/tests/path_confine_test.cpp`) cover only the confine function and
`FileResolver`, not this call site — the hole is untested.

Same lines: the cycle-guard key is the **raw, non-canonical** string (the SEC-4
fix was applied to the PROTO resolver only), so two spellings of one file can
alias past the guard.

**Fix:** route the Inline path through `confineLocalIncludePath(url, baseUrl,
detail::activeConfineRoot())` exactly like the PROTO resolver; use the returned
canonical path for both `parseFile` and the `activeFiles` cycle-guard key. Add a
regression test with a `../` Inline url.

## MEDIUM

### 2. S3 backend is the unguarded sibling of the curl backend
**File:** `runtime/io/s3/S3Resolver.cpp:60-77`

The curl resolver has a careful post-DNS private-network guard, proxy disabling,
protocol allowlist, size/timeout caps. The S3 resolver has none of that:
- hardcoded `Scheme::HTTP` (plaintext credentials/object data in transit)
- anonymous access, `endpointOverride` with no validation
- unbounded read (`std::istreambuf_iterator` into a vector, no maxBytes
  equivalent → memory-exhaustion DoS on a huge object)

It's embedder-opt-in, but the asymmetry invites the same SSRF/DoS class that
SEC-6 was written to prevent. `parseS3Url` also doesn't percent-decode the key.

### 3. `DynamicFieldStore` global singleton keyed on node pointer
**File:** `tools/x3d-cli/canon_gate.cpp:85-90` (acknowledged in a comment):
"new nodes may reuse freed addresses from pass 1 and pick up phantom author
fields", worked around by spawning subprocesses.

A pointer-keyed global that outlives the graphs whose pointers it indexes is a
use-after-free-shaped correctness landmine for any embedder that parses two
documents in one process (the primary supported use case — `RuntimeSession` per
document). Deserves an epoch/generation key or per-document scoping in the
library, not a CLI workaround.

### 4. QuickJS backend: single shared deadline not thread-safe
**File:** `runtime/script/QuickJsBackend.cpp:687-701`

`deadlineArmed`/`deadline` are plain members on the shared `Impl` (one runtime
for all scripts). Fine while calls are strictly sequential; if any embedder
calls the engine from two threads it is a data race on state the interrupt
handler reads (torn/unrelated deadline would abort the wrong script). At
minimum, document/enforce single-threadedness on the `ScriptEngine` seam, since
the seam itself doesn't. Also (per the code's own comment) the memory cap is on
the *combined* heap — one script can exhaust the budget for all others
(intentional, but worth surfacing per-script).

## LOW / NITS

### 5. Multi-member gzip unsupported with misleading error
**File:** `runtime/parse/Inflate.hpp:132-152`
Footer is always taken from the last 8 bytes of the whole input, so a spec-legal
concatenation of gzip members fails with a spurious "corrupt stream"/CRC error.
Fine to reject, but the error message is misleading; alternatively decode
member-by-member.

### 6. `JsonLite::Parser::parse()` accepts trailing garbage
**File:** `runtime/parse/JsonLite.hpp:77-82`
Top-level value is parsed, whitespace skipped, then whatever follows is ignored.
Malformed X3D-JSON with trailing junk parses "successfully".

### 7. Blocked-IP gaps in the SSRF guard
**File:** `runtime/io/curl/HttpResolver.cpp:33-42`
Doesn't block 224.0.0.0/4 (multicast), 240.0.0.0/4 (reserved, incl.
255.255.255.255), 192.0.0.0/24, or 198.18.0.0/15 (benchmark). Defense-in-depth
nitting; the important ranges are covered.

### 8. Cycle-guard stack leaks on non-`std::exception` throws
**File:** `runtime/parse/X3DParse.cpp:289-326` (both resolvers)
`activeFiles.push_back`/`pop_back` bracket a `catch (const std::exception&)`;
anything else (or a deliberate `throw 0`) leaves a stale entry that blocks that
path for the rest of the thread's life. An RAII guard (like `NestingGuard`,
which got this exact treatment) would fix it.

### 9. `parseFile` base-dir split mixes separators
**File:** `runtime/parse/X3DParse.cpp:389-391`
`find_last_of("/\\")` treats a literal backslash in a POSIX filename as a path
separator, producing a wrong `baseUrl` for such files.

### 10. `confineLocalIncludePath` returns the root itself
**File:** `runtime/parse/PathConfine.hpp:~80`
`rel == "."` passes the escape check, so `url='.'` yields the root directory as
an "include path" (harmlessly fails at `ifstream`, but the claimed invariant is
"strictly under").

### 11. `canon_gate.cpp` popen — `g_x3dBin` unquoted
**File:** `tools/x3d-cli/canon_gate.cpp:~100`
Path quoting is correctly done; only `g_x3dBin` (from `argv[1]`) goes into the
shell unquoted. Dev-tool-only, low risk.

## CHECKED OUT (no action needed)

- `PathConfine.hpp` itself (weak-canonical resolve + `lexically_relative`
  escape check) is sound; symlinks inside the root pointing out are caught
  because `weakly_canonical` resolves them.
- Inflate's decompression-bomb handling (clamped initial reservation, ceiling on
  the grow loop) and the `NestingGuard` check-before-increment trick are both
  correct.
- curl guard runs in `OPENSOCKETFUNCTION` (post-DNS, per redirect hop), env
  proxies disabled in the hardened default, write callback enforces the byte cap
  even without Content-Length — solid.
- `validateUnits` version parsing handles the empty/dotless/malformed cases;
  UNIT category/factor validation is thorough.

## Suggested fix order

1. `#1` first — one-call fix (route Inline through `confineLocalIncludePath`
   like its sibling) plus a regression test with a `../` Inline url.
2. `#8` (RAII cycle-guard) folds naturally into the same edit.
3. `#2`, `#3`, `#4` as follow-up PRs.
4. Low/nits opportunistically.