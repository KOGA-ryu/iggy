# File Spec

Files: `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`

Verified at: `0ba40cb9`

## Owns

- Receipt field emission for runtime session presence, gameplay activation, runtime state hash, scene/debug item counts, gameplay command proof, tick proof, movement proof, jump, reset, traversal, wall-run, and dash proof fields.
- Table-driven mapping from gameplay proof packets and movement proof packet to receipt keys.

## Does Not Own

- Runtime session execution.
- Movement planning or collision resolution.
- Gameplay command admission/execution.
- State hash calculation.
- Top-level receipt build order.

## Reads

- `ProductAppWindowState.gameplay`, viewport gameplay visibility, `ProductMovementProofPacket`, and runtime state hash passed by the receipt builder.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate gameplay, movement proof, or runtime state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `floatReceiptValue(...)` for numeric movement/jump/traversal/dash fields.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductGameplayRuntimeMovementFields(...)`.
- Focused proof: `rg -n "appendProductGameplayRuntimeMovementFields|runtime_state_hash|gameplay_movement_attempted|gameplay_dash_status" src/app tests`.

## Invariants

- Runtime and gameplay proof fields are read-only receipt output.
- Movement proof is supplied by `buildProductMovementProofPacket(...)` before this appender runs.
- Runtime state hash is passed in; this file must not compute it.
- Movement, jump, traversal, wall-run, and dash receipt facts stay in one runtime movement block.

## Tests / Proof Commands

- `rg -n "runtime_state_hash|gameplay_movement_attempted|gameplay_dash_status" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_gameplay_controls_smoke|product_gameplay_tape_smoke|product_ascii_authoring_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/MovementProof.*` unless movement proof fields change.
- `src/app/iggy3d/gameplay/Controller*.{hpp,cpp}` unless gameplay proof packets change.
- `src/runtime/session/*` unless runtime hash/state proof contracts change.

## Update When

- Gameplay runtime/movement receipt keys, source proof packet fields, or runtime movement receipt grouping changes.

## Do Not Update When

- Only movement math, collision internals, or runtime tick behavior changes without changing emitted receipt fields.
