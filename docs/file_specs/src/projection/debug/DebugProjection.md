# File Spec

Files: `src/projection/debug/DebugProjection.hpp`, `src/projection/debug/DebugProjection.cpp`

Verified at: `3dab1aa4`

## Owns

- Debug projection packets: `DebugProjectionItem`, `DebugProjectionResult`, configs, and append functions.
- Conversion of session/runtime/NPC/physics debug facts into app-facing debug items and HUD strings.
- Player physics debug AABB/contact projection caps.

## Does Not Own

- Runtime simulation, AI perception truth, physics query truth, or render draw submission.
- Product HUD layout beyond plain line payloads.
- Save/hash state.

## Reads

- `SessionState`, `RuntimeDebugSnapshot`, `NpcBehaviorDebugSnapshot`, `PhysicsDebugSnapshot`, physics AABB colliders, and motor hits.
- AI/debug status codes and row mirrors.

## Writes / Mutates

- `DebugProjectionResult` vectors and HUD line arrays supplied by caller.
- No runtime state mutation.

## Calls Out To / Wires Out To

- Uses status/name helpers from runtime snapshots and AI state.
- Produces `DebugProjectionResult` consumed by app view/render bridge and tests.

## Called By / Entry Points

- `buildDebugProjection(...)`, `appendRuntimeDebugSnapshot(...)`, `appendNpcBehaviorDebugSnapshot(...)`, `appendPhysicsDebugSnapshot(...)`, `appendPlayerPhysicsMovePlannerDebugProjection(...)`.
- `ProjectionRefresh.cpp` and tests feed these results toward frame input.
- Grep proof: `rg -n "appendNpcBehaviorDebugSnapshot|DebugProjectionResult|DebugProjectionKind|buildDebugProjection" src tests cmake CMakeLists.txt`.

## Invariants

- Projection mirrors runtime facts; it does not make gameplay decisions.
- Disabled or non-ready snapshots are ignored by append functions.
- Existing HUD line arrays are preserved unless a function intentionally owns that line family.
- Physics geometry caps prevent unbounded debug item emission.

## Tests / Proof Commands

- `rg -n "projection_tests|runtime_debug_snapshot_tests|product_physics_debug_hud_tests|product_npc_behavior_debug_hud_tests" cmake tests/unit`.
- `rg -n "PhysicsAabb|PhysicsContactNormal|NpcBehavior" tests/unit/projection_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/NpcBehaviorDebugSnapshot.*` unless snapshot shape changes.
- `src/projection/scene/*` unless scene geometry projection changes.
- `src/render/*` unless render consumes new debug item kinds.

## Update When

- Debug item kinds, HUD line payloads, append status gates, caps, or input snapshot contracts change.

## Do Not Update When

- Runtime internals change without changing projected debug facts.
