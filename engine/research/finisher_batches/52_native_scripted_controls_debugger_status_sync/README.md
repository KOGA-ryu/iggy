# 52 Native Scripted Controls Debugger Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt scripted controls, scripted control
debugging, player-tile expectations, and final-state dump landed.

## Integrated Commits

- `e6225bdd Add native scripted control trigger`
- `c4803361 Add native scripted control debugger`
- `e1fbea27 Add native scripted final-state dump`

## Integrated Surface

- `iggy_native_play --play PATH --scripted-controls LIST` runs comma-separated
  controls such as `east,east,south` or repeated tokens such as `right*3,wait`.
- Scripted controls inject the same product input path as keyboard controls.
- `--scripted-control-interval-ms` controls delay between scripted controls.
- `--debug-scripted-controls` prints per-step product frame diagnostics.
- `--dump-final-state` prints final scripted state after the scripted sequence
  completes.
- `--expect-player-tiles 'x,y;x,y'` validates the player tile after each expanded
  scripted control.
- `--quit-after-script` exits after the scripted sequence completes.

## Output Contract

- Debugger output includes before/after player tile, frame request status, play
  mode status, surface status, loop status, input event count, ignored input
  event count, accepted/blocked/rejected counts, `npcMoved`, and render command
  count.
- Final-state dump includes player tile, next frame index, render command count,
  active input count, and held input count.
- Expectation mismatch exits nonzero and reports the actual player tile.

## Boundaries Preserved

- Native no-Qt app-shell tooling only.
- No gameplay semantics changes.
- No runtime/product API changes.
- No Qt path changes.
- No persistence changes.
- No renderer extraction, mesh-buffer extraction, math extraction, glTF, asset,
  texture, or material policy changes.

## Verification

- `git diff --check`
- `git status --short --branch`
