# E177 - God Struct Remainder Audit

## Status

Ready.

## Objective

Do a read-only current-state audit of the remaining top-level
`ProductAppWindowState` members after E176. Do not implement moves or deletes in
this card.

The store folds are now complete through FrontendWindowShell. The remaining
fields are a smaller mixed set, and the next implementation should be based on a
fresh map rather than assumptions from older audit counts.

## Scope

Inspect:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `docs/god_struct_member_ownership.tsv`
- `docs/god_struct_decomposition_target_map.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- direct source/test call sites for the remaining top-level fields

Current expected remaining top-level fields at E176 are:

- app-global/lifecycle candidates: `requested`, `sdlAvailable`, `created`,
  `drawable`, `automationControl`
- existing store members: `frontendShell`, `inputDevice`, `debugHud`,
  `creativeAuthoring`, `gameplay`, `room`, `saveSession`, `viewport`,
  `presentPath`
- cleanup/freshness candidates: `runtimeStateHash`, `creativeWorldEpoch`

## Do Not Edit

- No source changes.
- No tests/CMake changes.
- No receipt golden changes.
- Do not move or delete `runtimeStateHash`.
- Do not move or delete `creativeWorldEpoch`.
- Do not move `automationControl`.
- Do not create implementation cards in `ready/` unless the audit result is
  exact and reviewable. Draft follow-up card text inside this done card instead.

## Required Audit

1. List the current top-level `ProductAppWindowState` members in declaration
   order and count them.
2. Reconcile every current top-level member against
   `docs/god_struct_member_ownership.tsv`.
3. Reconcile the remaining fields against
   `docs/god_struct_decomposition_target_map.md`, especially the claim that the
   final target is a thin composition plus app-global lifecycle bits.
4. Count direct call sites for:
   - `requested`
   - `sdlAvailable`
   - `created`
   - `drawable`
   - `automationControl`
   - `runtimeStateHash`
   - `creativeWorldEpoch`
5. Classify each remaining scalar as:
   - keep app-global;
   - delete/rederive candidate;
   - move into an existing store;
   - needs owner decision before implementation.
6. Specifically answer:
   - Is `runtimeStateHash` still used, and can it be deleted safely in one
     future card?
   - Is `creativeWorldEpoch` correctly owned by `ViewportStore`, or should it
     remain app-global because it is a window-owned generation token?
   - Is `automationControl` correctly app-global, or is it a store candidate
     with too much blast radius?

## Suggested Commands

```sh
sed -n '1,140p' src/app/iggy3d/ProductAppWindowState.hpp
sed -n '1,80p' docs/god_struct_member_ownership.tsv
rg -n "runtimeStateHash|creativeWorldEpoch|automationControl|requested|sdlAvailable|drawable|\\bcreated\\b" src/app/iggy3d tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "runtimeStateHash|creativeWorldEpoch|automationControl|app-global-remainder|delete|ViewportStore" docs/god_struct_decomposition_target_map.md docs/god_struct_member_ownership.tsv docs/creative_mode/builder_tasks/PRIORITY.md
git -C /Users/kogaryu/iggy3d diff --check
```

Use additional `rg` commands as needed, but report the exact commands and
counts.

## Deliverable

Append the audit to this card and move it to `done/`.

The completion brief must include:

- current top-level member count and ordered member list;
- TSV reconciliation result;
- direct call-site counts by field and hottest files;
- classification table for remaining scalars;
- recommendation for the next one or two implementation cards;
- whether any stale wording remains in `docs/god_struct_decomposition_target_map.md`
  or `PRIORITY.md`;
- confirmation that no source/test/CMake/receipt golden files were edited.

## Verification

Run:

```sh
git -C /Users/kogaryu/iggy3d status --short
git -C /Users/kogaryu/iggy3d diff --check
```

No build or broad CTest is required for this read-only audit unless the builder
edits files beyond the task card by mistake.

## Completion Brief

### Files inspected

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `docs/god_struct_member_ownership.tsv`
- `docs/god_struct_decomposition_target_map.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- Direct source/test call sites under `src/app/iggy3d` and `tests/unit`.

### Current top-level members

Current top-level `ProductAppWindowState` member count: **16**.

Declaration order:

1. `requested`
2. `sdlAvailable`
3. `created`
4. `drawable`
5. `frontendShell`
6. `inputDevice`
7. `debugHud`
8. `creativeAuthoring`
9. `gameplay`
10. `room`
11. `saveSession`
12. `runtimeStateHash`
13. `creativeWorldEpoch`
14. `viewport`
15. `automationControl`
16. `presentPath`

Command used:

```sh
perl -ne 'if (/^\s*(?:bool|std::uint64_t|FrontendWindowShell|InputDeviceStore|DebugHudStore|CreativeAuthoringStore|GameplayStore|ProductRoomStore|SaveSessionStore|ProductViewportState|ProductAutomationControlState|PresentPathStore)\b.*?\s+([A-Za-z_][A-Za-z0-9_]*)\b/) { print "$1\n"; }' src/app/iggy3d/ProductAppWindowState.hpp
```

### TSV reconciliation

`docs/god_struct_member_ownership.tsv` has exactly one row for each current
top-level member, and no extra rows.

Command used:

```sh
comm -3 <(perl -ne 'if (/^\s*(?:bool|std::uint64_t|FrontendWindowShell|InputDeviceStore|DebugHudStore|CreativeAuthoringStore|GameplayStore|ProductRoomStore|SaveSessionStore|ProductViewportState|ProductAutomationControlState|PresentPathStore)\b.*?\s+([A-Za-z_][A-Za-z0-9_]*)\b/) { print "$1\n"; }' src/app/iggy3d/ProductAppWindowState.hpp | sort) <(cut -f1 docs/god_struct_member_ownership.tsv | sort)
```

Result: no output.

Current TSV owner map:

| Member | TSV owner | Reconciled state |
| --- | --- | --- |
| `requested` | `app-global-remainder` | matches lifecycle scalar |
| `sdlAvailable` | `app-global-remainder` | matches lifecycle scalar |
| `created` | `app-global-remainder` | matches lifecycle scalar |
| `drawable` | `app-global-remainder` | matches lifecycle scalar |
| `frontendShell` | `FrontendWindowShell` | existing store |
| `inputDevice` | `InputDeviceStore` | existing store |
| `debugHud` | `DebugHudStore` | existing store |
| `creativeAuthoring` | `CreativeAuthoringStore` | existing store |
| `gameplay` | `GameplayStore` | existing store |
| `room` | `RoomStore` | existing store |
| `saveSession` | `SaveSessionStore` | existing store |
| `runtimeStateHash` | `delete` | still top-level and still used; delete/rederive remains a future card |
| `creativeWorldEpoch` | `ViewportStore` | still top-level; should move under `viewport` or its creative-fly substore |
| `viewport` | `ViewportStore` | existing store |
| `automationControl` | `app-global-remainder` | matches E164 ruling |
| `presentPath` | `PresentPathStore` | existing store |

### Direct call-site counts

Count command:

```sh
rg -n "\b(window|request\.window)\.(requested|sdlAvailable|created|drawable|automationControl|runtimeStateHash|creativeWorldEpoch)\b" src/app/iggy3d tests/unit --glob '*.cpp' --glob '*.hpp' |
  perl -ne 'while (/\b(?:window|request\.window)\.(requested|sdlAvailable|created|drawable|automationControl|runtimeStateHash|creativeWorldEpoch)\b/g) { $c{$1}++; } END { for $k (sort keys %c) { print "$k $c{$k}\n"; }}'
```

Results:

| Field | Direct-window occurrences | Hottest files |
| --- | ---: | --- |
| `requested` | 6 | `src/app/iggy3d/window/Loop.cpp` 2; one each in `AppKernel.cpp`, `ReceiptBuilder.cpp`, `receipt/FrontendSettingsWindowFields.cpp`, `tests/unit/product_creative_ui_window_frame_tests.cpp` |
| `sdlAvailable` | 3 | `src/app/iggy3d/window/Loop.cpp` 2; `receipt/FrontendSettingsWindowFields.cpp` 1 |
| `created` | 7 | `src/app/iggy3d/window/Loop.cpp` 4; one each in `AppKernel.cpp`, `ReceiptBuilder.cpp`, `receipt/FrontendSettingsWindowFields.cpp` |
| `drawable` | 6 | `src/app/iggy3d/window/Loop.cpp` 4; one each in `FramePresenter.cpp`, `receipt/FrontendSettingsWindowFields.cpp` |
| `automationControl` | 133 | `automation/Automation.cpp` 58; `automation/AutomationRoomEditing.cpp` 29; `automation/AutomationControl.cpp` 11; `receipt/FeedbackSurfaceAutomationVulkanFields.cpp` 11; `automation/AutomationGameplay.cpp` 8 |
| `runtimeStateHash` | 24 | `gameplay/Controller.cpp` 8; `Operations.cpp` 4; `tests/unit/product_creative_fly_tests.cpp` 3; `tests/unit/product_creative_world_launch_tests.cpp` 2; one each in `ascii_room/Activation.cpp`, `automation/AutomationGameplay.cpp`, `gameplay/ProjectionRefresh.cpp`, `gameplay/TapeRunner.cpp`, `receipt/GameplayRuntimeMovementFields.cpp`, `window/FramePresenter.cpp`, `tests/unit/product_ascii_room_activation_tests.cpp` |
| `creativeWorldEpoch` | 23 | `tests/unit/product_creative_fly_tests.cpp` 8; `tests/unit/product_creative_world_launch_tests.cpp` 7; `view/CreativeFlyAnchorStore.cpp` 4; `gameplay/ProjectionRefresh.cpp` 2; `tests/unit/product_vulkan_room_frame_tests.cpp` 2 |

Per-file count command:

```sh
rg -n "\b(window|request\.window)\.(requested|sdlAvailable|created|drawable|automationControl|runtimeStateHash|creativeWorldEpoch)\b" src/app/iggy3d tests/unit --glob '*.cpp' --glob '*.hpp' |
  perl -ne 'if (/^([^:]+):/) { $f=$1; while (/\b(?:window|request\.window)\.(requested|sdlAvailable|created|drawable|automationControl|runtimeStateHash|creativeWorldEpoch)\b/g) { $c{"$1 $f"}++; }} END { for $k (sort keys %c) { print "$k $c{$k}\n"; }}'
```

### Scalar classification

| Field | Classification | Reason |
| --- | --- | --- |
| `requested` | keep app-global | Window-loop lifecycle bit. Set in `window/Loop.cpp`, read by app-exit/receipt paths. |
| `sdlAvailable` | keep app-global | SDL/window substrate availability, not a domain store. |
| `created` | keep app-global | Native window creation status, used by loop/app-kernel failure handling and receipts. |
| `drawable` | keep app-global | Current drawable status, used by frame presenter and loop. |
| `automationControl` | keep app-global | Correctly app-global per E164. It is already a nested automation driver/control state, has 133 direct hits across automation command routing and receipts, and is not FrontendWindowShell state. Moving it now would be broad and low-value unless a future dedicated `AutomationControlStore` is explicitly designed. |
| `runtimeStateHash` | delete/rederive candidate | Still live: 24 direct-window occurrences. The target-map delete direction is conceptually right, but the field cannot be removed by a blind delete. Readers need a resolver or explicit hash input from the active session/tape result/no-session fallback. |
| `creativeWorldEpoch` | move into existing store | TSV says `ViewportStore`, and current call sites confirm it only serves creative-fly anchor freshness. It should not stay app-global just because it is window-owned; move it to `window.viewport.creativeWorldEpoch` or the nested creative-fly anchor store in a small follow-up. |

### Specific answers

- Is `runtimeStateHash` still used?
  - Yes. It is written from session hashes in `Operations.cpp`, `gameplay/Controller.cpp`, `gameplay/ProjectionRefresh.cpp`, `gameplay/TapeRunner.cpp`, `ascii_room/Activation.cpp`, and `automation/AutomationGameplay.cpp`.
  - It is read by receipt emission in `receipt/GameplayRuntimeMovementFields.cpp` and by `window/FramePresenter.cpp` to pass the hash into the SDL gameplay panel.
- Can `runtimeStateHash` be deleted safely in one future card?
  - Yes, if the card is an explicit delete/rederive slice rather than a mechanical removal. The slice needs to replace the window mirror with a resolver that derives from the active `Session` where available and returns `0` for no-session paths, while preserving tape/ascii activation behavior through their existing local result structs. `FramePresenter` currently has no `activeSession` in `ProductWindowFramePresenterRequest`, so that seam must be part of the delete card.
- Is `creativeWorldEpoch` correctly owned by `ViewportStore`?
  - Yes. The direct users are `view/CreativeFlyAnchorStore.cpp`, `gameplay/ProjectionRefresh.cpp`, and focused tests. It is a freshness token for `window.viewport.creativeFlyAnchor`, not general app-global state.
- Should `creativeWorldEpoch` remain app-global because it is a window-owned generation token?
  - No. It is window-owned in the same sense `viewport` is window-owned, but the domain owner is viewport/creative-fly freshness. The current top-level location contradicts the TSV owner.
- Is `automationControl` correctly app-global?
  - Yes for the current architecture. It is cross-cutting automation-driver state with its own nested struct and receipt namespace. It should not be folded into FrontendWindowShell. A future dedicated automation store is possible, but not needed to finish the god-struct decomposition.

### Target-map and priority wording

Stale or incomplete wording remains:

- `docs/god_struct_decomposition_target_map.md:41` says `runtimeStateHash` is among deletes that "are removed"; it is still present.
- `docs/god_struct_decomposition_target_map.md:100` says `window.runtimeStateHash` is removed; it is still present.
- `docs/god_struct_decomposition_target_map.md:102` still references old line ranges and the delete set with `runtimeStateHash`; this needs a post-E177 wording update once the delete/rederive card lands.
- `docs/creative_mode/builder_tasks/PRIORITY.md:13-15` says the god-struct was 153 members after old deletes; that is stale relative to the current 16-member remainder.
- `docs/creative_mode/builder_tasks/PRIORITY.md:115` still lists `runtimeStateHash` as a delete cleanup; that is accurate as pending work, but should be split from the already-complete `inputOwner`/`gameplayInputSuppressed` deletion wording.

### Recommended next implementation cards

Do not create these in `ready/` until reviewer approves; draft text only.

#### Draft next card 1: RuntimeStateHash delete/rederive

Objective: remove the top-level `ProductAppWindowState::runtimeStateHash`
mirror and rederive the runtime hash at its consumers.

Scope:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- `src/app/iggy3d/automation/AutomationGameplay.cpp`
- `src/app/iggy3d/gameplay/Controller.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- `src/app/iggy3d/gameplay/TapeRunner.cpp`
- `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
- `src/app/iggy3d/window/FramePresenter.hpp/.cpp`
- focused tests that currently seed/assert the window mirror.

Policy:

- No session available -> receipt/panel hash is `0`.
- Active session available -> derive from `activeSession->stateHash()`.
- Keep local result structs such as ascii activation/tape runner hashes as local operation results, not window mirrors.
- Preserve receipt golden key order and values.

Acceptance:

- No `window.runtimeStateHash` / `request.window.runtimeStateHash` direct hits remain.
- `ProductAppWindowState` no longer declares `runtimeStateHash`.
- `docs/god_struct_member_ownership.tsv` no longer has `runtimeStateHash`.
- Gameplay receipt and SDL gameplay panel still show the same hash values in focused tests.

#### Draft next card 2: Move creativeWorldEpoch into ViewportStore

Objective: move the top-level `ProductAppWindowState::creativeWorldEpoch` into
`ProductViewportState` because it is the freshness token for
`viewport.creativeFlyAnchor`.

Scope:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/view/ViewportState.hpp`
- `src/app/iggy3d/view/CreativeFlyAnchorStore.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- focused creative-fly/world-launch/vulkan-room tests that currently seed/assert the top-level field.

Policy:

- Move only `creativeWorldEpoch`.
- Do not reshape `ProductCreativeFlyAnchorStore` unless reviewer chooses that as the owner instead of `ProductViewportState`.
- Preserve current epoch bump/freshness semantics and test expectations.

Acceptance:

- No `window.creativeWorldEpoch` direct hits remain.
- `ProductAppWindowState` no longer declares `creativeWorldEpoch`.
- `ProductViewportState` owns the epoch, or the nested creative-fly anchor store owns it if reviewer revises the card.
- Creative-fly anchor freshness tests remain green.

### Verification

Commands run:

```sh
git -C /Users/kogaryu/iggy3d status --short
git -C /Users/kogaryu/iggy3d diff --check
```

No source, test, CMake, or receipt-golden files were edited. Only this task card
was appended and moved from `ready/` to `claimed/` during the audit.
