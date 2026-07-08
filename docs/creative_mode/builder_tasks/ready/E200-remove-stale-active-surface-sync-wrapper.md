# E200: Remove Stale Active Surface Sync Wrapper

## Status

Ready.

## Context

E199 proved that `syncProductWindowInputOwnerFromActiveSurface(...)` is now a
misnamed compatibility wrapper. It calls
`resolveProductActiveSurface(productActiveSurfaceContextForWindow(...))` and
returns the frame, but it does not mutate `ProductAppWindowState`.

There are no production consumers of the returned frame. Production call sites
are bare no-op calls left over from the deleted input-owner cache era. Focused
tests/support still consume the returned frame for active-surface assertions.

## Objective

Remove no-op production calls to
`syncProductWindowInputOwnerFromActiveSurface(...)`, repoint tests/support to
the real resolver seam, and delete the stale helper declaration/definition
without changing active-surface routing behavior.

## Scope

Edit only the active-surface sync tail and focused tests/support:

- `src/app/iggy3d/automation/AutomationControl.cpp`
- `src/app/iggy3d/automation/AutomationGameplay.cpp`
- `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/window/Loop.cpp`
- `src/app/iggy3d/menu/Transitions.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/menu/FrontendRouter.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `tests/unit/ProductActiveSurfaceTestSupport.hpp`
- `tests/unit/product_frontend_router_tests.cpp`
- `tests/unit/product_interaction_mode_state_tests.cpp`
- this task card

Do not edit CMake, receipt golden, active-surface routing policy,
`resolveProductActiveSurface(...)`, `productActiveSurfaceContextForWindow(...)`,
or unrelated tests.

## Required Production Removals

Remove the ignored production calls identified by E199:

- `src/app/iggy3d/automation/AutomationControl.cpp`
  - `applyProductAutomationControl(...)`: the call around E199 line 51.
- `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `applyGameplayActionState(...)`: the call around E199 line 68.
  - `applyGameplayJumpActionState(...)`: the call around E199 line 91.
- `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `recordProductRoomEditingLeave(...)`: the call around E199 line 149.
  - room-editing automation input dispatch in the command application loop:
    calls around E199 lines 732-733.
- `src/app/iggy3d/window/InputFrame.cpp`
  - `applyProductWindowInputActionsImpl(...)`: the call around E199 line 424.
  - `cancelProductRoomEditorPendingPreviewFromBack(...)`: the call around E199
    line 736.
  - `applyProductWindowEditorMousePickPreview(...)`: the call around E199 line
    981.
- `src/app/iggy3d/window/Loop.cpp`
  - `runProductWindowLoop(...)`: the explicit `(void)` call around E199 line
    138.
- `src/app/iggy3d/menu/Transitions.cpp`
  - `applyReturnProductToTitleTransition(...)`: the call around E199 line 154.
  - `initializeProductStarterTransition(...)`: the call around E199 line 169.
  - `enterProductGameplayTransition(...)`: the call around E199 line 178.
  - `openProductPauseTransition(...)`: the call around E199 line 187.
  - `openProductPauseSettingsTransition(...)`: the call around E199 line 201.
  - `openProductPauseDevToolsTransition(...)`: the call around E199 line 212.
  - `closeProductOverlayToGameplayTransition(...)`: the call around E199 line
    220.
- `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `openStarterDevTools(...)`: the call around E199 line 169.
  - `applyProductGameplayMapMakerToggleAction(...)`: the call around E199 line
    821.

Use current code context rather than trusting the old line numbers blindly.

## Required Test/Support Repoints

- Change `tests/unit/ProductActiveSurfaceTestSupport.hpp::liveSurface(...)` to
  return:

  ```cpp
  resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))
  ```

- In `tests/unit/product_frontend_router_tests.cpp`, repoint the direct sync
  helper call to:

  ```cpp
  resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))
  ```

- Rename `inputOwnerCacheSyncUsesResolvedActiveSurface()` to a non-cache,
  non-sync name such as `windowContextResolveMatchesExpectedActiveSurface()`,
  and update the `main()` call.

- In `tests/unit/product_interaction_mode_state_tests.cpp`, compute
  `surfaceAfterLeave` with direct
  `resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))`.

- After all callers are repointed, delete
  `syncProductWindowInputOwnerFromActiveSurface(...)` from
  `FrontendRouter.hpp` and `FrontendRouter.cpp`.

## Required Checks

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_frontend_router_tests product_menu_transitions_tests product_starter_menu_action_tests product_window_input_frame_tests product_interaction_mode_state_tests product_automation_dispatch_tests product_room_editor_action_controller_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_frontend_router_tests|product_menu_transitions_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_automation_dispatch_tests|product_room_editor_action_controller_tests)$' --output-on-failure

rg -n "syncProductWindowInputOwnerFromActiveSurface\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'

git -C /Users/kogaryu/iggy3d diff --check
```

The `rg` check must return no hits.

Also run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report exact evidence instead of widening scope if:

- any removed production call has observable receipt/test behavior;
- any production code still needs the returned active-surface frame;
- direct resolver use in tests differs from the old sync helper result;
- the work requires CMake, receipt golden, or routing-policy changes.

Do not reintroduce a hidden cache or a renamed wrapper unless production code
actually consumes the returned frame.

## Completion Brief Requirements

Report:

- production no-op calls removed, grouped by file/function;
- test/support direct resolver repoints made;
- confirmation that the stale helper declaration/definition was deleted;
- final `rg` result for `syncProductWindowInputOwnerFromActiveSurface(...)`;
- focused build/CTest results;
- `git diff --check` and trailing-whitespace results;
- confirmation that active-surface routing policy, receipt golden, CMake, and
  unrelated tests were not changed.
