# File Spec

Files: `src/app/iggy3d/window/RendererLifecycle.hpp`, `src/app/iggy3d/window/RendererLifecycle.cpp`

Verified at: `bdb9e108`

## Owns

- Product window renderer lifecycle selection and status recording.
- Vulkan renderer availability/readiness proof, submit proof, menu UI draw-list proof, renderer creation, shutdown, and final frontend window status.
- SDL renderer creation/destruction fallback when Vulkan is not requested.

## Does Not Own

- Vulkan backend implementation, SDL window creation, frame presentation layout, gameplay projection, menu draw-list construction, receipt field emission, or app option parsing.

## Reads

- Product renderer request, SDL window/create info, `ProductAppWindowState`, renderer diagnostics receipts, render submit receipts, package runtime lookup, and compile-time SDL/Vulkan backend availability.
- Present-path, viewport, and frontend shell fields used to decide gameplay readiness.

## Writes / Mutates

- Mutates `window.presentPath` Vulkan lifecycle/status fields.
- Mutates `window.frontendShell.productVulkanMenu` menu UI proof fields.
- Mutates `window.frontendShell.status` for renderer unavailable, create failure, frame presented, or opening-menu ready states.
- Owns the transient `ProductWindowRendererState` lifetime packet returned to the loop.

## Calls Out To / Wires Out To

- `resolvePackageRuntimeLookup(...)` to build Vulkan renderer config.
- `RendererApi`, `VulkanBackend`, and SDL Vulkan surface provider when Vulkan backend is compiled.
- `SDL_CreateRenderer(...)` and `SDL_DestroyRenderer(...)` when using SDL renderer path.
- Render diagnostics helpers and receipt field helpers.

## Called By / Entry Points

- `createProductWindowRenderer(...)`, `shutdownProductWindowRenderer(...)`, and `finalizeProductWindowRendererStatus(...)` are called by `Loop.*`.
- `recordProductVulkanSubmit(...)` and `recordProductVulkanMenuUiDrawList(...)` are called by `FramePresenter.*`.
- `evaluateProductVulkanGameplayReadiness(...)` is called by receipt building and tests.
- Grep proof: `rg -n "createProductWindowRenderer|shutdownProductWindowRenderer|finalizeProductWindowRendererStatus|recordProductVulkanSubmit|evaluateProductVulkanGameplayReadiness" src/app/iggy3d tests/unit`.

## Invariants

- Vulkan readiness requires requested renderer, built backend, created/ready renderer, CPU room mesh readiness, submitted frame, and backend-presented room mesh.
- Vulkan unavailable paths must set renderer created/ready false and preserve a concrete reason code.
- Successful Vulkan submit increments submitted count and marks surface/swapchain/frame submitted.
- Menu UI submit proof is separate from gameplay room mesh readiness.
- Shutdown must idle/shutdown Vulkan or destroy SDL renderer, then mark transient renderer state not ready.

## Tests / Proof Commands

- `rg -n "product_window_renderer_lifecycle_tests|product_vulkan_room_frame_tests|product_render_bridge_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "evaluateProductVulkanGameplayReadiness|recordProductVulkanSubmit|recordProductVulkanMenuUiDrawList|productWindowVulkanBackendBuilt" tests/unit/product_window_renderer_lifecycle_tests.cpp tests/unit/product_vulkan_room_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/render/**` unless backend API contracts change.
- `src/app/iggy3d/window/Loop.*` unless renderer lifecycle ordering changes.
- `src/app/iggy3d/window/FramePresenter.*` unless submit/present routing changes.

## Update When

- Renderer selection, Vulkan readiness rules, renderer create/shutdown behavior, submit proof fields, menu UI proof fields, or final window status rules change.

## Do Not Update When

- Only shader/resource implementation, menu layout, gameplay projection facts, or receipt field ordering changes without changing renderer lifecycle contracts.
