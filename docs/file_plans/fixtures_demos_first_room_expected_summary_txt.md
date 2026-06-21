# `fixtures/demos/first_room/expected_summary.txt`

Updated: 2026-06-20

Exact purpose: store the byte-exact expected output of `iggy3d_headless_demo` for the complete runtime acceptance proof.

## Build Position

- priority rank: 132
- tier: Tier 8: Product Proof And Tools
- module: `first_room fixture`
- file kind: `fixture`

## Ownership

This file owns:

- summary field order
- expected final player state
- command counts
- first rejection reason
- visible retry command-id linkage
- clock/camera final mode
- state hash line

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- documented first-party fixture schema only
- no old iggy asset/schema dependency

## Data Contract

- scenario `iggy3d.first_room:first_room.runtime_loop`
- outcome `DemoComplete`
- lifecycle `Complete`
- final player position `(2.000,0.000,1.000)`
- inventory contains `gold_key:1`
- `tactical_marker_alpha.active=true`
- first rejection `OutOfRange`
- final clock `Normal`
- final camera `ThirdPerson`
- previous realtime camera `ThirdPerson`

## Semantics

- after implementation first green locks the literal state hash value here
- acceptance compares full file contents, not loose substrings

## Detailed Design Contract

The planned repo file is literal expected output for the headless demo:
`fixtures/demos/first_room/expected_summary.txt`.

It must contain exactly these fields in this order, one `key=value` per line:

```text
scenario=iggy3d.first_room:first_room.runtime_loop
lifecycle=Complete
outcome=DemoComplete
final_tick=<locked integer after implementation>
player.position=(2.000,0.000,1.000)
inventory.player0=gold_key:1
gold_key.active=false
tactical_marker_alpha.active=true
objective.collect_gold_key=Complete
clock.mode=Normal
camera.mode=ThirdPerson
camera.previousRealtime=ThirdPerson
commands.submitted=10
commands.accepted=9
commands.rejected=1
commands.retry=1
first_rejection=OutOfRange
retry.original_rejected_command_id=1
retry.retry_command_id=3
retry.sourceCommandId=3
retry.retrySourceCommandId=1
retry.executed.command_id=3
retry.executed.sequence=3
save.roundtrip=pass
reset.baseline=pass
replay.hash=pass
state_hash=<locked lowercase 16-hex value after first green implementation>
```

`<locked ...>` placeholders are planning text only. The implemented fixture must
replace them with concrete deterministic values in the first green acceptance
update. No extra comments, blank lines, localized text, renderer facts, or app
paths belong in the real expected summary file.

## Implementation Plan

1. write deterministic fixture data in the documented schema;
2. keep entity order stable because ids derive from it;
3. avoid renderer-only or old iggy references;
4. make acceptance-sensitive values explicit.

## Compute Cost

- O(summary bytes) to compare.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- expected summary text is loaded and compared by tests/tools;
- it is not a C++ build target and owns no save truth;
- save/load/replay proof must produce this text from runtime state, not store it
  in `SaveEnvelope`.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `fixtures/demos/first_room/expected_summary.txt` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
