# `src/runtime/interaction/InteractionSystem.hpp`

Updated: 2026-06-20

Exact purpose: declare interaction effects after command admission and reach validation.

## Build Position

- priority rank: 81
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/interaction`
- file kind: `header`

## Ownership

This file owns:

- pickup effect
- activation/door effect
- objective trigger handoff
- inspect no-op result
- unsupported interaction diagnostics

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

- input accepted interact command
- world state
- inventory state
- objective state
- runtime events

## Semantics

- does not discover targets itself except by validating provided id
- pickup adds item then deactivates entity
- objective interaction marks the objective system through its API
- failed effects do not partially mutate state

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(entity lookup plus inventory/objective update cost).
- Optimizations must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- `InteractionRequest` and `InteractionResult` are transient and excluded from
  save/state hash unless runtime events explicitly record them.
- Replay reconstructs interaction execution from command log, world,
  inventory, objectives, and canonical `InteractionDefinition` fields.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/interaction/InteractionSystem.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Header Contract

Required repo path:

```text
src/runtime/interaction/InteractionSystem.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>

#include "runtime/command/Command.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/world/WorldState.hpp"
```

Forward declarations can replace heavy includes where possible, but the API
must make mutation owners explicit and must expose `CommandId` source linkage.

Required enum:

```cpp
enum class InteractionStatus : std::uint8_t {
  Succeeded,
  InvalidCommand,
  InvalidWorld,
  InvalidInventory,
  InvalidObjectiveState,
  InvalidActor,
  InvalidTarget,
  TargetInactive,
  UnsupportedInteraction,
  InventoryFailed,
  ObjectiveFailed,
};
```

Required context/request/result:

```cpp
struct InteractionSystemContext {
  WorldState* world = nullptr;
  InventoryState* inventory = nullptr;
  ObjectiveState* objectives = nullptr;
};

struct InteractionRequest {
  CommandRecord command;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandId retrySourceCommandId = kInvalidCommandId;
};

struct InteractionResult {
  InteractionStatus status = InteractionStatus::InvalidCommand;
  EntityId actor;
  EntityId target;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  InteractionKind kind = InteractionKind::None;
  InteractionEffectKind primaryEffect = InteractionEffectKind::None;
  std::string itemId;
  std::uint32_t itemCount = 0;
  std::string objectiveId;
  bool deactivateTargetOnSuccess = false;
  bool inventoryMutated = false;
  bool targetDeactivated = false;
  bool objectiveMutated = false;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandId retrySourceCommandId = kInvalidCommandId;
};
```

Required API:

```cpp
InteractionResult executeInteraction(
    InteractionSystemContext& context,
    const InteractionRequest& request);
```

Semantics:

- Input command must already be accepted by admission/reach.
- Input command kind must be `Interact`. A raw accepted `Retry` is
  `InvalidCommand`; `SessionTick` must normalize retry into an accepted effective
  interact command before calling this system.
- `sourceCommandId` identifies the `CommandRecord::commandId` responsible for
  the effect. For a retry pickup it is the accepted retry
  `CommandRecord::commandId`, while `retrySourceCommandId` is the original
  rejected interact `CommandRecord::commandId`.
- `sourceCommandId` and `retrySourceCommandId` are interaction request/result
  fields; they are not `CommandRecord` fields. `CommandPayload::retrySourceCommandId`
  is only the retry command payload input used to locate the rejected command.
- Null `InteractionSystemContext::world` returns `InvalidWorld`.
- Null `InteractionSystemContext::inventory` returns `InvalidInventory`.
- Null `InteractionSystemContext::objectives` returns `InvalidObjectiveState`.
- Null-context failures copy request command linkage where available, use invalid
  sentinels for unavailable actor/target facts, and occur before world,
  inventory, or objective mutation.
- This system validates the referenced actor and target still exist/active.
- It does not run target discovery or reach checks.
- Successful pickup reads only canonical `InteractionDefinition` fields, adds
  item to inventory, then deactivates the target.
- Objective completion is triggered by calling/evaluating `ObjectiveSystem`,
  not by hand-editing objective fields.
- Before mutating inventory or world, this system validates objective context
  shape for the requested evaluation.
- Objective evaluation is infallible after that precheck for structurally valid
  `ObjectiveState` and `InventoryState`.
- Failed prechecks or inventory effects do not partially mutate state. If
  inventory add fails, target remains active and objective remains unchanged.

Acceptance fact:

- Retry of rejected interact after movement succeeds: inventory gains
  `gold_key:1`, `gold_key.active=false`, and objective
  `collect_gold_key=Complete`.
- The successful `gold_key` result reports `kind=Pickup`,
  `primaryEffect=AddItemToInventory`, `itemId=gold_key`, `itemCount=1`,
  `objectiveId=collect_gold_key`, and `deactivateTargetOnSuccess=true`.
- The retry success result uses `cmd_retry_key.commandId` as `sourceCommandId`
  and `cmd_interact_oob.commandId` as `retrySourceCommandId`.
