# File Spec

Files: `src/app/iggy3d/debug/InteractionModeHud.hpp`, `src/app/iggy3d/debug/InteractionModeHud.cpp`

Verified at: `24df6b99`

## Owns

- Interaction mode HUD request/result packets.
- Mapping product interaction mode to HUD label and feedback tone.
- Visibility, status, reason, mode, label, tone, and room-editing-ready proof for the HUD.

## Does Not Own

- Interaction mode toggle state or input chord decisions.
- Room editor readiness production.
- SDL/Vulkan drawing of the HUD.
- Receipt field emission.
- Runtime gameplay or creative mode behavior.

## Reads

- `ProductInteractionMode`.
- Gameplay active flag.
- Room editing readiness flag.

## Writes / Mutates

- Returns `InteractionModeHud`.
- Does not mutate window state, input state, frontend state, or runtime state.

## Calls Out To / Wires Out To

- Calls `productInteractionModeName(...)` for mode string output.
- `ProjectionRefresh.cpp` builds and copies this HUD into window state.
- `DebugHudView.cpp`, `OpeningMenuView.cpp`, and `FramePresenter.cpp` draw or project the HUD.
- Receipt appenders emit the copied fields.

## Called By / Entry Points

- `buildInteractionModeHud(...)`.
- Focused proof: `rg -n "buildInteractionModeHud|InteractionModeHud|interaction_mode_hud" src/app tests`.

## Invariants

- Unknown modes produce a hidden `unknown` HUD with an explicit unknown-mode status.
- Known modes keep mode and label distinct but aligned with descriptor rows.
- HUD is visible only when gameplay is active.
- Player mode is neutral; creative mode is warning-toned.
- This builder is presentation proof, not interaction-mode authority.

## Tests / Proof Commands

- `rg -n "product_interaction_mode_hud_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "interaction_mode_hud_ready|interaction_mode_hud_unknown_mode|buildInteractionModeHud" tests/unit/product_interaction_mode_hud_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/InteractionMode.*` unless interaction mode enum or naming changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless HUD copy/build orchestration changes.
- `src/app/iggy3d/view/DebugHudView.*` unless drawing changes.
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.*` unless receipt keys change.

## Update When

- HUD labels, tones, visibility policy, status strings, request/result fields, or interaction-mode mapping changes.

## Do Not Update When

- Only interaction mode input toggling, room editor behavior, or renderer drawing changes without changing HUD builder output.
