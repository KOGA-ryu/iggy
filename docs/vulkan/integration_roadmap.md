# Vulkan Integration Roadmap

This roadmap starts after the runtime-first project spine, not before it. `iggy3d` must ship a headless runtime demo before Vulkan becomes implementation work.

## Baseline Decision

Recommended baseline: plan for modern Vulkan using dynamic rendering, strong validation, synchronization validation, and a shader build step that emits SPIR-V. This matches the current Khronos tutorial direction and avoids preserving old render-pass structure in a standalone repo.

Decision gate: Phase 3 and Phase 6 must prove the chosen baseline on macOS/MoltenVK as the first local validation lane, and on Linux and Windows native Vulkan before the backend is shippable. If feature, SDK, validation, or packaging evidence blocks the modern path, the fallback is a documented compatibility path using Vulkan 1.2 style render passes and binary semaphore patterns. The old renderer may inform that fallback, but it is not build input.

Evidence needed before finalizing:

- selected device API version and enabled feature list;
- required instance/device extensions on macOS/MoltenVK, Linux, and Windows;
- validation and synchronization validation status;
- empty-frame presentation and resize behavior;
- shader compiler availability and reproducible SPIR-V output.

## Phase 0: Headless Runtime Gate Before Renderer Work

Purpose: prove the shippable runtime loop before any renderer code exists.

Likely local paths:

- `docs/architecture.md`
- `docs/acceptance_demo.md`
- `docs/roadmap.md`
- `docs/ownership.md`
- `apps/iggy3d_headless_demo/`
- `apps/iggy3d_validate_package/`
- `apps/iggy3d_replay_tool/`
- `tests/acceptance/`

Data ownership: runtime owns gameplay truth, camera mode truth, command legality, save/load, replay, deterministic hash, and summary output.

Dependency rules: no Vulkan dependency. No window, GPU, renderer, shader compiler, or platform surface requirement.

Compute/runtime cost expectations: CPU-only deterministic simulation. Renderer cost is zero.

Validation command or test gate:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/apps/iggy3d_validate_package/iggy3d_validate_package fixtures/demos/first_room
./build/apps/iggy3d_headless_demo/iggy3d_headless_demo fixtures/demos/first_room
./build/apps/iggy3d_replay_tool/iggy3d_replay_tool fixtures/demos/first_room
```

Exit criteria: acceptance demo proves load, session creation, target discovery, movement/retry/interaction, inventory/objective, camera modes, pause/step/resume, reset, save/load, replay, and deterministic hash/summary.

Read first: local runtime docs, not Vulkan manuals.

## Phase 1: Renderer Boundary And Dependency Firewall

Purpose: create renderer-facing interfaces without creating a Vulkan backend dependency.

Likely local paths:

- `src/render/RendererApi.hpp`
- `src/render/RendererApi.cpp`
- `src/render/FrameInput.hpp`
- `src/render/RenderBackend.hpp`
- `src/render/RenderDiagnostics.hpp`
- `src/render/null/NullRenderer.hpp`
- `src/render/null/NullRenderer.cpp`
- `tests/unit/render_boundary_tests.cpp`

Data ownership: `FrameInput` owns a per-frame copy or view of backend-neutral projection/camera data. `RendererApi` owns renderer lifetime orchestration. Runtime still owns all gameplay truth.

Dependency rules: `src/render/**` may depend on `src/projection/**` value types only through stable backend-neutral inputs. `src/runtime/**`, `src/content/**`, `src/projection/**`, and `src/runtime/save/**` must not include Vulkan headers, `Vk*` types, or `VK_*` constants.

Compute/runtime cost expectations: negligible CPU cost; no GPU cost. Null renderer should be cheap enough for tests.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_boundary|dependency_firewall'
rg -n '#include[ <"]vulkan/|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Exit criteria: a null renderer can consume a frame input without mutating runtime state, and the Vulkan dependency scan is clean.

Read first: [boundaries.md](boundaries.md), Vulkan Guide validation overview concepts, Building a Simple Engine architecture introduction.

## Phase 2: Window/Platform Shell And Surface Ownership

Purpose: define where window creation, event polling, and Vulkan surface ownership live.

Likely local paths:

- `apps/iggy3d_visual_demo/main.cpp`
- `src/render/RendererApi.hpp`
- `src/render/vulkan/VulkanBackend.hpp`
- `src/render/vulkan/InstanceDeviceSurface.hpp`
- `tests/smoke/vulkan_platform_smoke.cpp`

Data ownership: the app owns OS/window event collection and passes resize/input intentions into runtime or renderer APIs. Vulkan surface handles are renderer-backend resources, not runtime data.

Dependency rules: platform-window headers are allowed in the visual app and platform adapter files only. Runtime does not depend on windowing. Vulkan headers remain under `src/render/vulkan/**` and Vulkan-specific smoke/app files.

Compute/runtime cost expectations: small event-loop CPU overhead; no sustained GPU work yet.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_platform|render_boundary'
```

Exit criteria: the app can create a window/platform shell, hand a surface creation request to the backend, shut down cleanly, and keep headless tests unchanged.

Read first: Khronos Vulkan Tutorial setup/surface chapters, LunarG macOS SDK guide, MoltenVK Runtime User Guide.

## Phase 3: Instance/Device/Surface Diagnostics

Purpose: create Vulkan instance/device/surface selection with a diagnostics receipt before drawing.

Likely local paths:

- `src/render/vulkan/InstanceDeviceSurface.hpp`
- `src/render/vulkan/InstanceDeviceSurface.cpp`
- `src/render/vulkan/DebugValidation.hpp`
- `src/render/vulkan/DebugValidation.cpp`
- `src/render/RenderDiagnostics.hpp`
- `src/render/RenderDiagnostics.cpp`
- `tests/smoke/vulkan_device_smoke.cpp`

Data ownership: Vulkan backend owns instance, physical device selection, logical device, queues, surface, enabled features, enabled extensions, and diagnostics strings.

Dependency rules: diagnostics exported to non-Vulkan code must be plain data: strings, enums, booleans, numeric versions. No `Vk*` leaks through public renderer API.

Compute/runtime cost expectations: startup-only device enumeration and capability checks. No frame cost yet.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|render_diagnostics'
```

Exit criteria: smoke output reports device name, API version, driver version, validation enabled, sync validation enabled or unavailable reason, selected queue families, required extensions, and portability notes.

Read first: Vulkan Tutorial instance/device/surface chapters, Vulkan Guide extensions/validation, Vulkan Specification/Registry for required feature and extension behavior, LunarG macOS SDK guide, MoltenVK guide.

## Phase 4: Swapchain And Resize/Recreate Lifecycle

Purpose: own presentable images and recreate them correctly on resize, minimize, surface loss, and format changes.

Likely local paths:

- `src/render/vulkan/Swapchain.hpp`
- `src/render/vulkan/Swapchain.cpp`
- `src/render/vulkan/VulkanBackend.cpp`
- `src/render/RenderDiagnostics.cpp`
- `tests/smoke/vulkan_swapchain_smoke.cpp`

Data ownership: swapchain module owns swapchain handle, image views, extent, format, present mode, image count, and recreate state.

Dependency rules: app reports resize events; renderer decides when to recreate. Runtime never sees swapchain dimensions except through derived camera/frame inputs that the app/runtime pipeline already owns.

Compute/runtime cost expectations: low normal frame cost; expensive recreate path only on resize/surface events. Recreate must wait for safe device/image ownership.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_swapchain'
```

Exit criteria: swapchain creation and cleanup are validation-clean, resize can be triggered repeatedly, and diagnostics report swapchain format, present mode, extent, and frame count.

Read first: Vulkan Tutorial swapchain chapters, Vulkan Guide WSI/swapchain notes, Vulkan Specification WSI sections, MoltenVK guide for Apple presentation constraints.

## Phase 5: Command Buffers And Frame Graph Baseline

Purpose: define minimal per-frame command recording and submission structure before real scene drawing.

Likely local paths:

- `src/render/vulkan/CommandBuffers.hpp`
- `src/render/vulkan/CommandBuffers.cpp`
- `src/render/vulkan/VulkanBackend.cpp`
- `src/render/vulkan/VulkanTypes.hpp`
- `tests/smoke/vulkan_empty_frame_smoke.cpp`

Data ownership: command module owns command pool, command buffers, per-frame recording lifecycle, and debug labels if available.

Dependency rules: command recording consumes renderer-owned resources and backend-neutral `FrameInput`; it never reaches into runtime state.

Compute/runtime cost expectations: one command buffer recording pass per frame. Empty frame cost should be near minimal and stable.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_empty_frame|vulkan_commands'
```

Exit criteria: backend records and submits an empty or clear-only frame without validation errors.

Read first: Vulkan Tutorial command buffer/drawing chapters, Vulkan Guide command buffer notes, Synchronization Validation docs.

## Phase 6: Synchronization, Frames-In-Flight, Validation/Syncval Gate

Purpose: make frame ownership explicit and catch synchronization mistakes before adding scene resources.

Likely local paths:

- `src/render/vulkan/FrameSync.hpp`
- `src/render/vulkan/FrameSync.cpp`
- `src/render/vulkan/DebugValidation.cpp`
- `src/render/vulkan/VulkanBackend.cpp`
- `tests/smoke/vulkan_sync_smoke.cpp`

Data ownership: sync module owns per-frame fences, semaphores, frame index, image acquisition/present ownership, and timeline or binary semaphore policy.

Dependency rules: sync primitives remain private to Vulkan backend. Public diagnostics can report policy and validation state only.

Compute/runtime cost expectations: bounded frames-in-flight memory and synchronization overhead. Default target is two frames in flight unless measurements justify otherwise.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_sync|vulkan_empty_frame'
```

Exit criteria: sync validation is enabled in development when available, empty-frame loop runs for sustained frames, resize/recreate remains clean, and the dynamic-rendering/timeline-semaphore baseline decision is either accepted or downgraded with evidence.

Read first: Vulkan Tutorial frames-in-flight/synchronization chapters, Vulkan Guide synchronization, LunarG Synchronization Validation, Vulkan Specification synchronization chapters.

## Phase 7: Shader/Pipeline Baseline For First 3D Room

Purpose: compile shaders and create a minimal graphics pipeline for drawing the first room.

Likely local paths:

- `src/render/vulkan/PipelinesShaders.hpp`
- `src/render/vulkan/PipelinesShaders.cpp`
- `shaders/vulkan/src/`
- `cmake/` shader build integration
- `tests/unit/render_shader_policy_tests.cpp`
- `tests/smoke/vulkan_pipeline_smoke.cpp`

Data ownership: pipeline module owns shader modules, pipeline layouts, descriptor layout decisions, pipeline objects, and shader diagnostics.

Dependency rules: shader language is a build/tooling decision. Runtime and projection never depend on Slang, GLSL, glslang, SPIR-V paths, descriptor layouts, or pipeline state.

Compute/runtime cost expectations: pipeline creation is startup/recreate cost; per-frame pipeline binding cost should be minimal. Shader compilation should happen at build time, not during normal runtime.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'shader_policy|vulkan_pipeline'
```

Exit criteria: shader toolchain is selected or explicitly deferred behind checked-in SPIR-V policy; first pipeline creation is validation-clean; diagnostics identify shader source and generated artifact provenance.

Read first: Vulkan Tutorial shaders/pipeline/depth chapters, Vulkan Guide Slang and shader-language docs, glslang docs, Vulkan Samples pipeline examples.

## Phase 8: GPU Resource Ownership, Buffers/Images, VMA Adoption

Purpose: introduce durable GPU resource ownership for geometry, uniforms, staging, depth, textures, and future material data.

Likely local paths:

- `src/render/vulkan/BuffersImagesMemory.hpp`
- `src/render/vulkan/BuffersImagesMemory.cpp`
- `src/render/vulkan/VulkanResult.hpp`
- `src/render/vulkan/VulkanResult.cpp`
- `tests/unit/render_memory_policy_tests.cpp`
- `tests/smoke/vulkan_memory_smoke.cpp`

Data ownership: memory module owns Vulkan buffers, images, allocations, mapped ranges, staging resources, depth images, allocation names, budgets, and memory diagnostics.

Dependency rules: VMA types stay private to Vulkan backend. Public renderer API exposes only backend-neutral resource status/diagnostics. No texture/material work until allocation naming and diagnostics exist.

Compute/runtime cost expectations: startup and asset-load allocation cost; per-frame uniform/staging updates must be bounded and measured. Avoid per-frame allocation churn.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_memory|vulkan_memory'
```

Exit criteria: VMA or documented allocation policy is in place, allocations are named, memory diagnostics are printable, and validation catches incorrect usage in development.

Read first: VMA overview and reference docs, Vulkan Guide memory chapters, Vulkan Tutorial buffer/image chapters, Vulkan Specification memory sections.

## Phase 9: Projection Consumption From SceneProjection, DebugProjection, And CameraState Only

Purpose: connect renderer input to `iggy3d` projection outputs without letting renderer read or mutate runtime truth.

Likely local paths:

- `src/render/FrameInput.hpp`
- `src/render/RendererApi.cpp`
- `src/projection/scene/`
- `src/projection/debug/`
- `src/runtime/camera/`
- `tests/unit/render_projection_input_tests.cpp`

Data ownership: projection produces backend-neutral scene/debug outputs. runtime camera owns camera truth. renderer consumes derived matrices/frame data and draw descriptions only.

Dependency rules: renderer may depend on projection value types selected for render input. Projection must not depend on renderer. Runtime must not depend on renderer. Renderer picking/input later routes through runtime commands, not direct mutation.

Compute/runtime cost expectations: frame input assembly should be linear in projected visible items and allocation-light. No gameplay recomputation in renderer.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_projection|camera_projection|dependency_firewall'
```

Exit criteria: renderer can draw or count a frame from `SceneProjection`, `DebugProjection`, and `CameraState` only; replay hash is unchanged with renderer enabled or disabled.

Read first: local projection/runtime docs, [boundaries.md](boundaries.md), Building a Simple Engine architecture notes.

## Phase 10: First Visual Demo App

Purpose: ship a small visual executable that renders the first room without becoming gameplay authority.

Likely local paths:

- `apps/iggy3d_visual_demo/main.cpp`
- `src/render/RendererApi.cpp`
- `src/render/vulkan/VulkanBackend.cpp`
- `fixtures/demos/first_room/`
- `tests/smoke/vulkan_first_room_smoke.cpp`

Data ownership: app owns user-facing loop orchestration; runtime owns session; projection owns frame outputs; renderer owns presentation.

Dependency rules: app may wire runtime, projection, and renderer together. The renderer still cannot call runtime mutation APIs directly.

Compute/runtime cost expectations: realtime target suitable for first-person/small-third-person play and tactical camera during slow time. First milestone optimizes for correctness and diagnostics, not final performance.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_first_room|headless_acceptance'
```

Exit criteria: first room presents a visible frame, camera matrices update from runtime-derived state, headless demo still passes, and renderer on/off does not affect deterministic replay result.

Read first: Vulkan Samples triangle/cube/depth/camera examples, Building a Simple Engine renderer flow, RenderDoc tooling docs.

## Phase 11: Renderer Smoke Tests And Diagnostics Receipt

Purpose: make renderer state observable enough to trust.

Likely local paths:

- `src/render/RenderDiagnostics.hpp`
- `src/render/RenderDiagnostics.cpp`
- `tests/smoke/vulkan_renderer_smoke.cpp`
- `tests/smoke/vulkan_diagnostics_smoke.cpp`

Data ownership: diagnostics owns a backend-neutral receipt generated by renderer state.

Dependency rules: diagnostics must not expose raw Vulkan handles outside Vulkan files. Smoke tests can be Vulkan-specific.

Compute/runtime cost expectations: diagnostics gathering should be startup/frame-bound and cheap unless an explicit verbose/debug flag is enabled.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_.*smoke|render_diagnostics'
```

Exit criteria: smoke output prints device name, API version, driver version, validation enabled, sync validation enabled, queue families, swapchain format, present mode, frame count, depth format, draw count, shader policy, memory allocation summary, and portability notes.

Read first: Vulkan Validation Overview, Synchronization Validation, VMA diagnostics docs, RenderDoc docs.

## Phase 12: Asset/Model/Material Growth After The First Room Renders

Purpose: expand from primitive room rendering into assets, models, materials, textures, and debug overlays.

Likely local paths:

- `src/render/vulkan/BuffersImagesMemory.cpp`
- `src/render/vulkan/PipelinesShaders.cpp`
- `src/render/FrameInput.hpp`
- `src/projection/scene/`
- `tests/unit/render_asset_projection_tests.cpp`
- `tests/smoke/vulkan_asset_room_smoke.cpp`

Data ownership: content/runtime define accepted asset identity and gameplay use. projection emits backend-neutral renderable descriptions. renderer owns GPU upload, residency, material pipeline binding, texture/image state, and fallback visuals.

Dependency rules: renderer must not validate package truth or invent content identity. Asset/model/material additions happen only after allocation naming and memory diagnostics exist.

Compute/runtime cost expectations: asset upload cost belongs to load/stream boundaries, not unbounded frame work. Per-frame material binding and draw count must be visible in diagnostics.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_asset|vulkan_asset|headless_acceptance'
```

Exit criteria: asset growth does not affect replay hash, missing asset fallback is visible and diagnosed, and GPU memory/draw diagnostics remain readable.

Read first: Vulkan Samples texture/depth/descriptors examples, VMA docs, Vulkan Guide descriptor/memory docs.

## Phase 13: Multiplayer/Replay-Safe Renderer Constraints

Purpose: ensure renderer work cannot compromise deterministic simulation, save/load, replay, or future network reconciliation.

Likely local paths:

- `src/render/RendererApi.hpp`
- `src/render/FrameInput.hpp`
- `src/runtime/save/`
- `src/runtime/replay/`
- `tests/acceptance/`
- `tests/unit/render_replay_invariance_tests.cpp`

Data ownership: runtime/replay owns deterministic results. renderer owns only presentation artifacts and non-authoritative diagnostics.

Dependency rules: renderer timing, frame drops, GPU errors, capture tools, and input picking must not alter runtime commands except through explicit command submission paths.

Compute/runtime cost expectations: renderer can run at variable display rate while runtime tick/replay rules remain deterministic. Slow-time tactical camera is runtime camera truth plus renderer presentation, not renderer simulation.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'replay|render_replay|headless_acceptance'
```

Exit criteria: deterministic replay and state hash are identical with null renderer, Vulkan renderer, and renderer disabled.

Read first: local replay/save docs, [boundaries.md](boundaries.md), Vulkan docs only for diagnostics that might affect runtime loop scheduling.

## Phase 14: Packaging And Platform Validation For macOS/MoltenVK, Linux, And Windows

Purpose: package and validate the renderer across all required target lanes without mistaking any one platform's quirks for engine truth.

Likely local paths:

- `cmake/`
- `apps/iggy3d_visual_demo/`
- `src/render/vulkan/`
- `docs/vulkan/manuals.md`
- `tests/smoke/vulkan_platform_smoke.cpp`

Data ownership: build/package scripts own SDK/tool discovery. renderer owns backend diagnostics. runtime remains platform-neutral.

Dependency rules: macOS/MoltenVK is the first local validation lane. Linux and Windows native Vulkan are required shipping lanes. Cross-platform Vulkan behavior follows Vulkan docs/spec; MoltenVK explains Apple constraints and exceptions.

Compute/runtime cost expectations: package startup should report backend setup cost and shader/resource load time. Runtime simulation cost remains independently measurable.

Validation command or test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_platform|vulkan_renderer_smoke|headless_acceptance'
```

Exit criteria: macOS/MoltenVK validates the local portability lane, Linux validates native Vulkan behavior, Windows validates native Vulkan behavior, and all three report the same renderer contract. Packaging records SDK, loader, validation, shader compiler, MoltenVK where applicable, and driver information.

Read first: LunarG macOS SDK guide, MoltenVK Runtime User Guide, Vulkan Registry/spec for portability behavior, Vulkan Guide platform/WSI notes, RenderDoc docs for capture setup where available.
