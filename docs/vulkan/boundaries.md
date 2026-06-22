# Vulkan Boundaries

Vulkan is an implementation backend. It is not an engine authority.

Current baseline note: `iggy3d-main` already contains backend-neutral render APIs, `src/render/null/**`, Vulkan-private files under `src/render/vulkan/**`, first-room renderer proof surfaces, and Runtime Packet 8 tactical combat runtime state. The boundary rule is not that these files are absent; it is that renderer code remains outside runtime authority and Vulkan/SDL types stay confined to approved renderer/app-platform/smoke surfaces.

## Hard Include Rule

Vulkan headers, `Vk*` types, and `VK_*` constants are allowed only in:

- `src/render/vulkan/**`
- Vulkan-specific app glue when unavoidable for surface creation
- Vulkan-specific smoke tests named under `tests/smoke/vulkan_*`
- Vulkan-specific unit tests only when clearly named as Vulkan backend tests

They are forbidden in:

- `src/runtime/**`
- `src/content/**`
- `src/projection/**`
- `src/runtime/save/**`
- core gameplay docs and file plans

Use this scan as the dependency firewall:

```sh
rg -n '#include[ <"]vulkan/|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected result: no matches.

## Ownership Rules

Runtime owns:

- gameplay truth;
- command legality;
- camera mode truth;
- save/load truth;
- replay determinism;
- deterministic state hash and replay result;
- the authoritative meaning of player actions and world state.

Projection owns:

- backend-neutral scene output;
- backend-neutral debug output;
- derived draw descriptions;
- no gameplay mutation;
- no renderer resources.

Renderer owns:

- GPU resources;
- Vulkan instance/device/surface;
- swapchain;
- shaders and pipelines;
- command buffers;
- synchronization;
- presentation;
- renderer diagnostics;
- platform-specific graphics backend behavior.

## Data Flow

Allowed flow:

```text
runtime truth -> projection output -> FrameInput -> renderer backend -> presentation
```

Forbidden flow:

```text
renderer backend -> runtime truth
renderer backend -> save truth
renderer backend -> replay result
renderer backend -> command legality
```

The renderer may consume matrices, frame timing metadata, projected scene items, projected debug items, and derived camera/frame data. It cannot become save truth and cannot affect deterministic state hash or replay result.

Renderer picking or input features must route through runtime commands later. A renderer may report a picked backend-neutral object id or screen ray to an app/controller layer, but it must not mutate runtime state directly.

## Projection Producer And Renderer Consumer

Use these words when planning the interface:

- Projection producer: runtime/projection code creates backend-neutral scene and debug outputs.
- Renderer consumer: Vulkan consumes backend-neutral draw/frame data and decides only how to draw it.

One module may physically assemble the data at first, but the ownership language must stay strict.

## Dependency Shape

Allowed:

- app depends on runtime, projection, and renderer API;
- renderer API depends on backend-neutral frame input and diagnostics;
- Vulkan backend depends on Vulkan SDK, shader artifacts, and renderer-private modules;
- tests may depend on public APIs or explicitly named backend internals.

Forbidden:

- runtime depending on renderer;
- projection depending on Vulkan;
- content validation depending on renderer;
- save/load depending on renderer;
- deterministic replay depending on frame presentation;
- renderer importing runtime internals to discover state on its own.

## Review Gates

Every renderer packet should answer:

1. Does this introduce a Vulkan include outside the allowed surface?
2. Does this make renderer output part of save or replay truth?
3. Does this let renderer mutate runtime state directly?
4. Does this keep diagnostics backend-neutral outside `src/render/vulkan/**`?
5. Does headless acceptance still pass without a window or GPU?

If any answer is wrong, the packet fails review.
