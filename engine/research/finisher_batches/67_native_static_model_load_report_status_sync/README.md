# 67 Native Static Model Load Report Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering added a backend-free
static model load report over the value-only `.igmesh` policy path.

## Integrated Commit

- `8b125b81 Add native static model load report`

## Integrated Surface

- `NativeStaticModelLoadReport.hpp` adds a backend-free app-local report over
  the existing value-only `NativeStaticModelPolicy` and existing `.igmesh`
  loader.
- Report iteration is fixed: `Floor`, `Wall`, `NpcActor`, `Player`.
- Each per-slot entry records:
  - slot;
  - mesh filename;
  - load status;
  - fallback kind;
  - issue count;
  - vertex count;
  - index count.
- Aggregate report counts track loaded, failed, and missing entries.
- Status values are `MissingPolicyRef`, `Loaded`, and `LoadFailed`.
- Report-only fallback mapping is:
  - `Floor -> Cube`
  - `Wall -> Cube`
  - `NpcActor -> ProceduralNpcMarker`
  - `Player -> ProceduralBean`
- The builder uses `FindNativeStaticModelAsset(policy, slot)` and
  `LoadNativeStaticMeshAssetFile(assetRoot / meshFilename)` only for explicit
  policy refs.
- Tests cover default checked-in assets and counts:
  - `floor` 4 vertices / 6 indices;
  - `wall` 8 vertices / 36 indices;
  - `npc` 7 vertices / 30 indices;
  - `player` 6 vertices / 24 indices.
- Tests also cover missing policy refs, bad filename/load failure, and no
  inference of unlisted assets.
- `NativeVulkanRenderer.cpp` was not touched.
- No renderer mutation or GPU/Vulkan/SDL behavior was added.

## Boundaries Preserved

- No glTF/glb parser.
- No GLB binary parser.
- No JSON parser.
- No custom glTF subset parser.
- No dependency fetch or package install.
- No file discovery, package discovery, registry/catalog, model authoring
  policy, or asset manifest beyond explicit policy filenames.
- No `IggyNativePlay.cpp`, public renderer API, `NativeSceneDrawList.hpp`,
  runtime/product/scene/server API, gameplay/input/scripted-control/CLI/debugger
  output change.
- No shader/material/texture/descriptor/sampler policy.
- No normals/UVs/animation/skins/scene graph/transforms work.
- No staging/device-local upload policy.
- No Linux/dGPU policy or backend abstraction.
- No docs mixed into the source commit.

## Verification

- `git merge-base --is-ancestor 8b125b81 HEAD`
- `git diff --check`
- `git diff --cached --check`
- `git status --short --branch`
