# File Spec

Files: `src/app/iggy3d/gameplay/ScriptedDriver.hpp`, `src/app/iggy3d/gameplay/ScriptedDriver.cpp`

Verified at: `a65a2f58`

## Owns

- No-window scripted gameplay smoke driver for approaching and attacking a target.
- Simple scripted approach loop using target/reach queries and movement action injection.
- Final scripted attack action injection and smoke proof flag.

## Does Not Own

- General gameplay automation, tape execution, target/query kernels, command admission, movement planning, save/load, or UI routing.
- Production player input behavior.

## Reads

- Optional active `Session`, product window state, player actor, target query result, reach query result, active room collision surfaces, and gameplay command proof state.

## Writes / Mutates

- Sets missing-session status when no active session exists.
- Marks `scriptedGameplaySmoke`, target discovery, reach gate, and applies scripted movement/attack actions through gameplay controller.
- Mutates session/window indirectly through `applyProductGameplayActions(...)`.

## Calls Out To / Wires Out To

- `queryTarget(...)` and `queryReach(...)`.
- `rejectionReasonForReach(...)`.
- `applyProductGameplayActions(...)`.
- `productActiveRoomCollisionSurfaces(activeRoomCollision(window))`.

## Called By / Entry Points

- `runScriptedProductGameplaySmoke(...)`.
- App kernel no-window scripted gameplay path.
- Grep proof: `rg -n "runScriptedProductGameplaySmoke|scriptedGameplaySmoke|scripted-gameplay-smoke" src tests/smoke`.

## Invariants

- Missing active session must not crash; it reports `missing_session`.
- Approach loop is bounded.
- Scripted movement stops if the gameplay command is not accepted.
- This is a smoke driver only; it must not become a general gameplay AI or input router.

## Tests / Proof Commands

- `rg -n "product_ascii_package_smoke|product_menu_usefulness_smoke|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests/smoke`.
- `rg -n "scripted_gameplay_smoke|gameplay_input_source.*scripted|target_discovered" tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/TapeRunner.*` unless switching scripted smoke to tape playback.
- `src/app/iggy3d/gameplay/Controller.*` unless gameplay action entrypoint changes.
- `src/runtime/targeting/*` unless target/reach query contracts change.

## Update When

- Scripted smoke approach/attack flow, proof fields, target/reach query use, bounded loop policy, or no-session behavior changes.

## Do Not Update When

- Only live input, gameplay tape runner, target query internals, or render receipts change without scripted-driver contract changes.
