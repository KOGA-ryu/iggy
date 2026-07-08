# E199: Active Surface Sync Tail Audit

## Status

Done.

## Context

The old window input-owner cache fields were deleted. The remaining helper
`syncProductWindowInputOwnerFromActiveSurface(...)` appears to be a stale name:
it now resolves and returns a `ProductActiveSurfaceFrame`, but does not write
input-owner or gameplay-suppression state back into `ProductAppWindowState`.

Quick reviewer evidence before this card:

- `src/app/iggy3d/menu/FrontendRouter.cpp:384-390` currently calls
  `resolveProductActiveSurface(productActiveSurfaceContextForWindow(...))` and
  returns the frame.
- many production call sites call it as a bare statement and ignore the return;
- test helpers and a few focused tests still consume the returned frame.

This card is read-only. Do not remove calls or rename the helper in this card.

## Objective

Classify all remaining `syncProductWindowInputOwnerFromActiveSurface(...)`
callers so the next implementation can safely remove no-op call sites and/or
rename the helper without changing active-surface routing behavior.

## Scope

Inspect only:

- `src/app/iggy3d/menu/FrontendRouter.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- all production call sites under `src/app/iggy3d`
- direct test/support call sites under `tests/unit`

Do not edit source, tests, CMake, docs outside this task card, or receipt
golden.

## Required Inventory

Document:

- the current implementation of `syncProductWindowInputOwnerFromActiveSurface`;
- whether it mutates `ProductAppWindowState`;
- every production call site that ignores the return;
- every production call site that consumes the return, if any;
- every test/support call site that consumes the return;
- whether `tests/unit/ProductActiveSurfaceTestSupport.hpp::liveSurface(...)`
  should keep wrapping the helper or switch to `resolveProductActiveSurface(...)`
  directly in a follow-up;
- whether the helper name is now misleading enough to rename/delete;
- which focused tests cover active-surface routing if callers are removed.

## Required Commands

Run and summarize:

```sh
rg -n "syncProductWindowInputOwnerFromActiveSurface\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'

rg -n "const .*syncProductWindowInputOwnerFromActiveSurface|=\\s*syncProductWindowInputOwnerFromActiveSurface|return syncProductWindowInputOwnerFromActiveSurface|\\(void\\)syncProductWindowInputOwnerFromActiveSurface" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'

rg -n "inputOwner|gameplayInputSuppressed|ProductActiveSurfaceFrame|resolveProductActiveSurface" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/menu/FrontendRouter.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/menu/FrontendRouter.hpp \
  /Users/kogaryu/iggy3d/tests/unit/product_frontend_router_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_menu_transitions_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_starter_menu_action_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/ProductActiveSurfaceTestSupport.hpp

git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required because this card is read-only.

## Decision Buckets

Classify the next implementation as one of:

1. **Remove Bare No-Ops, Keep Return Helper**: production bare calls are no-ops,
   but return-consuming tests/support still need the helper for now.
2. **Rename To Resolve Helper And Repoint Tests**: the helper should be renamed
   or replaced by direct `resolveProductActiveSurface(...)` usage, with bare
   production calls removed.
3. **Still Mutating Through A Hidden Seam**: stop and report evidence if any
   call still depends on mutation or side effects.

## Draft Follow-Up Card

Append a draft implementation card to this done card. The draft must include:

- exact production call sites to remove;
- exact test/support call sites to keep, rename, or repoint;
- whether the helper declaration/definition should remain;
- focused tests to run;
- self-blockers.

Do not create the follow-up card in `ready/`; reviewer will promote it after
reviewing this audit.

## Completion Brief Requirements

Report:

- call-site counts by category: ignored production, consumed production,
  consumed tests/support;
- exact file/function buckets for ignored production calls;
- implementation decision bucket;
- draft follow-up card title and scope;
- commands run;
- confirmation that no source/test/CMake files were edited.

## Completion Brief

- Card moved to done: yes.
- Files inspected:
  - `src/app/iggy3d/menu/FrontendRouter.hpp`
  - `src/app/iggy3d/menu/FrontendRouter.cpp`
  - `src/app/iggy3d/automation/AutomationControl.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `src/app/iggy3d/menu/Transitions.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `tests/unit/ProductActiveSurfaceTestSupport.hpp`
  - `tests/unit/product_frontend_router_tests.cpp`
  - `tests/unit/product_interaction_mode_state_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `tests/unit/product_menu_transitions_tests.cpp`
  - `tests/unit/product_starter_menu_action_tests.cpp`
  - `cmake/iggy3d_tests.cmake` for focused target names only.
- Current implementation:
  - `src/app/iggy3d/menu/FrontendRouter.cpp:384-390` defines `syncProductWindowInputOwnerFromActiveSurface(...)`.
  - It calls `resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))`, stores the result in a local `surface`, and returns it.
  - It does not mutate `ProductAppWindowState`; `window` is only passed to `productActiveSurfaceContextForWindow(...)`, whose parameter is `const ProductAppWindowState&` at `FrontendRouter.cpp:251-253`.
  - There is no hidden write in the implementation; the helper is now a stale compatibility wrapper around active-surface resolution.
- Call-site counts:
  - Ignored production calls: 18.
  - Consumed production calls: 0.
  - Direct consumed tests/support calls: 3.
  - Downstream `liveSurface(...)` consumers in targeted tests: 36 call expressions, all through `tests/unit/ProductActiveSurfaceTestSupport.hpp`.
  - Declaration/definition occurrences are not counted in the above call-site counts.
- Ignored production call buckets:
  - `src/app/iggy3d/automation/AutomationControl.cpp`
    - `applyProductAutomationControl(...)`: line 51.
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
    - `applyGameplayActionState(...)`: line 68.
    - `applyGameplayJumpActionState(...)`: line 91.
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
    - `recordProductRoomEditingLeave(...)`: line 149.
    - room-editing automation input dispatch in the command application loop: lines 732-733.
  - `src/app/iggy3d/window/InputFrame.cpp`
    - `applyProductWindowInputActionsImpl(...)`: line 424.
    - `cancelProductRoomEditorPendingPreviewFromBack(...)`: line 736.
    - `applyProductWindowEditorMousePickPreview(...)`: line 981.
  - `src/app/iggy3d/window/Loop.cpp`
    - `runProductWindowLoop(...)`: line 138, explicitly `(void)` casts the return.
  - `src/app/iggy3d/menu/Transitions.cpp`
    - `applyReturnProductToTitleTransition(...)`: line 154.
    - `initializeProductStarterTransition(...)`: line 169.
    - `enterProductGameplayTransition(...)`: line 178.
    - `openProductPauseTransition(...)`: line 187.
    - `openProductPauseSettingsTransition(...)`: line 201.
    - `openProductPauseDevToolsTransition(...)`: line 212.
    - `closeProductOverlayToGameplayTransition(...)`: line 220.
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
    - `openStarterDevTools(...)`: line 169.
    - `applyProductGameplayMapMakerToggleAction(...)`: line 821.
- Consumed production calls:
  - None found. The required assignment/return grep reports no production assignment or return consumption. The only production explicit cast is `Loop.cpp:138`, which confirms ignored return.
- Direct test/support calls that consume the return:
  - `tests/unit/ProductActiveSurfaceTestSupport.hpp:8-10`
    - `liveSurface(...)` returns `syncProductWindowInputOwnerFromActiveSurface(...)`.
    - This wrapper is used by active-surface assertions in `product_menu_transitions_tests.cpp`, `product_starter_menu_action_tests.cpp`, and `product_window_input_frame_tests.cpp`.
  - `tests/unit/product_frontend_router_tests.cpp:745-814`
    - `inputOwnerCacheSyncUsesResolvedActiveSurface()` calls the sync helper directly and asserts returned `inputOwner` / `gameplayInputSuppressed`.
    - The test name is now stale because there is no cache sync.
  - `tests/unit/product_interaction_mode_state_tests.cpp:105-121`
    - after `recordProductRoomEditingLeave(...)`, it calls the sync helper directly, stores `surfaceAfterLeave`, and asserts gameplay owner/unsuppressed state.
- Test/support recommendation:
  - `ProductActiveSurfaceTestSupport.hpp::liveSurface(...)` should switch to direct `resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))` in the follow-up. Keeping this helper is useful because 36 downstream `liveSurface(...)` calls are readability-oriented active-surface assertions, but wrapping the stale sync helper keeps the misleading name alive.
  - `product_frontend_router_tests.cpp::inputOwnerCacheSyncUsesResolvedActiveSurface()` should be repointed to direct resolve and renamed to describe resolved/window-context active surface behavior.
  - `product_interaction_mode_state_tests.cpp` should compute `surfaceAfterLeave` with direct resolve.
- Active-surface routing coverage if callers are removed:
  - `product_frontend_router_tests` covers the active-surface route matrix, window-context legacy gameplay gate, room-editor active surface, and the stale sync-return parity test.
  - `product_menu_transitions_tests` covers starter/gameplay/pause/settings/devtools/return-to-title transition surfaces through `liveSurface(...)`.
  - `product_starter_menu_action_tests` covers starter/pause/gameplay ownership transitions through `liveSurface(...)`.
  - `product_window_input_frame_tests` covers editor/gameplay/settings/pause/devtools ownership and suppression after input-frame paths through `liveSurface(...)`.
  - `product_interaction_mode_state_tests` covers room-editing leave returning to gameplay owner/unsuppressed state.
- Decision bucket:
  - **Rename To Resolve Helper And Repoint Tests**.
  - Practical shape: do not create a new renamed helper. Remove the 18 production no-op calls, repoint tests/support to the existing `resolveProductActiveSurface(productActiveSurfaceContextForWindow(...))` seam, then delete `syncProductWindowInputOwnerFromActiveSurface(...)` declaration/definition once no callers remain.
  - Rationale: the helper no longer syncs or writes anything, no production code consumes its return, and retaining it would keep a stale cache-era name in the active-surface API.
- Draft follow-up card:
  - Title: `E200: Remove Stale Active Surface Sync Wrapper`
  - Objective: Remove no-op production calls to `syncProductWindowInputOwnerFromActiveSurface(...)`, repoint tests/support to the real resolver, and delete the stale helper declaration/definition without changing active-surface routing behavior.
  - Source files to edit:
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
    - this task card only if using the builder workflow.
  - Exact production calls to remove:
    - `AutomationControl.cpp:51`
    - `AutomationGameplay.cpp:68`
    - `AutomationGameplay.cpp:91`
    - `AutomationRoomEditing.cpp:149`
    - `AutomationRoomEditing.cpp:732-733`
    - `InputFrame.cpp:424`
    - `InputFrame.cpp:736`
    - `InputFrame.cpp:981`
    - `Loop.cpp:138`
    - `Transitions.cpp:154`
    - `Transitions.cpp:169`
    - `Transitions.cpp:178`
    - `Transitions.cpp:187`
    - `Transitions.cpp:201`
    - `Transitions.cpp:212`
    - `Transitions.cpp:220`
    - `ActionHandlers.cpp:169`
    - `ActionHandlers.cpp:821`
  - Test/support changes:
    - Change `ProductActiveSurfaceTestSupport.hpp::liveSurface(...)` to:
      - `return resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window));`
    - Repoint `product_frontend_router_tests.cpp:803-804` to direct `resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))`.
    - Rename `inputOwnerCacheSyncUsesResolvedActiveSurface()` to a non-cache/non-sync name such as `windowContextResolveMatchesExpectedActiveSurface()`, and update the `main()` call.
    - Repoint `product_interaction_mode_state_tests.cpp:107-108` to direct `resolveProductActiveSurface(productActiveSurfaceContextForWindow(frontend, window))`.
    - After repointing all callers, delete `syncProductWindowInputOwnerFromActiveSurface(...)` from `FrontendRouter.hpp/.cpp`.
  - Focused verification:
    - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_frontend_router_tests product_menu_transitions_tests product_starter_menu_action_tests product_window_input_frame_tests product_interaction_mode_state_tests product_automation_dispatch_tests product_room_editor_action_controller_tests -j10`
    - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_frontend_router_tests|product_menu_transitions_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_automation_dispatch_tests|product_room_editor_action_controller_tests)$' --output-on-failure`
    - `rg -n "syncProductWindowInputOwnerFromActiveSurface\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'` should return no hits.
    - `git -C /Users/kogaryu/iggy3d diff --check`
    - Focused trailing-whitespace scan over touched files.
  - Self-blockers:
    - If any removed production call has observable receipt/test behavior, stop and report the exact dependency instead of reintroducing a hidden cache.
    - If any production code still needs the returned frame, stop and keep or rename a real resolver wrapper instead of deleting it.
    - If direct resolver use in tests reveals a difference from the sync helper, stop; that would contradict the current audit evidence.
  - Non-scope:
    - Do not change `resolveProductActiveSurface(...)` routing policy.
    - Do not reintroduce `inputOwner` or `gameplayInputSuppressed` cache fields on `ProductAppWindowState`.
    - Do not change receipt keys/golden, active-surface semantics, menu routing, input routing, automation command behavior, renderer/window launch, or CMake.
- Commands run:
  - `rg -n "syncProductWindowInputOwnerFromActiveSurface\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'`
  - `rg -n "const .*syncProductWindowInputOwnerFromActiveSurface|=\\s*syncProductWindowInputOwnerFromActiveSurface|return syncProductWindowInputOwnerFromActiveSurface|\\(void\\)syncProductWindowInputOwnerFromActiveSurface" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'`
  - `rg -n "inputOwner|gameplayInputSuppressed|ProductActiveSurfaceFrame|resolveProductActiveSurface" /Users/kogaryu/iggy3d/src/app/iggy3d/menu/FrontendRouter.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/menu/FrontendRouter.hpp /Users/kogaryu/iggy3d/tests/unit/product_frontend_router_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_menu_transitions_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_starter_menu_action_tests.cpp /Users/kogaryu/iggy3d/tests/unit/ProductActiveSurfaceTestSupport.hpp`
  - focused `nl -ba ... | sed -n ...` reads for the implementation and each call-site bucket listed above.
  - `rg` scans for impacted focused test targets and downstream `liveSurface(...)` consumers.
  - `git -C /Users/kogaryu/iggy3d diff --check`
- Tests run:
  - None. This is a read-only audit card; no build or CTest was required.
- Confirmation:
  - No source, test source, CMake, receipt golden, or production docs were edited.
  - Only this task card was moved/appended.
