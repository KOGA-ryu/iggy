# 71 - Native Static Mesh Built-In Export CLI Status Sync

## Goal

Sync planning/API docs after `f651f706 Add native static mesh built-in export CLI`.

## Integrated Surface

- `iggy_native_play --dump-static-mesh-asset NAME` is now available.
- Supported names are exactly `cube`, `bean`, and `npc-marker`.
- The command chooses the existing built-in procedural mesh, serializes it
  through `WriteNativeStaticMeshAssetText(...)`, prints raw deterministic
  `.igmesh` text to stdout, and exits 0 before `NativeVulkanApp` construction,
  SDL initialization, or Vulkan launch.
- Unknown names fail nonzero with compact errors such as
  `iggy_native_play: unknown static mesh asset: nope`.
- Combining `--dump-static-model-load-report` with
  `--dump-static-mesh-asset` fails nonzero with a compact conflict error so
  stdout formats stay unambiguous.
- Existing help/report/scripted/final-state/product/session/render behavior is
  preserved except for the added help option.
- Sample output for `cube`, `bean`, and `npc-marker` starts with
  `# Native static mesh asset`; cube output includes cube vertex rows and a
  later `tri 0 1 2`, while bean and NPC marker output include their existing
  procedural vertex rows.

## Boundaries Preserved

- No file writes, fixture rewrites, arbitrary asset path input, or checked-in
  asset normalization.
- No glTF/glb/JSON parsing, dependency work, or `.igmesh` schema expansion.
- No material, texture, descriptor, sampler, normal, UV, animation, or scene
  graph fields.
- No renderer behavior/API changes.
- No runtime/product/scene/server/draw-list API changes.
- No gameplay/scripted/final-state semantic changes.
- No docs mixed into the source commit.

## Verification

- `git merge-base --is-ancestor f651f706 HEAD`
- `git diff --check`
- `git diff --cached --check`
- Docs-only changed file set.
