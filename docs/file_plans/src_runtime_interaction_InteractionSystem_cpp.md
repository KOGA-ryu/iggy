# `src/runtime/interaction/InteractionSystem.cpp`

Updated: 2026-06-20

Exact purpose: implement interaction effects after command admission and reach validation.

## Build Position

- priority rank: 82
- tier: Tier 5: Playable Gameplay Systems
- module: `src/runtime/interaction`
- file kind: `source`

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

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return the exact `InteractionResult`/effect status documented by
   `InteractionSystem.hpp`; diagnostics are emitted only by the owning
   session/runtime event path when that path wraps the result;
4. avoid hidden static mutable state and wall-clock reads.

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

- `src/runtime/interaction/InteractionSystem.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Implementation Contract

Required repo path:

```text
src/runtime/interaction/InteractionSystem.cpp
```

Include paired header first:

```cpp
#include "runtime/interaction/InteractionSystem.hpp"
```

`executeInteraction` algorithm:

1. Reject non-accepted, non-`Interact`, or raw `Retry` command as
   `InvalidCommand`.
2. Validate context pointers before any lookup:
   - null `world` returns `InvalidWorld`;
   - null `inventory` returns `InvalidInventory`;
   - null `objectives` returns `InvalidObjectiveState`.
3. Null-context failures copy `sourceCommandId` and `retrySourceCommandId` from
   the request, keep actor/target only when the command payload already supplies
   them, use invalid sentinels otherwise, and mutate nothing.
4. Look up actor in world; missing/inactive actor returns `InvalidActor`.
5. Look up target from command entity target; missing returns `InvalidTarget`,
   inactive returns `TargetInactive`.
6. Read target canonical `InteractionDefinition`.
7. For unsupported or `None` interaction kind, return
   `UnsupportedInteraction`.
8. For `Pickup`:
   - require `primaryEffect=AddItemToInventory`;
   - validate `itemId` and `itemCount`;
   - copy `objectiveId` into the result for diagnostics/events;
   - validate objective evaluation context shape before mutation;
   - call `InventorySystem::addItem` for command player slot;
   - if inventory fails, return `InventoryFailed` and do not deactivate target;
   - deactivate target through `WorldState` public API;
   - call objective evaluation/completion API so `collect_gold_key` can
     complete from inventory truth;
   - return `Succeeded` with mutation flags.
9. For `Inspect`, return success with no mutation when supported.
10. For `Activate`, `OpenDoor`, or `ObjectiveTrigger`, return
   `UnsupportedInteraction` in the complete build.

Atomicity:

- Pickup mutation order is objective-context precheck, inventory add, world
  deactivation, objective evaluation.
- Null-context failures occur before mutation and return `InvalidWorld`,
  `InvalidInventory`, or `InvalidObjectiveState` exactly.
- Objective-context precheck catches invalid objective state or invalid inventory
  structure before any mutation and returns `ObjectiveFailed`.
- If inventory add fails, no target or objective mutation occurs.
- After the precheck succeeds, objective evaluation cannot fail for structurally
  valid `ObjectiveState` and `InventoryState` in the first build.
- Runtime invariant violations detected after inventory/world mutation are
  reported through the owning session runtime-failure path; they are not exposed
  as partial pickup success or ambiguous `ObjectiveFailed`.
- No rollback is required after world deactivation because all fallible
  first-build checks have already completed.

Mutation boundaries:

- World mutation only through `WorldState::setActive`.
- Inventory mutation only through `InventorySystem`.
- Objective mutation only through `ObjectiveSystem`.
- No command log, admission, target/reach, clock, camera, save, projection,
  renderer, app, filesystem, or old `iggy` mutation.

Acceptance-sensitive result:

- Successful key pickup is executed exactly once by retry command.
- `sourceCommandId` on the interaction result/event is
  `cmd_retry_key.commandId`.
- `retrySourceCommandId` on the interaction result/event is
  `cmd_interact_oob.commandId`.
- The interaction result/event reports canonical metadata:
  `kind=Pickup`, `primaryEffect=AddItemToInventory`, `itemId=gold_key`,
  `itemCount=1`, `objectiveId=collect_gold_key`,
  `deactivateTargetOnSuccess=true`.
- A second interact against inactive `gold_key` must not add a second key.
