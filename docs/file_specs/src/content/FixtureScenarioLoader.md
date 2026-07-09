# File Spec

Files: `src/content/FixtureScenarioLoader.hpp`, `src/content/FixtureScenarioLoader.cpp`

Verified at: `33a36cc1`

## Owns

- Fixture scenario seed data model for players, entities, objectives, AI actors, guard anchors, runtime config, clock/camera defaults, and load result/status.
- Scenario text parser for supported fixture tables and keys.
- Required-key validation for parsed scenario sections.

## Does Not Own

- Package manifest parsing, semantic package validation, runtime `Session` creation, save/load, entity behavior systems, room assets, or renderer state.

## Reads

- Scenario text, table sections, defaults, player/entity/objective/AI/guard fields, numeric vectors, target action arrays, camera/clock/player/entity enums, and NPC behavior profile ids.

## Writes / Mutates

- Returns `ScenarioLoadResult` with `FixtureScenarioSeed` and diagnostics.
- Populates entity targeting, interaction definitions, combatant state, patrol waypoints, facing, and guard anchor data.
- Does not write files or mutate runtime state.

## Calls Out To / Wires Out To

- Uses `isValidNpcBehaviorProfileId(...)`.
- Parsed seeds are consumed by `PackageLoader`, package validation, runtime session creation, app save marker binding, and many unit tests.

## Called By / Entry Points

- `parseScenarioText(...)`.
- Grep proof: `rg -n "parseScenarioText|FixtureScenarioSeed|ScenarioEntitySeed|ScenarioAiActorSeed|ScenarioAiGuardAnchorSeed" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Scenario id and defaults are required.
- Players require slot, kind, and actor binding.
- Entities require stable name, kind, active/persistent flags, transform position, bounds, targetability, and target actions.
- Pickup interactions require item/objective/deactivation fields.
- Combatant fields are all-or-nothing behind `combatant=true`.
- AI actor behavior profile ids are parsed and validated at load time.
- Repeated AI actor `waypoint` keys append ordered patrol route points.

## Tests / Proof Commands

- `rg -n "parseScenarioText|package_loader_tests|stealth_garden_tests|stealth_tuning_readout_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "ScenarioLoadResult|scenario.invalid_enum|waypoint|ai_guard_anchors" tests/unit/package_loader_tests.cpp tests/unit/stealth_garden_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/content/PackageLoader.*` unless parser integration changes.
- `src/content/PackageValidator.*` unless semantic validation fields change.
- `src/runtime/session/Session.*` unless seed consumption changes.

## Update When

- Scenario seed fields, parser grammar, required-key rules, enum parsing, diagnostics, AI patrol/facing/guard fields, or default config parsing changes.

## Do Not Update When

- Only package path loading, package semantic validation, or runtime session behavior changes.
