# File Spec

Files: `src/app/platform/ExecutablePath.hpp`, `src/app/platform/ExecutablePath.cpp`

Verified at: `a7c5f1db`

## Owns

- Cross-platform executable path resolution.
- Canonical executable path and executable directory result packet.
- Platform-specific failure reason mapping for unavailable, buffer-too-small, and canonicalization failures.

## Does Not Own

- Package asset lookup policy.
- Shader, content, save, or runtime path selection.
- App startup options, renderer diagnostics aggregation, or receipt emission.

## Reads

- Platform executable location APIs for Apple, Linux, and Windows.
- Filesystem canonicalization results.

## Writes / Mutates

- Returns `ExecutablePathResult`.
- Does not mutate app state, environment variables, filesystem contents, or renderer state.

## Calls Out To / Wires Out To

- Uses `_NSGetExecutablePath`, `/proc/self/exe`, or `GetModuleFileNameW` depending on platform.
- Uses `std::filesystem::absolute` and `std::filesystem::weakly_canonical`.
- `PackageRuntimeLookup.cpp` consumes `resolveExecutablePath()` when no executable override is provided.

## Called By / Entry Points

- `resolveExecutablePath()`.
- Package runtime lookup calls it to locate installed app-relative assets.
- Focused proof: `rg -n "ExecutablePathResult|resolveExecutablePath|executable_path_" src tests`.

## Invariants

- Success requires a non-empty canonical absolute executable path and parent directory.
- Failure returns `resolved=false` and an explicit `RenderReason`.
- Platform-specific buffers must not leak partially resolved paths on failure.
- This file reports executable location only; package search policy belongs in package runtime lookup.

## Tests / Proof Commands

- `rg -n "resolveExecutablePath|executablePathOverride" src/app tests/unit/package_runtime_lookup_tests.cpp tests/smoke/package_shader_lookup_smoke.cpp`.
- `rg -n "package_runtime_lookup_tests|package_shader_lookup_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/PackageRuntimeLookup.*` unless executable path consumption or package root search changes.
- `src/app/platform/SdlWindow.*` unless platform window creation changes.
- `src/render/*` unless render diagnostics contracts change.

## Update When

- Platform executable resolution, result fields, canonicalization policy, or reason codes change.

## Do Not Update When

- Only package root ranking, shader lookup, or asset discovery changes without changing executable path resolution.
