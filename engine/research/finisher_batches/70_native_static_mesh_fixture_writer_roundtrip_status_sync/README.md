# 70 Native Static Mesh Fixture Writer Roundtrip Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt added test-only writer roundtrip
coverage for checked-in renderer-bound `.igmesh` fixtures.

## Integrated Commit

- `bfa660be Add native static mesh fixture writer roundtrip tests`

## Integrated Surface

- Test-only extension of `native_static_mesh_asset_writer_tests.cpp`.
- Writer roundtrip now covers checked-in renderer-bound `.igmesh` fixtures:
  - `floor.igmesh`: 4 vertices / 6 indices;
  - `wall.igmesh`: 8 vertices / 36 indices;
  - `npc.igmesh`: 7 vertices / 30 indices;
  - `player.igmesh`: 6 vertices / 24 indices.
- Test flow for each fixture:
  - load with `LoadNativeStaticMeshAssetFile`;
  - write with `WriteNativeStaticMeshAssetText`;
  - reload with `LoadNativeStaticMeshAssetText`;
  - write again;
  - assert canonical writer idempotence.
- Procedural NPC marker write/reload count coverage was added to pair with the
  existing procedural bean coverage.
- CMake only adds `IGGY_NATIVE_PLAY_TEST_ASSET_DIR` to
  `native_static_mesh_asset_writer_tests`.
- No production source, renderer, app shell, asset fixture, CLI, shader,
  runtime/product/scene, or docs changes were in the source packet.

## Boundaries Preserved

- No glTF/glb/JSON parser.
- No GLB/custom parser.
- No dependency fetch, package install, vendoring, or web lookup.
- No `.igmesh` schema expansion or fixture rewrites.
- No file writing/export CLI.
- No normals, UVs, materials, textures, descriptors, samplers, skins, animation,
  scene graph, transforms, or metadata fields.
- No file discovery beyond explicit checked-in test filenames.
- No directory scanning, package discovery, registry/catalog, manifest expansion,
  or authoring package policy.
- No renderer behavior change.
- No `NativeVulkanRenderer.cpp` change.
- No public renderer API change.
- No draw-list/runtime/product/scene/server API change.
- No app-shell/CLI change.
- No gameplay/input/scripted-control change.
- No docs mixed into the source commit.

## Verification

- `git merge-base --is-ancestor bfa660be HEAD`
- `git diff --check`
- `git diff --cached --check`
- `git status --short --branch`
