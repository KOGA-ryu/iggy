# `src/runtime/multiplayer/Authority.hpp`

Updated: 2026-06-20

Exact purpose: declare command authority policy for local single-player and multiplayer-ready sessions.

## Build Position

- priority rank: 124
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `header`

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

Declare `enum class AuthorityMode : std::uint8_t` with `LocalOnly`,
`LocalMultiplayer`, and `RemoteAuthoritativeReserved`.

Declare `enum class AuthorityDecisionStatus : std::uint8_t` with `Allowed`,
`UnauthorizedSlot`, `InvalidSlot`, `ActorNotControlledBySlot`, and
`CommandKindNotAllowed`.

Declare `AuthorityContext` with read-only player roster/slot ownership,
authority mode, max local players, and a deterministic command kind allow-list.

Declare `AuthorityDecision authorizeCommand(const AuthorityContext& context,
const Command& command)`.

Invariants:

- authority runs before command admission and gameplay validation;
- it checks who may submit, not whether reach/path/objective rules pass;
- local-only accepts slot 0 only;
- local multiplayer supports up to four local slots in deterministic slot order;
- remote authoritative mode is a reserved value with no sockets;
- failures are stable pre-admission statuses and are not appended to
  `CommandLog`, saved in `SaveEnvelope`, or included in `StateHash`.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(player count) or O(1) if direct slot lookup is used.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- authority policy is value configuration read by command submission and is not
  saved as standalone gameplay truth in the first build;
- saved player-slot ownership in `SaveEnvelope` must be sufficient to reproduce
  authority decisions after load;
- command replay covers commands that passed authority and reached admission;
  authority rejection tests assert non-mutation at the authority boundary.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/Authority.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
