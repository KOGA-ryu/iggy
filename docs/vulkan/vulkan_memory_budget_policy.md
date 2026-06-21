# Vulkan Memory Budget Policy

This document defines the Vulkan Memory Allocator adoption policy, heap budget diagnostics, allocation naming, pool usage, staging behavior, defragmentation deferral, and out-of-memory behavior for the `iggy3d` Vulkan backend.

Vulkan memory is renderer-owned implementation detail. Runtime, content, projection, save, replay, and gameplay code may identify assets and frame inputs, but they do not own Vulkan memory, VMA objects, Vulkan allocation handles, staging buffers, image residency, or GPU budget truth.

## Purpose

This policy exists to prevent resource growth from becoming anonymous GPU memory with unclear ownership.

Required direction:

```text
allocator=Vulkan Memory Allocator
first_room_manual_allocation=allowed_only_as_narrow_bootstrap
vma_required_before_texture_material_growth=true
allocation_names=required
heap_budget_receipts=required
default_pools_first=true
custom_pools=measured_only
staging_allocations=bounded_and_diagnosed
defragmentation=deferred
out_of_memory=diagnosed_renderer_failure_or_allowed_fallback
runtime_state_mutation_from_memory_policy=forbidden
```

This document narrows:

- [resource_model.md](resource_model.md)
- [gpu_resource_upload.md](gpu_resource_upload.md)
- [render_assets_and_materials.md](render_assets_and_materials.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [lifetime.md](lifetime.md)
- [device_selection.md](device_selection.md)
- [debug_validation.md](debug_validation.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| VMA reference docs: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/ | allocator creation, allocation flags, budget queries, naming, pools, stats, mapping, defrag |
| VMA GPUOpen overview: https://gpuopen.com/vulkan-memory-allocator/ | project role, capability overview, adoption rationale |
| VMA repository: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | release acquisition, source layout, integration notes |
| Vulkan Guide memory allocation: https://docs.vulkan.org/guide/latest/memory_allocation.html | Vulkan memory model, suballocation expectations, staging guidance, UMA/discrete GPU behavior |
| Vulkan Specification/Registry: https://registry.khronos.org/vulkan/ | exact memory requirements, error results, device-memory behavior, extension rules |
| Vulkan Validation docs and synchronization validation docs | correctness checks around memory usage, image layout, copies, and lifetime |
| How to Vulkan in 2026: https://howtovulkan.com/ | secondary practical reference for early VMA setup shape, allocator handoff after instance/device creation, and resource-growth context |

Priority rule:

```text
exact_api_behavior=Vulkan Specification and Registry
allocator_policy=VMA reference docs
practical_memory_shape=Vulkan Guide
secondary_memory_walkthrough=How to Vulkan after VMA policy is settled
debugging=Validation plus VMA diagnostics
```

## Scope

In scope:

- VMA dependency acquisition policy;
- allocator creation and destruction;
- VMA function pointer policy;
- allocation classes for buffers/images/staging;
- stable allocation names;
- Vulkan debug object name relationship;
- heap budget query and diagnostic receipts;
- default pool policy;
- custom pool gates;
- staging allocation policy;
- out-of-memory classification;
- budget warning and critical thresholds;
- defragmentation deferral;
- memory smoke tests and failure reason codes.

Out of scope:

- shader authoring;
- first-room draw command recording;
- asset package validation;
- content schema;
- streaming residency implementation;
- bindless descriptor strategy;
- sparse resources;
- external memory;
- GPU crash dump integration;
- deterministic runtime state hash changes.

## Local File Surface

Likely future files:

```text
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/VulkanMemoryAllocator.hpp
src/render/vulkan/VulkanMemoryAllocator.cpp
src/render/vulkan/VulkanFunctions.hpp
src/render/vulkan/VulkanFunctions.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
cmake/iggy3d_vulkan_deps.cmake
third_party/vma/ or external/vma/
tests/unit/render_memory_budget_policy_tests.cpp
tests/unit/render_memory_naming_tests.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| `VmaAllocator` | Vulkan resource module or private allocator helper | runtime truth, asset legality, save truth |
| `VmaAllocation` handles | Vulkan resource module | gameplay identity |
| allocation names | Vulkan resource module | content schema authority |
| heap budget snapshots | renderer diagnostics | command legality |
| default VMA pools | VMA allocator | asset package ownership |
| custom VMA pools | Vulkan resource module after measured need | broad engine memory policy |
| staging resources | Vulkan upload/resource module | runtime-visible mutable memory |
| OOM reason mapping | Vulkan result/diagnostics module | replay outcome changes |

Rules:

- runtime/content/projection may pass asset ids, CPU payloads, and frame data through backend-neutral contracts;
- renderer converts backend-neutral data into Vulkan buffers, images, and allocations;
- renderer diagnostics may report memory pressure but cannot mutate deterministic runtime state;
- memory failures must be isolated to renderer fallback, renderer disablement, or app-level failure reporting.

## Entry Gates

VMA adoption may begin only after:

- headless runtime acceptance is green;
- renderer boundary exists;
- Vulkan device selection is implemented enough to provide instance, physical device, logical device, queue data, API version, enabled extensions, and feature chain diagnostics;
- Vulkan function loading policy is settled enough for VMA integration;
- lifetime ordering is documented and enforceable.

First-room exception:

```text
manual_vulkan_memory_allowed=true
scope=first_room_vertex_index_depth_only
duration=until_phase_8_vma_adoption
required_diagnostics=allocation_names_or_resource_names_plus_no_per_frame_churn
```

Texture/material growth is blocked until:

- VMA or a documented equivalent is active;
- allocation naming is active;
- heap budget diagnostics are active;
- staging high-water diagnostics exist;
- smoke tests prove create/upload/destroy behavior.

## Dependency Acquisition

Preferred acquisition:

```text
vma_source=vendored_pinned_snapshot
location=third_party/vma/ or external/vma/
network_at_configure_time=false
```

Acceptable alternatives:

- CMake `FetchContent` with a pinned tag/hash and opt-in network use;
- system package override for developer or CI machines if diagnostics record it.

Rejected:

- unpinned downloads;
- silent latest-branch tracking;
- package-manager-only dependency with no reproducible fallback;
- embedding VMA use in runtime/content/projection files.

The chosen release/tag/hash must be recorded in `docs/vulkan/decisions.md` or a dependency manifest when implementation begins.

## Allocator Creation

Allocator creation happens after:

```text
VkInstance created
VkPhysicalDevice selected
VkDevice created
API version known
enabled device extensions known
Vulkan function loading ready
```

Allocator destruction happens:

```text
after all VMA allocations are destroyed
after all VMA pools are destroyed
before vkDestroyDevice
```

Creation inputs:

- instance;
- physical device;
- logical device;
- Vulkan API version;
- optional VMA Vulkan function table;
- optional flags for enabled extensions.

Budget extension policy:

```text
device_extension=VK_EXT_memory_budget
enable_when=available_and_accepted_by_device_selection
allocator_flag=VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
required_for_first_room=false
required_for_texture_material_growth=preferred_not_absolute
diagnostic_required=true
```

If `VK_EXT_memory_budget` is unavailable, VMA may still estimate budgets. Diagnostics must make the budget source clear.

## Function Pointer Policy

The allocator follows [vulkan_function_loading.md](vulkan_function_loading.md).

Allowed first path:

```text
VMA_STATIC_VULKAN_FUNCTIONS=1
VMA_DYNAMIC_VULKAN_FUNCTIONS=0
```

This is acceptable only when the project links the Vulkan loader normally on macOS, Linux, and Windows.

Dynamic function path:

```text
VMA_STATIC_VULKAN_FUNCTIONS=0
VMA_DYNAMIC_VULKAN_FUNCTIONS=1
required=vmaVulkanFunctions table or vkGetInstanceProcAddr/vkGetDeviceProcAddr path
```

Dynamic function loading must use the same instance/device selected by the renderer. Missing allocator-required function pointers are startup failure, not delayed draw failure.

## Allocation Classes

Initial allocation classes:

| Class | Usage | Preferred VMA usage | Required before |
| --- | --- | --- | --- |
| `buffer.first_room.vertices` | transfer destination, vertex buffer | `VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE` | first-room draw |
| `buffer.first_room.indices` | transfer destination, index buffer | `VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE` | indexed first-room draw |
| `image.depth.swapchain_extent` | depth attachment | `VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE` | depth test |
| `buffer.staging.upload.<sequence>` | transfer source, CPU writes | `VMA_MEMORY_USAGE_AUTO` or `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` with host access flags | uploads |
| `image.texture.<asset_id>` | transfer destination, sampled image | `VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE` | texture/material growth |
| `buffer.uniform.frame.<slot>` | uniform or storage data if adopted | `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` or measured device-local path | descriptor growth |
| `buffer.readback.<reason>` | transfer destination, CPU reads | `VMA_MEMORY_USAGE_AUTO_PREFER_HOST` with random or sequential host access | debug/readback growth |

For VMA auto usage, mapped CPU-write staging allocations must request host access explicitly. The default first staging choice should be:

```text
usage=VMA_MEMORY_USAGE_AUTO
flags=VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
optional_flags=VMA_ALLOCATION_CREATE_MAPPED_BIT
```

Use `VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT` for optional allocations where failing early is better than overcommitting memory.

## Allocation Naming

Every VMA allocation must be named before the relevant smoke test can pass.

Naming rules:

- use stable semantic names;
- include resource class and purpose;
- use sanitized asset ids, not raw filesystem paths;
- include frame slot or sequence only when the allocation is slot/sequence-specific;
- do not encode pointer values as the primary identity.

Required examples:

```text
buffer.first_room.vertices
buffer.first_room.indices
image.depth.swapchain_extent
buffer.staging.upload.0001
image.texture.<asset_id>
buffer.uniform.camera.frame0
```

VMA allocation name policy:

```text
primary_call=vmaSetAllocationName
required=true
```

Debug utils relationship:

```text
vma_allocation_name=required
vk_debug_object_name=separate_optional_until_debug_utils_enabled
```

VMA allocation names help allocator diagnostics and optional allocator-native
stats, but allocator-native dumps are not a repo-facing smoke-test format. They
do not automatically name Vulkan buffer/image objects for debug tools. If
`VK_EXT_debug_utils` is enabled, Vulkan objects should also receive debug names
through the debug utils path.

## Budget Diagnostics

Heap budget diagnostics are part of renderer smoke output.

Required query:

```text
query=vmaGetHeapBudgets
frequency=memory_smoke_plus_periodic_renderer_diagnostics
per_frame_allowed=true
```

When `VK_EXT_memory_budget` is enabled, call `vmaSetCurrentFrameIndex()` once per rendered frame so VMA can maintain frame-aware budget behavior.

Required receipt fields:

```text
memory_allocator=vma|manual_bootstrap|none
vma_version=
vma_source=vendored|fetchcontent|system|unknown
vma_allocator_created=true|false
vma_ext_memory_budget_enabled=true|false
memory_budget_source=VK_EXT_memory_budget|vma_estimate|unavailable
heap_count=
heap_0_usage_bytes=
heap_0_budget_bytes=
heap_0_allocation_count=
heap_0_block_count=
budget_warning_count=
budget_critical_count=
allocation_count=
buffer_allocation_count=
image_allocation_count=
staging_allocation_count=
named_allocation_count=
unnamed_allocation_count=
custom_pool_count=
defrag_enabled=false
per_frame_allocation_count=
out_of_memory_count=
runtime_state_touched=false
reason=
```

Initial budget thresholds:

```text
warning_threshold=85_percent_of_reported_heap_budget
critical_threshold=95_percent_of_reported_heap_budget
threshold_effect=diagnostic_only_until_asset_residency_policy_exists
```

Budget warnings may influence renderer fallback decisions later, but they cannot change runtime simulation, save truth, command legality, deterministic state hash, or replay result.

## Default And Custom Pools

Default policy:

```text
custom_pools_allowed=false_initially
use_default_vma_pools=true
```

VMA default pools are the first implementation path. Custom pools are often unnecessary early and must not be created just to make the architecture look complete.

Custom pools become allowed only when a file-plan packet documents:

- measured allocation pattern;
- resource class;
- pool lifetime;
- pool block size or sizing policy;
- whether linear allocation is used;
- whether allocations can be individually freed;
- resize/recreate behavior;
- diagnostics name;
- tests proving no live allocations at pool destroy.

Likely future custom pool candidates:

| Candidate | When it may be justified |
| --- | --- |
| staging ring pool | repeated upload batches produce measurable allocation churn |
| transient upload pool | initialization batches need bounded grouping |
| streaming texture pool | texture residency policy exists and allocation classes are stable |
| readback pool | debug readbacks become recurring enough to need isolation |

Rejected early custom pools:

- one pool per asset;
- one pool per material;
- one pool per frame without measured need;
- pool creation in runtime/content/projection.

All custom pools must be destroyed only after every allocation from that pool is destroyed.

## Staging Policy

First-room staging:

```text
policy=transient_initialization_batch
allowed=true
per_frame_allocation=false
```

Rules:

- staging memory is host-visible;
- staging buffers use transfer source usage;
- CPU writes use mapped memory only through renderer-owned upload helpers;
- non-coherent memory must be flushed before GPU reads;
- staging allocation remains alive until upload commands complete;
- staging high-water bytes are diagnosed.

Growth staging:

```text
policy=reusable_or_batched
required_before=texture_material_growth
diagnostics=high_water_bytes_plus_allocation_churn
```

Persistent mapped staging is allowed only if:

- mapping flags are explicit;
- CPU write ranges are tracked;
- flush behavior is correct;
- in-flight reuse is fenced;
- diagnostics report mapped byte ranges and high-water use.

## Out-Of-Memory Behavior

Classify memory failures explicitly:

| Failure | Meaning | First response |
| --- | --- | --- |
| `VK_ERROR_OUT_OF_DEVICE_MEMORY` | device-local memory allocation failed | fail required resource or attempt allowed fallback |
| `VK_ERROR_OUT_OF_HOST_MEMORY` | host allocation failed | fail renderer startup/resource creation |
| `VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT` failure | optional allocation would exceed reported budget | skip optional resource or use fallback |
| budget warning | usage above warning threshold | diagnostic only initially |
| budget critical | usage above critical threshold | diagnostic plus optional-resource suppression |
| device lost | GPU/device failure | route through device-lost policy, not normal OOM |

Required resources:

```text
first_room_vertex_buffer=required
first_room_depth_image=required_if_depth_enabled
fallback_texture=required_once_texture_path_exists
```

If a required allocation fails, the Vulkan backend should fail the relevant smoke/app startup with a clear reason code. It must not partially mutate runtime state.

Optional resources:

```text
optional_texture=may_fallback
optional_material=may_fallback
optional_debug_readback=may_disable
optional_cache_resource=may_skip
```

Fallback is allowed only when the fallback is diagnosed and does not alter runtime determinism.

## Defragmentation

Defragmentation is deferred.

```text
defrag_enabled=false
defrag_allowed_before_first_room=false
defrag_allowed_before_texture_material_growth=false
```

Reason:

- first-room and early texture work need clear lifetime and residency before moving allocations;
- defrag requires a relocation protocol;
- descriptors, image views, buffer bindings, command buffers, and in-flight work must be coordinated before moving backing memory;
- some pool strategies, especially linear allocation, do not support normal defragmentation patterns.

A future defrag packet must define:

- which allocation classes may move;
- in-flight fence requirements;
- descriptor/image-view update rules;
- resource handle invalidation rules;
- debug receipt fields;
- tests that prove no active command buffer references moved memory.

## Lifetime Rules

Creation order:

```text
instance
physical_device
logical_device
vulkan_function_loading
vma_allocator
resources
swapchain_dependent_depth_resources
```

Shutdown order:

```text
wait_for_device_idle_or_prove_no_in_flight_work
destroy_swapchain_dependent_depth_resources
destroy_texture_image_resources
destroy_vertex_index_uniform_buffers
destroy_staging_resources
destroy_custom_pools
destroy_vma_allocator
destroy_logical_device
```

Swapchain resize:

- swapchain-dependent depth images are recreated;
- first-room vertex/index buffers survive resize;
- texture resources survive resize;
- allocator survives resize;
- budget receipt after resize must still report sane counts.

## Validation And Tests

Firewall scan:

```sh
rg -n "Vma|vma|VkDeviceMemory|VK_ERROR_OUT_OF_DEVICE_MEMORY|VK_ERROR_OUT_OF_HOST_MEMORY|VK_EXT_memory_budget" src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
```

Expected result:

```text
no runtime/content/projection/save/renderer-api memory ownership leak
```

Unit tests should cover:

- VMA source/version decision is recorded;
- allocation names are required;
- unnamed allocations fail policy tests;
- custom pools are rejected unless approved by policy;
- OOM results map to stable renderer reason codes;
- budget receipt fields remain stable;
- required-resource allocation failure is fatal to renderer startup/smoke;
- optional-resource allocation failure can route to fallback only with diagnostics.

Smoke tests should cover:

- allocator create/destroy;
- vertex buffer allocation and naming;
- index buffer allocation and naming if indexed draw is used;
- depth image allocation and naming;
- staging buffer allocation, upload, and destruction/reuse;
- one sampled texture allocation once texture path exists;
- heap budget receipt printing;
- zero steady-state per-frame allocation churn after initialization.

## Failure Reason Codes

Use stable reason codes in diagnostics:

```text
memory_budget_scope_blocked
vma_dependency_missing
vma_version_unpinned
vma_allocator_create_failed
vma_function_table_incomplete
vma_budget_extension_unavailable
vma_budget_query_failed
vma_allocation_unnamed
vma_pool_unapproved
vma_pool_destroy_with_live_allocations
vma_allocation_failed
vma_out_of_device_memory
vma_out_of_host_memory
vma_budget_exceeded
vma_required_resource_oom
vma_optional_resource_fallback
vma_staging_lifetime_unsafe
vma_per_frame_allocation_churn
vma_defrag_unapproved
vma_runtime_leak
```

## Acceptance Gate

This policy is ready for implementation planning when:

- the VMA acquisition decision is recorded;
- allocator creation inputs are known from device selection;
- function loading strategy is compatible with VMA;
- allocation class names are accepted;
- budget receipt fields are accepted;
- custom pools remain blocked by default;
- staging policy is tied to upload synchronization;
- OOM reason codes are wired into diagnostics planning;
- the firewall scan has no Vulkan/VMA memory leaks outside renderer-owned files.
