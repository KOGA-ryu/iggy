# File Spec

Files: `src/runtime/replay/StateHash.hpp`, `src/runtime/replay/StateHash.cpp`

Verified at: `9a81866f`

## Owns

- Deterministic hash calculation over durable `SessionState`.
- Stable hash formatting as fixed-width lowercase hexadecimal text.
- Hash membership for durable runtime state categories used by save/load and replay checks.

## Does Not Own

- Save envelope schema, text serialization, compatibility policy, runtime mutation, replay execution, or product receipts.
- Transient/debug mirror state unless explicitly promoted into durable hash truth.

## Reads

- Durable `SessionState` identity, lifecycle/config, world entities, players, clock, camera, abilities, command log, inventory, combat, AI, and objectives.

## Writes / Mutates

- No runtime mutation. Returns `StateHashValue` and formatted hash strings.

## Calls Out To / Wires Out To

- Used by `SaveLoad.*` to stamp and verify saved state hashes.
- Used by replay code to detect state divergence.
- Indirectly constrains save codec/load tests that compare restored state.

## Called By / Entry Points

- `computeStateHash(...)`.
- `formatStateHash(...)`.
- Grep proof: `rg -n "computeStateHash|formatStateHash|StateHashValue" src/runtime tests/unit`.

## Invariants

- Hash input ordering must be deterministic.
- Durable save/hash membership must stay aligned with `SaveEnvelope` and load mapping decisions.
- Float hashing is quantized by the stable hasher policy; do not change precision casually.
- Off-save debug/transient mirrors must remain excluded unless a persistence contract changes.

## Tests / Proof Commands

- `rg -n "save_load_tests|CommandReplayStatus::StateHashDiverged" cmake/iggy3d_tests.cmake src/runtime/replay tests/unit`.
- `rg -n "computeStateHash|formatStateHash" src/runtime tests/unit/save_load_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/save/SaveEnvelope.hpp` unless durable fields change.
- `src/runtime/save/SaveLoad.*` unless load/save hash verification changes.
- `src/runtime/replay/CommandReplay.*` unless replay divergence behavior changes.

## Update When

- Durable state membership, stable hash ordering, quantization, hash formatting, or save/replay hash semantics change.

## Do Not Update When

- UI receipts, debug-only mirrors, temporary metrics, or non-durable runtime caches change.
