# `src/runtime/diagnostics/RuntimeEvent.hpp`

Updated: 2026-06-20

Exact purpose: declare structured runtime events emitted by command admission, systems, save/load, replay, and acceptance demo execution.

## Build Position

- priority rank: 96
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/diagnostics`
- file kind: `header`

## Ownership

This file owns:

- event kind enum
- tick/command/entity references
- stable event payload fields

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

- event kinds include `CommandAccepted`, `CommandRejected`, `Moved`,
  `Interacted`, `ItemAcquired`, `ObjectiveCompleted`, `ClockChanged`,
  `CameraChanged`, `SaveCreated`, `LoadCompleted`, `ResetCompleted`, and
  `ReplayCompleted`

## Semantics

- events are derived/transient and excluded from first-build save truth
- events explain behavior but do not cause behavior

## Detailed Design Contract

Declare `enum class RuntimeEventKind : std::uint8_t` with at least:
`CommandAccepted`, `CommandRejected`, `Moved`, `Interacted`, `ItemAcquired`,
`ObjectiveCompleted`, `ClockChanged`, `CameraChanged`, `SaveCreated`,
`LoadCompleted`, `ResetCompleted`, `ReplayCompleted`, and `RuntimeFailed`.

Declare `RuntimeEvent` with stable fields: kind, tick, command id, sequence,
player slot, actor entity, target entity, rejection reason, and optional stable
ids for objective/item/save labels.

Invariants:

- event order is append order and deterministic;
- `CommandRejected` carries the exact rejection reason;
- `cmd_interact_oob` produces `CommandRejected` with `OutOfRange`;
- save/load/reset/replay events are proof diagnostics, not gameplay command
  count truth;
- events are excluded from `SaveEnvelope` and `StateHash` in the first build;
- events explain behavior after the owning mutation or rejection has occurred.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) per event plus payload bytes.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- runtime events are transient diagnostics and are excluded from `SaveEnvelope`
  and `StateHash` in the first build;
- save/load/reset/replay events prove tool phases but do not count as gameplay
  commands;
- multiplayer may emit future diagnostics without changing authority truth.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/diagnostics/RuntimeEvent.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
