# 31 Qt Product Play Launch Status Sync

Status: complete.

Goal: update planning and API docs after the Qt product play launch/load/build
consumer integrated.

Scope:
- Docs/status sync only.
- Mark `iggy_qt_shell --play PATH` as complete for explicit-path product
  launch/load/build.
- Record that launch runs only `RuntimeGameplayProductScenarioLoader::load`,
  `RuntimeGameplayProductLoop::build`, and
  `RuntimeGameplayProductPlayMode::build`.
- Record stable `IggyQtShellWindow` storage for load/loop/play-mode build
  results and play-mode state, product play context pointer wiring, and reveal
  of existing read-only `panel:product_play`.
- Record `--play`/`--preview` mutual exclusion and exit-2 path errors.
- Frame remaining work as focused input ownership, optional Qt/raw-device input
  mapping, product frame stepping/frame pump, camera lifecycle/presentation
  policy, player sprite and modern NPC actor render projection, pause/retry/reset
  policy, completion/failure evaluator, and save/load productization.

Hard stops preserved:
- No production code changes.
- Qt shell launch/load/build consumer only; not product frame execution, app tick
  loop, raw input mapping, or camera/render/presentation lifecycle.
- No raw Qt/device event mapping or Qt event types into scene/player/runtime
  surfaces.
- No calls to `RuntimeGameplayProductPlayMode::frame(...)` or
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)` from Qt launch.
- No default camera/render config, product input events, latest frame result
  synthesis, or persisted camera/presentation/render-frame data.
- No raw input hidden in runtime/session/gameplay/product-loop/play-mode
  state/snapshots/saves/UI models.
- No product loader/loop/play-mode semantic changes beyond app-shell
  composition.
- No automatic tick loop/frame pump, held-key cadence, pause/retry/reset,
  completion/failure, save/load productization, package scanning/watching/
  discovery, source mutation, or new gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
