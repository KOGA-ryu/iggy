# Vulkan Platform Matrix

`iggy3d` targets a cross-platform Vulkan renderer. MoltenVK is a required Apple portability lane, not renderer authority. Linux and Windows native Vulkan are required shipping lanes.

This matrix defines platform gates. It does not implement platform code.

## Platform Lanes

| Platform | Backend lane | Role | Required before renderer is shippable |
| --- | --- | --- | --- |
| macOS Apple Silicon | Vulkan through MoltenVK | First local validation lane and Apple portability proof | device, surface, swapchain, sync, shader/pipeline, first-room, diagnostics, replay invariance |
| Linux | Native Vulkan | Required shipping lane | native loader/ICD, validation, sync validation, first-room, memory smoke, diagnostics, replay invariance |
| Windows | Native Vulkan | Required shipping lane | native runtime/SDK, SDL3 DLL/runtime handling, multi-config shader paths, validation, first-room, diagnostics, replay invariance |

## Shippable Renderer Definition

The Vulkan backend is not considered shippable until:

- macOS/MoltenVK validates the Apple portability lane;
- Linux validates native Vulkan behavior;
- Windows validates native Vulkan behavior;
- all lanes use the same public renderer API;
- all lanes consume the same `FrameInput` contract;
- all lanes emit the same receipt schema;
- renderer enabled/disabled/null/Vulkan does not alter replay hash;
- platform differences are diagnostics or backend-private conditionals, not runtime architecture forks.

## Minimum API Baseline

Proposed minimum: Vulkan 1.3 feature baseline where available.

Allowed fallback: Vulkan 1.2 plus documented extensions only if a required lane proves the fallback is necessary.

Not required for first room: Vulkan 1.4.

API evidence required per lane:

```text
api_version=
driver_version=
enabled_features=
enabled_instance_extensions=
enabled_device_extensions=
portability_subset=true|false|unavailable
```

Rules:

- do not assume API version from OS/platform name;
- query and print actual device API version;
- extension fallback must be visible in diagnostics;
- old render-pass compatibility is fallback only, not default.

## Feature Matrix

| Feature | Proposed status | macOS/MoltenVK | Linux native Vulkan | Windows native Vulkan | Fallback |
| --- | --- | --- | --- | --- | --- |
| SDL3 window/surface shell | Required for visual lane | required | required | required | GLFW only if SDL3 gate fails |
| Vulkan validation layers | Required in strict smoke | required when available through SDK | required in strict lane | required in strict lane | fail strict lane if required and missing |
| Synchronization validation | Required in strict sync lane | required when available | required in strict lane | required in strict lane | fail strict sync lane if required and missing |
| Dynamic rendering | Proposed default | must be queried/proven | must be queried/proven | must be queried/proven | render-pass path if required lane fails |
| Binary WSI semaphores/fences | Required first sync path | required | required | required | none for first frame |
| Timeline semaphores | Optional/deferred for internal work | query only | query only | query only | binary sync remains default |
| GLSL/glslang shader compile | Proposed first shader path | required when compile enabled | required when compile enabled | required when compile enabled | packaged SPIR-V only if source/provenance exists |
| VMA | Deferred until Phase 8 | required for growth lane | required for growth lane | required for growth lane | documented allocation policy only if VMA blocked |
| RenderDoc | Optional/deferred | optional | optional | optional | none |
| Software Vulkan/lavapipe | Optional CI aid | not applicable | optional CI lane | not primary | cannot replace native/hardware shipping proof |

## Required Extension/Capability Categories

Exact extension names are selected during device/surface implementation, but each lane must report these categories.

Instance categories:

- platform-required WSI extensions from SDL3;
- validation/debug utils extension when validation is enabled;
- portability enumeration where required by MoltenVK;
- no runtime/content/projection dependency.

Device categories:

- swapchain support;
- dynamic rendering support through core feature or extension path;
- synchronization features required by selected sync policy;
- portability subset where required by MoltenVK;
- depth format feature support.

Diagnostics must distinguish:

```text
platform_required_instance_extensions=
renderer_required_instance_extensions=
enabled_instance_extensions=
enabled_device_extensions=
enabled_features=
```

## Swapchain Policy Matrix

| Policy | Required behavior |
| --- | --- |
| Surface format | select a supported format; report `swapchain_format` and color space |
| Present mode | prefer mailbox if available, otherwise fifo; fifo is always acceptable baseline |
| Extent | use framebuffer size from platform shell; zero extent means not drawable/minimized |
| Image count | select supported count; report `swapchain_image_count` |
| Resize | repeated resize/recreate must be validation-clean |
| Minimize | zero-size frame must skip presentation without runtime mutation |

Platform notes:

- macOS/MoltenVK may have portability/presentation constraints; record them but do not redefine runtime behavior.
- Linux may differ between X11 and Wayland under SDL3; this is shell/platform diagnostics, not runtime state.
- Windows multi-config build paths must not affect swapchain behavior.

## Depth Format Matrix

Initial fallback order:

```text
VK_FORMAT_D32_SFLOAT
VK_FORMAT_D24_UNORM_S8_UINT
VK_FORMAT_D32_SFLOAT_S8_UINT
```

Rules:

- selected format must support depth-stencil attachment usage;
- pure depth is preferred when stencil is unused;
- chosen format must be printed as `depth_format`;
- unsupported depth format fails first-room Vulkan rendering.

## Shader Compiler Matrix

| Platform | First compiler path | Required proof |
| --- | --- | --- |
| macOS/MoltenVK | `glslangValidator` from SDK/PATH or configured path | compile first-room GLSL to SPIR-V, package/dev shader root resolves |
| Linux native Vulkan | `glslangValidator` from package/PATH or configured path | compile first-room GLSL to SPIR-V, generated path stable |
| Windows native Vulkan | `glslangValidator` from SDK/PATH or configured path | compile first-room GLSL to SPIR-V under multi-config output |

Rules:

- shader compiler is build-time only;
- packaged SPIR-V is runtime input for visual package;
- missing compiler fails only when shader compilation is required;
- runtime/content/projection/save must not mention shader language.

## Validation Lane Matrix

| Lane | Default behavior | Strict behavior |
| --- | --- | --- |
| Headless runtime | no Vulkan tests required | must still not require Vulkan |
| Optional Vulkan smoke | skip missing GPU/display/loader with receipt and exit code 77 | not applicable |
| Strict Vulkan smoke | not applicable | missing required GPU/display/loader/tooling fails |
| Strict validation | validation unavailable fails | validation errors fail |
| Strict sync validation | sync validation unavailable fails | sync validation errors fail |

Required receipt fields:

```text
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
validation_strict=true|false
sync_validation_clean=true|false|unavailable
```

## Platform Proof Commands

Default headless proof:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional Vulkan proof:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Strict platform proof:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VALIDATION_LAYERS=ON -DIGGY3D_REQUIRE_SYNC_VALIDATION=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Replay invariance proof:

```sh
ctest --test-dir build --output-on-failure -R 'render_replay|replay_state_hash'
```

## Per-Platform Gates

### macOS/MoltenVK Gate

Required labels:

```text
macos;moltenvk
```

Required proof:

- SDL3 creates a Vulkan-capable window;
- SDL3 reports required instance extensions;
- Vulkan instance enables portability requirements where needed;
- MoltenVK device is selected and diagnosed;
- surface creation succeeds;
- swapchain creation succeeds;
- validation/sync validation status is printed;
- shader/pipeline smoke passes;
- first-room smoke passes;
- replay invariance passes.

Required diagnostics:

```text
platform=macos
platform_lane=moltenvk
window_shell=sdl3
moltenvk_runtime=found|missing
portability_subset=true|false|unavailable
device_name=
api_version=
swapchain_format=
present_mode=
shader_language=glsl
```

Exit criteria:

- local Apple lane is proven;
- MoltenVK quirks are documented as platform notes;
- no MoltenVK requirement leaks into runtime/content/projection/save.

### Linux Native Vulkan Gate

Required labels:

```text
linux;native_vulkan
```

Required proof:

- Vulkan loader exists;
- native ICD/device exists;
- SDL3 window/surface path works;
- validation layers are available in strict lane;
- sync validation is available in strict lane or failure is explicit;
- shader compile path works;
- memory smoke passes;
- first-room smoke passes;
- replay invariance passes.

Required diagnostics:

```text
platform=linux
platform_lane=native_vulkan
vulkan_loader=found|missing
device_name=
driver_version=
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
```

Exit criteria:

- Linux proves native Vulkan behavior;
- optional lavapipe/software lane, if added, is labeled separately;
- native/hardware lane remains required for shipping.

### Windows Native Vulkan Gate

Required labels:

```text
windows;native_vulkan
```

Required proof:

- Vulkan runtime/SDK path works;
- native device is selected and diagnosed;
- SDL3 runtime/DLL lookup works;
- multi-config shader output path works;
- validation layers are available in strict lane;
- shader/pipeline smoke passes;
- first-room smoke passes;
- replay invariance passes.

Required diagnostics:

```text
platform=windows
platform_lane=native_vulkan
vulkan_loader=found|missing
sdl3_runtime=linked_static|linked_dynamic|missing|unavailable
shader_root=
shader_root_exists=true|false
device_name=
driver_version=
```

Exit criteria:

- Windows proves native Vulkan behavior;
- shader/runtime lookup works in multi-config layout;
- SDL3 DLL/runtime handling is explicit.

## Platform Macros And Conditional Code

Allowed platform conditionals:

- app/platform shell files;
- Vulkan backend platform setup;
- packaging/install scripts;
- smoke-test skip logic;
- diagnostics platform names.

Forbidden platform conditionals:

- gameplay command legality;
- runtime state hash;
- save/load truth;
- projection semantics;
- content validation truth.

Macro policy:

- prefer CMake target definitions over scattered compiler checks;
- platform macros must be isolated to app/platform, renderer backend, packaging, or tests;
- public runtime headers should not need OS macros for Vulkan.

Reviewer scan:

```sh
rg -n 'VK_|Vk[A-Z]|SDL_|_WIN32|__APPLE__|__linux__' src/runtime src/content src/projection src/runtime/save
```

Expected result: no matches unless a non-render platform abstraction is explicitly reviewed later.

## Fallback Recording Rules

Every fallback must be visible in diagnostics.

Required fallback fields when relevant:

```text
rendering_path=dynamic|render_pass
sync_policy=binary_wsi|timeline|mixed|unavailable
shader_language=glsl|slang|spirv_only|unavailable
memory_allocator=none|manual_bootstrap|vma
window_shell=sdl3|glfw|native|unavailable
fallback_reason=
```

Rules:

- no silent fallback;
- fallback on one platform does not automatically become global default;
- if fallback affects file plans, update `decisions.md`;
- if fallback affects package layout, update `packaging.md`;
- if fallback affects test behavior, update `diagnostics_and_tests.md`.

## Acceptance Criteria

The platform matrix is ready for file plans when:

- macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan gates are explicit;
- minimum API baseline and fallback policy are explicit;
- feature matrix is explicit;
- swapchain/depth/shader/validation gates are explicit;
- platform proof commands are listed;
- platform macros are constrained;
- fallback recording rules are explicit;
- shippable renderer definition requires all three lanes.

## Open Detail Items

These belong in future platform file plans:

- exact extension list after SDL3/Vulkan implementation starts;
- exact MoltenVK SDK version used by local dev lane;
- exact Linux distro/package assumptions;
- exact Windows SDK/DLL discovery;
- exact CI provider and runner matrix;
- exact lavapipe/software Vulkan policy if chosen;
- exact RenderDoc availability notes per platform.
