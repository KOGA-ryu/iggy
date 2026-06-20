# 118 - Native Static Model Slot Text Helper Extraction Status Sync

## Goal

Sync planning and API docs after source commit
`f956d6d8 Move static model slot text helper`.

## Integrated Surface

- Added central inline helper `NativeStaticModelSlotText(...)` beside
  `NativeStaticModelSlot` in `NativeStaticModelPolicy.hpp`.
- Removed the duplicate CLI-local `NativeStaticModelSlotName(...)` switch from
  `IggyNativePlay.cpp`.
- Static model load report `slot=...` rendering now uses the central helper.
- `NativeStaticModelLoadStatusName(...)` and
  `NativeStaticModelFallbackKindName(...)` remain local and unchanged for
  possible later packets.

## Stable Strings

- `Floor`
- `Wall`
- `NpcActor`
- `Player`
- fallback `Unknown`

## Preserved Behavior Contract

- Successful `--dump-static-model-load-report` output is preserved byte-for-byte
  for checked-in assets.
- The preserved output contract includes the summary row, fixed row order, slot
  names, filenames, statuses, fallbacks, counts, issue counts, and trailing
  newlines.
- Static model policy defaults, lookup behavior, load status behavior, fallback
  behavior, CLI parser/help/dispatch/conflict/exit behavior, and renderer/model-
  slot behavior are unchanged.

## Verification

- `git show --stat --oneline --name-only f956d6d8` showed only
  `NativeStaticModelPolicy.hpp`, `IggyNativePlay.cpp`, and
  `native_static_model_policy_tests.cpp`.
- `git diff --check f956d6d8^ f956d6d8` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target native_static_model_policy_tests` passed.
- `ctest --test-dir /Users/kogaryu/iggy/engine/build -R native_static_model_policy_tests --output-on-failure` passed.
- `cmake --build /Users/kogaryu/iggy/engine/build --target iggy_native_play` passed; `NativeVulkanRenderer.cpp` rebuilt because it includes the edited policy header, but no renderer source changed.
- CLI smoke passed:
  `/Users/kogaryu/iggy/engine/build/iggy_native_play --dump-static-model-load-report`
  matched the exact expected summary plus Floor, Wall, NpcActor, and Player rows.
- Direct static model slot text tests cover `Floor`, `Wall`, `NpcActor`,
  `Player`, and `Unknown` fallback.

## Boundaries Preserved

- No static model policy default or lookup changes.
- No static model load report output, row order, load status, fallback, CLI
  parser/help/dispatch/conflict/exit, renderer/model-slot behavior changes.
- No static mesh export/report/manifest/package/verification/package-directory
  behavior changes.
- No CMake, fixtures, assets, package loading/discovery, write policy, schema,
  glTF/glb/JSON parser work, or source/docs scope mixing.

## Finisher Verification

- `git merge-base --is-ancestor f956d6d8 HEAD`.
- Docs-only changed-file set.
- `git diff --check`.
- `git diff --cached --check`.
- Clean final status on `codex/native-renderer-extraction-prep`.
