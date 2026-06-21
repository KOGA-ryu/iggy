# Vulkan Packaging

This document defines how the visual demo and future game package Vulkan runtime dependencies, SDL3, validation tooling expectations, and shader artifacts.

Packaging must not change runtime determinism or renderer boundaries. Headless runtime tools remain shippable without SDL, Vulkan, MoltenVK, shader compilers, GPU drivers, or display access.

## Packaging Modes

`iggy3d` has three packaging/build modes:

| Mode | Purpose | Graphics dependencies | Shader artifacts |
| --- | --- | --- | --- |
| Headless runtime | content validation, demo script, save/load, replay | none | none |
| Build-tree visual dev | local visual demo and smoke tests | SDL3, Vulkan SDK/loader, optional validation layers | `build/generated/shaders/vulkan/<config>/` |
| Installed visual package | future distributable visual demo/game | platform runtime deps as needed | installed beside executable or under package resource dir |

Rules:

- headless runtime mode must work without graphics dependencies;
- visual dev mode can rely on developer-installed SDKs/tools;
- installed visual package must have deterministic runtime lookup paths;
- runtime/content/projection/save never load shader artifacts;
- renderer config owns shader root lookup.

## Local File Surface

Likely future paths:

```text
cmake/iggy3d_vulkan_deps.cmake
cmake/iggy3d_shaders.cmake
cmake/iggy3d_install.cmake
apps/iggy3d_visual_demo/main.cpp
src/render/RendererApi.hpp
src/render/RenderDiagnostics.hpp
src/render/vulkan/VulkanBackend.hpp
src/render/vulkan/PipelinesShaders.hpp
shaders/vulkan/src/
build/generated/shaders/vulkan/<config>/
build/artifacts/render_diagnostics/
```

No package script should reference old repo paths or import old build targets.

## Dependency Classes

### Build-Time Dependencies

Needed only when corresponding features are enabled:

- C++ compiler and CMake;
- Vulkan headers/loader import target for Vulkan backend;
- SDL3 for visual demo/platform shell;
- glslang for GLSL shader compilation;
- VMA source/header when Phase 8 resource growth begins.

### Runtime Dependencies

Visual package may require:

- Vulkan loader/runtime;
- platform GPU driver/ICD;
- SDL3 runtime library if dynamically linked;
- MoltenVK runtime library on Apple platforms;
- shader SPIR-V artifacts;
- optional validation layers for developer/smoke builds.

Headless package requires none of those.

### Developer-Only Dependencies

- validation layers;
- synchronization validation;
- RenderDoc;
- shader compiler if generated SPIR-V is already packaged.

Developer-only dependencies must not be required for normal installed runtime unless the package is explicitly a dev/smoke build.

## CMake Options

Renderer packaging options belong to the renderer-enabled build pass.

Proposed options:

```text
IGGY3D_ENABLE_VISUAL_DEMO
IGGY3D_ENABLE_VULKAN
IGGY3D_ENABLE_SHADER_COMPILE
IGGY3D_ENABLE_VULKAN_SMOKE
IGGY3D_REQUIRE_VULKAN_SMOKE
IGGY3D_USE_SYSTEM_SDL3
IGGY3D_USE_SYSTEM_VMA
IGGY3D_SHADER_OUTPUT_DIR
IGGY3D_SHADER_INSTALL_DIR
IGGY3D_RENDER_DIAGNOSTICS_DIR
```

Rules:

- all options use `IGGY3D_` prefix;
- visual/Vulkan options default off until headless runtime acceptance is green;
- no configure-time network dependency unless explicitly selected by a future option;
- configure output prints the selected graphics dependency mode;
- disabling visual/Vulkan must still build headless runtime targets.

## Shader Artifact Layout

Build-tree generated root:

```text
build/generated/shaders/vulkan/<config>/
```

First generated files:

```text
first_room.vert.spv
first_room.frag.spv
```

Recommended installed visual package layout:

```text
<package_root>/
  bin/
    iggy3d_visual_demo
  share/
    iggy3d/
      shaders/
        vulkan/
          first_room.vert.spv
          first_room.frag.spv
```

Windows alternative layout:

```text
<package_root>/
  iggy3d_visual_demo.exe
  shaders/
    vulkan/
      first_room.vert.spv
      first_room.frag.spv
```

Rules:

- renderer config owns shader root;
- visual demo may default shader root to build-generated path in dev mode;
- installed package must pass or discover installed shader root explicitly;
- missing shader files produce `reason_code=shader_missing`;
- runtime/content/projection/save do not know shader paths.

## Diagnostics Receipt Additions

Package/startup diagnostics should add:

```text
package_mode=headless|build_tree_visual|installed_visual
executable_path=
resource_root=
shader_root=
shader_root_exists=true|false
vulkan_loader=found|missing|unavailable
vulkan_sdk_path=
sdl3_runtime=linked_static|linked_dynamic|missing|unavailable
moltenvk_runtime=found|missing|not_applicable
validation_layers=found|missing|not_requested
sync_validation=found|missing|not_requested
renderdoc=found|missing|not_requested
```

Rules:

- missing optional developer tools do not fail normal runtime;
- missing required runtime dependency fails strict visual package smoke;
- diagnostics do not affect replay hash.

## macOS/MoltenVK

Role: first local validation lane and Apple portability lane.

Expected development setup:

- LunarG Vulkan SDK installed;
- MoltenVK available through SDK or package configuration;
- SDL3 available through system/package/developer setup;
- validation layers available for smoke/dev when requested;
- shader compiler available when shader compilation is enabled.

Runtime expectations:

- Vulkan is provided through MoltenVK;
- package must locate MoltenVK runtime library;
- package must locate SDL3 runtime library if dynamically linked;
- shader artifacts must be packaged or shader root configured.

CLI visual demo development layout:

```text
build/apps/iggy3d_visual_demo/iggy3d_visual_demo
build/generated/shaders/vulkan/<config>/
build/artifacts/render_diagnostics/
```

Future app bundle layout, if chosen:

```text
iggy3d_visual_demo.app/
  Contents/
    MacOS/
      iggy3d_visual_demo
    Frameworks/
      libMoltenVK.dylib or MoltenVK framework
      SDL3 runtime if dynamically linked
    Resources/
      shaders/
        vulkan/
          first_room.vert.spv
          first_room.frag.spv
```

rpath/bundle rules:

- executable must resolve MoltenVK and SDL3 without global shell-only environment in installed package;
- dev builds may rely on SDK environment if documented in diagnostics;
- installed package must prefer relative bundle/library lookup;
- validation layers are dev/smoke dependencies, not normal shipped runtime requirements.

macOS diagnostics must report:

```text
platform=macos
platform_lane=moltenvk
moltenvk_runtime=found|missing
vulkan_sdk_path=
portability_subset=true|false|unavailable
```

Failure examples:

- missing MoltenVK in strict visual package: `reason_code=no_vulkan_loader` or more specific future code;
- missing shader artifact: `reason_code=shader_missing`;
- missing validation layers in strict validation lane: `reason_code=validation_layer_required_missing`.

Open macOS details:

- exact SDK discovery variables;
- exact bundle vs CLI first delivery choice;
- exact framework/dylib copy command;
- notarization/signing if ever needed.

## Linux Native Vulkan

Role: required shipping lane.

Expected development setup:

- Vulkan loader installed;
- GPU driver/ICD installed;
- SDL3 installed or provided;
- validation layers installed for dev/smoke lane;
- glslang available when shader compilation is enabled.

Runtime expectations:

- package depends on system Vulkan loader and GPU driver/ICD;
- package depends on SDL3 runtime unless statically linked or bundled;
- shader artifacts are installed under package resource root;
- validation layers are optional developer dependencies.

Recommended installed layout:

```text
<prefix>/
  bin/
    iggy3d_visual_demo
  share/
    iggy3d/
      shaders/
        vulkan/
          first_room.vert.spv
          first_room.frag.spv
```

Loader/ICD rules:

- package should not bundle GPU drivers;
- diagnostics must distinguish missing loader from missing device;
- diagnostics must identify native Vulkan lane;
- optional lavapipe/software Vulkan lane may exist for CI but must not replace hardware/native validation for shipping.

Linux diagnostics must report:

```text
platform=linux
platform_lane=native_vulkan
vulkan_loader=found|missing
device_name=
driver_version=
```

Open Linux details:

- distro package names;
- Wayland vs X11 expectations through SDL3;
- optional lavapipe package/CI policy;
- install rule for SDL3 if bundling is chosen.

## Windows Native Vulkan

Role: required shipping lane.

Expected development setup:

- Vulkan SDK or runtime installed;
- GPU driver with Vulkan support;
- SDL3 available;
- glslang available when shader compilation is enabled;
- validation layers available for strict dev/smoke lane.

Runtime expectations:

- installed visual package can locate Vulkan runtime through system Vulkan loader;
- package includes/copies SDL3 DLL if dynamically linked and not otherwise installed;
- shader artifacts are placed in a known path relative to executable or configured resource root;
- multi-config generator output paths are handled explicitly.

Recommended installed layout:

```text
<package_root>/
  iggy3d_visual_demo.exe
  SDL3.dll
  shaders/
    vulkan/
      first_room.vert.spv
      first_room.frag.spv
```

Multi-config build-tree layout:

```text
build/
  apps/
    iggy3d_visual_demo/
      <config>/
        iggy3d_visual_demo.exe
  generated/
    shaders/
      vulkan/
        <config>/
          first_room.vert.spv
          first_room.frag.spv
```

Windows diagnostics must report:

```text
platform=windows
platform_lane=native_vulkan
vulkan_loader=found|missing
sdl3_runtime=linked_static|linked_dynamic|missing|unavailable
shader_root=
shader_root_exists=true|false
```

Open Windows details:

- exact DLL copy/install command;
- Visual Studio generator expression paths;
- Vulkan SDK environment variable handling;
- validation layer setup instructions;
- RenderDoc capture notes.

## Validation And Developer Tool Packaging

Validation layers:

- required only in strict validation/smoke lanes;
- optional in normal installed visual package;
- unavailable validation must be diagnosed;
- release builds do not force validation on.

Synchronization validation:

- same policy as validation layers;
- required only when strict sync validation lane requests it.

RenderDoc:

- never a runtime dependency;
- never required for automated pass;
- manual/deferred debugging tool after first visual frame exists;
- capture files go under diagnostics artifacts when produced.

Shader compiler:

- build-time dependency when `IGGY3D_ENABLE_SHADER_COMPILE=ON`;
- not a runtime dependency if SPIR-V artifacts are packaged;
- missing compiler fails shader compile policy only when shader compilation is required.

## Package Smoke Tests

Recommended smoke checks:

```text
package_headless_smoke
package_visual_startup_smoke
package_shader_lookup_smoke
package_vulkan_dependency_smoke
```

Labels:

```text
iggy3d;packaging
iggy3d;packaging;headless
iggy3d;packaging;vulkan;requires_display
```

Headless package smoke:

- runs without SDL/Vulkan;
- validates first-room fixture;
- runs headless demo/replay;
- proves no shader/resource lookup occurs.

Visual package startup smoke:

- starts visual demo with `--frames 1`;
- prints receipt;
- locates shader root;
- locates SDL/Vulkan runtime dependencies;
- exits cleanly.

Shader lookup smoke:

- verifies packaged SPIR-V exists;
- verifies missing shader produces `reason_code=shader_missing`;
- does not invoke runtime mutation.

Strict command:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'packaging|vulkan'
```

## Failure Policy

Packaging failures must use stable reason codes when possible:

```text
no_vulkan_loader
no_vulkan_device
missing_validation_layers
missing_sync_validation
missing_shader_compiler
shader_missing
surface_create_failed
unsupported_platform_lane
```

Additional package-specific codes may be added later:

```text
sdl3_runtime_missing
moltenvk_runtime_missing
resource_root_missing
shader_root_missing
package_layout_invalid
```

Rules:

- strict visual package smoke fails on missing required runtime dependency;
- optional developer dependencies may warn or mark unavailable;
- missing shader root fails visual package startup;
- headless package smoke must not care about shader root or graphics runtime.

## Acceptance Criteria

The packaging contract is ready for file plans when:

- headless, build-tree visual, and installed visual modes are distinct;
- shader artifact layout is named;
- macOS/MoltenVK runtime expectations are named;
- Linux native Vulkan runtime expectations are named;
- Windows native Vulkan runtime expectations are named;
- diagnostics fields are named;
- package smoke tests are named;
- missing dependency behavior is defined;
- headless package remains graphics-free.

## Open Detail Items

These belong in future file plans or platform-specific docs:

- exact CMake install commands;
- exact SDL3 dynamic/static link policy;
- exact MoltenVK copy/rpath commands;
- exact Linux package dependency list;
- exact Windows DLL copy commands;
- exact shader root resolver API;
- exact package smoke implementation;
- exact release packaging format.
