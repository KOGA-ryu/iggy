# `src/runtime/interaction/InteractionDefinition.hpp`

Updated: 2026-06-20

Exact purpose: declare interaction kinds and effect metadata used by interaction execution.

## Build Position

- priority rank: 80
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/interaction`
- file kind: `header`

## Ownership

This file owns:

- canonical interaction kind enum
- canonical interaction effect enum
- canonical interaction metadata stored by `EntityState`

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

- kinds pickup, activate, open door, inspect, objective trigger, attack if routed through interaction
- range requirement reference
- repeatability flag

## Semantics

- definitions are content/runtime metadata, not UI prompts
- execution lives in `InteractionSystem`
- targeting/reach checks occur before effects

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) per definition validation/access.

## Diagnostics And Errors

- interaction metadata helpers do not emit diagnostics;
- content/package validation reports malformed interaction metadata before
  session creation;
- `InteractionSystem` reports runtime execution failures with
  `InteractionStatus`.

## Save Replay Multiplayer Notes

- `InteractionDefinition` is durable entity metadata when stored in
  `EntityState`.
- Save, state hash, replay, and multiplayer packet docs use this canonical
  schema and field order.
- No duplicate interaction schema is allowed in `EntityState` or save docs.

## Tests And Verification

- package loader, world state, interaction system, save/load, and state-hash
  tests assert first-room `gold_key` metadata through this schema;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/runtime/interaction/InteractionDefinition.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/interaction/InteractionDefinition.hpp
```

This header declares interaction metadata stored on or referenced by world
entities. It does not discover targets, check reach, mutate inventory, mutate
objectives, or execute effects.

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
```

Required enums:

```cpp
enum class InteractionKind : std::uint8_t {
  None,
  Pickup,
  Activate,
  OpenDoor,
  Inspect,
  ObjectiveTrigger,
};

enum class InteractionEffectKind : std::uint8_t {
  None,
  AddItemToInventory,
  DeactivateTarget,
  CompleteObjective,
  EmitEventOnly,
};
```

Required value type:

```cpp
struct InteractionDefinition {
  InteractionKind kind = InteractionKind::None;
  InteractionEffectKind primaryEffect = InteractionEffectKind::None;
  std::string itemId;
  std::uint32_t itemCount = 0;
  std::string objectiveId;
  bool repeatable = false;
  bool deactivateTargetOnSuccess = false;
};
```

Invariants:

- `Pickup` with `AddItemToInventory` requires non-empty `itemId` and
  `itemCount > 0`.
- First-room `gold_key` uses `kind=Pickup`, `itemId=gold_key`,
  `itemCount=1`, `objectiveId=collect_gold_key`, `repeatable=false`,
  `primaryEffect=AddItemToInventory`, and
  `deactivateTargetOnSuccess=true`.
- `Inspect` is read-only in the complete build.
- Definitions contain gameplay metadata only; no UI text, prompt layout,
  renderer mesh id, audio handle, or file path belongs here.

Save/replay/hash mapping:

- Save entity records flatten canonical fields in this order:
  `interactionKind`, `interactionPrimaryEffect`, `interactionItemId`,
  `interactionItemCount`, `interactionObjectiveId`, `interactionRepeatable`,
  `interactionDeactivateTargetOnSuccess`.
- These flattened names are an exact representation of `InteractionDefinition`
  fields; legacy interaction metadata aliases are not accepted in complete-build
  docs or tests.
- State hash visits the same canonical fields in the same order.
- The active/inactive world entity and inventory/objective state are save truth
  alongside the canonical metadata copied into `EntityState`.
