# File Spec

Files: `src/content/PackageValidator.hpp`, `src/content/PackageValidator.cpp`

Verified at: `33a36cc1`

## Owns

- Semantic validation of a parsed package manifest plus scenario seed.
- `PackageValidationStatus`, request/result packets, package schema checks, scenario/player/entity/objective checks, AI actor validation, guard anchor validation, and legacy dependency rejection.

## Does Not Own

- Text parsing, file loading, asset parsing, package session seed conversion, runtime session creation, gameplay rules beyond package fixture validity, or save/load.

## Reads

- `PackageManifest`, `FixtureScenarioSeed`, scenario entities/objectives/players/AI actor bindings/guard anchors, transform/bounds validity, target actions, interactions, and NPC behavior profile ids.

## Writes / Mutates

- Returns `PackageValidationResult` with status and diagnostics.
- Does not mutate manifest, scenario seed, files, app state, or runtime state.

## Calls Out To / Wires Out To

- Uses `isValidNpcBehaviorProfileId(...)`.
- Uses runtime state helpers for finite transform/bounds and target action support.

## Called By / Entry Points

- `validatePackage(...)`.
- Package loader tests exercise both generic package validation and guard/AI validation.
- Grep proof: `rg -n "validatePackage|PackageValidationStatus|PackageValidationRequest" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Schema versions must be supported and scenario path must match the expected fixture path.
- Stable entity names must be present and unique.
- Local player slot zero must bind to `player`, and a `player` entity must exist.
- Entity transforms and bounds must be finite and valid.
- AI actors must reference NPC entities and valid behavior profiles without duplicate bindings.
- Guard anchors must reference NPC actors and marker anchors with valid leash/return/tolerance radii.
- The gold-key objective/interaction fixture contract remains explicit here.

## Tests / Proof Commands

- `rg -n "validatePackage|PackageValidationStatus|guard" tests/unit/package_loader_tests.cpp`.
- `rg -n "package_loader_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/content/PackageLoader.*` unless parse/load result shape changes.
- `src/content/FixtureScenarioLoader.*` unless scenario seed fields change.
- `src/runtime/ai/NpcBehaviorProfile.*` unless profile id validation changes.

## Update When

- Package validation statuses, scenario semantic checks, AI/guard rules, legacy path rules, or diagnostics change.

## Do Not Update When

- Only text parsing or runtime session behavior changes without package validation contract changes.
