# Vulkan Resize Minimize Test Plan

This document defines the focused Vulkan WSI test plan for resize storms, minimize/restore, zero extent, DPI/display-scale changes, display movement, and bounded repeated swapchain recreates in `iggy3d`.

This is a test plan, not a second swapchain policy. The behavior rules live in [vulkan_swapchain_failure_modes.md](vulkan_swapchain_failure_modes.md), [swapchain_contract.md](swapchain_contract.md), [render_loop.md](render_loop.md), and [sync_contract.md](sync_contract.md). This document turns those rules into concrete tests and receipts.

## Purpose

Define stress proof for the unstable WSI surfaces:

```text
resize_storms=covered
minimize_restore=covered
zero_extent=covered
dpi_scale_change=covered
display_movement=covered
repeated_recreate_limit=covered
runtime_mutation_from_resize=false
fence_deadlock_on_resize=false
strict_resize_lane=must_pass_or_fail
optional_resize_lane=may_skip_missing_display_only
```

The renderer may skip, recreate, or fail a frame during resize/minimize handling. It must not mutate runtime truth, save data, replay results, command legality, camera truth, package validation, or projection semantics.

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Khronos Vulkan Tutorial swapchain recreation: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/04_Swap_chain_recreation.html | first implementation shape, out-of-date/suboptimal handling, explicit resize flag, minimization, fence reset deadlock avoidance |
| Khronos Vulkan Tutorial swapchain chapter: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html | surface extent selection and window-pixel sizing |
| Vulkan Specification/Registry: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html | exact WSI return codes, swapchain creation rules, surface capability behavior |
| SDL3 high DPI README: https://wiki.libsdl.org/SDL3/README-highdpi | pixel size versus content/display scale behavior for SDL shell |
| Project swapchain/sync/result docs | local out-of-date/suboptimal, zero extent, fence reset, skip/fail, and diagnostics policy |

Priority rule:

```text
wsi_api_truth=Vulkan Specification and reference pages
first_recreate_shape=Khronos Vulkan Tutorial
dpi_shell_truth=SDL3 docs for SDL shell
project_behavior=iggy3d swapchain/sync/result docs
runtime_truth=iggy3d runtime docs, never WSI tests
```

## Scope

In scope:

- automated and manual resize smoke tests;
- resize storm test cases;
- minimize/restore tests;
- zero-extent tests;
- DPI/display-scale tests;
- moving between displays;
- repeated recreate limit and loop detection;
- out-of-date/suboptimal recovery receipts;
- fence/semaphore deadlock checks;
- strict versus optional lane behavior;
- runtime hash/replay invariance during resize tests.

Out of scope:

- implementing tests now;
- implementing native platform event injection now;
- multi-window rendering;
- fullscreen-exclusive mode;
- HDR/color-management policy beyond diagnostics;
- user graphics settings UI;
- render-thread resize message queues;
- texture/material reallocation stress.

## Local File Surface

Likely future files:

```text
apps/iggy3d_visual_demo/main.cpp
src/app/platform/SdlWindow.hpp
src/app/platform/SdlWindow.cpp
src/app/platform/SdlVulkanSurface.hpp
src/app/platform/SdlVulkanSurface.cpp
src/render/FrameInput.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_resize_policy_tests.cpp
tests/unit/render_zero_extent_policy_tests.cpp
tests/smoke/vulkan_resize_smoke.cpp
tests/smoke/vulkan_resize_storm_smoke.cpp
tests/smoke/vulkan_minimize_restore_smoke.cpp
tests/smoke/vulkan_dpi_change_smoke.cpp
tests/smoke/vulkan_display_move_smoke.cpp
tests/smoke/vulkan_recreate_limit_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| raw window events | platform shell/app | runtime truth |
| drawable pixel extent | platform shell/app frame assembly | swapchain policy authority |
| display scale/DPI data | platform shell diagnostics | gameplay scale |
| resize dirty flag | renderer/swapchain coordinator | runtime command legality |
| recreate counter | swapchain diagnostics | save/replay state |
| zero-extent skip | renderer result policy | runtime tick policy |
| strict/optional exit | smoke test wrapper | renderer core |
| runtime hash comparison | test harness | renderer behavior control |

Rules:

- platform shell reports window/drawable/display facts;
- swapchain decides whether to recreate;
- sync owns fence/semaphore safety around early exits;
- runtime does not know WSI result codes;
- renderer resize behavior must not change deterministic runtime hash.

## Required Test Controls

Future smoke tests need a small app/test control surface. These are test hooks, not user-facing gameplay features.

Suggested CLI flags:

```text
--vulkan-resize-script=<name>
--vulkan-resize-iterations=<n>
--vulkan-resize-min-width=<px>
--vulkan-resize-min-height=<px>
--vulkan-resize-max-width=<px>
--vulkan-resize-max-height=<px>
--vulkan-resize-frame-delay=<n>
--vulkan-minimize-restore
--vulkan-force-zero-extent-test
--vulkan-recreate-limit=<n>
--vulkan-dpi-diagnostics
--vulkan-display-move-diagnostics
```

Rules:

- test hooks belong to visual demo/smoke apps only;
- headless runtime tools must not accept or require these flags;
- resize scripts must not mutate runtime directly;
- tests may use platform APIs or SDL APIs to request window size changes only from app/platform code;
- if a platform cannot automate a case, optional lane may skip with `77`, strict lane must fail only when the case is required for that platform lane.

## Test Lane Summary

| Test | macOS/MoltenVK | Linux native Vulkan | Windows native Vulkan | Software Vulkan |
| --- | --- | --- | --- | --- |
| create/destroy swapchain | strict platform proof | strict platform proof | strict platform proof | optional aid |
| simple resize | strict platform proof | strict platform proof | strict platform proof | optional aid |
| resize storm | strict after first-room smoke | strict after first-room smoke | strict after first-room smoke | optional aid |
| minimize/restore | strict if platform automation works | strict if display/session supports it | strict if platform automation works | optional/skip |
| zero extent | strict simulated policy test plus platform if possible | strict simulated policy test plus platform if possible | strict simulated policy test plus platform if possible | unit/optional |
| DPI/display scale | diagnostic initially, strict later | diagnostic initially, strict later | diagnostic initially, strict later | optional/skip |
| display movement | manual/diagnostic initially | manual/diagnostic initially | manual/diagnostic initially | optional/skip |

Rules:

- Linux and Windows native Vulkan lanes are required for shipping proof;
- software Vulkan cannot replace native resize/minimize proof;
- platform automation limitations must be reported as `unsupported_test_control`, not as pass;
- a manual diagnostic lane may exist, but it does not replace strict automated smoke where automation is required.

## Test 01: Simple Resize

Purpose: prove one resize triggers one safe recreate and returns to presenting.

Setup:

```text
start_window=1280x720
resize_to=1024x768
frames_before_resize=3
frames_after_resize=5
strict_vulkan=true
validation=required
sync_validation=required
```

Expected behavior:

- platform shell reports resize event or changed drawable extent;
- swapchain dirty flag is set;
- renderer recreates at a safe point;
- no fence deadlock;
- no validation/sync validation errors;
- rendering resumes with nonzero drawable extent;
- runtime hash before/after renderer resize path is unchanged.

Required receipt fields:

```text
test_case=simple_resize
resize_event_count=1
drawable_extent_before=
drawable_extent_after=
swapchain_extent_before=
swapchain_extent_after=
swapchain_recreate_count=1
frames_presented_after_recreate=
validation_error_count=0
sync_validation_clean=true
runtime_hash_before=
runtime_hash_after=
```

Pass criteria:

- `swapchain_recreate_count >= 1`;
- final drawable extent is nonzero;
- at least one frame presents after recreate;
- runtime hash unchanged;
- no validation/sync errors.

## Test 02: Resize Storm

Purpose: prove rapid resize events are coalesced and do not cause recreate churn, deadlock, or runtime mutation.

Setup:

```text
resize_sequence=alternating_small_large
resize_iterations=30_initial
resize_min=640x360
resize_max=1600x900
frame_delay=0_or_1
recreate_limit=10_initial
```

Expected behavior:

- platform may emit many raw events;
- renderer coalesces dirty state;
- recreate happens only at safe points;
- repeated out-of-date/suboptimal is diagnosed;
- renderer stops with a stable failure if recreate limit is exceeded;
- runtime hash remains unchanged.

Required receipt fields:

```text
test_case=resize_storm
resize_event_count=
drawable_extent_change_count=
swapchain_dirty_count=
swapchain_recreate_count=
recreate_limit=
recreate_limit_exceeded=true|false
out_of_date_count=
suboptimal_count=
frames_presented=
frames_skipped_not_drawable=
max_consecutive_recreate_attempts=
runtime_hash_before=
runtime_hash_after=
```

Pass criteria:

- no deadlock;
- no device loss;
- no sync validation errors;
- recreate count stays at or under configured limit;
- at least one final frame presents after storm;
- runtime hash unchanged.

Failure criteria:

- endless recreate loop;
- fence wait timeout caused by skipped submit;
- command buffer reset before safe fence;
- strict lane silently skips;
- runtime hash changes.

## Test 03: Minimize And Restore

Purpose: prove minimized/non-drawable windows skip before acquire and restore cleanly.

Setup:

```text
start_drawable=true
minimize_window=true
hold_minimized_frames=5
restore_window=true
frames_after_restore=5
```

Expected behavior:

- platform reports minimized or zero drawable extent;
- renderer returns a diagnosed skip before acquire;
- swapchain is not recreated while extent is zero;
- fence is not reset for skipped frames with no submit;
- restore marks swapchain dirty if needed;
- renderer recreates once drawable extent is nonzero;
- presentation resumes.

Required receipt fields:

```text
test_case=minimize_restore
minimize_event_count=
restore_event_count=
zero_extent_frame_count=
acquire_attempts_while_zero_extent=0
submit_attempts_while_zero_extent=0
fence_resets_while_zero_extent=0
swapchain_recreate_count=
frames_presented_after_restore=
runtime_hash_before=
runtime_hash_after=
```

Pass criteria:

- no acquire while extent is zero;
- no submit while extent is zero;
- no fence reset for skipped zero-extent frame;
- presentation resumes after restore;
- runtime hash unchanged.

## Test 04: Forced Zero Extent Policy

Purpose: prove zero extent behavior even on platforms where minimize cannot be automated reliably.

Setup:

```text
inject_drawable_extent=0x0
inject_policy_layer=platform_shell_test_double_or_renderer_test_hook
frames=3
```

Expected behavior:

- `FrameInput.viewport` or platform drawable reports zero extent;
- renderer returns `SkippedNotDrawable`;
- swapchain creation/recreation is not attempted with zero extent;
- no acquire/present occurs;
- diagnostics say `drawable=false`.

Required receipt fields:

```text
test_case=forced_zero_extent
drawable=false
requested_drawable_extent=0x0
swapchain_create_attempted=false
swapchain_recreate_attempted=false
acquire_attempted=false
submit_attempted=false
present_attempted=false
result=skip|pass
reason_code=not_drawable
```

Pass criteria:

- zero extent is handled without Vulkan validation errors;
- no zero-sized swapchain is attempted;
- no runtime mutation occurs.

## Test 05: DPI / Display Scale Change

Purpose: prove the renderer distinguishes logical window size from drawable pixel size and diagnoses scale changes.

Setup:

```text
start_display_scale=platform_current
trigger_scale_change=move_or_platform_setting_or_manual
record_window_size_points=true
record_drawable_size_pixels=true
record_display_scale=true
```

Expected behavior:

- platform shell records logical size, drawable pixel size, and display scale when available;
- swapchain extent follows drawable pixel size/surface capabilities, not logical UI points;
- resize/recreate occurs if drawable pixel extent changes;
- camera aspect uses drawable/render viewport policy, not stale logical size;
- runtime world scale is unchanged.

Required receipt fields:

```text
test_case=dpi_scale_change
window_size=
window_size_pixels=
display_scale=
pixel_density=
drawable_extent_before=
drawable_extent_after=
swapchain_extent_before=
swapchain_extent_after=
aspect_before=
aspect_after=
swapchain_recreate_count=
runtime_hash_before=
runtime_hash_after=
```

Initial pass criteria:

- diagnostic fields are present;
- selected swapchain extent matches drawable/surface capabilities;
- no runtime hash change;
- no validation/sync errors.

Strict automation gate:

- mark this test diagnostic-only until platform automation for display-scale change is proven;
- once automation exists, strict lane requires at least one real scale/extent change or a stable `unsupported_test_control` failure.

## Test 06: Display Movement

Purpose: prove moving a window between displays is diagnosed and swapchain-dependent state recovers if surface capabilities change.

Setup:

```text
start_display=A
move_window_to_display=B
record_display_id_or_name=true
record_surface_capabilities_before_after=true
```

Expected behavior:

- platform shell reports display move or changed display association when available;
- drawable extent/display scale may change;
- swapchain surface capabilities are re-queried before recreate;
- format/color space/present mode changes are diagnosed;
- dependent pipeline/depth compatibility is rechecked if formats change.

Required receipt fields:

```text
test_case=display_movement
display_before=
display_after=
display_change_detected=true|false|unavailable
surface_capabilities_changed=true|false|unavailable
drawable_extent_before=
drawable_extent_after=
swapchain_format_before=
swapchain_format_after=
present_mode_before=
present_mode_after=
depth_format_before=
depth_format_after=
pipeline_recreate_required=true|false|unavailable
runtime_hash_before=
runtime_hash_after=
```

Initial pass criteria:

- diagnostic fields are present;
- if display change is unavailable to automation, test reports `unsupported_test_control`;
- if surface capabilities change, recreate path is validation-clean;
- runtime hash unchanged.

## Test 07: Repeated Recreate Limit

Purpose: prove the renderer fails with a useful diagnostic instead of spinning forever when recreate cannot stabilize.

Setup:

```text
force_recreate_dirty_each_frame=true
recreate_limit=5_initial
frames=10
strict_vulkan=true
```

Expected behavior:

- renderer attempts bounded recreates;
- after the configured limit, renderer returns failure;
- reason code is stable;
- no runtime mutation occurs;
- receipt includes the last known surface/swapchain facts.

Required receipt fields:

```text
test_case=recreate_limit
swapchain_recreate_count=
recreate_limit=
recreate_limit_exceeded=true
last_recreate_reason=
last_drawable_extent=
last_surface_current_extent=
last_present_mode=
last_swapchain_format=
result=fail
reason_code=recreate_limit_exceeded
runtime_hash_before=
runtime_hash_after=
```

Pass criteria:

- failure is bounded and diagnosed;
- process exits `1` in strict lane;
- optional lane also fails because this is a renderer bug simulation, not missing environment;
- runtime hash unchanged.

## Test 08: Out-Of-Date And Suboptimal Injection

Purpose: prove result-policy handling independent of platform resize automation.

Setup:

```text
inject_acquire_result=VK_ERROR_OUT_OF_DATE_KHR
inject_present_result=VK_SUBOPTIMAL_KHR
inject_present_result_later=VK_ERROR_OUT_OF_DATE_KHR
```

Expected behavior:

- acquire out-of-date skips without submit/present/fence reset;
- present suboptimal accepts the submitted frame and marks recreate soon;
- present out-of-date records that submit already happened and recreates later;
- device loss is not misclassified as swapchain recreate.

Required receipt fields:

```text
test_case=wsi_result_injection
injected_acquire_result=
injected_present_result=
submit_happened=true|false
present_happened=true|false
fence_reset=true|false
swapchain_recreate_requested=true|false
reason_code=
```

Pass criteria:

- result mapping matches [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md);
- fence reset safety matches [sync_contract.md](sync_contract.md);
- no runtime mutation occurs.

## Recreate Limit Policy

Initial limits:

```text
max_recreates_per_10_seconds=10
max_consecutive_recreate_attempts=5
max_resize_events_per_test=100
```

Rules:

- limits are test/app safeguards, not gameplay rules;
- a clean resize storm should stay under limits after coalescing;
- exceeding a limit in strict smoke is a failure with `recreate_limit_exceeded`;
- optional smoke must not hide recreate loops as skipped tests;
- receipt must include event count, dirty count, recreate count, and final state.

These values are starting test thresholds. They may be tuned after platform evidence, but the doc must be updated before the implementation silently changes them.

## Expected Reason Codes

Resize/minimize reason codes:

```text
resize_requested
resize_coalesced
not_drawable
minimized
restored
zero_extent
surface_out_of_date
surface_suboptimal
surface_capabilities_changed
dpi_scale_changed
display_changed
recreate_started
recreate_succeeded
recreate_limit_exceeded
unsupported_test_control
resize_test_timeout
fence_reset_without_submit
acquire_attempted_while_zero_extent
submit_attempted_while_zero_extent
runtime_hash_changed
```

Rules:

- reason codes are stable machine-readable strings;
- pass receipts may include success reason codes for traceability;
- fail/skip receipts must include a reason code;
- missing platform automation is `unsupported_test_control`, not generic failure.

## Required Receipt Fields

Every resize/minimize smoke receipt should include:

```text
receipt_version=1
test_case=
platform=
platform_lane=
window_shell=sdl3|glfw|native|unavailable
thread_model=
strict_vulkan=true|false
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
window_size=
window_size_pixels=
display_scale=
pixel_density=
drawable=
drawable_extent_before=
drawable_extent_after=
swapchain_extent_before=
swapchain_extent_after=
swapchain_recreate_count=
swapchain_dirty_count=
resize_event_count=
minimize_event_count=
restore_event_count=
zero_extent_frame_count=
out_of_date_count=
suboptimal_count=
frames_presented=
frames_skipped_not_drawable=
acquire_attempts_while_zero_extent=
submit_attempts_while_zero_extent=
fence_resets_without_submit=
recreate_limit=
recreate_limit_exceeded=
result=pass|fail|skip
reason_code=
runtime_hash_before=
runtime_hash_after=
```

Rules:

- unavailable platform facts use `unavailable`;
- zero counters must be printed as `0`;
- final selected swapchain facts must be printed after recovery;
- runtime hash fields are required for tests that start runtime simulation.

## Strict And Optional Behavior

Optional lane may skip only when:

- Vulkan loader/driver/display is unavailable;
- platform automation for a specific case is unavailable;
- test requires multiple displays but only one is available;
- test requires scale-change automation that the platform shell cannot provide yet.

Optional lane must fail when:

- renderer deadlocks;
- validation/sync validation catches an error in an attempted test;
- recreate limit is exceeded in a simulated renderer-bug test;
- runtime hash changes;
- zero extent triggers acquire/submit/present.

Strict lane must fail when:

- required display/GPU/platform support is missing for the lane;
- required validation/sync validation cannot be enabled;
- required resize/minimize test case cannot run after being promoted from diagnostic to strict;
- any pass criteria above fail.

## Manual Test Notes

Manual tests are allowed early for display movement and DPI changes, but they must print receipts.

Manual test prompts should ask the tester to:

- resize the window repeatedly for 10 seconds;
- minimize and restore the window;
- move the window between displays;
- move between displays with different scale factors if available;
- confirm the app continues rendering after each action.

Manual tests do not replace automated smoke for strict CI. They are evidence-gathering lanes until automation exists.

## Test Order

Recommended implementation order:

1. Unit policy tests for zero extent and result injection.
2. Simple resize smoke.
3. Minimize/restore smoke.
4. Resize storm smoke.
5. Recreate limit forced-failure smoke.
6. DPI diagnostic smoke.
7. Display movement diagnostic smoke.
8. Promote DPI/display tests to strict only after automation is reliable on macOS, Linux, and Windows lanes.

Do not start with display movement. It is platform-flaky and should not block basic swapchain stability proof.

## Review Checklist

Reviewer should reject a resize/minimize test packet if:

- it recreates swapchain on every raw resize event without coalescing;
- it attempts swapchain creation with zero extent;
- it acquires or submits while drawable extent is zero;
- it resets a fence before a path that may skip submit;
- it hides strict failures as optional skips;
- it lacks final receipt fields;
- it mutates runtime to recover from resize;
- it requires software Vulkan as shipping proof;
- it ignores Linux or Windows native Vulkan lanes.

## Acceptance Criteria

This test plan is ready for implementation when:

- [ ] future smoke tests map to the cases above;
- [ ] zero extent is covered by automated unit/policy test;
- [ ] simple resize is covered by strict smoke;
- [ ] resize storm has bounded recreate limits;
- [ ] minimize/restore has platform or simulated coverage;
- [ ] DPI/display movement diagnostics exist before strict promotion;
- [ ] receipts include resize/minimize/DPI/display fields;
- [ ] runtime hash is checked where runtime simulation runs;
- [ ] strict and optional skip/fail behavior is explicit;
- [ ] Linux and Windows native Vulkan lanes remain first-class proof targets.

## Non-Goals For First Test Pass

Do not add:

- fullscreen-exclusive tests;
- HDR format migration tests;
- multi-window swapchain tests;
- render-thread resize queues;
- swapchain maintenance extension tests;
- texture/material resource stress;
- asset streaming during resize;
- automated RenderDoc capture during resize;
- UI layout scaling policy.
