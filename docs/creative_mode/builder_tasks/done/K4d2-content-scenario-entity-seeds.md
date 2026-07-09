# K4d2 - Content Scenario Entity Seed Boundary

## Status

READY. K4 batch step 5 of 6. Claim only after K4d1 is committed and in `done/`.
Authority:
docs/creative_mode/builder_tasks/blocked/K4-department-dependency-dag-plan.md.

On green completion, commit this card, move it to `done/`, and continue to K4e
without waiting for Reviewer. A STOP pauses the whole K4 batch.

## Goal

Remove the final two content -> runtime edges by moving entity, targeting,
interaction, combatant, objective, and complete scenario seed ownership into
content/ScenarioSeed.hpp. Route Session and RoomMarkerBinding through the one
runtime conversion owner established by K4d1.

## Scope

Update only:

- src/content/ScenarioSeed.hpp
- src/runtime/session/ScenarioSeedConversion.hpp/.cpp
- tests/unit/scenario_seed_conversion_tests.cpp
- src/content/FixtureScenarioLoader.hpp/.cpp
- src/content/PackageValidator.cpp
- src/runtime/session/Session.hpp/.cpp
- src/app/iggy3d/save/RoomMarkerBinding.cpp
- src/app/iggy3d/world/PackageSessionSeed.hpp/.cpp
- src/app/iggy3d/creative/CreativeBlankStageSession.cpp
- focused tests named below

Do not edit CMake registration, the graph tool, policy JSON, SceneModel files,
runtime save/replay/hash schemas, receipts, goldens, or unrelated enums.

## Required Content Boundary

Expand ScenarioSeed.hpp with:

- ScenarioEntityKind
- ScenarioTargetAction
- ScenarioInteractionKind
- ScenarioInteractionEffectKind
- ScenarioTargetingSeed
- ScenarioInteractionSeed
- ScenarioCombatantSeed

ScenarioSeed.hpp also owns an inline content-side target-action membership
predicate over ScenarioTargetingSeed. PackageValidator uses that predicate; it
must not retain or duplicate runtime isTargetActionSupported behavior.

Move every public scenario seed struct from FixtureScenarioLoader.hpp into
ScenarioSeed.hpp while preserving public struct names and field names.
FixtureScenarioLoader.hpp becomes parser status/result/API plus one include of
ScenarioSeed.hpp.

ScenarioCombatantSeed contains authored combat facts only: no runtime EntityId
and no derived defeated field. Player/objective slots continue using the
content-owned alias from K4d1. FixtureScenarioLoader.hpp removes CombatState.hpp
and EntityState.hpp; no runtime include may remain anywhere below src/content/.

## Required Conversion Boundary

Expand ScenarioSeedConversion.hpp/.cpp with exhaustive fail-closed Result<T>
conversion for:

- scenario entity -> EntityState
- optional scenario combatant plus caller-supplied EntityId -> CombatantState
- scenario objective -> ObjectiveRecord

Targeting and interaction sub-conversion remain private to the .cpp. Do not use
numeric enum casts.

- Session.cpp delegates entity, combatant, objective, and the K4d1 control/AI
  mappings to ScenarioSeedConversion.
- RoomMarkerBinding.cpp delegates entity, combatant, and objective mapping to
  the same owner; delete its duplicate conversion helpers.
- Session.hpp includes content/ScenarioSeed.hpp instead of the parser header.
- PackageSessionSeed.hpp includes ScenarioSeed.hpp; its .cpp constructs content
  entity values and removes CombatState.hpp, InteractionDefinition.hpp, and
  EntityState.hpp.
- CreativeBlankStageSession.cpp constructs ScenarioEntityKind.

Runtime EntityState, PlayerSlot, ClockState, CameraState, CombatState, AiState,
save schema, replay schema, and hash coverage do not change.

## Final Production Graph Gate

After K4a-K4d2, tool output must be exactly:

- 320 cross-department edges
- 16 directed pairs and 16 unordered pairs
- zero SCCs
- no content -> runtime row
- no projection -> render row

Reconcile every pair row with the governing plan before reporting success. Do
not create or update the policy manifest in this card.

## Focused Verification

    cmake -S . -B build
    cmake --build build --target package_loader_tests npc_behavior_profile_tests product_npc_profile_assignment_tests product_package_session_seed_tests product_saved_room_marker_binding_tests scenario_seed_conversion_tests session_state_tests session_tick_tests session_runner_tests ability_command_tests save_load_tests
    ctest --test-dir build -R '^(package_loader_tests|npc_behavior_profile_tests|product_npc_profile_assignment_tests|product_package_session_seed_tests|product_saved_room_marker_binding_tests|scenario_seed_conversion_tests|session_state_tests|session_tick_tests|session_runner_tests|ability_command_tests|save_load_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --format json
    rg -n '#include[[:space:]]+[<"]runtime/' src/content
    rg -n 'static_cast.*(ScenarioEntity|ScenarioTarget|ScenarioInteraction)' src/content src/runtime/session
    rg -n 'objectiveStatusFromSeed|entityFromSeed|addCombatantFromSeed' src/runtime/session/Session.cpp src/app/iggy3d/save/RoomMarkerBinding.cpp
    git diff --check

All three greps must return no match.

## Stop Conditions

- Any seed field requires direct save/hash/replay schema treatment.
- Conversion depends on enum numeric equivalence or leaves a second mapping
  table in Session or RoomMarkerBinding.
- Any runtime include remains below src/content/.
- The graph differs from 320/16/16 or has an SCC.
- A non-listed production caller, policy file, receipt, golden, or app/window
  path must change.

## Completion Brief

Append every migrated seed family, shared-converter evidence, invalid-enum pins,
deleted duplicate helpers, exact final graph rows, focused tests, and untouched
save/hash/replay/receipt/golden evidence. On success, commit and continue to
K4e without per-slice review.
