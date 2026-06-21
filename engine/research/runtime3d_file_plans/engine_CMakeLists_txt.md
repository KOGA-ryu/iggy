# `engine/CMakeLists.txt`

Purpose: include runtime3d source and test registration in the engine build.

Must contain changes:

- Include `cmake/iggy_runtime3d_sources.cmake`.
- Under `IGGY_BUILD_TESTS`, set `IGGY_TEST_DEFAULT_LABELS runtime3d` and
  include `cmake/iggy_runtime3d_tests.cmake`.

Construction rules:

- Prefer source include order after core sources.
- If dependencies require runtime3d after old runtime or scene, choose the
  compiling order and state why in the builder summary.
- Do not reshuffle unrelated test registration.
- Restore/unset labels consistently with surrounding CMake.

Completion:

- Runtime3D files compile as part of `iggy_engine`.
- Runtime3D tests have `runtime3d` label.

