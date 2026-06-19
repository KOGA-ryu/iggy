# 66 Native Static Model Policy Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering added an app-local
value-only static model slot policy with `.igmesh` filenames first.

## Integrated Commit

- `36e5881f Add native static model policy`

## Integrated Surface

- `NativeStaticModelPolicy.hpp` adds an app-local value-only policy under
  `engine/apps/native_play`.
- `NativeStaticModelSlot` covers `Floor`, `Wall`, `NpcActor`, and `Player`.
- `NativeStaticModelAssetRef` carries `slot` plus `meshFilename`.
- `NativeStaticModelPolicy` stores a `models` vector.
- `DefaultNativeStaticModelPolicy()` provides stable default entries:
  - `Floor -> floor.igmesh`
  - `Wall -> wall.igmesh`
  - `NpcActor -> npc.igmesh`
  - `Player -> player.igmesh`
- `FindNativeStaticModelAsset(...)` returns the first matching slot entry.
- The policy has no filesystem, file loading, parsing, GPU, or Vulkan knowledge.
- `NativeVulkanRenderer.cpp` consumes the policy only for filenames while
  preserving existing loaded `.igmesh` behavior and fallbacks.

## Future glTF/glb Boundary

- No glTF/glb parser or dependency exists yet.
- The future subset remains separate and constrained:
  - one mesh;
  - one primitive;
  - triangles;
  - positions required;
  - optional vertex colors/default later;
  - indexed `uint16` first;
  - no materials, textures, normals, UVs, animation, skins, scene graph, or
    transforms.

## Source Verification Facts

- Source packet touched:
  - `engine/apps/native_play/NativeStaticModelPolicy.hpp`
  - `engine/apps/native_play/NativeVulkanRenderer.cpp`
  - `engine/cmake/iggy_core_tests.cmake`
  - `engine/tests/native_static_model_policy_tests.cpp`
- Planner applied `PASS-planner` after builder verification/source gate checks
  were clean.
- Focused policy tests cover stable default entries, lookup, missing slots,
  duplicate first-match behavior, and value-only filenames.

## Boundaries Preserved

- No glTF/glb parser.
- No JSON/GLB parser or new dependency/vendored parser.
- No move to `engine/src/servers/render`.
- No public renderer API change.
- No app shell change.
- No runtime/product/scene/server/draw-list API change.
- No shader/material/texture/descriptor/sampler policy.
- No staging/device-local upload policy.
- No Linux/dGPU policy or backend abstraction.
- No CLI/debugger output change.
- No gameplay, session, input, or scripted-control change.
- No docs mixed into the source commit.

## Verification

- `git merge-base --is-ancestor 36e5881f HEAD`
- `git diff --check`
- `git diff --cached --check`
- `git status --short --branch`
