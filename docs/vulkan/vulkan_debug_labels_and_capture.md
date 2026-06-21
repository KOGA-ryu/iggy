# Vulkan Debug Labels And Capture

This document defines the `iggy3d` Vulkan policy for debug object names, command buffer labels, queue labels, GPU marker naming, and optional capture workflows with RenderDoc and GFXReconstruct.

Debug labels and captures are renderer diagnostics. They help builders inspect Vulkan state and GPU work. They do not own runtime truth, command legality, camera mode truth, save/load truth, replay determinism, package validation, or gameplay semantics.

## Purpose

Define a useful first debug labeling path:

```text
debug_label_api=VK_EXT_debug_utils
object_names=required_in_debug_validation_lane_after_objects_exist
command_labels=required_for_first_room_capture
queue_labels=optional_initially
gpu_marker_names=stable_hierarchical_renderer_names
renderdoc=manual_optional_capture_tool
gfxreconstruct=manual_optional_capture_replay_tool
runtime_capture_dependency=false
headless_capture_dependency=false
```

This document narrows:

- [debug_validation.md](debug_validation.md)
- [vulkan_function_loading.md](vulkan_function_loading.md)
- [command_recording.md](command_recording.md)
- [pipeline_cache_and_variants.md](pipeline_cache_and_variants.md)
- [resource_model.md](resource_model.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [packaging.md](packaging.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Specification debugging chapter: https://docs.vulkan.org/spec/latest/chapters/debugging.html | exact `VK_EXT_debug_utils` structs, callback behavior, severity/type rules, and valid usage |
| Vulkan Guide `VK_EXT_debug_utils`: https://docs.vulkan.org/guide/latest/extensions/VK_EXT_debug_utils.html | practical object naming, labels, and external-tool integration |
| Vulkan Samples debug utils: https://docs.vulkan.org/samples/latest/samples/extensions/debug_utils/README.html | command/queue labels, nested regions, markers, object naming examples |
| Khronos Simple Engine RenderDoc tooling: https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Tooling/03_debugging_and_renderdoc.html | RenderDoc workflow in a Vulkan engine context |
| RenderDoc docs: https://renderdoc.org/docs/index.html | manual capture and frame inspection workflow |
| GFXReconstruct desktop Vulkan docs: https://vulkan.lunarg.com/doc/view/latest/windows/capture_tools.html | capture layer, replay, `.gfxr` files, environment variables, desktop platform notes |

Priority rule:

```text
debug_utils_api_truth=Vulkan Specification and Registry
label_usage_shape=Vulkan Guide and Vulkan Samples
manual_frame_capture=RenderDoc docs
api_capture_replay=GFXReconstruct docs
project_authority=iggy3d renderer boundary docs
```

## Scope

In scope:

- `VK_EXT_debug_utils` enablement relationship;
- debug utils function loading relationship;
- Vulkan object naming;
- VMA allocation name relationship;
- command buffer label scopes;
- queue label scopes;
- marker naming and color policy;
- RenderDoc manual capture policy;
- GFXReconstruct capture/replay policy;
- capture artifact storage;
- diagnostics receipt fields;
- validation and smoke tests.

Out of scope:

- implementing debug labels now;
- RenderDoc in-application API integration;
- automated capture in CI;
- GPU performance profiling policy;
- vendor profiler integration beyond naming compatibility;
- crash dump tooling;
- shader source-level debugging flags;
- capture artifact upload;
- gameplay debug text rendering.

## Local File Surface

Likely future files:

```text
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/VulkanFunctions.hpp
src/render/vulkan/VulkanFunctions.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
build/artifacts/render_diagnostics/captures/
tests/unit/render_debug_label_policy_tests.cpp
tests/unit/render_capture_policy_tests.cpp
tests/smoke/vulkan_debug_labels_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| debug utils extension request | `DebugValidation` / instance setup | runtime truth |
| debug utils function pointers | `VulkanFunctions` or `DebugValidation` | gameplay behavior |
| object names | owning Vulkan module | content identity authority |
| command labels | command recording module | runtime command sequence |
| queue labels | queue submit/sync module | replay determinism |
| capture files | diagnostics artifact owner | source assets or save truth |
| RenderDoc/GFXReconstruct availability | diagnostics/tooling layer | build success by default |

Rules:

- debug labels may include backend-neutral ids for human orientation, but they remain renderer diagnostics;
- debug labels must not be parsed by runtime code;
- capture success must not affect deterministic runtime state;
- headless runtime tools must not require RenderDoc, GFXReconstruct, debug utils, or Vulkan labels.

## Entry Gates

Debug labels may be implemented after:

- `VK_EXT_debug_utils` extension policy exists in [debug_validation.md](debug_validation.md);
- debug utils function loading is defined in [vulkan_function_loading.md](vulkan_function_loading.md);
- first Vulkan objects have stable ownership names;
- first command recording path exists;
- diagnostics receipt has fields for debug utils and label availability.

RenderDoc/GFXReconstruct capture instructions become useful after:

- a windowed visual demo can present a frame;
- first-room draw produces visible geometry;
- object names and command labels exist enough to make a capture navigable.

Reviewer should reject capture automation before first visual proof unless the packet is explicitly a tooling spike and cannot affect normal build/test success.

## Debug Utils API Policy

Primary extension:

```text
VK_EXT_debug_utils
```

Required functions for this policy:

```text
vkSetDebugUtilsObjectNameEXT
vkCmdBeginDebugUtilsLabelEXT
vkCmdEndDebugUtilsLabelEXT
vkCmdInsertDebugUtilsLabelEXT
vkQueueBeginDebugUtilsLabelEXT optional
vkQueueEndDebugUtilsLabelEXT optional
vkQueueInsertDebugUtilsLabelEXT optional
```

Rules:

- load debug utils functions only when `VK_EXT_debug_utils` is enabled;
- missing debug utils fails only strict debug/validation lanes;
- non-strict lanes may run without labels when diagnostics say labels are unavailable;
- `VK_EXT_debug_marker` is deprecated for this project and must not be used for new work;
- debug labels cannot be required for gameplay correctness.

## Object Naming Policy

Every persistent Vulkan object that appears in a first-room capture should receive a debug name when debug utils is available.

First required object names:

| Object kind | Example name |
| --- | --- |
| instance | `iggy3d.instance` |
| physical device diagnostic only | `iggy3d.device.physical.<device_name>` |
| logical device | `iggy3d.device.logical` |
| graphics queue | `iggy3d.queue.graphics` |
| present queue | `iggy3d.queue.present` |
| swapchain | `iggy3d.swapchain.main` |
| swapchain image view | `iggy3d.swapchain.image_view.<index>` |
| depth image | `iggy3d.image.depth.swapchain_extent` |
| depth image view | `iggy3d.image_view.depth.swapchain_extent` |
| command pool | `iggy3d.command_pool.graphics.frame` |
| command buffer | `iggy3d.command_buffer.frame.<slot>` |
| semaphore | `iggy3d.semaphore.image_available.<slot>` |
| semaphore | `iggy3d.semaphore.render_finished.<slot>` |
| fence | `iggy3d.fence.in_flight.<slot>` |
| vertex buffer | `iggy3d.buffer.first_room.vertices` |
| index buffer | `iggy3d.buffer.first_room.indices` |
| staging buffer | `iggy3d.buffer.staging.upload.<sequence>` |
| pipeline layout | `iggy3d.pipeline_layout.first_room.v1` |
| pipeline | `iggy3d.pipeline.first_room.dynamic.depth.vertex_color.v1` |
| shader module | `iggy3d.shader_module.first_room.vert` |
| descriptor pool later | `iggy3d.descriptor_pool.material.smoke` |
| descriptor set later | `iggy3d.descriptor_set.material.<material_id>` |
| texture image later | `iggy3d.image.texture.<asset_id>` |
| sampler later | `iggy3d.sampler.linear.repeat` |

Rules:

- object names use lowercase, dots, underscores, and stable ids;
- object names should be concise enough to read in a capture tool;
- object names must not include raw filesystem paths;
- object names must not include raw pointer values as primary identity;
- object names must not expose private/user data;
- renaming an object after creation is allowed only if the new name is more specific and stable.

VMA relationship:

```text
vma_allocation_name=required_by_memory_policy
vk_debug_object_name=required_when_debug_utils_available_for_named_vulkan_objects
```

VMA allocation names and Vulkan object names are separate. Use both when both mechanisms are available.

## Command Label Policy

First-room command buffer label tree:

```text
Frame <frame_index>
  AcquireSwapchainImage
  FirstRoomRender
    TransitionSwapchainToColorAttachment
    TransitionDepthToAttachment
    BeginDynamicRendering
    BindPipeline first_room
    BindVertexBuffers first_room
    PushConstants clipFromModel
    Draw first_room
    EndDynamicRendering
    TransitionSwapchainToPresent
  Present
```

Rules:

- command labels are recorded only inside active command buffers;
- begin/end label calls must be balanced;
- labels may be nested but should stay shallow for first-room proof;
- labels should wrap meaningful renderer operations, not every single Vulkan call;
- inserted markers are for single points of interest, not large regions;
- labels must not include changing memory addresses or raw handles;
- labels must not be required for draw correctness.

Required first labels:

```text
Frame <frame_index>
FirstRoomRender
BeginDynamicRendering
Draw first_room
```

Optional first labels:

```text
AcquireSwapchainImage
TransitionSwapchainToColorAttachment
TransitionDepthToAttachment
TransitionSwapchainToPresent
Present
```

## Queue Label Policy

Queue labels are optional initially.

First queue label candidates:

```text
QueueSubmit graphics frame <frame_index>
QueuePresent present frame <frame_index>
UploadSubmit graphics sequence <sequence>
```

Rules:

- queue label functions are loaded only when available;
- queue labels must not replace command buffer labels;
- queue labels should wrap submits, presents, and uploads only when useful;
- if queue labels create noise or platform trouble, disable them and keep command labels.

## GPU Marker Naming

Use stable renderer names, not gameplay prose.

Good marker names:

```text
FirstRoomRender
Draw first_room
Draw material_unlit_textured asset=<asset_id>
Upload texture <asset_id>
RecreateSwapchain
```

Rejected marker names:

```text
Player is doing something cool
Maybe draw stuff
0x0000600000123456
/Users/name/private/path/texture.png
save_slot_3_truth
```

Rules:

- names should describe renderer work;
- backend-neutral ids are allowed only as context;
- localized UI text is forbidden in marker names;
- marker names are not a serialization format;
- marker names may change without changing runtime behavior.

## Label Color Policy

Colors are optional and only for capture readability.

Suggested palette:

| Work | RGBA |
| --- | --- |
| frame | `0.20, 0.45, 1.00, 1.00` |
| render pass/dynamic rendering | `0.15, 0.70, 0.35, 1.00` |
| resource upload | `1.00, 0.65, 0.15, 1.00` |
| swapchain/resize | `0.85, 0.35, 0.95, 1.00` |
| synchronization | `0.95, 0.85, 0.20, 1.00` |
| fallback/error marker | `1.00, 0.10, 0.20, 1.00` |

Rules:

- color is never parsed;
- color must not encode the only copy of a status;
- tools may ignore color.

## RenderDoc Policy

RenderDoc is an optional manual capture tool.

Policy:

```text
renderdoc_build_dependency=false
renderdoc_runtime_dependency=false
renderdoc_required_for_ci=false
renderdoc_capture_mode=manual_initially
renderdoc_in_app_api=deferred
```

Use RenderDoc when:

- first visual frame exists;
- a draw call is missing or wrong;
- pipeline state, descriptors, textures, or vertex inputs need inspection;
- validation is clean but output is visually wrong.

Do not use RenderDoc as:

- a substitute for validation layers;
- a required dependency for normal tests;
- a way to define runtime truth;
- the first proof before a presentable frame exists.

Capture artifact policy:

```text
capture_root=build/artifacts/render_diagnostics/captures/
renderdoc_extension=.rdc
committed=false
upload_by_default=false
```

Capture checklist:

1. Build visual demo with Vulkan, validation, and debug labels enabled.
2. Launch app through RenderDoc or attach before the target frame.
3. Capture the first-room frame.
4. Verify Event Browser contains `Frame`, `FirstRoomRender`, and `Draw first_room`.
5. Verify Resource Inspector shows named buffers/images/pipelines.
6. Save `.rdc` only under ignored diagnostics artifacts if needed.
7. Record capture path in diagnostics or handoff notes, not in runtime state.

## GFXReconstruct Policy

GFXReconstruct is an optional capture/replay tool for Vulkan API streams.

Policy:

```text
gfxreconstruct_build_dependency=false
gfxreconstruct_runtime_dependency=false
gfxreconstruct_required_for_ci=false
gfxreconstruct_capture_mode=manual_layer_initially
gfxreconstruct_extension=.gfxr
```

Use GFXReconstruct when:

- API call replay is useful;
- a device/platform behavior needs a capture that can be replayed;
- a bug is not easily inspected through a single RenderDoc frame.

Initial desktop capture concepts:

```text
layer=VK_LAYER_LUNARG_gfxreconstruct
capture_process_filter=GFXRECON_CAPTURE_PROCESS_NAME
output_artifact=build/artifacts/render_diagnostics/captures/<name>.gfxr
```

Rules:

- GFXReconstruct layer enablement is manual/developer tooling initially;
- capture env vars are not baked into normal app startup;
- replay results are diagnostics, not runtime truth;
- capture files may contain resource data and must not be committed by default.

## Platform Notes

macOS/MoltenVK:

- debug utils availability follows Vulkan-visible extension discovery through MoltenVK;
- RenderDoc support on macOS/Vulkan may be more constrained than Linux/Windows and must be treated as optional;
- GFXReconstruct desktop docs cover macOS, but local SDK/layer availability still must be verified.

Linux:

- RenderDoc and GFXReconstruct are strong optional developer tools on native Vulkan lanes;
- layer paths and capture env vars must be recorded in diagnostics only when used;
- software Vulkan/lavapipe captures must be labeled as software evidence.

Windows:

- Visual Studio and packaged app launch paths must be explicit when capturing;
- GFXReconstruct layer paths may include Debug/Release build folders;
- capture tools must not become required for normal Windows smoke success.

## Diagnostics Receipt

Debug label/capture smoke should print:

```text
debug_labels_policy_version=
debug_utils_enabled=true|false
debug_utils_functions_loaded=true|false
object_naming_enabled=true|false
named_object_count=
unnamed_required_object_count=
command_labels_enabled=true|false
command_label_count=
queue_labels_enabled=true|false
queue_label_count=
first_frame_label_present=true|false
first_room_render_label_present=true|false
first_room_draw_marker_present=true|false
renderdoc_available=true|false|unqueried
renderdoc_required=false
renderdoc_capture_path=
gfxreconstruct_available=true|false|unqueried
gfxreconstruct_required=false
gfxreconstruct_capture_path=
capture_artifact_written=true|false
capture_artifact_committed=false
runtime_state_touched=false
reason=
```

## Validation And Tests

Firewall scan:

```sh
rg -n "vkSetDebugUtils|vkCmd.*DebugUtils|vkQueue.*DebugUtils|RenderDoc|GFXReconstruct|gfxreconstruct|debug_label|debug_marker|VK_EXT_debug_marker" src/runtime src/content src/projection src/runtime/save apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no runtime/content/projection/save/headless debug-label or capture-tool dependency leak
```

Unit tests should cover:

- required first-room object names are generated;
- rejected names with raw paths or handles fail policy tests;
- command label begin/end helper balances labels;
- disabled debug utils path is a no-op with diagnostics;
- deprecated `VK_EXT_debug_marker` is rejected for new work;
- capture artifacts default to ignored diagnostics paths;
- RenderDoc/GFXReconstruct are optional by default.

Smoke tests should cover:

- debug utils function loading when extension is enabled;
- object naming of first-room buffers, images, pipeline, command buffers, semaphores, and fences;
- command labels around first-room render path;
- first-room smoke receipt includes debug label fields;
- strict debug-label lane fails if required object names are missing;
- non-strict lane runs without labels when debug utils is unavailable and diagnosed.

Manual capture checklist should be run after first-room visual proof:

- one RenderDoc capture can show named objects and labeled draw path;
- one GFXReconstruct capture can be created and replayed on the same platform when tooling is installed;
- capture artifacts are not committed.

## Failure Reason Codes

Use stable reason codes in diagnostics:

```text
debug_labels_scope_blocked
debug_utils_extension_missing
debug_utils_function_missing
debug_object_name_failed
debug_required_object_unnamed
debug_label_unbalanced
debug_label_recorded_outside_command_buffer
debug_queue_label_unavailable
debug_marker_deprecated_extension_used
debug_label_runtime_leak
renderdoc_unavailable
renderdoc_capture_failed
renderdoc_capture_unexpected_dependency
gfxreconstruct_unavailable
gfxreconstruct_layer_missing
gfxreconstruct_capture_failed
gfxreconstruct_replay_failed
capture_artifact_wrong_location
capture_artifact_committed
```

## Acceptance Gate

This policy is ready for implementation planning when:

- debug utils function loading policy is accepted;
- required first-room object names are accepted;
- first-room command label tree is accepted;
- queue labels remain optional;
- `VK_EXT_debug_marker` is rejected for new work;
- RenderDoc and GFXReconstruct are optional developer tools, not build/runtime dependencies;
- capture artifact root is accepted;
- diagnostics receipt fields are accepted;
- firewall scan has no debug-label/capture leaks outside renderer/tooling-owned files.

