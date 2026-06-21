# Vulkan Screenshot And Frame Hash

This document defines the first-room visual proof policy for screenshot artifacts, readback, normalized frame hashes, tolerance checks, and software-versus-hardware differences.

Screenshots and hashes are renderer diagnostics. They prove that a Vulkan frame was drawn and captured; they do not become runtime truth, save truth, replay truth, content validation, or gameplay authority.

## Purpose

Define the first visual evidence contract:

```text
first_room_visual_proof=true
screenshot_artifact=required_for_screenshot_smoke
frame_hash=diagnostic_and_lane_specific
byte_exact_cross_gpu_hash=false
software_vulkan_hash=separate_baseline
hardware_hash=per_platform_tolerant
visible_room_rendered=geometry_marker_depth_receipt_and_image_coverage
runtime_mutation_from_capture=false
```

This document narrows:

- [first_room_render_contract.md](first_room_render_contract.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_resize_minimize_test_plan.md](vulkan_resize_minimize_test_plan.md)
- [vulkan_texture_format_policy.md](vulkan_texture_format_policy.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)
- [vulkan_debug_labels_and_capture.md](vulkan_debug_labels_and_capture.md)
- [platform_matrix.md](platform_matrix.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Specification copy commands: https://docs.vulkan.org/spec/latest/chapters/copies.html | image-to-buffer copy behavior, transfer-operation synchronization expectations |
| `vkCmdCopyImageToBuffer` reference: https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdCopyImageToBuffer.html | exact command requirements and valid usage for screenshot readback |
| Vulkan Specification resource creation: https://docs.vulkan.org/spec/latest/chapters/resources.html | image usage flags, format behavior, buffer/image creation requirements |
| Project image layout policy | source image transition into transfer source and back to presentation or shader use |
| Project diagnostics and CI docs | artifact locations, strict/optional lane policy, receipt fields, reason codes |
| Project first-room contract | semantic definition of visible room, player marker, and runtime invariance |

Priority rule:

```text
exact_copy_behavior=Vulkan Specification and Reference Pages
artifact_and_test_policy=iggy3d diagnostics and CI docs
visible_room_semantics=iggy3d first-room contract
runtime_truth=iggy3d runtime docs, not screenshots
```

## Scope

In scope:

- screenshot capture path;
- staging/readback buffer policy;
- image layout transitions for capture;
- screenshot file naming;
- PNG versus raw RGBA artifact policy;
- normalized frame hash policy;
- visual coverage thresholds;
- tolerance rules;
- software Vulkan versus hardware Vulkan baselines;
- strict/optional lane behavior;
- receipt fields;
- tests and failure reason codes.

Out of scope:

- implementing readback before the renderer packet reaches this phase;
- external screenshot tools as primary automated proof;
- RenderDoc automation;
- UI screenshot comparison;
- final art validation;
- exact pixel-perfect cross-GPU output;
- runtime state mutation;
- save/replay image serialization.

## Local File Surface

Likely future files:

```text
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/Swapchain.hpp
src/render/vulkan/Swapchain.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
tests/unit/render_frame_hash_policy_tests.cpp
tests/unit/render_visible_room_policy_tests.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_screenshot_smoke.cpp
tests/smoke/vulkan_frame_hash_smoke.cpp
build/artifacts/render_diagnostics/screenshots/
build/artifacts/render_diagnostics/frame_hashes/
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| screenshot readback command | Vulkan command/resource modules | gameplay truth |
| readback buffer | Vulkan resource module | runtime-visible mutable state |
| PNG/raw artifacts | smoke test harness and diagnostics | source assets |
| exact frame hash | diagnostics/test harness | cross-GPU shipping truth |
| tolerant visibility score | smoke test harness | content legality |
| baseline files | test harness or artifact tooling | runtime replay outcome |
| capture reason codes | Vulkan result/diagnostics modules | command legality |

Rules:

- runtime/content/projection/save code must not read screenshot pixels or frame hashes;
- renderer diagnostics may report image proof, but cannot mutate runtime state;
- frame hashes are test evidence, not save/replay state;
- screenshot capture failure may fail or skip a renderer lane, but it must not change deterministic runtime hashes;
- picking/input proof later must route through runtime commands, not through screenshot pixels.

## Capture Path

The first reliable screenshot path should use Vulkan readback, not OS window screenshots.

Preferred path:

```text
1. Render first-room frame.
2. Transition capture source image to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
3. Copy source image to a host-visible readback buffer with vkCmdCopyImageToBuffer.
4. Transition source image back to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR when presenting.
5. Submit and wait for the capture fence or the relevant frame fence.
6. Map readback buffer only after GPU copy completion is guaranteed.
7. Normalize row layout and channel order to diagnostic RGBA8.
8. Write raw RGBA, PNG, metadata, and hash receipt.
```

The capture source may be:

- the swapchain image, if it was created with `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`;
- an offscreen color image created with transfer-source usage, then copied or blitted into the present path.

Rules:

- do not silently disable screenshot proof when the swapchain lacks transfer-source support;
- if swapchain transfer-source usage is unavailable, use an offscreen capture target or fail the strict screenshot lane with a reason code;
- readback commands must be outside dynamic rendering;
- readback must be validation-clean and synchronization-validation-clean in strict lanes;
- CPU code must not map/read the buffer until the GPU copy is complete;
- screenshot capture must not allocate new GPU resources every steady-state frame.

## Image Layout Policy

Swapchain capture path:

```text
after_render:
  oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
  copy=vkCmdCopyImageToBuffer
  finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
```

Offscreen capture path:

```text
after_render:
  color_target_layout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
  copy=vkCmdCopyImageToBuffer
  present_path=copy_or_blit_to_swapchain_then_present
```

Rules:

- the selected source image must be created with transfer-source usage;
- the readback buffer must be created with transfer-destination usage;
- source image aspect must be color only;
- row pitch and buffer-image copy layout must be normalized by the diagnostics layer before hashing;
- if MSAA is later enabled, screenshot proof must capture a resolved single-sample image.

## Artifact Paths

Default artifact root:

```text
build/artifacts/render_diagnostics/
```

Screenshot artifacts:

```text
build/artifacts/render_diagnostics/screenshots/<test_name>/<platform_lane>/<backend>/<device_slug>/<frame_index>.png
build/artifacts/render_diagnostics/screenshots/<test_name>/<platform_lane>/<backend>/<device_slug>/<frame_index>.rgba
build/artifacts/render_diagnostics/screenshots/<test_name>/<platform_lane>/<backend>/<device_slug>/<frame_index>.meta.kv
```

Frame hash artifacts:

```text
build/artifacts/render_diagnostics/frame_hashes/<test_name>/<platform_lane>/<backend>/<device_slug>/<frame_index>.hash.kv
```

Rules:

- PNG is the human artifact;
- raw RGBA is the hashing artifact;
- metadata is the reproducibility artifact;
- artifacts are build outputs and must not be written under source directories by default;
- artifacts may be retained on failure even when passing lanes clean temporary files later;
- file paths must be printed in the receipt when files are written.

## Artifact Naming Fields

Each artifact path or metadata file must identify:

```text
test_name
platform_lane
backend
device_name
device_vendor_id
device_id
driver_version
swapchain_format
swapchain_color_space
extent
shader_interface_hash
frame_index
```

`device_slug` may be a sanitized compact form, but the metadata must keep the full device and driver fields.

Rules:

- platform lane is required because macOS/MoltenVK, Linux native Vulkan, Windows native Vulkan, and software Vulkan are separate evidence classes;
- shader interface hash is required once the shader-interface contract exists;
- extent is required because hashes at different resolutions are not comparable;
- exact hash comparison across different device or driver metadata is forbidden unless a later doc defines a specific tolerance-aware hash family.

## Pixel Normalization

The hash input is normalized diagnostic pixels, not the encoded PNG.

Initial policy:

```text
normalized_format=rgba8
hash_channels=rgb
alpha_policy=record_and_ignore_for_first_room_hash
color_space_policy=store_source_format_and_color_space_do_not_linearize_initially
default_extent=640x360
msaa=disabled
blend=disabled
dithering=not_required_for_first_room
tolerance_border_pixels=1
```

Rules:

- normalize channel order to RGBA8 before writing raw data;
- handle row pitch explicitly;
- do not hash padding bytes;
- do not hash the PNG encoding;
- exact hash uses the full normalized image except padding;
- tolerant coverage checks may ignore a one-pixel border;
- first-room visual tests should use a fixed extent for reproducibility;
- if the swapchain format is BGRA, diagnostics still normalize to RGBA before hashing;
- source format and color space must be recorded in metadata.

## Frame Hashes

Required hash fields:

```text
exact_rgba_sha256=
hash_input_format=rgba8
hash_channels=rgb
hash_extent=
hash_source=raw_readback
hash_cross_gpu_comparable=false
```

Recommended diagnostic fields:

```text
downsampled_luma_hash=
coverage_hash=
semantic_visibility_score=
non_background_ratio=
player_marker_pixel_count=
room_proxy_pixel_count=
unique_color_bucket_count=
```

Policy:

- use SHA-256 for the exact normalized RGB frame hash unless a later file plan justifies another stable algorithm;
- exact hashes are useful for same-lane drift detection;
- exact hashes are not a global macOS/Linux/Windows pass criterion;
- tolerant visibility metrics are the first cross-platform pass signal;
- hash mismatches must print enough metadata to compare device, driver, format, extent, shader hash, and frame index.

## Tolerance Rules

First-room smoke must not require byte-perfect equality across:

- MoltenVK on macOS;
- native Vulkan on Linux;
- native Vulkan on Windows;
- software Vulkan;
- different GPU vendors;
- different driver versions;
- different swapchain formats;
- different extents.

Byte-exact comparison is allowed only when all of these match:

```text
platform_lane
device_vendor_id
device_id
driver_version
swapchain_format
swapchain_color_space
extent
shader_interface_hash
pipeline_family
test_fixture
frame_index
```

Initial tolerant thresholds:

```text
min_non_background_ratio=0.02
max_background_ratio=0.98
min_player_marker_pixels=16
min_room_proxy_pixels=128
min_unique_color_buckets=3
```

Rules:

- threshold values are starting points for first-room proxy geometry, not art quality gates;
- lowering thresholds requires a doc update or an implementation-plan note with evidence;
- exact hash mismatch alone must not fail a cross-platform strict lane when tolerant visibility passes;
- tolerant visibility failure must include which threshold failed;
- a screenshot that is entirely clear color is always a failure in screenshot smoke.

## Visible Room Rendered

`visible_room_rendered=true` means all required proof signals agree.

Strict visible-room criteria:

```text
frame_captured=true
draw_count_gt_zero=true
first_room_visible=true
non_background_ratio_pass=true
player_marker_visible=true
room_proxy_visible=true
depth_test_enabled=true
runtime_hash_unchanged=true
validation_error_count=0
sync_validation_clean=true
```

Human read:

```text
I can tell there is a playable space, a player position, and at least one projected gameplay marker or a diagnosed reason it is absent.
```

Rules:

- `draw_count > 0` is not enough;
- screenshot coverage without runtime hash invariance is not enough;
- validation-clean rendering without visible geometry is not enough;
- a captured first frame must identify whether the room came from projection geometry or a diagnosed temporary room proxy;
- missing pickup/objective markers may pass only when the projection/fixture receipt diagnoses that none exist.

## Software Versus Hardware Baselines

Software Vulkan is useful, but it is not shipping proof.

Policy:

```text
software_vulkan_lane=optional_ci_aid_only
software_exact_hash=allowed_per_software_stack_version
hardware_exact_hash=allowed_per_device_driver_lane
software_hash_cannot_approve_hardware_shipping=true
moltenvk_baseline=separate_portability_baseline
linux_native_baseline=separate_shipping_baseline
windows_native_baseline=separate_shipping_baseline
```

Rules:

- store software baselines separately from hardware baselines;
- record software renderer name and version when available;
- do not compare software exact hashes against hardware exact hashes;
- MoltenVK behavior may explain Apple-specific differences but does not define cross-platform Vulkan truth;
- Linux and Windows native Vulkan lanes must produce their own screenshot/hash artifacts before shipping claims.

## Test Lane Behavior

Initial lane split:

| Test | Purpose | Screenshot required | Exact hash required | Tolerant visibility required |
| --- | --- | --- | --- | --- |
| `vulkan_first_room_smoke` | first draw/present/receipt proof | preferred until capture packet, required after capture packet | no | yes after capture packet |
| `vulkan_screenshot_smoke` | Vulkan readback and artifact proof | yes | records only | yes |
| `vulkan_frame_hash_smoke` | normalized hash generation and baseline comparison | yes | same-lane only | yes |

Rules:

- before the screenshot packet exists, first-room smoke may pass with receipt-only visual diagnostics if the phase explicitly says capture is not implemented;
- after screenshot capture is implemented, strict first-room visual proof requires a screenshot artifact;
- screenshot smoke fails in strict lanes when capture is unavailable;
- optional lanes may skip with the project skip code only before attempting capture or when platform prerequisites are absent;
- if capture begins and Vulkan returns an error, the lane fails unless the exact policy says the platform capability is optional.

## Receipt Fields

Required screenshot/hash receipt fields:

```text
screenshot_capture=enabled|disabled|unavailable
screenshot_required=true|false
screenshot_written=true|false
screenshot_path=
raw_rgba_path=
screenshot_meta_path=
frame_hash_path=
capture_source=swapchain|offscreen|unavailable
capture_source_usage_transfer_src=true|false|unavailable
capture_extent=
capture_source_format=
capture_source_color_space=
normalized_format=rgba8|unavailable
exact_rgba_sha256=
hash_cross_gpu_comparable=false
non_background_ratio=
player_marker_pixel_count=
room_proxy_pixel_count=
unique_color_bucket_count=
visible_room_rendered=true|false|unavailable
visible_room_failure=
```

Required invariance fields when runtime is present:

```text
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false
```

## Failure Reason Codes

Renderer screenshot/hash failures should use stable reason codes:

```text
screenshot_capture_unavailable
screenshot_write_failed
frame_hash_mismatch
frame_hash_unstable
visible_room_missing
player_marker_missing
room_proxy_missing
background_coverage_too_high
non_background_coverage_too_low
readback_unsupported
readback_copy_failed
readback_sync_failed
image_format_unsupported_for_capture
runtime_hash_changed
software_baseline_only
exact_hash_cross_gpu_forbidden
```

Rules:

- failures must name one primary reason code;
- secondary evidence belongs in receipt fields;
- `runtime_hash_changed` is always a hard fail for runtime-connected render smoke;
- `exact_hash_cross_gpu_forbidden` indicates a bad test expectation, not a renderer rendering failure.

## Acceptance Criteria

This policy is ready for implementation planning when:

- the first screenshot path is Vulkan readback, not OS screenshot automation;
- artifact paths are stable and outside source directories;
- raw RGBA and PNG roles are distinct;
- exact hash input is normalized pixels, not PNG bytes;
- cross-platform exact hash comparison is forbidden by default;
- tolerant first-room visibility criteria are explicit;
- software Vulkan and hardware Vulkan baselines are separated;
- macOS/MoltenVK, Linux native Vulkan, and Windows native Vulkan can each produce independent evidence;
- runtime hash invariance remains mandatory;
- failure reason codes are listed before tests are written.
