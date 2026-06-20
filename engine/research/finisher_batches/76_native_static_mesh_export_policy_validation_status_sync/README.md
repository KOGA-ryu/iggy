# 76 - Native Static Mesh Export Policy Validation Status Sync

## Goal

Sync planning/API docs after `06358d91 Validate native static mesh export policy`.

## Integrated Surface

- `ValidateNativeStaticMeshExportPolicy(...)` is now available in
  `NativeStaticMeshExportPolicy.hpp`.
- Validation is backend-free and filesystem-free.
- Structured issue codes are:
  - `EmptyName`
  - `DuplicateName`
  - `EmptyDefaultFilename`
  - `DefaultFilenameContainsSeparator`
  - `DuplicateDefaultFilename`
- `NativeStaticMeshExportPolicyValidationIssue` records issue details.
- `NativeStaticMeshExportPolicyValidationResult::valid()` reports clean policy
  status.
- The default export policy remains unchanged:
  - `cube` / `cube.igmesh`
  - `bean` / `bean.igmesh`
  - `npc-marker` / `npc-marker.igmesh`
- `NativeStaticMeshFileExport.hpp` validates supplied policies before single or
  batch file export performs asset lookup, directory checks, target preflight,
  writer work, or writes.
- `NativeStaticMeshFileExportStatus::InvalidPolicy` and native CLI status-name
  text now cover invalid policies.
- Invalid single export and invalid batch export both return `InvalidPolicy`
  with issue counts and write no files.
- Valid default export report and valid default batch export output remain
  unchanged.

## Boundaries Preserved

- No source, test, CMake, asset, shader, or runtime changes in this docs packet.
- No package/export manifest, sidecar output, package discovery,
  registry/catalog, manifest expansion, source mutation, or authoring package
  policy.
- No checked-in fixture canonicalization or rewrites.
- No overwrite, force, create-directory, or temp replacement policy.
- No renderer loading cleanup or `NativeVulkanRenderer.cpp` changes.
- No glTF/glb/JSON parser or dependency work.
- No `.igmesh` schema, writer, loader, report byte math, valid default CLI
  output, gameplay, or scripted/final-state behavior changes.
- No Linux/dGPU validation lane.

## Verification

- `git merge-base --is-ancestor 06358d91 HEAD`
- `git diff --check`
- `git diff --cached --check`
- Docs-only changed file set.
