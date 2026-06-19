# 56 Native GPU Mesh Resource Wrapper Status Sync

Status: complete.

## Goal

Sync planning and API docs after the native no-Qt Vulkan renderer wrapped the
existing cube mesh GPU buffers in renderer-private resource structs.

## Integrated Commit

- `e2c61007 Wrap native Vulkan mesh resources`

## Integrated Surface

- Source change is limited to `engine/apps/native_play/NativeVulkanRenderer.cpp`.
- Renderer-private `NativeVulkanBufferResource` and
  `NativeVulkanMeshResource` wrappers now live in the renderer implementation.
- `HasBuffer` and `HasMesh` centralize readiness checks for the existing cube
  mesh path.
- Cube mesh creation now returns/fills wrapped vertex and index buffer
  resources while preserving host-visible/coherent upload semantics and
  caller-provided usage flags.
- Cube data, all scene model ids mapping to the single cube mesh, draw order,
  shader interface, push constants, tint behavior, and
  `VK_INDEX_TYPE_UINT16` indexed draw parameters are intended unchanged.
- Destruction is centralized through `destroyBuffer(...)` and
  `destroyMeshResource(...)`, preserving buffer-before-memory destruction,
  index-before-vertex mesh cleanup, and reset-to-default idempotent cleanup.
- Public renderer API remains unchanged: `NativeVulkanRenderer.hpp` is
  unchanged, `IggyNativePlay.cpp` is unchanged, and CMake is unchanged.
- Reviewer noted that the pre-existing failure-path behavior where a throw
  between `vkCreateBuffer` and ownership assignment can leak a transient
  resource is not newly introduced and is not a blocker for this mechanical
  wrapper packet; a future RAII/allocation exception-safety packet can address
  it if desired.

## Source Verification Facts

- `git diff --check 596f3a9c..HEAD` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play`
  passed.
- Focused product CTest regex passed 10/10:
  `runtime_gameplay_product_(scenario_loader|loop|play_mode|input_accumulator|input_context|input_frame_target_context|input_frame_target_action|frame_request|play_surface_frame|presentation_camera)_tests`.
- Focused render/resource CTest regex passed 9/9:
  `(level_render_frame_2d|render_command_2d|render_command_list_2d_composer|render_resource_registry|material_resource|shader_resource|texture_resource|sprite_render_commands_2d|runtime_gameplay_product_actor_render_commands)_tests`.
- `./engine/build/iggy_native_play --help` passed.
- Native scripted smoke with `multi_frame_guard_room.toml`,
  `--scripted-controls wait`, `--debug-scripted-controls`, `--dump-final-state`,
  and `--quit-after-script` preserved debug/final output, including
  `scripted debug[0] control=wait before=5,1 after=5,1 request=Stepped playMode=Stepped surface=Stepped loop=Stepped inputEvents=1 ignoredInputEvents=0 accepted=1 blocked=0 rejected=0 npcMoved=1 renderCommands=30`
  and
  `scripted final-state playerTile=5,1 nextFrameIndex=1 renderCommands=30 activeInputCount=0 heldInputCount=0`.

## Boundaries Preserved

- No public renderer API changes.
- No `IggyNativePlay.cpp` app-shell behavior changes.
- No CLI/debugger output changes.
- No SDL app-shell extraction or product/session/input/scripted-control semantic
  changes.
- No runtime/product/scene/server/render-command API changes.
- No `NativeSceneDrawList.hpp` model-id or draw-order changes.
- No shader interface/source changes or new shaders.
- No glTF/assets/textures/materials/animation.
- No mesh registry, model-slot binding, resource catalog, resource handles,
  asset loader, or new file IO policy.
- No staging buffer/device-local upload policy; existing host-visible/coherent
  upload remains.

## Verification

- `git merge-base --is-ancestor e2c61007 HEAD`
- `git diff --check`
- `git status --short --branch`
