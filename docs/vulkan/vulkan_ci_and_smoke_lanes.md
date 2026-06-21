# Vulkan CI And Smoke Lanes

This document defines the exact optional and strict Vulkan smoke lanes for `iggy3d`: macOS/MoltenVK, Linux native Vulkan, Windows native Vulkan, optional software Vulkan, skip/fail exit codes, CTest labels, and required receipt fields.

CI and smoke tests are proof infrastructure. They must not change runtime truth, command legality, save/load state, replay determinism, camera truth, package validation, or projection semantics.

## Purpose

Define the renderer proof ladder:

```text
headless_runtime_lane=always_required
optional_vulkan_smoke=may_skip_with_exit_77
strict_vulkan_smoke=must_pass_or_fail_exit_1
macos_moltenvk_lane=required_for_apple_portability
linux_native_vulkan_lane=required_for_shipping
windows_native_vulkan_lane=required_for_shipping
software_vulkan_lane=optional_ci_aid_only
skip_exit_code=77
fail_exit_code=1
pass_exit_code=0
receipt_required_for_attempted_vulkan=true
runtime_hash_must_not_change=true
```

This document narrows:

- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [platform_matrix.md](platform_matrix.md)
- [packaging.md](packaging.md)
- [debug_validation.md](debug_validation.md)
- [vulkan_shader_build_pipeline.md](vulkan_shader_build_pipeline.md)
- [vulkan_device_loss_recovery.md](vulkan_device_loss_recovery.md)
- [boundaries.md](boundaries.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| CTest `SKIP_RETURN_CODE`: https://cmake.org/cmake/help/latest/prop_test/SKIP_RETURN_CODE.html | mapping process exit `77` to skipped tests |
| CTest manual: https://cmake.org/cmake/help/latest/manual/ctest.1.html | labels, output behavior, resource constraints, test execution |
| CMake `add_test`: https://cmake.org/cmake/help/latest/command/add_test.html | registering smoke/unit tests |
| CMake test labels / `ctest_test`: https://cmake.org/cmake/help/latest/command/ctest_test.html | configure-time and runtime label expectations |
| Vulkan Validation Overview: https://docs.vulkan.org/guide/latest/validation_overview.html | validation as development/smoke infrastructure |
| LunarG Synchronization Validation docs: https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html | strict sync validation lane behavior |
| Project platform matrix and diagnostics docs | local platform roles, receipt schema, reason codes |

Priority rule:

```text
ctest_behavior=CMake official docs
vulkan_validation_behavior=Khronos and LunarG docs
lane_ownership=iggy3d platform/diagnostics docs
runtime_truth=iggy3d runtime docs, not CI
```

## Scope

In scope:

- CI lane definitions;
- optional versus strict smoke behavior;
- macOS/MoltenVK lane;
- Linux native Vulkan lane;
- Windows native Vulkan lane;
- optional software Vulkan lane;
- skip/fail/pass exit codes;
- CTest labels and filters;
- CMake options for smoke lanes;
- required receipt fields;
- artifact layout;
- platform proof commands;
- replay invariance requirements.

Out of scope:

- choosing a CI provider;
- writing workflow YAML now;
- implementing CMake tests now;
- GPU runner procurement;
- cloud GPU setup;
- RenderDoc automation;
- shipping package certification;
- replacing hardware lanes with software Vulkan.

## Local File Surface

Likely future files:

```text
CMakeLists.txt
cmake/iggy3d_tests.cmake
cmake/iggy3d_vulkan_smoke.cmake
cmake/iggy3d_ci_presets.cmake
CMake preset file
tests/smoke/vulkan_platform_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_diagnostics_smoke.cpp
tests/smoke/render_replay_invariance_smoke.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
build/artifacts/render_diagnostics/
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| lane selection | CI/CMake/test harness | runtime behavior |
| skip/fail policy | smoke test harness | gameplay semantics |
| receipt schema | `RenderDiagnostics` / smoke test wrapper | save truth |
| platform labels | CTest/test registration | renderer API forks |
| diagnostics artifacts | test harness | source assets |
| software Vulkan proof | optional CI lane | shipping proof |
| replay invariance checks | runtime/test harness | renderer state mutation |

Rules:

- test harness may enable or require renderer proof;
- runtime/content/projection/save must not depend on CI lane names;
- optional skip is allowed only where strict proof is not requested;
- strict platform lanes never hide missing graphics dependencies as pass.

## Lane Summary

| Lane | Required by default | May skip | Strict failure on missing deps | Shipping proof |
| --- | --- | --- | --- | --- |
| headless runtime | yes | no | yes | yes, for runtime only |
| null renderer replay | yes | no | yes | yes, for renderer firewall |
| optional Vulkan smoke | no | yes, exit `77` | no | no |
| macOS/MoltenVK strict | platform validation | no | yes, exit `1` | Apple portability |
| Linux native Vulkan strict | platform validation | no | yes, exit `1` | required |
| Windows native Vulkan strict | platform validation | no | yes, exit `1` | required |
| Linux software Vulkan | no | yes in optional mode | only if explicitly strict | CI aid only |

Hard rule:

```text
software_vulkan_cannot_replace_native_platform_shipping_lanes=true
```

## Exit Codes

Process exit code policy:

```text
0 = pass
1 = fail
77 = skip
```

CTest policy:

```cmake
set_tests_properties(<test> PROPERTIES SKIP_RETURN_CODE 77)
```

Rules:

- optional Vulkan smoke may return `77`;
- strict Vulkan smoke must return `1` for missing required GPU/display/loader/tooling;
- skip must print a receipt with `result=skip`;
- fail must print a receipt when the process reaches diagnostics code;
- pass must print a receipt for Vulkan smoke;
- headless runtime tests should not use `77` for missing Vulkan because they must not require Vulkan.

## CMake Options

Proposed options:

```text
IGGY3D_ENABLE_VISUAL_DEMO
IGGY3D_ENABLE_VULKAN
IGGY3D_ENABLE_VULKAN_SMOKE
IGGY3D_REQUIRE_VULKAN_SMOKE
IGGY3D_ENABLE_SHADER_COMPILE
IGGY3D_REQUIRE_SHADER_COMPILE
IGGY3D_REQUIRE_VALIDATION_LAYERS
IGGY3D_REQUIRE_SYNC_VALIDATION
IGGY3D_ENABLE_SOFTWARE_VULKAN_SMOKE
IGGY3D_REQUIRE_SOFTWARE_VULKAN_SMOKE
IGGY3D_RENDER_DIAGNOSTICS_DIR
```

Rules:

- visual/Vulkan options default off until headless runtime acceptance is green;
- Vulkan smoke tests are built only when Vulkan is enabled;
- strict platform lanes set `IGGY3D_REQUIRE_VULKAN_SMOKE=ON`;
- shader compiler is required only when shader compile is required;
- software Vulkan lane is opt-in and labeled separately;
- disabling Vulkan must still build and test headless runtime.

## CTest Labels

Required labels:

```text
iggy3d
unit
acceptance
render
vulkan
gpu
software_gpu
requires_display
smoke
slow
macos
linux
windows
moltenvk
native_vulkan
software_vulkan
shader
memory
sync
platform
replay
diagnostics
strict
optional
```

Label rules:

- every test has `iggy3d`;
- Vulkan tests have `vulkan`;
- GPU-backed Vulkan tests have `gpu`;
- software Vulkan tests have `software_gpu;software_vulkan`;
- window/swapchain tests have `requires_display`;
- optional smoke tests have `optional`;
- strict lane tests have `strict`;
- macOS/MoltenVK tests have `macos;moltenvk`;
- Linux native tests have `linux;native_vulkan`;
- Windows native tests have `windows;native_vulkan`;
- replay invariance tests have `replay`;
- shader-only policy tests have `shader` but not `gpu` unless Vulkan objects are created.

Recommended filters:

```sh
ctest --test-dir build --output-on-failure -L 'vulkan'
ctest --test-dir build --output-on-failure -L 'vulkan;optional'
ctest --test-dir build --output-on-failure -L 'vulkan;strict'
ctest --test-dir build --output-on-failure -L 'linux;native_vulkan'
ctest --test-dir build --output-on-failure -L 'software_vulkan'
```

## Required Receipt Fields

Every attempted Vulkan smoke must print:

```text
receipt_version=1
repo=iggy3d
backend=vulkan
app=
test_name=
ci_lane=
platform=macos|linux|windows|unknown
platform_lane=moltenvk|native_vulkan|software_vulkan|headless
runner_kind=local|ci|unknown
strict_vulkan=true|false
optional_vulkan=true|false
result=pass|fail|skip
exit_code=0|1|77
reason_code=
message=
```

Required environment/dependency fields:

```text
vulkan_loader=found|missing|unavailable
vulkan_icd=found|missing|unavailable
vulkan_sdk_path=
window_shell=none|sdl3|glfw|native|unavailable
display_available=true|false|unavailable
surface_created=true|false|unavailable
shader_compile_enabled=true|false
shader_compiler=found|missing|not_required|unavailable
shader_root=
shader_root_exists=true|false|unavailable
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
validation_strict=true|false
sync_validation_strict=true|false
```

Required device/proof fields when Vulkan device selection succeeds:

```text
device_name=
device_vendor_id=
device_id=
device_type=
api_version=
driver_version=
selected_physical_device_index=
enabled_instance_extensions=
enabled_device_extensions=
enabled_features=
portability_subset=true|false|unavailable
software_device=true|false|unavailable
```

Required replay/firewall fields when renderer consumes a frame:

```text
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false|unavailable
runtime_state_touched=false
```

Rules:

- `reason_code=none` for pass;
- `reason_code` is required for skip/fail;
- optional missing dependency must say exactly what was missing;
- strict missing dependency is fail, not skip;
- no raw Vulkan handles in public receipt.

## Reason Codes

Skip reason codes:

```text
vulkan_smoke_disabled
visual_demo_disabled
no_display
no_window_server
no_vulkan_loader
no_vulkan_icd
no_vulkan_device
no_surface_support
missing_shader_compiler
missing_shader_artifacts
missing_validation_layers
missing_sync_validation
software_vulkan_unavailable
unsupported_platform_lane
strict_not_required
```

Failure reason codes:

```text
dependency_firewall_failed
unexpected_skip_in_strict_lane
surface_create_failed
instance_create_failed
device_select_failed
device_create_failed
validation_layer_required_missing
sync_validation_required_missing
swapchain_create_failed
swapchain_recreate_failed
command_record_failed
sync_failed
shader_compile_failed
shader_missing
shader_module_create_failed
pipeline_create_failed
allocation_failed
upload_failed
depth_format_unsupported
device_lost
frame_input_invalid
replay_hash_changed
validation_error
receipt_missing
receipt_invalid
unexpected_exception
```

Pass reason code:

```text
none
```

## Headless Runtime Lane

Purpose:

```text
prove_runtime_without_graphics_dependencies
```

Required:

- build runtime/content/projection/save tests;
- run acceptance/headless demo;
- run null renderer replay invariance where renderer boundary exists;
- no SDL, Vulkan loader, Vulkan SDK, MoltenVK, glslang, GPU, or display required.

Commands:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected result:

```text
pass or fail based on runtime correctness
never skip because Vulkan is missing
```

Labels:

```text
iggy3d;acceptance
iggy3d;render;replay
```

## Optional Vulkan Smoke Lane

Purpose:

```text
developer convenience proof when local graphics stack exists
```

Allowed skips:

- missing display/window server;
- missing Vulkan loader;
- missing Vulkan device;
- missing validation layers when validation is optional;
- missing shader compiler when prebuilt shader artifacts are not required;
- unsupported platform lane.

Commands:

```sh
cmake -S . -B build \
  -DIGGY3D_ENABLE_VISUAL_DEMO=ON \
  -DIGGY3D_ENABLE_VULKAN=ON \
  -DIGGY3D_ENABLE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan;optional'
```

Expected behavior:

```text
available_stack -> pass_or_fail_real_errors
missing_optional_stack -> exit_77_with_receipt
```

Rules:

- optional does not mean vague;
- optional skip must name the missing requirement;
- validation errors may fail if the test got far enough to enable validation and strict-clean validation is the test purpose;
- optional smoke cannot be used as shipping proof.

## Strict Vulkan Smoke Lane

Purpose:

```text
platform validation proof
```

Strict requirements:

- Vulkan loader present;
- Vulkan device present;
- display/window path present for surface/swapchain/first-room tests;
- shader compile or shader artifacts present as configured;
- validation layers present when required;
- sync validation present when required;
- receipt printed;
- replay invariance passes where renderer consumes frame input.

Commands:

```sh
cmake -S . -B build \
  -DIGGY3D_ENABLE_VISUAL_DEMO=ON \
  -DIGGY3D_ENABLE_VULKAN=ON \
  -DIGGY3D_ENABLE_SHADER_COMPILE=ON \
  -DIGGY3D_ENABLE_VULKAN_SMOKE=ON \
  -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON \
  -DIGGY3D_REQUIRE_SHADER_COMPILE=ON \
  -DIGGY3D_REQUIRE_VALIDATION_LAYERS=ON \
  -DIGGY3D_REQUIRE_SYNC_VALIDATION=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan;strict'
```

Expected behavior:

```text
missing_required_stack -> exit_1_with_receipt
validation_error -> exit_1_with_receipt
runtime_hash_changed -> exit_1_with_receipt
all_required_proof_green -> exit_0_with_receipt
```

Hard rule:

```text
strict_lane_must_not_return_77=true
```

## macOS/MoltenVK Lane

Role:

```text
first local validation lane and Apple portability proof
```

Labels:

```text
iggy3d;vulkan;gpu;smoke;strict;macos;moltenvk
```

Required strict proof:

- SDL3 creates a Vulkan-capable window;
- platform shell reports required instance extensions;
- portability enumeration/portability subset handling is correct when required;
- MoltenVK-backed device selected and diagnosed;
- surface creation succeeds;
- swapchain creation succeeds;
- validation/sync validation status printed and required when strict options demand it;
- shader build/path works;
- first-room pipeline and draw pass;
- replay invariance passes.

Required lane fields:

```text
platform=macos
platform_lane=moltenvk
moltenvk_runtime=found|missing|unavailable
portability_subset=true|false|unavailable
platform_required_instance_extensions=
device_name=
api_version=
driver_version=
swapchain_format=
present_mode=
shader_root_exists=true|false
```

Optional behavior:

- local optional macOS lane may skip if window server/GPU/SDK is absent;
- strict macOS lane fails on missing required proof.

## Linux Native Vulkan Lane

Role:

```text
required native Vulkan shipping lane
```

Labels:

```text
iggy3d;vulkan;gpu;smoke;strict;linux;native_vulkan
```

Required strict proof:

- native Vulkan loader exists;
- native ICD/device exists;
- selected device is not software unless the lane explicitly says software;
- SDL3 window/surface path works on active WSI backend;
- validation layers available when required;
- sync validation available when required;
- shader compile/path works;
- memory smoke passes;
- first-room smoke passes;
- replay invariance passes.

Required lane fields:

```text
platform=linux
platform_lane=native_vulkan
vulkan_loader=found|missing
vulkan_icd=found|missing
linux_wsi=x11|wayland|headless|unknown
software_device=false
device_name=
device_type=
driver_version=
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
```

Rules:

- Linux native lane is not satisfied by lavapipe/software Vulkan;
- lavapipe can run in the software lane only;
- X11 and Wayland differences are diagnostics, not runtime architecture forks.

## Windows Native Vulkan Lane

Role:

```text
required native Vulkan shipping lane
```

Labels:

```text
iggy3d;vulkan;gpu;smoke;strict;windows;native_vulkan
```

Required strict proof:

- native Vulkan runtime/loader exists;
- native GPU/device selected;
- SDL3 runtime/DLL lookup works;
- Win32 surface path works through platform shell;
- shader compiler discovery works;
- Debug and Release shader roots do not collide in multi-config builds;
- validation layers available when required;
- shader/pipeline smoke passes;
- first-room smoke passes;
- replay invariance passes.

Required lane fields:

```text
platform=windows
platform_lane=native_vulkan
vulkan_loader=found|missing
sdl3_runtime=linked_static|linked_dynamic|missing|unavailable
shader_config=Debug|Release|RelWithDebInfo|MinSizeRel|default
shader_root=
shader_root_exists=true|false
multi_config_generator=true|false
software_device=false
device_name=
driver_version=
```

Multi-config commands:

```sh
cmake -S . -B build-vs -G "Visual Studio 17 2022" \
  -DIGGY3D_ENABLE_VISUAL_DEMO=ON \
  -DIGGY3D_ENABLE_VULKAN=ON \
  -DIGGY3D_ENABLE_VULKAN_SMOKE=ON \
  -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build-vs --config Debug
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Debug --output-on-failure -L 'windows;native_vulkan'
ctest --test-dir build-vs -C Release --output-on-failure -L 'windows;native_vulkan'
```

## Software Vulkan Lane

Role:

```text
optional CI aid for Linux-style environments
```

Expected implementation:

```text
software_vulkan_candidate=lavapipe_or_equivalent
platform=linux
platform_lane=software_vulkan
software_device=true
```

Labels:

```text
iggy3d;vulkan;software_gpu;smoke;optional;linux;software_vulkan
```

Allowed use:

- exercise Vulkan instance/device/resource code in CI without hardware GPU;
- catch basic validation, shader, resource, and command recording errors;
- provide early smoke signal before native hardware lanes are available.

Forbidden use:

- shipping renderer proof;
- replacement for Linux native Vulkan lane;
- replacement for Windows native Vulkan lane;
- replacement for macOS/MoltenVK lane;
- performance evidence;
- platform-specific WSI proof unless a real display/surface path is also proven.

Optional software lane behavior:

```text
software_vulkan_missing -> exit_77
software_vulkan_present_and_smoke_passes -> exit_0
software_vulkan_present_and_real_error -> exit_1
```

Strict software lane is allowed only for CI experiments:

```text
IGGY3D_REQUIRE_SOFTWARE_VULKAN_SMOKE=ON
software_vulkan_missing -> exit_1
```

Required software receipt fields:

```text
platform_lane=software_vulkan
software_device=true
device_name=
device_vendor_id=
device_type=
driver_version=
software_vulkan_driver=lavapipe|swiftshader|unknown
shipping_proof=false
```

## Test Inventory

Expected future renderer tests:

| Test | Labels | Optional lane | Strict lane |
| --- | --- | --- | --- |
| `render_boundary_tests` | `iggy3d;unit;render` | required in normal tests | required |
| `render_replay_invariance_tests` | `iggy3d;render;replay` | required with null renderer | required with Vulkan when enabled |
| `render_shader_policy_tests` | `iggy3d;unit;render;shader` | when shader compile enabled | required |
| `vulkan_platform_smoke` | `iggy3d;vulkan;gpu;requires_display;smoke;platform` | may skip | required |
| `vulkan_device_smoke` | `iggy3d;vulkan;gpu;smoke;diagnostics` | may skip | required |
| `vulkan_swapchain_smoke` | `iggy3d;vulkan;gpu;requires_display;smoke` | may skip | required |
| `vulkan_sync_smoke` | `iggy3d;vulkan;gpu;smoke;sync` | may skip | required when strict sync |
| `vulkan_pipeline_smoke` | `iggy3d;vulkan;gpu;smoke;shader` | may skip | required |
| `vulkan_memory_smoke` | `iggy3d;vulkan;gpu;smoke;memory` | may skip | required after VMA/resource phase |
| `vulkan_first_room_smoke` | `iggy3d;vulkan;gpu;requires_display;smoke` | may skip | required after first-room phase |
| `vulkan_diagnostics_smoke` | `iggy3d;vulkan;gpu;smoke;diagnostics` | may skip | required |
| `vulkan_software_device_smoke` | `iggy3d;vulkan;software_gpu;software_vulkan;smoke` | may skip | only if explicitly required |

## Artifact Policy

Default artifact root:

```text
build/artifacts/render_diagnostics/
```

Per-lane artifact root:

```text
build/artifacts/render_diagnostics/<ci_lane>/<test_name>/
```

Expected files:

```text
receipt.txt
stdout.txt
stderr.txt
validation.log
shader_compile.log
```

Optional files:

```text
screenshot.ppm
frame_hash.txt
device_fault.txt
renderdoc_capture.rdc
gfxreconstruct_capture.gfxr
```

Rules:

- `receipt.txt` is required when diagnostics code is reached;
- CI should upload artifacts only from failed or explicitly requested runs;
- capture files are never committed by default;
- receipts must not expose raw Vulkan handles;
- artifacts are diagnostics, not replay input or save data.

## CI Matrix Policy

Provider-neutral required jobs:

| Job | Required now | Purpose |
| --- | --- | --- |
| `headless-runtime` | yes | runtime/content/projection/save proof without graphics |
| `render-boundary` | yes after renderer boundary exists | no Vulkan leakage and null renderer replay invariance |
| `vulkan-macos-moltenvk-strict` | platform validation | Apple portability proof |
| `vulkan-linux-native-strict` | platform validation | required shipping native Vulkan proof |
| `vulkan-windows-native-strict` | platform validation | required shipping native Vulkan proof |
| `vulkan-linux-software-optional` | optional | early CI aid |

Rules:

- headless/runtime job is the first mandatory job;
- strict Vulkan jobs become required only when that implementation phase exists;
- optional software lane may run before hardware lanes but cannot satisfy shipping gates;
- if a strict Vulkan job is configured, missing infrastructure is a failure;
- if a lane is not implemented yet, do not register it as required CI.

## Acceptance Gate

This policy is ready for implementation planning when:

- exit codes `0`, `1`, and `77` are accepted;
- CTest `SKIP_RETURN_CODE=77` policy is accepted;
- optional and strict lane behavior is unambiguous;
- macOS/MoltenVK, Linux native, and Windows native lanes are all explicit;
- software Vulkan is clearly optional and non-shipping;
- receipt fields and reason codes are accepted;
- artifact paths are accepted;
- headless runtime independence is preserved;
- CI provider remains an implementation detail until workflow files are planned.
