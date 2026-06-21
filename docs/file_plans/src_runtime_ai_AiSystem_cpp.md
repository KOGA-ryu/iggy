# `src/runtime/ai/AiSystem.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic AI command proposal generation.

## Build Position

- priority rank: 90
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/ai`
- file kind: `source`

## Ownership

This file owns:

- AI scan order
- proposal creation
- cooldown/decision tick updates

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

- reads world/player/objective state
- returns command proposal container for external admission
- uses stable actor/entity ordering

## Semantics

- no unseeded randomness
- dormant first complete build mutates no runtime state and creates no proposals

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(ai actor count times relevant entity scans).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- AI state is save/hash truth; proposal/result values are transient and excluded
  from save/hash unless admitted/logged as `CommandRecord` values or explicitly
  recorded as runtime events.
- Save/hash order is `AiState::actors` vector order.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/ai/AiSystem.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/ai/AiSystem.cpp
```

Include paired header first:

```cpp
#include "runtime/ai/AiSystem.hpp"
```

Implementation rules:

- Return `AiSystemStatus::InvalidWorld` with empty `proposals`,
  `actorsConsidered=0`, and a stable diagnostic when world or player context is
  missing.
- Return `AiSystemStatus::InvalidState` with empty `proposals`,
  `actorsConsidered=0`, and a stable diagnostic when `AiState` contains duplicate
  actor records, invalid entity ids, or invalid negative tick data.
- Iterate `AiState::actors` in stable vector order.
- Skip disabled AI actors.
- Skip actors whose `nextDecisionTick` is greater than current tick.
- Count enabled due actors in `actorsConsidered`.
- Produce no `CommandRecord` values for policy id `0`.
- Produce no `CommandRecord` values for nonzero policy ids because the complete
  build no-proposal behavior applies to every policy id.
- Successful scans return `AiSystemStatus::Ok`.
- Do not admit, execute, log, or mutate any command.
- Do not update `nextDecisionTick` or any other AI bookkeeping in this dormant
  behavior.

First complete policy:

- No proactive AI policy is implemented.
- `generateAiProposals` is a deterministic scan that returns no proposals.
- Empty result is valid and must be tested for empty, disabled, not-due,
  policy-zero, and reserved-nonzero-policy actors.

Forbidden:

- no random device;
- no wall-clock time;
- no direct world mutation;
- no command log append;
- no renderer/projection/app dependencies;
- no old `iggy` AI imports.

Compute:

- O(AI actor count times any explicit world scans).
- First-room empty AI state is O(1)/empty scan.
