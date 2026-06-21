# `src/app/CliParser.hpp`

Updated: 2026-06-20

Exact purpose: parse deterministic tool/demo command lines into `AppConfig` plus diagnostics.

## Build Position

- priority rank: 30
- tier: Tier 2: Content And Configuration
- module: `app support`
- file kind: `header`

## Ownership

This file owns:

- accepted option names
- help/version output shape
- CLI error diagnostics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- config/core helpers
- standard filesystem/string containers
- no gameplay mutation rules

## Data Contract

- supports `--fixture`, `--save`, `--load`, `--replay`, `--camera`, `--summary`, `--verbose`, `--help`

## Semantics

- parsing is order independent except last value wins for repeated scalar options
- unknown options fail fast
- no runtime object is created during parsing

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(argument count plus total argument bytes).
- Parsing cost is independent of package size, runtime world size, renderer
  state, and command log size.

## Diagnostics And Errors

- parser failures return `CliParseResult` with `AppConfigStatus` and stable
  `cli.*` diagnostic codes;
- unknown options and missing values are parser errors, not runtime errors;
- human help text must not be the only machine-readable outcome.

## Save Replay Multiplayer Notes

- CLI arguments are process inputs, not save truth.
- Replay files are selected by path here, but replay execution and command
  history ownership live in runtime/replay docs.
- No multiplayer identity, socket identity, entity id, or command id is stored
  in parser output.

## Tests And Verification

- app/CLI tests cover accepted options, aliases, repeated scalar behavior,
  unknown options, missing values, invalid cameras, help/version output, and
  argc/argv parity;
- tests must not create runtime sessions or load packages;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/app/CliParser.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Repo Path
`src/app/CliParser.hpp`

### Required Includes And Namespace

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/AppConfig.hpp"
#include "core/diagnostics/Diagnostic.hpp"
```

All declarations live in `namespace iggy3d`.

### Required Result Type

```cpp
struct CliParseResult {
  AppConfigStatus status = AppConfigStatus::Ok;
  AppConfig config;
  std::vector<Diagnostic> diagnostics;
};
```

### Required API

```cpp
CliParseResult parseCommandLine(int argc, const char* const* argv);
CliParseResult parseCommandLine(std::vector<std::string_view> args);
std::string_view cliHelpText();
std::string_view cliVersionText();
```

The `argc/argv` overload converts arguments to views and delegates to the vector
overload. `args[0]` is treated as program name when present and is ignored for
option parsing.

### Accepted Option Grammar

Primary modes:

- `--demo`: `mode=HeadlessDemo`.
- `--validate-package`: `mode=ValidatePackage`.
- `--replay`: `mode=Replay` and requires `--replay-path`.
- `--help` or `-h`: `mode=Help`, `printHelp=true`.
- `--version`: `mode=Version`, `printVersion=true`.

Value options:

- `--package <path>` or `--fixture <path>`: set `packagePath`.
- `--save <path>`: set `savePath`.
- `--load <path>`: set `loadPath`.
- `--replay-path <path>`: set `replayPath`.
- `--summary <path>`: set `expectedSummaryPath`.
- `--camera <FirstPerson|ThirdPerson>`: set `requestedRealtimeCamera`.

Flag options:

- `--strict`: set `strict=true`.
- `--verbose` or `-v`: set `verbose=true`.

No short bundled flags are accepted; `-vh` is `UnknownOption`.

### Diagnostics

Parser-origin diagnostics use these exact codes:

- `cli.unknown_option`
- `cli.missing_option_value`
- `cli.conflicting_mode`
- `cli.invalid_camera_mode`
- `cli.invalid_app_config`

The status field mirrors the primary diagnostic status. `config` is always
returned so tests can inspect partial parse state, but callers must ignore it
when status is not `Ok`.

### Dependencies

Allowed: app config, diagnostics, standard strings/vectors.
Forbidden: content loading, session creation, renderer, projection, filesystem
mutation, wall-clock reads, sockets, and old iggy headers.

### Tests

App/CLI tests must cover help text, version text, default demo parse, package
alias, repeated scalar last-value policy, missing value, unknown option,
invalid camera, conflicting primary modes, replay mode requiring replay path,
and `parseCommandLine(argc, argv)` parity with the vector overload.

### Completion Criteria

The header locks the CLI public API, accepted grammar, result shape, and
diagnostic codes without constructing runtime state.
