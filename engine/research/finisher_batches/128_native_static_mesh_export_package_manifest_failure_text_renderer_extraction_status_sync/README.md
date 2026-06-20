# 128 - Native Static Mesh Export Package Manifest Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`c1014062 Extract static mesh package manifest failure text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshExportPackageManifestFailureText(const NativeStaticMeshExportPackageManifestResult &result)`
  beside the package manifest result/status boundary in
  `NativeStaticMeshExportPackageManifest.hpp`.
- `PrintNativeStaticMeshExportPackageManifest()` now delegates the non-written
  failure body through the helper after the existing `!result.written()` check.
- Successful package manifest dumping still prints `result.text` unchanged.

## Preserved Failure Format

```text
static mesh export package manifest failed: <Status> issues=<N>
```

The helper intentionally excludes the `iggy_native_play:` prefix and trailing
newline. The existing exception/catch path still supplies both.

## Verification

- Baseline was clean at `c18d69d6` before source edits.
- `git show --stat --oneline --name-only c1014062` showed only
  `NativeStaticMeshExportPackageManifest.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_export_package_manifest_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_manifest_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_manifest_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Successful package manifest smoke matched the exact summary row and
  cube/bean/npc-marker asset rows.
- Exact invalid-policy helper coverage uses
  `BuildNativeStaticMeshExportPackageManifestText(...)`.

## Boundaries Preserved

- No successful `--dump-static-mesh-export-package-manifest` output, generated
  package manifest text, package manifest build semantic, `written()` semantic,
  `NativeStaticMeshExportPackageManifestResult` data shape, status string, issue
  count semantic, CLI parser/help/dispatch/conflict/exit, app-level error
  prefix/newline, mesh manifest failure text, file export helper, built-in asset
  dump writer failure, verification/report/package-directory, static model,
  renderer/model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor c1014062 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
