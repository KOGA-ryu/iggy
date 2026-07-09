# File Spec

File: `src/app/iggy3d/window/ProductVulkanMenuState.hpp`

Verified at: `a48d779a`

## Owns

- Vulkan menu render/projection proof packet embedded in `FrontendWindowShell`.
- Menu requested/visible/status/reason/surface facts.
- UI draw-list readiness, partial status, primitive counts, row count, and selected action proof.

## Does Not Own

- Menu route decisions, draw-list construction, Vulkan backend creation, gameplay room rendering, creative UI command handling, or receipt field ordering.

## Reads

- No live inputs; this header defines a packet.
- Receipt field appenders and tests read fields through `window.frontendShell.productVulkanMenu`.

## Writes / Mutates

- `RendererLifecycle.cpp` writes submit and menu UI draw-list proof.
- `FramePresenter.cpp` calls `recordProductVulkanMenuUiDrawList(...)` for starter menu presentation.
- Creative UI tests preserve preexisting Vulkan menu proof while exercising creative surfaces.

## Calls Out To / Wires Out To

- No calls; embedded by `FrontendWindowShell`.
- Read by `FeedbackSurfaceAutomationVulkanFields.cpp` for receipt output.

## Called By / Entry Points

- Reached through `ProductAppWindowState.frontendShell.productVulkanMenu`.
- Grep proof: `rg -n "productVulkanMenu\\.|ProductVulkanMenuState|recordProductVulkanMenuUiDrawList" src/app/iggy3d tests/unit`.

## Invariants

- Menu UI proof is separate from gameplay Vulkan readiness.
- `requested` and `visible` describe menu render attempts, not active gameplay state.
- Draw-list counts are presentation facts and must not become gameplay or save truth.
- Creative UI paths must preserve this packet unless they intentionally present the Vulkan menu surface.

## Tests / Proof Commands

- `rg -n "product_window_renderer_lifecycle_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_creative_ui_projection_receipt_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "recordProductVulkanMenuUiDrawList|productVulkanMenu\\.uiReady|productVulkanMenu\\.uiSelectedAction" src/app/iggy3d tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/RendererLifecycle.*` unless writer behavior changes.
- `src/app/iggy3d/window/FramePresenter.*` unless menu presentation routing changes.
- `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp` unless receipt field emission changes.

## Update When

- Vulkan menu proof fields, default statuses, writer ownership, or receipt-facing semantics change.

## Do Not Update When

- Only menu visual layout or backend renderer internals change without changing this packet contract.
