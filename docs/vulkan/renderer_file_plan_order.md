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

If this broader ordering document conflicts with `docs/vulkan/file_plans/PRIORITY.md`, `PRIORITY.md` is authoritative.

Current baseline note: this document preserves the historical packet ladder as provenance, but the current `iggy3d-main` source is already beyond the old boundary-only packets. The implemented baseline includes the backend-neutral renderer API, concrete `NullRenderer`, factory wiring for `RendererBackendKind::Null`, Vulkan-private implementation files, Vulkan smoke tests, first-room proof surfaces, and Runtime Packet 8 tactical combat runtime state. Do not use the historical Packet 1 wording to remove or defer current source files.

Historical implementation order:

1. Packet 1: backend-neutral `RendererApi`, `RenderBackend`, `FrameInput`, diagnostics/config, and injected test-backend boundary tests only.
2. Packet 2: concrete `NullRenderer`, replay/hash invariance, and factory wiring for `RendererBackendKind::Null`.
3. Packet 3: visual demo boot, package/runtime lookup, and SDL platform shell.
4. Later packets: Vulkan bootstrap, swapchain, shaders, first-room drawing, screenshot/frame hash, and richer visual content.

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

## Current Builder Packet Readiness

Next builder packet: `Renderer Current Baseline Reconciliation Packet`.

This packet reconciles docs and tests around the source that exists now. It does not rewind the tree to the historical Packet 1 boundary.

Current baseline includes:

- backend-neutral `RendererApi`, `RenderBackend`, `FrameInput`, diagnostics, and config;
- concrete `NullRenderer`;
- factory wiring where `createRenderer(RendererBackendKind::Null)` constructs `NullRenderer`;
- Vulkan-private implementation files and smokes under the approved Vulkan surfaces;
- first-room proof and renderer diagnostics artifacts from later renderer packets;
- Runtime Packet 8 tactical combat runtime truth.

Still deferred: new visual app behavior, new SDL input behavior, new Vulkan backend features, shader pipeline changes, screenshot/frame-hash changes, package fixture changes, and Packet 8 combat visual mapping for `training_dummy` or defeated state.

## Packet 0: Renderer Planning Reconciliation

Purpose: make sure the Vulkan docs and current repo state still match before file plans begin.

Plan docs/files:

- `docs/vulkan/README.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/file_plans/PRIORITY.md`.

Read first:

- [boundaries.md](boundaries.md)
- [file_surface.md](file_surface.md)
- [integration_roadmap.md](integration_roadmap.md)

Output:

- updated file-plan order if current repo state has drifted;
- no renderer implementation.

Exit criteria:

- current runtime gate named;
- first renderer packet starts at the backend-neutral boundary only;
- no legacy path dependency.

## Packet 1: Backend-Neutral Renderer Boundary

Historical status: implemented. Current source also includes later renderer packets; use this section as provenance for the backend-neutral API contract, not as an instruction to remove `NullRenderer` or Vulkan-private files.

Purpose: introduce renderer API contracts without Vulkan, SDL, GPU, or window dependencies.

File plans:

- `src/render/RendererApi.hpp`
- `src/render/RendererApi.cpp`
- `src/render/RenderBackend.hpp`
- `src/render/FrameInput.hpp`
- possible `src/render/FrameInput.cpp`
- `src/render/RenderDiagnostics.hpp`
- `src/render/RenderDiagnostics.cpp`
- `src/render/RendererConfig.hpp`
- `src/render/RendererConfig.cpp`
- `tests/unit/render_boundary_tests.cpp`
- `tests/unit/render_diagnostics_tests.cpp`
- `tests/unit/render_config_tests.cpp`
- `tests/unit/render_projection_input_tests.cpp`
- `tests/unit/render_camera_frame_tests.cpp`

Read first:

- [boundaries.md](boundaries.md)
- [frame_input_contract.md](frame_input_contract.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [file_surface.md](file_surface.md)

Must prove:

- public renderer headers compile without Vulkan, SDL, display access, shader artifacts, or package lookup;
- `RendererApi` can be exercised with an injected test backend;
- current baseline `RendererBackendKind::Null` factory construction returns a real `NullRenderer`;
- Vulkan requests remain diagnosed or feature-gated unless an app-owned surface/provider path is active;
- renderer diagnostics exist as backend-neutral values;
- no Vulkan/SDL leaks into public renderer API;
- headless acceptance remains green.

Historically blocked in Packet 1, now implemented by later packets:

- concrete `NullRenderer`;
- Vulkan backend;
- SDL window;
- interactive input mapping;
- shader compiler;
- first-room drawing;
- screenshot/frame hash;
- Packet 8 combat visual mapping;
- GPU tests.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'render_boundary'
rg -n '#include[ <"](SDL3/|SDL\\.h|SDL_vulkan|vulkan/)|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/render/RendererApi.hpp src/render/FrameInput.hpp src/render/RenderBackend.hpp
```

Expected scan result: no matches.

## Packet 2: Null Renderer And Invariance

Historical status: implemented. Current source expects this packet's factory wiring and invariance tests to remain green.

Purpose: add the first concrete renderer backend without GPU, window, shader, or Vulkan dependencies.

File plans:

- `src/render/null/NullRenderer.hpp`
- `src/render/null/NullRenderer.cpp`
- `tests/unit/render_null_renderer_tests.cpp`
- `tests/unit/render_replay_invariance_tests.cpp`

Read first:

- [boundaries.md](boundaries.md)
- [frame_input_contract.md](frame_input_contract.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [file_plans/src_render_null_NullRenderer_hpp.md](file_plans/src_render_null_NullRenderer_hpp.md)
- [file_plans/src_render_null_NullRenderer_cpp.md](file_plans/src_render_null_NullRenderer_cpp.md)

Must prove:

- `NullRenderer` implements the backend-neutral `RenderBackend` contract;
- null renderer can consume a valid `FrameInput`;
- invalid frame input is diagnosed without runtime mutation;
- factory wiring maps `RendererBackendKind::Null` to `NullRenderer`;
- null renderer consumption does not change runtime summary, state hash, command results, or replay truth.

Blocked:

- Vulkan drawing;
- SDL shell;
- shader compiler;
- first-room drawing;
- screenshot/frame hash;
- Packet 8 combat visual mapping;
- GPU resource creation.

Exit criteria:

```sh
ctest --test-dir build --output-on-failure -R 'render_null|render_replay|render_boundary'
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
- SDL3 source/system/vendored acquisition is decided or explicitly deferred for the platform/dependency packet;
- headless builds still work with visual/Vulkan options off;
- app/platform shell owns event polling;
- runtime does not include SDL;
- visual demo can boot in scripted/fixed-frame mode before interactive input mapping is accepted;
- surface provider boundary is clear.

Blocked:

- swapchain;
- command buffers;
- first-room rendering.
- screenshot/frame hash;
- Packet 8 combat visual mapping.

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
