# File Spec

File: `src/runtime/save/SaveEnvelope.hpp`

Verified at: `9a81866f`

## Owns

- Durable save envelope schema structs and version constants.
- Metadata, session, world, authored-room, creative-document, player, clock, camera, ability, command-log, inventory, combat, AI, and objective save records.
- Field defaults used when old or partial save sections decode successfully.

## Does Not Own

- Text encoding/decoding, compatibility validation, runtime state replacement, filesystem paths, or product save selection.
- Creative document mutation rules or authored-room editing behavior.
- State hash calculation.

## Reads

- Runtime public state types used as durable record field types, including entity IDs, transforms, AI enums, command records, inventory/combat/objective types, and session lifecycle enums.

## Writes / Mutates

- No runtime mutation. This header defines data packets consumed by codec/load/store/product bridge code.

## Calls Out To / Wires Out To

- `SaveCodec.*` encodes and decodes these records.
- `SaveLoad.*` maps runtime `SessionState` to/from this envelope.
- `SaveFileStore.*` persists envelopes and projects record metadata into file records.
- App save bridge and creative document save-section code consume `SaveCreativeDocumentSection` and metadata.

## Called By / Entry Points

- Included by save codec, save load, file store, app save bridge, creative document save tests, and save section tests.
- Grep proof: `rg -n "SaveEnvelope|SaveCreativeDocumentSection|SaveAuthoredRoomSection" src/runtime/save src/app/iggy3d tests/unit`.

## Invariants

- Version constants and default field values are part of the compatibility contract.
- New durable runtime fields require coordinated updates to envelope, codec, load/save mapping, hash rules when applicable, and tests.
- Off-save or transient runtime state must not be added here without an explicit persistence decision.
- Creative document and product session records share one envelope but must remain distinguishable by metadata/content-kind consumers.

## Tests / Proof Commands

- `rg -n "save_load_tests|save_creative_document_section_tests|creative_document_save_section_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "SaveEnvelope|SaveCreativeDocumentSection|SaveAuthoredRoomSection" src/runtime/save tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/session/SessionState.hpp` unless durable runtime state changes.
- `src/app/iggy3d/creative/document/*` unless creative save-section fields change.
- `src/content/authoring/*` unless authored-room persistence fields change.

## Update When

- A save record, section, default, enum persistence field, or schema version changes.
- A runtime field moves into or out of durable save truth.

## Do Not Update When

- Only file I/O, UI labels, catalog sorting, or runtime-only transient mirrors change.
