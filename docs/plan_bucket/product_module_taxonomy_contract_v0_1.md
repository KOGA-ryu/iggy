# Product Module Taxonomy Contract v0.1

## Objective

Define folder and naming architecture before moving code.

Consolidation without a destination map is refactoring drift. Before any source moves,
this contract fixes where each product-spine domain belongs, how file names are
chosen, and how migration tables prove each slice.

## Principle

Ownership should come from folders first, file names second.

Current noisy pattern:

```text
src/app/iggy3d/ProductRoomEditorCursor.cpp
src/app/iggy3d/ProductRoomEditorOverlay.cpp
src/app/iggy3d/ProductRoomEditorActionController.cpp
```

Preferred consolidated pattern:

```text
src/app/iggy3d/product/Room.cpp
src/app/iggy3d/product/Room.hpp
```

The folder marks product-domain ownership. File names only encode the stable
function under that folder.

## Target Top-Level Ownership

| Path | Owns | Must not own |
| --- | --- | --- |
| `src/app/iggy3d/Shell.*` | app entry shell, option result handling, top-level lifecycle | product domain policy |
| `src/app/iggy3d/product/` | product behavior and orchestration owned by durable domains | runtime simulation truth, Vulkan backend |
| `src/app/frontend/` | reusable pure frontend/menu models | product receipts, product runtime orchestration |
| `src/runtime/` | engine/runtime state and systems | product receipts, product UI |
| `src/content/` | package/scenario/assets/authoring truth | product app flow |
| `src/projection/` | derived scene/debug projection | product lifecycle, renderer allocation |
| `src/render/` | renderer/backend/platform code | product flow policy |
| `tests/unit/` | focused unit proof | broad scenario orchestration |
| `tests/smoke/` | product/path proof | implementation detail internals as assertions |

## Target Product Folder

Preferred target module folder:

```text
src/app/iggy3d/product/
```

Preferred durable modules:

```text
src/app/iggy3d/product/Automation.cpp / Automation.hpp
src/app/iggy3d/product/Frontend.cpp / Frontend.hpp
src/app/iggy3d/product/Gameplay.cpp / Gameplay.hpp
src/app/iggy3d/product/Projection.cpp / Projection.hpp
src/app/iggy3d/product/Receipt.cpp / Receipt.hpp
src/app/iggy3d/product/Room.cpp / Room.hpp
src/app/iggy3d/product/RoomPipeline.cpp / RoomPipeline.hpp
src/app/iggy3d/product/Save.cpp / Save.hpp
src/app/iggy3d/product/World.cpp / World.hpp
```

Shell entrypoint:

```text
src/app/iggy3d/Shell.cpp
src/app/iggy3d/Shell.hpp
```

`src/app/iggy3d/AppShell.*` may remain temporarily as compatibility wrappers,
but the target naming remains `Shell.*` once migration is safe.

## Module Responsibilities

| Module | Owns | Existing files likely absorbed |
| --- | --- | --- |
| `product/Automation.*` | automation registry, parser, dispatch, command result recording | `ProductAutomationCommandRegistry.*`, automation helpers from `AppShell.cpp` |
| `product/Frontend.*` | starter/pause/settings/dev-tools/save-selector/world-setup routing | `ProductFrontendRouter.*`, `ProductMenuTransitions.*`, `FrontendActionExecutor.*`, selected pure frontend adapters |
| `product/Save.*` | continue/load/save/delete/recover, active/deleted row selection, save-catalog bridge | save-flow parts of `ProductAppOperations.*`; calls `SaveBridge.*` and `ProductSaveCatalog.*` |
| `product/World.*` | world setup draft behavior, title/seed/dungeon selection, new world launch orchestration | world-flow parts of `ProductAppOperations.*`, `DefaultWorldTemplate.*`, `ProductWorldCreation.*` |
| `product/Room.*` | editable room authoring, cursor, tool, overlay, edit application | `ProductRoomEditingState.*`, `ProductRoomAuthoringController.*`, `ProductRoomEditorCursor.*`, `ProductRoomEditorActionController.*`, `ProductRoomEditorOverlay.*`, `EditableRoomToAuthoredRoom.*` |
| `product/RoomPipeline.*` | ASCII/generated-map to authored/editable/room-asset/active-room/collision pipeline | `AsciiRoom*`, `ProductAsciiRoom*`, `ProductActiveRoom*` |
| `product/Projection.*` | product projection assembly, primitive draw list, viewport frame, render-bridge proof, product HUD proof | `ProductPrimitiveDrawList.*`, `ProductRenderBridge.*`, `ProductViewportFraming.*`, product debug HUD files |
| `product/Receipt.*` | descriptor-driven receipt projection and field naming | `ReceiptBuilder.*` after descriptor migration |
| `product/Gameplay.*` | gameplay command adapter, gameplay feedback, tape proof | `ProductGameplay*`, `ProductCameraController.*`, `ProductScriptedGameplayDriver.*` |

Keep separate lower-level owners:

- `src/app/iggy3d/SaveBridge.*`
- `src/app/iggy3d/ProductSaveCatalog.*`
- `src/runtime/save/*`
- `src/runtime/session/*`
- `src/content/*`
- `src/render/*`

`SaveBridge` and `ProductSaveCatalog` may be folded under `product/Save.*` only if
that does not collapse boundary clarity. For v0.1, keep these as explicit save
layer seams.

## Naming Mechanics

### Files

Use domain files with explicit ownership and stable nouns:

```text
Automation.cpp / Automation.hpp
Frontend.cpp / Frontend.hpp
Save.cpp / Save.hpp
World.cpp / World.hpp
Room.cpp / Room.hpp
RoomPipeline.cpp / RoomPipeline.hpp
Projection.cpp / Projection.hpp
Receipt.cpp / Receipt.hpp
Gameplay.cpp / Gameplay.hpp
Shell.cpp / Shell.hpp
```

Avoid temporary implementation words in file names:

- `Controller`
- `Manager`
- `FlowController`
- `Handler`
- `Helper`
- `Utils`
- `Common`

Those words are only allowed inside types when they describe a stable role, not
as an ownership dodge.

### Types

Use domain-scoped request/result and command/result types:

```cpp
struct ProductSaveRequest;
struct ProductSaveResult;
struct ProductRoomCommand;
struct ProductRoomResult;
struct ProductAutomationCommand;
```

Avoid repeated concept stacking:

```cpp
ProductSaveFlowControllerAction
ProductRoomEditorCursorControllerResult
```

### Functions

Use verb + domain object naming:

```cpp
applyProductSaveCommand(...)
applyProductRoomCommand(...)
buildProductProjection(...)
parseProductAutomationCommand(...)
```

Avoid vague verbs:

- `handleThing`
- `processStuff`
- `doAction`
- `runManager`

### Migration Table Format

Each consolidation slice must publish a migration table (stable row order):

`current_file | target_file | action | compatibility_path | deletion_gate`

- `current_file` is the pre-move path.
- `target_file` is the destination durable module path, or unchanged path if wrapper-only.
- `action` is `move`, `merge`, `split`, `shim`, or `defer`.
- `compatibility_path` names the shim/wrapper and behavior-preservation note.
- `deletion_gate` is the explicit condition for removing legacy ownership.

Example:

`src/app/iggy3d/ProductAutomationCommandRegistry.cpp | src/app/iggy3d/product/Automation.cpp | merge | Keep temporary wrapper header for one slice | remove after automation dispatch coverage and slice regression pass`

### Compatibility Wrappers

Wrappers are allowed only as a temporary compatibility path with explicit deletion
intent:

- wrappers must include a concrete `deletion_gate` in the migration table;
- wrappers must preserve existing include and behavior contracts;
- wrappers must not become permanent because they lower migration pressure.

### Status And Receipt Strings

Use lower snake for stable status strings:

```text
product_save_written
room_editor_command_applied
world_setup_dungeon_selected
```

Receipt key normalization to dotted domain keys is allowed only for new fields:

```text
save.write.status
room.editor.cursor.x
room.mesh.wall_draw_count
```

Existing lower-snake receipt keys must not be renamed in this consolidation phase.

### IDs

Use sortable zero-padded IDs where stable IDs are introduced:

```text
save_0001
room_floor_0001
room_wall_0001
```
