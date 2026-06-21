# Vulkan File Surface

This is the proposed future file, test, shader, and CMake surface for Vulkan integration. It is not starter implementation scope. The first renderer file plans should be created only after the headless runtime gate passes.

General include rule: Vulkan headers, `Vk*` types, and `VK_*` constants are allowed only in `src/render/vulkan/**`, Vulkan-specific app/platform glue, and Vulkan-specific smoke tests. SDL headers are allowed only in visual app/platform shell files and platform smoke tests.

## Layer Summary

| Layer | Purpose | Starts when |
| --- | --- | --- |
| Renderer API | backend-neutral render boundary and null renderer | after headless acceptance gate |
| Platform shell | SDL3 window/event/surface bridge | Phase 2 |
| Vulkan backend core | instance/device/surface/swapchain/commands/sync | Phases 3-6 |
| Shader/pipeline | GLSL to SPIR-V and first-room pipeline | Phase 7 |
| Resource model | buffers/images/depth/VMA/descriptors/material growth | Phase 8+ |
| Visual demo | first app wiring runtime/projection/renderer | Phase 10 |
| Tests/diagnostics | receipts, smoke tests, strict/skip lanes | Phases 1-14 |
| Packaging | shader/runtime dependency layout | Phase 14 |

## Renderer API Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected tests |
| --- | --- | --- | --- | --- | --- |
| `src/render/RendererApi.hpp` | Public renderer entry point used by apps. | Backend-neutral renderer lifecycle API, backend selection, frame submission shape. | Gameplay state, package validation, save truth, replay rules, Vulkan/SDL handles. | Allow standard library and backend-neutral render/projection value headers. Forbid Vulkan, SDL, and platform-window headers. | `tests/unit/render_boundary_tests.cpp`, replay invariance tests. |
| `src/render/RendererApi.cpp` | Renderer API implementation and backend factory bridge. | Backend creation, null/Vulkan backend selection behind build options, high-level error conversion. | Device selection internals, swapchain internals, runtime mutation. | May include `RenderBackend.hpp`, null backend, Vulkan backend behind options. Forbid leaking `Vk*` through public data. | Boundary tests, null renderer tests, smoke construction tests. |
| `src/render/FrameInput.hpp` | Backend-neutral per-frame input contract. | Viewport, frame clock, derived camera matrices, scene/debug projection references. | Runtime authority, save data, raw input events, Vulkan descriptors, SDL events. | Allow core math, projection values, camera value headers. Forbid Vulkan, SDL, backend internals. | `tests/unit/render_projection_input_tests.cpp`, `tests/unit/render_replay_invariance_tests.cpp`. |
| `src/render/RenderDiagnostics.hpp` | Backend-neutral diagnostics receipt data. | Stable receipt fields, result/reason codes, backend/platform summaries. | Raw Vulkan handles, gameplay state, authoritative errors. | Standard library only where possible. Forbid Vulkan and SDL. | Diagnostics unit tests, smoke receipt tests. |
| `src/render/RenderDiagnostics.cpp` | Receipt formatting and aggregation. | Key-value receipt text, field normalization, artifact-path helpers if needed. | Device probing, validation-layer setup, runtime mutation. | Allow `RenderDiagnostics.hpp`. Forbid Vulkan and SDL. | Snapshot-style diagnostics tests. |
| `src/render/RenderBackend.hpp` | Private backend interface behind `RendererApi`. | Abstract lifecycle contract: initialize, render frame, resize, diagnostics, wait idle, shutdown. | Public Vulkan API, gameplay commands, projection production. | Allow `FrameInput.hpp` and diagnostics. Forbid Vulkan and SDL. | Backend contract tests. |
| `src/render/null/NullRenderer.hpp` | Null backend declaration. | No-op renderer implementation type. | Vulkan setup, windowing, gameplay mutation. | Allow render interfaces only. Forbid Vulkan, SDL, platform headers. | Null renderer unit tests. |
| `src/render/null/NullRenderer.cpp` | Null backend behavior. | Frame count/draw count diagnostics for tests. | GPU resources, swapchain, shaders. | Allow render interfaces only. Forbid Vulkan, SDL, platform headers. | Null renderer and replay invariance tests. |

## Platform Shell Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected tests |
| --- | --- | --- | --- | --- | --- |
| `src/app/platform/SdlWindow.hpp` | SDL window wrapper declaration. | Backend-neutral window state API, event polling interface, framebuffer size query. | Runtime command legality, Vulkan swapchain, GPU resources. | May include SDL if wrapper is app-private. Forbid Vulkan unless split requires otherwise. | Platform shell unit/smoke tests. |
| `src/app/platform/SdlWindow.cpp` | SDL init/window/event implementation. | SDL video init/shutdown, SDL window lifetime, app-event conversion. | Runtime mutation, Vulkan device/swapchain, save/replay truth. | SDL allowed. Forbid runtime internals beyond app-level command routing types. | `tests/smoke/vulkan_platform_smoke.cpp`. |
| `src/app/platform/SdlVulkanSurface.hpp` | SDL Vulkan surface bridge declaration. | Required instance extension query, surface creation bridge shape. | Swapchain, device selection, runtime state. | SDL Vulkan and Vulkan headers allowed only if this file remains platform/Vulkan glue. Forbid runtime/content/projection/save. | Platform/device smoke tests. |
| `src/app/platform/SdlVulkanSurface.cpp` | SDL Vulkan extension/surface implementation. | `SDL_Vulkan_GetInstanceExtensions`, `SDL_Vulkan_CreateSurface` bridge, surface error diagnostics. | Device/swapchain ownership, gameplay input, runtime mutation. | SDL/Vulkan allowed. Forbid runtime internals. | `tests/smoke/vulkan_platform_smoke.cpp`, `tests/smoke/vulkan_device_smoke.cpp`. |
| `src/render/vulkan/VulkanSurfaceProvider.hpp` or private equivalent | Vulkan-specific callback contract for surface creation. | Required instance extensions and `VkSurfaceKHR` creation callback type. | Generic renderer API, runtime/projection types. | Vulkan allowed. Keep under `src/render/vulkan/**` or Vulkan-specific app glue. | Compile/firewall and platform smoke tests. |

## Vulkan Backend Core Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected tests |
| --- | --- | --- | --- | --- | --- |
| `src/render/vulkan/VulkanBackend.hpp` | Vulkan backend facade hidden behind `RenderBackend`. | Backend object lifetime, module composition, bridge from backend-neutral API to Vulkan modules. | Runtime state, package validation, projection creation. | May include private Vulkan wrappers. Public declarations should avoid raw `Vk*` unless private to Vulkan files. | Backend construction/smoke tests. |
| `src/render/vulkan/VulkanBackend.cpp` | Wires Vulkan modules into frame rendering. | Initialize/draw/resize/wait/shutdown flow. | Low-level ownership better held by modules, gameplay mutation. | Vulkan and Vulkan module headers allowed. Forbid runtime internals beyond frame input values. | Empty frame, first room, resize smoke tests. |
| `src/render/vulkan/InstanceDeviceSurface.hpp` | Instance, physical device, logical device, queues, surface contract. | Instance/device/surface handles, queue families, enabled features/extensions. | Swapchain images, command buffers, app event loop. | Vulkan allowed. Forbid runtime/content/save headers. | Device diagnostics smoke tests. |
| `src/render/vulkan/InstanceDeviceSurface.cpp` | Device/surface setup implementation. | Extension/layer discovery, device scoring, queue creation, surface ownership. | App event loop, gameplay rules. | Vulkan allowed; platform surface bridge only through approved provider. | Device selection and portability tests. |
| `src/render/vulkan/Swapchain.hpp` | Swapchain lifecycle interface. | Swapchain handles, image views, format, extent, present mode, image count. | Surface creation, runtime camera truth, command recording. | Vulkan allowed. Forbid runtime/content/save headers. | Swapchain recreate smoke tests. |
| `src/render/vulkan/Swapchain.cpp` | Swapchain creation/recreation/destruction. | Format/present/extent selection, image view ownership, recreate state. | Window event polling, runtime state changes. | Vulkan allowed. Forbid runtime internals. | Resize/recreate validation tests. |
| `src/render/vulkan/CommandBuffers.hpp` | Command pool/buffer ownership. | Command pools, command buffers, recording lifecycle. | Swapchain selection, shader compilation, runtime state. | Vulkan allowed. Forbid runtime/content/save headers. | Empty frame command tests. |
| `src/render/vulkan/CommandBuffers.cpp` | Command recording implementation. | Begin/end command buffers, dynamic rendering commands, debug labels if enabled. | Gameplay decisions, projection production. | Vulkan allowed. Forbid runtime mutation APIs. | Empty frame, first room smoke tests. |
| `src/render/vulkan/FrameSync.hpp` | Frames-in-flight and synchronization policy. | Fences, binary WSI semaphores, frame index, acquire/present sync state. | Runtime tick scheduling, replay timing truth. | Vulkan allowed. Forbid runtime/content/save headers. | Sync validation smoke tests. |
| `src/render/vulkan/FrameSync.cpp` | Synchronization implementation. | Wait/reset/signal/acquire/present sequencing. | Command recording content, runtime mutation. | Vulkan allowed. Forbid runtime internals. | Sustained empty-frame and resize sync tests. |
| `src/render/vulkan/DebugValidation.hpp` | Validation/debug messenger interface. | Validation enablement policy, debug callback plumbing, syncval flags. | Gameplay logging authority, release crash policy. | Vulkan allowed. Forbid runtime/content/save headers. | Validation-enabled smoke tests. |
| `src/render/vulkan/DebugValidation.cpp` | Validation/debug messenger implementation. | Layer discovery, debug utils messenger, message routing. | Runtime mutation, gameplay error handling. | Vulkan allowed. Forbid runtime internals. | Validation layer availability tests. |
| `src/render/vulkan/VulkanTypes.hpp` | Private handle wrappers and small Vulkan-only structs. | RAII wrappers or handle bundles, queue family structs, feature structs. | Public renderer API, gameplay types. | Vulkan allowed. Forbid non-render runtime headers. | Compile/firewall tests. |
| `src/render/vulkan/VulkanResult.hpp` | Error conversion declarations. | Vulkan result-to-render-error conversion signatures. | Runtime errors, gameplay messages. | Vulkan allowed only because file is Vulkan-private. | Result conversion tests. |
| `src/render/vulkan/VulkanResult.cpp` | Error conversion implementation. | Stable conversion from `VkResult` and context strings into diagnostics/errors. | Recovery policy that belongs to backend/app. | Vulkan allowed. Forbid runtime internals. | Result conversion unit tests. |

## Shader And Pipeline Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected tests |
| --- | --- | --- | --- | --- | --- |
| `src/render/vulkan/PipelinesShaders.hpp` | Shader and pipeline ownership. | Shader module loading, pipeline layouts, push constants, future descriptor layouts, graphics pipelines. | Shader language choice in runtime, material gameplay rules. | Vulkan allowed. Forbid runtime/content/save headers. | Shader policy and pipeline smoke tests. |
| `src/render/vulkan/PipelinesShaders.cpp` | Shader/pipeline implementation. | SPIR-V loading, shader modules, pipeline layout, first-room graphics pipeline. | Runtime state, asset acceptance. | Vulkan allowed. Forbid runtime internals. | Pipeline creation validation tests. |
| `shaders/vulkan/src/first_room.vert.glsl` | First-room vertex shader source. | Position/color input, `clipFromModel` push constant use. | Gameplay logic, package validation, generated output. | Shader language only. No C++ includes. | Shader compile tests. |
| `shaders/vulkan/src/first_room.frag.glsl` | First-room fragment shader source. | Vertex color to fragment output. | Lighting/material gameplay truth, texture policy. | Shader language only. No C++ includes. | Shader compile tests. |
| `build/generated/shaders/vulkan/<config>/first_room.vert.spv` | Generated vertex SPIR-V. | Build artifact emitted by shader compiler. | Source of truth unless packaging later commits derived artifacts. | Generated files only. | SPIR-V existence/reproducibility checks. |
| `build/generated/shaders/vulkan/<config>/first_room.frag.spv` | Generated fragment SPIR-V. | Build artifact emitted by shader compiler. | Source of truth unless packaging later commits derived artifacts. | Generated files only. | SPIR-V existence/reproducibility checks. |

## Resource Model Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected tests |
| --- | --- | --- | --- | --- | --- |
| `src/render/vulkan/BuffersImagesMemory.hpp` | GPU resources and allocation API. | Buffers, images, allocation handles, staging resources, depth resources, allocation names. | Content package truth, material semantics, runtime inventory. | Vulkan/VMA allowed when adopted. Forbid runtime/content/save headers. | Memory policy and allocation smoke tests. |
| `src/render/vulkan/BuffersImagesMemory.cpp` | GPU allocation/upload implementation. | Bootstrap allocations or VMA, mapping, upload, budget diagnostics. | Per-frame unbounded allocation, package validation. | Vulkan/VMA allowed. Forbid runtime internals. | VMA/memory diagnostics tests. |
| `third_party/vma/` or `external/vma/` | Pinned VMA source/header dependency. | Vendored VMA snapshot when Phase 8 starts. | Runtime code, project-specific gameplay logic. | Third-party code only. No edits except documented vendoring. | Dependency/version checks. |

## App And Packaging Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected tests |
| --- | --- | --- | --- | --- | --- |
| `apps/iggy3d_visual_demo/main.cpp` | First visual executable. | App loop, SDL shell wiring, runtime/projection/renderer orchestration, CLI flags. | Runtime internals, renderer module internals, save truth. | May include app/window headers and public renderer/runtime APIs. Avoid direct Vulkan unless constructing Vulkan-specific provider. | Visual demo smoke tests. |
| `cmake/iggy3d_vulkan_deps.cmake` | Vulkan/SDL/VMA dependency discovery. | Feature-gated dependency lookup, imported target wiring, diagnostics. | Runtime behavior, old repo paths. | CMake only. | Configure/dependency smoke. |
| `cmake/iggy3d_shaders.cmake` | Shader compile CMake helpers. | glslang discovery, SPIR-V custom commands, shader aggregate target. | Runtime behavior, package validation. | CMake only. | Shader policy tests. |
| `cmake/iggy3d_install.cmake` | Future install/package layout. | Shader/resource install paths, runtime dependency install/copy rules. | Gameplay state, renderer behavior. | CMake only. | Package smoke tests. |
| `build/artifacts/render_diagnostics/` | Diagnostics artifact root. | Receipts, validation logs, shader logs, optional screenshots/captures. | Source truth, replay input. | Generated artifacts only. | Diagnostics smoke tests. |

## Test Surface

| Path | Purpose | Owns | Must never own | Includes allowed/forbidden | Expected labels |
| --- | --- | --- | --- | --- | --- |
| `tests/unit/render_boundary_tests.cpp` | Renderer dependency firewall and null-boundary tests. | Generic render API boundary checks. | Vulkan platform behavior. | Forbid Vulkan and SDL. | `iggy3d;unit;render` |
| `tests/unit/render_projection_input_tests.cpp` | `FrameInput` contract tests. | Viewport/camera/projection validation behavior. | Runtime mutation. | Forbid Vulkan and SDL. | `iggy3d;unit;render` |
| `tests/unit/render_replay_invariance_tests.cpp` | Renderer on/off replay hash proof. | Null/Vulkan-enabled invariance shape. | Renderer as gameplay authority. | Generic path forbids Vulkan; Vulkan lane may be separate. | `iggy3d;render;replay` |
| `tests/unit/render_shader_policy_tests.cpp` | Shader path and policy tests. | Source/generated path rules, compiler-required behavior. | GPU pipeline behavior unless explicitly split. | Forbid Vulkan unless testing Vulkan shader module creation. | `iggy3d;unit;render;shader` |
| `tests/unit/render_memory_policy_tests.cpp` | Resource naming/policy tests. | Depth format order, allocation naming, diagnostics fields. | GPU allocation unless smoke test. | Forbid Vulkan unless clearly named backend unit. | `iggy3d;unit;render;memory` |
| `tests/smoke/vulkan_platform_smoke.cpp` | SDL/window/surface smoke. | Window creation, extension query, surface creation, receipt. | Runtime authority. | SDL/Vulkan allowed. | `iggy3d;vulkan;gpu;requires_display;smoke;platform` |
| `tests/smoke/vulkan_device_smoke.cpp` | Device diagnostics smoke. | Instance/device/queue/extension receipt. | Swapchain/rendering unless test scope includes it. | Vulkan allowed. | `iggy3d;vulkan;gpu;smoke;diagnostics` |
| `tests/smoke/vulkan_swapchain_smoke.cpp` | Swapchain smoke. | Swapchain create/recreate/destroy receipt. | Runtime state. | Vulkan allowed. | `iggy3d;vulkan;gpu;requires_display;smoke` |
| `tests/smoke/vulkan_empty_frame_smoke.cpp` | Empty/clear frame smoke. | Command recording/submission first frame. | Scene rendering. | Vulkan allowed. | `iggy3d;vulkan;gpu;smoke` |
| `tests/smoke/vulkan_sync_smoke.cpp` | Frames-in-flight and sync validation smoke. | Binary WSI sync path and sync receipt. | Runtime timing truth. | Vulkan allowed. | `iggy3d;vulkan;gpu;smoke;sync` |
| `tests/smoke/vulkan_pipeline_smoke.cpp` | Shader/pipeline smoke. | SPIR-V load, shader module, pipeline layout/pipeline creation. | Runtime state. | Vulkan allowed. | `iggy3d;vulkan;gpu;smoke;shader` |
| `tests/smoke/vulkan_memory_smoke.cpp` | GPU memory/resource smoke. | Vertex/index/depth resources, upload, memory receipt. | Content validation truth. | Vulkan/VMA allowed. | `iggy3d;vulkan;gpu;smoke;memory` |
| `tests/smoke/vulkan_first_room_smoke.cpp` | First visible room smoke. | Draw first room, depth, receipt, optional screenshot/frame hash later. | Gameplay truth. | Vulkan allowed. | `iggy3d;vulkan;gpu;requires_display;smoke` |
| `tests/smoke/vulkan_diagnostics_smoke.cpp` | Receipt completeness smoke. | Key-value receipt schema and required fields. | Renderer behavior beyond diagnostics. | Vulkan allowed if exercising backend. | `iggy3d;vulkan;gpu;smoke;diagnostics` |
| `tests/smoke/package_headless_smoke.cpp` | Headless package proof. | Package runs without SDL/Vulkan/shaders. | Visual startup. | Forbid Vulkan and SDL. | `iggy3d;packaging;headless` |
| `tests/smoke/package_visual_startup_smoke.cpp` | Visual package startup proof. | Runtime dependency lookup, shader root, one-frame startup. | Runtime truth. | SDL/Vulkan allowed. | `iggy3d;packaging;vulkan;requires_display` |
| `tests/smoke/package_shader_lookup_smoke.cpp` | Packaged shader lookup proof. | Shader root exists/missing behavior. | Shader compilation unless explicitly part of test. | No Vulkan required unless loading modules. | `iggy3d;packaging;shader` |
| `tests/smoke/package_vulkan_dependency_smoke.cpp` | Packaged dependency proof. | Vulkan/SDL/MoltenVK/runtime dependency receipt. | Runtime mutation. | Platform/Vulkan allowed. | `iggy3d;packaging;vulkan` |

## Generated SPIR-V Policy

Default policy: commit shader source, generate SPIR-V in the build tree, and record compiler/version inputs in diagnostics. Commit generated SPIR-V only if packaging or platform constraints require it, and then treat generated files as derived artifacts with a reproducibility check.

The renderer consumes SPIR-V. The rest of the engine consumes none of the shader toolchain.

## File Plan Read Order

When creating implementation file plans from this surface, read:

1. [boundaries.md](boundaries.md)
2. [frame_input_contract.md](frame_input_contract.md)
3. [decisions.md](decisions.md)
4. [platform_shell.md](platform_shell.md), for app/platform files
5. [shader_pipeline.md](shader_pipeline.md), for shader/pipeline files
6. [resource_model.md](resource_model.md), for memory/resource files
7. [diagnostics_and_tests.md](diagnostics_and_tests.md), for tests/receipts
8. [packaging.md](packaging.md), for install/package files
9. [platform_matrix.md](platform_matrix.md), for platform gates

## Acceptance Criteria

This file surface is ready for implementation file plans when:

- every planned renderer/platform/shader/resource/test/package path has an owner;
- every path says what it must never own;
- include boundaries are explicit;
- test expectations are named;
- SDL and Vulkan leakage rules are clear;
- headless runtime remains independent;
- old renderer code remains historical reference only.
