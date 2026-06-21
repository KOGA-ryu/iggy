# Vulkan Renderer Config

This document defines exact startup configuration for the future `iggy3d` renderer path: backend selection, shader root, validation mode, sync validation mode, present-mode request, debug labels, diagnostics directory, strict mode, and platform lane behavior.

Renderer config is app/renderer startup policy. It must not become runtime truth, gameplay command legality, camera truth, save/load state, replay determinism, package validation, or projection semantics.

## Purpose

Define one boring config contract before implementation:

```text
renderer_config_scope=app_and_renderer_startup_only
runtime_receives_renderer_config=false
backend_selection=null_or_vulkan
shader_root=renderer_only_path
validation_mode=off_optional_required
sync_validation_mode=off_optional_required
present_mode_request=fifo_mailbox_immediate_or_auto
debug_labels=off_optional_required
diagnostics_dir=artifact_output_only
strict_mode=test_app_policy_not_runtime_truth
```

This document narrows:

- [boundaries.md](boundaries.md)
- [platform_shell.md](platform_shell.md)
- [debug_validation.md](debug_validation.md)
- [swapchain_contract.md](swapchain_contract.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)
- [vulkan_debug_labels_and_capture.md](vulkan_debug_labels_and_capture.md)
- [shader_pipeline.md](shader_pipeline.md)
- [vulkan_shader_build_pipeline.md](vulkan_shader_build_pipeline.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [packaging.md](packaging.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan `VkPresentModeKHR` reference: https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html | exact present-mode names and FIFO baseline behavior |
| Vulkan Validation Overview: https://docs.vulkan.org/guide/latest/validation_overview.html | validation as development/smoke infrastructure |
| LunarG Synchronization Validation docs: https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html | sync validation request/require policy |
| Vulkan Specification debugging chapter: https://docs.vulkan.org/spec/latest/chapters/debugging.html | debug utils and labels as diagnostics-only Vulkan features |
| CTest `SKIP_RETURN_CODE`: https://cmake.org/cmake/help/latest/prop_test/SKIP_RETURN_CODE.html | strict/optional smoke exit behavior at test boundary |
| Project Vulkan docs listed above | local ownership, firewall, diagnostics fields, shader path, and platform lane rules |

Priority rule:

```text
present_mode_truth=Vulkan Specification and reference pages
validation_truth=Khronos and LunarG validation docs
process_exit_truth=CTest docs
project_config_authority=iggy3d renderer boundary docs
runtime_truth=iggy3d runtime docs, never renderer config
```

## Scope

In scope:

- renderer backend selection;
- Vulkan enable/disable mode;
- shader source/generated artifact roots;
- validation layer mode;
- sync validation mode;
- present-mode request and fallback policy;
- debug labels/debug utils request;
- diagnostics output directory;
- strict mode and optional mode;
- config source precedence;
- config receipt fields;
- runtime firewall rules;
- smoke-test config examples.

Out of scope:

- implementing CLI parsing now;
- selecting a general config library;
- gameplay difficulty/options;
- input binding config;
- save-game config;
- replay metadata;
- package validation policy;
- material authoring config;
- dynamic in-game renderer settings UI.

## Local File Surface

Likely future files:

```text
src/render/RendererConfig.hpp
src/render/RendererConfig.cpp
src/render/RendererApi.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/vulkan/VulkanBackend.hpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
apps/iggy3d_visual_demo/main.cpp
tests/unit/render_config_tests.cpp
tests/unit/render_config_firewall_tests.cpp
tests/smoke/vulkan_config_smoke.cpp
tests/smoke/vulkan_validation_smoke.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| CLI/env/config parsing | app startup or renderer config helper | runtime state |
| renderer backend choice | app/renderer factory | gameplay behavior |
| shader root | renderer config and shader module loader | content package truth |
| validation mode | renderer config and `DebugValidation` | replay result |
| sync validation mode | renderer config and `DebugValidation` | command legality |
| present-mode request | renderer config and `Swapchain` | runtime tick determinism |
| debug labels | renderer config and debug label modules | gameplay debug semantics |
| diagnostics directory | diagnostics module/app harness | source assets or save files |
| strict mode | app/test harness | runtime truth |

Rules:

- runtime/content/projection/save code must not depend on renderer config;
- renderer config may change diagnostics and graphics behavior only;
- renderer config must not alter runtime tick rules, runtime hash, command legality, or replay output;
- strict mode is a proof setting, not a gameplay setting;
- backend-neutral renderer config may exist under `src/render/**`, but Vulkan-specific values must remain in Vulkan-owned types or translated enum values.

## Config Sources And Precedence

Recommended first precedence:

```text
compiled_defaults
environment_variables
cli_flags
test_harness_overrides
```

Rules:

- CLI flags override environment variables;
- test harness overrides are explicit and local to tests;
- config files are deferred until renderer startup is stable;
- no renderer config source is serialized into saves or replay truth;
- receipts must print the final resolved config, not only the requested flags;
- invalid config should fail before Vulkan object creation when possible.

Deferred config file:

```text
config/render.local.toml optional_later
```

Rules for a future config file:

- local developer convenience only;
- not part of package acceptance;
- not replay input;
- not required for CI;
- must be ignored by headless runtime tools unless explicitly running a visual renderer app.

## Required Config Fields

Future `RendererConfig` shape should contain at least:

```text
backend
enable_vulkan
strict_mode
validation_mode
sync_validation_mode
debug_labels_mode
present_mode_request
shader_source_root
shader_binary_root
diagnostics_dir
diagnostics_stdout
platform_lane
allow_software_vulkan
require_discrete_gpu
preferred_device_index
max_frames_in_flight
enable_capture_labels
```

Rules:

- use typed enums in C++ for modes;
- convert text to enums once at app/config boundary;
- do not pass raw strings through the renderer;
- do not expose `VkPresentModeKHR` outside Vulkan-owned code;
- required paths must be normalized before use and printed in receipts.

## Backend Selection

Allowed backend values:

```text
null
vulkan
auto
```

Recommended defaults:

| Context | Default backend | Notes |
| --- | --- | --- |
| headless runtime tests | `null` or no renderer | Vulkan must not be required |
| replay invariance tests | `null` first, optional Vulkan comparison later | renderer must not alter runtime hash |
| visual demo | `vulkan` | fail or skip by strictness if unavailable |
| optional Vulkan smoke | `vulkan` requested, may skip unsupported env | exit `77` at test boundary |
| strict Vulkan smoke | `vulkan` required | exit `1` if unavailable |

Rules:

- `auto` may select `null` only for non-visual tools;
- `auto` must not silently select `null` for strict visual smoke;
- visual demo must print the selected backend;
- runtime code must not branch on selected backend;
- projection output must be identical regardless of backend.

Suggested CLI:

```text
--renderer=null|vulkan|auto
--require-renderer
--no-renderer
```

Conflict policy:

| Flags | Result |
| --- | --- |
| `--renderer=vulkan --no-renderer` | fail config parse |
| `--renderer=null --require-renderer` | fail config parse for visual app |
| `--renderer=auto --require-renderer` | must resolve to non-null backend or fail/skip by lane |

## Vulkan Enable Mode

Vulkan enablement is distinct from backend selection:

```text
IGGY3D_ENABLE_VULKAN=ON|OFF build capability
--renderer=vulkan runtime request
```

Rules:

- if the build has no Vulkan support and app requests Vulkan, optional smoke may skip and strict smoke must fail;
- if build has Vulkan support but runtime request is `null`, Vulkan must not initialize;
- headless tools should build and run when Vulkan is disabled;
- renderer config should print both build capability and runtime request.

Receipt fields:

```text
vulkan_build_enabled=true|false
renderer_backend_requested=null|vulkan|auto
renderer_backend_selected=null|vulkan
```

## Strict Mode

Allowed strict values:

```text
strict_mode=false
strict_mode=true
```

Strict mode means proof requirements are mandatory. It does not mean gameplay rules change.

Strict mode controls:

- whether missing Vulkan support is skip or fail;
- whether missing validation layers fail;
- whether missing sync validation fails;
- whether missing shader compiler/generated SPIR-V fails;
- whether missing debug labels fail in debug-label smoke;
- whether suboptimal but successful present is reported only or escalated by a specific test.

Strict mode must not control:

- runtime tick rate;
- camera mode truth;
- command legality;
- save/load behavior;
- replay hash;
- projection contents.

Suggested CLI:

```text
--vulkan-strict
--vulkan-optional
```

Receipt fields:

```text
strict_vulkan=true|false
optional_vulkan=true|false
```

## Validation Mode

Allowed values:

```text
off
optional
required
```

Recommended defaults:

| Context | Validation mode |
| --- | --- |
| local visual dev | `optional` |
| optional Vulkan smoke | `optional` |
| strict validation smoke | `required` |
| release package | `off` unless debug build explicitly requests it |
| headless runtime tests | not applicable |

Rules:

- `required` fails startup if `VK_LAYER_KHRONOS_validation` is unavailable;
- `optional` enables validation when available and reports unavailable otherwise;
- `off` must not request validation layer or debug messenger for callback use;
- validation messages are diagnostics only;
- validation availability must be printed in receipts.

Suggested CLI/env:

```text
--vulkan-validation=off|optional|required
IGGY3D_VULKAN_VALIDATION=off|optional|required
```

Receipt fields:

```text
validation_requested=off|optional|required
validation=enabled|disabled|unavailable
validation_layer_name=VK_LAYER_KHRONOS_validation
validation_error_count=
validation_warning_count=
```

## Sync Validation Mode

Allowed values:

```text
off
optional
required
```

Rules:

- sync validation requires validation layer availability;
- `required` fails if sync validation cannot be enabled;
- `optional` enables sync validation when available and reports unavailable otherwise;
- sync validation errors fail strict sync lanes;
- sync validation must never be required by headless runtime tests.

Suggested CLI/env:

```text
--vulkan-sync-validation=off|optional|required
IGGY3D_VULKAN_SYNC_VALIDATION=off|optional|required
```

Receipt fields:

```text
sync_validation_requested=off|optional|required
sync_validation=enabled|disabled|unavailable
sync_validation_error_count=
```

## Present Mode Request

Allowed text values:

```text
fifo
mailbox
immediate
auto
```

Vulkan-private mapping:

| Config value | Vulkan value | Use |
| --- | --- | --- |
| `fifo` | `VK_PRESENT_MODE_FIFO_KHR` | default cross-platform baseline |
| `mailbox` | `VK_PRESENT_MODE_MAILBOX_KHR` | optional low-latency request when available |
| `immediate` | `VK_PRESENT_MODE_IMMEDIATE_KHR` | explicit testing only, may tear |
| `auto` | resolve policy default | first policy resolves to FIFO |

Recommended default:

```text
present_mode_request=fifo
```

Rules:

- FIFO is the first default because it is the safest WSI baseline;
- requested present mode is a request, not a guarantee;
- selected present mode comes from surface capabilities;
- unavailable requested mode falls back to FIFO in optional/dev mode with diagnostics;
- strict present-mode-specific smoke may fail if the exact requested mode is unavailable;
- present mode must not affect runtime tick determinism or replay hash.

Suggested CLI/env:

```text
--vulkan-present-mode=fifo|mailbox|immediate|auto
IGGY3D_VULKAN_PRESENT_MODE=fifo|mailbox|immediate|auto
```

Receipt fields:

```text
present_mode_requested=fifo|mailbox|immediate|auto
present_mode_selected=
present_mode_fallback=true|false
present_mode_fallback_reason=
available_present_modes=
```

## Shader Root Config

Required fields:

```text
shader_source_root
shader_binary_root
shader_language
shader_target_env
```

Recommended first defaults:

```text
shader_source_root=shaders/vulkan/src
shader_binary_root=build/generated/shaders/vulkan/<config>
shader_language=glsl
shader_target_env=vulkan
```

Rules:

- shader roots are renderer/build artifact paths, not content package truth;
- generated SPIR-V should not be searched from source directories by default;
- Windows multi-config builds must include the active config in generated shader path;
- visual demo may accept explicit shader binary root for local testing;
- runtime/content/projection/save must not read shader roots;
- missing required first-room shader artifacts fail strict visual smoke.

Suggested CLI/env:

```text
--vulkan-shader-root=<path>
--vulkan-shader-binary-root=<path>
IGGY3D_VULKAN_SHADER_ROOT=<path>
IGGY3D_VULKAN_SHADER_BINARY_ROOT=<path>
```

Receipt fields:

```text
shader_source_root=
shader_binary_root=
shader_language=glsl|slang|spirv_only|unavailable
vertex_shader=
fragment_shader=
shader_artifacts_found=true|false
```

## Debug Labels Mode

Allowed values:

```text
off
optional
required
```

Rules:

- debug labels require `VK_EXT_debug_utils`;
- `required` fails debug-label smoke if debug utils labels cannot be used;
- `optional` enables labels when available and reports unavailable otherwise;
- `off` must not emit command/object labels;
- labels are diagnostics, not runtime behavior;
- debug label names may include backend-neutral object ids for orientation, but runtime must not parse them.

Suggested CLI/env:

```text
--vulkan-debug-labels=off|optional|required
IGGY3D_VULKAN_DEBUG_LABELS=off|optional|required
```

Receipt fields:

```text
debug_labels_requested=off|optional|required
debug_utils=enabled|disabled|unavailable
debug_labels=enabled|disabled|unavailable
debug_object_name_count=
debug_command_label_count=
```

## Diagnostics Directory

Required behavior:

```text
diagnostics_dir defaults to build/artifacts/render_diagnostics
smoke tests may override diagnostics_dir
visual demo may print receipt to stdout only when requested
diagnostics path is never a save path
diagnostics path is never source asset truth
```

Suggested CLI/env:

```text
--render-diagnostics-dir=<path>
--render-diagnostics-stdout
IGGY3D_RENDER_DIAGNOSTICS_DIR=<path>
```

Rules:

- diagnostics directory must be created by app/test harness or diagnostics module;
- failure to create diagnostics directory fails strict smoke;
- optional visual app may continue with stderr diagnostics only if explicitly configured;
- diagnostics output must not be placed in source shader, content, or save directories;
- receipts must print the diagnostics directory even on skip when path resolution succeeded.

Receipt fields:

```text
diagnostics_dir=
diagnostics_stdout=true|false
diagnostics_file=
receipt_written=true|false
```

## Platform Lane Config

Allowed lane values:

```text
headless
macos_moltenvk
linux_native_vulkan
windows_native_vulkan
software_vulkan
unknown
```

Rules:

- lane is proof context, not gameplay behavior;
- macOS/MoltenVK lane verifies Apple portability constraints;
- Linux and Windows native Vulkan lanes are required for shipping proof;
- software Vulkan is optional CI aid only and must not replace native platform proof;
- lane must be printed in every Vulkan smoke receipt.

Suggested CLI/env:

```text
--vulkan-platform-lane=macos_moltenvk|linux_native_vulkan|windows_native_vulkan|software_vulkan
IGGY3D_VULKAN_PLATFORM_LANE=<lane>
```

Receipt fields:

```text
platform=
platform_lane=
software_vulkan_allowed=true|false
```

## Device Selection Config

Allowed first fields:

```text
preferred_device_index
require_discrete_gpu
allow_software_vulkan
```

Rules:

- default should select the first device that passes the documented device-selection policy;
- explicit device index is for smoke/dev diagnostics, not gameplay identity;
- `require_discrete_gpu` is a strict lane/tooling option, not a normal game requirement;
- `allow_software_vulkan` is false for shipping proof unless the lane is explicitly software Vulkan;
- selected device must be printed in receipts.

Suggested CLI/env:

```text
--vulkan-device-index=<n>
--vulkan-require-discrete-gpu
--vulkan-allow-software
```

Receipt fields:

```text
preferred_device_index=
selected_physical_device_index=
device_name=
device_type=
software_vulkan_selected=true|false
```

## Frames In Flight Config

Allowed value:

```text
max_frames_in_flight=1..3
```

Recommended first default:

```text
max_frames_in_flight=2
```

Rules:

- first renderer should reject `0`;
- values above `3` are out of first scope;
- changing frames in flight changes renderer latency/resource count, not runtime determinism;
- strict smoke should print the resolved count.

Suggested CLI/env:

```text
--vulkan-frames-in-flight=2
IGGY3D_VULKAN_FRAMES_IN_FLIGHT=2
```

Receipt fields:

```text
frames_in_flight_requested=
frames_in_flight=
```

## Suggested CLI Summary

Initial visual/smoke app flags:

```text
--renderer=null|vulkan|auto
--require-renderer
--no-renderer
--vulkan-strict
--vulkan-optional
--vulkan-validation=off|optional|required
--vulkan-sync-validation=off|optional|required
--vulkan-present-mode=fifo|mailbox|immediate|auto
--vulkan-shader-root=<path>
--vulkan-shader-binary-root=<path>
--vulkan-debug-labels=off|optional|required
--render-diagnostics-dir=<path>
--render-diagnostics-stdout
--vulkan-platform-lane=<lane>
--vulkan-device-index=<n>
--vulkan-require-discrete-gpu
--vulkan-allow-software
--vulkan-frames-in-flight=<n>
```

Rules:

- do not add these flags to headless runtime tools unless the tool explicitly supports renderer proof;
- visual demo may own these flags;
- smoke tests may own these flags;
- runtime package validation tools must not need these flags.

## Suggested Environment Variables

Initial environment variables:

```text
IGGY3D_RENDERER=null|vulkan|auto
IGGY3D_VULKAN_STRICT=true|false
IGGY3D_VULKAN_VALIDATION=off|optional|required
IGGY3D_VULKAN_SYNC_VALIDATION=off|optional|required
IGGY3D_VULKAN_PRESENT_MODE=fifo|mailbox|immediate|auto
IGGY3D_VULKAN_SHADER_ROOT=<path>
IGGY3D_VULKAN_SHADER_BINARY_ROOT=<path>
IGGY3D_VULKAN_DEBUG_LABELS=off|optional|required
IGGY3D_RENDER_DIAGNOSTICS_DIR=<path>
IGGY3D_VULKAN_PLATFORM_LANE=<lane>
IGGY3D_VULKAN_DEVICE_INDEX=<n>
IGGY3D_VULKAN_ALLOW_SOFTWARE=true|false
IGGY3D_VULKAN_FRAMES_IN_FLIGHT=<n>
```

Rules:

- environment variables are developer/test convenience, not authoritative runtime data;
- CLI flags override environment variables;
- receipts must show the resolved value, not just the source value;
- avoid reading environment variables from deep Vulkan modules if a typed config can be passed at construction.

## Config Validation Rules

Invalid config should fail before creating Vulkan objects when possible.

Required failures:

| Invalid input | Reason code |
| --- | --- |
| unknown renderer backend | `config_invalid_backend` |
| Vulkan requested but build disabled | `vulkan_build_disabled` |
| unknown validation mode | `config_invalid_validation_mode` |
| unknown sync validation mode | `config_invalid_sync_validation_mode` |
| unknown present mode request | `config_invalid_present_mode` |
| empty required shader binary root for visual Vulkan | `config_missing_shader_binary_root` |
| diagnostics path points inside save directory | `config_invalid_diagnostics_dir` |
| frames in flight outside first supported range | `config_invalid_frames_in_flight` |
| conflicting renderer flags | `config_conflicting_renderer_flags` |

Rules:

- invalid config is not an unsupported Vulkan environment skip;
- invalid config fails both optional and strict lanes unless the test intentionally verifies parse rejection;
- errors must include which config source supplied the bad value when known;
- app/test boundary maps invalid config to process exit `1`.

## Runtime Firewall

Forbidden:

```text
src/runtime/** reads renderer config
src/content/** reads renderer config
src/projection/** reads renderer config
src/runtime/save/** reads renderer config
replay files serialize renderer config
save files serialize renderer config
runtime hash includes renderer config
runtime command legality depends on renderer config
camera mode truth depends on renderer config
```

Allowed:

```text
apps/iggy3d_visual_demo/** parses renderer CLI/env
tests/smoke/vulkan_* supplies renderer config
src/render/** owns backend-neutral renderer config
src/render/vulkan/** owns Vulkan-private config translation
diagnostics output prints resolved renderer config
```

Firewall scans:

```sh
rg -n "IGGY3D_VULKAN|RendererConfig|vulkan-present|vulkan-validation|render-diagnostics" \
  src/runtime src/content src/projection src/runtime/save \
  apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no runtime/content/projection/save ownership leak
```

## Smoke Examples

Optional local Vulkan smoke:

```sh
iggy3d_visual_demo \
  --renderer=vulkan \
  --vulkan-optional \
  --vulkan-validation=optional \
  --vulkan-sync-validation=optional \
  --vulkan-present-mode=fifo \
  --render-diagnostics-stdout
```

Strict platform smoke:

```sh
iggy3d_visual_demo \
  --renderer=vulkan \
  --require-renderer \
  --vulkan-strict \
  --vulkan-validation=required \
  --vulkan-sync-validation=required \
  --vulkan-present-mode=fifo \
  --vulkan-debug-labels=optional \
  --render-diagnostics-dir=build/artifacts/render_diagnostics/strict
```

Debug-label smoke:

```sh
iggy3d_visual_demo \
  --renderer=vulkan \
  --vulkan-strict \
  --vulkan-validation=required \
  --vulkan-debug-labels=required \
  --render-diagnostics-dir=build/artifacts/render_diagnostics/debug_labels
```

Rules:

- these examples are future command shapes, not implemented commands yet;
- command examples must be updated when app names or flags are implemented;
- examples must not become runtime acceptance commands until visual renderer packets exist.

## Diagnostics Receipt Fields

Renderer config receipt fields:

```text
config_version=1
config_source_defaults=true
config_source_env=true|false
config_source_cli=true|false
renderer_backend_requested=
renderer_backend_selected=
vulkan_build_enabled=true|false
strict_vulkan=true|false
validation_requested=off|optional|required
sync_validation_requested=off|optional|required
debug_labels_requested=off|optional|required
present_mode_requested=fifo|mailbox|immediate|auto
present_mode_selected=
present_mode_fallback=true|false
shader_source_root=
shader_binary_root=
diagnostics_dir=
diagnostics_stdout=true|false
platform_lane=
preferred_device_index=
selected_physical_device_index=
allow_software_vulkan=true|false
software_vulkan_selected=true|false
frames_in_flight_requested=
frames_in_flight=
runtime_state_touched=false
runtime_hash_before=
runtime_hash_after=
```

Rules:

- `runtime_state_touched=false` is required when config parsing happens before runtime startup;
- runtime hash fields are required only for tests that intentionally combine runtime simulation and renderer proof;
- selected values must be printed after fallback, not guessed from requested values.

## Reason Codes

Config-related reason codes:

```text
config_ok
config_invalid_backend
config_conflicting_renderer_flags
config_invalid_validation_mode
config_invalid_sync_validation_mode
config_invalid_debug_labels_mode
config_invalid_present_mode
config_invalid_platform_lane
config_invalid_device_index
config_invalid_frames_in_flight
config_missing_shader_source_root
config_missing_shader_binary_root
config_invalid_shader_root
config_invalid_diagnostics_dir
config_diagnostics_dir_unavailable
vulkan_build_disabled
renderer_backend_unavailable
present_mode_unavailable
present_mode_fallback
validation_required_unavailable
sync_validation_required_unavailable
debug_labels_required_unavailable
runtime_config_leak
runtime_hash_changed
```

Rules:

- reason codes are stable machine-readable strings;
- config parse failures must not reuse Vulkan driver failure codes;
- fallback is a success with a reason only when policy permits fallback;
- strict exact-mode smoke may convert fallback into failure.

## Test Gates

Unit tests should prove:

- defaults resolve to `backend=auto` or the documented app-specific default;
- CLI overrides environment variables;
- invalid enum text fails with the expected reason code;
- conflicting renderer flags fail before Vulkan object creation;
- `strict_mode` never changes runtime config or runtime hash;
- present-mode text maps to backend-neutral enum, not public Vulkan constants;
- shader root paths normalize and reject empty required values;
- diagnostics dir rejects save/source-owned paths;
- frames-in-flight rejects `0` and values above first supported range;
- runtime/content/projection/save firewall scan stays clean.

Smoke tests should prove:

- optional Vulkan smoke can skip unsupported environment with receipt and exit `77`;
- strict Vulkan smoke fails unsupported environment with receipt and exit `1`;
- validation required config fails when layer is unavailable;
- sync validation required config fails when sync validation is unavailable;
- requested unavailable present mode falls back or fails according to lane;
- diagnostics receipt prints requested and selected config values;
- renderer config does not change runtime hash in combined runtime/render smoke.

## Acceptance Criteria

This policy is ready for implementation when:

- [ ] future `RendererConfig` fields match this document or the document is updated first;
- [ ] app CLI/env parsing has one owner;
- [ ] renderer config is typed before it reaches backend code;
- [ ] Vulkan-private translations stay under `src/render/vulkan/**`;
- [ ] present-mode fallback policy is tested;
- [ ] validation/sync/debug-label required modes are tested;
- [ ] diagnostics receipt prints resolved config;
- [ ] runtime firewall scan is clean;
- [ ] replay/hash test proves renderer config has no deterministic runtime effect.

## Non-Goals For First Renderer Pass

Do not add:

- global game settings system;
- user profile persistence;
- save-file renderer options;
- replay-controlled renderer config;
- in-game graphics settings menu;
- live renderer backend switching;
- hot shader-root reload;
- platform-specific native config files;
- parsing raw Vulkan enum names in runtime code.
