# Vulkan Debug Validation

This document defines validation layer enablement, debug messenger ownership, synchronization validation policy, validation message routing, strict smoke behavior, and failure artifacts for `iggy3d`.

Validation is renderer diagnostics infrastructure. It catches Vulkan API misuse. It does not own runtime truth, command legality, camera mode truth, save/load truth, replay determinism, or projection semantics.

## Purpose

Define the first validation contract:

```text
select validation mode
enumerate instance layers
enable validation layer when requested/required
enable debug utils extension when debug messenger is enabled
create debug messenger after instance creation
enable sync validation when requested/available
route validation callback messages to diagnostics
fail strict smoke on validation errors
write validation artifacts
destroy debug messenger before instance
```

This document sits beside [device_selection.md](device_selection.md), [sync_contract.md](sync_contract.md), [diagnostics_and_tests.md](diagnostics_and_tests.md), [fallbacks.md](fallbacks.md), and [packaging.md](packaging.md).

## Source Priority

Use these sources before implementation:

- Vulkan Validation Overview for why validation is development/smoke infrastructure.
- Vulkan Specification/Registry for exact debug utils structs, flags, and callback behavior.
- LunarG Synchronization Validation docs for sync validation enablement and expected coverage.
- Vulkan Guide validation topics for setup guidance.
- LunarG macOS SDK guide for validation layer availability on Apple SDK installs.
- Platform package docs only for how validation layers are installed, not for API semantics.

## Scope

In scope:

- validation layer discovery;
- required/optional validation modes;
- debug utils extension policy;
- debug messenger lifetime;
- callback severity/type filtering;
- synchronization validation request/require policy;
- strict smoke failure rules;
- validation log artifacts;
- diagnostics receipt fields;
- platform notes for macOS/MoltenVK, Linux, and Windows.

Out of scope:

- Vulkan object lifetime other than debug messenger lifetime;
- device feature selection except validation-related feature reporting;
- RenderDoc capture workflow beyond artifact handoff;
- runtime or projection behavior;
- release telemetry.

## Local File Surface

Likely future files:

```text
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
tests/smoke/vulkan_validation_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_diagnostics_smoke.cpp
```

`DebugValidation` owns validation configuration, layer/extension checks, debug messenger creation/destruction, callback routing, and validation counters.

## Ownership

| Item | Owner | Lifetime |
| --- | --- | --- |
| validation config | renderer/app startup config | app lifetime |
| validation layer list | `DebugValidation` diagnostics | instance creation time |
| debug utils extension request | `DebugValidation` | instance creation time |
| debug messenger | `DebugValidation` or `InstanceDeviceSurface` | instance lifetime |
| callback counters | `DebugValidation` | renderer lifetime |
| validation log | diagnostics artifact owner | test/app lifetime |

Rules:

- debug messenger is destroyed before Vulkan instance;
- callback must not call runtime mutation APIs;
- callback must not throw exceptions across the Vulkan C callback boundary;
- callback may update renderer-owned atomic/counter/log state;
- validation messages are diagnostics, not gameplay events.

## Modes

Recommended modes:

```text
validation_mode=off|optional|required
sync_validation_mode=off|optional|required
strict_vulkan=true|false
```

Mode behavior:

| Mode | Missing layer | Validation error |
| --- | --- | --- |
| off | continue, report not requested | no callback expected |
| optional | continue or skip validation-specific test, report unavailable | fail only if test requires clean validation |
| required | fail startup/smoke | fail strict smoke |

Rules:

- dev/smoke defaults should enable validation when available;
- strict Vulkan lanes require validation;
- strict sync lanes require sync validation when that lane is selected;
- release builds do not force validation;
- headless runtime tests do not require Vulkan validation.

## Layer Discovery

Required first layer when validation is requested:

```text
VK_LAYER_KHRONOS_validation
```

Layer discovery rules:

- enumerate instance layer properties before instance creation;
- check exact layer name;
- print found/missing status;
- fail if required and missing;
- continue with `validation=unavailable` if optional and missing;
- do not treat missing validation layer as runtime failure.

Diagnostics:

```text
validation_layer_requested=true|false
validation_layer_required=true|false
validation_layer_found=true|false
validation_layer_name=VK_LAYER_KHRONOS_validation
enabled_instance_layers=
missing_instance_layers=
```

## Debug Utils Extension

When debug messenger is enabled, request:

```text
VK_EXT_debug_utils
```

Rules:

- debug utils extension is required only when debug messenger is required;
- missing debug utils extension fails strict validation lane;
- optional validation lane can continue without debug messenger only if diagnosed;
- debug object naming can be added later, but callback support comes first.

Diagnostics:

```text
debug_utils_requested=true|false
debug_utils_found=true|false
debug_messenger_created=true|false
```

## Debug Messenger Lifetime

Creation order:

1. Enumerate available layers/extensions.
2. Create Vulkan instance with validation layers/extensions if enabled.
3. Create debug messenger.
4. Continue surface/device setup.

Destruction order:

1. Stop submitting renderer work.
2. Destroy child Vulkan objects.
3. Destroy debug messenger.
4. Destroy Vulkan instance.

Rules:

- early instance-creation validation messages may require chaining debug messenger create info into instance creation if supported by the implementation plan;
- debug messenger creation failure fails strict validation lane;
- debug messenger handle is private to Vulkan backend;
- debug messenger does not own validation message storage.

## Callback Contract

The validation callback may:

- increment renderer-owned counters;
- store first error/warning summary;
- append sanitized message text to validation log;
- print to stderr in dev mode if configured;
- include message id/name/severity/type in diagnostics.

The validation callback must not:

- mutate runtime state;
- mutate projection data;
- submit runtime commands;
- allocate unbounded memory per message;
- throw exceptions;
- block on long filesystem work;
- expose raw handles in public diagnostics.

Recommended callback result:

```text
return VK_FALSE
```

Do not abort the Vulkan call from the callback unless a later design explicitly requires it.

## Severity Policy

Track these severities:

```text
verbose
info
warning
error
```

First policy:

- errors fail strict smoke;
- warnings are logged and counted;
- repeated warnings may fail a specific warning-clean lane later, but not first-room strict lane unless explicitly required;
- verbose/info are logged only when verbose validation diagnostics are enabled.

Diagnostics:

```text
validation_error_count=
validation_warning_count=
validation_info_count=
validation_verbose_count=
first_validation_error=
first_validation_warning=
```

## Message Type Policy

Track these message types:

```text
general
validation
performance
device_address_binding
```

First policy:

- validation messages are always counted;
- performance warnings are logged but do not fail first strict smoke unless promoted by a packet;
- general messages are logged according to severity;
- device-address binding is irrelevant until that feature is used.

## Synchronization Validation

Sync validation is required for strict sync lanes.

Rules:

- request sync validation through the supported validation-layer settings path for the target SDK;
- print whether sync validation is enabled, unavailable, disabled, or not requested;
- if required and unavailable, fail with `sync_validation_required_missing`;
- any sync validation error fails strict sync smoke;
- sync validation warnings are logged and counted.

Sync validation must cover:

- image layout transitions;
- acquire/submit/present semaphore use;
- command buffer reset while in use;
- upload-before-draw hazards;
- resize destroying in-use resources;
- present without render-finished wait.

Diagnostics:

```text
sync_validation_requested=true|false
sync_validation_required=true|false
sync_validation=enabled|disabled|unavailable
sync_validation_error_count=
sync_validation_warning_count=
first_sync_validation_error=
```

## Strict Smoke Behavior

Strict Vulkan smoke fails when:

- required validation layer is missing;
- required debug utils extension is missing;
- required debug messenger cannot be created;
- required sync validation is unavailable;
- any validation error occurs;
- any sync validation error occurs;
- validation setup contradicts requested strict mode;
- renderer suppresses validation messages without diagnostics.

Strict Vulkan smoke may still skip only if the entire Vulkan lane is optional and skip policy allows it. Required platform proof must fail, not skip.

## Optional Behavior

Optional Vulkan smoke may:

- run without validation if layers are unavailable;
- skip validation-specific tests if validation is unavailable;
- print `validation=unavailable`;
- continue with warning diagnostics.

Optional behavior must not be used to claim shippable Vulkan readiness.

## Release Behavior

Release builds:

- do not enable validation by default;
- do not require validation layers to be installed;
- may support an explicit developer flag to enable validation if available;
- must not include validation as normal gameplay logic;
- must preserve runtime determinism regardless of validation mode.

Validation layer absence in release is not an error.

## Diagnostics Receipt Fields

Validation diagnostics should include:

```text
validation_mode=off|optional|required
sync_validation_mode=off|optional|required
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
validation_layer_requested=true|false
validation_layer_required=true|false
validation_layer_found=true|false
validation_layer_name=
debug_utils_requested=true|false
debug_utils_found=true|false
debug_messenger_created=true|false
validation_strict=true|false
validation_error_count=
validation_warning_count=
validation_info_count=
validation_verbose_count=
sync_validation_error_count=
sync_validation_warning_count=
first_validation_error=
first_validation_warning=
first_sync_validation_error=
validation_log_path=
reason=
```

Use `none` for first-message fields when there is no message. Use `unavailable` only when a field truly does not apply.

## Failure Reason Codes

Recommended reason codes:

```text
missing_validation_layers
missing_sync_validation
validation_layer_required_missing
sync_validation_required_missing
debug_utils_required_missing
debug_messenger_create_failed
validation_error
sync_validation_failed
validation_setup_mismatch
validation_callback_failure
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during implementation.

## Failure Artifacts

When validation is enabled, write artifacts under:

```text
build/artifacts/render_diagnostics/<test_name>/
```

Expected files:

```text
receipt.txt
validation.log
stderr.txt
stdout.txt
```

Rules:

- validation log may contain raw validation text;
- public receipt should keep stable fields and short first-message summaries;
- failure artifacts do not affect runtime state or replay hash;
- logs should be bounded or rotated if a failure floods messages.

## Platform Notes

macOS/MoltenVK:

- validation layers depend on SDK/install setup;
- sync validation availability must be printed honestly;
- MoltenVK translation messages may differ from native Vulkan diagnostics;
- local Apple validation does not replace Linux/Windows native Vulkan validation.

Linux:

- validation layer availability depends on installed Vulkan SDK/packages;
- native Vulkan validation lane should run with a real display-backed path when required;
- software Vulkan/lavapipe validation is optional and must be diagnosed as software.

Windows:

- validation layer availability depends on SDK/runtime install;
- installed package smoke must distinguish missing loader, missing device, and missing validation layers;
- validation logs should be captured even when the app exits nonzero.

## Tests

Future tests:

```text
tests/smoke/vulkan_validation_smoke.cpp
tests/smoke/vulkan_device_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_diagnostics_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Smoke expectations:

- optional validation lane reports enabled or unavailable;
- strict validation lane fails if required layer is missing;
- debug messenger creates/destroys validation-clean;
- first-room smoke has zero validation errors;
- sync smoke has zero sync validation errors when sync validation is enabled;
- validation callback does not mutate runtime hash/state.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_validation|vulkan_device|vulkan_sync|vulkan_diagnostics|vulkan_first_room'
```

## Acceptance Criteria

This validation contract is ready for file plans when:

- validation modes are defined;
- layer discovery behavior is defined;
- debug utils extension behavior is defined;
- debug messenger ownership/lifetime is defined;
- callback allowed/forbidden behavior is defined;
- severity/type policy is defined;
- sync validation behavior is defined;
- strict versus optional behavior is defined;
- release behavior is defined;
- diagnostics and failure reason codes are defined;
- failure artifact policy is defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- validation cannot mutate runtime truth.

## Open Detail Items

These belong in future file plans:

- exact validation layer settings mechanism for sync validation;
- exact debug messenger create-info helper;
- exact callback storage type and thread-safety policy;
- exact maximum validation log size;
- exact message-id suppression policy if noisy platform messages appear;
- exact validation CLI/CMake flags;
- exact Linux and Windows validation smoke commands;
- exact RenderDoc capture relationship, if added.
