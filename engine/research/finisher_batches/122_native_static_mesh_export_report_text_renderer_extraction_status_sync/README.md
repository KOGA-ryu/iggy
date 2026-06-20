# 122 - Native Static Mesh Export Report Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`2f39ab95 Extract static mesh export report text renderer`.

## Integrated Surface

- Added pure header-only renderer
  `BuildNativeStaticMeshExportReportText(const NativeStaticMeshExportReport &report)`
  in `NativeStaticMeshExportReport.hpp`.
- `PrintNativeStaticMeshExportReport(...)` now delegates to the renderer helper.
- Removed no-longer-needed app-shell direct using declarations for export report
  entry/status text rendering.

## Preserved Report Format

- Summary row.
- Entry row order.
- `asset=`.
- `filename=`.
- `status=`.
- Vertex, index, byte, and issue counts.
- Trailing newlines.

## Preserved Behavior Contract

- CLI output and exit behavior remain unchanged.
- Report data-building semantics are unchanged.
- Export policy defaults, writer behavior, byte count semantics, CLI parser/help/
  dispatch/conflict/exit behavior, and renderer/model-slot behavior are
  unchanged.

## Verification

- Baseline was clean at `deab3153` before source edits.
- `git show --stat --oneline --name-only 2f39ab95` showed only
  `NativeStaticMeshExportReport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_export_report_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-mesh-export-report`
  and `rg` checks confirmed the exact summary row plus all three default asset
  rows.
- Exact text tests cover the default three-row report and a custom duplicate
  two-entry policy report.

## Boundaries Preserved

- No export policy default, report data-building semantic, writer behavior, byte
  count semantic, CLI parser/help/dispatch/conflict/exit, static model,
  manifest/package/verification/package-directory, exact verification, generated
  sidecar, export write policy, package loading/discovery/acceptance, renderer/
  model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, glTF/glb/JSON
  parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor 2f39ab95 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
