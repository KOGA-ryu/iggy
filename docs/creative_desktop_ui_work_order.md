# Creative Desktop UI - Current Claude Work Order

> This file holds one implementation batch. This order supersedes the former
> UI-5 order and the proposed typed-status batch. Build this post-state exactly.
> This is a strict UI-only lane. If the supplied backend contract is incomplete,
> stop and report it; do not implement, repair, or redesign the backend.

## Batch UI-BLOCKOUT-1: Building Blockout Surface

**Required baseline:** `creative-only` at or after:

- `3fade654` - building blockout semantic backend and typed command
- `a7a41203` - declarative tool presentation model

The checkout may show modified nested Claude worktree gitlinks and this
Codex-owned work-order file. They are pre-existing/outside Claude's batch. Do
not stage, reset, clean, modify, or commit them.

## Why This Batch Exists

The semantic backend now creates a complete one-level building source from a
single rectangular footprint. The desktop UI does not expose it. Add one
focused authoring surface so a creator can choose a footprint and room split,
stage the source, then use the already-existing Preview 3D and Confirm &
Generate workflow.

Do not begin typed status, Tool Settings, document tabs, category-mode toolbox,
controller navigation, mirror deletion, preview coalescing, or another roadmap
milestone in this batch.

## Backend Contract - Consume, Do Not Recreate

### Pure blockout planner

Owner:
`src/app/iggy3d/creative/world/WorldLayoutBlockout.hpp/.cpp`

Use these existing symbols:

- `CreativeWorldLayoutBuildingBlockoutPattern`
  - `SingleRoom`
  - `SplitX`
  - `SplitZ`
  - `Grid2x2`
- `CreativeWorldLayoutBuildingBlockoutRequest`
- `CreativeWorldLayoutBuildingBlockoutPlan`
- `planCreativeWorldLayoutBuildingBlockout(...)`

The planner owns all split math and validity rules. It produces deterministic,
row-major integer-grid room rectangles. On odd spans, the positive X/Z side
owns the extra cell. The UI must not duplicate or reinterpret those rules.

### App mutation

Owners:

- `apps/iggy3d_creative/EditorWorldLayoutState.hpp`
- `apps/iggy3d_creative/EditorWorldLayout.hpp`
- `apps/iggy3d_creative/EditorWorldLayoutShells.cpp`

Use:

- `CreativeEditorWorldLayoutBuildingBlockoutSettings`
  - `.shell` is a `CreativeEditorWorldLayoutRoomSettings`
  - `.pattern` is the planner enum
- `createCreativeEditorWorldLayoutBuildingBlockout(...)`

The mutation atomically stages one building, one level, and one/two/four rooms,
selects the new building, and advances source history once. It owns overlap
rejection, including the vertical wall interval. Exact vertical stacking at a
previous wall top is legal.

### Typed desktop command

Owners:

- `apps/iggy3d_creative/EditorDesktopCommands.hpp`
- `apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp`
- `apps/iggy3d_creative/EditorDesktopWorldLayoutBuildingCommands.cpp`

Emit exactly:

```cpp
commands.push(
    CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
    CreativeDesktopWorldLayoutBuildingBlockoutPayload{settings});
```

Do not call the mutation directly from a widget. Do not add a command id,
payload type, queue, or dispatch site.

### Existing lifecycle

The bottom Build panel already owns:

- `WorldLayoutPreview`
- `WorldLayoutConfirm`
- `WorldLayoutCancelPreview`
- diagnostics, status, and generation receipts

Do not duplicate those buttons in the new blockout surface. After staging, the
creator uses the existing bottom panel to preview and generate. Surface the
existing `worldLayout.statusMessage`; do not classify it or create a second
status channel.

## Required UI

Add a `Building Blockout` section at the top of the existing **Create** tab,
before the legacy palette. This is a parameterized creation action, not a
continuous canvas tool:

- Do not add it to the toolbox glyph table.
- Do not add a new active-tool state.
- Do not make placement depend on the current selected tool.
- Do not mutate the World Layout source while editing fields.

Store one transient draft in `CreativeEditorDesktopUiState`. It is UI state,
not document truth and not persistent state.

### Draft Defaults

- footprint minimum: `X=0, Z=0`
- footprint maximum: `X=8, Z=8`
- pattern: `SingleRoom`
- all shell values other than the footprint come from the existing
  `CreativeEditorWorldLayoutRoomSettings` defaults

The rectangle fields are integer grid-line coordinates. Maximums are exclusive.
Use four direct fields:

- Min X
- Min Z
- Max X
- Max Z

Do not introduce meters, center/half-extents, inclusive maximums, or an
alternate coordinate convention.

### Layout Pattern

Render a compact four-choice segmented/radio control:

- `1 room`
- `Split X`
- `Split Z`
- `2 x 2`

The selected choice writes only the enum. Do not preview split lines by locally
recomputing them. The canvas/source reflects the rooms after the command is
accepted.

### Shell Settings

Always show:

- Floor top
- Wall height
- Wall thickness
- Floor layers
- Roof layers

Place roof shape controls under a compact `Roof` disclosure:

- Style: Flat / Gable
- Overhang
- For Gable only: ridge axis X/Z and pitch

Match the labels, numeric formats, steps, and enum choices already used by
`EditorDesktopWorldLayoutSourceInspector.cpp`. Reuse constants from the
existing contracts. Do not create UI-only limits or copy a backend validator.

### Action

One primary button: `Stage blockout`.

On press, emit exactly one `WorldLayoutCreateBuildingBlockout` command with a
by-value copy of the draft settings. The widget must not:

- edit `state.source` directly;
- select or create source records itself;
- invoke preview or confirm automatically;
- reset a valid draft after success;
- infer success before dispatch.

Show the existing `state.statusMessage` below the action. Backend rejection is
visible there, including invalid dimensions and overlap.

### Availability

Disable the entire section under the same edit locks already applied to Create:

- Play mode / asset edit
- building transform
- building-template placement
- terrain-region editing

Also disable `Stage blockout` while an exact World Layout preview is active.
Do not silently cancel another workflow.

## Strict Write Scope

Claude may edit only:

- `apps/iggy3d_creative/EditorDesktopUi.hpp`
- `apps/iggy3d_creative/EditorWorldLayoutToolPalette.cpp`
- `apps/iggy3d_creative/EditorWorldLayoutPanelInternal.hpp` only if a small
  UI-only helper declaration is required
- `tests/unit/creative_editor_toolbox_tests.cpp` only for a pure UI-model pin
  that materially proves this batch

Do not edit any other file. In particular, do not edit:

- anything under `src/`;
- `EditorWorldLayoutState.hpp`, `EditorWorldLayout.hpp`, or any
  `EditorWorldLayout*.cpp` semantic owner other than the allowlisted
  ToolPalette UI file;
- `EditorDesktopCommands.hpp`, `EditorDesktopCommandPayloads.hpp`, or any
  dispatcher;
- `EditorToolPresentation.hpp/.cpp`;
- CMake/build registration;
- recipe, compiler, reconciliation, history, codec, save, renderer, input, or
  controller code;
- the bottom Build panel or existing Preview/Confirm behavior.

No fallback enum, struct, command, split helper, geometry helper, compatibility
shim, or duplicate validator is allowed.

## Stop Conditions

Stop and report the exact symbol/path and command output if:

1. any backend symbol above is absent or does not compile;
2. the typed command cannot carry the complete settings value;
3. the Create-tab lock state cannot be reused without leaving the allowlist;
4. the widget would need direct source mutation or local split/overlap math;
5. a required behavior needs a non-allowlisted production edit;
6. baseline focused tests fail for a reason unrelated to this UI slice.

An incomplete backend capability is Codex's concern, not Claude's. Do not widen
the lane to fix it.

## Verification

Run headlessly; do not launch a window:

```sh
cmake --build build --target \
  i3dc \
  creative_editor_toolbox_tests \
  creative_editor_building_blockout_tests \
  creative_desktop_ui_command_tests

ctest --test-dir build -R \
  '^(creative_editor_toolbox_tests|creative_editor_building_blockout_tests|creative_desktop_ui_command_tests)$' \
  --output-on-failure

git diff --check
```

Do not run broad CTest. Do not launch `i3dc`. Do not modify a failing backend
test.

## Acceptance

- The Create tab exposes one Building Blockout section.
- Drafting fields do not mutate the source.
- All four pattern values map one-to-one to the existing enum.
- One click emits one typed blockout command.
- Existing Preview/Confirm remains the only generation lifecycle.
- Backend status is visible without string classification.
- No non-allowlisted file changes.
- Focused build/tests and whitespace check pass.

## Completion Brief

Report:

- commit hash;
- exact changed files;
- UI behavior added;
- exact command/payload emitted;
- tests and results;
- confirmation that no backend, command, dispatcher, build, persistence,
  renderer, input, or controller file changed;
- any backend limitation encountered as a STOP, without repairing it.
