# Vulkan Reading Map For Iggy

This map turns the Vulkan manuals into renderer work packets for Iggy. Use it
as the routing layer between external Vulkan documentation and local modules.

Iggy rule: Vulkan backend work must stay behind a sane renderer boundary.
Gameplay state, authored package validation, save/load truth, and scene
semantics stay outside the Vulkan backend.

Vulkan is an implementation backend, not an engine authority.

## Source Stack

Primary sources:

- Khronos Vulkan Tutorial: https://docs.vulkan.org/tutorial/latest/00_Introduction.html
- Vulkan Specification and Registry: https://registry.khronos.org/vulkan/
- Vulkan Guide: https://docs.vulkan.org/guide/latest/index.html
- Vulkan Samples: https://docs.vulkan.org/samples/latest/README.html
- Building a Simple Engine: https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/introduction.html
- Vulkan Validation Overview: https://docs.vulkan.org/guide/latest/validation_overview.html
- Synchronization Validation: https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html
- Vulkan Memory Allocator: https://gpuopen.com/vulkan-memory-allocator/
- VMA reference docs: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/
- LunarG macOS Vulkan SDK guide: https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html
- MoltenVK Runtime User Guide: https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md

Reading order:

1. Khronos Vulkan Tutorial.
2. Vulkan Guide sections for validation, swapchain, synchronization, memory, and descriptors.
3. Vulkan Samples for triangle, cube, dynamic rendering, descriptors, texture, depth, and camera patterns.
4. VMA docs before building a general buffer/image allocator.
5. MoltenVK and macOS Vulkan SDK docs before claiming Apple portability.
6. Vulkan Specification only when exact behavior matters.
7. Building a Simple Engine after the first Iggy Vulkan path is stable.

Cross-platform Vulkan behavior follows the Vulkan docs/spec. MoltenVK notes
explain Apple backend constraints and exceptions; they do not become the source
of truth for the whole renderer.

## Local Module Map

| Iggy module | Local surface | External sources | Plan |
| --- | --- | --- | --- |
| InstanceDeviceSurface | `engine/apps/native_play/NativeVulkanRenderer.*` | Tutorial, Vulkan Guide, LunarG macOS SDK, MoltenVK | Own instance creation, validation-layer enablement, required extensions, physical device selection, logical device, queues, and SDL/MoltenVK surface setup. Emit diagnostics for missing layers/extensions/devices. |
| Swapchain | `NativeVulkanRenderer` private state | Tutorial, Vulkan Guide, MoltenVK | Own swapchain create/recreate/destroy, image views, format/present-mode selection, resize handling, and Apple portability notes. Keep gameplay/session state out. |
| CommandBuffers | `NativeVulkanRenderer` frame path | Tutorial, Vulkan Specification | Own command pool, per-frame command buffers, recording boundaries, submit order, and reset policy. Use the spec only for exact flags, layouts, and lifetime questions. |
| Sync | renderer-private frame resources | Vulkan Guide, Synchronization Validation, Vulkan Specification | Own fences, semaphores, frames-in-flight policy, image acquisition/presentation ordering, and barrier ownership. Enable sync validation in development gates. |
| BuffersImagesMemory | future renderer resource layer | VMA docs, Vulkan Guide, Samples | Introduce VMA before expanding beyond small hardcoded buffers. Define buffer/image creation helpers, allocation naming, budget reporting, and debug stats before textures/materials broaden. |
| PipelinesShaders | renderer pipeline resources and `engine/apps/native_play/shaders` | Tutorial, Samples, Vulkan Guide | Own shader module loading, pipeline layout, descriptor set layout, graphics pipeline, depth, texture, and material policy. Keep shader interface changes in narrow packets. |
| RendererApi | public renderer-facing boundary | Building a Simple Engine, Samples | Shape the backend API around frame input, draw lists, resources, diagnostics, and resize/lifetime calls. Do not expose Vulkan handles to gameplay/runtime code. |
| DebugValidation | renderer init and test/smoke gates | Validation Overview, Synchronization Validation, LunarG SDK | Always support development validation layers, debug messenger output, best-practices validation when useful, and sync-validation passes. Do not ship validation layers in release builds. |
| ProjectionProducer / RendererConsumer | `NativeSceneDrawList.hpp`, runtime projection code, renderer frame input | Iggy architecture docs, not Vulkan manuals | ProjectionProducer creates backend-neutral draw lists from runtime state. RendererConsumer consumes those draw lists. Projection decides what to draw; Vulkan decides how to draw it. |

## Implementation Phases

### Phase 0: Baseline The Current Renderer

Goal: make the current Vulkan code easy to audit before expanding it.

Tasks:

- Map current `NativeVulkanRenderer` private state to the module names above.
- Write down which code owns instance/device/swapchain/pipeline/buffers/sync.
- Do not split `NativeVulkanRenderer` into modules until the ownership inventory
  maps current object lifetimes and failure paths.
- Add a short renderer diagnostic dump for device name, API version, driver
  version, selected extensions, queue families, swapchain format, present mode,
  depth format, validation state, sync-validation state, frame count, and draw
  count.
- Run the renderer dependency scan:

  ```sh
  rg -n "#include[ <\"]vulkan/|#include <vulkan/vulkan.h>|\\bVk[A-Z]|\\bVK_" \
    engine/src/runtime engine/src/content engine/src/projection engine/src/save 2>/dev/null
  ```

- Verify `iggy_native_play --help`, a native smoke launch, and existing render/resource tests.

Exit criteria:

- A reviewer can point to the owner of each Vulkan object lifetime.
- No gameplay/runtime surface includes Vulkan headers.
- Dependency scan has no production runtime/content/projection/save ownership leaks.

### Phase 1: InstanceDeviceSurface

Goal: make startup explicit and debuggable on macOS/MoltenVK and native Vulkan platforms.

Tasks:

- Centralize instance creation and validation-layer enablement.
- Record required instance extensions from SDL and platform portability requirements.
- Centralize physical-device scoring and logical-device queue selection.
- Report missing validation layers, surface extensions, portability extensions, queue families, and device features as structured diagnostics.

Exit criteria:

- Bad local Vulkan SDK/MoltenVK setup produces a clear Iggy diagnostic before renderer construction proceeds.

### Phase 2: Swapchain

Goal: make frame presentation robust enough for the product loop.

Tasks:

- Isolate swapchain creation, image views, format selection, extent selection, and present mode selection.
- Add resize/recreate path without touching product session or gameplay code.
- Track MoltenVK-specific portability notes separately from cross-platform Vulkan behavior.

Exit criteria:

- Window resize or surface invalidation does not require recreating gameplay/session state.

### Phase 3: CommandBuffers

Goal: make command recording a stable renderer-owned frame contract.

Tasks:

- Define per-frame command buffer ownership.
- Keep command recording synchronized with swapchain image acquisition.
- Make the renderer consume only `NativeVulkanFrameInput` and renderer-owned resources.
- Use the Vulkan Specification for exact command-buffer reset and lifetime rules when validation flags them.

Exit criteria:

- Product session can build a frame input; renderer records and submits without pulling runtime state.

### Phase 4: Sync

Goal: stop black-frame and race-condition debugging from becoming guesswork.

Tasks:

- Define max frames in flight.
- Name every semaphore/fence responsibility.
- Add a development path for synchronization validation.
- Document allowed places for image layout transitions and memory barriers.

Exit criteria:

- Sync validation can be run as a focused renderer gate and produces no known issues for the current draw path.

### Phase 5: BuffersImagesMemory

Goal: move from hardcoded mesh buffers to a resource layer without hand-rolling full Vulkan memory management.

Tasks:

- Add VMA as the allocation plan before broad buffer/image expansion.
- Wrap mesh vertex/index buffers with named allocations.
- Add allocation naming and optional budget/stat diagnostics.
- Defer texture streaming, defragmentation, and advanced pools until texture/material work needs them.
- Do not start texture/material work until buffer/image allocation naming and
  diagnostics exist.

Exit criteria:

- New buffers/images go through one allocation path, and renderer diagnostics can identify which resource owns each allocation.

### Phase 6: PipelinesShaders

Goal: support the first real 3D scene without baking product semantics into shaders.

Tasks:

- Keep shader modules, pipeline layouts, descriptor layouts, and graphics pipelines renderer-private.
- Add depth when 3D room geometry needs it.
- Add descriptor sets only with a specific resource need: camera uniforms, material data, texture samplers, or per-object transforms.
- Use Vulkan Samples for production-ish descriptor and texture examples after the cube path is stable.

Exit criteria:

- Player, floor, wall, door, item, and NPC model slots can render through a documented shader/pipeline contract.

### Phase 7: RendererApi

Goal: turn `NativeVulkanRenderer` from an app-local proof into an engine-quality backend boundary.

Tasks:

- Keep the public API close to: initialize, resize, draw frame, wait idle, shutdown.
- Pass semantic draw items and camera/frame data in backend-neutral structs.
- Return diagnostics by value; do not leak Vulkan handles or gameplay pointers.
- Use Building a Simple Engine for architecture patterns after Iggy has a working first path.

Exit criteria:

- A future backend or test renderer can consume the same frame input without rewriting gameplay projection.

### Phase 8: DebugValidation

Goal: make validation a normal development gate.

Tasks:

- Enable `VK_LAYER_KHRONOS_validation` in development builds.
- Add a debug messenger and stable log formatting.
- Add a focused sync-validation run path.
- Keep validation disabled for release/shipping builds.

Exit criteria:

- Renderer smoke checks fail loudly on validation errors instead of silently drawing black frames.

### Phase 9: ProjectionProducer And RendererConsumer

Goal: keep the renderer fed by Iggy scene facts, not by gameplay ownership leaks.

Tasks:

- Keep `NativeSceneDrawList` or its successor as the semantic draw boundary.
- Keep ProjectionProducer runtime-side: it creates backend-neutral draw lists.
- Keep RendererConsumer renderer-side: it consumes backend-neutral draw lists.
- Add model-slot ids for floor, wall, door, player, NPC, item, and interaction marker.
- Keep package loading, asset validation, and gameplay command semantics outside Vulkan.
- Add draw-list tests before renderer tests when projection behavior changes.

Exit criteria:

- Runtime state projects into a backend-neutral draw list; renderer tests verify consumption, not gameplay decisions.

## Guardrails

- Do not put Vulkan types in runtime, scene, package, save/load, or authoring APIs.
- `engine/src/runtime/**`, `engine/src/content/**`, `engine/src/projection/**`,
  and `engine/src/save/**` must not include `<vulkan/vulkan.h>`, other Vulkan
  SDK headers, `Vk*` handle/type names, or `VK_*` constants.
- Do not let renderer packets also change gameplay behavior.
- Do not add glTF runtime loading before the `.igmesh` package asset contract is stable.
- Do not broaden VMA integration into texture/material architecture until a concrete texture/material packet exists.
- Do not treat Qt render paths as shipping renderer targets.
- Do not read the full Vulkan Specification as a planning step; use it to answer exact validation/spec questions.
- Extraction is allowed only after owner inventory is written. Do not split
  `NativeVulkanRenderer` into module classes until a reviewer can map current
  object lifetime and failure paths.

## Renderer Smoke Receipt

Every renderer smoke check should print enough state to make startup/presentation
trustworthy:

- device name;
- Vulkan API version;
- driver version;
- validation enabled;
- synchronization validation enabled;
- selected queue families;
- swapchain format;
- present mode;
- frame count;
- depth format;
- draw count.

## First Packets To Open

1. Renderer ownership inventory: map current `NativeVulkanRenderer` members/functions into this file's module names.
2. Validation baseline: debug messenger plus development validation-layer toggle and diagnostic text.
3. Instance/device/surface diagnostics: explicit failure reasons for macOS/MoltenVK and standard Vulkan.
4. Swapchain ownership extraction: create/recreate/destroy helpers and resize gate.
5. Sync validation gate: focused smoke command and documented expected-clean output.
6. VMA adoption scout: dependency plan, wrapper shape, and one vertex/index buffer migration target.
