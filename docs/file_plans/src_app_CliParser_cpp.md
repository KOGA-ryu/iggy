# `src/app/CliParser.cpp`

Updated: 2026-06-20

Exact purpose: parse deterministic tool/demo command lines into `AppConfig` plus diagnostics.

## Build Position

- priority rank: 31
- tier: Tier 2: Content And Configuration
- module: `app support`
- file kind: `source`

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

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

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

- `src/app/CliParser.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Repo Path
`src/app/CliParser.cpp`

### Include Order

```cpp
#include "app/CliParser.hpp"

#include <array>
```

No package loader, session, renderer, app main loop, filesystem mutation, socket,
timer, test, or old iggy include is permitted.

### Parsing Algorithm

1. Start from `makeDefaultAppConfig()`.
2. Skip `args[0]` when it does not begin with `-`.
3. Iterate left to right over remaining args.
4. For a primary mode option, set the mode if no previous primary mode was set;
   otherwise return `ConflictingMode` with `cli.conflicting_mode`.
5. For a value option, require a following token that is not another option;
   missing values return `MissingOptionValue` with `cli.missing_option_value`.
6. For repeated scalar value options, the last value wins.
7. For repeated flag options, the value remains true and no diagnostic is added.
8. Unknown tokens beginning with `-` return `UnknownOption` with
   `cli.unknown_option`.
9. Positional tokens are not accepted in the first build and return
   `UnknownOption`.
10. Parse `--camera` value exactly; any other spelling returns
    `InvalidCameraMode` with `cli.invalid_camera_mode`.
11. If no primary mode was set, keep default `HeadlessDemo`.
12. Call `validateAppConfig`; if it fails, return that status with
    `cli.invalid_app_config` and the app status code as diagnostic detail.
13. Return `Ok`.

### Option Value Rules

- `--package` and `--fixture` are synonyms; both set `packagePath`.
- `--replay` selects replay mode only; it does not consume a path.
- `--replay-path` supplies the replay file path.
- `--camera FirstPerson` maps to `CameraMode::FirstPerson`.
- `--camera ThirdPerson` maps to `CameraMode::ThirdPerson`.
- `TacticalOverhead` is rejected for realtime startup camera by the parser.

### Help And Version

`cliHelpText()` returns a static string containing every accepted option name
listed in `CliParser.hpp`. `cliVersionText()` returns a static string beginning
with `iggy3d ` followed by the planned package/runtime schema version. Neither
function reads files, environment variables, git state, or wall-clock time.

### Failure Behavior

Parsing mutates only the local `AppConfig` inside `CliParseResult`. It never
creates runtime state, opens files, creates directories, starts replay, or calls
package validation.

### Compute Cost

Parsing is O(argument count plus total argument bytes). Help/version access is
O(1).

### Tests

App/CLI tests must assert:

- `[]` or only program name returns default first-room `HeadlessDemo`;
- `--help` ignores invalid absent package and returns `Help`;
- `--version` returns `Version`;
- `--validate-package --package fixtures/demos/first_room/package.iggy3d.toml`
  returns `ValidatePackage`;
- `--demo --validate-package` returns `ConflictingMode`;
- `--package a --package b` leaves `packagePath=="b"`;
- `--camera TacticalOverhead` returns `InvalidCameraMode`;
- `--save` returns `MissingOptionValue`;
- `--unknown` returns `UnknownOption`;
- `--replay --replay-path run.iggy3d.replay` returns `Replay`.

### Completion Criteria

The source implements the documented grammar with stable parser diagnostics and
delegates all process-config validation to `AppConfig.cpp`.
