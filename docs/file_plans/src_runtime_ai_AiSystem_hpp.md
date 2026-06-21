# `src/runtime/ai/AiSystem.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic AI command proposal generation.

## Build Position

- priority rank: 89
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/ai`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/runtime/ai/AiSystem.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/ai/AiSystem.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <vector>

#include "runtime/ai/AiState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/world/WorldState.hpp"
```

Required enums:

```cpp
enum class AiSystemStatus : std::uint8_t {
  Ok,
  InvalidState,
  InvalidWorld,
};

enum class AiProposalStatus : std::uint8_t {
  NoProposal,
  Proposed,
};
```

Required value types:

```cpp
struct AiSystemContext {
  const WorldState* world = nullptr;
  const PlayerRoster* players = nullptr;
  std::uint64_t currentTick = 0;
};

struct AiProposal {
  AiProposalStatus status = AiProposalStatus::NoProposal;
  EntityId actor;
  CommandRecord command;
};

struct AiSystemResult {
  AiSystemStatus status = AiSystemStatus::Ok;
  std::vector<AiProposal> proposals;
  std::uint32_t actorsConsidered = 0;
  std::string diagnostic;
};
```

Required API:

```cpp
AiSystemResult generateAiProposals(
    const AiState& ai,
    const AiSystemContext& context);
```

Semantics:

- First complete behavior is dormant and deterministic.
- `AiSystemStatus::Ok` means the scan completed; `proposals` is empty for every
  policy id in the complete runtime demo.
- `AiSystemStatus::InvalidWorld` means required world/player context is missing;
  `proposals` is empty, `actorsConsidered == 0`, and no state mutates.
- `AiSystemStatus::InvalidState` means `AiState` contains structurally invalid
  actor records; `proposals` is empty and no state mutates.
- The function scans enabled actors whose `nextDecisionTick <= currentTick` in
  AI actor vector order.
- It does not mutate `AiState`, world, inventory, combat, objective, command log,
  clock, or camera.
- It returns no proposals for policy id `0`.
- Nonzero policy ids are reserved and also return no proposal in the complete
  build.
- Proposal output is always empty for all policy ids in the complete runtime
  demo. Non-empty proposal behavior is outside this complete-build contract.
- Empty AI state returns no proposals and is valid for first-room acceptance.
- Scan order is AI actor vector order.
