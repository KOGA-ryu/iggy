# 92 - Native Static Mesh Package Directory Report CLI Status Sync

## Goal

Sync planning and API docs after source commit
`3beafe78 Add native static mesh package directory report CLI`.

## Integrated Surface

- Added CLI flag `--dump-static-mesh-export-package-directory-report`.
- The flag requires `--output-dir DIR` and is included in the existing
  output-dir allow-list.
- The flag conflicts with existing dump/report/export/verify/asset/model modes:
  - `--dump-static-model-load-report`
  - `--dump-static-mesh-export-package-manifest`
  - `--dump-static-mesh-export-verification-report`
  - `--verify-static-mesh-export`
  - `--export-static-mesh-assets`
  - `--dump-static-mesh-asset`
  - `--dump-static-mesh-export-report`
  - `--dump-static-mesh-export-manifest`
- Added help text for the new flag.
- Early dispatch calls
  `BuildNativeStaticMeshExportPackageDirectoryReport(outputDir)`, writes
  `report.text` to stdout, and returns success only when `report.readOk()` is
  true.
- On non-`Read`, it prints report text first, then exits nonzero through the
  existing `iggy_native_play:` error path with compact status/output/issues.
- Dispatch happens before `NativeVulkanApp` construction and before SDL/Vulkan
  startup.

## Unchanged Behavior

- No package loading, verification semantics, scanning/discovery, export
  mutation, renderer behavior, source parser/dependency work, or package report
  builder changes.
- No `NativeVulkanRenderer.cpp`, model-slot expansion, package discovery/
  scanning/catalog/registry, source mutation, repair, exact-extra-file
  rejection, export behavior changes, write policy changes, deterministic
  verification replacement, export-policy reconstruction, built-in id
  reconstruction, parser/dependency work, `.igmesh` schema/material/texture/
  normal/UV/animation expansion, gameplay/scripted/final-state behavior
  changes, fixture rewrites, docs-in-source, or CMake changes.

## Verification

- Builder verified focused build for
  `native_static_mesh_export_package_directory_report_tests` and
  `iggy_native_play`.
- Builder verified focused CTest for
  `native_static_mesh_export_package_directory_report_tests` passed 1/1.
- Builder verified `./engine/build/iggy_native_play --help` passed and includes
  the new flag.
- Builder verified `./engine/build/iggy_native_play --dump-static-mesh-export-package-directory-report`
  fails nonzero with `requires --output-dir`.
- Builder verified valid export/report smoke:
  `--export-static-mesh-assets --output-dir <tmpdir>` then
  `--dump-static-mesh-export-package-directory-report --output-dir <tmpdir>`;
  output includes `status=Read`, `assets=3`, `issues=0`, package manifest path,
  nested manifest path, and three asset rows.
- Builder verified missing package sidecar smoke failed nonzero after report
  text with `PackageManifestReadFailed`, `issues=1`, and
  `packageManifestReadIssue code=FileOpenFailed`.
- Builder verified malformed package sidecar smoke failed nonzero after report
  text with `PackageManifestReadFailed` and parser issue row.
- Builder verified missing nested `static-mesh-export-manifest.txt` smoke exited
  zero with `status=Read` and projected paths, proving the CLI report remains
  non-verifying.
- Builder verified conflict smoke with `--verify-static-mesh-export` failed
  during parse/conflict handling.
- Builder verified `git diff --check` and `git diff --cached --check` before
  source commit.

## Boundaries Preserved

- Docs-only packet; no source, test, CMake, asset, shader, or runtime edits.
- No next source packet opened or mixed into this docs packet.

## Finisher Verification

- `git merge-base --is-ancestor 3beafe78 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
