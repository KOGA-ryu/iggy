# `src/content/PackageLoader.hpp`

Updated: 2026-06-20

Exact purpose: declare package/scenario load request, result, and status types
that convert TOML fixture files into content-owned seed data.

## Build Position

- priority rank: 33
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/content/PackageLoader.hpp
```

Required includes:

```cpp
#pragma once

#include <string>
#include <vector>

#include "core/diagnostics/Diagnostic.hpp"
#include "core/result/Result.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "content/PackageManifest.hpp"
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Status Enum

Declare:

```cpp
enum class PackageLoadStatus : std::uint8_t {
  Ok,
  MissingPackageFile,
  PackageReadFailed,
  ScenarioReadFailed,
  ParseError,
  UnsupportedKey,
  MissingRequiredKey,
  MissingScenarioId,
  InvalidNumber,
  InvalidEnum,
  InvalidPath,
};
```

## Required Types And API

Declare:

```cpp
struct PackageLoadRequest {
  std::string packagePath;
};

struct PackageLoadResult {
  PackageLoadStatus status = PackageLoadStatus::Ok;
  PackageManifest manifest;
  FixtureScenarioSeed scenario;
  std::vector<Diagnostic> diagnostics;
};

PackageLoadResult loadPackage(const PackageLoadRequest& request);
PackageLoadResult parsePackageText(
    const std::string& packageText,
    const std::string& scenarioText,
    const std::string& packageDirectory);
```

`parsePackageText` exists for unit tests and does not read files.

## Loader Boundary

Loader reads and parses content. It must not validate gameplay legality beyond
syntax/path sanity, create `SessionState`, allocate runtime ids outside seed
order, mutate runtime, or load renderer assets.

## Failure Behavior

- Missing/failed package file returns a non-Ok status with diagnostics.
- Unsupported TOML key returns `UnsupportedKey`.
- Missing required package/scenario key returns `MissingRequiredKey`.
- Missing `[scenario].id` in scenario text returns `MissingScenarioId`.
- Bad scenario numeric values return `InvalidNumber`.
- Unrecognized scenario enum strings return `InvalidEnum`.
- Package scenario path with absolute path or `..` returns `InvalidPath`.
- Scenario parse errors return `ParseError` or `ScenarioReadFailed`.
- `FixtureScenarioLoader` failures map exactly to the public package-load status:
  `ParseError -> ParseError`, `UnsupportedKey -> UnsupportedKey`,
  `MissingRequiredKey -> MissingRequiredKey`,
  `MissingScenarioId -> MissingScenarioId`, `InvalidNumber -> InvalidNumber`,
  `InvalidEnum -> InvalidEnum`, and `InvalidPath -> InvalidPath`.
- On non-Ok status, diagnostics and status are authoritative; callers must ignore
  `manifest` and `scenario` payload fields unless `status == Ok`.
- Non-Ok payload fields must not be passed to `PackageValidator` or session
  creation.

Required diagnostic codes:

- `package.missing_file`
- `package.read_failed`
- `package.unsupported_key`
- `package.missing_required_key`
- `package.invalid_path`
- `package.parse_error`
- `scenario.read_failed`
- `scenario.parse_error`
- `scenario.unsupported_key`
- `scenario.missing_required_key`
- `scenario.missing_id`
- `scenario.invalid_number`
- `scenario.invalid_enum`

## Tests

Package loader tests call `parsePackageText` for deterministic success/failure
cases and `loadPackage` for fixture-path smoke coverage.

## Completion Criteria

The header defines exact load API, statuses, and data handoff to validation.
