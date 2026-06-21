# `src/runtime/replay/StateHash.hpp`

Updated: 2026-06-20

Exact purpose: declare canonical runtime state hashing for replay, save verification, and acceptance summaries.

## Build Position

- priority rank: 119
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/replay`
- file kind: `header`

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

Declare this exact first-build value type:

```cpp
using StateHashValue = std::uint64_t;
```

`StateHashValue` intentionally matches `SaveMetadataSection::savedStateHash`.
Do not introduce a strong wrapper or alternate integer type in the first
complete build.

Declare:

- `StateHashValue computeStateHash(const SessionState& state)`;
- `std::string formatStateHash(StateHashValue value)`.

Included durable truth:

- `SaveSessionSection`: lifecycle/outcome, current tick, `nextCommandId`, and
  deterministic session seed/schema when owned by session identity;
- world/entity stable ids, active flags, transforms, targetability, and
  canonical interaction metadata;
- player roster/slots and controlled entities;
- clock durable fields: mode, previous unpaused mode, previous unpaused time
  scale, tick index, fixed tick rate, and current time scale;
- camera mode/style state that affects gameplay proof;
- inventory, combat, AI, objectives;
- command log records, next sequence, and epoch.

Summary counts are derived output and are not hash input.
`SaveCommandLogSection.resetPolicy` is a schema constant for save text and is
not mutable runtime state.

Hash visitation order must match the first-build save envelope section order:
metadata identity fields `schemaVersion`, `minimumReadableSchemaVersion`,
`runtimeSaveVersion`, `packageId`, and `scenarioId`; then session, world,
players, clock, camera, commandLog, inventory, combat, ai, and objectives. The
diagnostic metadata field `createdByToolId` and cached hash fields
`savedStateHash`/`savedStateHashHex` are never hash input.

Entity interaction metadata is visited in canonical flattened save order:
`interactionKind`, `interactionPrimaryEffect`, `interactionItemId`,
`interactionItemCount`, `interactionObjectiveId`, `interactionRepeatable`, and
`interactionDeactivateTargetOnSuccess`.

Excluded data: current cached hash field itself, `ClockState::stepRequested`,
`CameraState::inputClearRequested`, runtime events, metrics, projection/debug
projection, summary text, app paths, raw input, renderer/GPU, network/socket
state, wall-clock time, and pointer/container addresses.

Formatting invariant: exactly 16 lowercase hex digits for acceptance output.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(saved state size).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- state hash APIs emit `StateHashValue` hash values only.
- invalid input shape is a caller/test precondition failure; this header does not
  define runtime diagnostics or status results for hashing.
- logging text is not part of the hash contract.

## Save Replay Multiplayer Notes

- hash input follows the durable save section order but excludes cached hash
  metadata;
- replay and save/load compare formatted hash output as proof, not as mutation;
- multiplayer value ordering must not introduce nondeterministic hash input.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/replay/StateHash.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
