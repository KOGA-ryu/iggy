# File Spec

Files: `src/app/iggy3d/creative/ui/UiFrame.hpp`, `src/app/iggy3d/creative/ui/UiFrame.cpp`

Verified at: `d40a476b`

## Owns

- Creative UI frame request/result packets.
- Active gating for CreativeDocument UI projection.
- Projection build call and recording of creative UI projection receipt into product window state.
- Frame-level receipt summarizing projection readiness and counts.

## Does Not Own

- Window coordinate conversion.
- UI hit testing, command execution, or downstream click suppression.
- Creative document mutation or facade install.
- Renderer overlay composition.

## Reads

- Product window state, optional creative app state, virtual dimensions, and UI theme.
- CreativeDocument active predicate from frontend router.

## Writes / Mutates

- Records creative UI projection receipt into `ProductAppWindowState`.
- Returns `ProductCreativeUiFrame`.

## Calls Out To / Wires Out To

- `productCreativeDocumentEditorActiveForSource(...)`.
- `buildProductCreativeUiProjection(...)`.
- `recordProductCreativeUiProjection(...)`.

## Called By / Entry Points

- `UiWindowFrame.cpp` builds window-sized creative UI frames through this file.
- Tests call `buildProductCreativeUiFrame(...)` directly.
- Grep proof: `rg -n "buildProductCreativeUiFrame|productCreativeUiActiveForWindow|recordProductCreativeUiProjection" src tests cmake`.

## Invariants

- Missing window returns a missing-window receipt without projection recording.
- Inactive CreativeDocument state records an inactive projection receipt.
- Missing creative app records a facade-missing receipt.
- This file builds/records projection facts only; it does not route clicks or commands.

## Tests / Proof Commands

- `rg -n "product_creative_ui_frame_tests|product_creative_ui_projection_tests|product_creative_ui_projection_receipt_tests" cmake tests`.
- `rg -n "product_creative_ui_frame_inactive|product_creative_ui_frame_facade_missing" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/ui/UiProjection.*` unless projection payload changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless CreativeDocument active gating changes.
- `src/app/iggy3d/creative/bridge/UiWindowFrame.*` unless window sizing changes.

## Update When

- Creative UI frame request/receipt, active gate, projection recording, or projection build wiring changes.

## Do Not Update When

- Only click routing or command dispatch changes downstream.
