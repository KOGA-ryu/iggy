# `src/runtime/save/SaveCodec.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic save envelope encoding and decoding.

## Build Position

- priority rank: 114
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/save`
- file kind: `header`

## Ownership

This file owns:

- deterministic tagged text codec rules
- field ordering
- round-trip parse diagnostics
- escaping rules

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- first complete build uses a first-party deterministic tagged text format
- all sections written in stable order

## Semantics

- codec does not create gameplay effects
- decode returns envelope or diagnostics
- unknown keys are rejected in the first complete build

## Detailed Design Contract

Declare a deterministic first-party tagged text codec. No external serialization
dependency and no binary save codec are allowed in the first complete build.
Header owns pure encode/decode APIs:

- `SaveEncodeResult encodeSaveEnvelope(const SaveEnvelope& envelope)`;
- `SaveDecodeResult decodeSaveEnvelope(std::string_view bytes)`.

Declare codec statuses: `Ok`, `EncodeFailed`, `DecodeFailed`, `DuplicateKey`,
`MissingField`, `UnsupportedVersion`, `InvalidEnum`, `InvalidNumber`,
`InvalidId`, `InvalidSequence`, `InvalidSectionOrder`, `SaveTooLarge`, and
`UnknownKey`.

Declare exact result values:

```cpp
struct SaveEncodeResult {
  SaveCodecStatus status = SaveCodecStatus::Ok;
  std::string encodedText;
  StateHashValue savedStateHash = 0;
  std::string diagnosticSection;
  std::string diagnosticKey;
  std::uint32_t diagnosticLine = 0;
  std::uint32_t diagnosticOffset = 0;
  std::string diagnostic;
};

struct SaveDecodeResult {
  SaveCodecStatus status = SaveCodecStatus::Ok;
  SaveEnvelope envelope;
  std::string diagnosticSection;
  std::string diagnosticKey;
  std::uint32_t diagnosticLine = 0;
  std::uint32_t diagnosticOffset = 0;
  std::size_t duplicateOrMissingIndex = 0;
  std::string diagnostic;
};
```

Result validity:

- `SaveEncodeResult::status == Ok`: `encodedText` is complete and
  `savedStateHash` mirrors `envelope.metadata.savedStateHash`;
- non-`Ok` encode results leave `encodedText` ignored and use diagnostic fields
  to identify the failed section/key/line/offset;
- `SaveDecodeResult::status == Ok`: `envelope` is valid codec output;
- non-`Ok` decode results leave `envelope` ignored and use diagnostic fields,
  including `duplicateOrMissingIndex` for repeated-field duplicate/missing
  context.

Text rules:

- UTF-8 text;
- LF newlines;
- no trailing spaces;
- first line exactly `iggy3d.save_envelope.v1`;
- one ordered `key=value` field per following line;
- repeated fields use deterministic indexes.
- keys are ASCII `[A-Za-z0-9_.]+`;
- bool values are `true` or `false`;
- enum values are their exact C++ enumerator names without enum-class prefix;
- integers are base-10 with no leading plus sign;
- floats are finite decimal values using the same precision as state hash
  quantization;
- empty strings encode as an empty value after `=`;
- string values percent-escape `%`, LF, CR, and `=` as `%25`, `%0A`, `%0D`, and
  `%3D`; decode rejects malformed percent escapes.

Required top-level section order after the header:

1. `metadata`;
2. `session`;
3. `world`;
4. `players`;
5. `clock`;
6. `camera`;
7. `commandLog`;
8. `inventory`;
9. `combat`;
10. `ai`;
11. `objectives`.

Required indexed key style examples:

- `world.entity.0.id=...`;
- `world.entity.0.stableName=...`;
- `players.slot.0.slotId=...`;
- `commandLog.record.0.kind=...`;
- `commandLog.record.0.sequence=...`;
- `inventory.player.0.stack.0.itemId=...`;
- `objectives.record.0.status=...`.

Required key order within each section:

- metadata: `metadata.schemaVersion`,
  `metadata.minimumReadableSchemaVersion`, `metadata.runtimeSaveVersion`,
  `metadata.packageId`, `metadata.scenarioId`, `metadata.createdByToolId`,
  `metadata.savedStateHash`, `metadata.savedStateHashHex`;
- session: `session.lifecycle`, `session.outcome`, `session.currentTick`,
  `session.nextCommandId`, `session.sessionSeed`, `session.sessionSchemaVersion`,
  `session.packageId`, `session.scenarioId`;
- world: `world.nextEntityId`, `world.entity.count`, then for each entity:
  `id`, `stableName`, `kind`, `transform.position`, `transform.rotation`,
  `transform.scale`, `localBounds.min`, `localBounds.max`, `active`,
  `persistent`, `targetable`, `targetAction.count`, indexed
  `targetAction.N`, `interactionKind`, `interactionPrimaryEffect`,
  `interactionItemId`, `interactionItemCount`, `interactionObjectiveId`,
  `interactionRepeatable`, `interactionDeactivateTargetOnSuccess`;
- players: `players.slot.count`, then each slot `slotId`, `kind`,
  `controlledActor`, `stableName`;
- clock: `clock.mode`, `clock.previousUnpausedMode`,
  `clock.previousUnpausedTimeScale`, `clock.tickIndex`,
  `clock.fixedTickRateHz`, `clock.timeScale`;
- camera: `camera.activeMode`, `camera.previousRealtimeMode`,
  `camera.targetEntity`, `camera.targetPoint`, `camera.targetHasPoint`,
  `camera.yawDegrees`, `camera.pitchDegrees`, `camera.orbitDistance`;
- commandLog: `commandLog.resetPolicy`, `commandLog.nextSequence`,
  `commandLog.epoch`, `commandLog.record.count`, then each record `commandId`,
  `sequence`, `kind`, `source`, `playerSlot`, `actor`, `hasTargetEntity`,
  `targetEntity`, `hasTargetPoint`, `targetPoint`, `retrySourceCommandId`,
  `issuedTick`, `scheduledTick`, `admission`, `rejection`;
- inventory: `inventory.player.count`, then each player `playerSlot`,
  `stack.count`, and indexed stack `itemId`, `count`;
- combat: `combat.combatant.count`, then each combatant `entity`,
  `factionId`, `hitPoints`, `maxHitPoints`, `defeated`;
- ai: `ai.actor.count`, then each actor `actor`, `nextDecisionTick`,
  `deterministicPolicy`, `enabled`;
- objectives: `objectives.record.count`, then each objective `objectiveId`,
  `status`, `conditionKind`, `conditionPlayerSlot`, `conditionItemId`,
  `conditionItemCount`.

Decode rejects duplicate keys, missing required keys, unsupported version,
invalid enum, invalid number, invalid id/sequence, invalid section order,
`SaveTooLarge`, and unknown keys. Save payload hash mismatch belongs
to `SaveLoad` after envelope-to-state mapping; the codec does not own an
internal checksum in the first complete build.

Invariants:

- codec owns bytes-to-value mapping only, not file IO and not session mutation;
- encoded fields are emitted in the exact section order from `SaveEnvelope`;
- `commandLog.resetPolicy` always encodes `Clear` and decode rejects any other
  value in the first complete build;
- repeated values preserve save section vector order; the codec never sorts;
- decode rejects unknown keys in the first complete build;
- no locale-sensitive formatting, unordered iteration, wall-clock data, or old
  iggy schema adapters.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(save bytes).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use `SaveCodecStatus` plus section/key/line context for expected failures;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- codec owns bytes-to-`SaveEnvelope` mapping only;
- save truth is the decoded `SaveEnvelope`, not the text buffer;
- replay uses decoded command records through normal replay/session APIs.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/save/SaveCodec.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
