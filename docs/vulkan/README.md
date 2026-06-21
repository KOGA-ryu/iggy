# Vulkan Documentation

This folder is the Vulkan planning surface for `iggy3d`.

Vulkan is a downstream renderer backend, not runtime authority. Runtime state, command legality, camera truth, package validation, save/load truth, replay determinism, and scene semantics stay outside the Vulkan backend. The renderer consumes backend-neutral frame data and projection output; it does not decide game truth.

The current project priority remains the shippable headless runtime demo. Vulkan work starts only after the headless acceptance loop is proven through the runtime docs and gates:

- [Architecture](../architecture.md)
- [Acceptance demo](../acceptance_demo.md)
- [Roadmap](../roadmap.md)
- [Ownership](../ownership.md)

## Document Order

Read and use these documents in this order:

1. [boundaries.md](boundaries.md) - hard renderer/runtime firewall rules.
2. [manuals.md](manuals.md) - curated source map for Vulkan manuals and tools.
3. [integration_roadmap.md](integration_roadmap.md) - phased integration plan from headless gate through platform validation.
4. [file_surface.md](file_surface.md) - proposed future renderer file and test surface.
5. [legacy_reference.md](legacy_reference.md) - historical notes from the old renderer; reference only, not build input.
6. [file_plans/INDEX.md](file_plans/INDEX.md) - file-by-file Vulkan renderer/app/test/CMake implementation contracts.

Skeleton detail topics for the next pass:

- [decisions.md](decisions.md) - unresolved renderer decisions and evidence needed.
- [platform_matrix.md](platform_matrix.md) - macOS, Linux, and Windows validation lanes.
- [platform_shell.md](platform_shell.md) - window, event, and surface ownership.
- [frame_input_contract.md](frame_input_contract.md) - exact renderer input contract still to define.
- [shader_pipeline.md](shader_pipeline.md) - shader language, compilation, and SPIR-V policy.
- [resource_model.md](resource_model.md) - GPU memory, buffers, images, descriptors, and materials.
- [diagnostics_and_tests.md](diagnostics_and_tests.md) - smoke receipt, labels, skips, and CI behavior.
- [packaging.md](packaging.md) - platform packaging and runtime dependency rules.
- [first_room_render_contract.md](first_room_render_contract.md) - first visible Vulkan room proof.
- [camera_render_contract.md](camera_render_contract.md) - runtime camera truth to renderer camera frame.
- [fallbacks.md](fallbacks.md) - allowed Vulkan fallback rules and required diagnostics.
- [renderer_file_plan_order.md](renderer_file_plan_order.md) - ordered implementation file-plan packets.
- [reviewer_checklist.md](reviewer_checklist.md) - pass/fail review gate for Vulkan packets.
- [renderer_packet_template.md](renderer_packet_template.md) - copyable template for renderer file-plan packets.
- [lifetime.md](lifetime.md) - Vulkan object lifetime, resize, and cleanup order.
- [render_loop.md](render_loop.md) - per-frame acquire, record, submit, present contract.
- [command_recording.md](command_recording.md) - command pool, command buffer, dynamic rendering, and draw recording rules.
- [sync_contract.md](sync_contract.md) - WSI semaphores, fences, image barriers, resize, and upload synchronization.
- [swapchain_contract.md](swapchain_contract.md) - surface capabilities, format, present mode, extent, image views, and recreate policy.
- [device_selection.md](device_selection.md) - physical/logical device, queue families, extensions, features, and portability gates.
- [debug_validation.md](debug_validation.md) - validation layers, debug messenger, sync validation, strict failure, and message routing.
- [depth_and_coordinates.md](depth_and_coordinates.md) - world axes, matrices, Vulkan clip/depth range, winding, culling, and depth policy.
- [gpu_resource_upload.md](gpu_resource_upload.md) - staging buffers, upload commands, fences, device-local buffers/images, and upload diagnostics.
- [descriptor_policy.md](descriptor_policy.md) - descriptor set layouts, pools, updates, camera/material/texture bindings, and runtime firewall.
- [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md) - pipeline naming, variant keys, layout compatibility, recreate rules, and cache policy.
- [render_assets_and_materials.md](render_assets_and_materials.md) - renderer-owned asset/material residency, fallback resources, and backend-neutral asset identity.
- [shader_interface_contract.md](shader_interface_contract.md) - C++/SPIR-V interface contract for vertex formats, push constants, descriptors, matrices, and shader compatibility checks.
- [vulkan_feature_baseline.md](vulkan_feature_baseline.md) - Vulkan API version, required features/extensions, dynamic rendering baseline, deferred features, and unsupported-device gates.
- [vulkan_feature_query_chain.md](vulkan_feature_query_chain.md) - exact Vulkan instance/device extension enumeration, `pNext` feature query, enable chain, portability, and diagnostics policy.
- [vulkan_function_loading.md](vulkan_function_loading.md) - Vulkan loader, `vkGetInstanceProcAddr`, `vkGetDeviceProcAddr`, core/KHR function names, debug utils, and platform loader policy.
- [vulkan_surface_wsi_platforms.md](vulkan_surface_wsi_platforms.md) - Vulkan WSI surface creation, SDL3/GLFW extension queries, macOS Metal surface, Linux X11/Wayland, Windows Win32, and platform receipts.
- [vulkan_swapchain_failure_modes.md](vulkan_swapchain_failure_modes.md) - detailed acquire/present/recreate behavior for out-of-date, suboptimal, minimized, zero extent, device lost, and surface lost cases.
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md) - first-room image layout and barrier policy for swapchain color, depth, staging uploads, sampled textures, and future render targets.
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md) - VMA allocator creation, allocation naming, heap budget receipts, default/custom pool rules, staging policy, defrag deferral, and out-of-memory behavior.
- [vulkan_texture_format_policy.md](vulkan_texture_format_policy.md) - texture format ladder, sRGB versus linear rules, fallback formats, mip policy, sampler compatibility, and macOS/Linux/Windows format quirks.
- [vulkan_shader_build_pipeline.md](vulkan_shader_build_pipeline.md) - exact CMake/glslang shader build policy for compiler discovery, generated SPIR-V paths, stale artifact detection, and Windows multi-config handling.
- [vulkan_debug_labels_and_capture.md](vulkan_debug_labels_and_capture.md) - debug object naming, command/queue labels, GPU marker names, and optional RenderDoc/GFXReconstruct capture policy.
- [vulkan_device_loss_recovery.md](vulkan_device_loss_recovery.md) - `VK_ERROR_DEVICE_LOST` diagnostics, shutdown path, runtime firewall, and restart/recreate policy.
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md) - optional versus strict smoke lanes for macOS, Linux, Windows, software Vulkan, skip/fail exit codes, labels, and receipt fields.
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md) - central `VkResult` mapping for renderer outcomes, recovery classes, reason codes, skip/fail policy, and runtime firewall behavior.
- [vulkan_renderer_config.md](vulkan_renderer_config.md) - exact renderer startup config for backend selection, shader roots, validation, sync validation, present mode, debug labels, diagnostics, strict mode, and runtime firewall rules.
- [vulkan_first_file_plans_index.md](vulkan_first_file_plans_index.md) - bridge from Vulkan docs to the first implementation file-plan packets: renderer API, null renderer, diagnostics, platform shell, device, swapchain, commands, sync, shader build, and first-room pipeline.
- [vulkan_threading_and_frame_ownership.md](vulkan_threading_and_frame_ownership.md) - first Vulkan thread ownership policy: main-thread rendering, `FrameInput` crossing, command pool/queue ownership, and render-thread deferral rules.
- [vulkan_resize_minimize_test_plan.md](vulkan_resize_minimize_test_plan.md) - focused WSI test plan for resize storms, minimize/restore, zero extent, DPI/display-scale changes, display movement, and bounded recreate loops.
- [vulkan_validation_message_policy.md](vulkan_validation_message_policy.md) - validation-message failure policy for strict lanes, message ID/VUID logging, allowlist storage, suppression limits, and anti-papering rules.
- [vulkan_spirv_reflection_policy.md](vulkan_spirv_reflection_policy.md) - SPIR-V reflection decision and shader-interface drift policy for manual first-room metadata, SPIRV-Reflect/SPIRV-Cross choices, SPIRV-Tools validation, and descriptor/material gates.
- [vulkan_screenshot_and_frame_hash.md](vulkan_screenshot_and_frame_hash.md) - first-room visual proof policy for screenshot artifacts, Vulkan readback, normalized frame hashes, tolerance thresholds, software/hardware baselines, and visible-room criteria.
- [vulkan_package_runtime_lookup.md](vulkan_package_runtime_lookup.md) - installed visual package lookup policy for shader roots, resource roots, SDL3/MoltenVK runtime dependencies, diagnostics output paths, and macOS/Linux/Windows layout differences.
- [vulkan_unsupported_device_policy.md](vulkan_unsupported_device_policy.md) - unsupported-device policy for machines with Vulkan present but missing required API versions, features, extensions, queues, formats, or portability gates, including optional skip, strict fail, and user-facing messages.
- [file_plans/INDEX.md](file_plans/INDEX.md) - Vulkan file-plan index: one planned renderer/app/shader/CMake/test file per detailed builder-facing plan.
- [file_plans/PRIORITY.md](file_plans/PRIORITY.md) - Vulkan file-plan priority order from backend-neutral boundary through first-room visual proof.
- [file_plans/COMPLETE_RENDERER_SURFACE.md](file_plans/COMPLETE_RENDERER_SURFACE.md) - complete planned Vulkan renderer surface and naming reconciliation.

## First Builder Entry Point

Start at [integration_roadmap.md](integration_roadmap.md), Phase 0. Do not create renderer file plans until the headless runtime gate is green and the dependency firewall in [boundaries.md](boundaries.md) can be checked by command.

After Phase 0 passes, the first Vulkan builder packet should use [file_plans/PRIORITY.md](file_plans/PRIORITY.md), Packet 1: create the renderer boundary and null renderer shape without introducing Vulkan headers outside Vulkan-owned files.

## Open Baseline Decision

The preferred path for new `iggy3d` work is modern Vulkan: dynamic rendering, synchronization validation, and a clear shader compilation policy. The exact baseline remains gated by Phase 3 and Phase 6 diagnostics across macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan.

The old render-pass-style renderer is historical reference only. It should not force the standalone repo onto an older baseline unless platform evidence shows that the modern path is not viable.
