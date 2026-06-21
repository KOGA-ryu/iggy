# src/app/PackageRuntimeLookup.hpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/app/PackageRuntimeLookup.hpp`

Purpose: Declare deterministic visual package lookup for executable path, resource root, shader root, runtime dependencies, and diagnostics output.

## Build Position

Packet order: 3 - Visual app and SDL platform shell
Owner module: `package runtime lookup`
File kind: `header`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/boundaries.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/diagnostics_and_tests.md`
- `docs/vulkan/vulkan_package_runtime_lookup.md`
- `docs/vulkan/packaging.md`

## Ownership

This file owns:
- Declare deterministic visual package lookup for executable path, resource root, shader root, runtime dependencies, and diagnostics output.
- the public or private names listed in the file shape section;
- diagnostics fields directly tied to its responsibility.

This file must never own:
- gameplay truth;
- command legality;
- save/load truth;
- replay or deterministic state-hash truth;
- content package validation truth;
- renderer fallback without diagnostics;
- legacy renderer linkage.

Runtime firewall boundaries:
- runtime, content, projection, and save code do not depend on this file unless it is a backend-neutral render contract explicitly consumed by an app layer;
- this file cannot mutate runtime state directly;
- renderer output cannot affect replay results.

## Required Include Policy

Allowed includes: standard library, app config helpers, diagnostics, SDL3 headers for SDL window files, and platform OS path helpers.

Forbidden includes: Vulkan headers except through `SdlVulkanSurface.*`, runtime mutation internals, content validators, save/load internals, and old repo headers.

Vulkan headers are forbidden in this file unless the exact path is `SdlVulkanSurface.*`.

SDL headers are allowed only in SDL platform implementation files and not in runtime/content/projection/save.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `PackageRuntimeLookup`
- `PackageLookupConfig`
- `resolvePackageRuntimeLookup`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

App/platform code owns window and path lookup objects. The renderer owns GPU resources. Runtime owns gameplay state. Platform events may be translated into app-level requests, but they do not mutate runtime state directly. Diagnostics receipts are emitted by app or render diagnostics helpers.

## Semantics

Normal path: resolve platform paths, window state, events, or diagnostics roots without touching runtime truth. Skip/fail: visual-only work may skip in optional graphics lanes and must fail in strict package lanes when required paths are missing.

Platform behavior:
macOS/MoltenVK: report `platform=macos` and `platform_lane=moltenvk` when Vulkan is attempted; MoltenVK portability details are diagnostics, not cross-platform law.
Linux: report `platform=linux` and `platform_lane=native_vulkan` for hardware/native validation; software Vulkan uses a separate lane.
Windows: report `platform=windows` and `platform_lane=native_vulkan`; multi-config shader/package paths must include the active config where relevant.
Software Vulkan: allowed for optional development evidence only; it cannot replace native macOS/Linux/Windows proof.
Strict lane: required gates fail with `result=fail`.
Optional lane: unsupported environment or missing optional Vulkan prerequisites may skip with `result=skip` before unsafe renderer work begins.

## Diagnostics And Result Policy

Stable reason names must use lowercase snake-case text. Receipts use deterministic key-value lines.

Required receipt fields:
```text
receipt_version=1
repo=iggy3d
file_plan=src/app/PackageRuntimeLookup.hpp
packet_order=3
allowed_to_implement_code_now=false
window_shell=sdl3|none|unavailable
surface_provider=sdl3|none|unavailable
drawable=true|false
platform_required_instance_extensions=
resource_root=
shader_root=
diagnostics_dir=
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: platform lookup may use documented path candidates. Runtime/content/projection behavior never changes because a graphics path is missing.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=package_runtime_lookup
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: one SDL/window/path lookup setup and platform extension query.
Per-frame cost: event polling and drawable-size query only.
Resize cost: event bookkeeping; Vulkan swapchain recreate is owned elsewhere.
GPU memory cost: none in the platform shell.

## Tests And Verification

Unit tests:
- `tests/unit/package_runtime_lookup_tests.cpp`

Smoke tests:
- `tests/smoke/package_shader_lookup_smoke.cpp`

CTest labels:
```text
iggy3d;render
```

Expected pass behavior: required receipt fields are present and the file owns only the declared responsibility.
Expected skip behavior: optional Vulkan lanes may skip only before required Vulkan work begins and must print `result=skip` plus `reason_code`.
Expected fail behavior: strict lanes fail on missing required dependency, validation error, runtime mutation, or boundary leak.

Firewall scan:
```sh
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected firewall scan result:
```text
no matches
```

## Packet 3 Detailed Contract

Current-build API:
```cpp
namespace iggy3d {

enum class PackageMode {
    Headless,
    BuildTreeVisual,
    InstalledVisual
};

struct PackageLookupConfig {
    PackageMode packageMode;
    std::filesystem::path executablePathOverride;
    std::filesystem::path resourceRootOverride;
    std::filesystem::path shaderRootOverride;
    std::filesystem::path diagnosticsDirOverride;
    bool requireShaderRoot;
    bool requireGraphicsRuntime;
};

struct PackageRuntimeLookup {
    PackageMode packageMode;
    std::filesystem::path executablePath;
    std::filesystem::path executableDir;
    std::filesystem::path packageRoot;
    std::filesystem::path resourceRoot;
    std::filesystem::path shaderRoot;
    std::filesystem::path diagnosticsDir;
    bool resourceRootExists;
    bool shaderRootExists;
    bool diagnosticsDirWritable;
    std::string resourceRootSource;
    std::string shaderRootSource;
    std::string diagnosticsDirSource;
};

struct PackageLookupResult {
    PackageRuntimeLookup lookup;
    RenderOutcome outcome;
    RenderReason reason;
};

PackageLookupResult resolvePackageRuntimeLookup(const PackageLookupConfig& config);

}
```

Include policy:
- Header includes are limited to standard library filesystem/string types and render result vocabulary.
- No Vulkan, SDL, platform OS, runtime, content, projection, or save headers are allowed here.
- Visual app code may depend on this header.
- Runtime code must not depend on this header.

Ownership:
- Owns path-resolution policy for visual packages, shader roots, resource roots, and diagnostics output.
- Does not own package loading, shader compilation, renderer creation, runtime assets, saves, or replay files.
- Does not create resource or shader directories.
- Diagnostics directory creation is a source-file responsibility, not header behavior.

Resolution contract:
- `Headless` mode may leave shader and graphics runtime paths empty.
- `BuildTreeVisual` mode resolves build-tree resources and generated shader outputs.
- `InstalledVisual` mode resolves executable-relative or platform package resources.
- The current working directory is never an accepted source.

Reason codes:
- `package_lookup_ok`
- `package_lookup_executable_missing`
- `package_lookup_resource_root_missing`
- `package_lookup_shader_root_missing`
- `package_lookup_diagnostics_dir_unwritable`
- `package_lookup_cwd_forbidden`

Receipt fields:
```text
package_mode=<headless|build_tree_visual|installed_visual>
executable_dir=<absolute-path>
resource_root=<absolute-path-or-empty>
resource_root_source=<override|environment|executable_relative|bundle_resource|install_prefix|build_tree|none>
shader_root=<absolute-path-or-empty>
shader_root_source=<override|environment|executable_relative|resource_root|build_tree|none>
diagnostics_dir=<absolute-path-or-empty>
diagnostics_dir_source=<override|environment|build_artifacts|platform_pref|temp>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Verification:
- `package_runtime_lookup_tests` prove every lookup source, missing-required failure, diagnostics writability, and no-current-directory behavior.
- `package_shader_lookup_smoke` proves the visual package can find shader artifacts after shader build packets land.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not rely on current working directory for installed package lookup.
- Do not let SDL event polling mutate runtime state directly.

## Completion Criteria

- File `src/app/PackageRuntimeLookup.hpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
