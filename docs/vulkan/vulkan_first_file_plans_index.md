# Vulkan First File Plans Index

This document is the bridge from Vulkan planning docs to the first implementation file-plan packets for `iggy3d`.

It does not authorize implementation yet. It defines the first concrete file-plan order once the headless runtime gate is green and the renderer boundary work is allowed to start.

The detailed one-file-per-file plans now live under:

```text
docs/vulkan/file_plans/
```

Start with [file_plans/PRIORITY.md](file_plans/PRIORITY.md), then open the specific plan file listed for the implementation packet.

## Purpose

Turn the Vulkan docs into a narrow implementation packet ladder:

```text
01 RendererApi
02 NullRenderer
03 RenderDiagnostics
04 Platform shell
05 InstanceDeviceSurface
06 Swapchain
07 CommandBuffers
08 FrameSync
09 Shader build
10 First-room pipeline
```

This index is intentionally smaller than the full renderer roadmap. It covers only the first file plans needed to get from no renderer to a validation-clean first visible room path.

## Hard Gate

Do not create implementation file plans from this index until:

- headless runtime acceptance is green;
- runtime replay/hash behavior is tested;
- projection can produce backend-neutral scene/debug output;
- camera truth is owned outside the renderer;
- Vulkan dependency firewall scan is clean;
- legacy renderer material remains historical reference only.

Gate command shape:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Dependency firewall scan:

```sh
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' \
  src/runtime src/content src/projection src/runtime/save
```

Expected result:

```text
no matches
```

## Packet Rules

Each packet creates a file plan first. Do not combine packets unless the reviewer explicitly accepts the merge.

Every file plan must state:

- exact files to create or modify;
- owner for each file;
- forbidden ownership;
- allowed and forbidden includes;
- diagnostics fields added or consumed;
- unit/smoke gates;
- strict versus optional behavior when relevant;
- docs to read first;
- exit criteria;
- blocked later work.

Hard rules:

- renderer config and diagnostics do not become runtime truth;
- `Vk*`, `VK_*`, and Vulkan headers stay out of runtime/content/projection/save;
- SDL/window headers stay out of runtime/content/projection/save;
- first visible room comes before textures, materials, model streaming, lighting, picking, or editor visuals;
- VMA/material growth waits until the first-room path and memory diagnostics are stable.

## First Order Summary

| Order | Packet | Primary output | May include Vulkan? | Main proof |
| --- | --- | --- | --- | --- |
| 01 | `RendererApi` | backend-neutral render API | no | public renderer boundary compiles without Vulkan/SDL |
| 02 | `NullRenderer` | no-op backend | no | frame consumption does not mutate runtime/replay |
| 03 | `RenderDiagnostics` | receipt/result/reason plumbing | no public Vulkan | stable key-value diagnostics and reason codes |
| 04 | platform shell | visual app/window/surface provider shape | only in Vulkan-specific glue | SDL/window boundary does not leak |
| 05 | instance/device/surface | Vulkan backend bootstrap diagnostics | yes, private | device receipt prints instance/device/surface facts |
| 06 | swapchain | presentable image lifecycle | yes, private | create/recreate/destroy validation-clean |
| 07 | command buffers | empty/clear frame command path | yes, private | record/submit empty frame |
| 08 | sync | frames-in-flight and sync validation | yes, private | sync validation gate is meaningful |
| 09 | shader build | GLSL/glslang to SPIR-V path | CMake/Vulkan shader modules | reproducible first shader artifacts |
| 10 | first-room pipeline | first visible room pipeline | yes, private | first room draw path is ready for proof |

## Packet 01: RendererApi

Purpose: create the backend-neutral renderer entry point and lifecycle shape.

Likely file plans:

```text
src/render/RendererApi.hpp
src/render/RendererApi.cpp
src/render/RenderBackend.hpp
src/render/FrameInput.hpp
tests/unit/render_boundary_tests.cpp
```

Owns:

- renderer lifecycle API;
- backend selection as backend-neutral values;
- frame submission boundary;
- `FrameInput` validation surface;
- no-op stub wiring until `NullRenderer` lands.

Must never own:

- runtime gameplay state;
- command legality;
- save/replay truth;
- Vulkan handles;
- SDL window handles;
- shader paths;
- swapchain state.

Include rules:

```text
allowed=standard_library,backend_neutral_math,projection_value_types,render_headers
forbidden=vulkan_headers,Vk_types,VK_constants,SDL_headers,platform_window_headers
```

Read first:

- [boundaries.md](boundaries.md)
- [frame_input_contract.md](frame_input_contract.md)
- [camera_render_contract.md](camera_render_contract.md)
- [file_surface.md](file_surface.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_boundary'
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' \
  src/render/RendererApi.hpp src/render/RenderBackend.hpp src/render/FrameInput.hpp
```

Exit criteria:

- public renderer headers compile;
- public renderer API exposes no Vulkan/SDL types;
- frame input shape is backend-neutral;
- headless runtime tests still pass.

Blocked later work:

- Vulkan backend;
- visual app shell;
- shader compiler;
- GPU tests.

## Packet 02: NullRenderer

Purpose: create the first backend implementation without GPU, window, or shader dependencies.

Likely file plans:

```text
src/render/null/NullRenderer.hpp
src/render/null/NullRenderer.cpp
tests/unit/render_null_renderer_tests.cpp
tests/unit/render_replay_invariance_tests.cpp
```

Owns:

- no-op render backend;
- frame count and draw count diagnostics;
- backend-neutral success/failure result shape;
- replay/hash invariance proof for renderer consumption.

Must never own:

- Vulkan setup;
- SDL setup;
- GPU resources;
- shader loading;
- content validation;
- runtime mutation.

Include rules:

```text
allowed=src/render/RenderBackend.hpp,src/render/FrameInput.hpp,src/render/RenderDiagnostics.hpp
forbidden=vulkan_headers,Vk_types,VK_constants,SDL_headers,runtime_mutation_internals
```

Read first:

- [boundaries.md](boundaries.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [frame_input_contract.md](frame_input_contract.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_null|render_replay|render_boundary'
```

Exit criteria:

- null renderer can be constructed and shut down;
- null renderer can consume a valid `FrameInput`;
- invalid `FrameInput` is rejected without runtime mutation;
- replay hash is unchanged with renderer enabled versus disabled.

Blocked later work:

- windowed app;
- Vulkan instance;
- swapchain;
- command buffers.

## Packet 03: RenderDiagnostics

Purpose: create stable diagnostics and result/reason plumbing before Vulkan emits receipts.

Likely file plans:

```text
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/RendererConfig.hpp
src/render/RendererConfig.cpp
tests/unit/render_diagnostics_tests.cpp
tests/unit/render_result_reason_tests.cpp
tests/unit/render_config_tests.cpp
```

Owns:

- key-value receipt format;
- stable renderer reason codes;
- backend-neutral renderer outcome names;
- renderer config resolved-value receipt fields;
- artifact-path policy for diagnostics.

Must never own:

- raw Vulkan handles;
- gameplay errors;
- save/replay serialization;
- driver probing;
- validation layer setup;
- shader compilation.

Include rules:

```text
allowed=standard_library,filesystem_if_needed,src/render_headers
forbidden=vulkan_headers,Vk_types,VK_constants,SDL_headers,runtime_save_headers
```

Read first:

- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_diagnostics|render_config|render_result'
```

Exit criteria:

- receipt keys are lowercase snake_case;
- pass/fail/skip reason codes are stable;
- renderer config is typed before backend use;
- CTest exit code policy stays in app/test wrapper, not renderer core;
- diagnostics do not expose Vulkan handles through public API.

Blocked later work:

- validation callback routing;
- Vulkan result conversion implementation;
- device receipts.

## Packet 04: Platform Shell

Purpose: plan the first visual app shell and window/surface boundary before Vulkan device ownership.

Likely file plans:

```text
apps/iggy3d_visual_demo/main.cpp
src/app/platform/SdlWindow.hpp
src/app/platform/SdlWindow.cpp
src/app/platform/SdlVulkanSurface.hpp
src/app/platform/SdlVulkanSurface.cpp
cmake/iggy3d_vulkan_deps.cmake
tests/smoke/vulkan_platform_smoke.cpp
```

Owns:

- visual demo process entry;
- CLI/env renderer config parsing for visual/smoke app;
- SDL init/shutdown;
- window lifetime;
- event polling;
- framebuffer/drawable extent query;
- Vulkan surface extension query and surface creation bridge.

Must never own:

- Vulkan logical device;
- swapchain;
- command buffers;
- frame sync;
- runtime command legality;
- projection production;
- save/replay truth.

Include rules:

```text
SdlWindow_allowed=SDL_headers
SdlVulkanSurface_allowed=SDL_headers,Vulkan_headers_only_as_platform_vulkan_glue
visual_demo_allowed=public_runtime_api,public_projection_api,public_renderer_api,platform_shell
forbidden_in_runtime=SDL_headers,Vulkan_headers
```

Read first:

- [platform_shell.md](platform_shell.md)
- [vulkan_surface_wsi_platforms.md](vulkan_surface_wsi_platforms.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [platform_matrix.md](platform_matrix.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_platform|render_config'
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan)' src/runtime src/content src/projection src/runtime/save
```

Exit criteria:

- visual app can start without changing headless tools;
- window/platform shell is isolated;
- required Vulkan instance extensions can be queried;
- surface creation contract is documented and diagnosed;
- optional lane may skip missing display, strict lane fails missing required display.

Blocked later work:

- Vulkan device creation if platform shell cannot provide required extensions;
- swapchain;
- first-room draw.

## Packet 05: InstanceDeviceSurface

Purpose: plan Vulkan instance, validation, physical/logical device, queue, and surface ownership.

Likely file plans:

```text
src/render/vulkan/VulkanBackend.hpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/unit/render_result_mapping_tests.cpp
```

Owns:

- Vulkan instance;
- validation layer discovery;
- debug messenger lifetime;
- physical device enumeration;
- device selection diagnostics;
- logical device;
- queue family selection;
- `VkSurfaceKHR` ownership after platform creation;
- enabled features/extensions receipt.

Must never own:

- SDL event loop;
- swapchain image views;
- command buffers;
- gameplay state;
- save/replay truth;
- projection creation.

Include rules:

```text
allowed=Vulkan_headers,src/render/vulkan_private_headers,src/render_diagnostics
forbidden=runtime_mutation_headers,content_package_mutation,SDL_window_direct_dependency_except_approved_provider
```

Read first:

- [device_selection.md](device_selection.md)
- [debug_validation.md](debug_validation.md)
- [vulkan_feature_baseline.md](vulkan_feature_baseline.md)
- [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md)
- [vulkan_function_loading.md](vulkan_function_loading.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_surface_wsi_platforms.md](vulkan_surface_wsi_platforms.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|render_result'
```

Required receipt fields:

```text
device_name
api_version
driver_version
validation
sync_validation
debug_messenger
graphics_queue_family
present_queue_family
enabled_instance_extensions
enabled_device_extensions
portability_subset
```

Exit criteria:

- instance/device/surface setup is validation-clean;
- missing loader/driver/extensions map through stable reason codes;
- MoltenVK/macOS portability is diagnosed but not cross-platform authority;
- raw Vulkan types stay under Vulkan-owned files.

Blocked later work:

- swapchain;
- frame rendering;
- shaders;
- GPU memory resources.

## Packet 06: Swapchain

Purpose: plan presentable image ownership, format/present-mode selection, extent selection, and recreate lifecycle.

Likely file plans:

```text
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/VulkanBackend.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_minimize_smoke.cpp
```

Owns:

- `VkSwapchainKHR`;
- swapchain image view ownership;
- selected surface format/color space;
- selected present mode;
- selected extent;
- image count;
- resize/recreate state;
- zero-extent/not-drawable state;
- old-swapchain handoff policy.

Must never own:

- platform event polling;
- runtime camera truth;
- command buffer recording;
- sync primitive lifetime;
- depth images beyond reporting required coupling.

Include rules:

```text
allowed=Vulkan_headers,InstanceDeviceSurface,RenderDiagnostics,VulkanResult
forbidden=runtime_headers,content_headers,save_headers,SDL_event_loop_headers
```

Read first:

- [swapchain_contract.md](swapchain_contract.md)
- [vulkan_swapchain_failure_modes.md](vulkan_swapchain_failure_modes.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [platform_shell.md](platform_shell.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_swapchain|vulkan_resize|vulkan_minimize'
```

Required receipt fields:

```text
swapchain_created
swapchain_format
swapchain_color_space
present_mode_requested
present_mode_selected
swapchain_extent
swapchain_image_count
resize_recreate_count
drawable
```

Exit criteria:

- swapchain creation and destruction are validation-clean;
- zero extent skips before acquire;
- resize/recreate is explicit;
- out-of-date/suboptimal handling routes through result policy;
- runtime hash is unaffected by resize/minimize.

Blocked later work:

- command recording;
- presentable frames;
- depth resource allocation;
- first-room pipeline.

## Packet 07: CommandBuffers

Purpose: plan command pool/buffer ownership and empty or clear-only frame recording.

Likely file plans:

```text
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/VulkanBackend.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
tests/unit/render_command_recording_policy_tests.cpp
```

Owns:

- command pool;
- per-frame command buffers;
- begin/end command buffer lifecycle;
- dynamic rendering begin/end for empty or clear-only path;
- debug command labels when available;
- command recording diagnostics.

Must never own:

- swapchain selection;
- synchronization wait/reset policy;
- shader compilation;
- gameplay decisions;
- projection production.

Include rules:

```text
allowed=Vulkan_headers,Swapchain_metadata,RenderDiagnostics,VulkanResult
forbidden=runtime_mutation_headers,content_package_headers,save_headers
```

Read first:

- [command_recording.md](command_recording.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [vulkan_debug_labels_and_capture.md](vulkan_debug_labels_and_capture.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_empty_frame|render_command'
```

Required receipt fields:

```text
command_pool_created
command_buffer_count
rendering_path
dynamic_rendering_enabled
command_label_count
draw_count
```

Exit criteria:

- command buffers are allocated and reset according to policy;
- empty/clear-only frame records without validation errors;
- no scene/runtime data is read directly by command recording;
- dynamic rendering baseline or fallback is diagnosed.

Blocked later work:

- first-room shader pipeline;
- vertex/index buffers;
- descriptor sets;
- texture/material work.

## Packet 08: FrameSync

Purpose: plan WSI semaphores, fences, frames-in-flight, acquire/submit/present order, and sync validation gate.

Likely file plans:

```text
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/DebugValidation.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/unit/render_sync_policy_tests.cpp
```

Owns:

- frames-in-flight count;
- image-available semaphores;
- render-finished semaphores;
- in-flight fences;
- frame index;
- acquire/submit/present sync order;
- fence reset safety;
- sync validation diagnostics.

Must never own:

- runtime tick scheduling;
- replay timing truth;
- swapchain format/extent selection;
- command contents;
- GPU resource uploads beyond first explicit coordination points.

Include rules:

```text
allowed=Vulkan_headers,VulkanResult,RenderDiagnostics,Swapchain_frame_metadata
forbidden=runtime_headers,content_headers,save_headers
```

Read first:

- [sync_contract.md](sync_contract.md)
- [vulkan_swapchain_failure_modes.md](vulkan_swapchain_failure_modes.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_sync|vulkan_empty_frame'
```

Required receipt fields:

```text
sync_policy
frames_in_flight
acquire_semaphore_count
render_finished_semaphore_count
in_flight_fence_count
sync_validation
sync_validation_clean
```

Exit criteria:

- default frames in flight is `2` unless evidence changes it;
- WSI path uses binary semaphores for first implementation;
- fence reset never happens unless a submit will signal it;
- sync validation required mode fails correctly;
- optional mode reports unavailable sync validation instead of pretending clean proof.

Blocked later work:

- timeline semaphore adoption;
- async uploads;
- multi-queue scheduling;
- advanced frame graph.

## Packet 09: Shader Build

Purpose: plan first shader source, compiler discovery, generated SPIR-V layout, and stale artifact detection.

Likely file plans:

```text
cmake/iggy3d_shaders.cmake
shaders/vulkan/src/first_room.vert.glsl
shaders/vulkan/src/first_room.frag.glsl
tests/unit/render_shader_policy_tests.cpp
tests/smoke/vulkan_shader_build_smoke.cpp
```

Owns:

- GLSL first-room shader source;
- glslang discovery;
- generated SPIR-V paths;
- Windows multi-config shader output policy;
- stale artifact checks;
- shader build diagnostics.

Must never own:

- runtime scene meaning;
- content package validation;
- material system;
- texture policy;
- pipeline layout compatibility beyond first shader interface facts.

Include rules:

```text
allowed=CMake_shader_helpers,shader_source_files,render_shader_policy_tests
forbidden=runtime_shader_dependencies,content_shader_dependencies,projection_shader_dependencies
```

Read first:

- [shader_pipeline.md](shader_pipeline.md)
- [vulkan_shader_build_pipeline.md](vulkan_shader_build_pipeline.md)
- [shader_interface_contract.md](shader_interface_contract.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'shader_policy|vulkan_shader_build'
```

Required receipt/build facts:

```text
shader_language=glsl
shader_compiler
shader_target_env
shader_source_root
shader_binary_root
vertex_shader
fragment_shader
shader_artifacts_found
```

Exit criteria:

- first-room GLSL sources exist in `shaders/vulkan/src/`;
- generated SPIR-V lands in build tree;
- Windows multi-config path is explicit;
- missing compiler/artifacts map through renderer config/result policy;
- runtime/content/projection/save do not reference shader tooling.

Blocked later work:

- Slang adoption;
- texture sampling;
- material descriptors;
- shader hot reload;
- checked-in generated SPIR-V unless packaging requires it.

## Packet 10: First-Room Pipeline

Purpose: plan first graphics pipeline creation for the first visible room using the shader build output.

Likely file plans:

```text
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/VulkanBackend.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/unit/render_shader_interface_tests.cpp
```

Owns:

- shader module loading;
- first pipeline layout;
- push constant range;
- vertex input state;
- dynamic rendering color/depth format coupling;
- first-room pipeline object;
- bootstrap vertex/index/depth resources needed for first room only;
- first draw count diagnostics.

Must never own:

- gameplay room truth;
- projection production;
- save/replay truth;
- material system;
- texture asset loading;
- model streaming;
- lighting system.

Include rules:

```text
allowed=Vulkan_headers,Swapchain_format_metadata,shader_artifact_paths,FrameInput_values,RenderDiagnostics
forbidden=runtime_mutation_headers,content_package_validation_headers,save_headers
```

Read first:

- [first_room_render_contract.md](first_room_render_contract.md)
- [shader_interface_contract.md](shader_interface_contract.md)
- [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md)
- [depth_and_coordinates.md](depth_and_coordinates.md)
- [gpu_resource_upload.md](gpu_resource_upload.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)

Validation gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_pipeline|vulkan_first_room|render_replay'
```

Required receipt fields:

```text
pipeline_family=first_room
shader_language=glsl
vertex_shader
fragment_shader
rendering_path
swapchain_format
depth_format
depth_test
cull_mode
front_face
draw_count
first_room_visible
runtime_hash_before
runtime_hash_after
```

Exit criteria:

- pipeline creation is validation-clean;
- first-room draw uses backend-neutral `FrameInput`;
- `draw_count > 0`;
- runtime hash is unchanged;
- no texture/material/model-streaming work is pulled into the first proof;
- failure paths emit stable diagnostics.

Blocked later work:

- asset/model/material growth;
- VMA expansion beyond first required resources unless already accepted;
- descriptor/material policy beyond first push-constant baseline;
- picking;
- lighting;
- multiplayer/replay visual extras.

## Dependency Flow

```text
RendererApi
  -> NullRenderer
  -> RenderDiagnostics
  -> Platform shell
  -> InstanceDeviceSurface
  -> Swapchain
  -> CommandBuffers
  -> FrameSync
  -> Shader build
  -> First-room pipeline
```

Important nuance:

- `RenderDiagnostics` should begin before Vulkan, but Vulkan packets will add fields later.
- `RendererConfig` can be planned with diagnostics, but only visual/smoke apps should parse Vulkan-specific CLI/env settings.
- `CommandBuffers` and `FrameSync` will be implemented close together, but the file plans should stay separate so sync ownership is reviewable.
- shader build can be planned before command buffers exist, but first pipeline should not merge until command/sync smoke is stable.

## Reviewer Checklist

Before accepting a file plan from this index, reviewer should verify:

- file ownership is narrow;
- forbidden ownership is explicit;
- include firewall command exists;
- old repo paths are not build input;
- optional versus strict lane behavior is named where relevant;
- diagnostics fields are named before code writes them;
- runtime hash/replay invariance is protected;
- Linux and Windows native Vulkan proof remains in the platform lane plan, not just macOS/MoltenVK;
- no packet pulls in asset/material/engine systems ahead of the first visible room.

## Acceptance Criteria

This index is complete enough when:

- the first concrete file-plan order is unambiguous;
- each packet names likely files, owners, read-first docs, gates, and blocked scope;
- the order begins with backend-neutral renderer API and null renderer;
- Vulkan object work starts only at `InstanceDeviceSurface`;
- swapchain, commands, sync, shader build, and first pipeline are separate;
- first-room pipeline is the final first-proof packet before asset/material growth;
- no document references legacy renderer paths except through the dedicated legacy reference doc.
