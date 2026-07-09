# File Spec

Files: `src/app/iggy3d/menu/FrontendRouter.hpp`, `src/app/iggy3d/menu/FrontendRouter.cpp`

Verified at: `6c79462b`

## Owns

- Product frontend surface enum and active-surface frame: `ProductFrontendSurface`, `ProductActiveSurfaceFrame`, and mouse capture policy.
- Mapping from `FrontendState` plus gameplay/session/window context to input owner, active surface, parent surface, input surface, suppression flags, and accepted action groups.
- Legacy map-maker versus CreativeDocument surface classification.
- Product active-surface context construction from `ProductAppWindowState`.

## Does Not Own

- Actual menu command execution.
- UI draw-list creation or hit testing.
- Creative document mutation, map-maker behavior, or room editor commands.
- Low-level keyboard/mouse/gamepad action mapping.

## Reads

- `FrontendState` screen/child screen.
- Gameplay-active/session-ready flags.
- `ProductAppWindowState` interaction mode, room-editor readiness, and map-maker status.
- Optional `creative::CreativeAppState` identity for CreativeDocument ownership.

## Writes / Mutates

- No external state.
- Returns route/active-surface decision packets.

## Calls Out To / Wires Out To

- Uses `menuOwnerBlocksGameplay(...)` and frontend state names.
- Produces `ProductInputSurface` and `MenuOwner` facts consumed by menu input routing, window input, receipts, projection refresh, automation, and interaction-mode state.

## Called By / Entry Points

- `InputRouter.cpp`, `InputFrame.cpp`, `Loop.cpp`, `ReceiptBuilder.cpp`, `ProjectionRefresh.cpp`, automation dispatch, and tests call `resolveProductActiveSurface(...)` or surface-kind helpers.
- Grep proof: `rg -n "resolveProductActiveSurface|productMapMakerLiveForSource|productCreativeDocumentEditorActiveForSource|chooseProductFrontendOwner" src tests cmake`.

## Invariants

- Boot and unavailable gameplay suppress gameplay input.
- Starter and pause child screens preserve their parent owner.
- Room editor upgrades gameplay surface to editor only when room editor is ready and no CreativeDocument world is active.
- CreativeDocument and legacy map-maker classification stay separate.
- Relative mouse capture is only for active player gameplay, not menu/editor/CreativeDocument surfaces.

## Tests / Proof Commands

- `rg -n "product_frontend_router_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_creative_world_launch_tests" cmake tests`.
- `rg -n "ProductCreativeSurfaceKind|productActiveMouseCapturePolicyName|productInputOwnerFor" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/InputRouter.*` unless dispatch ownership changes.
- `src/app/iggy3d/window/InputFrame.cpp` unless window input gating changes.
- `src/app/iggy3d/creative/*` unless CreativeDocument active identity rules change.

## Update When

- Active-surface, input-owner, suppression, mouse-capture, or creative/map-maker classification rules change.

## Do Not Update When

- A menu action implementation changes but surface ownership is unchanged.
- Only UI draw-list appearance changes.
