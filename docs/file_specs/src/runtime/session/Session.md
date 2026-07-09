# File Spec

Files:

- `src/runtime/session/Session.hpp`
- `src/runtime/session/Session.cpp`

Verified at: `f5f0a691`

## Owns

- `Session` public API, create/load/reset/finalize/tick entry points, and owned `SessionState`.
- Command admission handoff, pending accepted command collection, dirty/hash marking, and transient cleanup.
- Live per-tick AI sense mapping, NPC behavior enqueueing, and session-level reasoning graph carry.
- Same-tick collision-surface bake orchestration for AI occlusion and movement planner reuse.

## Does Not Own

- Pure AI kernels, physics queries, movement execution internals, save codec serialization, or app presentation.
- Cross-tick bake cache, surface revision scheme, or persistent physics storage.

## Reads

- Fixture scenario seeds, runtime config, command log, transient pending sequences, AI state, reasoning graph, and optional collision surfaces.
- `SegmentOcclusionVerdict` from AI occlusion and baked physics colliders from spatial surfaces.

## Writes / Mutates

- Owned `SessionState`: lifecycle, outcome, clock, command log cursor, baseline, current hash, transient queues/events/metrics, AI mirrors, and loaded state.
- NPC behavior commands through session command submission.

## Calls Out To / Wires Out To

- `runSessionTick(...)` with optional `precomputedSurfaceBake`.
- `bakePhysicsAabbCollidersFromSpatialSurfaces(...)` once per tick path when collision surfaces are present.
- `segmentOcclusion(...)`, `reasoningSegmentBlocked(...)`, `planRoute(...)`, and runtime AI behavior/perception helpers.

## Called By / Entry Points

- `Session::create(...)`, `submitCommand(...)`, `tickWithOptions(...)`, `stepOneTickWithOptions(...)`, `runUntilIdleWithOptions(...)`, `replaceStateFromLoad(...)`.
- `SessionRunner.cpp`, app gameplay command execution, creative activation, and unit/smoke tests.
- Grep proof: `rg -n "bakeSessionTickSurfaceColliders|actorLineOfSightToTarget|soundHasBlockerBetween|perception\.perceived|precomputedSurfaceBake" src/runtime/session src/runtime/movement src/runtime/player tests/unit`.

## Invariants

- `Clear` maps to clear LOS; `Blocked` maps to blocked LOS; `Unknown` maps to unknown LOS.
- Sound blocker is false only on `Clear`; `Blocked` and `Unknown` apply wall-loss.
- R1 visual confirmation uses `perception.perceived` only.
- Debug mirrors such as `lastTargetHasLineOfSight` are observability, not detection truth.
- Same-tick bake is not retained across ticks and must not become save/hash truth.

## Tests / Proof Commands

- `rg -n "session_tick_tests|stealth_garden_tests|reasoning_route_tests" cmake tests/unit`.
- `rg -n "SegmentOcclusionVerdict|segmentOcclusion" src/runtime/ai src/runtime/session tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/session/SessionTick.*` unless tick input/output or bake reuse changes.
- `src/runtime/ai/*` and `src/runtime/physics/*` unless kernel contracts change.
- `src/runtime/save/*`, app/window/render files, and golden fixtures unless persistence or presentation contracts change.

## Update When

- Session tick orchestration, AI sense mapping, bake reuse, transient reset, load validation, or hash/summary boundaries change.
- A transient mirror becomes save/hash truth, or the reverse.

## Do Not Update When

- Pure AI, physics, movement, or ability internals change without changing the session contract.
