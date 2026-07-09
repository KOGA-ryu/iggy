# File Spec

Files: `src/runtime/ai/GuardRecon.hpp`, `src/runtime/ai/GuardRecon.cpp`

Verified at: `9cd3a6b5`

## Owns

- Pure projection of one guard's durable AI state plus caller-provided world position into `GuardReconObservation`.
- Scoutable guard facts: position, facing, alert level, behavior string, patrol facts, last-known target memory, and watched reasoning node lookup.

## Does Not Own

- Recon packet serialization, notebook UI, production caller scheduling, or durable storage.
- Entity position lookup; caller supplies guard position.
- Reasoning graph construction or guard decision scoring.

## Reads

- `AiActorState`, caller-supplied guard position, and `ReasoningGraph`.
- Patrol waypoints/index/mode, last-known target fields, current search choice id, behavior, facing, and alert level.

## Writes / Mutates

- No external state.
- Returns `GuardReconObservation`.

## Calls Out To / Wires Out To

- Uses `aiBehaviorKindName(...)` for behavior projection.
- Looks up the watched reasoning node by id in the provided graph.
- `ReconIntel.*` consumes `GuardReconObservation` packets.

## Called By / Entry Points

- `guard_recon_tests` calls `projectGuardRecon(...)` directly.
- `ReconIntel.*` includes and aggregates guard recon observations.
- Grep proof: `rg -n "projectGuardRecon|GuardReconObservation|ReconIntel" src tests cmake`.

## Invariants

- Projection is deterministic and pure.
- Guard position is never fabricated or read from an entity store here.
- Unresolved/empty/stale watched node ids leave `hasWatchedNode=false`.
- Patrol target is populated only when the current index is valid.

## Tests / Proof Commands

- `rg -n "guard_recon_tests|recon_intel_tests" cmake tests`.
- `rg -n "projectGuardRecon|captureReconIntel" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/ReconIntel.*` unless recon packet aggregation changes.
- `src/runtime/ai/AiState.*` unless guard state fields change.
- `src/runtime/ai/ReasoningGraph.*` unless node identity/kind semantics change.

## Update When

- Guard recon observation fields, projection rules, behavior naming, patrol projection, last-known memory projection, or watched-node lookup changes.

## Do Not Update When

- Notebook UI or serialization changes without changing this projection packet.
