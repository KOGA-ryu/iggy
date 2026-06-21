# Vulkan Device Loss Recovery

This document defines what `iggy3d` does when Vulkan reports `VK_ERROR_DEVICE_LOST`: how the renderer records diagnostics, stops submitting work, shuts down Vulkan objects, preserves runtime determinism, and decides when a renderer/app restart is required.

Device loss is renderer failure handling. It must not become runtime truth, gameplay command handling, save/load state, replay state, camera truth, package validation, or projection semantics.

## Purpose

Define a conservative first policy:

```text
device_lost_policy=diagnose_stop_shutdown
logical_device_reset_possible=false
automatic_logical_device_recreate=deferred
swapchain_recreate_on_device_lost=false
runtime_mutation_on_device_lost=false
renderer_restart_required=true_initially
strict_visual_lane_result=fail
headless_runtime_result=unaffected
```

The first implementation should treat device loss as a fatal Vulkan backend state. Later work may add full renderer recreation only after the normal creation/lifetime paths are stable and after tests prove runtime state remains unchanged.

This document narrows:

- [vulkan_swapchain_failure_modes.md](vulkan_swapchain_failure_modes.md)
- [lifetime.md](lifetime.md)
- [render_loop.md](render_loop.md)
- [sync_contract.md](sync_contract.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)
- [vulkan_debug_labels_and_capture.md](vulkan_debug_labels_and_capture.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Specification devices/queues lost-device section: https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#devsandqueues-lost-device | exact lost-device behavior, child object lifetime, wait behavior, mapped memory state |
| Vulkan Specification error codes: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html | exact `VK_ERROR_DEVICE_LOST` meaning and command return behavior |
| Vulkan Specification object lifetime: https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-objectmodel-lifetime | parent/child destruction obligations |
| `VK_EXT_device_fault`: https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_device_fault.html | optional post-device-loss fault diagnostics |
| LunarG crash diagnostic layer: https://vulkan.lunarg.com/doc/view/latest/linux/crash_diagnostic_layer.html | optional crash/device-fault diagnostic tooling |
| GFXReconstruct docs: https://vulkan.lunarg.com/doc/view/latest/windows/capture_tools.html | optional capture/replay context when reproducing device loss |

Priority rule:

```text
device_loss_api_truth=Vulkan Specification
fault_diagnostics=VK_EXT_device_fault and LunarG crash diagnostics
capture_reproduction=GFXReconstruct and RenderDoc where applicable
project_behavior=iggy3d runtime/renderer boundary docs
```

## Scope

In scope:

- where `VK_ERROR_DEVICE_LOST` can be detected;
- renderer state transition after device loss;
- diagnostics receipt fields;
- validation/capture artifact policy;
- shutdown path after lost device;
- mapped memory and VMA implications;
- strict versus optional smoke behavior;
- runtime mutation firewall;
- restart requirement;
- deferred renderer recreation policy.

Out of scope:

- implementing logical device recreation now;
- GPU crash dump upload;
- vendor-specific checkpoint extensions as first path;
- automatic driver reset handling;
- external memory recovery;
- multi-GPU failover;
- live user-facing crash UI;
- treating device loss as normal swapchain resize.

## Local File Surface

Likely future files:

```text
src/render/vulkan/VulkanBackend.hpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
build/artifacts/render_diagnostics/device_lost/
tests/unit/render_device_loss_policy_tests.cpp
tests/unit/render_result_mapping_tests.cpp
tests/smoke/vulkan_device_lost_smoke.cpp
tests/smoke/vulkan_shutdown_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| lost-device state | Vulkan backend coordinator | runtime state |
| first lost result | Vulkan result/diagnostics module | save truth |
| shutdown decision | renderer backend/app shell | gameplay command legality |
| fault diagnostic query | debug/diagnostics module | replay result |
| lost-device artifact | diagnostics artifact owner | source assets |
| restart request | app-level renderer owner | runtime mutation |

Rules:

- runtime/projection never see `VK_ERROR_DEVICE_LOST`;
- renderer may report backend-neutral `renderer_device_lost`;
- runtime hash/replay result must not change because device loss occurred;
- renderer does not invent or submit runtime commands during recovery;
- if picking/input was in progress, it is abandoned and must route through normal runtime commands later.

## Device Loss Meaning

Once a logical device is lost:

```text
logical_device_lost=true
reset_same_logical_device=false
future_device_commands_may_return_device_lost=true
device_child_handles_still_exist=true
child_objects_must_still_be_destroyed=true
mapped_memory_address_space_valid=true
mapped_memory_contents_undefined=true
```

Important consequences:

- do not treat device loss as swapchain out-of-date;
- do not recreate only the swapchain;
- do not trust GPU resource contents;
- do not read mapped GPU memory contents for gameplay or diagnostics payload truth;
- still destroy Vulkan child objects in a valid lifetime order;
- expect wait commands such as `vkDeviceWaitIdle`, `vkQueueWaitIdle`, `vkWaitForFences`, or maximum-timeout acquire to return in finite time with success or device lost.

## Detection Points

Any Vulkan call that can return `VK_ERROR_DEVICE_LOST` must route through a common result conversion path.

High-priority detection points:

| Call site | First response |
| --- | --- |
| `vkQueueSubmit` | stop frame loop, mark device lost |
| `vkQueuePresentKHR` | mark device lost, do not attempt swapchain recreate |
| `vkAcquireNextImageKHR` | mark device lost, skip submit |
| `vkWaitForFences` | mark device lost, do not assume fence recovery |
| `vkDeviceWaitIdle` | proceed to lost-device cleanup path |
| `vkQueueWaitIdle` | proceed to lost-device cleanup path |
| upload submit/wait | mark upload failed by device loss |
| VMA/Vulkan allocation or map paths if returning device lost through Vulkan calls | mark renderer failed |

Rules:

- result mapping must preserve the original Vulkan call name;
- record first lost-device result only once;
- after lost state is set, new frame submission is forbidden;
- repeated `VK_ERROR_DEVICE_LOST` results are counted but should not spam logs every frame;
- strict smoke fails immediately with stable reason code.

## State Model

Renderer device state:

```text
not_started
initializing
ready
device_lost_detected
collecting_device_lost_diagnostics
shutdown_after_device_lost
shutdown_complete
restart_required
failed
```

Allowed transitions:

```text
ready -> device_lost_detected
initializing -> device_lost_detected
device_lost_detected -> collecting_device_lost_diagnostics
collecting_device_lost_diagnostics -> shutdown_after_device_lost
shutdown_after_device_lost -> shutdown_complete
shutdown_complete -> restart_required
```

Forbidden transitions:

```text
device_lost_detected -> ready
device_lost_detected -> swapchain_recreate_only
device_lost_detected -> runtime_mutation
device_lost_detected -> continue_submitting_frames
```

## Immediate Response

When device loss is detected:

1. Atomically mark renderer `device_lost_detected`.
2. Record first failing call name, result, frame index, queue, command buffer label, and swapchain state.
3. Stop accepting new Vulkan frame submissions.
4. Stop upload submissions.
5. Stop swapchain recreate attempts.
6. Stop pipeline/resource creation attempts.
7. Capture validation/debug-label context already available in memory.
8. Query optional fault diagnostics if enabled and safe.
9. Begin renderer shutdown path.
10. Return backend-neutral failure to app shell.

Hard rules:

- do not call runtime mutation APIs;
- do not mark runtime frame/tick failed;
- do not change save state;
- do not update replay truth;
- do not keep rendering with partially trusted resources;
- do not perform unbounded diagnostics inside a Vulkan callback.

## Diagnostics Receipt

Device-loss receipt fields:

```text
device_lost=true
device_lost_policy=diagnose_stop_shutdown
device_lost_first_call=
device_lost_first_result=VK_ERROR_DEVICE_LOST
device_lost_frame_index=
device_lost_in_flight_frame_slot=
device_lost_queue=graphics|present|transfer|compute|unknown
device_lost_command_label=
device_lost_last_completed_label=
device_lost_swapchain_state=
device_lost_during=init|acquire|record|submit|present|wait|upload|shutdown|unknown
device_lost_validation_error_count=
device_lost_validation_warning_count=
device_lost_first_validation_error=
device_fault_extension_enabled=true|false
device_fault_info_available=true|false|unqueried
device_fault_address_count=
device_fault_vendor_info_count=
device_fault_binary_written=true|false
gpu_capture_available=renderdoc|gfxreconstruct|none|unqueried
gpu_capture_path=
renderer_shutdown_started=true|false
renderer_shutdown_completed=true|false
renderer_restart_required=true
runtime_state_touched=false
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false|unavailable
reason=device_lost
```

Artifact root:

```text
build/artifacts/render_diagnostics/device_lost/
```

Allowed artifacts:

- key-value receipt;
- validation log;
- debug-label summary;
- selected device/driver/extension receipt;
- optional `VK_EXT_device_fault` text/binary dump;
- optional RenderDoc/GFXReconstruct capture path reference if manually produced.

Do not commit device-loss artifacts by default.

## Fault Diagnostics

Optional extension:

```text
VK_EXT_device_fault
```

Policy:

```text
device_fault_required=false_initially
device_fault_query_after_loss=enabled_only_if_extension_enabled_and_functions_loaded
device_fault_binary_artifact=optional_debug_only
```

Rules:

- `VK_EXT_device_fault` is diagnostic tooling, not recovery logic;
- absence of device fault information must not hide the primary device-lost error;
- if fault binary data is written, store it under diagnostics artifacts and do not commit it;
- fault addresses/vendor data are low-level renderer diagnostics only;
- runtime/content/projection/save must not parse fault data.

Deferred vendor-specific diagnostics:

- NVIDIA queue checkpoints;
- AMD buffer markers;
- vendor crash dump SDKs.

These are not first implementation requirements.

## Shutdown Path

Device loss shutdown follows normal reverse lifetime order, with extra tolerance for lost-device returns.

Recommended path:

1. Mark renderer as not accepting frames.
2. Attempt bounded `vkDeviceWaitIdle` or skip if already known unsafe by policy.
3. If `vkDeviceWaitIdle` returns `VK_ERROR_DEVICE_LOST`, continue cleanup.
4. Destroy transient upload resources.
5. Destroy buffers/images/views/samplers/descriptors.
6. Destroy pipelines and pipeline layouts.
7. Destroy frame sync objects.
8. Destroy command buffers/pools.
9. Destroy depth resources.
10. Destroy swapchain image views.
11. Destroy swapchain.
12. Destroy VMA allocator/bootstrap allocations.
13. Destroy logical device.
14. Destroy surface.
15. Destroy debug messenger.
16. Destroy instance.
17. Mark shutdown complete.

Rules:

- child objects are not implicitly destroyed by device loss;
- destroy/free device children before destroying device;
- cleanup must tolerate partially initialized modules;
- cleanup must tolerate repeated lost-device results from commands that return `VkResult`;
- destroy calls that do not return `VkResult` are still required when handles are valid;
- do not read mapped memory contents to decide game behavior.

## Sync And Fence Policy

Device loss invalidates normal assumptions about in-flight GPU work.

Rules:

- after device loss, do not wait forever for fences;
- wait calls must be routed through result conversion and bounded by Vulkan finite-return guarantees;
- do not reset fences after lost device;
- do not recycle command buffers for new submissions;
- do not consider an upload complete for renderer resource readiness if the upload ended in device loss;
- mark all GPU resources from the lost device as unusable.

If a command returns `VK_ERROR_DEVICE_LOST`, treat command buffers/resources as no longer pending for cleanup purposes, but not as valid completed render output.

## Memory And Resource Policy

After device loss:

```text
gpu_resource_contents=undefined
mapped_memory_contents=undefined
renderer_resource_ready=false
asset_gpu_residency=invalid
fallback_gpu_resources=invalid
```

Rules:

- renderer must not sample, present, or reuse any resource from the lost device;
- VMA allocations must still be destroyed through the normal allocator-owned path;
- allocation names remain useful as diagnostics only;
- memory budget data after loss is diagnostic at best and must be labeled stale/unavailable;
- asset/content identity remains valid outside the renderer, but GPU residency is gone.

## Runtime Firewall

Device loss must not mutate deterministic runtime state.

Required invariant test:

```text
runtime_hash_before == runtime_hash_after
```

Forbidden behavior:

- changing player position;
- changing command legality;
- changing camera mode truth;
- writing save files;
- changing replay result;
- removing content assets;
- turning renderer fallback into content truth;
- submitting input/picking results directly into runtime.

Allowed behavior:

- app shell reports renderer failure;
- visual demo exits nonzero in strict lane;
- optional interactive app pauses visual renderer;
- headless runtime loop continues only if the app was designed to run without renderer and no Vulkan call is required.

## Restart And Recreate Policy

First policy:

```text
automatic_renderer_recreate_after_device_loss=false
process_restart_recommended=true
strict_visual_demo_exit_nonzero=true
```

Why:

- the lost logical device cannot be reset;
- physical device may also be lost;
- all GPU resource residency is invalid;
- normal renderer creation path is not proven restart-safe yet;
- runtime determinism matters more than heroic visual recovery.

Future renderer recreation is allowed only after a dedicated file plan proves:

- renderer can tear down completely after partial or full initialization;
- renderer can create a new logical device and swapchain from clean state;
- all GPU resources can be rebuilt from backend-neutral source data;
- runtime hash remains unchanged;
- visual app can distinguish logical-device recreate from process restart;
- strict tests cover macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan.

Until that exists, device loss means renderer restart required.

## Platform Notes

macOS/MoltenVK:

- device loss may reflect Metal/MoltenVK/platform behavior;
- collect MoltenVK version/config diagnostics if available;
- do not design recovery solely around Apple behavior.

Linux:

- driver resets, compositor behavior, Mesa/proprietary driver differences, and software Vulkan lanes must be labeled;
- collect device name, driver version, Mesa/proprietary hint where available;
- lavapipe device loss evidence is not hardware evidence.

Windows:

- device loss may be related to driver reset/TDR-style platform behavior;
- collect vendor/device/driver version and Windows build where available;
- Visual Studio/RenderDoc/GFXReconstruct capture tooling remains optional.

Cross-platform rule:

```text
device_lost_is_renderer_failure_on_all_platforms=true
platform_specific_notes_are_diagnostics_not_policy_exceptions
```

## Validation And Tests

Firewall scan:

```sh
rg -n "VK_ERROR_DEVICE_LOST|device_lost|renderer_device_lost|vkDeviceWaitIdle|vkQueueWaitIdle|vkWaitForFences|VK_EXT_device_fault" src/runtime src/content src/projection src/runtime/save apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no runtime/content/projection/save/headless device-loss ownership leak
```

Unit tests should cover:

- `VK_ERROR_DEVICE_LOST` maps to stable renderer reason code;
- first lost-device call is recorded once;
- lost-device state forbids new submit/present/upload;
- lost-device result does not route to swapchain recreate;
- shutdown path proceeds after wait returns device lost;
- runtime hash fields remain equal;
- automatic recreate is rejected until future policy enables it;
- fault diagnostics are optional.

Smoke tests should cover:

- simulated result mapping for acquire/submit/present/wait;
- renderer receipt contains device-lost fields;
- strict visual lane exits nonzero on simulated device loss;
- optional/headless runtime lane is unaffected when Vulkan is disabled;
- shutdown cleanup can run after a simulated lost-device transition.

Manual reproduction checklist:

- run with validation and sync validation enabled;
- run with debug labels enabled;
- capture first failing call and last command label;
- record device/driver/platform;
- if available, query `VK_EXT_device_fault`;
- if useful, produce a GFXReconstruct capture for reproduction;
- do not commit capture/fault artifacts by default.

## Failure Reason Codes

Use stable reason codes in diagnostics:

```text
device_lost
device_lost_during_init
device_lost_during_acquire
device_lost_during_submit
device_lost_during_present
device_lost_during_wait
device_lost_during_upload
device_lost_shutdown_started
device_lost_shutdown_failed
device_lost_shutdown_complete
device_lost_restart_required
device_lost_swapchain_recreate_rejected
device_lost_runtime_mutation_blocked
device_lost_fault_info_unavailable
device_lost_fault_info_written
device_lost_capture_available
device_lost_capture_unavailable
device_lost_recreate_deferred
device_lost_runtime_leak
```

## Acceptance Gate

This policy is ready for implementation planning when:

- every `VK_ERROR_DEVICE_LOST` return path maps to renderer device-lost state;
- device loss is separate from swapchain out-of-date/suboptimal handling;
- shutdown order after device loss is accepted;
- runtime mutation firewall is accepted;
- restart-required first policy is accepted;
- optional fault diagnostics are clearly nonessential;
- receipt fields and reason codes are accepted;
- firewall scan has no device-loss ownership leaks outside renderer/app-shell-owned files.

