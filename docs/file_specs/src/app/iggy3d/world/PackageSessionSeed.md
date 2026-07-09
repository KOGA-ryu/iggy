# File Spec

Files: `src/app/iggy3d/world/PackageSessionSeed.hpp`, `src/app/iggy3d/world/PackageSessionSeed.cpp`

Verified at: `266066cb`

## Owns

- Product package-to-runtime session seed conversion.
- Synthesizing scenario seed entities/objectives from room anchors when package scenario entities are absent.
- NPC behavior profile assignment overlay and validation.
- Seed result counts and readiness/failure reason codes.

## Does Not Own

- Package loading or room asset parsing.
- Runtime `Session::create(...)`.
- NPC behavior execution.
- Save/load or active-room marker binding.

## Reads

- `PackageLoadResult`, scenario seed data, room anchors, and optional product NPC profile assignment table.
- Authored scenario AI actors when preserving authored routes/mode/facing while assigning profiles.

## Writes / Mutates

- Returns `ProductPackageSessionSeedResult` with `FixtureScenarioSeed` and counts.
- Mutates only local seed copies while building the result.

## Calls Out To / Wires Out To

- `validateProductNpcProfileAssignments(...)` and `resolveProductNpcProfileAssignment(...)`.
- Runtime seed packet types for players, entities, combatants, interactions, objectives, and AI actors.

## Called By / Entry Points

- Product session launch, saved room marker binding, ASCII room activation, gameplay tape tests, and product save/delete tests.
- Focused proof: `rg -n "buildProductPackageSessionSeed|ProductPackageSessionSeedResult|synthesizedFromRoomAnchors" src/app tests/unit`.

## Invariants

- Non-loaded package fails before seed synthesis.
- Authored scenario entities are used directly instead of anchor synthesis.
- Anchor synthesis requires a room and spawn anchor.
- NPC profile assignment must merge profile id over authored AI actor routes instead of rebuilding route data.
- Pickup and exit anchors generate matching objectives.

## Tests / Proof Commands

- `rg -n "product_package_session_seed_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "product_package_seed_ready|product_package_seed_missing_spawn_anchor|synthesizedFromRoomAnchors" tests/unit/product_package_session_seed_tests.cpp src/app/iggy3d/world`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/world/NpcProfileAssignment.*` unless profile assignment contracts change.
- `src/content/PackageLoader.*` unless package room/scenario seed contracts change.
- `src/runtime/session/*` unless session create seed input contracts change.

## Update When

- Anchor-to-entity/objective mapping, NPC profile assignment merge behavior, seed result fields, or seed failure codes change.

## Do Not Update When

- Only runtime AI behavior, package file parsing, or product launch UI changes without changing package seed construction.
