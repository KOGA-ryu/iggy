# 68 Native Static Model Load Report CLI Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt added a CLI dump for the static
model load report.

## Integrated Commit

- `2766132a Add native static model load report CLI`

## Integrated Surface

- `iggy_native_play` adds `--dump-static-model-load-report`.
- `LaunchOptions::dumpStaticModelLoadReport` plus parse/help integration live in
  `IggyNativePlay.cpp`.
- Main dispatch builds
  `BuildNativeStaticModelLoadReport(DefaultNativeStaticModelPolicy(),
  IGGY_NATIVE_PLAY_ASSET_DIR)`.
- The command prints a compact stdout report and exits before
  `NativeVulkanApp` construction/run.
- Exit code is 0 only when all fixed slots are loaded.
- Exit code is nonzero if any fixed slot is missing or failed.
- The command does not require `--play`, scripted controls, SDL display
  availability, or Vulkan renderer initialization beyond normal binary linkage.
- Compact output starts with:
  - `static-model-load-report loaded=N failed=N missing=N`
- Each slot row includes slot, filename or `<missing>`, status, fallback,
  vertices, indices, and issues.
- Current checked-in asset output includes:
  - Floor 4 vertices / 6 indices;
  - Wall 8 vertices / 36 indices;
  - NpcActor 7 vertices / 30 indices;
  - Player 6 vertices / 24 indices.
- Existing play/scripted/debug/final-state behavior and output are preserved
  except for the added help option.

## Boundaries Preserved

- No glTF/glb parser.
- No GLB binary parser.
- No JSON parser.
- No custom glTF subset parser.
- No dependency fetch, package install, or web lookup.
- No `.igmesh` format/schema change.
- No file discovery, directory scanning, package discovery, asset registry/
  catalog, manifest expansion, or model authoring policy.
- No `NativeVulkanRenderer.cpp` change.
- No public renderer API change.
- No `NativeSceneDrawList.hpp` change.
- No runtime/product/scene/server API change.
- No gameplay, input, scripted-control semantic, CLI/debugger output behavior
  change beyond the new asset diagnostic command/help option.
- No shader/material/texture/descriptor/sampler policy.
- No normals/UVs/animation/skins/scene graph/transforms work.
- No staging/device-local upload policy.
- No Linux/dGPU policy or backend abstraction.
- No docs mixed into the source commit.

## Verification

- `git merge-base --is-ancestor 2766132a HEAD`
- `git diff --check`
- `git diff --cached --check`
- `git status --short --branch`
