# Runtime3D Per-File Plan Index

Updated: 2026-06-20

Superseded for new implementation by
`engine/research/iggy3d_side_repo_plan.md`. Use this directory only as
historical detail for the earlier in-repo `engine/src/runtime3d/` plan. Fresh
work belongs in the standalone `iggy3d` repo.

This directory is the authoritative per-file construction plan for Runtime3D.
One implementation path maps to one document. Each document states what that
file owns, what it must contain, what it must not contain, and how completion is
verified.

Rules:

- Runtime3D authority lives under `engine/src/runtime3d/`.
- Shared math lives under `engine/src/core/math/`.
- CMake registration has its own documents.
- Each test file has its own document.
- Native integration files have their own documents, even when deferred.
- The older 2D runtime remains reference or temporary adapter input.

Use this index only for navigation. Builder contracts live in the individual
per-file documents.

Fill priority is tracked in `PRIORITY.md`.

## Core Math

- `engine_src_core_math_Vec3_hpp.md`
- `engine_src_core_math_Vec3_cpp.md`
- `engine_src_core_math_Ray3_hpp.md`
- `engine_src_core_math_Ray3_cpp.md`
- `engine_src_core_math_Aabb3_hpp.md`
- `engine_src_core_math_Aabb3_cpp.md`
- `engine_src_core_math_Mat4_hpp.md`
- `engine_src_core_math_Mat4_cpp.md`
- `engine_src_core_math_Transform3_hpp.md`
- `engine_src_core_math_Transform3_cpp.md`

## Runtime3D

- `engine_src_runtime3d_Runtime3DEntityId_hpp.md`
- `engine_src_runtime3d_Runtime3DTransform_hpp.md`
- `engine_src_runtime3d_Runtime3DCollisionVolume_hpp.md`
- `engine_src_runtime3d_Runtime3DInteractionVolume_hpp.md`
- `engine_src_runtime3d_Runtime3DEntityState_hpp.md`
- `engine_src_runtime3d_Runtime3DEntityState_cpp.md`
- `engine_src_runtime3d_Runtime3DWorldState_hpp.md`
- `engine_src_runtime3d_Runtime3DWorldState_cpp.md`
- `engine_src_runtime3d_Runtime3DClock_hpp.md`
- `engine_src_runtime3d_Runtime3DClock_cpp.md`
- `engine_src_runtime3d_Runtime3DCameraState_hpp.md`
- `engine_src_runtime3d_Runtime3DCameraState_cpp.md`
- `engine_src_runtime3d_Runtime3DCameraModePolicy_hpp.md`
- `engine_src_runtime3d_Runtime3DCameraModePolicy_cpp.md`
- `engine_src_runtime3d_Runtime3DCommand_hpp.md`
- `engine_src_runtime3d_Runtime3DCommand_cpp.md`
- `engine_src_runtime3d_Runtime3DCommandAdmission_hpp.md`
- `engine_src_runtime3d_Runtime3DCommandAdmission_cpp.md`
- `engine_src_runtime3d_Runtime3DTargetQuery_hpp.md`
- `engine_src_runtime3d_Runtime3DTargetQuery_cpp.md`
- `engine_src_runtime3d_Runtime3DRayProjection_hpp.md`
- `engine_src_runtime3d_Runtime3DRayProjection_cpp.md`
- `engine_src_runtime3d_Runtime3DSceneProjection_hpp.md`
- `engine_src_runtime3d_Runtime3DSceneProjection_cpp.md`
- `engine_src_runtime3d_Runtime3DSaveEnvelope_hpp.md`
- `engine_src_runtime3d_Runtime3DSaveEnvelope_cpp.md`
- `engine_src_runtime3d_Runtime3DSaveLoad_hpp.md`
- `engine_src_runtime3d_Runtime3DSaveLoad_cpp.md`
- `engine_src_runtime3d_Runtime3DLegacy2DAdapter_hpp.md`
- `engine_src_runtime3d_Runtime3DLegacy2DAdapter_cpp.md`
- `engine_src_runtime3d_Runtime3DSessionState_hpp.md`
- `engine_src_runtime3d_Runtime3DSessionState_cpp.md`
- `engine_src_runtime3d_Runtime3DSession_hpp.md`
- `engine_src_runtime3d_Runtime3DSession_cpp.md`

## CMake

- `engine_cmake_iggy_core_sources_cmake.md`
- `engine_cmake_iggy_runtime3d_sources_cmake.md`
- `engine_cmake_iggy_runtime3d_tests_cmake.md`
- `engine_CMakeLists_txt.md`

## Tests

- `engine_tests_runtime3d_session_state_tests_cpp.md`
- `engine_tests_runtime3d_clock_tests_cpp.md`
- `engine_tests_runtime3d_world_state_tests_cpp.md`
- `engine_tests_runtime3d_camera_mode_policy_tests_cpp.md`
- `engine_tests_runtime3d_command_admission_tests_cpp.md`
- `engine_tests_runtime3d_legacy_2d_adapter_tests_cpp.md`
- `engine_tests_runtime3d_ray_projection_tests_cpp.md`
- `engine_tests_runtime3d_target_query_tests_cpp.md`
- `engine_tests_runtime3d_scene_projection_tests_cpp.md`
- `engine_tests_runtime3d_save_load_tests_cpp.md`
- `engine_tests_runtime3d_acceptance_demo_tests_cpp.md`

## Native Integration

- `engine_apps_native_play_Native3DProductSession_hpp.md`
- `engine_apps_native_play_Native3DProductSession_cpp.md`
- `engine_apps_native_play_IggyNativePlay_cpp.md`
- `engine_apps_native_play_NativeSceneDrawList_hpp.md`
- `engine_apps_native_play_NativeVulkanRenderer_hpp.md`
- `engine_apps_native_play_NativeVulkanRenderer_cpp.md`
- `engine_cmake_iggy_native_play_cmake.md`
