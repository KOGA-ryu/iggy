# K4d1 - Content Scenario Control And AI Seed Values

## Status

READY. K4 batch step 4 of 6. Claim only after K4c is committed and in `done/`.
Authority:
docs/creative_mode/builder_tasks/blocked/K4-department-dependency-dag-plan.md.

On green completion, commit this card, move it to `done/`, and continue to K4d2
without waiting for Reviewer. A STOP pauses the whole K4 batch.

## Goal

Remove the AiState, CameraState, ClockState, and PlayerSlot dependencies from
the content scenario schema by introducing content-owned control/AI seed values
and one fail-closed runtime conversion owner. Leave entity, targeting,
interaction, and combat seed migration for K4d2.

## Scope

Add:

- src/content/ScenarioSeed.hpp
- src/runtime/session/ScenarioSeedConversion.hpp
- src/runtime/session/ScenarioSeedConversion.cpp
- tests/unit/scenario_seed_conversion_tests.cpp

Update only:

- CMakeLists.txt and cmake/iggy3d_tests.cmake
- src/content/FixtureScenarioLoader.hpp/.cpp
- src/content/PackageValidator.cpp
- src/runtime/session/Session.cpp
- src/app/iggy3d/world/PackageSessionSeed.cpp
- src/app/iggy3d/creative/CreativeBlankStageSession.cpp
- focused tests named below

Do not edit Session.hpp, RoomMarkerBinding.cpp, PackageSessionSeed.hpp, entity
seed field types, policy files, graph-tool files, save/hash/replay, receipts, or
goldens.

## Required Content Values

ScenarioSeed.hpp initially owns only the lower authored values needed by this
slice:

- ScenarioPlayerSlotId plus an invalid sentinel
- ScenarioPlayerSlotKind
- ScenarioClockMode
- ScenarioCameraMode
- ScenarioPatrolMode

The public seed structs remain temporarily declared in
FixtureScenarioLoader.hpp, but their player and objective slot fields, player
kind, initial clock, camera, and patrol fields use these content-owned values.
FixtureScenarioLoader.hpp includes ScenarioSeed.hpp and removes exactly these
runtime headers:

- runtime/ai/AiState.hpp
- runtime/camera/CameraState.hpp
- runtime/clock/ClockState.hpp
- runtime/player/PlayerSlot.hpp

CombatState.hpp and EntityState.hpp remain until K4d2. Do not move the complete
seed structs early.

## Required Runtime Conversion

ScenarioSeedConversion.hpp/.cpp owns exhaustive Result<T> conversion for:

- ScenarioPlayerSlotKind -> PlayerSlotKind
- ScenarioClockMode -> ClockMode
- ScenarioCameraMode -> CameraMode
- ScenarioPatrolMode -> PatrolMode

Every switch fails closed for an out-of-range cast value. Do not use
static_cast between content and runtime enums.

- Session.cpp delegates all four mappings to this owner.
- PackageSessionSeed.cpp constructs ScenarioPlayerSlotKind and drops only its
  direct runtime/player/PlayerSlot.hpp include in this slice.
- CreativeBlankStageSession.cpp includes content/ScenarioSeed.hpp directly and
  constructs ScenarioPlayerSlotKind.
- PackageValidator.cpp compares against the content player kind.
- No entity/interaction/combat conversion function lands yet.

## Intermediate Graph Gate

After K4a-K4d1, tool output must be exactly:

- 324 cross-department edges
- 17 directed pairs and 16 unordered pairs
- the only SCC is content/runtime
- content -> runtime has exactly 2 edges: CombatState.hpp and EntityState.hpp
- runtime -> content has 9 edges
- app -> content has 28 edges
- app -> runtime has 100 edges

Reconcile the full pair table before reporting success. Do not publish policy
from this intermediate graph.

## Focused Verification

    cmake -S . -B build
    cmake --build build --target package_loader_tests product_package_session_seed_tests product_saved_room_marker_binding_tests scenario_seed_conversion_tests session_state_tests session_tick_tests session_runner_tests ability_command_tests save_load_tests
    ctest --test-dir build -R '^(package_loader_tests|product_package_session_seed_tests|product_saved_room_marker_binding_tests|scenario_seed_conversion_tests|session_state_tests|session_tick_tests|session_runner_tests|ability_command_tests|save_load_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --format json
    rg -n '#include[[:space:]]+[<"]runtime/(ai/AiState|camera/CameraState|clock/ClockState|player/PlayerSlot)\.hpp' src/content/FixtureScenarioLoader.hpp
    rg -n 'static_cast.*(ScenarioPlayerSlot|ScenarioClock|ScenarioCamera|ScenarioPatrol)' src/content src/runtime/session
    git diff --check

Both greps must return no match.

## Stop Conditions

- The intermediate graph differs from 324/17/16 or has an SCC other than
  content/runtime.
- A conversion requires numeric enum equivalence or static_cast.
- This slice requires entity, targeting, interaction, combat, Session.hpp,
  RoomMarkerBinding, persistence, receipt, or golden changes.
- A second control/AI mapping table remains in Session.cpp.
- A non-listed production caller must change.

## Completion Brief

Append the four migrated value families, conversion and invalid-enum pins,
exact intermediate graph rows, focused tests, and untouched-scope evidence. On
success, commit and continue to K4d2 without per-slice review.
