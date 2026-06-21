# Vulkan Fallbacks

This document centralizes fallback policy for the Vulkan renderer path.

Core rule: no silent fallback. If the renderer uses anything other than the proposed default, the fallback must be visible in diagnostics, tied to a reason code, and reflected in the relevant owner document.

## Fallback Requirements

Every fallback must record:

```text
fallback_used=true
fallback_area=
fallback_from=
fallback_to=
fallback_reason=
platform=
platform_lane=
test_name=
```

Rules:

- fallback on one platform does not automatically become the global default;
- fallback must not alter runtime truth;
- fallback must not alter replay hash;
- fallback must not hide missing required platform proof in strict lanes;
- fallback must update `decisions.md` if it affects architecture;
- fallback must update `platform_matrix.md` if it affects platform gates;
- fallback must update `diagnostics_and_tests.md` if it affects pass/fail/skip behavior.

## Fallback Summary

| Area | Proposed default | Allowed fallback | Allowed when | Must update |
| --- | --- | --- | --- | --- |
| Rendering path | Dynamic rendering | Render pass | required platform lane cannot support dynamic rendering cleanly | `decisions.md`, `platform_matrix.md`, `shader_pipeline.md` |
| Sync policy | Binary WSI semaphores/fences first | none for WSI; timeline only later for internal work | timeline support is optional, not required | `decisions.md`, `diagnostics_and_tests.md` |
| Window shell | SDL3 | GLFW | SDL3 acquisition/surface path blocks first proof | `decisions.md`, `platform_shell.md`, `packaging.md` |
| Shader language | GLSL/glslang | Slang or packaged SPIR-V | GLSL toolchain blocks or Slang is proven better across all lanes | `decisions.md`, `shader_pipeline.md`, `packaging.md` |
| Shader artifacts | build-generated SPIR-V | checked-in generated SPIR-V | packaging/reproducibility requires it | `shader_pipeline.md`, `packaging.md` |
| Memory allocator | VMA for growth | documented manual allocator | VMA cannot be used after proof | `decisions.md`, `resource_model.md` |
| First-room room geometry | projected room proxy | renderer debug floor/grid | projection does not yet emit room geometry | `first_room_render_contract.md` |
| GPU smoke | strict platform proof | skip | only optional lane, never strict lane | `diagnostics_and_tests.md`, `platform_matrix.md` |
| Linux CI | native Vulkan hardware | lavapipe/software Vulkan | CI aid only, not shipping proof | `platform_matrix.md`, `packaging.md` |
| Validation | validation enabled in dev/smoke | unavailable/disabled | optional lane only; strict lane fails | `diagnostics_and_tests.md` |
| Sync validation | sync validation enabled when required | unavailable/disabled | optional lane only; strict sync lane fails | `diagnostics_and_tests.md` |

## Rendering Path Fallback

Default:

```text
rendering_path=dynamic
```

Allowed fallback:

```text
rendering_path=render_pass
fallback_area=rendering_path
fallback_from=dynamic
fallback_to=render_pass
```

Allowed only when:

- a required platform lane cannot support dynamic rendering through core feature or extension path;
- validation evidence shows dynamic rendering path is blocked;
- the fallback is isolated inside `src/render/vulkan/**`;
- runtime/projection/public renderer API remains unchanged.

Required diagnostics:

```text
rendering_path=render_pass
fallback_used=true
fallback_reason=
api_version=
enabled_features=
enabled_device_extensions=
```

Not allowed:

- choosing render pass just because old code used it;
- silently compiling both paths without diagnostics;
- exposing render-pass concepts to runtime/projection.

## Sync Policy Fallback

Default first sync path:

```text
sync_policy=binary_wsi
```

Important: timeline semaphores are not the default WSI fallback. Timeline semaphores are deferred for internal GPU work after the binary WSI path is clean.

Allowed fallback:

- none for first WSI path;
- if binary WSI sync fails, the renderer is not ready.

Allowed timeline adoption:

```text
sync_policy=mixed
fallback_used=false
```

Only after:

- binary WSI path passes;
- timeline support is queried;
- internal upload/async work needs it;
- sync validation remains clean.

Strict failure examples:

- missing acquire/present synchronization;
- fence deadlock;
- sync validation error.

## Window Shell Fallback

Default:

```text
window_shell=sdl3
surface_provider=sdl3
```

Allowed fallback:

```text
window_shell=glfw
surface_provider=glfw
fallback_area=window_shell
fallback_from=sdl3
fallback_to=glfw
```

Allowed only when:

- SDL3 cannot be acquired cleanly across target lanes;
- SDL3 Vulkan surface creation blocks the first proof;
- fallback still supports macOS, Linux, and Windows;
- input/event ownership remains app/platform shell owned.

Not allowed:

- native platform code as first fallback unless SDL3 and GLFW both fail;
- exposing GLFW/SDL types through runtime or projection;
- changing runtime command semantics because window shell changed.

Required owner updates:

- `platform_shell.md`;
- `packaging.md`;
- `file_surface.md`;
- `decisions.md`.

## Shader Language Fallback

Default:

```text
shader_language=glsl
shader_compiler=glslangValidator
```

Allowed fallbacks:

```text
shader_language=slang
```

or

```text
shader_language=spirv_only
```

Slang allowed when:

- `slangc` is available on macOS, Linux, and Windows;
- generated SPIR-V loads into the same first-room pipeline interface;
- CMake/package integration is no more fragile than glslang;
- runtime/projection remain shader-language-neutral.

SPIR-V-only allowed when:

- shader compiler is unavailable for an otherwise valid runtime package;
- generated SPIR-V has source/provenance;
- reproducibility test exists or the limitation is documented;
- it is not used to avoid writing shader build rules forever.

Not allowed:

- hand-authored binary-only shader source with no source file;
- runtime/content/projection knowing shader language;
- runtime shader compilation during normal gameplay.

## Shader Artifact Fallback

Default:

```text
shader_artifacts=build_generated
```

Allowed fallback:

```text
shader_artifacts=checked_in_generated
```

Allowed when:

- packaging requires prebuilt SPIR-V;
- shader compiler is not expected on end-user machines;
- source, compiler command, target environment, and generated output are all tracked;
- reproducibility can be checked by developers/CI.

Required diagnostics:

```text
shader_root=
shader_root_exists=true|false
shader_artifacts=build_generated|checked_in_generated
```

## Memory Allocator Fallback

Default for growth lane:

```text
memory_allocator=vma
```

Allowed first-room bootstrap:

```text
memory_allocator=manual_bootstrap
```

Allowed fallback after growth gate:

```text
memory_allocator=manual_documented
```

Allowed only when:

- VMA cannot be used after explicit proof;
- allocator lifetime, memory type selection, mapping, budget diagnostics, and allocation naming are documented;
- texture/material work remains blocked until diagnostics match VMA-level requirements.

Not allowed:

- anonymous allocations;
- per-frame allocation churn;
- texture/material growth without allocation diagnostics;
- VMA types leaking out of Vulkan backend.

## First-Room Geometry Fallback

Default:

```text
fallback_room_proxy=false
```

Allowed fallback:

```text
fallback_room_proxy=true
fallback_area=first_room_geometry
fallback_from=projected_room_geometry
fallback_to=renderer_debug_floor_grid
fallback_reason=no_projected_room_geometry
```

Allowed only when:

- runtime/projection does not yet emit room/floor/wall geometry;
- player marker still renders from projection;
- fallback is visible and diagnosed;
- fallback is temporary and not content truth.

Not allowed:

- renderer reading fixture files to invent gameplay geometry;
- renderer treating fallback floor/grid as save truth;
- passing first-room proof with no projected gameplay item visible.

## GPU Smoke Skip Fallback

Default optional lane behavior:

```text
result=skip
exit_code=77
```

Allowed when:

- `IGGY3D_REQUIRE_VULKAN_SMOKE=OFF`;
- missing display/window server;
- missing Vulkan loader/device;
- missing optional validation layers;
- missing optional shader compiler.

Not allowed when:

- strict Vulkan lane is enabled;
- platform is being counted as shipping proof;
- replay invariance with Vulkan is required.

Strict lane behavior:

```text
result=fail
exit_code=1
```

## Software Vulkan Fallback

Default shipping proof:

```text
platform_lane=native_vulkan
```

Allowed optional CI aid:

```text
platform_lane=software_vulkan
```

Allowed when:

- clearly labeled;
- used for CI coverage only;
- not counted as Linux shipping proof;
- diagnostics identify driver/device as software.

Not allowed:

- replacing hardware/native platform proof;
- claiming the renderer is shippable based only on software Vulkan.

## Validation Fallbacks

Default dev/smoke:

```text
validation=enabled
sync_validation=enabled
```

Allowed optional-lane fallback:

```text
validation=unavailable
sync_validation=unavailable
result=skip
```

Allowed only when validation is not required.

Strict lane:

- missing validation layer fails;
- missing sync validation fails if sync validation is required;
- validation error fails.

Required reason codes:

```text
missing_validation_layers
missing_sync_validation
validation_layer_required_missing
sync_validation_required_missing
validation_error
```

## Required Reason Codes

Use existing reason codes from [diagnostics_and_tests.md](diagnostics_and_tests.md) when possible.

Fallback-specific additions:

```text
dynamic_rendering_unavailable
sdl3_surface_unavailable
glslang_unavailable
slang_selected_after_gate
spirv_only_package
vma_unavailable
no_projected_room_geometry
software_vulkan_ci_lane
```

If a new code is needed, add it to [diagnostics_and_tests.md](diagnostics_and_tests.md) in the same change as the fallback.

## Reviewer Checklist

For any fallback, reviewer must verify:

1. Is the fallback allowed by this document?
2. Is the fallback visible in diagnostics?
3. Is there a stable `fallback_reason` or `reason_code`?
4. Is the fallback isolated to renderer/platform/package code?
5. Does headless acceptance still pass?
6. Does replay hash stay unchanged?
7. Did the owner doc get updated?
8. Is the platform lane still honestly represented?

If any answer is no, the fallback fails review.

## Acceptance Criteria

The fallback policy is satisfied when:

- every fallback path has a diagnostic field;
- strict lanes fail instead of skipping required proof;
- optional lanes skip with receipt and exit code 77;
- runtime/content/projection/save remain fallback-free;
- shippable renderer status still requires macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan proof.
