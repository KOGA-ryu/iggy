# File Spec

Files: `src/app/iggy3d/world/BuiltinDungeon.hpp`, `src/app/iggy3d/world/BuiltinDungeon.cpp`

Verified at: `eebd1820`

## Owns

- Built-in product dungeon catalog rows: title, room id, source name, and ASCII room text.
- Default dungeon selection and lookup by room id.
- Conversion from built-in/world setup draft to ASCII authoring request.
- Next/previous built-in dungeon selection into `WorldSetupDraft`.
- Product default world setup draft seeded from the first built-in dungeon.

## Does Not Own

- ASCII parsing, validation, room asset conversion, or package construction.
- World setup menu rendering or input routing.
- Runtime session creation.
- Save/load/catalog behavior.

## Reads

- Static built-in dungeon definitions in this file.
- `WorldSetupDraft` values when building authoring requests or changing selected dungeon.
- Movement test lab room-id policy for optional injected lab objects.

## Writes / Mutates

- Mutates caller-provided `WorldSetupDraft` when applying or cycling built-in dungeons.
- Returns ASCII authoring request packets.
- Does not mutate app/window state directly.

## Calls Out To / Wires Out To

- `makeDefaultWorldSetupDraft(...)`.
- `productMovementTestLabRoomId(...)` for lab-object injection eligibility.

## Called By / Entry Points

- App kernel default world setup draft initialization.
- World setup action handlers, automation, menu draw/list/view code, and new-world launch.
- Focused proof: `rg -n "productBuiltinDungeonCatalog|productWorldSetupAuthoringRequest|selectNextProductBuiltinDungeon|makeProductDefaultWorldSetupDraft" src/app tests/unit`.

## Invariants

- Catalog order defines default selection and next/previous cycling.
- Invalid apply index must fail without mutating through fallback.
- Built-in draft application sets world name, ASCII text/id/source, enables ASCII, and selects Create field.
- Movement test lab injection only applies when the draft text still matches the built-in catalog row.
- This catalog is product fixture/layout truth, not runtime simulation truth.

## Tests / Proof Commands

- `rg -n "product_builtin_dungeon_tests|product_new_world_menu_action_tests|product_dungeon_draft_tests|world_setup_model_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "productBuiltinDungeonCatalog|selectNextProductBuiltinDungeon|makeProductDefaultWorldSetupDraft" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/*` unless authoring request contracts change.
- `src/app/iggy3d/world/ProductNewWorldLaunch.*` unless launch consumption of drafts changes.
- `src/app/frontend/WorldSetupModel.*` unless draft fields or route rules change.

## Update When

- Built-in catalog rows, default/cycling policy, draft application fields, or authoring request conversion changes.

## Do Not Update When

- Only ASCII parser internals, room asset projection, or save behavior changes without changing built-in dungeon catalog/draft contracts.
