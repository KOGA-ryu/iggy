# `src/runtime/session/SessionTick.cpp`

Updated: 2026-06-20

Exact purpose: implement one deterministic gameplay tick and its fixed subsystem execution order.

## Build Position

- priority rank: 100
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/session`
- file kind: `source`

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
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- execution order: commands, movement, interactions, combat, AI proposals, objectives, metrics/summary
- tick result with events and lifecycle change

## Semantics

- one tick has no wall-clock dependency
- all mutation happens through owning systems
- AI proposals are commands for future admission, not direct mutations

## Detailed Design Contract

Implement the API declared by `SessionTick.hpp`:

```cpp
SessionTickResult runSessionTick(const SessionTickInput& input);
```

Algorithm:

1. validate `input.state`;
2. return `SessionNotPlayable` or `BlockedByPausedClock` before mutation when
   lifecycle/clock disallow a normal tick;
3. read accepted unexecuted gameplay records supplied by `Session` for the
   current tick;
4. resolve every accepted record into an `EffectiveCommandIntent`;
5. dispatch effective command kinds by owner without local gameplay rules;
6. evaluate objectives after systems mutate state;
7. advance deterministic tick exactly once when status is `Stepped`;
8. emit movement, interaction, pickup, objective, and lifecycle events;
9. update metrics and hash/summary dirty state.

Effective command resolution:

1. For a non-retry accepted command, copy the command into
   `EffectiveCommandIntent::command`, set `effectiveKind` to the command kind,
   set `sourceCommandId` to the command `commandId`, and leave `retrySourceCommandId`
   invalid.
2. For an accepted `Retry`, look up `command.payload.retrySourceCommandId` in the
   command log.
3. If the source record is missing, not rejected, or has unsupported kind, return
   `InvalidState` before mutation; this indicates the admission/log invariant was
   violated.
4. Copy the original rejected command into the effective command, then overwrite
   exactly these `CommandRecord` fields from the accepted retry record: `commandId`,
   `sequence`, `playerSlot`, `admission`, and `rejection`.
5. Set `effectiveKind` to the original rejected command kind, set
   `sourceCommandId` to the retry `commandId`, and set `retrySourceCommandId` to
   the original rejected `commandId`.
6. `sourceCommandId` and `retrySourceCommandId` are `EffectiveCommandIntent`
   wrapper fields, not `CommandRecord` fields.
7. Do not overwrite actor, target, target point, command kind, or interaction
   payload; those remain copied from the original rejected command.
8. Dispatch the normalized effective command. `InteractionSystem` receives only
   a normalized accepted `Interact` command, never a raw `Retry`.

Acceptance-specific behavior: the two move commands reach `(2,0,0)` and
`(2,0,1)`, retry executes one pickup interaction, and paused step does not
alter camera mode.

`cmd_retry_key` execution path:

- admission has already accepted the retry after `cmd_move_to_key`;
- SessionTick resolves it into an effective `Interact` copied from
  `cmd_interact_oob`;
- the effective interact keeps original actor, target, and interaction payload
  from `cmd_interact_oob`;
- the interaction event/result `sourceCommandId` is `cmd_retry_key.commandId`;
- the interaction event/result `retrySourceCommandId` is
  `cmd_interact_oob.commandId`;
- the pickup mutation occurs once: `gold_key:1` is added, `gold_key` is
  deactivated, and `collect_gold_key` completes.

Execution queue behavior:

- do not scan the full `CommandLog` for executable records;
- execute only the accepted unexecuted gameplay records provided in
  `SessionTickInput`;
- report executed command sequences so `Session` can remove them from
  `transient.pendingExecutionSequences`;
- never dispatch immediate control commands to movement, interaction, combat,
  AI, or objective systems.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(commands this tick plus entity count scans required by systems).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- durable mutations produced here are saved through the owning subsystem state
  sections, not through tick-local temporaries;
- emitted events/metrics are transient diagnostics and are excluded from
  `SaveEnvelope` and `StateHash`;
- replay and multiplayer merged-command execution must use this same effective
  command resolution path.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/session/SessionTick.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
