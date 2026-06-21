# Vulkan Swapchain Contract

This document defines swapchain ownership, selection policy, image view lifetime, resize/recreate behavior, and diagnostics for `iggy3d`.

The swapchain is renderer-owned presentation infrastructure. It depends on a platform-created surface and a selected Vulkan device. It does not own runtime truth, projection truth, camera mode truth, save data, or replay results.

## Purpose

Define the first swapchain contract:

```text
query surface support
query surface capabilities
select color format and color space
select present mode
select extent
select image count
select sharing mode
create swapchain
create image views
couple depth extent
feed dynamic rendering format metadata
handle out-of-date/suboptimal/resize recreate
print diagnostics receipt
```

This document sits between [platform_shell.md](platform_shell.md), [render_loop.md](render_loop.md), [sync_contract.md](sync_contract.md), [command_recording.md](command_recording.md), and [lifetime.md](lifetime.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for exact `VkSwapchainCreateInfoKHR`, WSI result codes, surface capabilities, sharing mode, and image usage behavior.
- Khronos Vulkan Tutorial swapchain chapter for first implementation shape.
- Vulkan Guide WSI/swapchain topics for explanation and portability notes.
- LunarG macOS SDK and MoltenVK docs for Apple portability constraints.
- Vulkan Samples only after the first validation-clean swapchain exists.

MoltenVK notes explain Apple constraints. They are not the cross-platform source of truth.

## Scope

In scope:

- surface support checks;
- surface capability query;
- surface format/color-space selection;
- present mode selection;
- extent selection;
- image count policy;
- image usage policy;
- graphics/present queue sharing mode;
- swapchain image view creation/destruction;
- old swapchain handoff;
- resize/minimize handling;
- suboptimal/out-of-date handling;
- depth resource coupling;
- dynamic rendering pipeline format coupling;
- platform diagnostics for macOS/MoltenVK, Linux, and Windows.

Out of scope:

- window/event polling;
- native surface creation;
- physical device selection beyond required swapchain support gates;
- command buffer recording;
- frame synchronization internals;
- GPU memory allocator policy beyond depth coupling;
- runtime or projection behavior.

## Local File Surface

Likely future files:

```text
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/InstanceDeviceSurface.hpp
src/render/vulkan/InstanceDeviceSurface.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/VulkanBackend.cpp
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
```

`Swapchain` owns `VkSwapchainKHR`, swapchain images as borrowed handles, image views, selected format/color space/present mode/extent, and recreate bookkeeping.

## Ownership

| Item | Owner | Notes |
| --- | --- | --- |
| `VkSurfaceKHR` | Vulkan backend surface owner | created through platform shell provider, not by swapchain |
| `VkSwapchainKHR` | `Swapchain` | destroyed before surface/device teardown |
| swapchain images | Vulkan implementation | retrieved/borrowed, not destroyed manually |
| swapchain image views | `Swapchain` | destroyed before swapchain |
| selected format/color space | `Swapchain` | consumed by pipeline/dynamic rendering |
| selected extent | `Swapchain` | consumed by viewport/scissor/depth |
| depth resources | resource module | extent coupled to swapchain |
| frame sync objects | `FrameSync` | not swapchain-owned |

Rules:

- surface must outlive swapchain;
- device must outlive swapchain and image views;
- swapchain image views must not outlive the swapchain they came from;
- old swapchain resources cannot be used after recreate begins;
- runtime/projection never see swapchain handles or WSI result codes.

## Preconditions

Before creating a swapchain:

- Vulkan instance exists;
- platform surface exists;
- physical device supports presentation to this surface;
- logical device has required swapchain extension enabled;
- graphics queue family is known;
- present queue family is known;
- surface capabilities/formats/present modes have been queried;
- drawable extent is nonzero;
- sync/lifetime policy for recreate is known.

If drawable extent is zero, do not create a swapchain. Return diagnosed `not_drawable`.

## Surface Support Query

Device selection must prove:

- physical device supports required swapchain extension;
- at least one queue family supports graphics;
- at least one queue family supports present for the surface;
- at least one surface format is available;
- at least one present mode is available;
- surface capabilities provide a valid image extent range.

Swapchain creation must re-query surface capabilities before initial creation and before every recreate. Surface capabilities can change after resize, display movement, compositor changes, or platform events.

Diagnostics:

```text
surface_support=true|false
graphics_queue_family=
present_queue_family=
surface_format_count=
present_mode_count=
surface_min_extent=
surface_max_extent=
surface_current_extent=
```

## Format And Color Space Selection

Preferred first format policy:

```text
preferred_format=VK_FORMAT_B8G8R8A8_SRGB
preferred_color_space=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
fallback_policy=first_supported_srgb_then_first_supported
```

Selection order:

1. Prefer `B8G8R8A8_SRGB` with `SRGB_NONLINEAR`.
2. Accept another SRGB 8-bit color format with `SRGB_NONLINEAR` if available.
3. Accept first supported format only if diagnosed and first-room smoke still validates.

Rules:

- selected format is renderer presentation policy, not material truth;
- selected format must be printed in diagnostics;
- pipeline creation must use the selected color format for dynamic rendering;
- if format changes during recreate, dependent pipelines must be recreated or validated compatible;
- color-space fallback must be explicit.

Open detail: exact fallback order may be refined by device/platform diagnostics.

## Present Mode Selection

Preferred first policy:

```text
default_present_mode=VK_PRESENT_MODE_FIFO_KHR
low_latency_present_mode=optional_mailbox_when_requested_and_available
```

Rationale:

- FIFO is required by Vulkan WSI and is the safest cross-platform baseline;
- MAILBOX can be added as an opt-in low-latency mode after smoke is stable;
- IMMEDIATE may tear and should not be default.

Selection order:

1. Use FIFO by default.
2. If app requests low latency and MAILBOX is available, use MAILBOX.
3. If requested mode is unavailable, fall back to FIFO with diagnostics.

Rules:

- present mode is presentation policy only;
- present mode must not affect runtime tick determinism;
- present mode changes may require swapchain recreate;
- strict smoke should prefer FIFO unless testing a specific mode.

## Extent Selection

Extent comes from surface capabilities and platform drawable size.

Rules:

- if surface reports fixed current extent, use it;
- otherwise clamp platform drawable size to min/max supported extent;
- zero width or height means `drawable=false` and no swapchain creation;
- extent must be printed in diagnostics;
- viewport/scissor use the selected swapchain extent for first-room rendering;
- runtime camera truth does not depend on swapchain extent.

Diagnostic fields:

```text
requested_drawable_extent=
selected_swapchain_extent=
drawable=true|false
extent_source=surface_current|platform_drawable_clamped
```

## Image Count Policy

Preferred first policy:

```text
requested_image_count=min_image_count_plus_one
max_image_count_respected=true
frames_in_flight=2
```

Rules:

- request `minImageCount + 1` where allowed;
- clamp to `maxImageCount` when max is nonzero;
- image count is swapchain image count, not frames-in-flight;
- frames-in-flight remains `2` for first implementation;
- diagnostics must print both values.

This avoids conflating swapchain buffering with CPU/GPU frame-slot policy.

## Image Usage Policy

First-room required usage:

```text
color_attachment=true
transfer_src=optional_for_screenshot_or_pixel_readback
transfer_dst=optional_for_clear_or_debug_tools
```

Rules:

- swapchain images must support color attachment usage;
- optional transfer usage must be checked before screenshot/pixel-readback features rely on it;
- unsupported optional usage disables that feature with diagnostics, not runtime mutation;
- image usage flags must match command recording expectations.

## Queue Family Sharing Mode

If graphics and present queue family are the same:

```text
sharing_mode=exclusive
```

If they differ, use the documented first policy:

```text
sharing_mode=concurrent_for_first_implementation
```

Recommended first policy:

- use exclusive when graphics and present are the same;
- use concurrent when they differ for the first implementation;
- defer explicit queue-family ownership transfers until profiling or platform evidence requires them.

Rules:

- selected sharing mode must be printed;
- differing queue families must not be ignored;
- if exclusive is used with differing queues, ownership transfer barriers must be designed in sync/command file plans first.

## Creation Order

Recommended swapchain creation order:

1. Query surface capabilities.
2. Query surface formats.
3. Query present modes.
4. Select format/color space.
5. Select present mode.
6. Select extent.
7. Select image count.
8. Select image usage flags.
9. Select sharing mode.
10. Fill `VkSwapchainCreateInfoKHR`.
11. Set `oldSwapchain` when recreating and safe to do so.
12. Create swapchain.
13. Retrieve swapchain images.
14. Create image views.
15. Store selected metadata for pipeline/resource/diagnostics consumers.
16. Trigger depth resource creation for selected extent.
17. Validate pipeline compatibility with color/depth formats.

Failure cleanup follows [lifetime.md](lifetime.md).

## Image View Policy

For each swapchain image:

- create one 2D color image view;
- use the selected swapchain format;
- use color aspect;
- store image and image view in swapchain-owned arrays;
- destroy all image views before destroying swapchain.

Rules:

- image views are recreated every swapchain recreate;
- command recording uses image view for acquired image index only;
- old image views are invalid after recreate;
- image view count must match swapchain image count.

## Depth Coupling

Depth resources are not swapchain-owned, but their extent is swapchain-coupled.

Rules:

- depth extent equals selected swapchain extent;
- depth format comes from resource model/depth selection;
- depth resources are destroyed before swapchain image views if lifetime plan requires it;
- depth resources are recreated after new swapchain extent is selected;
- if depth format changes, pipeline compatibility must be checked.

Diagnostics:

```text
depth_extent_matches_swapchain=true|false
depth_recreated_with_swapchain=true|false
```

## Dynamic Rendering Coupling

Dynamic rendering pipeline creation needs:

```text
color_format=selected_swapchain_format
depth_format=selected_depth_format
```

Rules:

- pipeline creation cannot happen without selected swapchain color format unless using deferred/lazy pipeline creation;
- pipeline must be recreated or validated compatible if color/depth format changes;
- command recording must use attachment formats matching pipeline metadata;
- render-pass fallback must create compatible render pass/framebuffers and diagnose `rendering_path=render_pass`.

Swapchain does not own pipeline objects, but it owns the metadata that pipeline creation depends on.

## Recreate Triggers

Swapchain must be marked dirty on:

- platform drawable resize;
- framebuffer size change;
- acquire returns out of date;
- present returns out of date;
- surface reports suboptimal when policy requires recreate;
- present mode change;
- color format/color space change request;
- surface capabilities change that invalidates extent/image count;
- full backend recreation after surface invalidation.

Suboptimal policy:

- interactive demo may draw and schedule recreate soon;
- strict smoke may accept suboptimal only if diagnosed and frame presents;
- repeated suboptimal across resize smoke should force recreate.

## Recreate Order

Recommended recreate order:

1. Mark `swapchain_dirty=true`.
2. If drawable size is zero, set `drawable=false`, skip recreate, and return diagnosed skip.
3. Stop submitting frames using old swapchain.
4. Wait using selected resize sync policy.
5. Re-query surface capabilities/formats/present modes.
6. Select new format/present mode/extent/image count/sharing mode.
7. Create new swapchain with old swapchain handle when safe.
8. Destroy old image views.
9. Destroy old swapchain.
10. Retrieve new images.
11. Create new image views.
12. Recreate depth resources for new extent.
13. Recreate or validate pipelines if format/depth changed.
14. Clear `swapchain_dirty`.
15. Print recreate diagnostics.

If new swapchain creation fails:

- keep old swapchain only if ownership and validity are proven;
- otherwise enter `not_drawable` or renderer failed state;
- strict smoke fails with receipt;
- runtime/projection state remains unchanged.

## Minimize And Zero Drawable

Zero drawable policy:

```text
drawable=false
swapchain_created=false_or_existing_paused
render_status=skipped_not_drawable
```

Rules:

- do not create swapchain with zero extent;
- do not acquire/present while drawable is zero;
- renderer may keep existing swapchain paused if platform behavior allows it, but must not draw;
- restore path re-queries capabilities before recreate;
- app decides whether runtime ticks while minimized.

## Surface Invalidation

Surface recreation is not normal swapchain recreation.

If platform reports surface/window invalid:

- stop rendering;
- wait for in-flight work;
- destroy swapchain-dependent resources;
- destroy old surface through backend surface owner;
- create a new surface through platform shell provider;
- re-run surface support/device compatibility checks;
- recreate swapchain.

First implementation may use full backend recreation instead. That policy must be diagnosed:

```text
surface_recreate_policy=full_backend_recreate
```

## Platform Notes

macOS/MoltenVK:

- surface is backed through MoltenVK/Metal via SDL provider;
- portability requirements are instance/device diagnostics, not runtime truth;
- preferred format/present mode may differ from native Vulkan platforms and must be printed;
- resize and layer drawable size behavior must be validated locally.

Linux:

- Wayland and X11 WSI behavior may differ;
- platform diagnostics must print active WSI backend;
- do not assume X11-only behavior;
- compositor resize/minimize must be tested or documented;
- native Vulkan validation lane must run swapchain smoke.

Windows:

- Win32 surface is tied to window handle lifetime;
- DPI and framebuffer extent may differ from logical window size;
- minimize/restore and alt-tab must be tested;
- installed Vulkan loader/runtime availability belongs in packaging/platform diagnostics.

Cross-platform rule: platform differences may affect selected format, present mode, extent, or recreate timing, but not runtime truth.

## Diagnostics Receipt Fields

Swapchain diagnostics should include:

```text
swapchain_created=true|false
swapchain_dirty=true|false
swapchain_recreated=true|false
swapchain_recreate_count=
swapchain_recreate_reason=
swapchain_recreate_result=
surface_support=true|false
surface_format_count=
present_mode_count=
selected_format=
selected_color_space=
selected_present_mode=
requested_drawable_extent=
selected_swapchain_extent=
min_image_count=
max_image_count=
selected_image_count=
frames_in_flight=
image_usage_flags=
graphics_queue_family=
present_queue_family=
sharing_mode=
old_swapchain_used=true|false
image_view_count=
depth_extent_matches_swapchain=true|false
pipeline_format_compatible=true|false
drawable=true|false
wsi_backend=
platform=
reason=
```

Use `unavailable` only when the field does not apply.

## Failure Reason Codes

Recommended reason codes:

```text
surface_support_missing
swapchain_extension_missing
surface_capabilities_query_failed
surface_formats_empty
present_modes_empty
swapchain_extent_zero
swapchain_format_unsupported
present_mode_fallback
swapchain_create_failed
swapchain_image_query_failed
swapchain_image_view_create_failed
swapchain_recreate_failed
swapchain_out_of_date
swapchain_suboptimal
old_swapchain_invalid
queue_family_sharing_unsupported
depth_recreate_failed
pipeline_format_mismatch
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during implementation.

## Validation Expectations

Validation blockers:

- creating swapchain without present support;
- using unsupported image usage flags;
- ignoring required min/max image counts;
- invalid extent;
- image views using wrong format/aspect;
- using old swapchain images/views after recreate;
- presenting image in wrong layout;
- destroying swapchain while work/present may still reference it;
- pipeline attachment format mismatch.

Strict smoke fails on validation errors.

## Tests

Future tests:

```text
tests/smoke/vulkan_swapchain_smoke.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_empty_frame_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

Smoke expectations:

- surface support is reported;
- swapchain creates validation-clean;
- image views create validation-clean;
- selected format/present mode/extent/image count print in receipt;
- empty clear frame can acquire/present;
- resize recreate does not use old image views;
- zero drawable skip is diagnosed;
- pipeline format compatibility is reported before first-room draw.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_swapchain|vulkan_resize|vulkan_empty_frame|vulkan_first_room|vulkan_sync'
```

Platform validation:

- macOS/MoltenVK: local first lane;
- Linux: native Vulkan display-backed lane or documented WSI strategy;
- Windows: native Vulkan lane with minimize/restore and loader/runtime checks.

## Acceptance Criteria

This swapchain contract is ready for file plans when:

- surface support requirements are explicit;
- format/color-space selection policy is defined;
- present mode policy is defined;
- extent and zero-drawable behavior are defined;
- image count policy is defined separately from frames-in-flight;
- image usage policy is defined;
- queue family sharing policy is defined;
- image view ownership/lifetime is defined;
- depth and pipeline format coupling are defined;
- recreate triggers and order are defined;
- suboptimal/out-of-date behavior is defined;
- diagnostics and reason codes are defined;
- macOS/MoltenVK, Linux, and Windows lanes are included;
- swapchain failure cannot mutate runtime truth.

## Open Detail Items

These belong in future file plans:

- exact format fallback order after first platform diagnostics;
- exact present-mode CLI flag and fallback wording;
- exact swapchain create info wrapper;
- exact old-swapchain handoff implementation;
- exact image view wrapper/destructor pattern;
- exact zero-drawable event handling per SDL platform;
- exact queue-family ownership transfers if concurrent sharing is rejected;
- exact depth recreate helper;
- exact pipeline recreate trigger helper;
- exact Linux/Windows swapchain smoke commands.
