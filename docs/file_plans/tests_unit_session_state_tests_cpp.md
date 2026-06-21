# `tests/unit/session_state_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove `SessionState` and the `Session` facade create, own,
reset, expose, and replace runtime state according to the complete `iggy3d`
architecture without renderer, app, raw input, network, or old `iggy`
dependencies.

## Build Position

- priority rank: 66
- tier: Tier 4: Time Camera Command Session Base
- module: `unit tests`
- file kind: `test`

This test file is the executable guardrail for:

- `src/runtime/session/SessionState.hpp`;
- `src/runtime/session/Session.hpp`;
- `src/runtime/session/Session.cpp`;
- the ownership boundaries in `docs/ownership.md`;
- the first-room creation/reset parts of `docs/acceptance_demo.md`.

## Ownership

This file owns:

- test fixture builders for session-state scenarios;
- assertions for default state;
- assertions for first-room session creation;
- assertions for state field ownership;
- assertions for baseline reset;
- assertions for command log integration at the session boundary;
- assertions for all-or-nothing load replacement;
- assertions that transient/derived data is not save truth;
- assertions that session state exposes no renderer/app/raw input state.

It must not own:

- production session creation logic;
- content parsing implementation;
- command admission implementation;
- movement/interaction/combat/AI/objective implementation;
- save codec implementation;
- replay loop implementation;
- renderer/window setup;
- raw input conversion;
- network transport;
- old `/Users/kogaryu/iggy` adapters.

## Required Test File Shape

The implementation file must be:

```text
tests/unit/session_state_tests.cpp
```

Required include categories:

```cpp
#include <test framework header>

#include "config/RuntimeConfig.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionState.hpp"
```

Add subsystem headers only when direct state assertions require public types.
Do not include app, projection, renderer, socket, private test-only production
hooks, or old `iggy` headers.

## Test Fixture Contract

Tests may use the public first-room fixture loader or a local deterministic
seed builder. The resulting seed must match:

- package id `iggy3d.first_room`;
- scenario id `first_room.runtime_loop`;
- entity count `3`;
- entity `player`, id `1`, kind `Player`, active, position
  `(0.000,0.000,0.000)`;
- entity `gold_key`, id `2`, kind `Pickup`, active, position
  `(3.000,0.000,0.000)`;
- entity `tactical_marker_alpha`, id `3`, kind `Marker`, active, position
  `(2.000,0.000,1.000)`;
- player slot `0`, kind `Local`, bound to `player`;
- objective `collect_gold_key`, status `Active`;
- inventory empty;
- clock `Normal`;
- camera `ThirdPerson`;
- command log empty.

Recommended local helper concepts:

```cpp
SessionCreateRequest makeFirstRoomCreateRequest();
Session makeFirstRoomSession();
CommandRecord makeInteractGoldKeyCommand();
CommandRecord makeMoveToKeyCommand();
SessionState cloneState(const Session& session);
```

Fixture helpers are test-local unless they already exist as public content
helpers.

## Required Assertion Categories

### 1. Default State

Tests:

- `defaultSessionState_isLoadingAndEmpty`;
- `defaultSession_hasLoadingState`;
- `defaultSession_containsNoRendererOrAppState`.

Expected:

- lifecycle `Loading`;
- outcome `None`;
- package id empty;
- scenario id empty;
- command log empty;
- player roster empty or explicitly invalid;
- world empty;
- no renderer handle field exists;
- no app path field exists;
- no raw input field exists.

If C++ cannot directly assert absence of fields, rely on include boundaries and
public API: tests must not need renderer/app/input headers to compile.

### 2. Session Creation From First-Room Seed

Tests:

- `createFirstRoomSession_setsIdentity`;
- `createFirstRoomSession_createsWorldEntitiesInOrder`;
- `createFirstRoomSession_bindsPlayerSlotZero`;
- `createFirstRoomSession_initializesClockAndCamera`;
- `createFirstRoomSession_initializesInventoryObjectiveAndCommandLog`;
- `createFirstRoomSession_buildsBaseline`.

Expected:

- `identity.packageId == "iggy3d.first_room"`;
- `identity.scenarioId == "first_room.runtime_loop"`;
- lifecycle `Playing`;
- outcome `None`;
- world has exactly three entities in deterministic id/order;
- player slot 0 exists;
- player slot 0 actor id is player id;
- player transform is `(0.000,0.000,0.000)`;
- `gold_key.active == true`;
- inventory is empty;
- objective `collect_gold_key == Active`;
- clock mode `Normal`;
- camera mode `ThirdPerson`;
- command log empty;
- baseline snapshot is valid.

### 3. Creation Failure Behavior

Tests:

- `createWithMissingPlayer_fails`;
- `createWithMissingPlayerSlotBinding_fails`;
- `createWithValidatedSeedUsesContentOwnedStableNameGuarantee`;
- `createWithInvalidScenarioId_fails`;

Expected:

- creation returns failed `Result<Session>` with structured failure;
- no partially valid session is returned as playable;
- failure is stable status/diagnostic, not prose-only text;
- session creation does not mutate global state.

Duplicate stable-name rejection is owned by package/content validation tests.
This session test uses a validated seed and asserts that session creation relies
on the content-owned stable-name guarantee instead of re-scanning duplicates.

### 4. State Ownership Shape

Tests:

- `sessionState_containsAuthoritativeRuntimeState`;
- `sessionState_separatesTransientFromSaveTruth`;
- `sessionState_excludesProjectionRendererAppAndRawInput`;
- `sessionState_ownsBaselineForReset`.

Expected:

- state exposes world, players, clock, camera, command log, inventory, combat,
  AI, and objectives through owned aggregate fields or accessors;
- transient events/metrics are clearly separate;
- projection/debug output is absent from authoritative state;
- renderer handles are absent;
- raw input events are absent;
- baseline snapshot can restore initial runtime truth.

### 5. Command Submission Boundary

Tests:

- `submitRejectedCommand_appendsRejectedRecord`;
- `submitRejectedCommand_doesNotExecuteInteraction`;
- `submitAcceptedMove_appendsAcceptedRecordBeforeExecution`;
- `submitCommand_usesAdmissionAndDoesNotBypassIt`;
- `submitCommand_preservesCommandSourceSlotAndActor`.

Expected:

- submitting baseline `Interact(player,gold_key)` logs rejected command;
- rejection reason is `OutOfRange`;
- `gold_key.active` remains `true`;
- inventory remains empty;
- objective remains `Active`;
- command log has one rejected record;
- accepted command record has `rejection=None`;
- tests prove command submission path used admission, not direct log injection.

Accepted gameplay execution is queued for `SessionTick`; this test must not
expect movement mutation until the queued command sequence is consumed by tick.

### 6. Retry Boundary

Tests:

- `retryCanFindRejectedSourceThroughSessionCommandLog`;
- `retryBeforeMovementStillRejectsOrUsesCurrentAdmission`;
- `retryAfterMovementCanBeAcceptedWhenInReach`;

Expected:

- rejected source `CommandRecord::commandId` is stored in command log;
- `SessionState::nextCommandId` advances only after the rejected source command is
  successfully appended;
- `CommandPayload::retrySourceCommandId` references source `commandId`, not a
  debug label, sequence, or vector index;
- session route uses command log lookup;
- retry does not erase original rejected command;
- retry does not bypass admission;
- session-state retry tests prove retry payload/source linkage,
  lifecycle/admission/logging behavior, and preservation of the original
  rejected command.

### Accepted Command Queue

Tests:

- `accepted_move_enqueues_once_after_append`;
- `accepted_retry_enqueues_retry_sequence_once`;
- `rejected_interact_does_not_enqueue`;
- `immediate_control_command_does_not_enqueue`.

Expected:

- enqueue happens only after `CommandLog::append` returns `Ok`;
- queued value is the stored `CommandRecord::sequence`, not a vector index or
  command label;
- accepted `Retry` queues its own sequence and is normalized by `SessionTick`;
- failed append leaves `pendingExecutionSequences` and `nextCommandId`
  unchanged.

Geometry-specific distance cases remain covered by target/reach tests, but the
session boundary proof is required here.

### 7. Clock Camera Session Control

Tests:

- `pauseCommand_updatesClockThroughSessionControl`;
- `resumeCommand_restoresPreviousRunMode`;
- `toggleTactical_entersSlowModeAndTacticalCamera`;
- `toggleTacticalAgain_restoresNormalAndRealtimeCamera`;
- `stepWhilePaused_executesExactlyOneTickThroughSessionRunner`.

Expected:

- session-control commands are admitted/logged;
- clock changes are owned by clock/session control path;
- camera changes are owned by camera mode policy;
- tactical entry stores previous realtime camera;
- paused state blocks automatic tick;
- step behavior matches `Session.cpp` plan.

The complete build must cover the public session boundary with the real `Clock`
and `CameraModePolicy` implementations. A staged substitute does not satisfy
this test plan.

### 8. Reset Baseline

Tests:

- `resetRestoresBaselineWorld`;
- `resetRestoresPlayerRoster`;
- `resetRestoresClockCameraInventoryObjective`;
- `resetClearsCommandLogUnderClearPolicy`;
- `resetRecomputesHashToBaseline`;
- `resetDoesNotParseFixtureFiles`.

Expected after reset:

- lifecycle `Playing`;
- outcome `None`;
- player position `(0.000,0.000,0.000)`;
- `gold_key.active == true`;
- inventory empty;
- objective `collect_gold_key == Active`;
- clock `Normal`;
- camera `ThirdPerson`;
- command log empty under clear policy;
- current hash equals baseline hash if hash is implemented;
- no app/file parser is called by reset.

### 9. Load Replacement

Tests:

- `replaceStateFromLoad_replacesValidState`;
- `replaceStateFromLoad_rejectsInvalidStateWithoutMutation`;
- `replaceStateFromLoad_clearsTransientState`;
- `replaceStateFromLoad_preservesLoadedCommandLog`;
- `replaceStateFromLoad_recomputesOrValidatesHash`.

Expected:

- valid loaded state replaces active state;
- invalid loaded state returns exact `SessionLoadStatus` and leaves previous
  state byte/logically unchanged;
- transient events are cleared and deterministic metrics are regenerated from
  loaded authoritative state;
- command log records are preserved from loaded state;
- no file IO occurs in session.

Invalid candidate examples:

- missing player slot 0;
- player slot actor missing from world;
- invalid lifecycle/identity;
- command log with inconsistent accepted/rejected fields if validation happens
  here.
- zero or stale `nextCommandId` returns `InvalidCommandIdCursor`;
- invalid command log restore facts return `InvalidCommandLogState`;
- invalid lifecycle/outcome pair returns `InvalidLifecycleState`;
- final replacement rejection after validation returns `ReplacementRejected`.

### 10. Save Truth Boundary

Tests:

- `sessionStateSaveTruth_includesAuthoritativeFields`;
- `sessionStateSaveTruth_excludesTransientAndDerivedFields`;
- `sessionStateHash_excludesCurrentStateHashSelfReference`;
- `sessionStateHashPolicyForCommandLog_isExplicit`.

Expected:

- identity, lifecycle, outcome, world, players, clock, camera, command log,
  inventory, combat, AI, and objectives are save truth;
- transient events and metrics are excluded unless explicit debug section exists;
- projection/debug output excluded;
- current hash is metadata, not hashed into itself;
- command log inclusion/exclusion in hash is tested according to chosen policy.

### 11. Deterministic Ordering

Tests:

- `entityOrder_isDeterministicFromSeed`;
- `playerSlotOrder_isDeterministic`;
- `commandLogOrder_isDeterministic`;
- `stateHashTraversalOrder_isDocumentedByStateHashTests`.

Expected:

- no unordered traversal determines externally visible order;
- entity ids match fixture order;
- command sequence order is append order;
- replay/hash can traverse deterministically.

### 12. Multiplayer Shape

Tests:

- `sessionStateSupportsMultiplePlayerSlots`;
- `commandSubmissionPreservesPlayerSlot`;
- `sessionDoesNotAssumeOnlySlotZeroOutsideFixtureCreation`;

Expected:

- roster can hold at least local slot 0 plus another test slot if API supports
  it;
- command record submitted through session preserves `playerSlot`;
- actor/slot binding checks remain data-driven.

Networking is not part of this test.

## Acceptance Demo Coverage

This unit test does not run the full acceptance demo, but it must prove the
session state pieces the demo depends on:

| Acceptance Fact | Unit Proof |
| --- | --- |
| first-room session creation | identity, world, player, clock, camera, objective initial state |
| first rejected interaction | command log receives rejected `OutOfRange` command, no mutation |
| movement command boundary | accepted move logs without bypassing admission |
| retry source | rejected source command remains findable |
| tactical control | session can mutate clock/camera through control path |
| pause/step | session exposes exactly-one-step path |
| reset | baseline restore and command log clear policy |
| save/load | all-or-nothing state replacement boundary |

## Diagnostics And Errors

Tests should assert structured values:

- lifecycle values;
- outcome values;
- command admission status;
- rejection reason `OutOfRange`;
- command log counts;
- load/reset result booleans;
- state hash equality when available.

Tests must not assert long human diagnostic prose.

## Save Replay Multiplayer Notes

Save/load:

- tests verify session state has all fields needed by save mapping;
- tests verify load replacement does not partially mutate active session.

Replay:

- tests verify command log/state are sufficient for replay to re-submit
  commands normally;
- full replay behavior belongs to replay tests.

Multiplayer:

- tests verify player slot is part of state and command submission;
- transport is out of scope.

## Compute Cost

Unit fixture size is tiny:

- three entities;
- one required player slot;
- one objective;
- command log usually zero to a few records.

Tests must not use random generated worlds. Larger deterministic stress tests,
if needed later, should live in a separate performance or integration target.

## Required Test Organization

Recommended suites:

1. `SessionStateDefault`;
2. `SessionCreate`;
3. `SessionOwnershipShape`;
4. `SessionCommandSubmission`;
5. `SessionControlCommands`;
6. `SessionReset`;
7. `SessionLoadReplacement`;
8. `SessionSaveTruth`;
9. `SessionDeterminism`;
10. `SessionMultiplayerShape`.

Each test name should include the exact behavior and expected status.

## Completion Criteria

- `tests/unit/session_state_tests.cpp` exists in `/Users/kogaryu/iggy3d`.
- It builds through CTest without app shell, renderer, network, or old `iggy`.
- It proves default and created session state.
- It proves first-room identity/world/player/clock/camera/objective state.
- It proves rejected command logging and no execution.
- It proves reset baseline behavior.
- It proves all-or-nothing load replacement.
- It proves save truth excludes transient/derived/app/renderer/raw input state.
- It proves deterministic ordering assumptions needed by replay/hash.
- It covers the session-level pieces of the acceptance demo.
