# 121 - Native Static Model Load Report Text Renderer Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`e02cf6c8 Extract static model load report text renderer`.

## Integrated Surface

- Added pure header-only renderer
  `BuildNativeStaticModelLoadReportText(const NativeStaticModelLoadReport &report)`
  in `NativeStaticModelLoadReport.hpp`.
- `PrintNativeStaticModelLoadReport(...)` now delegates to the renderer helper.
- Removed no-longer-needed app-shell direct using declarations for load-report
  entry/status/fallback/slot text helpers.

## Preserved Report Format

- Summary row.
- Entry row order.
- `slot=`.
- `filename=<missing>` handling.
- `status=`.
- `fallback=`.
- Vertex, index, and issue counts.
- Trailing newlines.

## Preserved Behavior Contract

- CLI output and exit behavior remain unchanged.
- Report data-building semantics are unchanged.
- Static model policy defaults, lookup behavior, fallback assignment behavior,
  CLI parser/help/dispatch/conflict/exit behavior, and renderer/model-slot
  behavior are unchanged.

## Verification

- Baseline was clean at `41a1ad80` before source edits.
- `git show --stat --oneline --name-only e02cf6c8` showed only
  `NativeStaticModelLoadReport.hpp`, `IggyNativePlay.cpp`, and
  `native_static_model_load_report_tests.cpp`.
- `git diff --check` passed before source staging.
- `git diff --cached --check` passed before source commit.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_model_load_report_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_model_load_report_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-model-load-report`
  and `rg` checks confirmed the exact summary row plus all four default slot
  rows.
- Exact text tests cover the default checked-in asset report and a missing-
  policy-ref report branch.

## Boundaries Preserved

- No `NativeStaticModelPolicy.hpp` change.
- No static model policy default, lookup, fallback assignment, report data-
  building semantic, CLI parser/help/dispatch/conflict/exit, renderer/model-slot
  behavior changes.
- No `NativeVulkanRenderer.cpp` changes.
- No checked-in assets or fixtures, `.igmesh` schema/loading, static mesh export/
  package/verification/package-directory behavior, exact verification behavior,
  generated sidecar/export write policy, package loading/discovery/acceptance,
  glTF/glb/JSON parser/dependency work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor e02cf6c8 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
