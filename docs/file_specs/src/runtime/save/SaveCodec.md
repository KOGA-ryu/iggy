# File Spec

Files: `src/runtime/save/SaveCodec.hpp`, `src/runtime/save/SaveCodec.cpp`

Verified at: `9a81866f`

## Owns

- Line-oriented text encoding and decoding for `SaveEnvelope`.
- Save codec status packets, diagnostics, duplicate/missing key detection, enum/name mappings, section ordering, and version gates.
- Save text size guard for codec input.

## Does Not Own

- Runtime state validity, save/load compatibility policy, filesystem durability, product catalog eligibility, or session replacement.
- State hash calculation except carrying encoded envelope hash fields.

## Reads

- `SaveEnvelope` records, state hash metadata fields, save schema version constants, and serialized text bytes.

## Writes / Mutates

- Produces encoded save text from an envelope.
- Produces decoded `SaveEnvelope` and diagnostic fields from input bytes.
- Does not mutate sessions, files, product catalog rows, or app/frontend state.

## Calls Out To / Wires Out To

- Used by `SaveLoad.*` for encoded save/load paths.
- Used by `SaveFileStore.*` for read/write validation and durable commit checks.
- Used by app save bridge through runtime save/file-store APIs.

## Called By / Entry Points

- `encodeSaveEnvelope(...)`.
- `decodeSaveEnvelope(...)`.
- Grep proof: `rg -n "encodeSaveEnvelope|decodeSaveEnvelope|SaveCodecStatus" src/runtime/save src/app/iggy3d tests/unit`.

## Invariants

- Encoded text must remain deterministic for equivalent envelopes.
- Unknown, duplicated, missing, invalid, or out-of-order fields must report explicit codec status/diagnostics.
- Version compatibility belongs at the codec envelope-read layer and the separate compatibility layer; do not bury product policy here.
- Large-save limits are codec input policy and must be changed deliberately with save-file tests.

## Tests / Proof Commands

- `rg -n "save_load_tests|save_creative_document_section_tests|creative_document_save_section_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "encodeSaveEnvelope|decodeSaveEnvelope|SaveCodecStatus|SaveTooLarge" src/runtime/save tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/save/SaveLoad.*` unless state mapping or load result behavior changes.
- `src/runtime/save/SaveFileStore.*` unless filesystem validation behavior changes.
- App save/menu files unless product-facing save behavior changes.

## Update When

- Save text format, enum string mappings, section readers/writers, diagnostics, version support, or save-size policy changes.

## Do Not Update When

- A caller changes catalog selection, delete/recover UI, or session tick behavior without changing save bytes.
