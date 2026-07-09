# File Spec

Files: `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`

Verified at: `0ba40cb9`

## Owns

- Recording CreativeDocument per-frame receipt packets into `ProductAppWindowState.creativeAuthoring`.
- Copying creative UI projection, input, downstream click, command, baked-room refresh, document revision, viewport pick, and wireframe diagnostics into durable app-window proof fields.
- Normalizing empty receipt strings to explicit receipt-friendly values where required.

## Does Not Own

- Creative UI projection, hit testing, or command execution.
- Creative document mutation.
- Baked active room refresh execution.
- Viewport pick or wireframe generation.
- Receipt field emission.

## Reads

- Creative UI projection/input/command receipts.
- Creative baked active room refresh results.
- Creative viewport pick and wireframe frame receipts.
- Document id and revision values supplied by callers.

## Writes / Mutates

- Mutates `ProductAppWindowState.creativeAuthoring`.
- Updates sticky last-click and last-command diagnostics.
- Marks baked room stale or fresh based on document revision observations.
- Resets command or auto-refresh baked-room diagnostics before new data is recorded.

## Calls Out To / Wires Out To

- `floatReceiptValue(...)`.
- Creative enum string helpers.
- Local copy/reset helpers for creative command and baked-room refresh diagnostics.

## Called By / Entry Points

- Creative UI frame, window input frame, window loop, baked active room refresh, and focused creative receipt tests.
- Focused proof: `rg -n "recordProductCreativeUiProjection|recordProductCreativeUiInputFrame|recordProductCreativeUiCommandFrame|recordProductCreativeViewportPickFrame|recordProductCreativeWireframeFrame" src/app tests/unit`.

## Invariants

- Recording functions mutate only app-window proof fields.
- Command recording preserves sticky last-click/last-command diagnostics when a command touches creative state.
- Document revision recording marks baked room stale only when observed document identity or revision changes.
- Receipt recording must not execute creative commands or refreshes.

## Tests / Proof Commands

- `rg -n "product_creative_ui_projection_receipt_tests|product_creative_ui_command_receipt_tests|product_creative_viewport_pick_frame_tests|product_creative_wireframe_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "recordProductCreativeUiProjection|recordProductCreativeBakedRoomFresh|recordProductCreativeWireframeFrame" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/ui/*` unless UI projection/input receipt packets change.
- `src/app/iggy3d/creative/bridge/*` unless command/pick/wireframe receipt packets change.
- `src/app/iggy3d/receipt/CreativeUiFields.cpp` and `CreativePickWireframeFields.cpp` unless emitted field keys change.

## Update When

- Creative receipt packet copy rules, sticky diagnostics, baked-room stale/fresh rules, or creative authoring proof fields change.

## Do Not Update When

- Only creative UI drawing, document mutation internals, or wireframe math changes without changing recorded proof fields.
