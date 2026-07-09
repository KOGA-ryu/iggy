# File Spec

Files: `src/app/iggy3d/creative/ui/Ui.hpp`, `src/app/iggy3d/creative/ui/Ui.cpp`

Verified at: `7c9e712f`

## Owns

- Creative editor UI data model: panels, rows, row flags, object summaries, and build requests.
- `CreativeUiPanelKind`, `CreativeUiRowKind`, `CreativeUiRowFlagMask`, `CreativeUiModel`, and `CreativeUiBuildReceipt`.
- Model construction for tools, create palette, status, selection/inspector, measurement, ghost, and snap panels.
- Mapping command catalog entries into UI rows without executing commands.

## Does Not Own

- Draw-list layout, hit-region rectangles, or presentation primitives.
- Creative command execution, input routing, or receipt recording.
- Creative document mutation, persistence, or render wireframe output.

## Reads

- Tool, selection, measurement, snap, ghost, undo, and object-summary state from `CreativeUiBuildRequest`.
- Command rows from `ProductCreativeUiCommandCatalog`.
- Object summary facts for selected target display and inspector rows.

## Writes / Mutates

- Builds `CreativeUiModel` and `CreativeUiBuildReceipt`.
- Does not mutate the request, command catalog, document, or creative app state.

## Calls Out To / Wires Out To

- Uses `productCreativeUiCommandCatalog`, `productCreativeUiCreatePalette`, and command-row lookup helpers.
- Supplies models to `Facade::buildUiModel`, creative UI draw-list projection, UI hit routing, and focused tests.

## Called By / Entry Points

- `makeDefaultCreativeUiBuildRequest`.
- `buildCreativeUiModel`.
- `Facade::buildUiModel`.
- Focused proof: `rg -n "makeDefaultCreativeUiBuildRequest|buildCreativeUiModel|CreativeUiBuildReceipt|CreativeUiPanelKind|CreativeUiRowKind" src/app/iggy3d/creative tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Panel row spans must remain valid: `firstRow + rowCount` stays within `model.rows`.
- Command-backed rows must use command-catalog row ids so draw-list hit semantics stay stable.
- Selection panel is visible with an explicit empty-inspector row when no target is selected.
- Undo row visibility is independent from enabled state; enabled requires available undo and positive depth.
- UI model construction stays data-only and does not perform creative edits.

## Tests / Proof Commands

- `creative_ui_tests` covers default panels, selection/inspector rows, measurement, ghost, snap, command row ids, and stable repeated builds.
- `creative_facade_tests` covers facade-built UI model integration.
- `rg -n "creative_ui_tests|creative_facade_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/ui/UiDrawList.*` unless model-to-draw projection changes.
- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp` unless command rows change.
- `src/app/iggy3d/creative/Facade.*` unless facade model build wiring changes.

## Update When

- UI panel/row kinds, row flags, model-building rules, command-catalog dependencies, or object-summary ownership changes.

## Do Not Update When

- Only visual placement, primitive formatting, hit routing, or command execution changes.
