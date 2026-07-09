# File Spec

Files: `src/app/iggy3d/menu/Transitions.hpp`, `src/app/iggy3d/menu/Transitions.cpp`, `src/app/iggy3d/menu/ProductTransitionState.hpp`

Verified at: `0996a5ee`

## Owns

- Product frontend/window transition helpers for starter, gameplay, pause, settings, dev-tools, resume, and return-to-title surfaces.
- `ProductTransitionState`, the gameplay-store transition mirror used by receipts and tests.
- Cleanup of product transient modes when crossing menu/gameplay/title boundaries.

## Does Not Own

- Frontend route definitions or action selection.
- Runtime session creation, save/load/delete execution, or world creation.
- Creative document state or creative authoring kernels.
- Reusable movement, map-maker, or room-editor behavior.

## Reads

- `FrontendState`, `FrontendSettings`, `ProductAppWindowState`, selected `FrontendAction`, settings tab, and dev-tools category.
- `productCreativeWorldActiveForWindow(window)` when pause cleanup must preserve an active CreativeDocument interaction mode.

## Writes / Mutates

- Mutates frontend screen, child screen, selected action, input ownership, status, and return-to-title flag.
- Mutates `window.gameplay.productTransition`.
- Clears menu-owned and gameplay-only transient window state, including map-maker mode, movement tuning/proof mirrors, jump timing, room-editor transient mirrors, and debug overlays.
- Clears gameplay/session-active flags on return to title.

## Calls Out To / Wires Out To

- Calls frontend transition helpers: `completeFrontendBoot`, `enterFrontendGameplay`, `openFrontendPause`, `openFrontendDevOverlay`, and `closeFrontendOverlayToGameplay`.
- Called from app startup, world/session launch, creative world launch/open, save flow, automation dispatch, and action handlers.

## Called By / Entry Points

- `initializeProductStarterTransition`.
- `enterProductGameplayTransition`.
- `openProductPauseTransition`.
- `openProductPauseSettingsTransition`.
- `openProductPauseDevToolsTransition`.
- `closeProductOverlayToGameplayTransition`.
- `returnProductToTitleTransition`.
- Focused proof: `rg -n "initializeProductStarterTransition|enterProductGameplayTransition|openProductPauseTransition|closeProductOverlayToGameplayTransition|returnProductToTitleTransition|ProductTransitionState|productTransition" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Starter initialization returns to `FrontendScreen::Starter` and selects Continue only when compatible save state is provided.
- Gameplay entry delegates to frontend gameplay transition and records a preserved-session gameplay-active transition.
- Pause opening clears pause-owned transient modes but preserves CreativeDocument mode when an active creative world owns that mode.
- Return-to-title clears gameplay/session-active flags, gameplay-only transient state, and records a non-preserved title transition.
- Transition helpers must not become owners of runtime kernels or feature-specific business logic.

## Tests / Proof Commands

- `product_menu_transitions_tests` covers starter readiness, gameplay entry, pause, overlay close, and return-to-title transition mirrors.
- `product_interaction_mode_state_tests` covers interaction-mode behavior across title transitions.
- `rg -n "product_menu_transitions_tests|product_interaction_mode_state_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/ActionHandlers.*` unless action decisions change.
- `src/app/iggy3d/menu/FrontendRouter.*` unless frontend route semantics change.
- `src/app/iggy3d/ProductAppWindowState.hpp` and gameplay store headers unless transition fields move.
- Runtime session/save/world launch files unless their call sites change.

## Update When

- A transition helper changes ownership, a boundary cleanup list changes, `ProductTransitionState` changes, or a new app surface uses this transition seam.

## Do Not Update When

- Only a caller chooses a different action before invoking the same transition helper.
