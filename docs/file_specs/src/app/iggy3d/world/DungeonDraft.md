# File Spec

Files: `src/app/iggy3d/world/DungeonDraft.hpp`, `src/app/iggy3d/world/DungeonDraft.cpp`

Verified at: `10135e4e`

## Owns

- Product dungeon draft cursor, direction, and operation result packets.
- Allowed ASCII draft glyph vocabulary.
- Draft row/column counting and cursor clamping.
- Draft cursor movement and cell painting into `WorldSetupDraft`.
- Custom dungeon room id/source marker applied after edits.

## Does Not Own

- World setup action routing.
- Automation command parsing.
- ASCII room validation or authored room conversion.
- Built-in dungeon catalog selection.

## Reads

- `WorldSetupDraft.asciiRoomText` and cursor/glyph inputs.

## Writes / Mutates

- Mutates caller-provided `WorldSetupDraft` when painting cells.
- Enables ASCII room mode and marks edited drafts with custom room id/source.
- Replaces existing player spawn glyphs when painting a new player spawn.

## Calls Out To / Wires Out To

- No external module calls; this file owns local split/join row mechanics.

## Called By / Entry Points

- Menu action handlers for draft edit-mode cursor movement and paint.
- Automation command handlers for draft cursor/paint/cell commands.
- Tests call draft helpers directly.
- Focused proof: `rg -n "moveProductDungeonDraftCursor|paintProductDungeonDraftCell|setProductDungeonDraftCell|productCustomDungeonRoomId" src/app tests/unit`.

## Invariants

- Empty draft text makes cursor unavailable.
- Cursor movement clamps within existing row and column bounds.
- Invalid glyphs do not mutate the draft.
- Painting player spawn keeps only one `P` glyph.
- Painting marks the draft as custom and ASCII-enabled.

## Tests / Proof Commands

- `rg -n "product_dungeon_draft_tests|product_new_world_menu_action_tests|product_automation_command_registry_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "dungeon_draft_cell_painted|dungeon_draft_invalid_glyph|custom_dungeon_draft" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/frontend/WorldSetupModel.*` unless draft fields or route semantics change.
- `src/app/iggy3d/menu/ActionHandlers.*` unless interactive draft routing changes.
- `src/app/iggy3d/automation/*` unless command parsing changes.

## Update When

- Draft glyph vocabulary, cursor movement rules, custom draft markers, or paint mutation behavior changes.

## Do Not Update When

- Only ASCII parser internals, built-in dungeon catalog text, or menu drawing changes without changing draft operations.
