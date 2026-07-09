# File Spec

Files: `src/app/iggy3d/window/FramePresenter.hpp`, `src/app/iggy3d/window/FramePresenter.cpp`

Verified at: `3dab1aa4`

## Owns

- Product window frame presentation seam for Vulkan menu/gameplay frames.
- Conversion of product UI draw lists and gameplay projection frames into `FrameInput`, UI rects, text glyph quads, and creative wireframe debug render lines.
- Overlay composition order for gameplay HUD, creative UI, and pause menu.
- First-submit measurement and render-submit result recording.

## Does Not Own

- Runtime projection construction.
- Frontend routing state changes.
- Vulkan resource creation internals.
- HUD model construction, save catalog logic, or creative editor command execution.

## Reads

- `ProductWindowFramePresenterRequest`, `ProductGameplayProjectionFrame`, frontend/menu state, save bridge result, SDL drawable extent, renderer state, and optional creative UI/debug line lists.

## Writes / Mutates

- Renderer submit side effects through `vulkanRenderer.submitFrame(...)`.
- Window present-path submit status, startup timing, Vulkan menu UI proof, and submit receipts.
- Local `ProductVulkanMenuFrame` / `ProductVulkanGameplayFrame` payloads.

## Calls Out To / Wires Out To

- `buildProductVulkanGameplayFrame(...)`, `refreshProductVulkanGameplayFrameInput(...)`, `buildProductVulkanStarterMenuFrame(...)`, `appendCreativeUiOverlay(...)`, and `appendPauseMenuOverlay(...)`.
- Pause UI, starter UI draw-list building, debug HUD text layout, and creative wireframe debug conversion.

## Called By / Entry Points

- `Loop.cpp` calls `presentProductWindowFrame(...)`.
- Tests call menu/gameplay frame builders and overlay append helpers directly.
- Grep proof: `rg -n "buildProductVulkanGameplayFrame|presentProductWindowFrame|appendCreativeUiOverlay|appendPauseMenuOverlay|appendNpcBehaviorDebugHudUi|appendPhysicsDebugHudUi|buildProductVulkanStarterMenuFrame" src/app/iggy3d tests/unit cmake CMakeLists.txt`.

## Invariants

- Creative UI overlays gameplay before pause UI, so pause remains topmost.
- Menu frame path is used when gameplay room projection is unavailable and starter is active.
- UI draw-list conversion must honor virtual-to-drawable scaling and theme tone colors.
- Presenter submits frames; it must not mutate runtime simulation.

## Tests / Proof Commands

- `rg -n "product_vulkan_menu_frame_tests|product_vulkan_room_frame_tests|product_vulkan_pause_overlay_tests|product_vulkan_creative_ui_overlay_tests" cmake tests/unit`.
- `rg -n "refreshProductVulkanGameplayFrameInput|refreshProductVulkanMenuFrameInput" src/app/iggy3d/window tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless projection frame payloads change.
- `src/render/*` unless `FrameInput`/render submit contracts change.
- `src/app/iggy3d/menu/*` unless UI draw-list input changes.

## Update When

- Frame payload shape, overlay order, submit recording, menu fallback, or UI scaling changes.

## Do Not Update When

- Projection generation changes but presenter consumption stays stable.
