# File Spec

Files: `src/app/frontend/WorldSetupModel.hpp`, `src/app/frontend/WorldSetupModel.cpp`

Verified at: `158627a3`

## Owns

- Frontend new-world setup draft, validation, create request, and route result packets.
- World setup field, difficulty, and scenario enums with stable names.
- Default draft creation, seed resolution, draft validation, and route-to-create/back logic.

## Does Not Own

- Product-specific dungeon draft helpers, ASCII room conversion, package/session creation, save creation, UI drawing, or frontend state mutation.

## Reads

- World setup draft text, selected field, difficulty, scenario, and ASCII-room fields.
- Selected frontend action for route handling.

## Writes / Mutates

- No mutation; returns draft, validation, route result, or create request packets.

## Calls Out To / Wires Out To

- Product world creation consumes `WorldSetupCreateRequest`.
- Product menu action handlers and window input paths hold/mutate the draft externally.
- Product dungeon helpers extend default draft behavior outside this file.

## Called By / Entry Points

- `makeDefaultWorldSetupDraft(...)`, `validateWorldSetupDraft(...)`, `routeWorldSetupAction(...)`, `resolveWorldSetupSeed(...)`, and enum name helpers.
- Grep proof: `rg -n "WorldSetupDraft|WorldSetupValidation|WorldSetupCreateRequest|worldSetupFieldName|resolveWorldSetupSeed|makeDefaultWorldSetupDraft|validateWorldSetupDraft|routeWorldSetupAction" src/app tests/unit`.

## Invariants

- Seed resolution is deterministic from seed text.
- World name and seed text are trimmed for validation/create request.
- ASCII room create requests require non-empty text, id, and source name when enabled.
- `Back` discards the draft; create routes return request packets and do not launch the world.
- Only supported difficulty/scenario values validate.

## Tests / Proof Commands

- `rg -n "world_setup_model_tests|product_world_creation_tests|product_new_world_menu_action_tests|product_dungeon_draft_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "routeWorldSetupAction|validateWorldSetupDraft|makeDefaultWorldSetupDraft|resolveWorldSetupSeed" tests/unit/world_setup_model_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/world/BuiltinDungeon.*` unless product dungeon draft behavior changes.
- `src/app/iggy3d/world/ProductNewWorldLaunch.*` unless create request execution changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless draft mutation/input changes.

## Update When

- Draft fields, validation rules, seed resolution, route result fields, create request fields, or enum names change.

## Do Not Update When

- Only product-specific world launch execution changes after a valid create request.
