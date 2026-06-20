# 97 - Native Static Mesh Package Directory Manifest Read Diagnostics Status Sync

## Goal

Sync planning and API docs after source commit
`4dadf5c9 Add native static mesh package manifest read diagnostics`.

## Integrated Surface

- Package directory report now parses the already-projected nested mesh manifest
  path with `ReadNativeStaticMeshExportManifestFile(...)`, but only when package
  directory read succeeds.
- Summary rows append nested manifest read diagnostics when `manifestPath` is
  available:
  - valid/readable: `manifestRead=ok manifestReadIssues=0`
  - missing, unreadable, or malformed:
    `manifestRead=invalid manifestReadIssues=N`
- The report emits deterministic nested manifest read issue rows after existing
  `packageManifestReadIssue` rows and before asset rows:
  `manifestReadIssue code=<CodeText> line=<line> token=<token>`.
- `NativeStaticMeshExportPackageDirectoryReport::readOk()` is unchanged and
  still wraps package directory read status only.
- Missing or malformed nested mesh manifest remains diagnostic-only: package
  directory report still returns `status=Read`, CLI exits 0, and package asset
  rows remain present.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid smoke passed: batch export followed by package directory report printed
  `manifestRead=ok manifestReadIssues=0` plus existing package, manifest, and
  asset file facts.
- Missing nested manifest smoke exited 0 and printed
  `manifestRead=invalid manifestReadIssues=1` plus
  `manifestReadIssue code=FileOpenFailed line=0 token=<manifest path>`, with
  asset rows still present.
- Malformed nested manifest smoke exited 0 and printed
  `manifestRead=invalid manifestReadIssues=1` plus
  `manifestReadIssue code=UnsupportedVersion line=1 token=2`, with asset rows
  still present.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- Report-only package directory integration; no package directory reader
  status/data change.
- `readOk()` and CLI exit behavior are unchanged.
- Missing/malformed nested mesh manifest remains diagnostic-only and does not
  suppress asset rows.
- No verification/report/export behavior changes beyond package directory report
  text fields and issue rows.
- No exact deterministic verification replacement, semantic package acceptance,
  generated-text comparison, or package acceptance validation.
- No per-mesh-manifest asset rows, package-vs-mesh row comparisons,
  policy/built-in id reconstruction, `.igmesh` loading, geometry validation,
  package loading, discovery/scanning/catalog/registry, exact-extra-file
  rejection, repair, source mutation, or write behavior.
- No renderer behavior, `NativeVulkanRenderer.cpp`, model-slot expansion,
  runtime/product/scene/server API, gameplay/scripted/final-state behavior,
  fixture/generated asset, CMake, glTF/glb/JSON dependency/parser, or
  schema/material/texture/normal/UV/animation changes.

## Finisher Verification

- `git merge-base --is-ancestor 4dadf5c9 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
