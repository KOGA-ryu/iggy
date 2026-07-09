# File Spec

Files: `src/runtime/save/SaveLoad.hpp`, `src/runtime/save/SaveLoad.cpp`

Verified at: `9a81866f`

## Owns

- Runtime `SessionState` to `SaveEnvelope` mapping.
- Encoded save/load wrappers over `SaveCodec`.
- Compatibility gate, reference validation, command-log restore validation, hash verification, transient clearing, and session replacement handoff.
- `SaveStateResult`, `LoadStateResult`, and `SaveLoadStatus`.

## Does Not Own

- File paths, durable write/rename policy, soft delete/recover, product save catalog selection, or UI flow.
- Text codec internals, stable hash algorithm internals, or pure session tick behavior.

## Reads

- `SessionState`, `SaveEnvelope`, save compatibility request/result, encoded save text, command-log records, runtime references, and computed state hash.

## Writes / Mutates

- Builds save envelopes and encoded save text.
- Loads a candidate state into `Session` through `replaceStateFromLoad(...)` only after validation succeeds.
- Clears or reconstructs load-time transient state as part of candidate state preparation.

## Calls Out To / Wires Out To

- `computeStateHash(...)` and `formatStateHash(...)`.
- `encodeSaveEnvelope(...)` and `decodeSaveEnvelope(...)`.
- `evaluateSaveCompatibility(...)`.
- `Session::replaceStateFromLoad(...)`.

## Called By / Entry Points

- `saveSessionState(...)`.
- `saveSessionStateEncoded(...)`.
- `loadEnvelopeIntoSession(...)`.
- `loadEncodedSaveIntoSession(...)`.
- Runtime file store durable write/read paths and app save bridge load paths.
- Grep proof: `rg -n "saveSessionState|saveSessionStateEncoded|loadEnvelopeIntoSession|loadEncodedSaveIntoSession" src tests`.

## Invariants

- Load must fail before session replacement when compatibility, references, command log, or hash validation fails.
- Computed hash must match both numeric and hex metadata fields.
- Runtime load compatibility and product catalog eligibility stay separate.
- Transient/runtime mirrors restored during load are not automatically durable truth unless represented in `SaveEnvelope` and `StateHash`.

## Tests / Proof Commands

- `rg -n "save_load_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "loadEnvelopeIntoSession|HashMismatch|InvalidReference|InvalidCommandLog" tests/unit/save_load_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/save/SaveCodec.*` unless save bytes change.
- `src/runtime/replay/StateHash.*` unless durable hash membership changes.
- `src/runtime/session/Session.*` unless load replacement or state ownership changes.

## Update When

- Session-to-envelope mapping, load validation, replacement semantics, compatibility gate behavior, command-log restore, or hash checks change.

## Do Not Update When

- Only save file movement, UI save-slot labels, delete/recover flow, or catalog sorting changes.
