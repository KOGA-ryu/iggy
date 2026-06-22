# Vulkan Diagnostics And Tests

This document defines renderer receipts, smoke tests, skip behavior, strict validation lanes, and failure artifacts.

The diagnostics contract is intentionally boring: machine-readable key-value lines, stable field names, and enough context to trust the renderer without reading a debugger transcript.

Current baseline note: diagnostics now cover backend-neutral render API tests, concrete `NullRenderer` tests, Vulkan-private smokes, first-room proof surfaces, and runtime/replay invariance with Runtime Packet 8 combat state present. Renderer receipts remain diagnostics only; they are not save, replay, command, package, or gameplay truth.

## Goals

Diagnostics and tests must prove:

- the renderer stayed behind the runtime/projection boundary;
- the selected platform lane is real;
- Vulkan device/surface/swapchain/sync/pipeline/resource setup is observable;
- missing GPU/display/tooling can skip only when skips are allowed;
- strict platform lanes fail when Vulkan proof is required;
- renderer presence never changes replay hash or runtime summary.

## Non-Goals

Diagnostics and tests must not:

- become gameplay truth;
- affect save/load or replay;
- expose raw Vulkan handles through public renderer API;
- require a GPU for normal headless runtime acceptance;
- require RenderDoc for automated success;
- hide platform failures behind vague logs.

## Receipt Format

Default format: UTF-8 text, one `key=value` pair per line.

Rules:

- one field per line;
- keys are lowercase snake_case;
- values are single-line strings;
- missing optional data uses `unavailable`;
- booleans use `true` or `false`;
- enabled/disabled state uses exact values listed by each field;
- lists use comma-separated values with no spaces;
- diagnostics may be printed to stdout for smoke tests;
- visual demo can print diagnostics to stdout only when requested, otherwise stderr or artifact file is preferred.

Reason for key-value text:

- easy to print from C++;
- easy for shell/CTest to inspect;
- stable without structured-output dependencies;
- less brittle than prose.

Key-value receipt text is the renderer compatibility format for the current
build; any later structured format must be a separate explicit renderer tooling
contract and cannot replace the smoke-test receipt.

## Receipt Sections

A complete Vulkan smoke receipt should be conceptually grouped by section. The actual output remains flat key-value lines.

### Common Fields

Required for every renderer receipt:

```text
receipt_version=1
repo=iggy3d
backend=null|vulkan
app=
test_name=
platform=macos|linux|windows|unknown
platform_lane=moltenvk|native_vulkan|headless
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
message=
```

Rules:

- `reason_code` is required for `fail` and `skip`;
- `message` is diagnostic text, not a parser contract;
- `backend=null` receipts are allowed for replay invariance tests.

### Boundary Fields

Required when a render frame is consumed:

```text
frame_input_valid=true|false|unavailable
source_tick=
frame_index=
scene_item_count=
debug_item_count=
draw_count=
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false|unavailable
```

Rules:

- runtime hash fields are required only for replay invariance tests;
- renderer diagnostics may differ by backend/platform;
- runtime hash must not differ because renderer ran.

### Platform Shell Fields

Required for platform/surface smoke:

```text
window_shell=none|sdl3|glfw|native|unavailable
window_width=
window_height=
framebuffer_width=
framebuffer_height=
drawable=true|false
surface_provider=none|sdl3|glfw|native|unavailable
platform_required_instance_extensions=
surface_created=true|false|unavailable
resize_count=
minimize_count=
focus_state=focused|unfocused|unavailable
```

### Device Fields

Required for device smoke:

```text
device_name=
device_vendor_id=
device_id=
device_type=
api_version=
driver_version=
selected_physical_device_index=
graphics_queue_family=
present_queue_family=
compute_queue_family=
transfer_queue_family=
enabled_instance_extensions=
enabled_device_extensions=
enabled_features=
portability_subset=true|false|unavailable
```

### Validation Fields

Required when Vulkan backend is attempted:

```text
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
debug_messenger=enabled|disabled|unavailable
validation_strict=true|false
validation_error_count=
validation_warning_count=
first_validation_error=
```

Rules:

- strict lanes fail if validation was required but unavailable;
- `first_validation_error` uses `none` when there is no error;
- validation messages must not mutate runtime.

### Swapchain Fields

Required after swapchain creation:

```text
swapchain_created=true|false|unavailable
swapchain_format=
swapchain_color_space=
present_mode=
swapchain_extent=
swapchain_image_count=
frames_in_flight=
resize_recreate_count=
```

### Synchronization Fields

Required after frame sync setup:

```text
sync_policy=binary_wsi|timeline|mixed|unavailable
frames_in_flight=
acquire_semaphore_count=
render_finished_semaphore_count=
in_flight_fence_count=
sync_validation_clean=true|false|unavailable
```

### Shader/Pipeline Fields

Required after pipeline creation:

```text
shader_language=glsl|slang|spirv_only|unavailable
shader_compiler=
shader_target_env=
shader_root=
vertex_shader=
fragment_shader=
pipeline_family=
pipeline_layout=
rendering_path=dynamic|render_pass|unavailable
depth_test=enabled|disabled|unavailable
cull_mode=none|front|back|front_and_back|unavailable
front_face=clockwise|counter_clockwise|unavailable
```

### Resource Fields

Required after resource setup:

```text
memory_allocator=none|manual_bootstrap|vma
vma_version=
allocation_count=
buffer_allocation_count=
image_allocation_count=
allocated_bytes=
budget_bytes=
used_budget_bytes=
per_frame_allocation_count=
upload_bytes_this_frame=
upload_bytes_total=
depth_format=
depth_extent=
vertex_buffer_count=
index_buffer_count=
texture_count=
sampler_count=
descriptor_set_layout_count=
descriptor_pool_count=
material_count=
missing_resource_count=
```

## Minimal Receipt Examples

### Skip Example

```text
receipt_version=1
repo=iggy3d
backend=vulkan
app=vulkan_device_smoke
test_name=vulkan_device_smoke
platform=linux
platform_lane=native_vulkan
strict_vulkan=false
result=skip
reason_code=no_vulkan_loader
message=Vulkan loader was not found and strict Vulkan smoke is disabled.
```

### Failure Example

```text
receipt_version=1
repo=iggy3d
backend=vulkan
app=vulkan_pipeline_smoke
test_name=vulkan_pipeline_smoke
platform=windows
platform_lane=native_vulkan
strict_vulkan=true
result=fail
reason_code=pipeline_create_failed
message=First-room graphics pipeline creation failed.
shader_language=glsl
rendering_path=dynamic
first_validation_error=none
```

### Pass Example

```text
receipt_version=1
repo=iggy3d
backend=vulkan
app=vulkan_renderer_smoke
test_name=vulkan_renderer_smoke
platform=macos
platform_lane=moltenvk
strict_vulkan=true
result=pass
reason_code=none
message=Renderer smoke passed.
window_shell=sdl3
device_name=Apple GPU via MoltenVK
api_version=
validation=enabled
sync_validation=enabled
swapchain_format=
present_mode=
frames_in_flight=2
shader_language=glsl
memory_allocator=manual_bootstrap
draw_count=1
replay_invariant=true
```

## Reason Codes

Use stable reason codes. Add new codes only when the old code would be misleading.

Skip reason codes:

```text
vulkan_smoke_disabled
no_display
no_window_server
no_vulkan_loader
no_vulkan_device
missing_validation_layers
missing_sync_validation
missing_shader_compiler
unsupported_platform_lane
strict_not_required
```

Failure reason codes:

```text
dependency_firewall_failed
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
frame_input_invalid
replay_hash_changed
validation_error
unexpected_exception
```

Pass reason code:

```text
none
```

## Exit Codes

Recommended process exit codes:

```text
0 = pass
1 = fail
77 = skip
```

Rules:

- CTest should treat `77` as skip through `SKIP_RETURN_CODE`;
- strict Vulkan lanes must not return skip for required Vulkan proof;
- skip must still print a receipt;
- failure must print a receipt when process startup reached diagnostics code.

## CTest Labels

Use exact labels where applicable:

```text
iggy3d
unit
acceptance
render
vulkan
gpu
requires_display
smoke
slow
macos
linux
windows
moltenvk
native_vulkan
shader
memory
sync
platform
replay
diagnostics
```

Label rules:

- every test has `iggy3d`;
- GPU Vulkan tests have `vulkan;gpu;smoke`;
- tests that create a window have `requires_display`;
- macOS/MoltenVK-specific lanes add `macos;moltenvk`;
- Linux native Vulkan lanes add `linux;native_vulkan`;
- Windows native Vulkan lanes add `windows;native_vulkan`;
- shader-only tests add `shader` but not `gpu` unless they create Vulkan objects;
- memory policy unit tests add `memory` but not `gpu` unless they create Vulkan resources;
- replay invariance tests add `replay`.

## Test Inventory

Expected future renderer tests:

| Test | Kind | Labels | Required by default | Strict lane |
| --- | --- | --- | --- | --- |
| `render_boundary_tests` | unit | `iggy3d;unit;render` | yes | yes |
| `render_projection_input_tests` | unit | `iggy3d;unit;render` | yes | yes |
| `render_replay_invariance_tests` | unit/acceptance | `iggy3d;render;replay` | yes with null renderer | yes with Vulkan if enabled |
| `render_shader_policy_tests` | unit | `iggy3d;unit;render;shader` | when shader compile enabled | yes |
| `render_memory_policy_tests` | unit | `iggy3d;unit;render;memory` | when render docs become file plans | yes |
| `vulkan_platform_smoke` | smoke | `iggy3d;vulkan;gpu;requires_display;smoke;platform` | no | yes |
| `vulkan_device_smoke` | smoke | `iggy3d;vulkan;gpu;smoke;diagnostics` | no | yes |
| `vulkan_swapchain_smoke` | smoke | `iggy3d;vulkan;gpu;requires_display;smoke` | no | yes |
| `vulkan_sync_smoke` | smoke | `iggy3d;vulkan;gpu;smoke;sync` | no | yes |
| `vulkan_pipeline_smoke` | smoke | `iggy3d;vulkan;gpu;smoke;shader` | no | yes |
| `vulkan_memory_smoke` | smoke | `iggy3d;vulkan;gpu;smoke;memory` | no | yes |
| `vulkan_first_room_smoke` | smoke | `iggy3d;vulkan;gpu;requires_display;smoke` | no | yes |
| `vulkan_diagnostics_smoke` | smoke | `iggy3d;vulkan;gpu;smoke;diagnostics` | no | yes |

## CMake Options

Renderer test options belong to the future renderer-enabled build pass, not the foundation build pass.

Proposed options:

```text
IGGY3D_ENABLE_VISUAL_DEMO
IGGY3D_ENABLE_VULKAN
IGGY3D_ENABLE_VULKAN_SMOKE
IGGY3D_REQUIRE_VULKAN_SMOKE
IGGY3D_ENABLE_SHADER_COMPILE
IGGY3D_REQUIRE_VALIDATION_LAYERS
IGGY3D_REQUIRE_SYNC_VALIDATION
IGGY3D_RENDER_DIAGNOSTICS_DIR
```

Rules:

- headless runtime tests do not require these options;
- Vulkan smoke tests are built only when Vulkan is enabled;
- strict platform validation sets `IGGY3D_REQUIRE_VULKAN_SMOKE=ON`;
- strict validation lanes may set validation requirements on;
- diagnostics dir defaults to `build/artifacts/render_diagnostics`.

## Skip Policy

Optional Vulkan lane:

- missing GPU/display/loader/tooling returns exit code `77`;
- receipt prints `result=skip`;
- receipt prints stable `reason_code`;
- CTest marks the test skipped.

Strict Vulkan lane:

- missing GPU/display/loader/tooling returns exit code `1`;
- receipt prints `result=fail`;
- receipt prints stable `reason_code`;
- CTest marks the test failed.

Validation layers:

- if validation is optional, missing validation prints `validation=unavailable` and may skip validation-specific smoke;
- if validation is required, missing validation fails;
- sync validation follows the same pattern.

Shader compiler:

- if shader compilation is disabled and generated artifacts exist, shader smoke may run;
- if shader compilation is required and compiler is missing, shader policy test fails;
- missing compiler must never affect headless runtime tests.

## Required Commands

Default headless/runtime lane:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional Vulkan smoke lane:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Strict Vulkan validation lane:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VALIDATION_LAYERS=ON -DIGGY3D_REQUIRE_SYNC_VALIDATION=ON
cmake --build build
ctest --test-dir build --output-on-failure -L 'vulkan'
```

Replay invariance lane:

```sh
ctest --test-dir build --output-on-failure -R 'render_replay|replay_state_hash'
```

## Platform Lanes

### macOS/MoltenVK

Role: first local validation lane.

Current readiness probe: before any swapchain or first-room Vulkan rendering is
attempted, `macos_vulkan_dependency_probe` records whether the host can support
future macOS visual work. It is diagnostics only and must not create renderer
runtime truth, save truth, replay truth, screenshots, frame hashes, or gameplay
visual state.

Required probe receipt fields:

```text
platform=macos
platform_lane=moltenvk
sdl3_target_available=true|false
sdl3_source=system|missing|disabled
vulkan_loader_found=true|false
vulkan_sdk_root=<absolute-path-or-empty>
vulkan_sdk_source=environment|default_path|cmake_discovery|not_found
vulkan_icd_path=<absolute-path-or-empty>
vulkan_icd_found=true|false
moltenvk_available=true|false|unavailable
glslc_path=<absolute-path-or-empty>
glslc_found=true|false
validation_layer_found=true|false|unavailable
sync_validation_available=true|false|unavailable
portability_enumeration_available=true|false|unavailable
portability_enumeration_required=true|false|unavailable
portability_subset_exposed=true|false|unavailable
strict_lane=true|false
result=pass|skip|fail
reason_code=<stable-lower-snake-case>
```

Optional probes may exit `77` with a receipt. Strict probes must fail nonzero and
must not return skip when required SDL3, Vulkan loader, MoltenVK/ICD, or
portability-enumeration support is missing.

Required proof:

- SDL3 window/surface path;
- MoltenVK device diagnostics;
- swapchain smoke;
- validation/sync validation where available;
- shader/pipeline smoke;
- first-room smoke;
- replay invariance.

Labels:

```text
macos;moltenvk
```

### Linux Native Vulkan

Role: required shipping lane.

Required proof:

- native Vulkan loader/ICD;
- SDL3 window/surface path;
- validation/sync validation;
- shader/pipeline smoke;
- memory smoke;
- first-room smoke;
- replay invariance.

Labels:

```text
linux;native_vulkan
```

Optional software Vulkan lane:

- may be added later for CI with lavapipe;
- must be labeled explicitly;
- must not replace hardware/native validation for shipping.

### Windows Native Vulkan

Role: required shipping lane.

Required proof:

- native Vulkan runtime/SDK;
- SDL3 window/surface path;
- shader compiler discovery in multi-config build;
- validation/sync validation;
- shader/pipeline smoke;
- first-room smoke;
- replay invariance.

Labels:

```text
windows;native_vulkan
```

## Failure Artifacts

Default artifact root:

```text
build/artifacts/render_diagnostics/
```

Per-test artifact folder:

```text
build/artifacts/render_diagnostics/<test_name>/
```

Expected files:

```text
receipt.txt
validation.log
shader_compile.log
stdout.txt
stderr.txt
```

Optional files:

```text
screenshot.ppm
frame_hash.txt
renderdoc_capture.rdc
```

Rules:

- `receipt.txt` should be written whenever possible;
- validation messages go to `validation.log` when validation is enabled;
- shader compiler output goes to `shader_compile.log`;
- screenshots/frame hashes are optional until first-room visual acceptance defines them;
- RenderDoc captures are manual/deferred and never required by default.

## Diagnostics Ownership

`RenderDiagnostics` owns backend-neutral receipt data.

Rules:

- public diagnostics cannot expose raw Vulkan handles;
- Vulkan backend may add private fields before formatting;
- app/test code can print/save receipts;
- diagnostics do not affect runtime state;
- diagnostics do not affect replay hash;
- diagnostics cannot become save/load truth.

## Acceptance Criteria

This diagnostics/test contract is ready for file plans when:

- key-value receipt format is accepted;
- required fields are named;
- reason codes are named;
- exit codes are named;
- CTest labels are named;
- test inventory is named;
- optional vs strict Vulkan behavior is defined;
- platform lanes are defined;
- artifact paths are defined;
- headless runtime lane remains independent.

## Open Detail Items

These belong in future file plans or platform docs:

- exact CTest `SKIP_RETURN_CODE` implementation;
- exact C++ receipt formatting helper;
- exact validation callback routing;
- exact shader compiler log capture;
- exact screenshot/frame hash policy;
- exact lavapipe/software Vulkan lane if chosen;
- exact CI provider matrix.
