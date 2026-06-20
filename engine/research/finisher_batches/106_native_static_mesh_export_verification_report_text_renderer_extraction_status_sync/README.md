# 106 - Native Static Mesh Export Verification Report Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`78650eb2 Extract native static mesh verification report renderer`.

## Integrated Surface

- Added pure verification report renderer helper:
  `BuildNativeStaticMeshExportDirectoryVerificationReportText(const NativeStaticMeshExportDirectoryVerificationReport &report)`.
- Moved only the existing `std::ostringstream` verification report
  serialization from `BuildNativeStaticMeshExportDirectoryVerificationReport(...)`
  into that helper.
- `BuildNativeStaticMeshExportDirectoryVerificationReport(policy, directory)`
  remains the full builder: it still calls
  `VerifyNativeStaticMeshExportDirectory(policy, directory)`, assigns
  `report.text = BuildNativeStaticMeshExportDirectoryVerificationReportText(report)`,
  and returns the same report surface.

## Preserved Text Contract

- Report text is preserved byte-for-byte.
- Preserved scope includes summary row, `status=`, `output=`, `verified=`,
  `issues=`, `manifest=`, `packageManifest=`, optional `problem=`, package
  manifest read issue rows, asset rows, row order, paths, tokens, counts, and
  trailing newlines.

## Verification

- `git show --stat --oneline --name-only 78650eb2` showed only
  `NativeStaticMeshExportDirectoryVerificationReport.hpp` and focused
  verification report tests.
- `git diff --check 78650eb2^ 78650eb2` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_directory_verification_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid CLI smoke passed with `status=Verified`,
  `manifest=ok packageManifest=ok`, and verified cube/bean/npc-marker asset rows.
- Missing package sidecar smoke preserved nonzero CLI behavior and report rows
  for `MissingPackageManifest`, `manifest=ok packageManifest=missing`, and the
  package manifest problem path.
- Malformed package sidecar smoke preserved nonzero CLI behavior and report rows
  for `PackageManifestReadFailed`, `packageManifest=invalid`, and
  `packageManifestReadIssue code=MalformedHeader`.
- Focused parity tests compare renderer output to `report.text` for valid
  export, missing mesh manifest, missing package manifest, malformed package
  sidecar, missing asset, and geometry mismatch.

## Boundaries Preserved

- No verification data/status/order/issue-count changes.
- No CLI exit behavior changes, exact sidecar matching changes, generated
  sidecar/export behavior changes, package acceptance semantics, package
  directory report changes, package loading/discovery, `.igmesh` loading beyond
  existing verifier behavior, renderer/model-slot changes, CMake/assets/fixtures,
  or glTF/glb/JSON parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor 78650eb2 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
