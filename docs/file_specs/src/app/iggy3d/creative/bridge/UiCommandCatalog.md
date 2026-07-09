# File Spec

Files: `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`

Verified at: `e9c2b0d9`

## Owns

- Static descriptor tables mapping creative UI semantic ids and row ids to product creative UI command kinds.
- Create-palette command rows for allowed object creation shortcuts.
- Command-kind metadata for receipt names and handler expectations.
- Lookup helpers by semantic id, command kind, and command-kind metadata.

## Does Not Own

- Creative UI draw-list row construction.
- Command execution behavior.
- Object descriptor definitions.
- Receipt field emission.
- Creative document mutation or facade state.

## Reads

- Creative object descriptors and descriptor palette policy.
- Creative tool enum values.
- Creative object kinds for create commands.

## Writes / Mutates

- No runtime state.
- Returns spans, descriptor pointers, receipt names, and handler expectation booleans.

## Calls Out To / Wires Out To

- Calls `creative::describeObject(...)`.
- Calls `creative::descriptorShowsInAuthoringBrushPalette(...)`.
- Creative UI model/projection and command frame paths use the catalog for semantic-id routing.

## Called By / Entry Points

- `productCreativeUiCommandCatalog()`.
- `productCreativeUiCreatePalette()`.
- `productCreativeUiCommandKindMetadataCatalog()`.
- `productCreativeUiCreatePaletteEntryAllowed(...)`.
- `findProductCreativeUiCommandBySemanticId(...)`.
- `findFirstProductCreativeUiCommandRowByKind(...)`.
- `productCreativeUiCommandKindReceiptName(...)`.
- `productCreativeUiCommandKindExpectsHandler(...)`.
- Focused proof: `rg -n "productCreativeUiCommandCatalog|findProductCreativeUiCommandBySemanticId|ProductCreativeUiCommandKind" src/app tests`.

## Invariants

- Semantic ids are product UI command routing contracts.
- Create-palette entries are accepted only when descriptor policy permits authoring palette visibility or room-container creation.
- Command-kind metadata must stay aligned with the command enum.
- Receipt names are command-kind facts, not localized labels.
- This catalog is descriptor data and routing lookup, not command execution logic.

## Tests / Proof Commands

- `rg -n "product_creative_ui_command_frame_tests|creative_ui_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "productCreativeUiCommandCatalog|productCreativeUiCreatePalette|findProductCreativeUiCommandBySemanticId" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless command routing or execution changes.
- `src/app/iggy3d/creative/ui/Ui.*` unless row semantic ids or model rows change.
- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless descriptor allow rules change.

## Update When

- Command semantic ids, row ids, labels, command kinds, create palette entries, metadata receipt names, or handler-expectation policy changes.

## Do Not Update When

- Only command execution internals, document mutation behavior, or receipt formatting changes without changing catalog data.
