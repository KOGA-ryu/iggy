# File Spec

Files:

- `src/runtime/session/SessionTick.hpp`
- `src/runtime/session/SessionTick.cpp`

Verified at: `f5f0a691`

## Owns

- `SessionTickInput`, `SessionTickResult`, `SessionTickStatus`, and `runSessionTick(...)`.
- Per-tick execution of accepted commands, retry resolution, runtime events, ability runtime ticking, objective outcome application, and clock advancement.
- Movement-system handoff with optional same-tick `precomputedSurfaceBake`.

## Does Not Own

- Command admission and pending queue selection.
- AI behavior command generation before tick execution.
- Physics bake creation or cross-tick cache ownership.
- Save/hash/golden policy or app presentation.

## Reads

- Mutable `SessionState`, accepted command records, optional collision surfaces, `forceStepWhilePaused`, `usePhysicsMovePlanner`, and optional `precomputedSurfaceBake`.
- World, config, abilities, combat, inventory, objectives, clock, and transient ability runtime.

## Writes / Mutates

- World transforms through movement execution.
- Inventory, objectives, combat, ability runtime, clock tick, transient events, metrics, sound events, and last movement result.
- Returned executed sequences and tick counters.

## Calls Out To / Wires Out To

- `executeMovement(...)` with `MovementSystemContext::precomputedSurfaceBake`.
- `executeInteraction(...)`, `applyAttack(...)`, `castAbility(...)`, `tickAbilityRuntime(...)`, and `tickAbilityState(...)`.

## Called By / Entry Points

- `Session.cpp` calls `runSessionTick(...)` from tick, step, and idle paths.
- Tests call `runSessionTick(...)` directly.
- Grep proof: `rg -n "\brunSessionTick\b|\bSessionTickInput\b|\bSessionTickResult\b|precomputedSurfaceBake" src tests/unit`.

## Invariants

- `precomputedSurfaceBake` is optional and per-call only.
- No bake is created or cached here; bake creation belongs upstream or fallback planner code.
- Paused ticks require explicit forced step.
- Per-tick sound bus is cleared before command execution.

## Tests / Proof Commands

- `rg -n "session_tick_tests|player_physics_move_planner_tests" cmake tests/unit`.
- `rg -n "bakeSessionTickSurfaceColliders|precomputedSurfaceBake" src/runtime/session src/runtime/movement src/runtime/player tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/session/Session.*` unless tick call shape changes.
- `src/runtime/movement/MovementSystem.*` unless movement context shape changes.
- `src/runtime/player/PlayerPhysicsMovePlanner.*` unless planner bake reuse changes.

## Update When

- Tick input/result packets, command execution sequencing, or precomputed bake handoff changes.
- Runtime events, transient sound bus, or tick status semantics change.

## Do Not Update When

- Session admission or AI behavior enqueueing changes before `runSessionTick(...)`.
