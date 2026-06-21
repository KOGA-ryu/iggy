# Vulkan File Plan Index

This directory is the Vulkan renderer file-plan surface for `iggy3d`.

One planned renderer, app, shader, CMake, or test file maps to one planning document. These are docs-only contracts and do not authorize renderer C++ implementation before the headless runtime and projection gates are green. For this Vulkan lane, that gate permits Packet 1 backend-neutral renderer boundary, Packet 2 null renderer, and Packet 3 visual app/platform shell after headless runtime plus projection are green.

Hard rule: no file plan in this directory may depend on old repo code, old renderer paths, or graphics authority leaking into runtime truth.

## Reviewer Audit Status

```text
allowed_to_implement_code_now=no
implementation_gate=headless_runtime_and_projection_gate
packet_1_2_3_gate=headless_runtime_plus_projection
visible_renderer_acceptance_gate=full_product_proof
renderer_authority=downstream_backend_only
detailed_packet_contracts=packets_1_through_7
file_plan_docs=98
control_docs=3
total_docs_in_directory=101
```

Gate clarification:

- Packet 1 may start after projection passes because `FrameInput` borrows `SceneProjectionResult*` and `DebugProjectionResult*`.
- Packet 2 may start after Packet 1 shape exists; `render_replay_invariance_tests.cpp` proves renderer submission does not mutate runtime hash, state, or replay truth.
- Packet 2 does not require the `CommandReplay` tool to be complete before the renderer boundary or null renderer starts.
- Packet 3 may start after the backend-neutral API and null renderer shape are usable.
- Packet 4 Vulkan bootstrap remains later and depends on Packets 1, 2, and 3.
- Final visible renderer acceptance still waits for the full product proof lane.

Packet coverage:

| Packet | File plans | Review focus |
| --- | ---: | --- |
| 1 | 14 | Backend-neutral renderer boundary, frame input, config, diagnostics, projection and camera input tests. |
| 2 | 4 | Null renderer and deterministic replay invariance. |
| 3 | 12 | Visual app shell, SDL window and Vulkan surface glue, package/runtime lookup, platform smoke. |
| 4 | 19 | Vulkan backend bootstrap, function loading, feature/device support, validation, result policy, device smoke. |
| 5 | 12 | Swapchain, render loop, command recording, frame sync, resize/minimize/empty-frame smoke. |
| 6 | 25 | Shader build, shader modules, pipeline layout, first-room pipeline, VMA wrapper, buffers/images, descriptors, shader/resource tests. |
| 7 | 12 | First visible room proof, screenshot/frame hash, device-loss path, unsupported-device lanes, package smoke. |

Reviewer audit commands:

```sh
find docs/vulkan/file_plans -maxdepth 1 -type f ! -name 'INDEX.md' ! -name 'PRIORITY.md' ! -name 'COMPLETE_RENDERER_SURFACE.md' | sort | wc -l
rg -n '^## Packet [1-7] Detailed Contract' docs/vulkan/file_plans
```

## Control Documents

- `INDEX.md`
- `PRIORITY.md`
- `COMPLETE_RENDERER_SURFACE.md`

## Backend-Neutral Render Boundary

- [src_render_RendererApi_hpp.md](src_render_RendererApi_hpp.md) - `src/render/RendererApi.hpp`
- [src_render_RendererApi_cpp.md](src_render_RendererApi_cpp.md) - `src/render/RendererApi.cpp`
- [src_render_RenderBackend_hpp.md](src_render_RenderBackend_hpp.md) - `src/render/RenderBackend.hpp`
- [src_render_FrameInput_hpp.md](src_render_FrameInput_hpp.md) - `src/render/FrameInput.hpp`
- [src_render_FrameInput_cpp.md](src_render_FrameInput_cpp.md) - `src/render/FrameInput.cpp`
- [src_render_RendererConfig_hpp.md](src_render_RendererConfig_hpp.md) - `src/render/RendererConfig.hpp`
- [src_render_RendererConfig_cpp.md](src_render_RendererConfig_cpp.md) - `src/render/RendererConfig.cpp`

## Diagnostics

- [src_render_RenderDiagnostics_hpp.md](src_render_RenderDiagnostics_hpp.md) - `src/render/RenderDiagnostics.hpp`
- [src_render_RenderDiagnostics_cpp.md](src_render_RenderDiagnostics_cpp.md) - `src/render/RenderDiagnostics.cpp`

## Null Renderer

- [src_render_null_NullRenderer_hpp.md](src_render_null_NullRenderer_hpp.md) - `src/render/null/NullRenderer.hpp`
- [src_render_null_NullRenderer_cpp.md](src_render_null_NullRenderer_cpp.md) - `src/render/null/NullRenderer.cpp`

## Visual App And Platform Shell

- [apps_iggy3d_visual_demo_main_cpp.md](apps_iggy3d_visual_demo_main_cpp.md) - `apps/iggy3d_visual_demo/main.cpp`
- [src_app_platform_SdlWindow_hpp.md](src_app_platform_SdlWindow_hpp.md) - `src/app/platform/SdlWindow.hpp`
- [src_app_platform_SdlWindow_cpp.md](src_app_platform_SdlWindow_cpp.md) - `src/app/platform/SdlWindow.cpp`
- [src_app_PackageRuntimeLookup_hpp.md](src_app_PackageRuntimeLookup_hpp.md) - `src/app/PackageRuntimeLookup.hpp`
- [src_app_PackageRuntimeLookup_cpp.md](src_app_PackageRuntimeLookup_cpp.md) - `src/app/PackageRuntimeLookup.cpp`
- [src_app_platform_ExecutablePath_hpp.md](src_app_platform_ExecutablePath_hpp.md) - `src/app/platform/ExecutablePath.hpp`
- [src_app_platform_ExecutablePath_cpp.md](src_app_platform_ExecutablePath_cpp.md) - `src/app/platform/ExecutablePath.cpp`

## SDL Vulkan Surface Glue

- [src_app_platform_SdlVulkanSurface_hpp.md](src_app_platform_SdlVulkanSurface_hpp.md) - `src/app/platform/SdlVulkanSurface.hpp`
- [src_app_platform_SdlVulkanSurface_cpp.md](src_app_platform_SdlVulkanSurface_cpp.md) - `src/app/platform/SdlVulkanSurface.cpp`

## CMake And Packaging

- [cmake_iggy3d_vulkan_deps_cmake.md](cmake_iggy3d_vulkan_deps_cmake.md) - `cmake/iggy3d_vulkan_deps.cmake`
- [cmake_iggy3d_shaders_cmake.md](cmake_iggy3d_shaders_cmake.md) - `cmake/iggy3d_shaders.cmake`
- [cmake_iggy3d_install_cmake.md](cmake_iggy3d_install_cmake.md) - `cmake/iggy3d_install.cmake`

## Vulkan Backend Files

- [src_render_vulkan_VulkanBackend_hpp.md](src_render_vulkan_VulkanBackend_hpp.md) - `src/render/vulkan/VulkanBackend.hpp`
- [src_render_vulkan_VulkanBackend_cpp.md](src_render_vulkan_VulkanBackend_cpp.md) - `src/render/vulkan/VulkanBackend.cpp`
- [src_render_vulkan_VulkanTypes_hpp.md](src_render_vulkan_VulkanTypes_hpp.md) - `src/render/vulkan/VulkanTypes.hpp`
- [src_render_vulkan_VulkanResult_hpp.md](src_render_vulkan_VulkanResult_hpp.md) - `src/render/vulkan/VulkanResult.hpp`
- [src_render_vulkan_VulkanResult_cpp.md](src_render_vulkan_VulkanResult_cpp.md) - `src/render/vulkan/VulkanResult.cpp`
- [src_render_vulkan_VulkanFunctions_hpp.md](src_render_vulkan_VulkanFunctions_hpp.md) - `src/render/vulkan/VulkanFunctions.hpp`
- [src_render_vulkan_VulkanFunctions_cpp.md](src_render_vulkan_VulkanFunctions_cpp.md) - `src/render/vulkan/VulkanFunctions.cpp`
- [src_render_vulkan_VulkanFeatureSupport_hpp.md](src_render_vulkan_VulkanFeatureSupport_hpp.md) - `src/render/vulkan/VulkanFeatureSupport.hpp`
- [src_render_vulkan_VulkanFeatureSupport_cpp.md](src_render_vulkan_VulkanFeatureSupport_cpp.md) - `src/render/vulkan/VulkanFeatureSupport.cpp`
- [src_render_vulkan_InstanceDeviceSurface_hpp.md](src_render_vulkan_InstanceDeviceSurface_hpp.md) - `src/render/vulkan/InstanceDeviceSurface.hpp`
- [src_render_vulkan_InstanceDeviceSurface_cpp.md](src_render_vulkan_InstanceDeviceSurface_cpp.md) - `src/render/vulkan/InstanceDeviceSurface.cpp`
- [src_render_vulkan_DebugValidation_hpp.md](src_render_vulkan_DebugValidation_hpp.md) - `src/render/vulkan/DebugValidation.hpp`
- [src_render_vulkan_DebugValidation_cpp.md](src_render_vulkan_DebugValidation_cpp.md) - `src/render/vulkan/DebugValidation.cpp`
- [src_render_vulkan_Swapchain_hpp.md](src_render_vulkan_Swapchain_hpp.md) - `src/render/vulkan/Swapchain.hpp`
- [src_render_vulkan_Swapchain_cpp.md](src_render_vulkan_Swapchain_cpp.md) - `src/render/vulkan/Swapchain.cpp`
- [src_render_vulkan_RenderLoop_hpp.md](src_render_vulkan_RenderLoop_hpp.md) - `src/render/vulkan/RenderLoop.hpp`
- [src_render_vulkan_RenderLoop_cpp.md](src_render_vulkan_RenderLoop_cpp.md) - `src/render/vulkan/RenderLoop.cpp`
- [src_render_vulkan_CommandRecording_hpp.md](src_render_vulkan_CommandRecording_hpp.md) - `src/render/vulkan/CommandRecording.hpp`
- [src_render_vulkan_CommandRecording_cpp.md](src_render_vulkan_CommandRecording_cpp.md) - `src/render/vulkan/CommandRecording.cpp`
- [src_render_vulkan_FrameSync_hpp.md](src_render_vulkan_FrameSync_hpp.md) - `src/render/vulkan/FrameSync.hpp`
- [src_render_vulkan_FrameSync_cpp.md](src_render_vulkan_FrameSync_cpp.md) - `src/render/vulkan/FrameSync.cpp`
- [src_render_vulkan_ShaderModule_hpp.md](src_render_vulkan_ShaderModule_hpp.md) - `src/render/vulkan/ShaderModule.hpp`
- [src_render_vulkan_ShaderModule_cpp.md](src_render_vulkan_ShaderModule_cpp.md) - `src/render/vulkan/ShaderModule.cpp`
- [src_render_vulkan_PipelineLayout_hpp.md](src_render_vulkan_PipelineLayout_hpp.md) - `src/render/vulkan/PipelineLayout.hpp`
- [src_render_vulkan_PipelineLayout_cpp.md](src_render_vulkan_PipelineLayout_cpp.md) - `src/render/vulkan/PipelineLayout.cpp`
- [src_render_vulkan_FirstRoomPipeline_hpp.md](src_render_vulkan_FirstRoomPipeline_hpp.md) - `src/render/vulkan/FirstRoomPipeline.hpp`
- [src_render_vulkan_FirstRoomPipeline_cpp.md](src_render_vulkan_FirstRoomPipeline_cpp.md) - `src/render/vulkan/FirstRoomPipeline.cpp`
- [src_render_vulkan_BufferImageResources_hpp.md](src_render_vulkan_BufferImageResources_hpp.md) - `src/render/vulkan/BufferImageResources.hpp`
- [src_render_vulkan_BufferImageResources_cpp.md](src_render_vulkan_BufferImageResources_cpp.md) - `src/render/vulkan/BufferImageResources.cpp`
- [src_render_vulkan_VulkanMemoryAllocator_hpp.md](src_render_vulkan_VulkanMemoryAllocator_hpp.md) - `src/render/vulkan/VulkanMemoryAllocator.hpp`
- [src_render_vulkan_VulkanMemoryAllocator_cpp.md](src_render_vulkan_VulkanMemoryAllocator_cpp.md) - `src/render/vulkan/VulkanMemoryAllocator.cpp`
- [src_render_vulkan_DescriptorSets_hpp.md](src_render_vulkan_DescriptorSets_hpp.md) - `src/render/vulkan/DescriptorSets.hpp`
- [src_render_vulkan_DescriptorSets_cpp.md](src_render_vulkan_DescriptorSets_cpp.md) - `src/render/vulkan/DescriptorSets.cpp`

## Shader Sources

- [shaders_vulkan_src_first_room_vert_glsl.md](shaders_vulkan_src_first_room_vert_glsl.md) - `shaders/vulkan/src/first_room.vert.glsl`
- [shaders_vulkan_src_first_room_frag_glsl.md](shaders_vulkan_src_first_room_frag_glsl.md) - `shaders/vulkan/src/first_room.frag.glsl`
- [shaders_vulkan_src_material_unlit_textured_vert_glsl.md](shaders_vulkan_src_material_unlit_textured_vert_glsl.md) - `shaders/vulkan/src/material_unlit_textured.vert.glsl`
- [shaders_vulkan_src_material_unlit_textured_frag_glsl.md](shaders_vulkan_src_material_unlit_textured_frag_glsl.md) - `shaders/vulkan/src/material_unlit_textured.frag.glsl`

## Backend-Neutral Tests

- [tests_unit_render_boundary_tests_cpp.md](tests_unit_render_boundary_tests_cpp.md) - `tests/unit/render_boundary_tests.cpp`
- [tests_unit_render_diagnostics_tests_cpp.md](tests_unit_render_diagnostics_tests_cpp.md) - `tests/unit/render_diagnostics_tests.cpp`
- [tests_unit_render_config_tests_cpp.md](tests_unit_render_config_tests_cpp.md) - `tests/unit/render_config_tests.cpp`
- [tests_unit_render_projection_input_tests_cpp.md](tests_unit_render_projection_input_tests_cpp.md) - `tests/unit/render_projection_input_tests.cpp`
- [tests_unit_render_camera_frame_tests_cpp.md](tests_unit_render_camera_frame_tests_cpp.md) - `tests/unit/render_camera_frame_tests.cpp`
- [tests_unit_render_null_renderer_tests_cpp.md](tests_unit_render_null_renderer_tests_cpp.md) - `tests/unit/render_null_renderer_tests.cpp`
- [tests_unit_render_replay_invariance_tests_cpp.md](tests_unit_render_replay_invariance_tests_cpp.md) - `tests/unit/render_replay_invariance_tests.cpp`
- [tests_unit_render_result_mapping_tests_cpp.md](tests_unit_render_result_mapping_tests_cpp.md) - `tests/unit/render_result_mapping_tests.cpp`
- [tests_unit_render_reason_code_tests_cpp.md](tests_unit_render_reason_code_tests_cpp.md) - `tests/unit/render_reason_code_tests.cpp`
- [tests_unit_render_unsupported_device_policy_tests_cpp.md](tests_unit_render_unsupported_device_policy_tests_cpp.md) - `tests/unit/render_unsupported_device_policy_tests.cpp`
- [tests_unit_render_shader_interface_tests_cpp.md](tests_unit_render_shader_interface_tests_cpp.md) - `tests/unit/render_shader_interface_tests.cpp`
- [tests_unit_render_vertex_format_tests_cpp.md](tests_unit_render_vertex_format_tests_cpp.md) - `tests/unit/render_vertex_format_tests.cpp`
- [tests_unit_render_shader_build_policy_tests_cpp.md](tests_unit_render_shader_build_policy_tests_cpp.md) - `tests/unit/render_shader_build_policy_tests.cpp`
- [tests_unit_render_memory_budget_policy_tests_cpp.md](tests_unit_render_memory_budget_policy_tests_cpp.md) - `tests/unit/render_memory_budget_policy_tests.cpp`
- [tests_unit_package_runtime_lookup_tests_cpp.md](tests_unit_package_runtime_lookup_tests_cpp.md) - `tests/unit/package_runtime_lookup_tests.cpp`
- [tests_smoke_package_headless_smoke_cpp.md](tests_smoke_package_headless_smoke_cpp.md) - `tests/smoke/package_headless_smoke.cpp`
- [tests_smoke_package_shader_lookup_smoke_cpp.md](tests_smoke_package_shader_lookup_smoke_cpp.md) - `tests/smoke/package_shader_lookup_smoke.cpp`

## Vulkan Smoke Tests

- [tests_smoke_vulkan_platform_smoke_cpp.md](tests_smoke_vulkan_platform_smoke_cpp.md) - `tests/smoke/vulkan_platform_smoke.cpp`
- [tests_smoke_vulkan_device_smoke_cpp.md](tests_smoke_vulkan_device_smoke_cpp.md) - `tests/smoke/vulkan_device_smoke.cpp`
- [tests_smoke_vulkan_feature_baseline_smoke_cpp.md](tests_smoke_vulkan_feature_baseline_smoke_cpp.md) - `tests/smoke/vulkan_feature_baseline_smoke.cpp`
- [tests_smoke_vulkan_validation_smoke_cpp.md](tests_smoke_vulkan_validation_smoke_cpp.md) - `tests/smoke/vulkan_validation_smoke.cpp`
- [tests_smoke_vulkan_swapchain_smoke_cpp.md](tests_smoke_vulkan_swapchain_smoke_cpp.md) - `tests/smoke/vulkan_swapchain_smoke.cpp`
- [tests_smoke_vulkan_resize_minimize_smoke_cpp.md](tests_smoke_vulkan_resize_minimize_smoke_cpp.md) - `tests/smoke/vulkan_resize_minimize_smoke.cpp`
- [tests_smoke_vulkan_empty_frame_smoke_cpp.md](tests_smoke_vulkan_empty_frame_smoke_cpp.md) - `tests/smoke/vulkan_empty_frame_smoke.cpp`
- [tests_smoke_vulkan_sync_smoke_cpp.md](tests_smoke_vulkan_sync_smoke_cpp.md) - `tests/smoke/vulkan_sync_smoke.cpp`
- [tests_smoke_vulkan_pipeline_smoke_cpp.md](tests_smoke_vulkan_pipeline_smoke_cpp.md) - `tests/smoke/vulkan_pipeline_smoke.cpp`
- [tests_smoke_vulkan_memory_smoke_cpp.md](tests_smoke_vulkan_memory_smoke_cpp.md) - `tests/smoke/vulkan_memory_smoke.cpp`
- [tests_smoke_vulkan_descriptor_smoke_cpp.md](tests_smoke_vulkan_descriptor_smoke_cpp.md) - `tests/smoke/vulkan_descriptor_smoke.cpp`
- [tests_smoke_vulkan_material_smoke_cpp.md](tests_smoke_vulkan_material_smoke_cpp.md) - `tests/smoke/vulkan_material_smoke.cpp`
- [tests_smoke_vulkan_first_room_smoke_cpp.md](tests_smoke_vulkan_first_room_smoke_cpp.md) - `tests/smoke/vulkan_first_room_smoke.cpp`
- [tests_smoke_vulkan_screenshot_smoke_cpp.md](tests_smoke_vulkan_screenshot_smoke_cpp.md) - `tests/smoke/vulkan_screenshot_smoke.cpp`
- [tests_smoke_vulkan_frame_hash_smoke_cpp.md](tests_smoke_vulkan_frame_hash_smoke_cpp.md) - `tests/smoke/vulkan_frame_hash_smoke.cpp`
- [tests_smoke_vulkan_diagnostics_smoke_cpp.md](tests_smoke_vulkan_diagnostics_smoke_cpp.md) - `tests/smoke/vulkan_diagnostics_smoke.cpp`
- [tests_smoke_vulkan_device_lost_smoke_cpp.md](tests_smoke_vulkan_device_lost_smoke_cpp.md) - `tests/smoke/vulkan_device_lost_smoke.cpp`
- [tests_smoke_vulkan_optional_unsupported_smoke_cpp.md](tests_smoke_vulkan_optional_unsupported_smoke_cpp.md) - `tests/smoke/vulkan_optional_unsupported_smoke.cpp`
- [tests_smoke_vulkan_strict_unsupported_smoke_cpp.md](tests_smoke_vulkan_strict_unsupported_smoke_cpp.md) - `tests/smoke/vulkan_strict_unsupported_smoke.cpp`
- [tests_smoke_package_visual_startup_smoke_cpp.md](tests_smoke_package_visual_startup_smoke_cpp.md) - `tests/smoke/package_visual_startup_smoke.cpp`
- [tests_smoke_package_vulkan_dependency_smoke_cpp.md](tests_smoke_package_vulkan_dependency_smoke_cpp.md) - `tests/smoke/package_vulkan_dependency_smoke.cpp`

File-plan document count excluding control docs: 98
