# File Spec

File: `src/app/iggy3d/ProductAppWindowState.hpp`

Verified at: `a48d779a`

## Owns

- Root product app-window state aggregate.
- Window lifecycle booleans: requested, SDL availability, created, and drawable.
- Embedding of domain stores for frontend shell, input devices, debug HUD, creative authoring, gameplay, room, save session, viewport, automation control, and present path.

## Does Not Own

- Runtime simulation state, save serialization, renderer backend behavior, menu routing, creative document state, gameplay controller logic, or receipt field emission.
- Detailed ownership of nested store fields; each nested store remains its own contract surface.

## Reads

- No live inputs; this header defines the aggregate packet.
- Callers read fields for app flow, presentation, automation, and receipt proof.

## Writes / Mutates

- Callers mutate the packet from app startup, menu actions, window loop, input frame, gameplay controllers, creative flows, save flows, renderer lifecycle, automation, and tests.
- This file itself has no behavior.

## Calls Out To / Wires Out To

- Includes the nested store types that partition the former app-window god state.
- Exposes a single packet passed through product app/window/gameplay/menu APIs.

## Called By / Entry Points

- Constructed directly in app tests and product app paths as `ProductAppWindowState`.
- Grep proof: `rg -n "ProductAppWindowState|ProductAppWindowState window|ProductAppWindowState& window|ProductAppWindowState\\*" src/app/iggy3d tests/unit`.

## Invariants

- Keep this file as an aggregate packet only.
- New state should usually live in the narrowest domain store, not as a new root field.
- Root booleans must stay limited to window lifecycle facts shared across app surfaces.
- Adding or moving members must keep `product_god_struct_ownership_coverage_tests` aligned.

## Tests / Proof Commands

- `rg -n "product_god_struct_ownership_coverage_tests|ProductAppWindowState" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "FrontendWindowShell|InputDeviceStore|DebugHudStore|CreativeAuthoringStore|GameplayStore|ProductRoomStore|SaveSessionStore|ProductViewportState|ProductAutomationControlState|PresentPathStore" src/app/iggy3d/ProductAppWindowState.hpp tests/unit/product_god_struct_ownership_coverage_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/FrontendWindowShell.hpp` unless frontend/window shell packet embedding changes.
- `src/app/iggy3d/gameplay/GameplayStore.hpp` unless gameplay app-state embedding changes.
- `src/app/iggy3d/ReceiptBuilder.*` unless receipt reads change.

## Update When

- Root members, nested-store membership, lifecycle boolean meanings, or aggregate ownership rules change.

## Do Not Update When

- Only behavior inside a nested store changes without changing this aggregate packet.
