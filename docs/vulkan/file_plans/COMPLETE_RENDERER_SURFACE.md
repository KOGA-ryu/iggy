# Complete Vulkan Renderer Surface

This document lists the planned Vulkan renderer build surface and records files deliberately collapsed or renamed from older broad docs.

Implementation readiness: gated. These are file-plan contracts only.

## Surface Audit Result

```text
allowed_to_implement_code_now=no
implementation_gate=headless_runtime_and_projection_gate
packet_1_2_3_gate=headless_runtime_plus_projection
visible_renderer_acceptance_gate=full_product_proof
planned_file_docs=98
control_docs=3
coverage=packets_1_through_7
surface_status=ready_for_reviewer_audit
```

Gate clarification: the backend-neutral renderer boundary, null renderer, and visual app/platform shell are allowed after headless runtime plus projection are green. `CommandReplay` is not a prerequisite for those packets. Vulkan bootstrap starts later after Packets 1, 2, and 3 are reviewed. Final visible renderer acceptance waits for the full product proof lane.

The planned renderer surface is complete for the first Vulkan build ladder: backend-neutral boundary, null renderer, visual app shell, Vulkan bootstrap, swapchain/render loop/sync, shader/resource path, first visible room proof, package lookup, and smoke evidence.

No planned file in this surface owns gameplay truth, package validation truth, save truth, replay truth, or camera-mode truth. The renderer consumes `FrameInput` and backend-neutral projection data, creates GPU-side resources, emits deterministic diagnostics, and leaves runtime state hashes unchanged.

Boundary audit rules:

- Runtime, content, projection, and save code remain free of Vulkan headers, `Vk*` types, and `VK_*` constants.
- Vulkan handles live under `src/render/vulkan/**` except for SDL surface creation glue and Vulkan smoke files.
- SDL/window headers live only in app/platform shell files and platform smoke files.
- Diagnostics are deterministic key-value text.
- Missing Vulkan capability is a skip in optional lanes before Vulkan work begins and a strict failure in strict lanes.
- Device loss reports diagnostics, tears down renderer-owned objects, and does not mutate runtime truth.

## Included File Plans

- `src/render/RendererApi.hpp` -> [src_render_RendererApi_hpp.md](src_render_RendererApi_hpp.md)
- `src/render/RendererApi.cpp` -> [src_render_RendererApi_cpp.md](src_render_RendererApi_cpp.md)
- `src/render/RenderBackend.hpp` -> [src_render_RenderBackend_hpp.md](src_render_RenderBackend_hpp.md)
- `src/render/FrameInput.hpp` -> [src_render_FrameInput_hpp.md](src_render_FrameInput_hpp.md)
- `src/render/FrameInput.cpp` -> [src_render_FrameInput_cpp.md](src_render_FrameInput_cpp.md)
- `src/render/RenderDiagnostics.hpp` -> [src_render_RenderDiagnostics_hpp.md](src_render_RenderDiagnostics_hpp.md)
- `src/render/RenderDiagnostics.cpp` -> [src_render_RenderDiagnostics_cpp.md](src_render_RenderDiagnostics_cpp.md)
- `src/render/RendererConfig.hpp` -> [src_render_RendererConfig_hpp.md](src_render_RendererConfig_hpp.md)
- `src/render/RendererConfig.cpp` -> [src_render_RendererConfig_cpp.md](src_render_RendererConfig_cpp.md)
- `src/render/null/NullRenderer.hpp` -> [src_render_null_NullRenderer_hpp.md](src_render_null_NullRenderer_hpp.md)
- `src/render/null/NullRenderer.cpp` -> [src_render_null_NullRenderer_cpp.md](src_render_null_NullRenderer_cpp.md)
- `apps/iggy3d_visual_demo/main.cpp` -> [apps_iggy3d_visual_demo_main_cpp.md](apps_iggy3d_visual_demo_main_cpp.md)
- `src/app/platform/SdlWindow.hpp` -> [src_app_platform_SdlWindow_hpp.md](src_app_platform_SdlWindow_hpp.md)
- `src/app/platform/SdlWindow.cpp` -> [src_app_platform_SdlWindow_cpp.md](src_app_platform_SdlWindow_cpp.md)
- `src/app/platform/SdlVulkanSurface.hpp` -> [src_app_platform_SdlVulkanSurface_hpp.md](src_app_platform_SdlVulkanSurface_hpp.md)
- `src/app/platform/SdlVulkanSurface.cpp` -> [src_app_platform_SdlVulkanSurface_cpp.md](src_app_platform_SdlVulkanSurface_cpp.md)
- `src/app/PackageRuntimeLookup.hpp` -> [src_app_PackageRuntimeLookup_hpp.md](src_app_PackageRuntimeLookup_hpp.md)
- `src/app/PackageRuntimeLookup.cpp` -> [src_app_PackageRuntimeLookup_cpp.md](src_app_PackageRuntimeLookup_cpp.md)
- `src/app/platform/ExecutablePath.hpp` -> [src_app_platform_ExecutablePath_hpp.md](src_app_platform_ExecutablePath_hpp.md)
- `src/app/platform/ExecutablePath.cpp` -> [src_app_platform_ExecutablePath_cpp.md](src_app_platform_ExecutablePath_cpp.md)
- `cmake/iggy3d_vulkan_deps.cmake` -> [cmake_iggy3d_vulkan_deps_cmake.md](cmake_iggy3d_vulkan_deps_cmake.md)
- `cmake/iggy3d_shaders.cmake` -> [cmake_iggy3d_shaders_cmake.md](cmake_iggy3d_shaders_cmake.md)
- `cmake/iggy3d_install.cmake` -> [cmake_iggy3d_install_cmake.md](cmake_iggy3d_install_cmake.md)
- `src/render/vulkan/VulkanBackend.hpp` -> [src_render_vulkan_VulkanBackend_hpp.md](src_render_vulkan_VulkanBackend_hpp.md)
- `src/render/vulkan/VulkanBackend.cpp` -> [src_render_vulkan_VulkanBackend_cpp.md](src_render_vulkan_VulkanBackend_cpp.md)
- `src/render/vulkan/VulkanTypes.hpp` -> [src_render_vulkan_VulkanTypes_hpp.md](src_render_vulkan_VulkanTypes_hpp.md)
- `src/render/vulkan/VulkanResult.hpp` -> [src_render_vulkan_VulkanResult_hpp.md](src_render_vulkan_VulkanResult_hpp.md)
- `src/render/vulkan/VulkanResult.cpp` -> [src_render_vulkan_VulkanResult_cpp.md](src_render_vulkan_VulkanResult_cpp.md)
- `src/render/vulkan/VulkanFunctions.hpp` -> [src_render_vulkan_VulkanFunctions_hpp.md](src_render_vulkan_VulkanFunctions_hpp.md)
- `src/render/vulkan/VulkanFunctions.cpp` -> [src_render_vulkan_VulkanFunctions_cpp.md](src_render_vulkan_VulkanFunctions_cpp.md)
- `src/render/vulkan/VulkanFeatureSupport.hpp` -> [src_render_vulkan_VulkanFeatureSupport_hpp.md](src_render_vulkan_VulkanFeatureSupport_hpp.md)
- `src/render/vulkan/VulkanFeatureSupport.cpp` -> [src_render_vulkan_VulkanFeatureSupport_cpp.md](src_render_vulkan_VulkanFeatureSupport_cpp.md)
- `src/render/vulkan/InstanceDeviceSurface.hpp` -> [src_render_vulkan_InstanceDeviceSurface_hpp.md](src_render_vulkan_InstanceDeviceSurface_hpp.md)
- `src/render/vulkan/InstanceDeviceSurface.cpp` -> [src_render_vulkan_InstanceDeviceSurface_cpp.md](src_render_vulkan_InstanceDeviceSurface_cpp.md)
- `src/render/vulkan/DebugValidation.hpp` -> [src_render_vulkan_DebugValidation_hpp.md](src_render_vulkan_DebugValidation_hpp.md)
- `src/render/vulkan/DebugValidation.cpp` -> [src_render_vulkan_DebugValidation_cpp.md](src_render_vulkan_DebugValidation_cpp.md)
- `src/render/vulkan/Swapchain.hpp` -> [src_render_vulkan_Swapchain_hpp.md](src_render_vulkan_Swapchain_hpp.md)
- `src/render/vulkan/Swapchain.cpp` -> [src_render_vulkan_Swapchain_cpp.md](src_render_vulkan_Swapchain_cpp.md)
- `src/render/vulkan/RenderLoop.hpp` -> [src_render_vulkan_RenderLoop_hpp.md](src_render_vulkan_RenderLoop_hpp.md)
- `src/render/vulkan/RenderLoop.cpp` -> [src_render_vulkan_RenderLoop_cpp.md](src_render_vulkan_RenderLoop_cpp.md)
- `src/render/vulkan/CommandRecording.hpp` -> [src_render_vulkan_CommandRecording_hpp.md](src_render_vulkan_CommandRecording_hpp.md)
- `src/render/vulkan/CommandRecording.cpp` -> [src_render_vulkan_CommandRecording_cpp.md](src_render_vulkan_CommandRecording_cpp.md)
- `src/render/vulkan/FrameSync.hpp` -> [src_render_vulkan_FrameSync_hpp.md](src_render_vulkan_FrameSync_hpp.md)
- `src/render/vulkan/FrameSync.cpp` -> [src_render_vulkan_FrameSync_cpp.md](src_render_vulkan_FrameSync_cpp.md)
- `src/render/vulkan/ShaderModule.hpp` -> [src_render_vulkan_ShaderModule_hpp.md](src_render_vulkan_ShaderModule_hpp.md)
- `src/render/vulkan/ShaderModule.cpp` -> [src_render_vulkan_ShaderModule_cpp.md](src_render_vulkan_ShaderModule_cpp.md)
- `src/render/vulkan/PipelineLayout.hpp` -> [src_render_vulkan_PipelineLayout_hpp.md](src_render_vulkan_PipelineLayout_hpp.md)
- `src/render/vulkan/PipelineLayout.cpp` -> [src_render_vulkan_PipelineLayout_cpp.md](src_render_vulkan_PipelineLayout_cpp.md)
- `src/render/vulkan/FirstRoomPipeline.hpp` -> [src_render_vulkan_FirstRoomPipeline_hpp.md](src_render_vulkan_FirstRoomPipeline_hpp.md)
- `src/render/vulkan/FirstRoomPipeline.cpp` -> [src_render_vulkan_FirstRoomPipeline_cpp.md](src_render_vulkan_FirstRoomPipeline_cpp.md)
- `src/render/vulkan/BufferImageResources.hpp` -> [src_render_vulkan_BufferImageResources_hpp.md](src_render_vulkan_BufferImageResources_hpp.md)
- `src/render/vulkan/BufferImageResources.cpp` -> [src_render_vulkan_BufferImageResources_cpp.md](src_render_vulkan_BufferImageResources_cpp.md)
- `src/render/vulkan/VulkanMemoryAllocator.hpp` -> [src_render_vulkan_VulkanMemoryAllocator_hpp.md](src_render_vulkan_VulkanMemoryAllocator_hpp.md)
- `src/render/vulkan/VulkanMemoryAllocator.cpp` -> [src_render_vulkan_VulkanMemoryAllocator_cpp.md](src_render_vulkan_VulkanMemoryAllocator_cpp.md)
- `src/render/vulkan/DescriptorSets.hpp` -> [src_render_vulkan_DescriptorSets_hpp.md](src_render_vulkan_DescriptorSets_hpp.md)
- `src/render/vulkan/DescriptorSets.cpp` -> [src_render_vulkan_DescriptorSets_cpp.md](src_render_vulkan_DescriptorSets_cpp.md)
- `shaders/vulkan/src/first_room.vert.glsl` -> [shaders_vulkan_src_first_room_vert_glsl.md](shaders_vulkan_src_first_room_vert_glsl.md)
- `shaders/vulkan/src/first_room.frag.glsl` -> [shaders_vulkan_src_first_room_frag_glsl.md](shaders_vulkan_src_first_room_frag_glsl.md)
- `shaders/vulkan/src/material_unlit_textured.vert.glsl` -> [shaders_vulkan_src_material_unlit_textured_vert_glsl.md](shaders_vulkan_src_material_unlit_textured_vert_glsl.md)
- `shaders/vulkan/src/material_unlit_textured.frag.glsl` -> [shaders_vulkan_src_material_unlit_textured_frag_glsl.md](shaders_vulkan_src_material_unlit_textured_frag_glsl.md)
- `tests/unit/render_boundary_tests.cpp` -> [tests_unit_render_boundary_tests_cpp.md](tests_unit_render_boundary_tests_cpp.md)
- `tests/unit/render_diagnostics_tests.cpp` -> [tests_unit_render_diagnostics_tests_cpp.md](tests_unit_render_diagnostics_tests_cpp.md)
- `tests/unit/render_config_tests.cpp` -> [tests_unit_render_config_tests_cpp.md](tests_unit_render_config_tests_cpp.md)
- `tests/unit/render_projection_input_tests.cpp` -> [tests_unit_render_projection_input_tests_cpp.md](tests_unit_render_projection_input_tests_cpp.md)
- `tests/unit/render_camera_frame_tests.cpp` -> [tests_unit_render_camera_frame_tests_cpp.md](tests_unit_render_camera_frame_tests_cpp.md)
- `tests/unit/render_null_renderer_tests.cpp` -> [tests_unit_render_null_renderer_tests_cpp.md](tests_unit_render_null_renderer_tests_cpp.md)
- `tests/unit/render_replay_invariance_tests.cpp` -> [tests_unit_render_replay_invariance_tests_cpp.md](tests_unit_render_replay_invariance_tests_cpp.md)
- `tests/unit/render_result_mapping_tests.cpp` -> [tests_unit_render_result_mapping_tests_cpp.md](tests_unit_render_result_mapping_tests_cpp.md)
- `tests/unit/render_reason_code_tests.cpp` -> [tests_unit_render_reason_code_tests_cpp.md](tests_unit_render_reason_code_tests_cpp.md)
- `tests/unit/render_unsupported_device_policy_tests.cpp` -> [tests_unit_render_unsupported_device_policy_tests_cpp.md](tests_unit_render_unsupported_device_policy_tests_cpp.md)
- `tests/unit/render_shader_interface_tests.cpp` -> [tests_unit_render_shader_interface_tests_cpp.md](tests_unit_render_shader_interface_tests_cpp.md)
- `tests/unit/render_vertex_format_tests.cpp` -> [tests_unit_render_vertex_format_tests_cpp.md](tests_unit_render_vertex_format_tests_cpp.md)
- `tests/unit/render_shader_build_policy_tests.cpp` -> [tests_unit_render_shader_build_policy_tests_cpp.md](tests_unit_render_shader_build_policy_tests_cpp.md)
- `tests/unit/render_memory_budget_policy_tests.cpp` -> [tests_unit_render_memory_budget_policy_tests_cpp.md](tests_unit_render_memory_budget_policy_tests_cpp.md)
- `tests/unit/package_runtime_lookup_tests.cpp` -> [tests_unit_package_runtime_lookup_tests_cpp.md](tests_unit_package_runtime_lookup_tests_cpp.md)
- `tests/smoke/vulkan_platform_smoke.cpp` -> [tests_smoke_vulkan_platform_smoke_cpp.md](tests_smoke_vulkan_platform_smoke_cpp.md)
- `tests/smoke/vulkan_device_smoke.cpp` -> [tests_smoke_vulkan_device_smoke_cpp.md](tests_smoke_vulkan_device_smoke_cpp.md)
- `tests/smoke/vulkan_feature_baseline_smoke.cpp` -> [tests_smoke_vulkan_feature_baseline_smoke_cpp.md](tests_smoke_vulkan_feature_baseline_smoke_cpp.md)
- `tests/smoke/vulkan_validation_smoke.cpp` -> [tests_smoke_vulkan_validation_smoke_cpp.md](tests_smoke_vulkan_validation_smoke_cpp.md)
- `tests/smoke/vulkan_swapchain_smoke.cpp` -> [tests_smoke_vulkan_swapchain_smoke_cpp.md](tests_smoke_vulkan_swapchain_smoke_cpp.md)
- `tests/smoke/vulkan_resize_minimize_smoke.cpp` -> [tests_smoke_vulkan_resize_minimize_smoke_cpp.md](tests_smoke_vulkan_resize_minimize_smoke_cpp.md)
- `tests/smoke/vulkan_empty_frame_smoke.cpp` -> [tests_smoke_vulkan_empty_frame_smoke_cpp.md](tests_smoke_vulkan_empty_frame_smoke_cpp.md)
- `tests/smoke/vulkan_sync_smoke.cpp` -> [tests_smoke_vulkan_sync_smoke_cpp.md](tests_smoke_vulkan_sync_smoke_cpp.md)
- `tests/smoke/vulkan_pipeline_smoke.cpp` -> [tests_smoke_vulkan_pipeline_smoke_cpp.md](tests_smoke_vulkan_pipeline_smoke_cpp.md)
- `tests/smoke/vulkan_memory_smoke.cpp` -> [tests_smoke_vulkan_memory_smoke_cpp.md](tests_smoke_vulkan_memory_smoke_cpp.md)
- `tests/smoke/vulkan_descriptor_smoke.cpp` -> [tests_smoke_vulkan_descriptor_smoke_cpp.md](tests_smoke_vulkan_descriptor_smoke_cpp.md)
- `tests/smoke/vulkan_material_smoke.cpp` -> [tests_smoke_vulkan_material_smoke_cpp.md](tests_smoke_vulkan_material_smoke_cpp.md)
- `tests/smoke/vulkan_first_room_smoke.cpp` -> [tests_smoke_vulkan_first_room_smoke_cpp.md](tests_smoke_vulkan_first_room_smoke_cpp.md)
- `tests/smoke/vulkan_screenshot_smoke.cpp` -> [tests_smoke_vulkan_screenshot_smoke_cpp.md](tests_smoke_vulkan_screenshot_smoke_cpp.md)
- `tests/smoke/vulkan_frame_hash_smoke.cpp` -> [tests_smoke_vulkan_frame_hash_smoke_cpp.md](tests_smoke_vulkan_frame_hash_smoke_cpp.md)
- `tests/smoke/vulkan_diagnostics_smoke.cpp` -> [tests_smoke_vulkan_diagnostics_smoke_cpp.md](tests_smoke_vulkan_diagnostics_smoke_cpp.md)
- `tests/smoke/vulkan_device_lost_smoke.cpp` -> [tests_smoke_vulkan_device_lost_smoke_cpp.md](tests_smoke_vulkan_device_lost_smoke_cpp.md)
- `tests/smoke/vulkan_optional_unsupported_smoke.cpp` -> [tests_smoke_vulkan_optional_unsupported_smoke_cpp.md](tests_smoke_vulkan_optional_unsupported_smoke_cpp.md)
- `tests/smoke/vulkan_strict_unsupported_smoke.cpp` -> [tests_smoke_vulkan_strict_unsupported_smoke_cpp.md](tests_smoke_vulkan_strict_unsupported_smoke_cpp.md)
- `tests/smoke/package_headless_smoke.cpp` -> [tests_smoke_package_headless_smoke_cpp.md](tests_smoke_package_headless_smoke_cpp.md)
- `tests/smoke/package_visual_startup_smoke.cpp` -> [tests_smoke_package_visual_startup_smoke_cpp.md](tests_smoke_package_visual_startup_smoke_cpp.md)
- `tests/smoke/package_shader_lookup_smoke.cpp` -> [tests_smoke_package_shader_lookup_smoke_cpp.md](tests_smoke_package_shader_lookup_smoke_cpp.md)
- `tests/smoke/package_vulkan_dependency_smoke.cpp` -> [tests_smoke_package_vulkan_dependency_smoke_cpp.md](tests_smoke_package_vulkan_dependency_smoke_cpp.md)

## Naming Reconciliations

- `src/render/vulkan/CommandBuffers.hpp/.cpp` from older broad docs is represented by `src/render/vulkan/CommandRecording.hpp/.cpp` in this file-plan set. The contract owns command pool, command buffers, and recording flow under one clearer module name.
- `src/render/vulkan/PipelinesShaders.hpp/.cpp` from older broad docs is split into `ShaderModule`, `PipelineLayout`, and `FirstRoomPipeline` so shader loading, layout compatibility, and first-room pipeline ownership are reviewable separately.
- `src/render/vulkan/BuffersImagesMemory.hpp/.cpp` from older broad docs is represented by `BufferImageResources.hpp/.cpp` plus `VulkanMemoryAllocator.hpp/.cpp`; allocation policy and resource ownership are separate surfaces.
- `src/render/vulkan/VulkanSurfaceProvider.hpp` is not a standalone planned file in this pass. The provider contract lives in `src/app/platform/SdlVulkanSurface.hpp` and `src/render/vulkan/InstanceDeviceSurface.hpp` until a second platform backend exists.

## Global Rules

- Vulkan headers, `Vk*` types, and `VK_*` constants stay under `src/render/vulkan/**`, `src/app/platform/SdlVulkanSurface.*`, and `tests/smoke/vulkan_*`.
- SDL headers stay under app/platform shell files and platform smoke tests.
- Runtime, content, projection, and save files remain graphics-free.
- Diagnostics use deterministic key-value text, not structured object payloads.
- Optional lanes may skip only before required Vulkan work begins; strict lanes fail on missing required graphics capability.
- Software Vulkan is development evidence only and not shipping proof.

## File Count

Planned per-file documents: 98
