# 119 - Native Static Model Load Status Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`c886bf6a Move static model load status text helper`.

## Integrated Surface

- Added enum-owned inline helper `NativeStaticModelLoadStatusText(...)` beside
  `NativeStaticModelLoadStatus` in `NativeStaticModelLoadReport.hpp`.
- Replaced the CLI-local static model load status switch in `IggyNativePlay.cpp`
  for existing `status=...` report field rendering.
- Static model load report `status=...` rendering now uses the central helper.
- `NativeStaticModelFallbackKindName(...)` remains local and unchanged for a
  possible later packet.

## Stable Strings

- `MissingPolicyRef`
- `Loaded`
- `LoadFailed`
- fallback `Unknown`

## Preserved Behavior Contract

- Successful `--dump-static-model-load-report` output is preserved byte-for-byte
  for checked-in assets.
- The preserved output contract includes the summary row, fixed row order, slot
  names, filenames, statuses, fallbacks, counts, issue counts, and trailing
  newlines.
- Static model policy defaults, lookup behavior, row order, load status
  assignment, fallback behavior, CLI parser/help/dispatch/conflict/exit
  behavior, and renderer/model-slot behavior are unchanged.

## Verification

- `git show --stat --oneline --name-only c886bf6a` showed only
  `NativeStaticModelLoadReport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_model_load_report_tests.cpp`.
- `git diff --check c886bf6a^ c886bf6a` passed before staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_model_load_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_model_load_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-model-load-report`
  and `rg` checks confirmed the summary row plus all four slot rows with
  `status=Loaded` and unchanged fallback/count fields.
- Direct static model load report tests cover `MissingPolicyRef`, `Loaded`,
  `LoadFailed`, and out-of-range `Unknown` fallback.

## Boundaries Preserved

- No static model policy default or lookup changes.
- No static model load report output, row order, load status assignment,
  fallback, CLI parser/help/dispatch/conflict/exit, renderer/model-slot behavior
  changes.
- No static mesh export/report/manifest/package/verification/package-directory
  behavior changes.
- No CMake, fixtures, assets, package loading/discovery, write policy, schema,
  glTF/glb/JSON parser work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor c886bf6a HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
