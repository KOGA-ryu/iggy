# Runtime3D File Plan Priority

Updated: 2026-06-20

Superseded for new implementation by
`engine/research/iggy3d_side_repo_plan.md`. This ranking applies only if the
older in-repo `engine/src/runtime3d/` plan is revived. Fresh work belongs in
the standalone `iggy3d` repo.

This ranks the one-file-one-document plans by build dependency, semantic risk,
and product-loop importance. Fill documents in this order. Files in the same
tier can be filled in parallel, but later tiers should not drive semantics back
into earlier tiers.

## Tier 0: Map And Build Boundary

Why: these docs define where Runtime3D lives and how builders find the work.
Without them, implementation spreads back into old 2D runtime or CMake gets
patched inconsistently.

How to fill: add exact registration rules, dependency order, labels, and
build/test commands.

1. `INDEX.md`
2. `engine_CMakeLists_txt.md`
3. `engine_cmake_iggy_runtime3d_sources_cmake.md`
4. `engine_cmake_iggy_runtime3d_tests_cmake.md`
5. `engine_cmake_iggy_core_sources_cmake.md`

## Tier 1: Core Value Types

Why: every 3D runtime file needs stable math and id semantics before world,
camera, target, and save contracts can be precise.

How to fill: specify fields, defaults, invariants, operators, edge behavior,
and what is deliberately not owned by math.

6. `engine_src_runtime3d_Runtime3DEntityId_hpp.md`
7. `engine_src_core_math_Vec3_hpp.md`
8. `engine_src_core_math_Vec3_cpp.md`
9. `engine_src_core_math_Transform3_hpp.md`
10. `engine_src_core_math_Transform3_cpp.md`
11. `engine_src_runtime3d_Runtime3DTransform_hpp.md`
12. `engine_src_core_math_Aabb3_hpp.md`
13. `engine_src_core_math_Aabb3_cpp.md`
14. `engine_src_core_math_Ray3_hpp.md`
15. `engine_src_core_math_Ray3_cpp.md`
16. `engine_src_core_math_Mat4_hpp.md`
17. `engine_src_core_math_Mat4_cpp.md`

## Tier 2: Entity And World Authority

Why: runtime3d becomes real only when entity identity, transforms, collision,
interaction volumes, and world ownership are defined.

How to fill: make explicit who owns state, id allocation, lookup, mutation,
targetability, persistence flags, and cost assumptions.

18. `engine_src_runtime3d_Runtime3DCollisionVolume_hpp.md`
19. `engine_src_runtime3d_Runtime3DInteractionVolume_hpp.md`
20. `engine_src_runtime3d_Runtime3DEntityState_hpp.md`
21. `engine_src_runtime3d_Runtime3DEntityState_cpp.md`
22. `engine_src_runtime3d_Runtime3DWorldState_hpp.md`
23. `engine_src_runtime3d_Runtime3DWorldState_cpp.md`
24. `engine_tests_runtime3d_world_state_tests_cpp.md`

## Tier 3: Time And Camera Mode Control

Why: the product decision depends on real-time first/close-third-person play
switching into tactical slow-time view. This must be runtime-owned before input
and renderer integration.

How to fill: define normal, slow, paused, step-requested, realtime camera,
tactical camera, previous-mode restoration, and transient-input clearing rules.

25. `engine_src_runtime3d_Runtime3DClock_hpp.md`
26. `engine_src_runtime3d_Runtime3DClock_cpp.md`
27. `engine_tests_runtime3d_clock_tests_cpp.md`
28. `engine_src_runtime3d_Runtime3DCameraState_hpp.md`
29. `engine_src_runtime3d_Runtime3DCameraState_cpp.md`
30. `engine_src_runtime3d_Runtime3DCameraModePolicy_hpp.md`
31. `engine_src_runtime3d_Runtime3DCameraModePolicy_cpp.md`
32. `engine_tests_runtime3d_camera_mode_policy_tests_cpp.md`

## Tier 4: Session And Command Authority

Why: single-player must already use the command/session model that future
multiplayer can replicate. Runtime3D cannot be durable or tactical without this.

How to fill: specify lifecycle, command record shape, admission status,
rejection reasons, mutation/no-mutation rules, and command-log ownership.

33. `engine_src_runtime3d_Runtime3DCommand_hpp.md`
34. `engine_src_runtime3d_Runtime3DCommand_cpp.md`
35. `engine_src_runtime3d_Runtime3DCommandAdmission_hpp.md`
36. `engine_src_runtime3d_Runtime3DCommandAdmission_cpp.md`
37. `engine_tests_runtime3d_command_admission_tests_cpp.md`
38. `engine_src_runtime3d_Runtime3DSessionState_hpp.md`
39. `engine_src_runtime3d_Runtime3DSessionState_cpp.md`
40. `engine_src_runtime3d_Runtime3DSession_hpp.md`
41. `engine_src_runtime3d_Runtime3DSession_cpp.md`
42. `engine_tests_runtime3d_session_state_tests_cpp.md`

## Tier 5: Targeting And Scene Projection

Why: playable runtime needs discovery, reach-gated interaction, tactical cursor
projection, and renderer-facing output without giving renderer gameplay truth.

How to fill: define target query inputs/results, ray hit rules, targetability,
tie behavior, scene item fields, item ordering, and compute cost.

43. `engine_src_runtime3d_Runtime3DTargetQuery_hpp.md`
44. `engine_src_runtime3d_Runtime3DTargetQuery_cpp.md`
45. `engine_tests_runtime3d_target_query_tests_cpp.md`
46. `engine_src_runtime3d_Runtime3DRayProjection_hpp.md`
47. `engine_src_runtime3d_Runtime3DRayProjection_cpp.md`
48. `engine_tests_runtime3d_ray_projection_tests_cpp.md`
49. `engine_src_runtime3d_Runtime3DSceneProjection_hpp.md`
50. `engine_src_runtime3d_Runtime3DSceneProjection_cpp.md`
51. `engine_tests_runtime3d_scene_projection_tests_cpp.md`

## Tier 6: Save/Load And Legacy Bridge

Why: the demo must be durable, but save/load should not harden too early around
wrong state. The 2D adapter is useful only after runtime3d ownership is clear.

How to fill: define in-memory envelope fields, compatibility checks,
non-mutation on incompatible load, and one-way 2D-to-3D mapping rules.

52. `engine_src_runtime3d_Runtime3DSaveEnvelope_hpp.md`
53. `engine_src_runtime3d_Runtime3DSaveEnvelope_cpp.md`
54. `engine_src_runtime3d_Runtime3DSaveLoad_hpp.md`
55. `engine_src_runtime3d_Runtime3DSaveLoad_cpp.md`
56. `engine_tests_runtime3d_save_load_tests_cpp.md`
57. `engine_src_runtime3d_Runtime3DLegacy2DAdapter_hpp.md`
58. `engine_src_runtime3d_Runtime3DLegacy2DAdapter_cpp.md`
59. `engine_tests_runtime3d_legacy_2d_adapter_tests_cpp.md`

## Tier 7: Acceptance And Native Integration

Why: these prove the product loop and native play experience, but they should
consume runtime3d semantics rather than inventing them.

How to fill: specify exact native responsibilities, input normalization, no-go
surfaces, renderer handoff, scripted demo proof, and acceptance state summary.

60. `engine_tests_runtime3d_acceptance_demo_tests_cpp.md`
61. `engine_apps_native_play_Native3DProductSession_hpp.md`
62. `engine_apps_native_play_Native3DProductSession_cpp.md`
63. `engine_apps_native_play_IggyNativePlay_cpp.md`
64. `engine_apps_native_play_NativeSceneDrawList_hpp.md`
65. `engine_apps_native_play_NativeVulkanRenderer_hpp.md`
66. `engine_apps_native_play_NativeVulkanRenderer_cpp.md`
67. `engine_cmake_iggy_native_play_cmake.md`
