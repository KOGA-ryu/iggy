# Renderer File Plan Order

This document turns the Vulkan planning docs into implementation file-plan packets.

It does not authorize renderer implementation before the headless runtime gate. It defines what to plan next once the gate is green.

The concrete file-by-file plan set now lives under:

```text
docs/vulkan/file_plans/
```

Use:

- [file_plans/INDEX.md](file_plans/INDEX.md) for the complete file-plan list;
- [file_plans/PRIORITY.md](file_plans/PRIORITY.md) for packet order;
- [file_plans/COMPLETE_RENDERER_SURFACE.md](file_plans/COMPLETE_RENDERER_SURFACE.md) for the full planned surface and naming reconciliations.

## Hard Gate

Do not create Vulkan implementation file plans until:

- headless runtime acceptance is green;
- projection emits backend-neutral scene/debug output;
- `CameraState` and replay/hash behavior are implemented and tested;
- dependency firewall scan is clean;
- old renderer remains historical reference only.

Required gate command shape:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Then:

```sh
rg -n '#include[ <"]vulkan/|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected result: no matches.

## Packet Rules

Each packet should produce file plans, not code, unless the user explicitly starts implementation.

Every packet must include:

- files to create or modify;
- owner for each file;
- forbidden ownership;
- allowed/forbidden includes;
- diagnostics fields if applicable;
- tests/smoke gates;
- fallback rules;
- docs to read first;
- exit criteria.

Do not batch packets across ownership boundaries just because files are adjacent.

## Packet 0: Renderer Planning Reconciliation

Purpose: make sure the Vulkan docs and current repo state still match before file plans begin.

Plan docs/files:

- `docs/vulkan/README.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/file_plans/PRIORITY.md` or a renderer-specific addendum if chosen.

Read first:

- [boundaries.md](boundaries.md)
- [file_surface.md](file_surface.md)
- [integration_roadmap.md](integration_roadmap.md)

Output:

- updated file-plan order if current repo state has drifted;
- no renderer implementation.

Exit criteria:

- current runtime gate named;
- first renderer packet still starts at boundary/null renderer;
- no legacy path dependency.

## Packet 1: Backend-Neutral Renderer Boundary

Purpose: introduce renderer API contracts without Vulkan, SDL, GPU, or window dependencies.

File plans:

- `src/render/RendererApi.hpp`
- `src/render/RendererApi.cpp`
- `src/render/RenderBackend.hpp`
- `src/render/RenderDiagnostics.hpp`
- `src/render/RenderDiagnostics.cpp`
- `src/render/null/NullRenderer.hpp`
- `src/render/null/NullRenderer.cpp`
- `tests/unit/render_boundary_tests.cpp`

Read first:

- [boundaries.md](boundaries.md)
- [frame_input_contract.md](frame_input_contract.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [file_surface.md](file_surface.md)

Must prove:

- null renderer can consume a frame boundary without runtime mutation;
- renderer diagnostics exist as backend-neutral values;
- no Vulkan/SDL leaks into public renderer API;
- headless acceptance remains green.

Blocked:

- Vulkan backend;
- SDL window;
- shader compiler;
- GPU tests.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'render_boundary'
rg -n '#include[ <"](SDL3/|SDL\\.h|SDL_vulkan|vulkan/)|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/render/RendererApi.hpp src/render/FrameInput.hpp src/render/RenderBackend.hpp
```

Expected scan result: no matches.

## Packet 2: FrameInput And Camera Render Contract

Purpose: plan exact frame input validation and camera derivation support.

File plans:

- `src/render/FrameInput.hpp`
- possible `src/render/FrameInput.cpp`
- possible camera derivation helper file if kept outside runtime;
- `tests/unit/render_projection_input_tests.cpp`
- `tests/unit/render_camera_frame_tests.cpp`
- `tests/unit/render_replay_invariance_tests.cpp`

Read first:

- [frame_input_contract.md](frame_input_contract.md)
- [camera_render_contract.md](camera_render_contract.md)
- [first_room_render_contract.md](first_room_render_contract.md)
- local runtime camera/projection file plans.

Must prove:

- `FrameInput` can reference/copy projection and camera-derived data safely;
- invalid camera/frame data is rejected;
- null renderer consumption does not change replay hash;
- camera aspect/near/far/matrix rules are explicit.

Blocked:

- Vulkan drawing;
- SDL shell;
- GPU resource creation.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'render_projection_input|render_camera|render_replay'
```

## Packet 3: Visual App And SDL Platform Shell

Purpose: plan the cross-platform visual shell without Vulkan device work beyond surface handoff shape.

File plans:

- `apps/iggy3d_visual_demo/main.cpp`
- `src/app/platform/SdlWindow.hpp`
- `src/app/platform/SdlWindow.cpp`
- `src/app/platform/SdlVulkanSurface.hpp`
- `src/app/platform/SdlVulkanSurface.cpp`
- `cmake/iggy3d_vulkan_deps.cmake`
- `tests/smoke/vulkan_platform_smoke.cpp`

Read first:

- [platform_shell.md](platform_shell.md)
- [platform_matrix.md](platform_matrix.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- SDL3 remains proposed shell or fallback is documented;
- headless builds still work with visual/Vulkan options off;
- app/platform shell owns event polling;
- runtime does not include SDL;
- surface provider boundary is clear.

Blocked:

- swapchain;
- command buffers;
- first-room rendering.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_platform|render_boundary'
```

Optional lane may skip if display/GPU requirements are absent; strict lane must fail if required.

## Packet 4: Vulkan Instance/Device/Surface Diagnostics

Purpose: plan Vulkan instance, device, queue, validation, and surface ownership with diagnostics receipt.

File plans:

- `src/render/vulkan/VulkanBackend.hpp`
- `src/render/vulkan/VulkanBackend.cpp`
- `src/render/vulkan/InstanceDeviceSurface.hpp`
- `src/render/vulkan/InstanceDeviceSurface.cpp`
- `src/render/vulkan/DebugValidation.hpp`
- `src/render/vulkan/DebugValidation.cpp`
- `src/render/vulkan/VulkanTypes.hpp`
- `src/render/vulkan/VulkanResult.hpp`
- `src/render/vulkan/VulkanResult.cpp`
- `tests/smoke/vulkan_device_smoke.cpp`

Read first:

- [manuals.md](manuals.md)
- [decisions.md](decisions.md)
- [platform_matrix.md](platform_matrix.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- API version/features/extensions are queried and printed;
- validation/debug messenger policy is explicit;
- MoltenVK portability is diagnosed, not architecture authority;
- raw Vulkan handles stay inside Vulkan files.

Blocked:

- swapchain image ownership;
- drawing;
- shaders/resources.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|render_diagnostics'
```

Required receipt includes `device_name`, `api_version`, `driver_version`, queue families, validation state, and enabled extensions.

## Packet 5: Swapchain Lifecycle

Purpose: plan swapchain creation, resize/recreate, surface-loss behavior, and diagnostics.

File plans:

- `src/render/vulkan/Swapchain.hpp`
- `src/render/vulkan/Swapchain.cpp`
- updates to `VulkanBackend.*`
- `tests/smoke/vulkan_swapchain_smoke.cpp`

Read first:

- [platform_shell.md](platform_shell.md)
- [platform_matrix.md](platform_matrix.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

Must prove:

- swapchain format/present mode/extent/image count are diagnosed;
- resize and minimize behavior do not mutate runtime;
- swapchain recreation owns its resources cleanly.

Blocked:

- first-room rendering;
- depth image;
- graphics pipeline.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_swapchain'
```

## Packet 6: Command Buffers And Empty Frame

Purpose: plan command pools/buffers and a validation-clean empty or clear-only frame.

File plans:

- `src/render/vulkan/CommandBuffers.hpp`
- `src/render/vulkan/CommandBuffers.cpp`
- updates to `VulkanBackend.*`
- `tests/smoke/vulkan_empty_frame_smoke.cpp`

Read first:

- [manuals.md](manuals.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- command recording lifecycle is explicit;
- dynamic rendering path or fallback is diagnosed;
- no scene/runtime access from command recording.

Blocked:

- shader pipeline;
- resource uploads;
- first-room draw.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_empty_frame'
```

## Packet 7: Frame Sync And Sync Validation

Purpose: plan frames-in-flight, acquire/present sync, fences/semaphores, and sync validation gate.

File plans:

- `src/render/vulkan/FrameSync.hpp`
- `src/render/vulkan/FrameSync.cpp`
- updates to `DebugValidation.*`
- `tests/smoke/vulkan_sync_smoke.cpp`

Read first:

- [decisions.md](decisions.md)
- [platform_matrix.md](platform_matrix.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- first sync policy is `binary_wsi`;
- frames in flight default is 2 unless evidence changes it;
- sync validation is enabled or unavailable reason is printed;
- strict sync lane fails correctly.

Blocked:

- resource upload queues beyond first simple path;
- timeline semaphore adoption for internal work.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_sync|vulkan_empty_frame'
```

## Packet 8: Shader Build And First Pipeline

Purpose: plan GLSL/glslang shader compilation and first-room pipeline creation.

File plans:

- `cmake/iggy3d_shaders.cmake`
- `shaders/vulkan/src/first_room.vert.glsl`
- `shaders/vulkan/src/first_room.frag.glsl`
- `src/render/vulkan/PipelinesShaders.hpp`
- `src/render/vulkan/PipelinesShaders.cpp`
- `tests/unit/render_shader_policy_tests.cpp`
- `tests/smoke/vulkan_pipeline_smoke.cpp`

Read first:

- [shader_pipeline.md](shader_pipeline.md)
- [decisions.md](decisions.md)
- [fallbacks.md](fallbacks.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

Must prove:

- GLSL/glslang first path is planned;
- generated SPIR-V path is deterministic;
- first pipeline uses push constants only;
- descriptor/material work remains blocked;
- shader diagnostics are named.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'shader_policy|vulkan_pipeline'
```

## Packet 9: First-Room Bootstrap Resources

Purpose: plan vertex/index/depth resources and upload path sufficient for first visible room.

File plans:

- `src/render/vulkan/BuffersImagesMemory.hpp`
- `src/render/vulkan/BuffersImagesMemory.cpp`
- updates to `PipelinesShaders.*`
- `tests/unit/render_memory_policy_tests.cpp`
- `tests/smoke/vulkan_memory_smoke.cpp`

Read first:

- [resource_model.md](resource_model.md)
- [shader_pipeline.md](shader_pipeline.md)
- [first_room_render_contract.md](first_room_render_contract.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- depth format fallback is planned;
- first-room bootstrap can use `manual_bootstrap` or VMA if already adopted;
- no per-frame allocation churn;
- texture/material work remains blocked.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'render_memory|vulkan_memory'
```

## Packet 10: First Visible Room

Purpose: plan the first visual proof from fixture/projection to Vulkan draw.

File plans:

- `apps/iggy3d_visual_demo/main.cpp` updates;
- `src/render/vulkan/VulkanBackend.cpp` updates;
- first-room proxy geometry owner, if separate;
- `tests/smoke/vulkan_first_room_smoke.cpp`;
- optional diagnostics artifact helper.

Read first:

- [first_room_render_contract.md](first_room_render_contract.md)
- [camera_render_contract.md](camera_render_contract.md)
- [frame_input_contract.md](frame_input_contract.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

Must prove:

- fixture flows through runtime/projection/FrameInput;
- player marker and room/floor/wall proxy or diagnosed fallback are visible;
- `draw_count > 0`;
- `first_room_visible=true`;
- replay hash unchanged.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_first_room|render_replay'
```

## Packet 11: Diagnostics And Artifact Hardening

Purpose: plan stable receipts, logs, strict/skip behavior, and artifact folders across Vulkan smoke tests.

File plans:

- `src/render/RenderDiagnostics.hpp` updates;
- `src/render/RenderDiagnostics.cpp` updates;
- diagnostics artifact helper if warranted;
- `tests/smoke/vulkan_diagnostics_smoke.cpp`;
- CTest option/label updates.

Read first:

- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [platform_matrix.md](platform_matrix.md)
- [packaging.md](packaging.md)

Must prove:

- key-value receipt schema is stable;
- skip exit code 77 is configured;
- strict lane fails instead of skipping;
- artifact paths are deterministic.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_diagnostics|render_diagnostics'
```

## Packet 12: VMA Growth Gate

Purpose: plan VMA adoption before textures/materials/assets.

File plans:

- `cmake/iggy3d_vulkan_deps.cmake` updates;
- `third_party/vma/` or `external/vma/` dependency plan;
- `src/render/vulkan/BuffersImagesMemory.*` updates;
- `tests/smoke/vulkan_memory_smoke.cpp` updates.

Read first:

- [resource_model.md](resource_model.md)
- [decisions.md](decisions.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- VMA version/acquisition is pinned;
- allocator lifetime is explicit;
- allocation naming and budget diagnostics exist;
- texture/material growth can start after this gate.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_memory|render_memory'
```

## Packet 13: Packaging And Platform Proof

Purpose: plan installed/build-tree visual layout and required platform lanes.

File plans:

- `cmake/iggy3d_install.cmake`
- `cmake/iggy3d_vulkan_deps.cmake` updates;
- package smoke tests;
- platform-specific doc addenda if needed.

Read first:

- [packaging.md](packaging.md)
- [platform_matrix.md](platform_matrix.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

Must prove:

- headless package is graphics-free;
- shader root is discoverable in visual package;
- macOS/MoltenVK lane is explicit;
- Linux native Vulkan lane is explicit;
- Windows native Vulkan lane is explicit.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -L 'packaging'
ctest --test-dir build --output-on-failure -L 'vulkan'
```

## Blocked Until Later

Do not plan these before first visible room:

- textures;
- real material system;
- asset/model streaming;
- lighting;
- shadows;
- animation;
- picking;
- RenderDoc automation;
- multiplayer-specific visuals;
- editor UI.

Do not plan these before VMA/resource diagnostics:

- texture upload;
- material descriptor sets;
- GPU residency cache;
- defragmentation;
- memory budget tuning.

## Acceptance Criteria

This file-plan order is ready when:

- every packet has files, docs, proof, and blocked scope;
- the first packet is backend-neutral;
- the Vulkan backend starts only after the runtime gate;
- first visible room comes before asset/material growth;
- strict platform proof remains macOS/MoltenVK plus Linux and Windows native Vulkan;
- no packet imports old renderer code.
