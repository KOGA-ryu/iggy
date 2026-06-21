# `engine/cmake/iggy_runtime3d_tests.cmake`

Purpose: register runtime3d tests.

Must contain:

- `iggy_add_test` entries for runtime3d tests.
- Initial skeleton tests:
  - `runtime3d_session_state_tests`
  - `runtime3d_clock_tests`
  - `runtime3d_world_state_tests`
  - `runtime3d_camera_mode_policy_tests`
- Later planned tests:
  - `runtime3d_command_admission_tests`
  - `runtime3d_legacy_2d_adapter_tests`
  - `runtime3d_ray_projection_tests`
  - `runtime3d_target_query_tests`
  - `runtime3d_scene_projection_tests`
  - `runtime3d_save_load_tests`
  - `runtime3d_acceptance_demo_tests`

Construction rules:

- Use existing repo test registration style.
- Do not put runtime3d tests in `iggy_runtime_tests.cmake`.
- Keep fixture compile definitions near tests that use fixtures.

Completion:

- `ctest -R runtime3d_` discovers runtime3d tests.

