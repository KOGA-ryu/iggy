# 57 Native Pipeline Shader Resource Wrapper Status Sync

Status: complete.

## Goal

Sync planning and API docs after the native no-Qt Vulkan renderer wrapped the
existing render pass, pipeline layout, graphics pipeline, and shader module
handles in renderer-private resource structs.

## Integrated Commit

- `ec849e23 Wrap native Vulkan pipeline resources`

## Integrated Surface

- Source change is limited to `engine/apps/native_play/NativeVulkanRenderer.cpp`.
- Renderer-private `NativeVulkanShaderModuleResource` and
  `NativeVulkanPipelineResource` wrappers now live in the renderer
  implementation.
- `HasShaderModule` and `HasPipeline` centralize readiness checks for the
  existing shader module and pipeline handles.
- `createRenderPass()` fills the wrapped pipeline resource's render pass.
- `createShaderModule(...)` returns a wrapped shader module resource, and
  `destroyShaderModule(...)` centralizes shader module destruction and reset.
- `createGraphicsPipeline()` still reads `cube.vert.spv` and `cube.frag.spv`,
  uses the same shader stage setup, `pName = "main"`, vertex input, fixed
  pipeline state, push constant range, pipeline layout semantics, and render
  pass semantics.
- Command recording and draw code now use `pipeline_.renderPass`,
  `pipeline_.layout`, and `pipeline_.graphics`.
- `destroyPipelineResource(...)` centralizes graphics pipeline, pipeline layout,
  and render pass destruction in that order and resets the resource.
- Public renderer API remains unchanged: `NativeVulkanRenderer.hpp`,
  `IggyNativePlay.cpp`, CMake, shader files, runtime/product/scene APIs,
  product session, and draw-list data are unchanged.

## Source Verification Facts

- `git diff --check` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play`
  passed.
- Focused product CTest regex passed 10/10:
  `runtime_gameplay_product_(scenario_loader|loop|play_mode|input_accumulator|input_context|input_frame_target_context|input_frame_target_action|frame_request|play_surface_frame|presentation_camera)_tests`.
- Focused render/resource CTest regex passed 9/9:
  `(level_render_frame_2d|render_command_2d|render_command_list_2d_composer|render_resource_registry|material_resource|shader_resource|texture_resource|sprite_render_commands_2d|runtime_gameplay_product_actor_render_commands)_tests`.
- `./engine/build/iggy_native_play --help` passed.
- Native scripted smoke with `multi_frame_guard_room.toml`,
  `--scripted-controls wait`, `--debug-scripted-controls`,
  `--dump-final-state`, and `--quit-after-script` passed outside the sandbox,
  preserving debug/final output, including
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
- No descriptors, samplers, textures/materials/assets/glTF, model slots,
  resource catalogs, or new file IO policy.
- No staging buffer/device-local upload policy.
- No Linux/dGPU validation policy or backend abstraction.

## Verification

- `git merge-base --is-ancestor ec849e23 HEAD`
- `git diff --check`
- `git status --short --branch`
