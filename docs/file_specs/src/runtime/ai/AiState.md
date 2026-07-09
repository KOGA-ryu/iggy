# File Spec

File: `src/runtime/ai/AiState.hpp`

Verified at: `3dab1aa4`

## Owns

- `AiBehaviorKind`, `AiIntentKind`, `AiPerceptionLos`, `PatrolMode`, and their string-name helpers.
- `AiActorState` durable AI actor state plus debug/perception mirrors.
- `AiState` as the session-owned vector of AI actors.

## Does Not Own

- NPC perception math, alert stepping, patrol stepping, investigation stepping, route search, or save codec implementation.
- Projection/debug row construction.
- Session tick orchestration.

## Reads

- `EntityId`, `Vec3`, and `GuardDecisionReceipt` type contracts.
- Nothing at runtime; this is a type/inline-helper header.

## Writes / Mutates

- No direct mutation.
- Callers mutate actor fields for behavior, alert, patrol, last-known target memory, route state, search choice, and observability mirrors.

## Calls Out To / Wires Out To

- Exposes AI state fields consumed by `SessionState`, save/load, state hash, AI kernels, debug snapshot, guard recon, and projection tests.

## Called By / Entry Points

- Included by `SessionState`, AI systems, save/load, state hash, debug snapshot, and unit tests.
- Grep proof: `rg -n "AiState|AiActorState|lastTargetHasLineOfSight|lastPerceived|lastLos|routeNodeIds|searchLastReceipt" src/runtime tests/unit cmake`.

## Invariants

- Enum value ordering is append-only where comments state saved/serialized stability.
- `lastTarget*`, `lastPerceived`, and `lastLos` are observability mirrors, not detection authority.
- Patrol, facing, alert, and last-known memory are durable where save/hash code includes them.
- Route/search fields marked transient-in-persistence must stay off save/hash unless deliberately promoted.

## Tests / Proof Commands

- `rg -n "save_load_tests|session_tick_tests|npc_behavior_debug_snapshot_tests|guard_recon_tests" cmake tests/unit`.
- `rg -n "AiActorState" src/runtime/save src/runtime/replay tests/unit/save_load_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/save/*` and `src/runtime/replay/StateHash.cpp` unless persistence boundaries change.
- `src/runtime/session/Session.cpp` unless live mirror writes change.
- `src/runtime/ai/NpcBehaviorDebugSnapshot.*` unless debug row shape changes.

## Update When

- AI actor fields, enum values, persistence boundaries, or mirror semantics change.

## Do Not Update When

- AI algorithms change without changing stored state or exposed mirrors.
