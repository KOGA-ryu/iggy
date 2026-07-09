# File Spec

Files: `src/app/iggy3d/menu/Notebook.hpp`, `src/app/iggy3d/menu/Notebook.cpp`

Verified at: `0996a5ee`

## Owns

- In-game recon notebook draw-list model and builder.
- `ProductNotebookTab`, `ProductNotebookReconPage`, and `ProductNotebookUiRequest`.
- Notebook tab labels and notebook sound-event ids shared by future interaction code.
- Pure request-to-`ProductUiDrawList` construction for the book shell, ASCII floor-plan sketch, notes sections, tabs, and page number.

## Does Not Own

- Runtime opening, tab switching, audio dispatch, or input handling.
- Editable room, creative document, or scout-data source truth.
- Widget primitive implementation.
- Hit routing beyond emitted primitive and semantic-id data.

## Reads

- Requested tab, virtual size, and optional recon page pointer.
- Recon page title, floor-plan ASCII rows, garrison notes, patrol notes, hazard notes, and page number.
- `ProductUiDrawList` and widget-layer primitive contracts.

## Writes / Mutates

- Builds and returns a `ProductUiDrawList`.
- Does not mutate the request or recon page.

## Calls Out To / Wires Out To

- Emits widget-layer panels/text through `emit(...)`.
- Flushes `WidgetOutput` into the draw list through `appendWidgetOutput(...)`.
- Uses `ProductUiThemeId::Journal` and notebook-prefixed semantic ids for downstream presentation and tests.

## Called By / Entry Points

- `productNotebookTabName`.
- `buildProductNotebookUiDrawList`.
- Focused proof: `rg -n "buildProductNotebookUiDrawList|ProductNotebook|notebook\\." src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Null page returns a not-ready draw list with `product_notebook_ui_missing_page`.
- Ready draw lists keep the request virtual size and set selected action from the active tab name.
- ASCII floor-plan cells are deterministic; walls fill cells and `D/W/V/G` emit accent markers plus glyphs.
- The draw builder remains pure; interaction actions and audio events are not queued here.

## Tests / Proof Commands

- `product_notebook_ui_draw_list_tests` covers ready layout, plan markers, notes, selected tab, and missing page behavior.
- `rg -n "product_notebook_ui_draw_list_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/DrawList.*` unless draw-list schema changes.
- `src/app/iggy3d/ui/Widget.*` unless widget emission contracts change.
- Runtime room, creative document, and scout-data files unless notebook input data ownership changes.

## Update When

- Notebook data structs, draw-list status strings, semantic-id scheme, tab set, theme, or emitted primitive responsibilities change.

## Do Not Update When

- Only the source of recon data or runtime input/audio routing changes without changing this draw-list contract.
