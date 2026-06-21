# `tests/unit/command_admission_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove command admission is deterministic, read-only, ordered by
first failure, and able to accept/reject the complete runtime command set before
any gameplay mutation occurs.

## Build Position

- priority rank: 60
- tier: Tier 4: Time Camera Command Session Base
- module: `unit tests`
- file kind: `test`

This test file is the executable guardrail for
`src/runtime/command/CommandAdmission.hpp` and
`src/runtime/command/CommandAdmission.cpp`.

## Ownership

This file owns:

- command admission unit scenarios;
- local test fixture builders;
- exact rejection reason assertions;
- no-mutation assertions;
- retry admission regression cases;
- paused/step admission cases;
- acceptance-demo command admission cases.

It must not own:

- production command admission logic;
- production fixture parsing unless using public fixture loader;
- movement, interaction, inventory, objective, save/load, or replay behavior;
- renderer/window setup;
- raw input conversion;
- network transport;
- old `/Users/kogaryu/iggy` adapters.

## Required Test File Shape

The implementation file must be:

```text
tests/unit/command_admission_tests.cpp
```

Required include categories:

```cpp
#include <test framework header>

#include "runtime/command/Command.hpp"
#include "runtime/command/CommandAdmission.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/world/WorldState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "config/RuntimeConfig.hpp"
```

Add targeting/reach/session helpers only as required. Do not include app,
renderer, projection, socket, or old `iggy` headers.

## Test Fixture Contract

The tests may use a hand-built in-memory fixture or the public first-room
fixture loader. If hand-built, it must match the acceptance fixture facts:

- `player` entity id `1`, kind `Player`, active, position
  `(0.000,0.000,0.000)`;
- `gold_key` entity id `2`, kind `Pickup`, active, position
  `(3.000,0.000,0.000)`, interactable, item id `gold_key`;
- `tactical_marker_alpha` entity id `3`, kind `Marker`, active, position
  `(2.000,0.000,1.000)`;
- player slot `0`, kind `Local`, bound to `player`;
- interaction range `1.500`;
- clock starts `Normal`;
- camera is not part of `CommandAdmissionContext`; camera transitions are covered
  by session and camera policy tests;
- command log starts empty unless the test scenario needs prior commands.

Required test-local helper names and behavior:

```cpp
AdmissionFixture makeFirstRoomAdmissionFixture();
CommandRecord makeCommand(CommandKind kind);
CommandAdmissionResult admit(AdmissionFixture&, CommandRecord);
CommandRecord acceptedRecord(CommandRecord);
CommandRecord rejectedRecord(CommandRecord, CommandRejectionReason);
```

Fixture helpers are test-local. They must not become production code.

## Required Assertions By Category

### 1. Accept/Reject Helpers

Tests:

- `acceptCommand_setsAcceptedAndClearsRejection`;
- `rejectCommand_setsRejectedAndStoresReason`;
- `rejectCommand_neverReturnsNoneReason`;

Expected:

- accepted command has `admission=Accepted`;
- accepted command has `rejection=None`;
- rejected command has `admission=Rejected`;
- rejected command has non-`None` rejection;
- `commandId`, sequence, player, actor, and payload are preserved.

### 2. Context Validation

Tests:

- `missingWorldContext_rejectsOrFailsWithoutCrash`;
- `missingPlayersContext_rejectsOrFailsWithoutCrash`;
- `missingClockContext_rejectsOrFailsWithoutCrash`;
- `retryWithoutCommandLog_rejectsOrFailsWithoutCrash`.

Expected:

- no null pointer dereference;
- command never returns accepted;
- returned reason is `InternalError`;
- no world/player/clock/log mutation.

### 3. Command Shape Validation

Tests:

- `noneCommand_rejectsInvalidCommand`;
- `moveWithoutPoint_rejectsInvalidTargetPoint`;
- `interactWithoutEntity_rejectsInvalidTarget`;
- `retryWithoutSourceId_rejectsRetrySourceMissing`;
- `sessionControlWithExtraneousTarget_doesNotUseTargetData`.

Expected:

- exact rejection reason matches contract;
- command stays rejected without mutation;
- malformed shape fails before player/actor/target lookup when applicable.

### 4. Player Slot Validation

Tests:

- `invalidPlayerSlot_rejectsInvalidPlayerSlot`;
- `missingPlayerSlot_rejectsInvalidPlayerSlot`;
- `validLocalPlayerSlot_allowsFurtherChecks`.

Expected:

- bad slot rejects `InvalidPlayerSlot`;
- valid slot does not fail this stage;
- test proves first-failure order by pairing valid/invalid actor data with
  invalid slot and expecting slot failure first.

### 5. Actor Binding Validation

Tests:

- `invalidActor_rejectsInvalidActor`;
- `missingActorEntity_rejectsInvalidActor`;
- `actorNotControlledBySlot_rejectsActorNotControlledBySlot`;
- `validActorBinding_allowsTargetChecks`.

Expected:

- actor missing/invalid rejects `InvalidActor`;
- actor bound to a different slot rejects `ActorNotControlledBySlot`;
- actor failure occurs before target failure.

### 6. Session And Clock Validation

Tests:

- `moveWhilePaused_rejectsSessionPaused`;
- `interactWhilePaused_rejectsSessionPaused`;
- `toggleTacticalWhilePaused_rejectsSessionPaused`;
- `waitWhilePaused_rejectsSessionPaused`;
- `pauseWhilePaused_acceptsIdempotentNoOp`;
- `stepWhileNotPaused_rejectsStepRequiresPaused`;
- `stepWhilePaused_accepts`;
- `resumeWhilePaused_accepts`;
- `pauseWhileRunning_accepts`;
- `waitInSlowModeAfterResume_accepts`.

Expected:

- paused blocks gameplay commands;
- paused blocks `Wait` and `ToggleTacticalMode`;
- `Pause` while paused is accepted idempotently;
- step requires paused;
- step accepted while paused;
- pause idempotence matches the documented complete-build implementation;
- slow mode after resume is not paused, so `cmd_wait` is accepted there;
- no tick execution occurs inside admission.

### 7. Target Existence And Activity

Tests:

- `interactInvalidTarget_rejectsInvalidTarget`;
- `interactMissingTarget_rejectsInvalidTarget`;
- `interactInactiveTarget_rejectsTargetInactive`;
- `inspectInactiveTarget_usesDocumentedBehavior`.

Expected:

- invalid/missing target rejects `InvalidTarget`;
- inactive target rejects `TargetInactive` unless inspect explicitly supports
  inactive targets;
- target validation runs after actor binding.

### 8. Targetability

Tests:

- `interactWithPickupTarget_allowsReachCheck`;
- `interactWithMarkerTarget_rejectsInvalidTarget`;
- `selfInteract_rejectsInvalidTarget`;
- `inspectPickupTarget_acceptsOrUsesDocumentedReachPolicy`.

Expected:

- `gold_key` is targetable for interact;
- marker is not accepted as pickup interaction target;
- self-targeting does not pass for interact in first demo;
- targetability failure maps to `InvalidTarget`.

### 9. Point Validation

Tests:

- `moveToFinitePoint_acceptsWhenOtherRulesPass`;
- `moveToNaNPoint_rejectsInvalidTargetPoint`;
- `moveToInfinitePoint_rejectsInvalidTargetPoint`;
- `moveWithEntityOnly_rejectsInvalidTargetPoint`.

Expected:

- finite target point can pass;
- non-finite point rejects `InvalidTargetPoint`;
- point farther than `RuntimeConfig::movementDistanceMeters` rejects
  `MovementTooFar` during admission;
- no movement mutation occurs.

### 10. Reach And Range

Tests:

- `initialInteractWithGoldKey_rejectsOutOfRange`;
- `interactWithGoldKeyAfterMoveIntoReach_accepts`;
- `outOfRangeUsesOutOfRangeNotTargetNotReachable`;
- `reachUsesRuntimeConfigInteractionRange`: with config range `1.500`,
  baseline interact rejects and post-move interact accepts; changing the config
  changes the reach decision through admission.
- `nonPositiveInteractionRangeFailsBeforeReachExecution`: config range `0.0` or
  negative returns `InternalError` and does not mutate state.
- `reachFailureDoesNotMutateState`.

Expected:

- player at `(0,0,0)`, key at `(3,0,0)`, range `1.500` rejects exactly
  `OutOfRange`;
- player moved or fixture-adjusted to `(2,0,0)` admits interaction;
- rejection reason is not `InvalidTarget`, `TargetNotReachable`, or prose-only;
- world, inventory, objective, and command log remain unchanged by admission.

### 11. Retry

Tests:

- `retryMissingSource_rejectsRetrySourceMissing`;
- `retryAcceptedSource_rejectsRetrySourceNotRejected`;
- `retryUnsupportedSource_rejectsRetryUnsupportedKind`;
- `retrySourceLookupUsesCommandIdNotSequenceOrAppendIndex`;
- `retryOutOfRangeInteractAfterMove_accepts`;
- `retryDoesNotBypassAdmission`;
- `retryAcceptanceDoesNotExecuteOriginalIntent`: accepted `cmd_retry_key` leaves
  inventory, world active flags, and objectives unchanged during admission.
- `retryFailureKeepsOriginalRejectedRecord`.

Expected:

- retry source lookup uses command log;
- retry source lookup uses stored `commandId`, not sequence, vector index, or
  append index;
- accepted/non-rejected source cannot be retried;
- `Retry`, `Reset`, `Save`, and `Load` sources are unsupported;
- retry of `cmd_interact_oob` only accepts after state changes put actor in
  reach;
- retry still checks target active/valid/reachable;
- accepted retry record remains `CommandKind::Retry`; the effective interact is
  resolved later by `SessionTick`;
- command log append is not performed by admission.

### 12. Save Load Reset Reserved Command Kinds

Tests:

- `resetSaveLoadKinds_areReservedForProofApis`;
- `resetSaveLoadKinds_areNotGameplayAdmissionPath`;
- `saveLoadCommandsDoNotStoreFilePaths`.

Expected:

- first complete build does not submit `Reset`, `Save`, or `Load` through the
  gameplay admission path;
- reset/save/load proof phases call their owning session/save APIs directly and
  emit proof events rather than gameplay command records;
- file paths are not present in command payloads;
- command admission tests verify these command kinds remain reserved and cannot
  affect the deterministic gameplay command count.

### 13. First Failure Order

Tests must prove order with compound-invalid commands:

- invalid slot plus invalid actor returns `InvalidPlayerSlot`;
- valid slot plus invalid actor plus invalid target returns `InvalidActor`;
- valid actor plus invalid target plus out-of-range geometry returns
  `InvalidTarget`;
- inactive target plus out-of-range geometry returns `TargetInactive`;
- valid target but out of range returns `OutOfRange`;
- retry missing source with malformed original-style target returns
  `RetrySourceMissing`.

Expected:

- first failure order matches `CommandAdmission.hpp` and
  `CommandAdmission.cpp` plans exactly.

### 14. No Mutation Guarantee

Every rejection test must snapshot relevant state before admission and assert:

- world state unchanged;
- player roster unchanged;
- clock state unchanged;
- command log unchanged;
- inventory unchanged if fixture includes it;
- objective unchanged if fixture includes it.

Acceptance of a command by admission also must not execute it. Accepted movement
does not move the actor until `MovementSystem`/session execution runs.

## Acceptance Demo Coverage

This unit test does not run the full demo, but it must cover admission for each
demo command:

| Label | Expected Admission Unit Proof |
| --- | --- |
| `cmd_interact_oob` | rejected `OutOfRange` from baseline |
| `cmd_move_to_key` | accepted from baseline |
| `cmd_retry_key` | accepted after command log has rejected source and player is in reach |
| `cmd_enter_tactical` | accepted from normal clock |
| `cmd_tactical_move` | accepted from slow clock |
| `cmd_pause` | accepted from slow clock |
| `cmd_step` | accepted from paused clock |
| `cmd_resume` | accepted from paused clock |
| `cmd_wait` | accepted from slow clock after resume |
| `cmd_exit_tactical` | accepted from slow clock |

## Diagnostics And Errors

Tests assert enum/status values, not prose.

Stable reasons that must be asserted:

- `InvalidCommand`;
- `InvalidPlayerSlot`;
- `InvalidActor`;
- `ActorNotControlledBySlot`;
- `InvalidTarget`;
- `TargetInactive`;
- `InvalidTargetPoint`;
- `OutOfRange`;
- `SessionPaused`;
- `StepRequiresPaused`;
- `RetrySourceMissing`;
- `RetrySourceNotRejected`;
- `RetryUnsupportedKind`.

If `InternalError` is used for missing context, tests must assert it does not
crash and does not accept.

## Save Replay Multiplayer Notes

Save/replay impact:

- tests must ensure admitted/rejected command records preserve fields needed by
  save/replay;
- tests should construct a rejected command record suitable for command log and
  retry tests;
- tests should verify retry uses stored `commandId`, not sequence, append index,
  vector index, or test-local labels.

Command identity coverage:

- command construction helper must assign nonzero `commandId` before calling
  `admitCommand`;
- accepted and rejected admission results preserve the input `commandId`;
- admission diagnostics/rejections can report `commandId`;
- admission never assigns or changes sequence.

Multiplayer impact:

- player slot validation must prove commands are slot-scoped;
- actor/slot mismatch test is the local stand-in for future remote authority;
- no test should rely on single hardcoded player outside the first-room fixture.

## Compute Cost

Unit fixture size is tiny:

- entity count: `3` for first-room cases;
- player slots: `1` or `2` for mismatch tests;
- command log entries: usually `0`, `1`, or `2`.

Tests must not create large random worlds. If later performance tests are
needed, they belong in a separate benchmark/perf target, not this unit file.

## Required Test Organization

Recommended sections or test suites:

1. `CommandAdmissionHelpers`;
2. `CommandAdmissionContext`;
3. `CommandAdmissionShape`;
4. `CommandAdmissionPlayerActor`;
5. `CommandAdmissionClock`;
6. `CommandAdmissionTargetReach`;
7. `CommandAdmissionRetry`;
8. `CommandAdmissionControlCommands`;
9. `CommandAdmissionFirstFailureOrder`;
10. `CommandAdmissionAcceptanceDemoCases`;

Each test name should describe the exact rejected/accepted behavior.

## Completion Criteria

- `tests/unit/command_admission_tests.cpp` exists in `/Users/kogaryu/iggy3d`.
- It builds through CTest without renderer, network, app shell, or old `iggy`.
- It covers every rejection reason listed above where admission owns the reason.
- It proves `cmd_interact_oob` rejects exactly `OutOfRange`.
- It proves retry accepts only after movement into reach.
- It proves admission is read-only and does not append to command log.
- It proves first-failure order.
- It covers every acceptance-demo command at the admission level.
