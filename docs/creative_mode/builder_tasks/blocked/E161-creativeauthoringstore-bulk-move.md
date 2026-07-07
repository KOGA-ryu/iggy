# E161 — CreativeAuthoringStore bulk-move (god-struct decomposition #4) — PARENT

**STATUS: STAGED in `blocked/` — parent card; MUST be sliced (largest store in the tree).**
Children released so far: `done/E168-creativeauthoringstore-g1-wireframe.md`;
`done/E169-creativeauthoringstore-g2-viewport-pick.md`;
`done/E170-creativeauthoringstore-g3-room-editor.md`;
`ready/E171-creativeauthoringstore-g4-world-ascii.md`.
Recon-grounded + spot-verified (workflow `wdnplylk0`). **Commit convention:** `claude: planned. codex: …`.

> **Tree mid-flight + EXECUTION SERIALIZES** on `ProductAppWindowState.hpp` — do after prior moves land;
> re-anchor field lines at slice time. **Anchor by field NAME.**

---

## Goal & ruling

Move the **86 creative + room-authoring fields** (verified — map said "~90") into **ONE flat**
`CreativeAuthoringStore creativeAuthoring;`. Plain owned/telemetry state, **no freshness token**
(`creativeBakedRoomStale*` already carries its own `{documentId,revision}` diagnostic token — plain, not a
scattered cache). **~1679 repoints across ~41 files — the biggest move; it MUST be sliced by sub-domain.**

## Member design — ONE flat store, no sub-grouping

`window.creativeAuthoring.<field>` (single extra hop). Sub-grouping (`.wireframe.<field>`) would add a second
path segment to 1679 sites for zero gate benefit — the receipt keys already encode the sub-domain in their
string literals.

**Critical ordering constraint:** 8 struct types are defined **inline in `ProductAppWindowState.hpp` above the
god struct** and must move so the store header is self-contained — `ProductCreativeUiCommandDiagnostics` + its
6 nested diag structs (`…Mutation/Create/Delete/Undo/RoomShellDiagnostics`) + `ProductCreativeBakedRoomRefreshDiagnostics`.
Move these into a shared header the store includes (e.g. `ProductCreativeUiCommandDiagnostics.hpp`). The store
header also needs the includes the god header currently carries for the other member types (WorldSetup/Creation,
AsciiRoom{Draft,Preview,Activation}, RoomEditing/RoomEditor*, CreativeDocumentRevision, CreativeUndo,
CreativeUi{Projection,Input,Last}). New header: `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`.

## Map corrections (verified)

- **86 fields, not ~90.**
- The map's **"FOUR appenders" is 3 receipt-key appenders + 1 recorder**: `WorldAuthoringFields.cpp`,
  `CreativeUiFields.cpp`, `CreativePickWireframeFields.cpp` emit keys; **`CreativeReceiptRecording.cpp` is NOT
  an appender** — it *writes* `window.creativeBakedRoomStale*` / mirrors `creativeUi*`, emits no keys. **All
  read the same `window` — one store member serves all; do NOT fork the store.**

## LAW

1. **Receipt golden byte-identical** across all 3 appenders — repointing the read/write path changes no
   emitted key/value/order. **A diff = a BUG — STOP.**
2. **Compiler-guided, NEVER `replace_all`** — a grep-repoint silently misses ~25 mis-named-var accesses and
   would over-match the foreign collisions below.

## HAZARDS (verified foreign collisions — do NOT repoint)

- `ProjectionFrame` (`gameplay/ProjectionRefresh.hpp:79-81`) has its own `roomEditorHud` /`roomEditorOverlay`
  (**note: a DIFFERENT type** than the window's), accessed `frame.roomEditorHud` / `projection.roomEditorOverlay`.
- `PrimitiveDrawList.hpp` / `RenderBridge.hpp` have `roomEditorPlacementPreviewVisible/Count` accessed as
  `list.`/`bridge.`/`drawList.`. Under the nested-member method the compiler only errors on the god-struct's
  `window.<field>`, so these are auto-safe — never bare-token sed.

## TSV & gates (per slice)

Coverage gate parses top-level members bidirectionally. **Each slice deletes only ITS OWN flat rows; the FIRST
slice to land also adds the single `creativeAuthoring␉CreativeAuthoringStore` row** (later slices keep it).
Per slice: build green · ctest 260/260 · golden byte-identical. Final slice: update decomposition map #4 → DONE + PRIORITY.md.

## Suggested slices (smallest/most-isolated first, to de-risk the shared header)

1. **WIREFRAME** — the ~27 `creativeWireframe*` fields (most isolated: `CreativePickWireframeFields.cpp` +
   `creative/bridge/WireframeFrame`). First slice sets up the store header + adds the TSV row.
2. **PICK** — the ~25 `creativeViewportPick*` fields.
3. **ROOM-EDITOR** — `roomEditing*`, `roomEditor*`, cursor/overlay/preview/placement/hud.
4. **ASCII + WORLD-SETUP** — `asciiRoom{Draft,Preview,Activation}`, `worldSetup`, `worldCreation`.
5. **REVISION/UNDO/BAKED-STALE + UI** — `creativeDocumentRevision`, `creativeUndo`, `creativeBakedRoom*`,
   `creativeNavigateActive`, `creativeUi{Projection,Input,Last,Command}`, `creativeBakedRoomAutoRefresh`.
(Slice 1 must land the header + the 8 relocated diag structs + the `creativeAuthoring` member; subsequent
slices just move their fields into it and delete their flat rows.)
