# File Spec

Files: `src/app/frontend/FrontendRoute.hpp`, `src/app/frontend/FrontendRoute.cpp`

Verified at: `a6f36636`

## Owns

- Frontend transition request enum.
- Generic frontend route result packet.
- Stable transition request names.
- Constructors for ignored and accepted frontend route results.

## Does Not Own

- Starter, pause, settings, dev-tools, save-browser, or world-setup route decision logic.
- Product menu handler execution, window state mutation, save execution, session launch, or input polling.

## Reads

- Menu owner, frontend screen/child screen, selected action, transition request, and status/reason inputs.
- `menuOwnerBlocksGameplay(...)` for ignored route suppression.

## Writes / Mutates

- No mutation; returns `FrontendRouteResult` packets.

## Calls Out To / Wires Out To

- Called by frontend menu model route functions to produce consistent result packets.
- Product frontend/router and app/window code consume route result fields.

## Called By / Entry Points

- `makeIgnoredFrontendRouteResult(...)` and `makeAcceptedFrontendRouteResult(...)` are used by starter, pause, settings, dev-tools, save-browser, and world-setup routes.
- Grep proof: `rg -n "FrontendTransitionRequest|FrontendRouteResult|frontendTransitionRequestName|makeIgnoredFrontendRouteResult|makeAcceptedFrontendRouteResult|requestedTransition|receiptReason" src/app tests/unit`.

## Invariants

- Ignored route default status and reason are `frontend_route_ignored`.
- Ignored routes derive gameplay suppression from input owner.
- Accepted routes must explicitly set transition, close request, suppression, status, receipt reason, and selected action.
- Route packets carry intent; they do not execute transitions.

## Tests / Proof Commands

- `rg -n "frontend_route_tests|starter_screen_tests|pause_menu_tests|settings_menu_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "makeIgnoredFrontendRouteResult|makeAcceptedFrontendRouteResult|frontendTransitionRequestName" tests/unit/frontend_route_tests.cpp src/app/frontend`.

## Nearby Files Usually Not Touched

- `src/app/frontend/MenuInput.*` unless owner/suppression semantics change.
- `src/app/frontend/StarterScreen.*`, `src/app/frontend/PauseMenu.*`, and `src/app/frontend/SettingsMenu.*` unless menu route behavior changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless product route application changes.

## Update When

- Route packet fields, transition enum values, transition names, or route result construction semantics change.

## Do Not Update When

- Only one menu surface changes its own route table while using the same result contract.
