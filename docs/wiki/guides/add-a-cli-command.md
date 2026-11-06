---
title: Add a CLI command
summary: Extend CLI dispatch, argument handling, and observable tests.
tags: [guide, cli]
updated: 2026-11-05
related: []
---

# Add a CLI command

`tools/x3d_cli.cpp` owns command dispatch and handlers. Supporting code lives
under `tools/x3d-cli/`. Read `cmdExtract` for a file-processing command and
`cmdSim` for runtime setup. The [CLI subsystem](../subsystems/cli-suite.md)
describes existing commands.

## Add the handler

Define `cmdName(const std::vector<std::string>& args)` in the file's anonymous
namespace. `main` passes arguments after the subcommand name.

Handle help before opening input. Reject missing option values, unknown flags,
and surplus positional arguments. Use the existing conventions:

| Exit code | Meaning |
|---|---|
| `0` | Success, including help and successfully processed empty results |
| `1` | Usage error |
| `2` | Parse, IO, or serialization failure |
| `3` | Reported validation issues |

Use `sdk::parseFile` for file input and `surfaceWarnings(doc)` for range and
PROTO notices. Catch failures with a useful stderr diagnostic.
`writeOutput` handles text files or stdout; use an appropriate binary stream
for binary formats.

## Register dispatch and help

Add `{name, brief, handler}` to `kCmds` and a corresponding line to `kTopUsage`.
Both live near the end of `tools/x3d_cli.cpp`. Verify top-level and command help
agree with the actual accepted options.

## Add observable checks

Extend `tools/tests/x3d_cli_test.sh`. Its CMake registration supplies the CLI,
fixture directory, golden directory, physics availability flag, and repository
root. Add fixtures under `tools/x3d-cli/fixtures/` when needed.

Check help, no arguments, unknown options, missing files, and successful output.
Assert the output's meaning, not only the exit code. For runtime behavior, use
a fixture with a known nonzero result; two empty traces can agree while both
implementations are broken. `tools/x3d-cli/sim_behavior_test.cpp` is an example.

Build and run the CLI test:

```sh
cmake --preset dev
cmake --build --preset dev --target x3d_cli -j3
ctest --preset dev -R x3d_cli_test --output-on-failure
```

If the command changes an existing corpus contract, inspect the
[regression gate](gate-system.md#corpus-regressions) before refreshing baselines.
Run `mise tasks` for the maintained build and verification commands.
