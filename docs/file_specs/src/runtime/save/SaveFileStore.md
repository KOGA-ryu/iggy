# File Spec

Files: `src/runtime/save/SaveFileStore.hpp`, `src/runtime/save/SaveFileStore.cpp`

Verified at: `9a81866f`

## Owns

- Runtime save-file path policy, save id validation, active/deleted paths, snapshot sidecar paths, and save-id extraction.
- Listing, reading, writing, durable temp-write/validate/commit flows, delete, soft delete, and recover.
- File-store request/result packets and write/recover proof flags.

## Does Not Own

- Snapshot image production, product menu flow, product catalog selection, session gameplay behavior, or creative document mutation.
- Save text format internals beyond calling codec/load APIs for validation.

## Reads

- Filesystem roots/paths, save ids, attempt tokens, encoded save text, `SessionState`, authored room sections, explicit `SaveEnvelope`, decoded envelope metadata, and snapshot sidecar existence.

## Writes / Mutates

- Creates save roots and deleted-save roots.
- Writes temp save files, validates readback/decode, commits final save files, moves active/deleted save files, and moves snapshot sidecars when present.
- Does not mutate `Session` directly.

## Calls Out To / Wires Out To

- `saveSessionState(...)` and `encodeSaveEnvelope(...)` for write paths.
- `decodeSaveEnvelope(...)` for read/list/validation paths.
- Filesystem operations for create, rename, remove, read, and write.

## Called By / Entry Points

- `isValidSaveFileId(...)`, `saveFilePathForId(...)`, `listSaveFiles(...)`, `readSaveFile(...)`, `writeSessionSaveFileDurably(...)`, `writeSaveEnvelopeFileDurably(...)`, `softDeleteSaveFile(...)`, `recoverDeletedSaveFile(...)`.
- App save bridge, creative world service save paths, and save-file unit tests.
- Grep proof: `rg -n "writeSessionSaveFileDurably|writeSaveEnvelopeFileDurably|softDeleteSaveFile|recoverDeletedSaveFile|listSaveFiles|readSaveFile" src tests`.

## Invariants

- Save ids must not allow path traversal or platform separator escapes.
- Durable write must validate temp bytes before commit and final bytes after commit.
- Soft delete/recover must preserve existing targets and report conflicts instead of overwriting.
- Snapshot sidecars are moved if present but are not produced here.
- File I/O failures must surface as explicit result reasons/status flags.

## Tests / Proof Commands

- `rg -n "save_file_store_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "isValidSaveFileId|writeSessionSaveFileDurably|writeSaveEnvelopeFileDurably|softDeleteSaveFile|recoverDeletedSaveFile" tests/unit/save_file_store_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/save/SaveCodec.*` unless validation or encoded text behavior changes.
- `src/runtime/save/SaveLoad.*` unless session-state envelope construction changes.
- `src/app/iggy3d/save/SaveBridge.*` unless product-facing save/delete/recover orchestration changes.

## Update When

- File naming, durable write staging, validation, list/read semantics, soft-delete/recover behavior, snapshot sidecar handling, or result packets change.

## Do Not Update When

- Only product UI labels, save-slot presentation, or runtime session fields change without file-store contract changes.
