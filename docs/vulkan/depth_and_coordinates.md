# Vulkan Depth And Coordinates

This document defines coordinate conventions, matrix ownership, Vulkan clip-space expectations, depth policy, winding/culling, and first-room validation rules for `iggy3d`.

The renderer consumes derived camera and projection data. It may adapt data into Vulkan commands and shader inputs, but it does not own runtime camera truth, world semantics, save data, replay determinism, or gameplay coordinate authority.

## Purpose

Define the first 3D coordinate/depth contract:

```text
world axes are stable
runtime owns camera mode truth
FrameInput carries renderer-ready camera matrices
Vulkan clip depth is 0..1
first-room shader consumes clipFromModel
depth test/write are enabled
winding/culling must be proven against geometry
coordinate fixes are explicit diagnostics, not hidden repairs
```

This document connects [frame_input_contract.md](frame_input_contract.md), [camera_render_contract.md](camera_render_contract.md), [shader_pipeline.md](shader_pipeline.md), [command_recording.md](command_recording.md), and [resource_model.md](resource_model.md).

## Source Priority

Use these sources before implementation:

- Vulkan Specification/Registry for clip-space, viewport, depth range, culling, and pipeline state behavior.
- Vulkan Guide for coordinate-system and projection explanation.
- Khronos Vulkan Tutorial depth buffer and projection chapters for first implementation shape.
- Project camera/runtime docs for `CameraState` authority.
- Existing local Vulkan docs listed above for renderer ownership and diagnostics.

If math helper conventions and Vulkan requirements disagree, do not patch silently inside command recording. Document the conversion point and test it.

## Scope

In scope:

- world axis convention;
- unit convention for first-room rendering;
- view/projection/clip matrix semantics;
- Vulkan normalized device coordinate/depth range;
- viewport and scissor relationship;
- near/far/FOV policy;
- depth image clear/test/write/compare policy;
- winding/front-face/culling policy;
- first-person, third-person, and tactical camera implications;
- diagnostics and tests.

Out of scope:

- runtime camera mode design beyond consuming `CameraState`;
- camera collision;
- animation interpolation beyond camera frame data;
- material/lighting depth prepass;
- shadow maps;
- reverse-Z adoption;
- non-Vulkan backend conventions.

## Local File Surface

Likely future files:

```text
src/render/FrameInput.hpp
src/render/RendererApi.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
tests/unit/render_camera_frame_tests.cpp
tests/unit/render_projection_input_tests.cpp
tests/smoke/vulkan_first_room_smoke.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
```

The exact camera derivation helper may live outside Vulkan if it uses only backend-neutral math and renderer public types. Vulkan-specific depth formats, viewport state, and pipeline state stay under `src/render/vulkan/**`.

## Ownership

| Concept | Owner | Notes |
| --- | --- | --- |
| gameplay/world truth | runtime | save/replay authority |
| camera mode truth | runtime `CameraState` | renderer cannot switch modes |
| backend-neutral projections | projection | no Vulkan handles |
| `RenderCameraFrame` | app/projection frame assembly | derived presentation data |
| Vulkan viewport/scissor | Vulkan command recording | presentation only |
| Vulkan depth image/state | Vulkan resource/pipeline modules | renderer-owned |
| winding/culling fallback | Vulkan pipeline diagnostics | must be visible |

Rules:

- renderer may reject invalid matrices;
- renderer may not repair runtime camera truth;
- renderer may compute `clipFromModel` from `FrameInput` and draw-item transforms;
- renderer may not write corrected matrices back into runtime/projection;
- resize/aspect changes presentation projection only, never runtime state.

## World Axes

Project convention:

```text
X = right
Y = forward
Z = up
```

First-room unit convention:

```text
1 world unit = 1 meter, for camera/projection tuning only
```

Rules:

- runtime owns world position/orientation truth;
- projection emits backend-neutral transforms in this convention;
- renderer consumes transforms as presentation data;
- any asset import coordinate conversion must happen before or at projection/render resource boundary and be diagnosed;
- Vulkan docs do not get to redefine project world axes.

## Matrix Semantics

Required matrices:

```text
viewFromWorld
clipFromView
clipFromWorld = clipFromView * viewFromWorld
clipFromModel = clipFromWorld * worldFromModel
```

Rules:

- all matrices must be finite;
- `clipFromWorld` equality must be testable within tolerance;
- matrix multiplication order must be documented in the math helper/file plan;
- command recording pushes `clipFromModel`;
- shader does not reconstruct camera matrices for first-room proof;
- runtime/projection/save do not know Vulkan matrix structs.

If the codebase math library stores matrices row-major or column-major, the file plan must define upload/transposition behavior explicitly. Do not rely on shader coincidence.

## Vulkan Clip And Depth Range

Renderer-facing `FrameInput` must already be Vulkan-compatible for depth:

```text
NDC X = -1..1
NDC Y = -1..1
NDC Z = 0..1
near maps to 0
far maps to 1
```

Rules:

- `RenderCameraFrame.clipFromView` must produce Vulkan-compatible depth range;
- viewport `minDepth=0.0`, `maxDepth=1.0`;
- depth clear value is `1.0`;
- depth compare is `less`;
- reverse-Z is deferred;
- OpenGL-style `-1..1` depth must not enter Vulkan command recording unless explicitly converted before `FrameInput`.

Diagnostic fields:

```text
clip_depth_range=vulkan_0_to_1
viewport_min_depth=0.0
viewport_max_depth=1.0
depth_clear=1.0
depth_compare=less
```

## Viewport And Y Convention

First viewport policy:

```text
viewport.x=0
viewport.y=0
viewport.width=swapchain_extent.width
viewport.height=swapchain_extent.height
viewport.minDepth=0.0
viewport.maxDepth=1.0
```

Rules:

- viewport/scissor use swapchain extent for first-room rendering;
- `FrameInput.viewport` validates drawable size and aspect;
- renderer backend owns any API-specific viewport inversion;
- inversion must be named in diagnostics if used;
- command recording must not hide camera/model coordinate fixes inside viewport settings.

Open gate:

- if the first rendered room appears vertically inverted, fix the matrix/viewport convention once and document the owner;
- do not allow both shader and viewport to compensate independently.

## Near/Far And FOV Policy

First defaults from camera render contract:

| Mode | Vertical FOV | Near | Far |
| --- | ---: | ---: | ---: |
| First person | 75 degrees | 0.05 | 250 |
| Small third person | 70 degrees | 0.1 | 300 |
| Tactical overhead | 55 degrees | 0.5 | 600 |

Rules:

- near must be finite and greater than zero;
- far must be finite and greater than near;
- aspect ratio comes from `FrameInput.viewport`;
- runtime camera truth does not store swapchain aspect;
- tactical camera may use farther range, but depth precision must be monitored;
- near/far defaults are presentation policy until runtime explicitly promotes them.

Failure reason:

```text
camera_near_far_invalid
camera_projection_invalid
```

## Depth Buffer Policy

First-room depth policy:

```text
depth_test=enabled
depth_write=enabled
depth_compare=less
depth_clear=1.0
depth_format=selected_depth_format
```

Rules:

- depth resource extent matches swapchain extent;
- depth format comes from resource model/device support;
- depth image is recreated with swapchain extent;
- depth attachment is present for first-room 3D rendering;
- missing supported depth format is fatal for first-room Vulkan rendering;
- depth state is renderer-owned and never enters runtime/projection.

Deferred:

- reverse-Z;
- depth prepass;
- stencil;
- shadow-map depth;
- order-independent transparency.

## Depth Precision Notes

First implementation accepts standard forward depth.

Risks:

- very small near plane plus very far tactical range reduces precision;
- tactical overhead may need separate near/far tuning;
- large future worlds may require origin rebasing or reverse-Z, but not before first room.

Rules:

- do not adopt reverse-Z until first-room render is stable;
- if z-fighting appears in first-room geometry, diagnose geometry scale/near/far before changing depth convention;
- first-room tactical far plane is allowed because tactical camera needs wider overview, but it must be visible in diagnostics.

Diagnostics:

```text
depth_precision_policy=forward_standard
reverse_z=false
camera_near=
camera_far=
```

## Winding And Front Face

Preferred first pipeline target:

```text
front_face=counter_clockwise
cull_mode=back
```

Acceptance gate:

- first-room geometry must visibly render with this policy;
- if geometry winding is not yet proven, culling may be temporarily disabled;
- disabled culling must be a fallback diagnostic, not a silent default.

Allowed fallback:

```text
cull_mode=none
reason=winding_unverified
```

Rules:

- culling fallback is renderer presentation policy only;
- runtime/projection does not change because culling is disabled;
- asset/procedural geometry winding must be normalized before production asset growth;
- debug geometry may use disabled culling if documented.

## Face Orientation And Asset Growth

First-room proxy geometry should establish:

- consistent triangle winding;
- outward-facing walls/floors where applicable;
- stable normals later if lighting is introduced;
- culling policy compatible with projection/model transforms.

Growth rule:

- no textured/material model expansion until winding policy is proven or diagnosed;
- importing asset formats must include coordinate/winding conversion policy;
- model loader must not guess silently.

## Camera Mode Implications

First person:

- close near plane for interiors;
- forward vector derived from runtime camera truth;
- no renderer-owned camera collision;
- player marker may be absent or represented by hands/tools later, but first-room proof can show room/object geometry.

Small third person:

- target actor and nearby room should be visible;
- camera eye is derived presentation data;
- orbit distance/height are not renderer truth.

Tactical overhead:

- larger far plane and higher eye;
- depth precision must be diagnosed;
- grid/debug overlays only render if projection/debug projection emits them;
- slow-time/tactical mode authority remains runtime.

## Shader Interface

First-room vertex shader expects:

```text
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(push_constant) uniform PushConstants {
  mat4 clipFromModel;
};
```

Rules:

- shader positions are model-local;
- renderer computes `clipFromModel`;
- vertex color is fallback presentation data;
- no descriptor camera buffer is required for first-room baseline;
- shader must not encode gameplay coordinate policy.

## Validation Rules

Frame/camera validation must reject:

- non-finite camera vectors;
- zero-length forward/up vectors;
- non-finite matrices;
- `clipFromWorld` not matching `clipFromView * viewFromWorld`;
- near <= 0;
- far <= near;
- nonpositive viewport dimensions for drawable frames;
- unsupported depth format;
- missing depth resource when depth is enabled.

Renderer may skip only for non-drawable viewport/minimized state. Invalid camera/depth data is a failure in strict first-room smoke.

## Diagnostics Receipt Fields

Coordinate/depth diagnostics should include:

```text
world_axes=x_right_y_forward_z_up
world_unit=meter_for_render_tuning
clip_depth_range=vulkan_0_to_1
matrix_layout=
matrix_upload_transposed=true|false
clip_from_world_valid=true|false
clip_from_model_valid=true|false
camera_mode=
camera_fov_degrees=
camera_near=
camera_far=
camera_aspect_ratio=
viewport=
viewport_y_inverted=true|false
depth_test=enabled|disabled
depth_write=enabled|disabled
depth_compare=less
depth_clear=1.0
depth_format=
reverse_z=false
front_face=counter_clockwise|clockwise|unavailable
cull_mode=back|none
culling_fallback_reason=
reason=
```

Use `none` for `culling_fallback_reason` when no fallback is active.

## Failure Reason Codes

Recommended reason codes:

```text
camera_matrix_invalid
camera_near_far_invalid
clip_depth_range_invalid
clip_from_world_mismatch
clip_from_model_invalid
viewport_invalid
depth_format_unsupported
depth_resource_missing
winding_unverified
culling_policy_mismatch
matrix_layout_unverified
```

These should align with [diagnostics_and_tests.md](diagnostics_and_tests.md) during implementation.

## Tests

Future tests:

```text
tests/unit/render_camera_frame_tests.cpp
tests/unit/render_projection_input_tests.cpp
tests/unit/render_matrix_convention_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

Unit expectations:

- first-person camera frame is finite;
- third-person camera frame is finite;
- tactical camera frame is finite;
- `clipFromWorld = clipFromView * viewFromWorld` within tolerance;
- Vulkan depth range maps near to `0` and far to `1`;
- invalid near/far rejected;
- aspect change updates projection matrix without mutating runtime camera truth.

Smoke expectations:

- first room renders with depth enabled;
- depth clear/test/write are diagnosed;
- culling policy is diagnosed;
- if culling disabled, fallback reason is printed;
- no runtime hash/state change occurs because renderer consumes camera/depth data.

Command shape:

```sh
ctest --test-dir build --output-on-failure -R 'render_camera|render_matrix|vulkan_pipeline|vulkan_first_room'
```

## Acceptance Criteria

This coordinate/depth contract is ready for file plans when:

- world axes are explicit;
- matrix multiplication order is explicit;
- Vulkan depth range is explicit;
- viewport/depth conventions are explicit;
- near/far/FOV defaults are tied to camera modes;
- depth buffer policy is defined;
- winding/culling policy and fallback are defined;
- shader matrix interface is defined;
- validation rules are defined;
- diagnostics and reason codes are defined;
- renderer coordinate/depth failures cannot mutate runtime truth.

## Open Detail Items

These belong in future file plans:

- exact math helper owner for perspective and look-at matrices;
- exact row-major/column-major storage and shader upload behavior;
- exact viewport Y inversion decision after first screenshot;
- exact first-room proxy winding proof;
- exact depth format fallback order if not finalized in resource model;
- exact culling default after first geometry exists;
- exact matrix tolerance constants;
- exact pixel/screenshot check for depth/culling correctness.
