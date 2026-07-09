# File Spec

Files: `src/app/iggy3d/creative/ui/UiProjection.hpp`, `src/app/iggy3d/creative/ui/UiProjection.cpp`

Verified at: `7c9e712f`

## Owns

- Product-facing creative UI projection wrapper from explicit UI model or creative app facade to draw list plus receipt.
- `ProductCreativeUiProjectionRequest`, `ProductCreativeUiProjectionReceipt`, and `ProductCreativeUiProjection`.
- Mirroring draw-list/model facts into stable projection receipt fields.

## Does Not Own

- Creative UI model construction internals.
- Draw-list row layout internals.
- Window frame timing, receipt recording, input routing, or command execution.

## Reads

- Optional `creative::CreativeUiModel`.
- Optional `creative::CreativeAppState` and facade for model construction.
- Virtual size and theme from projection request.
- Draw-list counters and status from `buildProductCreativeUiDrawList`.

## Writes / Mutates

- Builds `ProductCreativeUiProjection`.
- Marks whether the request used an explicit model or facade-built model.
- Mirrors panel count, model row count, primitive count, text/rect counts, row counts, disabled rows, and hit-region counts.
- Does not mutate creative app state or the supplied model.

## Calls Out To / Wires Out To

- Calls `creativeUndoAvailable`, `creativeUndoDepth`, and `Facade::buildUiModel` when no explicit model is supplied.
- Calls `buildProductCreativeUiDrawList`.
- Feeds `UiFrame` and `recordProductCreativeUiProjection` through projection receipts.

## Called By / Entry Points

- `buildProductCreativeUiProjection`.
- `buildProductCreativeUiFrame`.
- Focused proof: `rg -n "buildProductCreativeUiProjection|ProductCreativeUiProjection|usedFacade|usedModel|creative_ui_projection" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Explicit model input takes precedence over facade input.
- Null model and null creative app still return a requested receipt with draw-list rejection facts.
- Receipt status and reason code use stable creative UI projection strings.
- Projection mirrors draw-list counters exactly; it does not recompute visual facts independently.

## Tests / Proof Commands

- `product_creative_ui_projection_tests` covers null, explicit-model, facade, undo, precedence, and populated receipt behavior.
- `product_creative_ui_projection_receipt_tests` covers receipt recording fields.
- `product_creative_ui_frame_tests` and `product_creative_ui_window_frame_tests` cover frame/window consumers.
- `rg -n "product_creative_ui_projection_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/ui/Ui.*` unless facade model-building request ownership changes.
- `src/app/iggy3d/creative/ui/UiDrawList.*` unless draw-list receipt facts change.
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp` unless receipt recording changes.

## Update When

- Projection request/receipt fields, model-vs-facade precedence, draw-list mirroring, or frame wiring changes.

## Do Not Update When

- Only individual creative UI rows or visual text change without changing projection ownership.
