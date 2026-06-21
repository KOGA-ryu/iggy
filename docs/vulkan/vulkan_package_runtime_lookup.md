# Vulkan Package Runtime Lookup

This document defines exact runtime lookup order for installed visual packages: shader roots, resource directories, SDL3 runtime, MoltenVK/Vulkan runtime, diagnostics output paths, and platform differences for macOS, Linux, and Windows.

Runtime lookup is app/renderer startup policy. It must not become gameplay truth, package-content truth, save/load truth, replay truth, command legality, camera truth, or projection semantics.

## Purpose

Define one deterministic resolver contract:

```text
lookup_scope=installed_visual_package_and_visual_dev_startup
headless_runtime_uses_graphics_lookup=false
explicit_config_overrides_package_defaults=true
shader_lookup_is_renderer_only=true
resource_lookup_is_app_package_only=true
diagnostics_output_is_artifact_or_user_writable_path=true
runtime_state_mutation_from_lookup=false
platforms=macos_linux_windows
```

This document narrows:

- [packaging.md](packaging.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [vulkan_shader_build_pipeline.md](vulkan_shader_build_pipeline.md)
- [vulkan_surface_wsi_platforms.md](vulkan_surface_wsi_platforms.md)
- [vulkan_function_loading.md](vulkan_function_loading.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [platform_matrix.md](platform_matrix.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| SDL3 `SDL_GetBasePath`: https://wiki.libsdl.org/SDL3/SDL_GetBasePath | app data/resource base path discovery, especially app-bundle behavior |
| SDL3 `SDL_GetPrefPath`: https://wiki.libsdl.org/SDL3/SDL_GetPrefPath | user-writable diagnostics/default preference path |
| SDL3 Vulkan surface docs | relationship between SDL runtime and Vulkan-capable visual app startup |
| Vulkan Guide loader page: https://docs.vulkan.org/guide/latest/loader.html | loader role across Linux, macOS, and Windows |
| Vulkan Loader repository docs: https://github.com/KhronosGroup/Vulkan-Loader | loader/ICD behavior, platform loader expectations |
| LunarG macOS Vulkan SDK guide | Apple SDK, loader, validation layer, and MoltenVK development setup |
| MoltenVK Runtime User Guide | app integration and runtime packaging on Apple platforms |
| Project packaging/config docs | local shader roots, package modes, diagnostics fields, strict/optional behavior |

Priority rule:

```text
filesystem_api_behavior=SDL3_docs
vulkan_loader_behavior=Vulkan_Guide_and_Khronos_loader_docs
apple_runtime_behavior=MoltenVK_and_LunarG_docs
project_lookup_policy=iggy3d_docs
runtime_truth=iggy3d_runtime_docs_not_package_lookup
```

## Scope

In scope:

- build-tree visual development lookup;
- installed visual package lookup;
- executable-relative shader root;
- package resource root;
- SDL3 runtime discovery/diagnostics;
- MoltenVK runtime discovery/diagnostics on macOS;
- Vulkan loader/runtime diagnostics;
- default diagnostics output path;
- CLI/env override precedence;
- platform-specific macOS/Linux/Windows layouts;
- package startup smoke checks;
- failure reason codes.

Out of scope:

- implementing a general virtual filesystem;
- asset package schema;
- save-game paths;
- user mod directories;
- final installer technology;
- Steam/Epic/GOG package rules;
- notarization/signing;
- Linux AppImage/Flatpak/Snap specifics;
- dynamic runtime downloading;
- changing headless runtime acceptance.

## Local File Surface

Likely future files:

```text
src/app/PackageRuntimeLookup.hpp
src/app/PackageRuntimeLookup.cpp
src/app/platform/ExecutablePath.hpp
src/app/platform/ExecutablePath.cpp
src/render/RendererConfig.hpp
src/render/RendererConfig.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
apps/iggy3d_visual_demo/main.cpp
cmake/iggy3d_install.cmake
cmake/iggy3d_runtime_deps.cmake
tests/unit/package_runtime_lookup_tests.cpp
tests/unit/render_shader_lookup_tests.cpp
tests/smoke/package_visual_startup_smoke.cpp
tests/smoke/package_shader_lookup_smoke.cpp
tests/smoke/package_vulkan_dependency_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| executable path | app/platform startup helper | runtime tick truth |
| package root | package lookup helper | content validation truth |
| resource root | package lookup helper | renderer GPU state |
| shader root | renderer config/shader loader | gameplay asset identity |
| SDL3 runtime status | app/platform diagnostics | runtime command legality |
| Vulkan loader status | Vulkan startup diagnostics | runtime replay outcome |
| MoltenVK runtime status | Apple Vulkan diagnostics | cross-platform API truth |
| diagnostics output root | app/test harness diagnostics | source assets or save state |

Rules:

- runtime/content/projection/save code must not call package graphics lookup helpers;
- package lookup may provide paths to the renderer and visual app only;
- shader lookup is renderer-only and must not validate gameplay content packages;
- diagnostics paths are output paths, not package input truth;
- missing graphics runtime dependencies must not break headless runtime tools.

## Lookup Inputs

The resolver may use these input classes:

```text
compiled_defaults
environment_variables
cli_flags
test_harness_overrides
executable_path
sdl_base_path
sdl_pref_path
install_prefix_relative_candidates
```

Rules:

- explicit CLI/config values override automatic package defaults;
- test harness overrides are allowed only in tests;
- environment variables are developer/devops inputs, not save/replay inputs;
- resolved paths must be absolute or diagnosed as unresolved;
- all attempted lookup roots must be printable in diagnostics;
- path lookup must be deterministic for a given process environment.

## Config Precedence

First resolver precedence:

```text
compiled_defaults
environment_variables
cli_flags
test_harness_overrides
```

Path-specific override examples:

```text
--shader-root <path>
--resource-root <path>
--diagnostics-dir <path>
IGGY3D_SHADER_ROOT=<path>
IGGY3D_RESOURCE_ROOT=<path>
IGGY3D_RENDER_DIAGNOSTICS_DIR=<path>
```

Rules:

- CLI flags override environment variables;
- test harness overrides override CLI only inside test code;
- invalid explicit paths fail before fallback search unless the flag explicitly allows fallback;
- fallback search is for absent paths, not for overriding explicit user intent;
- final resolved values are printed in the startup receipt.

## Runtime Lookup Summary

The installed visual package resolver should compute:

```text
executable_path
executable_dir
package_root
resource_root
shader_root
diagnostics_dir
sdl3_runtime_status
vulkan_loader_status
moltenvk_runtime_status
validation_layer_status
```

Required order:

1. Resolve executable path and executable directory.
2. Resolve package root.
3. Resolve resource root.
4. Resolve shader root.
5. Resolve diagnostics output directory.
6. Initialize/load SDL3 as required by the visual app.
7. Resolve Vulkan loader/runtime status.
8. Resolve MoltenVK runtime status on Apple platforms.
9. Print lookup receipt before creating expensive renderer resources when possible.

Rules:

- shader lookup must happen before shader module creation;
- diagnostics directory may be created if missing;
- resource root must not be created by runtime lookup;
- shader root must not be created by runtime lookup;
- runtime dependency lookup should distinguish not applicable, found, missing, and unavailable.

## Shader Root Lookup

Shader root means the directory containing generated Vulkan SPIR-V artifacts.

Required installed first-room files:

```text
first_room.vert.spv
first_room.frag.spv
```

Lookup order:

```text
1. test_harness_override_shader_root
2. cli_shader_root
3. IGGY3D_SHADER_ROOT
4. executable_relative_shader_root
5. resource_root/shaders/vulkan
6. build_tree_generated_shader_root when package_mode=build_tree_visual
```

Rules:

- the first existing directory with required shader files wins;
- explicit missing shader root fails with `reason_code=shader_root_missing`;
- existing shader root missing required files fails with `reason_code=shader_missing`;
- installed package lookup must not silently fall back to source-tree shader paths;
- build-tree fallback is allowed only when `package_mode=build_tree_visual`;
- headless runtime mode does not resolve shader root.

Executable-relative candidates:

```text
<executable_dir>/shaders/vulkan
<executable_dir>/../shaders/vulkan
<executable_dir>/../share/iggy3d/shaders/vulkan
```

Rules:

- Windows installed packages should normally use `<executable_dir>/shaders/vulkan`;
- Linux prefix installs should normally use `<executable_dir>/../share/iggy3d/shaders/vulkan`;
- macOS app bundles should normally use `<resource_root>/shaders/vulkan`;
- candidate order may be platform-specific but must be printed in diagnostics on failure.

## Resource Root Lookup

Resource root is the package-owned read-only data root for installed visual assets and shader artifacts.

Lookup order:

```text
1. test_harness_override_resource_root
2. cli_resource_root
3. IGGY3D_RESOURCE_ROOT
4. SDL_GetBasePath result when it points at app resources
5. executable_relative_resource_root
6. install_prefix_relative_share_root
7. build_tree_resource_root when package_mode=build_tree_visual
```

Candidate paths:

```text
<executable_dir>/resources
<executable_dir>/../resources
<executable_dir>/../share/iggy3d
<sdl_base_path>
```

Rules:

- `SDL_GetBasePath` is allowed as a resource-root source because SDL defines it as an app data directory helper;
- on macOS app bundles, SDL base path may resolve to the bundle resource directory and should be preferred after explicit overrides;
- resource root is read-only package input;
- missing resource root fails installed visual package smoke with `reason_code=resource_root_missing`;
- visual demo may run without a broad resource root only if shader root is explicitly valid and no other package resources are required.

## Diagnostics Output Lookup

Diagnostics output is writable evidence, not package input.

Lookup order:

```text
1. test_harness_override_diagnostics_dir
2. cli_diagnostics_dir
3. IGGY3D_RENDER_DIAGNOSTICS_DIR
4. build/artifacts/render_diagnostics when package_mode=build_tree_visual
5. SDL_GetPrefPath("iggy3d", "iggy3d")/render_diagnostics
6. process_temp_dir/iggy3d/render_diagnostics as last resort
```

Rules:

- diagnostics directory may be created by the app/test harness;
- diagnostics output must never be placed under installed read-only resource root by default;
- installed visual packages prefer a per-user writable location;
- build-tree visual smoke prefers `build/artifacts/render_diagnostics`;
- failure to create diagnostics directory fails strict diagnostic lanes with `reason_code=diagnostics_dir_unwritable`;
- if diagnostics are optional for a normal visual launch, the app may continue only if the receipt can still report the failure to stderr.

## SDL3 Runtime Lookup

SDL3 is the first proposed window/platform shell dependency for visual packages.

Status values:

```text
sdl3_runtime=linked_static|linked_dynamic_found|linked_dynamic_missing|not_built|unavailable
```

Policy:

- statically linked SDL3 records `linked_static`;
- dynamically linked SDL3 must be resolvable by platform loader rules before visual app startup can succeed;
- installed packages that depend on a dynamic SDL3 runtime must either bundle it or declare a system dependency;
- missing SDL3 in strict visual package smoke fails with `reason_code=sdl3_runtime_missing`;
- headless runtime package must not require SDL3.

Lookup/diagnostic expectations:

```text
macos_app_bundle=Contents/Frameworks/SDL3.framework_or_dylib
windows_package=<executable_dir>/SDL3.dll
linux_package=system_loader_or_package_lib_dir
```

Rules:

- SDL3 load/link status is platform startup diagnostics, not renderer API;
- SDL3 window/surface creation errors use WSI/platform reason codes, not shader lookup reason codes;
- if SDL3 is not used because a future fallback is selected, receipts must identify the selected platform shell.

## Vulkan Loader Lookup

Vulkan loader lookup is platform runtime dependency discovery.

Status values:

```text
vulkan_loader=found|missing|not_built|unavailable
vulkan_loader_path=
vulkan_icd_status=found|missing|unavailable
```

Policy:

- Linux and Windows native Vulkan packages normally use the system Vulkan loader and GPU driver/ICD;
- macOS visual packages use the selected Apple Vulkan/MoltenVK integration path;
- packages should not bundle GPU drivers;
- missing loader is different from no physical device;
- missing ICD/driver is different from unsupported selected device;
- optional software Vulkan lanes must be labeled separately.

Rules:

- `no_vulkan_loader` means the loader/runtime entry point is unavailable;
- `no_vulkan_device` means the loader exists but no acceptable physical device was selected;
- diagnostics must print loader status before device-selection failure when possible;
- runtime/content/projection/save code must never branch on Vulkan loader state.

## MoltenVK Runtime Lookup

MoltenVK is the Apple portability runtime target. It is not the cross-platform Vulkan source of truth.

Status values:

```text
moltenvk_runtime=found|missing|not_applicable|unavailable
moltenvk_runtime_path=
```

macOS lookup candidates:

```text
<app_bundle>/Contents/Frameworks/MoltenVK.framework
<app_bundle>/Contents/Frameworks/libMoltenVK.dylib
<executable_dir>/../Frameworks/MoltenVK.framework
<executable_dir>/../Frameworks/libMoltenVK.dylib
developer_sdk_runtime_when_package_mode=build_tree_visual
```

Rules:

- installed macOS visual packages must not rely only on a developer shell environment;
- dev/build-tree visual mode may rely on LunarG SDK environment only if diagnostics say so;
- missing MoltenVK in strict macOS visual package smoke fails with `reason_code=moltenvk_runtime_missing`;
- MoltenVK-specific portability behavior belongs in platform diagnostics, not renderer public API;
- Linux and Windows report `moltenvk_runtime=not_applicable`.

## macOS Layouts

Preferred installed app bundle layout:

```text
iggy3d_visual_demo.app/
  Contents/
    MacOS/
      iggy3d_visual_demo
    Frameworks/
      MoltenVK.framework or libMoltenVK.dylib
      SDL3.framework or libSDL3.dylib when dynamically bundled
    Resources/
      shaders/
        vulkan/
          first_room.vert.spv
          first_room.frag.spv
```

macOS lookup defaults:

```text
executable_dir=<bundle>/Contents/MacOS
resource_root=<bundle>/Contents/Resources
shader_root=<bundle>/Contents/Resources/shaders/vulkan
diagnostics_dir=SDL_GetPrefPath("iggy3d","iggy3d")/render_diagnostics
moltenvk_runtime=<bundle>/Contents/Frameworks
sdl3_runtime=<bundle>/Contents/Frameworks_or_static
```

macOS CLI build-tree fallback:

```text
shader_root=build/generated/shaders/vulkan/<config>
diagnostics_dir=build/artifacts/render_diagnostics
moltenvk_runtime=developer_sdk_runtime
```

Rules:

- app bundle resource lookup must not depend on current working directory;
- installed bundle should prefer bundled MoltenVK/SDL3 over developer SDK paths;
- validation layers are developer/smoke dependencies and not required for normal release launch;
- receipts must identify `platform_lane=moltenvk`.

## Linux Layouts

Preferred prefix install layout:

```text
<prefix>/
  bin/
    iggy3d_visual_demo
  lib/
    optional bundled SDL3 runtime if chosen
  share/
    iggy3d/
      shaders/
        vulkan/
          first_room.vert.spv
          first_room.frag.spv
```

Linux lookup defaults:

```text
executable_dir=<prefix>/bin
resource_root=<prefix>/share/iggy3d
shader_root=<prefix>/share/iggy3d/shaders/vulkan
diagnostics_dir=SDL_GetPrefPath("iggy3d","iggy3d")/render_diagnostics
vulkan_loader=system_libvulkan
sdl3_runtime=system_or_<prefix>/lib
moltenvk_runtime=not_applicable
```

Rules:

- Linux native Vulkan shipping proof requires native Vulkan hardware validation, not only software Vulkan;
- packages should not bundle GPU drivers or ICDs by default;
- SDL3 may be system dependency or bundled runtime, but the choice must be diagnosed;
- Wayland/X11 selection remains SDL/platform-shell WSI behavior and should be printed by platform smoke when available;
- current working directory must not be required for shader lookup.

## Windows Layouts

Preferred installed package layout:

```text
<package_root>/
  iggy3d_visual_demo.exe
  SDL3.dll
  shaders/
    vulkan/
      first_room.vert.spv
      first_room.frag.spv
```

Windows lookup defaults:

```text
executable_dir=<package_root>
resource_root=<package_root>
shader_root=<package_root>/shaders/vulkan
diagnostics_dir=SDL_GetPrefPath("iggy3d","iggy3d")/render_diagnostics
vulkan_loader=system_vulkan-1.dll
sdl3_runtime=<package_root>/SDL3.dll_or_static
moltenvk_runtime=not_applicable
```

Build-tree multi-config fallback:

```text
executable_dir=build/apps/iggy3d_visual_demo/<config>
shader_root=build/generated/shaders/vulkan/<config>
diagnostics_dir=build/artifacts/render_diagnostics
```

Rules:

- Windows multi-config generator paths must include the active config;
- installed packages should not require launching from the package directory;
- if dynamically linked, `SDL3.dll` must be copied beside the executable or otherwise resolvable by Windows loader rules;
- Vulkan runtime is normally provided by the system driver/runtime through `vulkan-1.dll`;
- receipts must identify `platform_lane=native_vulkan`.

## Receipt Fields

Package runtime lookup receipts should add:

```text
package_mode=headless|build_tree_visual|installed_visual
lookup_version=1
executable_path=
executable_dir=
package_root=
resource_root=
resource_root_source=override|sdl_base_path|executable_relative|install_prefix_relative|build_tree|unresolved
shader_root=
shader_root_source=override|executable_relative|resource_root|build_tree|unresolved
shader_root_exists=true|false
required_shader_count=
missing_shader_count=
diagnostics_dir=
diagnostics_dir_source=override|build_tree|sdl_pref_path|temp|unresolved
diagnostics_dir_writable=true|false|unavailable
sdl_base_path=
sdl_pref_path=
sdl3_runtime=linked_static|linked_dynamic_found|linked_dynamic_missing|not_built|unavailable
sdl3_runtime_path=
vulkan_loader=found|missing|not_built|unavailable
vulkan_loader_path=
vulkan_icd_status=found|missing|unavailable
moltenvk_runtime=found|missing|not_applicable|unavailable
moltenvk_runtime_path=
current_working_directory_used_for_lookup=false
```

Rules:

- `current_working_directory_used_for_lookup` should remain `false` for installed visual packages;
- lookup source fields must say why a path was selected;
- unresolved required paths must produce reason codes;
- receipts must not include raw Vulkan handles.

## Failure Reason Codes

Package runtime lookup should use stable reason codes:

```text
executable_path_unavailable
resource_root_missing
shader_root_missing
shader_missing
diagnostics_dir_unwritable
sdl3_runtime_missing
no_vulkan_loader
no_vulkan_icd
moltenvk_runtime_missing
package_layout_invalid
package_lookup_ambiguous
explicit_lookup_path_invalid
cwd_lookup_forbidden
```

Rules:

- explicit invalid paths fail with `explicit_lookup_path_invalid`;
- missing shader files under an otherwise valid shader root use `shader_missing`;
- missing shader root directory uses `shader_root_missing`;
- ambiguous package roots fail strict package smoke until the resolver order is clarified;
- current-working-directory dependency in installed visual smoke fails with `cwd_lookup_forbidden`.

## Package Smoke Tests

Required future tests:

```text
package_shader_lookup_smoke
package_resource_lookup_smoke
package_diagnostics_dir_smoke
package_vulkan_dependency_smoke
package_visual_startup_smoke
```

Expected checks:

| Test | Checks |
| --- | --- |
| `package_shader_lookup_smoke` | required SPIR-V files resolved from installed or build-tree shader root |
| `package_resource_lookup_smoke` | resource root exists and does not depend on current working directory |
| `package_diagnostics_dir_smoke` | diagnostics directory can be created/written |
| `package_vulkan_dependency_smoke` | SDL3/Vulkan/MoltenVK status reported according to platform lane |
| `package_visual_startup_smoke` | visual app starts, resolves package paths, prints receipt, and exits cleanly |

Strict behavior:

- strict installed visual package smoke fails on missing required shader/runtime paths;
- optional visual smoke may skip when graphics runtime is unavailable before renderer startup;
- headless package smoke must not run graphics lookup;
- tests should run from a working directory outside the package root to catch accidental cwd dependency.

## Acceptance Criteria

This policy is ready for implementation planning when:

- shader root lookup order is explicit;
- resource root lookup order is explicit;
- diagnostics output lookup order is explicit;
- SDL3 runtime status values are explicit;
- Vulkan loader/ICD status values are explicit;
- MoltenVK lookup is macOS-specific and not treated as cross-platform truth;
- macOS, Linux, and Windows installed layouts are named;
- build-tree and installed package modes are separated;
- current working directory is forbidden as an installed package dependency;
- failure reason codes are listed before tests are written;
- headless runtime package remains graphics-free.

