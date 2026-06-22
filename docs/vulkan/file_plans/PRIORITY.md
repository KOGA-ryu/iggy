# Vulkan File Plan Priority

This file ranks Vulkan file-plan documents in build order. It is a planning order and historical provenance record; implementation remains gated by headless runtime and projection acceptance.

## Priority Audit State

```text
allowed_to_implement_code_now=no
implementation_gate=headless_runtime_and_projection_gate
packet_1_2_3_gate=headless_runtime_plus_projection
visible_renderer_acceptance_gate=full_product_proof
packet_order=1_then_2_then_3_then_4_then_5_then_6_then_7
file_plan_docs=98
control_docs=3
```

The first implementation packet after the Vulkan-lane gate is Packet 1, not the Vulkan backend. The Vulkan-lane gate for Packets 1, 2, and 3 is headless runtime plus projection. It is not blocked by the `CommandReplay` tool or full Phase 8 product proof. Final visible renderer acceptance still waits for full product proof.

Packet 4 work depends on the backend-neutral API, null renderer invariance, and platform shell contracts being reviewed first.

`render_replay_invariance_tests.cpp` means renderer submission must not mutate runtime hash, state, command results, or replay truth. It does not mean the replay tool or `CommandReplay` must be complete before Packet 1 or Packet 2.

Current baseline note: `iggy3d-main` is already beyond the old Packet 1 boundary-only stage. Current source includes the backend-neutral renderer API, concrete `NullRenderer`, factory wiring for `RendererBackendKind::Null`, Vulkan-private implementation files and smokes, first-room proof surfaces, and Runtime Packet 8 tactical combat runtime state. Builders reconciling current source must preserve that baseline instead of treating Packet 1/2/3 entries below as future work.

Every per-file plan in this ladder must carry:

- exact file path and purpose;
- packet position and owner module;
- ownership and firewall rules;
- include policy;
- public API or file shape;
- data lifetime and shutdown behavior;
- normal, skip, fail, strict-lane, and platform behavior;
- deterministic diagnostics and reason codes;
- fallback policy;
- compute-cost expectations;
- test and smoke evidence;
- builder traps;
- completion criteria.

Packet counts:

| Packet | Count | Scope |
| --- | ---: | --- |
| 1 | 14 | Backend-neutral boundary. |
| 2 | 4 | Null renderer and replay invariance. |
| 3 | 12 | Visual app and platform shell. |
| 4 | 19 | Vulkan bootstrap, device, validation, diagnostics. |
| 5 | 12 | Swapchain, render loop, commands, sync. |
| 6 | 25 | Shaders, pipeline, resources, descriptors. |
| 7 | 12 | First visible room proof and packaging smoke. |

## Packet 1: Backend-neutral renderer boundary

1. [src_render_RendererApi_hpp.md](src_render_RendererApi_hpp.md) - `src/render/RendererApi.hpp`
2. [src_render_RendererApi_cpp.md](src_render_RendererApi_cpp.md) - `src/render/RendererApi.cpp`
3. [src_render_RenderBackend_hpp.md](src_render_RenderBackend_hpp.md) - `src/render/RenderBackend.hpp`
4. [src_render_FrameInput_hpp.md](src_render_FrameInput_hpp.md) - `src/render/FrameInput.hpp`
5. [src_render_FrameInput_cpp.md](src_render_FrameInput_cpp.md) - `src/render/FrameInput.cpp`
6. [src_render_RenderDiagnostics_hpp.md](src_render_RenderDiagnostics_hpp.md) - `src/render/RenderDiagnostics.hpp`
7. [src_render_RenderDiagnostics_cpp.md](src_render_RenderDiagnostics_cpp.md) - `src/render/RenderDiagnostics.cpp`
8. [src_render_RendererConfig_hpp.md](src_render_RendererConfig_hpp.md) - `src/render/RendererConfig.hpp`
9. [src_render_RendererConfig_cpp.md](src_render_RendererConfig_cpp.md) - `src/render/RendererConfig.cpp`
61. [tests_unit_render_boundary_tests_cpp.md](tests_unit_render_boundary_tests_cpp.md) - `tests/unit/render_boundary_tests.cpp`
62. [tests_unit_render_diagnostics_tests_cpp.md](tests_unit_render_diagnostics_tests_cpp.md) - `tests/unit/render_diagnostics_tests.cpp`
63. [tests_unit_render_config_tests_cpp.md](tests_unit_render_config_tests_cpp.md) - `tests/unit/render_config_tests.cpp`
64. [tests_unit_render_projection_input_tests_cpp.md](tests_unit_render_projection_input_tests_cpp.md) - `tests/unit/render_projection_input_tests.cpp`
65. [tests_unit_render_camera_frame_tests_cpp.md](tests_unit_render_camera_frame_tests_cpp.md) - `tests/unit/render_camera_frame_tests.cpp`

## Packet 2: Null renderer and invariance

10. [src_render_null_NullRenderer_hpp.md](src_render_null_NullRenderer_hpp.md) - `src/render/null/NullRenderer.hpp`
11. [src_render_null_NullRenderer_cpp.md](src_render_null_NullRenderer_cpp.md) - `src/render/null/NullRenderer.cpp`
66. [tests_unit_render_null_renderer_tests_cpp.md](tests_unit_render_null_renderer_tests_cpp.md) - `tests/unit/render_null_renderer_tests.cpp`
67. [tests_unit_render_replay_invariance_tests_cpp.md](tests_unit_render_replay_invariance_tests_cpp.md) - `tests/unit/render_replay_invariance_tests.cpp`

## Packet 3: Visual app and SDL platform shell

12. [apps_iggy3d_visual_demo_main_cpp.md](apps_iggy3d_visual_demo_main_cpp.md) - `apps/iggy3d_visual_demo/main.cpp`
13. [src_app_platform_SdlWindow_hpp.md](src_app_platform_SdlWindow_hpp.md) - `src/app/platform/SdlWindow.hpp`
14. [src_app_platform_SdlWindow_cpp.md](src_app_platform_SdlWindow_cpp.md) - `src/app/platform/SdlWindow.cpp`
15. [src_app_platform_SdlVulkanSurface_hpp.md](src_app_platform_SdlVulkanSurface_hpp.md) - `src/app/platform/SdlVulkanSurface.hpp`
16. [src_app_platform_SdlVulkanSurface_cpp.md](src_app_platform_SdlVulkanSurface_cpp.md) - `src/app/platform/SdlVulkanSurface.cpp`
17. [src_app_PackageRuntimeLookup_hpp.md](src_app_PackageRuntimeLookup_hpp.md) - `src/app/PackageRuntimeLookup.hpp`
18. [src_app_PackageRuntimeLookup_cpp.md](src_app_PackageRuntimeLookup_cpp.md) - `src/app/PackageRuntimeLookup.cpp`
19. [src_app_platform_ExecutablePath_hpp.md](src_app_platform_ExecutablePath_hpp.md) - `src/app/platform/ExecutablePath.hpp`
20. [src_app_platform_ExecutablePath_cpp.md](src_app_platform_ExecutablePath_cpp.md) - `src/app/platform/ExecutablePath.cpp`
21. [cmake_iggy3d_vulkan_deps_cmake.md](cmake_iggy3d_vulkan_deps_cmake.md) - `cmake/iggy3d_vulkan_deps.cmake`
75. [tests_unit_package_runtime_lookup_tests_cpp.md](tests_unit_package_runtime_lookup_tests_cpp.md) - `tests/unit/package_runtime_lookup_tests.cpp`
76. [tests_smoke_vulkan_platform_smoke_cpp.md](tests_smoke_vulkan_platform_smoke_cpp.md) - `tests/smoke/vulkan_platform_smoke.cpp`

## Packet 4: Vulkan bootstrap, device, validation, diagnostics

24. [src_render_vulkan_VulkanBackend_hpp.md](src_render_vulkan_VulkanBackend_hpp.md) - `src/render/vulkan/VulkanBackend.hpp`
25. [src_render_vulkan_VulkanBackend_cpp.md](src_render_vulkan_VulkanBackend_cpp.md) - `src/render/vulkan/VulkanBackend.cpp`
26. [src_render_vulkan_VulkanTypes_hpp.md](src_render_vulkan_VulkanTypes_hpp.md) - `src/render/vulkan/VulkanTypes.hpp`
27. [src_render_vulkan_VulkanResult_hpp.md](src_render_vulkan_VulkanResult_hpp.md) - `src/render/vulkan/VulkanResult.hpp`
28. [src_render_vulkan_VulkanResult_cpp.md](src_render_vulkan_VulkanResult_cpp.md) - `src/render/vulkan/VulkanResult.cpp`
29. [src_render_vulkan_VulkanFunctions_hpp.md](src_render_vulkan_VulkanFunctions_hpp.md) - `src/render/vulkan/VulkanFunctions.hpp`
30. [src_render_vulkan_VulkanFunctions_cpp.md](src_render_vulkan_VulkanFunctions_cpp.md) - `src/render/vulkan/VulkanFunctions.cpp`
31. [src_render_vulkan_VulkanFeatureSupport_hpp.md](src_render_vulkan_VulkanFeatureSupport_hpp.md) - `src/render/vulkan/VulkanFeatureSupport.hpp`
32. [src_render_vulkan_VulkanFeatureSupport_cpp.md](src_render_vulkan_VulkanFeatureSupport_cpp.md) - `src/render/vulkan/VulkanFeatureSupport.cpp`
33. [src_render_vulkan_InstanceDeviceSurface_hpp.md](src_render_vulkan_InstanceDeviceSurface_hpp.md) - `src/render/vulkan/InstanceDeviceSurface.hpp`
34. [src_render_vulkan_InstanceDeviceSurface_cpp.md](src_render_vulkan_InstanceDeviceSurface_cpp.md) - `src/render/vulkan/InstanceDeviceSurface.cpp`
35. [src_render_vulkan_DebugValidation_hpp.md](src_render_vulkan_DebugValidation_hpp.md) - `src/render/vulkan/DebugValidation.hpp`
36. [src_render_vulkan_DebugValidation_cpp.md](src_render_vulkan_DebugValidation_cpp.md) - `src/render/vulkan/DebugValidation.cpp`
68. [tests_unit_render_result_mapping_tests_cpp.md](tests_unit_render_result_mapping_tests_cpp.md) - `tests/unit/render_result_mapping_tests.cpp`
69. [tests_unit_render_reason_code_tests_cpp.md](tests_unit_render_reason_code_tests_cpp.md) - `tests/unit/render_reason_code_tests.cpp`
70. [tests_unit_render_unsupported_device_policy_tests_cpp.md](tests_unit_render_unsupported_device_policy_tests_cpp.md) - `tests/unit/render_unsupported_device_policy_tests.cpp`
77. [tests_smoke_vulkan_device_smoke_cpp.md](tests_smoke_vulkan_device_smoke_cpp.md) - `tests/smoke/vulkan_device_smoke.cpp`
78. [tests_smoke_vulkan_feature_baseline_smoke_cpp.md](tests_smoke_vulkan_feature_baseline_smoke_cpp.md) - `tests/smoke/vulkan_feature_baseline_smoke.cpp`
79. [tests_smoke_vulkan_validation_smoke_cpp.md](tests_smoke_vulkan_validation_smoke_cpp.md) - `tests/smoke/vulkan_validation_smoke.cpp`

## Packet 5: Swapchain, render loop, command recording, sync

37. [src_render_vulkan_Swapchain_hpp.md](src_render_vulkan_Swapchain_hpp.md) - `src/render/vulkan/Swapchain.hpp`
38. [src_render_vulkan_Swapchain_cpp.md](src_render_vulkan_Swapchain_cpp.md) - `src/render/vulkan/Swapchain.cpp`
39. [src_render_vulkan_RenderLoop_hpp.md](src_render_vulkan_RenderLoop_hpp.md) - `src/render/vulkan/RenderLoop.hpp`
40. [src_render_vulkan_RenderLoop_cpp.md](src_render_vulkan_RenderLoop_cpp.md) - `src/render/vulkan/RenderLoop.cpp`
41. [src_render_vulkan_CommandRecording_hpp.md](src_render_vulkan_CommandRecording_hpp.md) - `src/render/vulkan/CommandRecording.hpp`
42. [src_render_vulkan_CommandRecording_cpp.md](src_render_vulkan_CommandRecording_cpp.md) - `src/render/vulkan/CommandRecording.cpp`
43. [src_render_vulkan_FrameSync_hpp.md](src_render_vulkan_FrameSync_hpp.md) - `src/render/vulkan/FrameSync.hpp`
44. [src_render_vulkan_FrameSync_cpp.md](src_render_vulkan_FrameSync_cpp.md) - `src/render/vulkan/FrameSync.cpp`
80. [tests_smoke_vulkan_swapchain_smoke_cpp.md](tests_smoke_vulkan_swapchain_smoke_cpp.md) - `tests/smoke/vulkan_swapchain_smoke.cpp`
81. [tests_smoke_vulkan_resize_minimize_smoke_cpp.md](tests_smoke_vulkan_resize_minimize_smoke_cpp.md) - `tests/smoke/vulkan_resize_minimize_smoke.cpp`
82. [tests_smoke_vulkan_empty_frame_smoke_cpp.md](tests_smoke_vulkan_empty_frame_smoke_cpp.md) - `tests/smoke/vulkan_empty_frame_smoke.cpp`
83. [tests_smoke_vulkan_sync_smoke_cpp.md](tests_smoke_vulkan_sync_smoke_cpp.md) - `tests/smoke/vulkan_sync_smoke.cpp`

## Packet 6: Shaders, pipeline, resources, descriptors

22. [cmake_iggy3d_shaders_cmake.md](cmake_iggy3d_shaders_cmake.md) - `cmake/iggy3d_shaders.cmake`
45. [src_render_vulkan_ShaderModule_hpp.md](src_render_vulkan_ShaderModule_hpp.md) - `src/render/vulkan/ShaderModule.hpp`
46. [src_render_vulkan_ShaderModule_cpp.md](src_render_vulkan_ShaderModule_cpp.md) - `src/render/vulkan/ShaderModule.cpp`
47. [src_render_vulkan_PipelineLayout_hpp.md](src_render_vulkan_PipelineLayout_hpp.md) - `src/render/vulkan/PipelineLayout.hpp`
48. [src_render_vulkan_PipelineLayout_cpp.md](src_render_vulkan_PipelineLayout_cpp.md) - `src/render/vulkan/PipelineLayout.cpp`
49. [src_render_vulkan_FirstRoomPipeline_hpp.md](src_render_vulkan_FirstRoomPipeline_hpp.md) - `src/render/vulkan/FirstRoomPipeline.hpp`
50. [src_render_vulkan_FirstRoomPipeline_cpp.md](src_render_vulkan_FirstRoomPipeline_cpp.md) - `src/render/vulkan/FirstRoomPipeline.cpp`
51. [src_render_vulkan_BufferImageResources_hpp.md](src_render_vulkan_BufferImageResources_hpp.md) - `src/render/vulkan/BufferImageResources.hpp`
52. [src_render_vulkan_BufferImageResources_cpp.md](src_render_vulkan_BufferImageResources_cpp.md) - `src/render/vulkan/BufferImageResources.cpp`
53. [src_render_vulkan_VulkanMemoryAllocator_hpp.md](src_render_vulkan_VulkanMemoryAllocator_hpp.md) - `src/render/vulkan/VulkanMemoryAllocator.hpp`
54. [src_render_vulkan_VulkanMemoryAllocator_cpp.md](src_render_vulkan_VulkanMemoryAllocator_cpp.md) - `src/render/vulkan/VulkanMemoryAllocator.cpp`
55. [src_render_vulkan_DescriptorSets_hpp.md](src_render_vulkan_DescriptorSets_hpp.md) - `src/render/vulkan/DescriptorSets.hpp`
56. [src_render_vulkan_DescriptorSets_cpp.md](src_render_vulkan_DescriptorSets_cpp.md) - `src/render/vulkan/DescriptorSets.cpp`
57. [shaders_vulkan_src_first_room_vert_glsl.md](shaders_vulkan_src_first_room_vert_glsl.md) - `shaders/vulkan/src/first_room.vert.glsl`
58. [shaders_vulkan_src_first_room_frag_glsl.md](shaders_vulkan_src_first_room_frag_glsl.md) - `shaders/vulkan/src/first_room.frag.glsl`
59. [shaders_vulkan_src_material_unlit_textured_vert_glsl.md](shaders_vulkan_src_material_unlit_textured_vert_glsl.md) - `shaders/vulkan/src/material_unlit_textured.vert.glsl`
60. [shaders_vulkan_src_material_unlit_textured_frag_glsl.md](shaders_vulkan_src_material_unlit_textured_frag_glsl.md) - `shaders/vulkan/src/material_unlit_textured.frag.glsl`
71. [tests_unit_render_shader_interface_tests_cpp.md](tests_unit_render_shader_interface_tests_cpp.md) - `tests/unit/render_shader_interface_tests.cpp`
72. [tests_unit_render_vertex_format_tests_cpp.md](tests_unit_render_vertex_format_tests_cpp.md) - `tests/unit/render_vertex_format_tests.cpp`
73. [tests_unit_render_shader_build_policy_tests_cpp.md](tests_unit_render_shader_build_policy_tests_cpp.md) - `tests/unit/render_shader_build_policy_tests.cpp`
74. [tests_unit_render_memory_budget_policy_tests_cpp.md](tests_unit_render_memory_budget_policy_tests_cpp.md) - `tests/unit/render_memory_budget_policy_tests.cpp`
84. [tests_smoke_vulkan_pipeline_smoke_cpp.md](tests_smoke_vulkan_pipeline_smoke_cpp.md) - `tests/smoke/vulkan_pipeline_smoke.cpp`
85. [tests_smoke_vulkan_memory_smoke_cpp.md](tests_smoke_vulkan_memory_smoke_cpp.md) - `tests/smoke/vulkan_memory_smoke.cpp`
86. [tests_smoke_vulkan_descriptor_smoke_cpp.md](tests_smoke_vulkan_descriptor_smoke_cpp.md) - `tests/smoke/vulkan_descriptor_smoke.cpp`
87. [tests_smoke_vulkan_material_smoke_cpp.md](tests_smoke_vulkan_material_smoke_cpp.md) - `tests/smoke/vulkan_material_smoke.cpp`

## Packet 7: First visible room proof and packaging smoke

23. [cmake_iggy3d_install_cmake.md](cmake_iggy3d_install_cmake.md) - `cmake/iggy3d_install.cmake`
88. [tests_smoke_vulkan_first_room_smoke_cpp.md](tests_smoke_vulkan_first_room_smoke_cpp.md) - `tests/smoke/vulkan_first_room_smoke.cpp`
89. [tests_smoke_vulkan_screenshot_smoke_cpp.md](tests_smoke_vulkan_screenshot_smoke_cpp.md) - `tests/smoke/vulkan_screenshot_smoke.cpp`
90. [tests_smoke_vulkan_frame_hash_smoke_cpp.md](tests_smoke_vulkan_frame_hash_smoke_cpp.md) - `tests/smoke/vulkan_frame_hash_smoke.cpp`
91. [tests_smoke_vulkan_diagnostics_smoke_cpp.md](tests_smoke_vulkan_diagnostics_smoke_cpp.md) - `tests/smoke/vulkan_diagnostics_smoke.cpp`
92. [tests_smoke_vulkan_device_lost_smoke_cpp.md](tests_smoke_vulkan_device_lost_smoke_cpp.md) - `tests/smoke/vulkan_device_lost_smoke.cpp`
93. [tests_smoke_vulkan_optional_unsupported_smoke_cpp.md](tests_smoke_vulkan_optional_unsupported_smoke_cpp.md) - `tests/smoke/vulkan_optional_unsupported_smoke.cpp`
94. [tests_smoke_vulkan_strict_unsupported_smoke_cpp.md](tests_smoke_vulkan_strict_unsupported_smoke_cpp.md) - `tests/smoke/vulkan_strict_unsupported_smoke.cpp`
95. [tests_smoke_package_headless_smoke_cpp.md](tests_smoke_package_headless_smoke_cpp.md) - `tests/smoke/package_headless_smoke.cpp`
96. [tests_smoke_package_visual_startup_smoke_cpp.md](tests_smoke_package_visual_startup_smoke_cpp.md) - `tests/smoke/package_visual_startup_smoke.cpp`
97. [tests_smoke_package_shader_lookup_smoke_cpp.md](tests_smoke_package_shader_lookup_smoke_cpp.md) - `tests/smoke/package_shader_lookup_smoke.cpp`
98. [tests_smoke_package_vulkan_dependency_smoke_cpp.md](tests_smoke_package_vulkan_dependency_smoke_cpp.md) - `tests/smoke/package_vulkan_dependency_smoke.cpp`

## Gate Reminder

Before implementation packets start, run:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected firewall result:

```text
no matches
```
