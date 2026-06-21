# `src/runtime/diagnostics/RuntimeSummary.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic text summary generation for demos, acceptance fixtures, and debugging.

## Build Position

- priority rank: 109
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/runtime/diagnostics`
- file kind: `header`

## Ownership

This file owns:

- summary field order
- formatting rules
- state/count extraction
- expected summary comparison support

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

- scenario id, lifecycle, outcome, final tick, player position, inventory,
  objective status, key/marker active flags, clock/camera mode, previous realtime
  camera, command counts, first rejection, retry command-id linkage, state hash

## Semantics

- summary is derived read-only output
- same state produces byte-identical summary
- tests compare the full expected summary file once hash is locked

## Detailed Design Contract

Declare `enum class RuntimeProofStatus : std::uint8_t` with `Unknown`, `Pass`,
and `Fail`.

Declare `RuntimeSummaryInput` with read-only pointers/references for
`SessionState`, `CommandLog` count/rejection facts, runtime metrics as
corroborating diagnostics, effective retry execution linkage facts,
save/load proof status, reset proof status, replay proof status, and optional
expected summary metadata.

Declare `RuntimeSummary` as ordered fields matching `docs/acceptance_demo.md`:
`scenario`, `lifecycle`, `outcome`, `final_tick`, `player.position`,
`inventory.player0`, `gold_key.active`, `tactical_marker_alpha.active`,
`objective.collect_gold_key`, `clock.mode`, `camera.mode`,
`camera.previousRealtime`, `commands.submitted`, `commands.accepted`,
`commands.rejected`, `commands.retry`, `first_rejection`,
`retry.original_rejected_command_id`, `retry.retry_command_id`,
`retry.sourceCommandId`, `retry.retrySourceCommandId`,
`retry.executed.command_id`, `retry.executed.sequence`, `save.roundtrip`,
`reset.baseline`, `replay.hash`, and `state_hash`.

Invariants:

- summary is read-only and never saved as runtime truth;
- field order is ABI-like for acceptance tests and CLI output;
- position formatting is fixed to three decimal places;
- state hash formatting is lowercase fixed-width hex;
- first-build summary prints only `commands.submitted`, `commands.accepted`,
  `commands.rejected`, and `commands.retry`; movement, interaction, control, and
  wait counts remain command-log diagnostics and are not printed in the
  acceptance summary;
- retry summary fields are visible product proof and must be derived from the
  command log plus effective retry execution result:
  `retry.original_rejected_command_id` is `cmd_interact_oob.commandId`,
  `retry.retry_command_id` and `retry.sourceCommandId` are
  `cmd_retry_key.commandId`, `retry.retrySourceCommandId` is
  `cmd_interact_oob.commandId`, and the executed command id/sequence are the
  effective interaction id/sequence used for the successful retry;
- proof statuses format as `pass`, `fail`, or `unknown`;
- no localized prose, wall-clock time, renderer state, app path, or raw input is
  part of the summary.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(state size plus output bytes).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- runtime summary text is derived output and is excluded from `SaveEnvelope` and
  `StateHash`;
- command count fields come from `CommandLog`, while metrics may corroborate but
  cannot override command-log counts;
- replay and save/load proof fields are comparison results supplied by the
  caller, not gameplay mutations.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/diagnostics/RuntimeSummary.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
