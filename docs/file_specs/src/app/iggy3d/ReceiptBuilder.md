# File Spec

Files: `src/app/iggy3d/ReceiptBuilder.hpp`, `src/app/iggy3d/ReceiptBuilder.cpp`

Verified at: `a48d779a`

## Owns

- Top-level product app receipt composition through `buildProductAppReceipt(...)`.
- Default creative identity fallback for receipt building.
- Public declaration surface for receipt recorders that mutate product app-window proof packets.

## Does Not Own

- Domain receipt field appenders in `src/app/iggy3d/receipt/`.
- Implementations of creative and physics receipt recorders; those live in receipt recording modules.
- Gameplay, creative UI, renderer, save, menu, or runtime behavior.

## Reads

- Product options, world template, frontend state, settings, app window state, save bridge result, creative active identity, and runtime state hash.
- Active-surface context, creative surface identity, gameplay feedback, movement proof, Vulkan gameplay readiness, and debug HUD packets.

## Writes / Mutates

- Builds and returns a `RenderReceipt`.
- Appends final `result` and `reason_code` fields.
- Does not mutate `ProductAppWindowState` inside `buildProductAppReceipt(...)`.

## Calls Out To / Wires Out To

- `resolveProductActiveSurface(...)` and creative surface helpers.
- `buildGameplayFeedback(...)`, `buildProductMovementProofPacket(...)`, and `evaluateProductVulkanGameplayReadiness(...)`.
- Debug HUD builders and all `appendProduct...Fields(...)` receipt appenders.

## Called By / Entry Points

- `AppKernel.cpp` uses `buildProductAppReceipt(...)` for app receipts.
- Unit tests call it directly for menu, creative, movement, debug HUD, Vulkan, and receipt key-order proof.
- Grep proof: `rg -n "buildProductAppReceipt|defaultProductReceiptCreativeIdentity" src/app/iggy3d tests/unit tests/smoke cmake/iggy3d_tests.cmake`.

## Invariants

- Receipt field ordering is part of the public proof surface.
- `buildProductAppReceipt(...)` composes receipt facts and must not become a behavior executor.
- Creative surface detection must distinguish CreativeDocument from legacy map maker.
- Recorder declarations here must match implementations in receipt recording modules.

## Tests / Proof Commands

- `rg -n "product_receipt_key_order_tests|product_creative_ui_projection_receipt_tests|product_window_renderer_lifecycle_tests|product_vulkan_room_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "recordProductCreativeUiProjection|recordProductCreativeUiInputFrame|recordProductCreativeUiCommandFrame|recordProductPhysicsMovementPlannerTickProof" src/app/iggy3d/receipt src/app/iggy3d/window src/app/iggy3d/gameplay src/app/iggy3d/creative tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/receipt/ReceiptFields.hpp` unless appender signatures or ordering change.
- `src/app/iggy3d/receipt/*Fields.cpp` unless emitted receipt fields change.
- `src/app/iggy3d/receipt/*Recording.cpp` unless recorder implementations change.

## Update When

- Top-level receipt composition, field ordering, active-surface receipt rules, default creative identity behavior, or recorder declaration surface changes.

## Do Not Update When

- Only a domain appender emits more fields behind the same appender contract.
