# 98 - Native Static Mesh Package Directory Manifest Asset Rows Status Sync

## Goal

Sync planning and API docs after source commit
`ae27d03b Add native static mesh package manifest asset rows`.

## Integrated Surface

- Package directory report now emits parsed nested mesh manifest asset rows only
  when the projected nested manifest reads successfully.
- New diagnostic row shape:
  `manifestAsset=<name> filename=<filename> vertices=<vertexCount> indices=<indexCount> bytes=<byteCount>`.
- Rows are emitted after any `manifestReadIssue` block and before existing
  package-declared `asset=` rows.
- Valid batch export reports three manifest rows:
  - `manifestAsset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523`
  - `manifestAsset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882`
  - `manifestAsset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474`
- Missing or malformed nested mesh manifests remain diagnostic-only: no
  `manifestAsset=` rows are emitted, package asset rows remain present,
  `status=Read` and CLI exit 0 are preserved.

## Verification

- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_mesh_export_package_directory_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_mesh_export_package_directory_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- Valid smoke passed: batch export followed by package directory report printed
  `manifestRead=ok manifestReadIssues=0` and three `manifestAsset=` rows before
  package `asset=` rows.
- Missing nested manifest smoke exited 0 with
  `manifestRead=invalid manifestReadIssues=1`,
  `manifestReadIssue code=FileOpenFailed ...`, no `manifestAsset=` rows, and
  package asset rows still present.
- Malformed nested manifest smoke exited 0 with
  `manifestReadIssue code=UnsupportedVersion line=1 token=2`, no
  `manifestAsset=` rows, and package asset rows still present.
- `git diff --check` passed.
- `git diff --cached --check` passed before source commit.

## Boundaries Preserved

- Diagnostic row projection only; no package-vs-mesh comparison semantics.
- No validity/mismatch statuses, semantic package acceptance, generated-text
  comparison, package acceptance validation, or exact deterministic verification
  replacement.
- No package directory reader status/data changes, `readOk()` changes, or CLI
  exit behavior changes.
- Missing/malformed nested mesh manifests still emit no `manifestAsset=` rows
  and remain `status=Read` / exit 0 with package asset rows present.
- No policy reconstruction, built-in id reconstruction, `.igmesh` loading,
  geometry validation, asset path traversal from mesh-manifest rows, file facts
  for mesh-manifest-declared filenames, package loading,
  discovery/scanning/catalog/registry, exact-extra-file rejection, repair,
  source mutation, or write behavior.
- No renderer behavior, `NativeVulkanRenderer.cpp`, model-slot expansion,
  runtime/product/scene/server API, gameplay/scripted/final-state behavior,
  fixture/generated asset, CMake, glTF/glb/JSON dependency/parser, or
  schema/material/texture/normal/UV/animation changes.

## Finisher Verification

- `git merge-base --is-ancestor ae27d03b HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
