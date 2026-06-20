# 130 - Native Static Mesh Export Verification Failure Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`df078350 Extract static mesh verification failure text`.

## Integrated Surface

- Added pure header helper
  `BuildNativeStaticMeshExportDirectoryVerificationFailureText(const NativeStaticMeshExportDirectoryVerificationResult &result)`
  beside the verification result/status boundary in
  `NativeStaticMeshExportDirectoryVerification.hpp`.
- `PrintNativeStaticMeshExportDirectoryVerification(...)` now delegates only the
  non-verified failure body through the helper after `!result.verified()`.
- App-level `iggy_native_play:` prefix and newline behavior remain owned by the
  existing catch path.
- Verification success text remains routed through the existing success helper
  and was not changed.

## Preserved Failure Format

```text
static mesh export verification failed: <Status> output=<path> issues=<N>
```

The helper emits the failure body without an embedded app prefix and without an
embedded trailing newline.

## Output Path Selection

- Uses `problemPath` when present.
- Falls back to `outputDirectory` otherwise.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_directory_verification_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_directory_verification_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed for missing manifest failure string.
- CLI smoke passed for missing package manifest failure string.
- CLI smoke passed for unchanged verification success string.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- No verification success text, verification report, package-directory report,
  verifier behavior, status ordering, result shape,
  `VerifyNativeStaticMeshExportDirectory(...)`, `verified()` semantic, status
  string, problem-path setting, issue-count semantic, package manifest read
  diagnostic, exact sidecar matching, CLI parser/help/dispatch/conflict/exit
  behavior, or app-level error prefix/newline behavior changes.
- No built-in asset dump, file export, manifest/package-manifest, CMake/docs
  mixing, ledger, sidecar generation, export write policy, package loading/
  discovery/acceptance, `NativeVulkanRenderer.cpp`, renderer/model-slot
  behavior, checked-in asset or fixture, `.igmesh` schema/loading, or glTF/glb/
  JSON parser/dependency work.

## Finisher Verification

- `git merge-base --is-ancestor df078350 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
