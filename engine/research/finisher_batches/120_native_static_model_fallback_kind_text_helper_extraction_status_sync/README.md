# 120 - Native Static Model Fallback Kind Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`20440310 Move static model fallback kind text helper`.

## Integrated Surface

- Added enum-owned inline helper `NativeStaticModelFallbackKindText(...)` beside
  `NativeStaticModelFallbackKind` in `NativeStaticModelLoadReport.hpp`.
- Removed the CLI-local fallback-kind switch from `IggyNativePlay.cpp`.
- Static model load report `fallback=...` rendering now uses the central helper.

## Stable Strings

- `Cube`
- `ProceduralBean`
- `ProceduralNpcMarker`
- fallback `Unknown`

## Preserved Behavior Contract

- Successful `--dump-static-model-load-report` output is preserved byte-for-byte
  for checked-in assets.
- The preserved output contract includes the summary row, fixed row order, slot
  names, filenames, statuses, fallback names, counts, issue counts, and trailing
  newlines.
- Fallback assignment behavior, static model policy defaults, lookup behavior,
  CLI parser/help/dispatch/conflict/exit behavior, and renderer/model-slot
  behavior are unchanged.

## Verification

- Baseline was clean at `989d0f8c` before source edits.
- `git show --stat --oneline --name-only 20440310` showed only
  `NativeStaticModelLoadReport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_model_load_report_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_model_load_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_model_load_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-model-load-report`
  and `rg` checks confirmed the summary row plus all four slot rows with
  unchanged fallback names and count fields.
- Direct static model load report tests cover `Cube`, `ProceduralBean`,
  `ProceduralNpcMarker`, and out-of-range `Unknown` fallback.

## Boundaries Preserved

- No `NativeStaticModelPolicy.hpp` change.
- No fallback assignment behavior, static model policy default, lookup, static
  model load report output, row order, CLI parser/help/dispatch/conflict/exit,
  renderer/model-slot behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, static mesh export/
  package/verification/package-directory behavior, package loading/discovery/
  acceptance, export write policy, glTF/glb/JSON parser/dependency work, or
  source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor 20440310 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
