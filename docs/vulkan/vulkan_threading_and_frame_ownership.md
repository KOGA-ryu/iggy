# Vulkan Threading And Frame Ownership

This document defines which thread owns Vulkan calls in the first `iggy3d` renderer path, whether rendering runs on the main thread, how `FrameInput` crosses the renderer boundary, and what is forbidden until a real render thread exists.

Threading is renderer/app ownership policy. It must not become runtime truth, gameplay command legality, camera truth, save/load state, replay determinism, package validation, or projection semantics.

## Purpose

Define a conservative first path:

```text
first_renderer_thread_model=single_thread_main_thread
vulkan_call_owner=main_visual_thread_initially
render_thread=deferred
worker_command_recording=deferred
async_upload_thread=deferred
frame_input_crossing=immutable_snapshot_or_frame_lifetime_view
runtime_mutation_from_renderer_thread=false
renderer_config_changes_runtime_truth=false
```

The first implementation should prove correctness before parallelism. Vulkan can be used from multiple host threads, but the application owns all required host synchronization and object ownership rules. `iggy3d` should not add a render thread until the single-thread path has device, swapchain, command, sync, shader, and first-room smoke coverage.

This document narrows:

- [render_loop.md](render_loop.md)
- [frame_input_contract.md](frame_input_contract.md)
- [command_recording.md](command_recording.md)
- [sync_contract.md](sync_contract.md)
- [vulkan_first_file_plans_index.md](vulkan_first_file_plans_index.md)
- [vulkan_renderer_config.md](vulkan_renderer_config.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)
- [descriptor_policy.md](descriptor_policy.md)
- [boundaries.md](boundaries.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Specification threading behavior: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#fundamentals-threadingbehavior | exact host-threading and externally synchronized parameter rules |
| Vulkan Specification command pool rules: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#commandbuffers-pools | command pool external synchronization and command-buffer allocation/reset constraints |
| Vulkan Specification synchronization chapter: https://docs.vulkan.org/spec/latest/chapters/synchronization.html | host/device synchronization responsibilities and queue/command synchronization rules |
| Vulkan Guide threading chapter: https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/threading.adoc | practical command-pool-per-thread guidance for later multithreaded recording |
| Project render loop and sync docs | local frame order, fence/semaphore ownership, and runtime firewall |

Priority rule:

```text
threading_truth=Vulkan Specification
practical_multithread_shape=Vulkan Guide
first_path_policy=iggy3d renderer docs
runtime_truth=iggy3d runtime docs, never renderer thread policy
```

## Scope

In scope:

- first renderer thread ownership;
- main-thread render policy;
- Vulkan call ownership;
- queue submission ownership;
- command pool and command buffer host ownership;
- `FrameInput` crossing rules;
- runtime/projection handoff timing;
- diagnostics fields;
- forbidden work before render-thread adoption;
- future render-thread decision gate;
- tests and review scans.

Out of scope:

- implementing a job system;
- implementing a render thread now;
- worker-thread command buffer recording;
- async resource upload thread;
- lock-free queues;
- frame pacing system;
- CPU scheduling policy;
- gameplay simulation threading;
- editor UI threading.

## Local File Surface

Likely future files:

```text
src/render/RendererApi.hpp
src/render/RendererApi.cpp
src/render/RenderBackend.hpp
src/render/FrameInput.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
src/render/null/NullRenderer.cpp
src/render/vulkan/VulkanBackend.hpp
src/render/vulkan/VulkanBackend.cpp
src/render/vulkan/CommandBuffers.hpp
src/render/vulkan/CommandBuffers.cpp
src/render/vulkan/FrameSync.hpp
src/render/vulkan/FrameSync.cpp
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
apps/iggy3d_visual_demo/main.cpp
tests/unit/render_threading_policy_tests.cpp
tests/unit/render_frame_input_lifetime_tests.cpp
tests/unit/render_replay_invariance_tests.cpp
tests/smoke/vulkan_threading_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
```

This document does not implement those files.

## First Thread Model

First implementation:

```text
app_main_thread=poll_events_tick_runtime_build_projection_render_present
vulkan_thread_owner=app_main_thread
renderer_backend_thread_affinity=created_thread_only
render_thread_exists=false
renderer_worker_threads=false
```

The visual app main thread owns this order:

1. Poll platform events.
2. Translate input into app/runtime command requests.
3. Submit commands through runtime APIs.
4. Tick runtime if app policy says to tick.
5. Build scene/debug projection.
6. Derive render camera frame.
7. Assemble `FrameInput`.
8. Call renderer `renderFrame`.
9. Let Vulkan acquire, record, submit, present.
10. Emit diagnostics.

Rules:

- the same thread that creates the Vulkan backend owns Vulkan calls for the first path;
- `RenderBackend::initialize`, `renderFrame`, `resize`, `waitIdle`, and `shutdown` are called from that owner thread;
- if a call arrives from another thread, first implementation should fail a debug assertion or return a diagnosed renderer error;
- runtime remains single-authority regardless of renderer thread model;
- headless runtime tools do not create a Vulkan owner thread.

## Ownership Table

| Item | First owner thread | May cross threads now | Notes |
| --- | --- | --- | --- |
| runtime session mutation | app/runtime owner thread | no | renderer cannot mutate it |
| projection build | app/runtime owner thread | no | produces backend-neutral data before render |
| `FrameInput` assembly | app main thread | by immutable snapshot only | first path passes by const reference during call |
| Vulkan backend object | app main thread | no | thread affinity must be diagnosed |
| Vulkan instance/device/surface | app main thread | no | no cross-thread destroy/create |
| swapchain | app main thread | no | resize/recreate on owner thread |
| command pool | app main thread | no | one graphics command pool first |
| command buffers | app main thread | no | reset/record/submit on owner thread |
| queue submit/present | app main thread | no | queue host access is owner-thread serialized |
| resource upload | app main thread | no | async upload deferred |
| diagnostics receipt | app main thread or test harness after call | yes, after immutable copy | no runtime mutation |

## FrameInput Crossing

First crossing shape:

```cpp
RenderFrameResult RenderBackend::renderFrame(const FrameInput& input);
```

First ownership rule:

```text
FrameInput lifetime=valid_for_renderFrame_call
renderer_retains_FrameInput_pointers_after_return=false
renderer_may_copy_small_values=true
renderer_may_cache_GPU_resources=true
renderer_may_not_cache_projection_pointers=true
```

Allowed:

- pass `FrameInput` by const reference for the duration of `renderFrame`;
- copy viewport, camera matrices, frame index, counts, and stable ids into backend-private frame state;
- read projection items while recording the current frame;
- include source tick/hash in diagnostics;
- create GPU resources keyed by backend-neutral asset ids only through documented resource policy.

Forbidden:

- renderer retaining `SceneProjectionResult*` or `DebugProjectionResult*` after `renderFrame`;
- renderer mutating projection vectors;
- renderer mutating runtime session state;
- renderer calling command admission;
- renderer changing camera mode truth;
- renderer writing save/replay data;
- renderer using raw runtime pointers on a later thread.

If a future render thread is added, `FrameInput` must become an immutable copied frame packet or an explicitly reference-counted immutable snapshot. A dangling view across threads is not acceptable.

## Runtime/Projection Timing

First visual frame timing:

```text
runtime_tick_complete_before_projection=true
projection_complete_before_FrameInput=true
FrameInput_complete_before_renderFrame=true
renderFrame_does_not_tick_runtime=true
renderFrame_does_not_build_projection=true
```

Rules:

- renderer consumes a completed frame snapshot;
- renderer failure cannot roll back runtime;
- renderer skip does not decide whether runtime should tick next frame;
- app policy decides whether minimized/non-drawable states pause or continue runtime;
- runtime hash before and after renderer call must match in replay-invariance tests.

## Vulkan Host Synchronization Policy

First path avoids host-side Vulkan sharing:

```text
one_host_thread_uses_vulkan_objects=true
no_concurrent_VkQueue_use=true
no_concurrent_command_pool_use=true
no_concurrent_descriptor_pool_use=true
no_concurrent_allocator_use_without_policy=true
```

Rules:

- queue submit and present are serialized by owner-thread execution;
- command pool allocation/reset/recording is serialized by owner-thread execution;
- descriptor pool allocation/update is deferred and later must define ownership before use;
- VMA allocator calls are owner-thread only until memory policy explicitly allows another model;
- debug callback may be invoked by Vulkan/driver context, but it must only write thread-safe diagnostics state and must never call runtime.

Important Vulkan rule applied locally:

```text
externally_synchronized_vulkan_objects_are_not_shared_between_threads_initially
```

This avoids early mutex policy, command-pool-per-thread policy, and queue submission arbitration.

## Command Pool Thread Policy

First implementation:

```text
graphics_command_pool_count=1
graphics_command_pool_owner_thread=main_visual_thread
secondary_command_buffers=false
worker_command_pools=false
```

Rules:

- all command buffers allocated from the first graphics command pool are reset and recorded on the owner thread;
- command pool is not touched from validation callback, loader callbacks, upload workers, or app helper threads;
- command buffers are not recorded in parallel;
- secondary command buffers are blocked until a render-thread/worker plan exists;
- command pool reset must respect frame fence ownership from [sync_contract.md](sync_contract.md).

Future worker-thread recording requires:

- one command pool per worker thread or an explicit lock policy;
- immutable frame packet;
- no shared descriptor pool mutation without policy;
- no shared upload allocator mutation without policy;
- clear merge point for primary command buffer submission;
- sync validation smoke that proves the new path.

## Queue Ownership

First implementation:

```text
graphics_queue_submit_owner=main_visual_thread
present_queue_submit_owner=main_visual_thread
transfer_queue_submit_owner=deferred_or_main_thread_only
queue_mutexes=false_initially
```

Rules:

- all queue submissions are serialized by the owner thread;
- if graphics and present queues differ, the owner thread still submits both in render-frame order;
- no background upload queue submission until async upload policy exists;
- no queue mutex is needed in the first path because no other thread uses the queue;
- if a future thread submits to any queue, queue ownership and external synchronization must be redesigned before code lands.

## Resource Upload Thread Policy

First implementation:

```text
resource_upload_thread=false
staging_upload_owner=main_visual_thread
upload_completion=explicit_fence_or_wait_path
texture_streaming=blocked
```

Rules:

- first-room vertex/index/depth resources are created/uploaded on the Vulkan owner thread;
- staging buffers and upload command buffers are not touched by worker threads;
- async file IO may be planned later, but it must hand immutable decoded data to renderer-owned upload policy;
- texture/material streaming is blocked until VMA, descriptor, and upload ownership are defined;
- out-of-memory and upload failures use renderer result policy, not runtime mutation.

## Diagnostics And Debug Callback Threading

Validation/debug callbacks are special because the Vulkan implementation may call them while processing Vulkan API calls.

Callback rules:

- callback may increment renderer-owned atomic counters;
- callback may copy first error metadata into a bounded diagnostics buffer;
- callback must not call runtime APIs;
- callback must not call projection APIs;
- callback must not submit Vulkan work;
- callback must not block on long file IO;
- callback must not acquire locks that can be held during `renderFrame` in a way that risks deadlock.

Diagnostics receipt should include:

```text
thread_model=single_main_thread|render_thread|worker_recording
vulkan_owner_thread=main|render|unknown
render_thread_enabled=true|false
worker_recording_enabled=true|false
async_upload_enabled=true|false
frame_input_snapshot=copied|call_lifetime_view|ref_counted_immutable|unknown
cross_thread_runtime_mutation=false
debug_callback_thread_safe=true|false|unavailable
```

## Forbidden Until Render Thread Exists

Do not add these before a render-thread decision packet:

- background thread calling `RenderBackend::renderFrame`;
- Vulkan backend methods callable from arbitrary threads;
- command buffer recording workers;
- secondary command buffers;
- command pools per worker;
- queue submission from multiple host threads;
- async upload queue submission;
- descriptor pool updates from worker threads;
- VMA allocation/free from worker threads;
- render-owned thread reading live runtime objects;
- renderer thread sending direct runtime mutations;
- frame pacing thread that changes runtime tick authority.

Do not fake a render thread with partial ownership. If Vulkan calls still happen from multiple threads without a full ownership plan, the packet should fail review.

## Future Render Thread Decision Gate

A render thread may be planned only after:

- first visible room renders;
- sync validation lane is clean;
- swapchain recreate/minimize path is stable;
- renderer result/diagnostics policy is stable;
- `FrameInput` can be copied or snapshotted immutably;
- runtime replay/hash tests prove render presence has no deterministic effect;
- resource upload ownership is documented;
- shutdown order is documented and tested.

Required future design decisions:

```text
render_thread_lifetime
frame_packet_queue_depth
drop_or_block_policy_when_render_lags
runtime_tick_policy_when_render_thread_stalls
swapchain_resize_message_flow
device_loss_message_flow
shutdown_join_order
diagnostics_thread_safety
resource_upload_thread_policy
command_pool_per_thread_policy
queue_submission_owner
```

Until those are answered, `render_thread=false`.

## Future Render Thread Shape

Likely future shape, not first implementation:

```text
main_thread:
  poll events
  tick runtime
  build projection
  copy immutable FramePacket
  enqueue FramePacket
  consume renderer results/diagnostics

render_thread:
  own Vulkan backend
  dequeue latest accepted FramePacket
  acquire/record/submit/present
  enqueue renderer result
```

Hard boundaries for that future:

- render thread owns all Vulkan calls;
- main thread does not destroy Vulkan objects directly;
- resize/device-loss messages are queued to render thread;
- render thread never mutates runtime;
- app shutdown joins render thread before runtime teardown destroys data referenced by queued packets;
- queued packets are immutable and self-contained enough for their lifetime.

## Config Policy

Renderer config may select threading mode only when implemented.

First allowed config:

```text
--vulkan-thread-model=main
```

Deferred values:

```text
render_thread
worker_recording
async_upload
```

Rules:

- any value other than `main` should fail config parsing until implemented;
- threading config is renderer/app startup policy only;
- threading config must not alter runtime command legality, replay hash, or save state;
- strict smoke should print selected thread model.

Suggested receipt fields:

```text
thread_model_requested=main
thread_model_selected=single_main_thread
thread_model_fallback=false
```

## Failure Policy

Threading policy failures map through renderer result/diagnostics:

| Failure | Result | Reason code |
| --- | --- | --- |
| renderer called from non-owner thread | fail in debug/strict, diagnosed failure in release | `renderer_wrong_thread` |
| `FrameInput` retained past call lifetime | test/review failure | `frame_input_lifetime_violation` |
| runtime mutation during renderer call | fail | `runtime_hash_changed` |
| Vulkan queue used from multiple threads without policy | fail | `vulkan_queue_thread_violation` |
| command pool used from multiple threads without policy | fail | `vulkan_command_pool_thread_violation` |
| render-thread config requested before support | config failure | `thread_model_unsupported` |

Rules:

- wrong-thread failures are renderer/app failures, not runtime failures;
- runtime hash change is always a hard failure;
- strict Vulkan smoke must not ignore thread-ownership violations;
- optional smoke may skip missing environment, but not real threading bugs.

## Test Gates

Unit tests should prove:

- default thread model resolves to single main thread;
- unsupported render-thread config fails with `thread_model_unsupported`;
- `RendererApi` public headers expose no thread handles or Vulkan types;
- `FrameInput` validation does not require mutable runtime access;
- null renderer does not retain frame input pointers;
- renderer call does not change runtime hash;
- diagnostics include thread model fields.

Smoke tests should prove:

- Vulkan smoke receipt prints `thread_model=single_main_thread`;
- render frame runs on the owner thread;
- validation/debug callback updates diagnostics without runtime mutation;
- resize/recreate runs on the Vulkan owner thread;
- shutdown/wait-idle runs on the Vulkan owner thread.

Review scans:

```sh
rg -n "std::thread|jthread|async|mutex|condition_variable" src/render src/app apps/iggy3d_visual_demo
rg -n "RenderBackend::renderFrame|renderFrame\\(" src/runtime src/content src/projection src/runtime/save
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected result:

```text
no runtime/content/projection/save renderer ownership leak
no Vulkan leak outside allowed surfaces
no render threading primitives before accepted render-thread packet
```

## Acceptance Criteria

This policy is ready for implementation when:

- [ ] first renderer file plans name main-thread ownership explicitly;
- [ ] `RenderBackend` documents thread affinity;
- [ ] `FrameInput` lifetime is call-bounded or copied explicitly;
- [ ] Vulkan command pool ownership is single-threaded;
- [ ] queue submit/present ownership is single-threaded;
- [ ] diagnostics include thread model fields;
- [ ] unsupported render-thread config fails;
- [ ] runtime replay/hash tests prove renderer calls do not mutate deterministic state;
- [ ] future render-thread work is blocked behind a separate decision packet.

## Non-Goals For First Renderer Pass

Do not add:

- render thread;
- worker command recording;
- secondary command buffers;
- queue submission mutex policy;
- async upload thread;
- texture streaming thread;
- frame pacing thread;
- runtime simulation thread;
- live cross-thread runtime object views;
- lock-free frame queue;
- render-thread shutdown choreography.
