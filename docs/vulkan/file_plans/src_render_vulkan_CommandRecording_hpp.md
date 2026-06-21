# src/render/vulkan/CommandRecording.hpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/render/vulkan/CommandRecording.hpp`

Purpose: Declare graphics command pool/buffer ownership and first-room command recording entry points.

## Build Position

Packet order: 5 - Swapchain, render loop, command recording, sync
Owner module: `command recording`
File kind: `header`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/boundaries.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/diagnostics_and_tests.md`
- `docs/vulkan/command_recording.md`
- `docs/vulkan/vulkan_image_layouts_and_barriers.md`

## Ownership

This file owns:
- Declare graphics command pool/buffer ownership and first-room command recording entry points.
- the public or private names listed in the file shape section;
- diagnostics fields directly tied to its responsibility.

This file must never own:
- gameplay truth;
- command legality;
- save/load truth;
- replay or deterministic state-hash truth;
- content package validation truth;
- renderer fallback without diagnostics;
- legacy renderer linkage.

Runtime firewall boundaries:
- runtime, content, projection, and save code do not depend on this file unless it is a backend-neutral render contract explicitly consumed by an app layer;
- this file cannot mutate runtime state directly;
- renderer output cannot affect replay results.

## Required Include Policy

Allowed includes: standard library, `src/render/**`, `src/render/vulkan/**`, Vulkan SDK headers, and test harness headers for Vulkan smoke tests.

Forbidden includes: runtime mutation internals, content package validators, save/load internals, old repo headers, and public exposure of raw `Vk*` handles outside Vulkan-owned declarations.

Vulkan headers are allowed because this file is under `src/render/vulkan/**` or is an explicitly named Vulkan smoke test.

SDL headers are allowed only when the smoke test exercises the platform shell; otherwise keep SDL behind app/platform files.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `class CommandRecording`
- `CommandRecordingContext`
- `recordEmptyFrame`
- `recordFirstRoomFrame`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Vulkan handles are created and destroyed only by their owning Vulkan module or explicitly named Vulkan glue file. Backend-neutral inputs are borrowed or copied for the duration of a frame and are not retained past the owning call unless the type says so. Diagnostics receipts are owned by `RenderDiagnostics` or the smoke harness. Resize and device-loss paths must stop use of stale swapchain resources before destroy/recreate. Shutdown order must destroy child Vulkan objects before the logical device and instance.

## Semantics

Normal path: create or use the Vulkan objects named by this file, emit receipt fields, return backend-neutral outcomes, and preserve runtime state. Skip/fail: optional lanes may skip before unsafe work; strict lanes fail on required Vulkan gaps, validation errors, or unsupported devices.

Platform behavior:
macOS/MoltenVK: report `platform=macos` and `platform_lane=moltenvk` when Vulkan is attempted; MoltenVK portability details are diagnostics, not cross-platform law.
Linux: report `platform=linux` and `platform_lane=native_vulkan` for hardware/native validation; software Vulkan uses a separate lane.
Windows: report `platform=windows` and `platform_lane=native_vulkan`; multi-config shader/package paths must include the active config where relevant.
Software Vulkan: allowed for optional development evidence only; it cannot replace native macOS/Linux/Windows proof.
Strict lane: required gates fail with `result=fail`.
Optional lane: unsupported environment or missing optional Vulkan prerequisites may skip with `result=skip` before unsafe renderer work begins.

## Diagnostics And Result Policy

Stable reason names must use lowercase snake-case text. Receipts use deterministic key-value lines.

Required receipt fields:
```text
receipt_version=1
repo=iggy3d
file_plan=src/render/vulkan/CommandRecording.hpp
packet_order=5
allowed_to_implement_code_now=false
backend=vulkan
device_name=
api_version=
enabled_instance_extensions=
enabled_device_extensions=
validation=enabled|disabled|unavailable
result=pass|fail|skip
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: optional lanes may skip with `result=skip` and a reason code before required Vulkan work begins. Strict lanes fail with `result=fail`. Render-pass fallback and software Vulkan shipping substitution are not allowed by this file plan.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=command_recording
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: Vulkan object creation, device queries, validation setup, or GPU resource setup according to module scope.
Per-frame cost: no runtime mutation; command/sync/resource modules pay only the documented frame work.
Resize cost: bounded wait/recreate/teardown for swapchain-dependent objects when the module owns them.
GPU memory cost: named allocations only; budget receipt required once resources are created.

## Tests And Verification

Unit tests:
- none for this file; covered by smoke or build tests

Smoke tests:
- `tests/smoke/vulkan_empty_frame_smoke.cpp`

CTest labels:
```text
iggy3d;vulkan;smoke
```

Expected pass behavior: required receipt fields are present and the file owns only the declared responsibility.
Expected skip behavior: optional Vulkan lanes may skip only before required Vulkan work begins and must print `result=skip` plus `reason_code`.
Expected fail behavior: strict lanes fail on missing required dependency, validation error, runtime mutation, or boundary leak.

Firewall scan:
```sh
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected firewall scan result:
```text
no matches
```

## Packet 5 Detailed Contract

Current-build API:
```cpp
namespace iggy3d::vulkan {

struct CommandRecordingCreateInfo {
    VkDevice device;
    std::uint32_t graphicsQueueFamily;
    std::uint32_t frameSlotCount;
    RenderDiagnosticsSink* diagnostics;
};

struct EmptyFrameRecordInfo {
    VkCommandBuffer commandBuffer;
    VkImage swapchainImage;
    VkImageView swapchainImageView;
    VkFormat colorFormat;
    VkExtent2D extent;
    std::uint32_t frameSlot;
    std::uint32_t imageIndex;
    float clearR;
    float clearG;
    float clearB;
    float clearA;
};

struct CommandRecordResult {
    RenderOutcome outcome;
    RenderReason reason;
    bool recorded;
    const char* stage;
    RenderReceipt receipt;
};

class CommandRecording {
public:
    RenderReceipt create(const CommandRecordingCreateInfo& createInfo);
    RenderReceipt destroy();
    VkCommandBuffer commandBufferForFrameSlot(std::uint32_t frameSlot) const;
    CommandRecordResult recordEmptyFrame(const EmptyFrameRecordInfo& recordInfo);
};

}
```

Include policy:
- Vulkan headers are allowed because this is a Vulkan-private header.
- Runtime, content, projection, save, SDL, app platform, shader compiler, and package lookup headers are forbidden.

Ownership:
- Owns one graphics command pool and one primary command buffer per frame slot.
- Does not own swapchain images/views, frame fences/semaphores, pipelines, GPU buffers, shader modules, runtime data, or projection data.

Packet 5 recording scope:
- Records a clear-only dynamic-rendering frame.
- Transitions swapchain image from discard state to color attachment layout.
- Begins dynamic rendering with one color attachment.
- Ends dynamic rendering.
- Transitions swapchain image to present layout.
- Does not bind pipelines, descriptors, vertex buffers, index buffers, or material resources.

Receipt fields:
```text
command_pool_created=true|false
command_pool_queue_family=<integer>
command_buffer_count=<integer>
command_buffer_level=primary
record_mode=empty_frame
record_begin=true|false
dynamic_rendering_begin=true|false
dynamic_rendering_end=true|false
swapchain_color_before_render=UNDEFINED->COLOR_ATTACHMENT_OPTIMAL
swapchain_color_after_render=COLOR_ATTACHMENT_OPTIMAL->PRESENT_SRC_KHR
command_recorded=true|false
command_record_stage=<stage-name-or-none>
```

Verification:
- `vulkan_empty_frame_smoke` proves command buffer recording succeeds and presents.
- Sync validation proves no command buffer reset or image-layout hazard exists in strict lanes.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not expose raw Vulkan handles through public renderer API.
- Do not convert unsupported required gates into a strict-lane skip.
- Do not use MoltenVK quirks as the global Vulkan design rule.

## Completion Criteria

- File `src/render/vulkan/CommandRecording.hpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.
