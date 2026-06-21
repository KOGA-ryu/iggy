# `src/runtime/replay/StateHash.cpp`

Updated: 2026-06-20

Exact purpose: implement canonical runtime state hashing for replay, save verification, and acceptance summaries.

## Build Position

- priority rank: 120
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/replay`
- file kind: `source`

## Ownership

This file owns:

- field visitation order
- which fields are included/excluded
- hash formatting

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

- includes session lifecycle, session `nextCommandId`, world, roster, clock,
  camera, inventory, combat, AI, objective, full command-log records, next
  sequence, and epoch
- excludes transient diagnostics/projection/app paths

## Semantics

- same saved truth produces same hash
- hash order follows deterministic storage order
- summary prints lowercase fixed-width hex

## Detailed Design Contract

Implementation algorithm:

1. visit sections in the order declared by `SaveEnvelope`;
2. visit repeated values in the exact order stored in the save envelope or the
   runtime authoritative vectors;
3. write field tags before field values so missing/default fields cannot collide;
4. hash `SaveSessionSection.nextCommandId` before world/player state and after
   current tick;
5. quantize floating position values with the same precision used by save and
   summary contracts;
6. visit canonical interaction metadata in entity records as
   `interactionKind`, `interactionPrimaryEffect`, `interactionItemId`,
   `interactionItemCount`, `interactionObjectiveId`, `interactionRepeatable`,
   and `interactionDeactivateTargetOnSuccess`;
7. include accepted and rejected command records with exact rejection reasons;
8. exclude cached hash, `ClockState::stepRequested`,
   `CameraState::inputClearRequested`, and all
   transient/projection/app/renderer state;
9. format the final value as fixed-width lowercase hex.

`StateHashValue` is exactly `std::uint64_t`; implementation stores and returns
that type directly so `SaveMetadataSection::savedStateHash` can carry the value
without conversion.

The hash algorithm must be deterministic across macOS, Linux, Windows, Steam
Deck, and console-class targets: no endian-dependent raw struct hashing, no
`std::hash`, no pointer values, no unordered iteration, and no reordering of
repeated values inside the hash implementation. Hash mismatch diagnostics name
the first known section when called by save/load or replay.

Repeated-value order is locked:

- command log records are visited in `CommandLog::records()` canonical vector
  order;
- world entities are visited in `WorldState::entities()` storage order;
- player slots are visited in the saved/runtime roster vector order;
- inventory player records and stacks are visited in saved/runtime vector order;
- combatants, AI actors, and objectives are visited in saved/runtime vector
  order.

If load validation needs to reject malformed ordering, that belongs to
save/load before hashing. Hashing itself never reorders values.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. treat non-finite or structurally invalid runtime state as a caller/test
   precondition failure before appending hash input; state-hash code emits only
   `StateHashValue`;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(saved state size).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- `StateHash` consumes save truth from runtime state or `SaveEnvelope` section
  values;
- projection, debug projection, runtime events, metrics, summary text, app
  process data, renderer state, and raw input are excluded;
- replay proof compares hashes after commands execute through the normal
  session path.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/replay/StateHash.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
