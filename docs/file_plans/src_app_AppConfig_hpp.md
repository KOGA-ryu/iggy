# `src/app/AppConfig.hpp`

Updated: 2026-06-20

Exact purpose: declare app-facing configuration produced by CLI parsing without leaking CLI concepts into runtime.

## Build Position

- priority rank: 28
- tier: Tier 2: Content And Configuration
- module: `app support`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- app/CLI tests cover default construction, status-code mapping, path
  validation, camera validation, help/version bypass, and conflicting modes;
- tests must not touch the filesystem beyond lexical path strings;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/app/AppConfig.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Repo Path
`src/app/AppConfig.hpp`

### Required Includes And Namespace

```cpp
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "config/RuntimeConfig.hpp"
#include "runtime/camera/CameraState.hpp"
```

All declarations live in `namespace iggy3d`.

### Required Enums

```cpp
enum class AppMode : std::uint8_t {
  HeadlessDemo,
  ValidatePackage,
  Replay,
  Help,
  Version,
};

enum class AppConfigStatus : std::uint8_t {
  Ok,
  MissingFixturePath,
  InvalidPackagePath,
  InvalidSavePath,
  InvalidLoadPath,
  InvalidReplayPath,
  InvalidExpectedSummaryPath,
  InvalidCameraMode,
  ConflictingMode,
  UnknownOption,
  MissingOptionValue,
};
```

Status semantics:

- `MissingFixturePath`: a mode that needs a package has empty `packagePath`.
- `InvalidPackagePath`: `packagePath` is absolute old-iggy path, contains `..`, or has the wrong filename.
- `InvalidSavePath`: save output path is empty when requested or points at a directory marker.
- `InvalidLoadPath`: load path is empty when requested.
- `InvalidReplayPath`: replay mode has empty replay path or a non-`.iggy3d.replay` extension.
- `InvalidExpectedSummaryPath`: expected summary path is empty when supplied by an option needing a value.
- `InvalidCameraMode`: requested realtime camera is not `FirstPerson` or `ThirdPerson`.
- `ConflictingMode`: more than one primary mode was selected.
- `UnknownOption` and `MissingOptionValue` are surfaced from `CliParser` without runtime creation.

### Required Struct

```cpp
struct AppConfig {
  AppMode mode = AppMode::HeadlessDemo;
  std::filesystem::path packagePath = "fixtures/demos/first_room/package.iggy3d.toml";
  std::filesystem::path savePath;
  std::filesystem::path loadPath;
  std::filesystem::path replayPath;
  std::filesystem::path expectedSummaryPath;
  RuntimeConfig runtimeConfig;
  CameraMode requestedRealtimeCamera = CameraMode::ThirdPerson;
  bool strict = false;
  bool verbose = false;
  bool printHelp = false;
  bool printVersion = false;
};
```

Default behavior is exact: absent CLI package input runs the first-room package at
`fixtures/demos/first_room/package.iggy3d.toml`. `ValidatePackage` and `Replay`
may override that path. Help and version modes do not require a package.

### Required API

```cpp
AppConfig makeDefaultAppConfig();
AppConfigStatus validateAppConfig(const AppConfig& config);
const char* appConfigStatusCode(AppConfigStatus status);
```

`appConfigStatusCode` returns stable lowercase diagnostic strings:

- `app.ok`
- `app.missing_fixture_path`
- `app.invalid_package_path`
- `app.invalid_save_path`
- `app.invalid_load_path`
- `app.invalid_replay_path`
- `app.invalid_expected_summary_path`
- `app.invalid_camera_mode`
- `app.conflicting_mode`
- `app.unknown_option`
- `app.missing_option_value`

### Invariants

- App config owns process configuration only; it is not save truth.
- App config selects a package path and startup defaults, while package
  validation and session creation own runtime seed acceptance.
- No field stores raw input events, socket identity, renderer handles, or
  runtime entity references.
- Path validation rejects old `/Users/kogaryu/iggy` dependencies before content
  loading.

### Dependencies

Allowed: standard filesystem/string headers, `RuntimeConfig`, and `CameraMode`.
Forbidden: runtime session/world mutation, content parsers, renderer, projection,
network, tests, and old iggy headers.

### Compute Cost

Validation is O(path string bytes). Field reads after startup are O(1).

### Tests

App/CLI tests must assert default first-room package path, help/version bypass
package validation, replay mode requires replay path, invalid old-iggy package
path returns `InvalidPackagePath`, invalid camera returns `InvalidCameraMode`,
and conflicting primary modes return `ConflictingMode`.

### Completion Criteria

The header locks app configuration fields, defaults, statuses, diagnostic codes,
and validation API without granting app code authority over runtime state.
