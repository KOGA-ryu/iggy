# `src/app/AppConfig.cpp`

Updated: 2026-06-20

Exact purpose: declare app-facing configuration produced by CLI parsing without leaking CLI concepts into runtime.

## Build Position

- priority rank: 29
- tier: Tier 2: Content And Configuration
- module: `app support`
- file kind: `source`

## Ownership

This file owns:

- fixture path
- save path
- replay path
- selected camera default
- verbosity
- validation mode

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

- paths are `std::filesystem::path` in the implementation file
- runtime config is embedded by value
- flags stay app-local

## Semantics

- invalid CLI data becomes diagnostics before runtime creation
- app config can select scenario but cannot bypass package validation

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(number of CLI arguments) during app startup, O(1) afterward.
- Validation cost is bounded by app path string length and does not scale with
  world, command log, renderer, or package entity count.

## Diagnostics And Errors

- expected app configuration failures use `AppConfigStatus`;
- `appConfigStatusCode` maps each status to a stable diagnostic code;
- human CLI text is secondary and must not be the only machine-readable result.

## Save Replay Multiplayer Notes

- `AppConfig` is process startup configuration, not runtime save truth.
- Package/session creation may copy validated values into runtime-owned state;
  after that point replay and save/load rely on runtime state, not this struct.
- No multiplayer identity, raw input, entity id, or command id is stored here.

## Tests And Verification

- app/CLI tests cover default construction, validation order, status-code
  mapping, path validation, camera validation, help/version bypass, and
  conflicting modes;
- tests must not touch the filesystem beyond lexical path strings;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/app/AppConfig.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Repo Path
`src/app/AppConfig.cpp`

### Include Order

```cpp
#include "app/AppConfig.hpp"

#include <string_view>
```

No content loader, session, renderer, filesystem mutation, socket, timer, test,
or old iggy include is permitted.

### Required Algorithms

`makeDefaultAppConfig`:

1. value-initialize `AppConfig`;
2. set `mode=HeadlessDemo`;
3. set `packagePath="fixtures/demos/first_room/package.iggy3d.toml"`;
4. leave save/load/replay/summary paths empty;
5. leave `runtimeConfig` at its documented defaults;
6. set `requestedRealtimeCamera=CameraMode::ThirdPerson`;
7. set all booleans false.

`validateAppConfig` must evaluate in this order and return the first failure:

1. if `printHelp` is true, require `mode==Help`, then return `Ok`;
2. if `printVersion` is true, require `mode==Version`, then return `Ok`;
3. reject incompatible flag/mode combinations as `ConflictingMode`;
4. reject `requestedRealtimeCamera` values other than `FirstPerson` or
   `ThirdPerson` as `InvalidCameraMode`;
5. for `HeadlessDemo` and `ValidatePackage`, reject empty `packagePath` as
   `MissingFixturePath`;
6. validate non-empty `packagePath`;
7. if `savePath` is present, validate save path;
8. if `loadPath` is present, validate load path;
9. if `mode==Replay`, reject empty `replayPath` as `InvalidReplayPath`;
10. if `replayPath` is present, validate replay path;
11. if `expectedSummaryPath` is present, validate summary path;
12. return `Ok`.

Path validation rules:

- call only lexical inspection helpers such as `path.lexically_normal()`;
- do not query the filesystem and do not create directories or files;
- reject any path whose generic string contains `/Users/kogaryu/iggy`;
- reject package paths containing `..` path components;
- reject package paths whose filename is not `package.iggy3d.toml`;
- reject replay paths whose extension sequence is not `.iggy3d.replay`;
- reject summary paths whose filename is empty;
- reject save/load paths whose filename is empty.

### No-Mutation Rule

Validation reads `config` only. It does not normalize in place, touch the
filesystem, load packages, create sessions, or mutate runtime state.

### Diagnostics

`appConfigStatusCode` must use a `switch` covering every
`AppConfigStatus` enumerator. Unknown enum values return `app.internal_error`.
Human-readable text is produced by CLI/front-end code, not by this source.

### Compute Cost

All functions are O(total path string bytes) and allocate only through standard
path/string operations.

### Tests

App/CLI tests must cover the validation order exactly: help bypasses package
path, invalid camera is reported before missing package for runtime modes,
missing package is reported before save/replay path checks, old iggy package
path maps to `app.invalid_package_path`, and validation leaves every input field
unchanged.

### Completion Criteria

The source provides deterministic app-config construction and validation with
stable statuses, no filesystem side effects, and no runtime authority.
