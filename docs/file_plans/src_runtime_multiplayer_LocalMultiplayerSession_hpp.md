# `src/runtime/multiplayer/LocalMultiplayerSession.hpp`

Updated: 2026-06-20

Exact purpose: declare local deterministic coordination for multiple player slots before real networking exists.

## Build Position

- priority rank: 129
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `header`

## Ownership

This file owns:

- per-slot command queues
- stable merge ordering
- authority handoff to `Authority` and `Session`

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

- commands grouped by source slot
- merged by tick then slot then sequence
- result summary per slot

## Semantics

- no network sockets
- does not bypass session admission
- models future multiplayer ordering in local tests

## Detailed Design Contract

Declare `LocalPlayerCommandQueue` with player slot, local sequence, requested
tick, and command values.

Declare `LocalMultiplayerStepRequest` with up to four local queues, authority
context, and max commands per step.

Declare:

```cpp
enum class LocalMultiplayerStepStatus : std::uint8_t {
  Ok,
  InvalidQueue,
  InvalidSlot,
  InvalidLocalSequence,
  AuthorityRejected,
  SessionSubmitFailed,
  CommandLimitExceeded,
};

struct LocalMultiplayerStepResult {
  LocalMultiplayerStepStatus status = LocalMultiplayerStepStatus::Ok;
  PlayerSlotId firstFailureSourceSlot = kInvalidPlayerSlotId;
  std::uint64_t firstFailureLocalSequence = 0;
  AuthorityDecisionStatus authorityStatus = AuthorityDecisionStatus::Allowed;
  CommandLogAppendStatus appendStatus = CommandLogAppendStatus::Ok;
  std::uint32_t submittedCount = 0;
  std::uint32_t acceptedCount = 0;
  std::uint32_t rejectedCount = 0;
  std::vector<CommandSequence> mergedCommandOrder;
  StateHashValue resultingStateHash = 0;
  std::string diagnostic;
};
```

Invariants:

- merge order is requested tick, player slot, local sequence;
- the merged command array/result order is the canonical order supplied to
  session submission and future replication packets;
- each merged command is authorized and submitted through normal session APIs;
- local step is all-or-nothing: any non-`Ok` result leaves the destination
  session unchanged and no command from that step is committed;
- local multiplayer is couch-co-op/battle readiness, not online transport;
- source slot identity is preserved for replay and future replication;
- no queue may mutate session directly.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(total queued commands log total queued commands) if sorted; O(total queued commands) if queues are preordered by slot/tick.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- local step failures use `LocalMultiplayerStepStatus` and the declared
  `LocalMultiplayerStepResult` fields.
- source authority detail uses `authorityStatus`; session append detail uses
  `appendStatus`; text diagnostics stay in `diagnostic`.
- logging text is never the only machine-readable outcome.

## Save Replay Multiplayer Notes

- local command queues are transient input and are excluded from `SaveEnvelope`
  and `StateHash`;
- merged commands enter durable truth only after normal session submission and
  `CommandLog` append;
- replay reproduces local multiplayer effects from the resulting command log,
  not from saved queue state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/LocalMultiplayerSession.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
