# File Spec

Files: `src/content/PackageLoader.hpp`, `src/content/PackageLoader.cpp`

Verified at: `33a36cc1`

## Owns

- Package manifest text parsing, scenario text parsing handoff, package file reading, relative path validation, asset file loading, and package load result assembly.
- `PackageLoadStatus`, `PackageLoadRequest`, and `PackageLoadResult`.

## Does Not Own

- Scenario parser internals, package semantic validation, room/mesh/material parser internals, package session seed conversion, save/load, renderer resources, or app launch policy.

## Reads

- Package text, scenario text, package directory, package and asset files, manifest sections, and asset paths.

## Writes / Mutates

- Returns `PackageLoadResult` with manifest, scenario seed, room assets, mesh library, material library, and diagnostics.
- Reads files from disk in `loadPackage(...)`; does not write files.

## Calls Out To / Wires Out To

- `parseScenarioText(...)`.
- `parseRoomAssetText(...)`.
- `parseMeshAssetText(...)`.
- `parseMaterialAssetText(...)`.
- Diagnostics from `core/diagnostics`.

## Called By / Entry Points

- `parsePackageText(...)`.
- `loadPackage(...)`.
- Product world launch, session tests, save tests, rendering smokes, and runtime demos use loaded packages.
- Grep proof: `rg -n "loadPackage|parsePackageText|PackageLoadResult|PackageLoadStatus" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Package and asset paths must be relative, must not contain parent components, and must not contain old legacy workspace prefixes.
- Scenario path must name `scenario.iggy3d.toml`.
- Scenario diagnostics from parser failures get a display path when needed.
- Asset file type dispatch is by path text containing `.room.`, `.meshes.`, or `.materials.`.
- Loader assembles content packets only; validation and runtime creation are separate steps.

## Tests / Proof Commands

- `rg -n "package_loader_tests|complete_runtime_demo_tests|room_asset_loader_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "parsePackageText|loadPackage|package.invalid_path|PackageLoadStatus" tests/unit/package_loader_tests.cpp tests/unit/room_asset_loader_tests.cpp tests/acceptance/complete_runtime_demo_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/content/FixtureScenarioLoader.*` unless scenario parser contract changes.
- `src/content/PackageValidator.*` unless validation moves into loading.
- `src/app/iggy3d/world/PackageSessionSeed.*` unless loaded package consumption changes.

## Update When

- Package parse grammar, path policy, asset dispatch, loaded result contents, diagnostics mapping, or file read behavior changes.

## Do Not Update When

- Only package validation, session seed construction, or runtime/app launch behavior changes.
