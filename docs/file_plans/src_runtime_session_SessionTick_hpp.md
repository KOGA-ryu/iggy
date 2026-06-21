# `src/runtime/session/SessionTick.hpp`

Updated: 2026-06-20

Exact purpose: declare one deterministic gameplay tick and its fixed subsystem execution order.

## Build Position

- priority rank: 99
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/session`
- file kind: `header`

## Ownership

This file owns:

- tick order
- accepted command dispatch
- event collection
- post-tick summary/hash update trigger

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- `runtime/command/Command.hpp` for `CommandId`, `CommandRecord`,
  `CommandKind`, and `kInvalidCommandId`
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- execution order: commands, movement, interactions, combat, AI proposals, objectives, metrics/summary
- tick result with events and lifecycle change
- effective command intent for accepted retry execution

## Semantics

- one tick has no wall-clock dependency
- all mutation happens through owning systems
- AI proposals are commands for future admission, not direct mutations

## Detailed Design Contract

Declare `SessionTickInput` with `SessionState* state`, accepted command view/span
for this tick, and `bool forceStepWhilePaused = false`.

Declare `EffectiveCommandIntent`:

```cpp
struct EffectiveCommandIntent {
  CommandRecord command;
  CommandKind effectiveKind = CommandKind::None;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandId retrySourceCommandId = kInvalidCommandId;
};
```

Semantics:

- For normal accepted commands, `command.kind == effectiveKind`,
  `sourceCommandId == command.commandId`, and `retrySourceCommandId` is invalid.
- For an accepted `Retry`, `command` is a normalized accepted command copied from
  the original rejected command intent, `effectiveKind` is the original kind,
  `sourceCommandId` is the retry `commandId`, and `retrySourceCommandId` is the
  original rejected `commandId`.
- Creating the normalized command overwrites exactly these copied-command fields:
  `commandId`, `sequence`, `playerSlot`, `admission`, and `rejection`. The
  `EffectiveCommandIntent` wrapper stores `sourceCommandId` and
  `retrySourceCommandId`.
- `sourceCommandId` and `retrySourceCommandId` are fields on
  `EffectiveCommandIntent`; they are not fields on `CommandRecord`.
- No actor, target, target point, command kind, or interaction payload field is
  overwritten during retry normalization.
- For `cmd_retry_key`, the normalized command passed to interaction has
  `effectiveKind == CommandKind::Interact`, `command.kind == CommandKind::Interact`,
  `sourceCommandId == cmd_retry_key.commandId`, and
  `retrySourceCommandId == cmd_interact_oob.commandId`.
- The `cmd_retry_key` effective interact keeps actor, target, and interaction
  payload from `cmd_interact_oob`.
- Raw accepted `Retry` records are never sent to `InteractionSystem`.

Declare `SessionTickStatus`: `Stepped`, `NoWork`, `BlockedByPausedClock`,
`SessionNotPlayable`, and `InvalidState`.

Declare `SessionTickResult` with status, tick before/after, movement executed,
interaction executed, combat executed, AI proposals generated, events emitted,
lifecycle changed, and resulting `SessionOutcome`.

Required API:

```cpp
SessionTickResult runSessionTick(const SessionTickInput& input);
```

Fixed execution order:

1. receive accepted unexecuted gameplay commands for this tick from `Session` in
   command-log sequence order;
2. resolve each accepted command into an `EffectiveCommandIntent`;
3. execute movement through `MovementSystem`;
4. execute interaction through `InteractionSystem` using normalized accepted
   `Interact` commands only;
5. execute combat through `CombatSystem`;
6. run AI proposal generation without direct mutation;
7. evaluate objectives;
8. update lifecycle/outcome through session-owned state;
9. append runtime events and update metrics;
10. mark state hash and summary dirty or recompute through delegated helpers.

Rejected commands never appear in execution input. One paused step advances
exactly one tick and leaves the clock paused.

Execution cursor ownership:

- `CommandLog` is immutable command history and is not an execution queue;
- `SessionState::transient.pendingExecutionSequences` records accepted gameplay
  commands that still need one execution;
- `Session` builds `SessionTickInput` only from pending unexecuted accepted
  gameplay commands;
- `SessionTick` consumes each accepted gameplay command at most once and reports
  executed sequences back to `Session` for queue removal;
- immediate control commands are never dispatched to gameplay systems.

Retry execution invariants:

- SessionTick revalidates that the retry record is accepted and that the retry
  source `commandId` resolves to the original rejected command.
- SessionTick does not bypass admission or reach; it executes only accepted retry
  records already produced by `CommandAdmission`.
- The normalized effective command executes exactly once at the retry command's
  sequence position.
- Runtime events/results use `sourceCommandId` equal to the retry `commandId` and
  include `retrySourceCommandId` for traceability.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(commands this tick plus entity count scans required by systems).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- session tick mutates durable subsystem state through owning systems only;
- runtime events, metrics updates, dirty flags, and summary/hash cache triggers
  are transient and excluded from `SaveEnvelope`;
- replay must execute accepted commands through this same tick path to reproduce
  final state hash.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/session/SessionTick.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
