# `src/runtime/multiplayer/Authority.cpp`

Updated: 2026-06-20

Exact purpose: implement command authority policy for local single-player and multiplayer-ready sessions.

## Build Position

- priority rank: 125
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `source`

## Ownership

This file owns:

- authority mode enum
- slot permission checks
- local-authoritative policy
- future server-authoritative boundary

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

- modes local single-player, local multiplayer host, remote-authoritative
  reserved value
- allowed command kinds per slot

## Semantics

- single-player is player slot 0 with local authority
- authority rejects commands before gameplay validation
- no sockets or transport code here

## Detailed Design Contract

Implementation algorithm:

1. validate player slot exists for current mode;
2. validate submitted actor/entity is controlled by that slot when the command
   targets an owned actor;
3. validate command kind is allowed for the source slot;
4. return `Allowed` or the first stable failure status;
5. leave reach, target, objective, inventory, and tactical command validation to
   command admission/session logic.

No mutation is allowed. Authority failures are pre-admission diagnostics:
no command is appended to `CommandLog`, saved in `SaveEnvelope`, or included in
`StateHash` in the first complete build.
Diagnostics include source slot, command id if already assigned, actor id, and
authority mode.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(player count) or O(1) if direct slot lookup is used.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- authority decisions are derived from command values, authority mode, and saved
  player-slot ownership;
- no authority result is written into `SaveEnvelope` as gameplay truth;
- local multiplayer must run this same implementation before command admission;
- command replay starts from durable command records that already passed
  authority.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/Authority.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
