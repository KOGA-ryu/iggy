# Creative Tooling v1 — the contract

> Planner-authored 2026-07-05 on takeover of the creative lane. Subordinate to
> `world_foundation_plan_v0_1.md` (content v0.4, D1–D8) — this doc governs the
> TOOLING layer built on the now-complete foundation (W1–W8 all landed;
> trunk `ba52da8d`, suite 234/234 green, verified by recon + run 2026-07-05).
> Dex deployment packets: `creative_tooling_dex_briefs.md`. Numbering: laws
> `TL-n`, decisions `TD-n`, slices `TV1-X`.

## 0. What is true today (4-reader recon @ ba52da8d — do not rediscover)

Already satisfied from the user's near-term list:
- **Product movement fully suppressed** in creative document mode: keyboard
  never polled, controller short-circuited (`InputFrame.cpp:865,1162`), mouse
  capture released (`MouseCapturePolicy.cpp:34`).
- **Active Tool row is display-only** — clicking it does NOT cycle
  (test-pinned: `product_creative_ui_command_frame_tests.cpp:186`). Keyboard
  cycling still exists (removal target, TL-2).
- Legacy HUDs/minimap suppressed under the overlay (`FramePresenter.cpp:1014-1052`);
  still drawing: base 3D scene (by design), DevTools overlay, pause journal,
  "Gameplay" window title.
- Save round-trips everything v1 needs per object (id/kind/name/transform/
  bounds/layerId/visible/locked/tags/parent) + doc grid/snap/worldBounds
  (`SaveEnvelope.hpp:179-215`). Version written, not checked (TV1-I).
- **Lock is already enforced in the mutation pipeline** (`MutationApply.cpp:349`:
  LockedObject unless SetLocked) — but has bypasses (legacy
  `createRoom`/`renameObject`, `removeDocumentObject` deletes locked objects),
  no facade toggle, no UI surface, and pick/selection ignore it.

Load-bearing facts:
- Tool enum = `{Select, Inspect, Measure}` (`Core.hpp:11`). No Move/Resize/Navigate.
- Pointer flow is CLICK-ONLY: window layer synthesizes `PointerPress` only
  (`CreativeInputFrame.cpp:193-199`). `PointerMove`/`PointerRelease` exist in
  the tool core but nothing emits them — **Measure can Begin but never End.**
- Pick = screen-space grid scan `{64,64,8}` HighestZFirst (not a camera ray).
- Command table = exactly 2 semantic ids (`CreativeUiCommandFrame.cpp:17-23`):
  create_room → `createDocumentObject(Room)`; selected_target →
  `toggleSelectedObjectVisibility`. No payload channel (kind is hardcoded).
- **Room rejects Move/Rotate/Scale/SetTransform** (`Mutation.cpp:394`,
  descriptor `hasTransform=false`); Rooms reposition via SetBounds/Stretch/
  Resize only. Real verbs ≈ 23 of 70; ~44 return truthful NoChange
  "no stored object field yet" (do NOT surface them in UI — TL-7).
- Move semantics (post-D8): absolute position, translates bounds with it,
  exact-equality no-change guard (`MutationApply.cpp:402-417`). Streaming is
  safe but each changed frame = +1 revision.
- **Document snap (W2, 3D, per-document, default Grid 1m XYZ) is consumed by
  NOTHING interactive** (`DocumentSnap.hpp:43`; only save/restore touch it).
  A second, older 2D snap (`Snap.hpp`) feeds only the ghost preview.
- W4 dirty accumulator drained ONLY by save; UI model + wireframes rebuild
  every frame unconditionally (`Loop.cpp:186-257`).
- Wireframes are Vulkan-only; input already gates on renderable overlay
  (`CreativeUiWindowFrame.cpp:30-80` — parity satisfied by gating).
- No selection highlight, no grid visuals, no ghost geometry — text rows only.
- Camera in creative document mode is a fixed first-person anchor: **no camera
  movement exists at all.** Fly camera lives in legacy map_maker only.
- Facade never resets on world exit (leak gated off by predicates, not fixed).
- Automation launches creative worlds headless (`frontend_select
  creative_new_world`) but cannot click at runtime (clickOverride socket is
  test-only; `Loop.cpp:236` passes `{}`).
- Panel placement today already approximates the target: Tools top-left,
  Status+Snap pinned bottom-left, Selection/Inspection/etc. right column
  (`CreativeUiDrawList.cpp:259-303`).

## 1. Laws (TL-n)

1. **Two categories, never mixed.** Pointer tools (Select, Move, Measure,
   Navigate) own pointer gestures and are mutually exclusive modes. Commands
   (create/toggle/save) are one-shot rows with receipts. A row click may set a
   tool or fire a command — never both, never implicitly.
2. **No tool cycling, anywhere.** Explicit per-tool buttons + direct keys
   (1/2/3/4) with visible active state. The keyboard cycle verbs
   (`EditorNextTool`/`EditorPreviousTool` rows, `nextProductCreativeTool`/
   `previousProductCreativeTool`) are REMOVED, not just unmapped. The
   display-only active_tool row is replaced by the segmented palette.
3. **Gesture lifecycle law.** The window layer emits the full pointer
   lifecycle: Press → Move* → Release (+ Cancel). Every drag-capable tool
   defines behavior for all four. No tool may mutate the document before
   Release (preview during drag, commit on release — TD-6).
4. **Snap at the tool boundary, document snap only.** Pointer tools snap
   world coordinates via `snapCreativeDocumentPoint/Bounds` (the W2 3D
   contract) BEFORE building mutation payloads (this is foundation D4). The
   legacy 2D `CreativeSnapSettings` is quarantined to ghost screen-space
   preview and scheduled for retirement (deferred ledger).
5. **Viewport purity.** Center hosts the 3D stage + wireframes + (future)
   handles only. Panels live in reserved edge regions. DevTools overlay and
   window title become creative-aware (TV1-H). Pause journal stays topmost by
   design.
6. **Every command and gesture ends in a receipt** (foundation LAW-16); the
   status bar renders the last receipt line — the UI explains itself.
7. **UI exposes only real verbs.** No row may fire a mutation that returns
   "no stored object field yet". Gate palette/inspector rows on the real-verb
   set (descriptor capability columns are the long-term gate — optimization
   doc 01).
8. **Vulkan-only, declared.** Creative mode requires the Vulkan renderer for
   v1; the input-side renderable-overlay gate is the enforcement (already
   landed, W1w-C). Box (Vulkan-OFF) coverage = headless unit/automation tests.

## 2. Decisions (TD-n)

1. **Inspect is retired as a tool.** Folded into Select: selecting an object
   populates the right inspector. `Tool` enum becomes
   `{Select, Move, Measure, Navigate}`. `inspectionState_`/InspectedTarget
   row/`InspectObjectCandidate` intent are removed with it (the accidental
   Inspect/Measure mixing dies at the root). Inspection *data* lives on in the
   inspector panel, driven by selection.
2. **Move applies to every placeable kind, by corner anchor.** For kinds with
   `hasTransform=false && hasBounds=true` (Room!), Move translates bounds so
   `bounds.min == position` (corner-anchored, matching D3). `Move` is added to
   Room's allowed verbs. For transform kinds, existing D8 semantics stand.
   One verb, one meaning: "place the object's anchor here."
3. **Lock semantics:** locked = immutable, not invisible. Locked objects ARE
   pickable/selectable (inspector shows state), all mutations reject
   (pipeline already does this), and **removal refuses locked objects**
   (closing today's bypass). Legacy no-receipt `createRoom`/`renameObject`
   document paths are deleted (they bypass the lock gate and receipts).
4. **Toggle Visible / Toggle Locked are inspector rows**, not the selection
   row. The selected_target row becomes display-only; explicit
   `visible: true|false` and `locked: true|false` rows in the inspector are
   the toggles. (Kills the "clicking the selection row secretly toggles
   visibility" surprise.)
5. **Creation is command + defaults; placement is Move's job (v1).** Create
   Room / Create Crate spawn at descriptor defaults (Room corner at origin;
   successive creates offset by one snap step on X so objects don't stack
   invisibly — small mutation to defaults policy, receipted). Click-to-place
   ghost preview is v1.5 (machinery exists; not the near-term minimum).
6. **Drag model: preview-then-commit.** During Move drag: ghost/wireframe
   preview only. On Release: ONE Move mutation (snapped). On Cancel/Escape:
   no mutation, receipt `move_cancelled`. Keeps revision = user intent count,
   receipts meaningful, undo-ready.
7. **Move plane (CORRECTED in TV1-G):** a drag must FOLLOW THE CURSOR, so its
   axes must match the viewport projection. The as-built creative projection is
   a **front view — screen = world XY, depth = world Z** (`worldToGridCoord`
   maps `position.y → gridY`; the pick maps screen-vertical → `coord.y`).
   Therefore drag maps screen-horizontal → world X, screen-vertical → world Y,
   and **holds depth Z** at the object's current anchor Z. (The original "XZ
   plane, Y unchanged" wording assumed a top-down camera that does not exist —
   it was wrong and would have moved the object in depth instead of tracking
   the cursor.) Depth (Z) moves = v1.5 modifier / inspector numeric. **Law: the
   drag reuses the pick's pointer→grid conversion so pick and drag can never
   disagree; if the camera/projection changes (e.g. TV1-H), the drag follows
   automatically.** Known future polish: screen-Y currently grows *downward*
   with pixel-Y (inherited from the pick); making "cursor up = world Y up" must
   be done in the projection layer (`ViewportPick.cpp`), never patched into the
   drag alone.
8. **Navigate is a tool** (not a mode outside the palette): while Navigate is
   active, WASD + mouse-look drive a creative fly camera (reusing
   `CreativeFly` from map_maker), mouse capture re-engages, and pointer
   gestures do NOT hit the document. Switching back to Select releases
   capture. Keyboard polling stays dead for all other tools.
9. **Resize is v1.5, display-first.** The inspector SHOWS bounds (min/max/
   size) read-only in v1. Numeric editing + handles come after Move is
   stable (handles need the drag lifecycle + pick-on-handle machinery).
   SetLength/SetWidth both writing size.x must be fixed before any resize UI.
10. **Measure stays, explicit-only, and gets its End.** Measure tool via its
    palette button only; the drag lifecycle (TL-3) finally delivers
    PointerRelease → EndMeasurement. Measurement panel is transient (visible
    only while Measure is active or a measurement exists — current behavior
    already close).
11. **Delete/Duplicate deferred.** Delete needs a confirm pattern + TD-3
    lock-refusal landed; Duplicate needs stable Move. Both post-v1; receipted
    remove already exists as the substrate.
12. **Facade resets on world exit.** SaveAndExit/ReturnToTitle uninstall the
    document (install-empty + receipt), killing the app-lifetime leak.
13. **QA harness = automation click injection.** The `clickOverride` socket
    (`InputFrame.hpp:55`) is promoted from test-only: automation control gains
    an in-loop scripted click/command channel so the acceptance script can run
    headless end to end on the box.
14. **Save section version gets checked on restore** (wrong-version →
    receipted reject, no silent forward-compat lies).

## 3. v1 surface

**Tools** (left palette, top): Select (default) · Move · Measure · Navigate.
**Commands**: Create Room · Create Crate (left palette, bottom) · Toggle
Visible · Toggle Locked (inspector rows) · Save / Save & Exit (pause, exists).
**Panels**: left = tools + create; right = inspector (kind, id, name, visible,
locked, bounds min/max/size, position, layer); bottom = status strip (mode ·
active tool · snap state · object count · last receipt); measurement panel
transient; center = stage + wireframes only.

New semantic ids (the complete v1 addition set):
`creative.row.tools.tool_select|tool_move|tool_measure|tool_navigate`
`creative.row.create.create_room|create_crate`
`creative.row.selection.inspector_visible|inspector_locked` (commands) and
`creative.row.selection.inspector_kind|id|name|bounds|position|layer` (display)
`creative.row.status.summary` (display).
(As-built: the inspector IS the now-always-visible Selection panel, so wire ids
carry the `selection.inspector_*` prefix rather than a bare `inspector.*` — the
panel-name-derived id rule. A future cleanup could rename the panel kind
Selection→Inspector to get bare `inspector.*` ids; deferred, not load-bearing.)
Command table generalizes from the 2-entry array to rows carrying
`{semanticId, commandKind, payload}` (palette kind = payload — kills the
hardcoded Room).

**Input grammar:** LMB press on viewport = pick (all tools except Navigate);
Select: press=select/deselect · Move: press-on-selected=begin drag,
move=preview, release=commit, Esc=cancel · Measure: press=begin, move=update,
release=end · Navigate: pointer captured, WASD+look fly · keys 1/2/3/4 = tool
select · Esc = cancel gesture, then deselect.

**Capability matrix v1** (rows the tooling must fully serve; everything else
stays descriptor-only until the per-object detailing thread rules on it):

| Kind | shape | create | select | move | resize | vis/lock | save | wireframe |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Room | BoxVolume | v1 | v1 | v1 (TD-2) | v1.5 | v1 | ✓ | ✓ Structural |
| Crate | MeshProxy | v1 | v1 | v1 | v1.5 | v1 | ✓ | ✓ |
| Note | Point | v1.5 | v1 | v1 | — | v1 | ✓ | point marker |

## 4. Slice plan (dex assignments in the briefs doc)

- **TV1-A [mutation dex]** — Lock completion: `Facade::toggleSelectedObjectLocked`
  (generalize `CreativeFacadeMutationReceipt` to before/after variant),
  removal refuses locked (TD-3), delete legacy `createRoom`/`renameObject`
  bypass paths. Tests. **DONE 2026-07-05** (built + reviewed APPROVE, 234/234;
  Commands.hpp/.cpp deleted whole; lock-refusal message lives in
  `objectReceipt.message` — the TV1-E status line must read it, not the
  generic top-level message). Pick/selection lock surfacing moved to TV1-E.
- **TV1-B [mutation dex]** — Move-for-all: TD-2 corner-anchor Move for
  no-transform kinds + Room allowed-verbs row + golden pins (Room move,
  locked-Room move rejected). Snap helper surface: one function the tool
  layer calls (`snapCreativeDocumentPoint`), exact-value pins.
  **DONE 2026-07-05** (reviewed APPROVE; `document.documentSnapSettings()` is
  the tool-facing seam; note: `dirtyFlagsForMutation(Room, Move)` includes
  Transform conservatively — TV1-G should know).
- **TV1-C [input dex]** — Tool enum reshape: retire Inspect (TD-1), add
  Move/Navigate; delete cycling (TL-2); keys 1–4; dispatch table + facade
  `setActiveTool` receipts; Ui rows/tests updated.
  **DONE 2026-07-05** (reviewed APPROVE; suite is now 233 — creative_inspect_tests
  deleted with its subject; tool keys are a typed poll reading ONLY scancodes
  1-4, gated to creative document mode; Navigate pointer fully inert with
  receipt reason; the old `creative.*.inspection.*` id namespace is GONE —
  TV1-E uses the new `creative.row.inspector.*` ids; follow-ups logged:
  controller has no creative tool bindings, tie-break helper/batch alignment,
  brief anchor drift in CreativeInputFrame/Tools/Facade line numbers).
- **TV1-D [ui dex]** — Palette + command generalization: per-tool rows with
  Active flag, create rows with payload column, command table → payload rows.
  **DONE 2026-07-05** (commit d6a44e49, reviewed APPROVE after two REJECT
  rounds). Deviations/notes: (a) selected_target keeps the visibility toggle
  until TV1-E moves it to the inspector — did NOT become display-only yet
  (sanctioned, avoids a dark loop between slices). (b) `creative/Placement.*`
  added: successive creates offset +X by count×snap-step (bounds-only for
  no-transform kinds, position+bounds for transform kinds, D8-coherent);
  receipt notes the offset. (c) **Product fix BG-1029** (InputFrame.cpp): the
  legacy opening-menu hit band (`openingMenuActionAt`, raw-coordinate, not
  screen-gated) stole creative-overlay clicks once the Create row reflowed into
  the phantom starter band (y≈174). Guard: skip that band during raw creative
  gameplay, keep it whenever the frontend owns the mouse (`frontendMouseOwnsInput`)
  so pause + its Settings/Load/Delete children stay clickable over a creative
  world. Pinned by `pauseMouseClickResumesInActiveCreativeWorld`. This is the
  same F6/F7 phantom-band class from the foundation review, re-exposed by layout
  reflow — **any future creative-row layout change must re-check the [136,188+52k]
  band.** (d) LESSON: incremental `cmake --build <touched-targets>` hides
  compile failures in SIBLING test targets that reference retired symbols — make
  cache stale-green. Gate discipline is now: `cmake --build build` (ALL targets)
  then full `ctest`, and reviewers force a clean rebuild.
- **TV1-E [ui dex]** — Inspector panel: display rows + visible/locked toggle
  rows (TD-4), no-selection state ("nothing selected — click an object"),
  gated on real verbs (TL-7). Needs TV1-A, TV1-D.
  **DONE 2026-07-05** (commit 8c521f7f, reviewed APPROVE_WITH_NITS, gate
  confirmed via forced clean rebuild 234/234). The Selection panel became the
  always-visible inspector (default UI now 9 rows / 7 panels — **anchor drift:
  any slice pinning the old 8-row default must update**). TD-4 fully closed:
  selected_target is display-only, visibility toggle lives on
  `inspector_visible`, new `inspector_locked` → ToggleSelectedObjectLocked
  (receipt locked_before/after). Notes: (a) bounds row shows min/max only — the
  size triple is carried in row fields but not rendered (620px column truncates
  min+max+size); if "show size" is load-bearing, widen the column or split the
  row (v1.5 numeric-edit territory anyway). (b) inspector display rows still
  emit clickable hit regions that consume-and-noop (prevents pick-through —
  intended). (c) `CreativeUiObjectSummary.name`/row `name` are string_views into
  the live document — safe under per-frame rebuild; copy if the model ever
  detaches from the document. (d) cosmetic: some test helper names still say
  `selectedTarget*` but route `inspector_visible` — follow-up rename.
- **TV1-F [input dex]** — Drag lifecycle: window layer emits
  Move/Release/Cancel packets with held-button tracking; Measure End works;
  Esc = cancel. Needs TV1-C.
  **DONE 2026-07-05** (commit 8aff9a35, reviewed APPROVE_WITH_NITS, gate
  confirmed via forced clean rebuild 234/234). Flagship delivered: Measure ends
  through the window frame entry. Held state lives in
  `ProductWindowInputFrameState.creativePointerLifecycle`, reset on tool switch
  and on any mode-exit frame (no phantom release). Coords reuse the pick/press
  window-pixel space. Lifecycle is inert under `clickOverride` (automation) —
  headless drag/measure-end wait on TV1-K's injected-gesture channel. **HARD
  REQUIREMENTS PUSHED TO TV1-G** (from TV1-F review nits): (i) add a
  lifecycle test THROUGH `processProductWindowInputFrame` (raw-mouse edge
  detection → resolver → dispatch is currently inspection-verified only — this
  is the window-plumbing seam the create_room bug taught us to test); (ii) the
  Move commit-on-release must tolerate a Release with NO preceding Press (a drag
  interrupted by pause then resumed with the button still held); (iii) fill the
  `pointerLifecycleTarget` seam — Move/Release currently carry an invalid target,
  and Move's snapped commit needs a picked target on release.
- **TV1-G [input+mutation dex]** — Move tool: drag preview via ghost line/box
  in wireframe layer, snapped single commit on release (TD-6/7), lock refusal
  surfaces in status. Needs TV1-B, TV1-F.
  **DONE 2026-07-05** (commit a5f0120c: mechanism APPROVE_WITH_NITS + a folded-in
  axis-correction slice APPROVE; gate confirmed forced clean rebuild 235/235 —
  new `product_creative_move_drag_frame_tests` target). **Drag-to-move works.**
  Press begins the drag, held-move previews the snapped destination via the
  EXISTING ghost (no mid-drag mutation), release commits ONE corner-anchor Move,
  Esc cancels, locked refuses truthfully, same-anchor is NoChange. All three
  TV1-F entry requirements met (window-plumbing test, orphan-release no-op,
  pointerLifecycleTarget filled). Axis corrected to cursor-following per the
  revised TD-7 (screen=XY, hold Z).
  **TD-7 SUPERSEDED (2026-07-05, commit defc4e44):** the hardcoded screen=XY hold
  assumed the product's flat front projection and floated objects under a 3D fly
  camera. Move is now VIEW-AGNOSTIC — `CreativeToolPointerPacket.moveHeldAxis`
  (default Z, so the product path is byte-identical) lets the CALLER pick the
  held axis from its camera; a ground-plane editor holds Y and slides XZ. The
  held axis is re-applied AFTER document snap so it never rounds up to a grid
  line (no float). `iggy3d_creative` uses Y (ground-plane).
  Deferrals: the dedicated wireframe preview
  BOX (vs the ghost point) → render slice TV1-J; depth-axis / numeric move →
  v1.5. Note: TV1-G added a minimal scripted `ProductWindowInputClickOverride.
  pointerLifecycle` socket to make the drag headless-testable — **TV1-K should
  fold its richer injected-gesture channel into/alongside this, not add a
  parallel one.** Cosmetic follow-up: the facade `committed` receipt field means
  "commit attempted" not "landed" — consider a rename.
- **TV1-H [lifecycle dex]** — Navigate camera (TD-8): CreativeFly reuse,
  capture policy branch, keyboard routed to fly only while Navigate active.
  Viewport purity polish: DevTools gate + creative window title. Needs TV1-C.
  **DONE 2026-07-05** (commit a747524f, reviewed APPROVE, gate confirmed forced
  clean rebuild 236/236 — new product_creative_navigate_fly_tests). Fly gated
  strictly to activeTool==Navigate; relative mouse capture re-engaged only then;
  narrow creative-only WASD poll (no gameplay keyboard resurrected); capture
  coherence verified (pause/menu/mode-exit all release — no stuck cursor).
  window.creativeNavigateActive mirror feeds MouseCapturePolicy + the camera
  override (least-coupling seam, contexts carry only window). DevTools overlay
  gated under the creative overlay; title says Creative. Notes: (a) fly is
  yaw-only (matches map_maker; pitch-driven translation is a future kernel
  change); (b) buildMapMakerGridForFrame resets creativeFlyActive every frame
  for a creative doc — harmless (that flag only feeds HUD/receipt; the real fly
  uses the valid-anchor gate) — revisit only if a Navigate fly HUD is wanted;
  (c) the fly WASD poll lives in app/input/KeyboardInput.cpp (input-dex surface,
  paralleling the TV1-C tool-key poll) — cross-dex touch, acceptable.
- **TV1-I [lifecycle dex]** — Hygiene: facade uninstall on exit (TD-12), save
  version check (TD-14), status strip consolidation (Status+Snap → one bottom
  strip with last-receipt line, TL-6).
- **TV1-J [render dex]** — Selection feedback: selected bit through
  wireframe chain (list→segments→debug lines→Vulkan), selected style/color +
  thickness; hover deferred. Parallel after TV1-C.
- **TV1-K [qa dex]** — Automation click channel (TD-13) + the acceptance
  script below as a headless test. Needs TV1-D..G.

Dependency spine: A,B,C first (parallel) → D → E; F → G; H, I, J parallel;
K last. Every slice: full ctest green, receipts, one lane per commit.

## 5. Acceptance (the v1 gate — TV1-K automates this)

Headless + manual: new creative world → palette shows 4 tools (Select active)
→ Create Room, Create Crate (offset, both wireframed) → click room (selected,
inspector shows kind/id/bounds, outline highlights) → 2 → drag room (preview,
snapped commit on release, receipt in status) → toggle locked → drag refused
(receipt says locked) → toggle visible → wireframe gone, pick misses it →
Measure begin/end works, Esc cancels → 4 → fly camera, 1 → capture released →
pause Save → reopen world → objects, transforms, visible/locked all restored →
no legacy HUD/minimap/DevTools drawn, title says Creative.

## 6. Deferred ledger

Resize handles + numeric editing (v1.5, after SetLength/SetWidth axis fix) ·
click-to-place ghost creation (v1.5) · Note kind + text editing · Delete/
Duplicate (TD-11) · vertical move modifier · 2D snap retirement · hover
states · per-frame rebuild → dirty-gated refresh (optimization doc 05) ·
grid floor visuals + deliberate stage contract · SDL parity · undo/redo ·
full create palette from descriptor table (needs optimization doc 01 columns).
