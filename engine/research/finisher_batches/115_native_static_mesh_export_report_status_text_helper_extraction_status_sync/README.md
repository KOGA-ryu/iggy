# 115 - Native Static Mesh Export Report Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`06f1251a Move export report status text helper`.

## Integrated Surface

- Added central inline helper `NativeStaticMeshExportReportStatusText(...)`
  beside `NativeStaticMeshExportReportStatus` in
  `NativeStaticMeshExportReport.hpp`.
- Removed the duplicate CLI-local `NativeStaticMeshExportReportStatusName(...)`
  switch from `IggyNativePlay.cpp`.
- Export report row rendering now uses the central helper.

## Stable Strings

- `Writable`
- `WriterFailed`
- fallback `Unknown`

## Preserved Behavior Contract

- Export report output is preserved byte-for-byte for default built-ins.
- The preserved output contract includes the summary row, cube/bean/npc-marker
  asset rows, row order, `status=Writable`, vertices, indices, bytes, issues,
  and trailing newlines.
- No CLI parser/help/dispatch/conflict behavior, report construction,
  writer/policy behavior, filesystem/write behavior, verifier behavior,
  package-directory behavior, file export behavior, manifest behavior, or
  package manifest behavior changed.

## Verification

- `git show --stat --oneline --name-only 06f1251a` showed only
  `NativeStaticMeshExportReport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_mesh_export_report_tests.cpp`.
- `git diff --check 06f1251a^ 06f1251a` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-mesh-export-report`
  matched the exact expected summary plus cube, bean, and npc-marker rows with
  `status=Writable`.
- Direct export report status tests cover `Writable`, `WriterFailed`, and
  `Unknown` fallback.

## Boundaries Preserved

- No export report output changes.
- No row/order/count/status assignment changes.
- No CLI parser/help/dispatch/conflict changes.
- No report construction, writer/policy, filesystem/write, verifier,
  package-directory, file export, manifest, or package manifest behavior
  changes.
- No CMake, fixtures, assets, renderer/model-slot behavior, package
  loading/discovery, schema, glTF/glb/JSON parser work, or source/docs scope
  mixing.

## Finisher Verification

- `git merge-base --is-ancestor 06f1251a HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
