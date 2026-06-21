# Vulkan Decisions

This file is the Vulkan decision ledger for `iggy3d`. It records proposed defaults, alternatives, evidence gates, file impact, and tests. A decision is not final just because it is convenient on one machine.

`iggy3d` targets a cross-platform Vulkan renderer:

- macOS through MoltenVK is the first local validation lane;
- Linux native Vulkan is a required shipping lane;
- Windows native Vulkan is a required shipping lane;
- MoltenVK constraints may create platform notes, but they must not become engine authority.

## Status Terms

Use these exact status labels:

- `Proposed`: recommended default, not yet proven.
- `Gate Required`: implementation may start only behind a test/proof gate.
- `Accepted`: proven across required lanes or explicitly accepted with documented limits.
- `Fallback`: allowed path if the proposed default fails a gate.
- `Rejected`: do not use unless the decision is reopened.
- `Deferred`: intentionally later than first visual room.

## Decision Summary

| Decision | Status | Proposed default | Gate |
| --- | --- | --- | --- |
| Vulkan API baseline | Proposed | Vulkan 1.3 feature baseline where available, with extension fallback only if documented | Phase 3 device diagnostics |
| Dynamic rendering vs render pass | Proposed | Dynamic rendering | Phase 6 empty frame and Phase 7 pipeline |
| Synchronization model | Proposed | Binary semaphores/fences for swapchain WSI first; timeline semaphore only for internal GPU work after proof | Phase 6 sync validation |
| Window/surface library | Proposed | SDL3 shell isolated in app/platform layer | Phase 2 platform smoke |
| Shader language | Proposed | GLSL/glslang for first room, shader interface remains language-neutral | Phase 7 shader compile proof |
| VMA adoption | Deferred until Phase 8 | Use VMA for real buffers/images | Phase 8 allocation diagnostics |
| Shader artifacts | Proposed | commit sources, generate SPIR-V in build tree, install/package generated artifacts | Phase 7 shader policy test |
| GPU smoke tests | Proposed | explicit CTest labels and skip behavior; required in platform validation lanes | Phase 11 diagnostics receipt |
| Validation policy | Proposed | validation on in dev/smoke, sync validation on when available, release off by default | Phase 3 and Phase 6 |
| RenderDoc | Deferred | secondary manual/debug capture tool after first visual frame | Phase 10/11 |

## Decision: Vulkan API Baseline

Status: `Proposed`

Recommended default: target a Vulkan 1.3 feature baseline for the first renderer path, while recording the actual selected API version and enabled features per platform. Do not require Vulkan 1.4 for the first room unless device diagnostics prove it is painless across macOS/MoltenVK, Linux, and Windows.

Why:

- dynamic rendering is available as a Vulkan 1.3 core feature or through `VK_KHR_dynamic_rendering`;
- timeline semaphores are available as Vulkan 1.2 core feature or through `VK_KHR_timeline_semaphore`;
- Vulkan 1.3 is a more conservative cross-platform target than requiring 1.4 immediately;
- the current Khronos tutorial may teach a newer baseline, but `iggy3d` needs a shippable cross-platform renderer, not tutorial lockstep.

Alternatives:

- require Vulkan 1.4: simplest alignment with current tutorial, but higher platform risk;
- target Vulkan 1.2 plus extensions: broader compatibility, but more feature-branch complexity;
- legacy render-pass-only Vulkan 1.2: fallback only, not preferred.

Evidence required:

- device diagnostics on macOS/MoltenVK, Linux, and Windows;
- `api_version` reported in smoke receipt;
- enabled feature list in smoke receipt;
- required extension list in smoke receipt;
- clear reason if a platform uses extension fallback instead of core feature path.

File impact:

- `src/render/vulkan/InstanceDeviceSurface.hpp/.cpp`
- `src/render/vulkan/RenderDiagnostics.hpp/.cpp`
- `tests/smoke/vulkan_device_smoke.cpp`
- `docs/vulkan/platform_matrix.md`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|render_diagnostics'
```

Exit criteria:

- all required platforms report API version, driver version, enabled features, and enabled extensions;
- no runtime/content/projection/save files include Vulkan headers;
- platform differences are diagnostics, not architecture forks.

## Decision: Dynamic Rendering vs Render Pass

Status: `Proposed`

Recommended default: use dynamic rendering for new Vulkan work.

Why:

- current Khronos learning material emphasizes the modern path;
- first-room rendering does not need the older render-pass/framebuffer object model;
- dynamic rendering reduces early coupling between swapchain image format, depth format, and pipeline setup;
- the standalone repo should not preserve old renderer structure just because it exists historically.

Fallback: use classic render passes only if dynamic rendering fails platform proof on a required lane. The fallback must be explicit and documented; it must not silently become the default.

Required feature/extension evidence:

- `VkPhysicalDeviceDynamicRenderingFeatures::dynamicRendering == VK_TRUE`, or documented extension path;
- API version or extension source recorded in diagnostics;
- validation-clean empty frame;
- validation-clean first pipeline.

Pipeline impact:

- dynamic path uses pipeline rendering metadata instead of a fixed `VkRenderPass`;
- pipeline creation must know color and depth formats;
- swapchain recreation must update format-dependent pipeline state if needed;
- depth format selection remains renderer-owned.

Fallback impact:

- render-pass path needs explicit render pass, framebuffer lifecycle, subpass/dependency policy, and swapchain-format coupling;
- fallback adds complexity and should be isolated to Vulkan backend modules only.

File impact:

- `src/render/vulkan/PipelinesShaders.hpp/.cpp`
- `src/render/vulkan/CommandBuffers.hpp/.cpp`
- `src/render/vulkan/Swapchain.hpp/.cpp`
- `src/render/vulkan/VulkanTypes.hpp`
- `tests/smoke/vulkan_pipeline_smoke.cpp`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_empty_frame|vulkan_pipeline'
```

Exit criteria:

- diagnostics print `rendering_path=dynamic` or `rendering_path=render_pass`;
- first accepted path is validation-clean;
- fallback path is absent unless a platform proof requires it.

## Decision: Synchronization Model

Status: `Proposed`

Recommended default: use the conservative WSI sync path first: per-frame fences plus binary semaphores for image acquire and present. Consider timeline semaphores for internal GPU work only after the swapchain path is validation-clean.

Why:

- swapchain acquire/present behavior is the highest-risk early sync surface;
- binary semaphores are the common WSI path and easiest to reason about for first frame;
- timeline semaphores are useful, but forcing them into presentation before proof increases early complexity;
- sync validation should shape the implementation before optimization does.

Alternatives:

- timeline semaphore everywhere: modern but riskier around WSI and portability details;
- binary semaphores only forever: simple, but may become clumsy once uploads/async work grow;
- no frames-in-flight abstraction: rejected.

Initial frames-in-flight default:

- `2` frames in flight;
- expose as a renderer constant or backend config;
- diagnostics must print the chosen count.

Required evidence:

- validation layers enabled in dev/smoke;
- synchronization validation enabled or unavailable reason printed;
- sustained empty-frame loop;
- repeated resize/recreate loop;
- no per-frame fence deadlock;
- no present/acquire validation errors.

File impact:

- `src/render/vulkan/FrameSync.hpp/.cpp`
- `src/render/vulkan/CommandBuffers.hpp/.cpp`
- `src/render/vulkan/Swapchain.hpp/.cpp`
- `src/render/vulkan/DebugValidation.hpp/.cpp`
- `tests/smoke/vulkan_sync_smoke.cpp`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_sync|vulkan_swapchain|vulkan_empty_frame'
```

Exit criteria:

- diagnostics print `sync_policy=binary_wsi`;
- diagnostics print `frames_in_flight=2` unless changed with evidence;
- sync validation is clean or the unavailable reason is explicit;
- timeline semaphore adoption is a later subdecision, not hidden scope.

## Decision: Window/Surface Library

Status: `Proposed`

Recommended default: use SDL3 for the first visual app/platform shell.

Why:

- `iggy3d` is a game, not just a Vulkan sample;
- SDL3 provides a cross-platform path for windows, input, controllers, and future app needs;
- SDL3 has Vulkan extension and surface APIs, including `SDL_Vulkan_GetInstanceExtensions` and `SDL_Vulkan_CreateSurface`;
- GLFW is excellent for minimal windowing, but SDL3 better matches future game input/platform needs.

Alternatives:

- GLFW: simpler surface/window path and very common in Vulkan tutorials; acceptable fallback if SDL3 dependency friction blocks first frame;
- native platform code: rejected for first implementation because it multiplies macOS/Linux/Windows surface code early;
- keep both SDL3 and GLFW: rejected for first implementation because it expands test matrix without improving the first renderer boundary.

Surface ownership rule:

- app/platform shell owns the native window object;
- Vulkan backend owns `VkSurfaceKHR` lifetime after creation contract is satisfied;
- runtime never sees the window object or surface;
- Vulkan headers stay out of runtime/content/projection/save.

Evidence required:

- SDL3 can be found or acquired by CMake on macOS, Linux, and Windows;
- SDL3 can create a Vulkan-capable window;
- SDL3 can report required instance extensions;
- SDL3 can create a Vulkan surface;
- resize/minimize/focus/quit events can be routed without runtime dependency on SDL.

File impact:

- `apps/iggy3d_visual_demo/main.cpp`
- possible future `src/app/platform/SdlWindow.hpp/.cpp`
- `src/render/vulkan/InstanceDeviceSurface.hpp/.cpp`
- `cmake/iggy3d_vulkan_deps.cmake`
- `tests/smoke/vulkan_platform_smoke.cpp`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_platform|vulkan_device'
```

Exit criteria:

- diagnostics print `window_shell=sdl3`;
- window creation and shutdown are clean;
- surface creation and destruction are clean;
- headless runtime apps still build without SDL.

Sources to consult:

- SDL3 Vulkan category: https://wiki.libsdl.org/SDL3/CategoryVulkan
- SDL3 surface creation: https://wiki.libsdl.org/SDL_Vulkan_CreateSurface
- GLFW Vulkan guide, if fallback is evaluated: https://www.glfw.org/docs/3.3/vulkan_guide.html

## Decision: Shader Language

Status: `Proposed`

Recommended default: use GLSL with glslang for the first room, while keeping the renderer shader interface language-neutral so Slang can replace or supplement it later.

Why:

- GLSL/glslang is a mature Khronos shader path;
- first-room shaders should minimize toolchain novelty;
- the renderer consumes SPIR-V either way;
- language neutrality keeps runtime/projection isolated from shader authoring choices;
- Slang remains attractive, especially because current Khronos tutorial material uses it, but it should earn adoption through a cross-platform compile proof.

Alternatives:

- Slang first: modern tutorial alignment and good future flexibility, but toolchain packaging must be proven on all target platforms;
- checked-in SPIR-V only: useful emergency fallback, but poor authoring experience and weak reproducibility unless paired with source/provenance;
- HLSL path: deferred unless Windows tooling becomes the dominant constraint.

Required evidence:

- compiler discovery on macOS, Linux, and Windows;
- build command recorded in CMake;
- deterministic output path;
- source hash or timestamp diagnostics;
- generated SPIR-V loads into first pipeline;
- failure output is visible in build/test logs.

File impact:

- `shaders/vulkan/src/`
- `build/generated/shaders/vulkan/`
- `src/render/vulkan/PipelinesShaders.hpp/.cpp`
- `cmake/iggy3d_shaders.cmake`
- `tests/unit/render_shader_policy_tests.cpp`
- `tests/smoke/vulkan_pipeline_smoke.cpp`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'shader_policy|vulkan_pipeline'
```

Exit criteria:

- diagnostics print `shader_language=glsl` or later accepted value;
- diagnostics print compiler name/version when available;
- generated SPIR-V is not guessed at runtime;
- runtime/content/projection/save do not mention shader language.

Sources to consult:

- Vulkan Guide Slang page: https://docs.vulkan.org/guide/latest/slang.html
- glslang project: https://github.com/KhronosGroup/glslang
- Slang compiler docs, if Slang is evaluated: https://shader-slang.org/slang/user-guide/compiling.html

## Decision: VMA Adoption And Acquisition

Status: `Deferred`

Recommended default: adopt Vulkan Memory Allocator in Phase 8, after device/swapchain/sync/pipeline basics are proven.

Why:

- first renderer boundary and empty-frame work do not need a full allocation layer;
- real buffers/images need allocation naming, budgets, mapping, and diagnostics;
- hand-rolling Vulkan memory management early creates unnecessary risk;
- VMA is built for this exact problem space.

Acquisition recommendation: vendor a pinned VMA release snapshot when Phase 8 begins, unless the project has a package/dependency policy by then that clearly prefers another method.

Why vendor by default:

- reproducible local builds;
- no configure-time network dependency;
- simple license review surface;
- easy version reporting.

Alternatives:

- CMake `FetchContent` with pinned tag/hash: acceptable for builder convenience, but avoid hidden network dependency in release/CI;
- system package: acceptable as an override, not as the only path;
- manual memory only: rejected for growth path unless VMA cannot be used.

Required evidence:

- version pin recorded;
- allocator creation/destruction order documented;
- allocation naming works;
- memory budget/stat diagnostics print;
- first vertex/index/depth allocations are validation-clean.

File impact:

- `third_party/` dependency folder when vendored;
- `cmake/iggy3d_vulkan_deps.cmake`;
- `src/render/vulkan/BuffersImagesMemory.hpp/.cpp`;
- `src/render/vulkan/RenderDiagnostics.cpp`;
- `tests/unit/render_memory_policy_tests.cpp`;
- `tests/smoke/vulkan_memory_smoke.cpp`.

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'render_memory|vulkan_memory|render_diagnostics'
```

Exit criteria:

- diagnostics print `memory_allocator=vma`;
- allocation names appear in verbose diagnostics;
- per-frame allocation churn is zero for steady-state rendering;
- texture/material work remains blocked until memory diagnostics exist.

Sources to consult:

- VMA overview: https://gpuopen.com/vulkan-memory-allocator/
- VMA reference docs: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/
- CMake FetchContent docs, if FetchContent is considered: https://cmake.org/cmake/help/latest/module/FetchContent.html

## Decision: Generated Shader Artifact Policy

Status: `Proposed`

Recommended default: commit shader source, generate SPIR-V into the build tree, and install/package generated SPIR-V beside the visual app or game executable.

Paths:

```text
shaders/vulkan/src/
build/generated/shaders/vulkan/
```

Install/package path is not final. It should be chosen in `packaging.md`, but the renderer must access it through a renderer config/path resolver, not through runtime/content code.

Rules:

- generated SPIR-V is a build artifact by default;
- generated SPIR-V is committed only if packaging or platform constraints require it;
- if generated SPIR-V is committed, source and generation command must be recorded so reproducibility can be tested;
- runtime/content/projection/save never read shader files;
- shader compile failures fail the build or shader test, not runtime simulation.

Required evidence:

- generated files appear in deterministic path;
- pipeline loader uses configured shader path;
- missing shader error is machine-readable;
- shader compiler version is reported in diagnostics or build logs;
- packaging can find shader artifacts on macOS, Linux, and Windows.

File impact:

- `shaders/vulkan/src/`
- `cmake/iggy3d_shaders.cmake`
- `src/render/vulkan/PipelinesShaders.cpp`
- `src/render/RenderDiagnostics.hpp/.cpp`
- `docs/vulkan/packaging.md`
- `tests/unit/render_shader_policy_tests.cpp`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'shader_policy'
```

Exit criteria:

- shader source exists;
- generated SPIR-V exists after build;
- source-to-SPIR-V command is visible;
- missing shader diagnostics are stable.

## Decision: GPU Smoke Test Policy

Status: `Proposed`

Recommended default: GPU tests are explicit, labeled, and skippable in normal headless development, but required in platform validation lanes.

CTest labels:

- `render`;
- `vulkan`;
- `gpu`;
- `requires_display`;
- `moltenvk`;
- `native_vulkan`;
- `linux`;
- `windows`;
- `macos`;
- `smoke`;
- `slow` when needed.

Normal local behavior:

- headless tests always run;
- Vulkan smoke tests run when enabled by CMake option or test label;
- missing GPU/display/loader returns a configured skip code, not a false failure;
- if the user enables strict Vulkan validation, missing requirements fail.

Recommended CMake/test policy:

- use a CTest `SKIP_RETURN_CODE` for environment skips;
- make skip reason machine-readable in stdout;
- provide `IGGY3D_ENABLE_VULKAN_SMOKE` for building/running smoke tests;
- provide `IGGY3D_REQUIRE_VULKAN_SMOKE` for platform validation lanes.

Required evidence:

- macOS/MoltenVK smoke receipt;
- Linux native Vulkan smoke receipt;
- Windows native Vulkan smoke receipt;
- null renderer replay invariance;
- Vulkan renderer replay invariance where GPU lane is enabled.

File impact:

- `cmake/iggy3d_tests.cmake`
- `tests/smoke/vulkan_device_smoke.cpp`
- `tests/smoke/vulkan_swapchain_smoke.cpp`
- `tests/smoke/vulkan_renderer_smoke.cpp`
- `src/render/RenderDiagnostics.hpp/.cpp`
- `docs/vulkan/diagnostics_and_tests.md`

Test gate:

```sh
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Strict platform gate:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Exit criteria:

- smoke tests skip cleanly only when allowed;
- strict lane fails if Vulkan cannot run;
- diagnostics receipt is printed for every attempted Vulkan smoke;
- headless acceptance remains independent.

## Decision: Validation And Debug Messenger Policy

Status: `Proposed`

Recommended default: enable Vulkan validation in development and smoke tests; enable synchronization validation when available; disable validation by default in release builds.

Why:

- Vulkan does not provide normal error checking for most usage mistakes;
- validation is the only sane early gate for instance/device/swapchain/sync/pipeline work;
- sync validation catches errors that otherwise look like black frames or intermittent driver behavior.

Rules:

- validation layer availability is discovered, not assumed;
- if requested and unavailable, diagnostics must say so;
- strict smoke lanes may fail when validation is unavailable;
- release builds do not ship with validation forced on;
- debug messenger output is routed to renderer diagnostics/logging, not runtime truth.

Required diagnostics fields:

```text
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
debug_messenger=enabled|disabled
validation_strict=true|false
```

File impact:

- `src/render/vulkan/DebugValidation.hpp/.cpp`
- `src/render/vulkan/InstanceDeviceSurface.cpp`
- `src/render/RenderDiagnostics.hpp/.cpp`
- `tests/smoke/vulkan_device_smoke.cpp`
- `tests/smoke/vulkan_sync_smoke.cpp`

Test gate:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_device|vulkan_sync|render_diagnostics'
```

Exit criteria:

- validation status is printed;
- sync validation status is printed;
- validation messages include enough context to identify subsystem;
- validation cannot mutate runtime/replay outcomes.

Sources to consult:

- Vulkan Validation Overview: https://docs.vulkan.org/guide/latest/validation_overview.html
- LunarG Synchronization Validation: https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html

## Decision: RenderDoc Policy

Status: `Deferred`

Recommended default: treat RenderDoc as a secondary manual/debug capture tool after the first visual frame exists.

Why:

- validation and diagnostics come first;
- RenderDoc is valuable once there are real draw calls, resources, and pipelines to inspect;
- automated RenderDoc capture is unnecessary for the first boundary and empty-frame phases.

Rules:

- do not make RenderDoc a build dependency;
- do not make RenderDoc a runtime dependency;
- document capture steps once first-room rendering exists;
- capture artifacts should live under an ignored artifacts path if generated.

File impact:

- `docs/vulkan/diagnostics_and_tests.md`
- optional future `docs/vulkan/renderdoc_capture.md`
- no production code dependency.

Test gate:

- none for first implementation;
- later manual capture checklist after Phase 10.

Exit criteria for reopening:

- first visual room exists;
- diagnostics receipt is insufficient to explain a rendering failure;
- capture instructions are needed for repeatable debugging.

Sources to consult:

- RenderDoc docs: https://renderdoc.org/docs/index.html
- Khronos Simple Engine RenderDoc tooling: https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Tooling/03_debugging_and_renderdoc.html

## Reviewer Checklist

For every renderer file plan, check these decisions:

1. Does the plan name the decision it depends on?
2. Does it use the proposed default or explicitly invoke a fallback?
3. Does the fallback have platform evidence?
4. Does the plan keep runtime/content/projection/save free of Vulkan headers?
5. Does the plan preserve headless acceptance?
6. Does the plan add diagnostics if it touches device, sync, shaders, memory, or platform shell?
7. Does the plan keep macOS/MoltenVK, Linux, and Windows in the validation story?

If not, the file plan is not ready.
