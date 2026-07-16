# Creative Desktop UI Field Survey — v1.0 (2026-07-16)

What shipped game editors converged on, cut against what Vaultwright Studio
already has. **Future desktop-UI batches cite coordinates from this doc**
(section.item), version-bump on corrections.

Method: 9-agent survey — 2 recon agents over this repo (work order + the
actual `EditorDesktop*` code), 7 field agents over Unreal 5, Unity, Godot 4,
Blender (operator paradigm), dedicated level editors (TrenchBroom, Hammer/
Source 2, LDtk, Tiled, Halo Forge, Far Cry Arcade), game-tools UX literature
(Insomniac postmortems, Lightbown, Storm's GDC talks, Tracy/ImGui practice),
and stealth-genre authoring (Mimimi, Thief/DromEd, SC:Blacklist TEAS, Mark of
the Ninja, Recast conventions).

Judging rule inherited from the work order: every borrowed feature must still
route through one typed command payload and stay headless-testable, or it is
rejected regardless of pedigree.

---

## 1. Verdict on the current course — keep going

The survey's strongest finding is that **the command spine we already built is
the converged industry architecture**, independently reinvented by everyone
who got tools right: Blender operators (typed properties + poll + execute,
backing menus/keys/gizmos/search/undo/scripting from ONE registration),
Insomniac's LunaEdit (all mutation through one module; undo falls out),
Unreal's named transactions, Godot's EditorUndoRedoManager. Our 34 typed
commands + single dispatcher + headless tests are that pattern, already
gate-pinned. Everything below is cheap *because* of it.

Also validated as-is:
- **Tool Settings as the next feature batch** — the literature's sharpest
  single datum is Storm's click audit (changing snap: Hammer 1 click, Unreal
  6+). Snap/grid state belongs permanently visible at 0–1 clicks. The ordered
  batch is exactly right.
- **The History panel (UI-4B)** — every surveyed editor lacks a visible
  history list (Forge shipped undo with none; Godot's is still a proposal;
  Blender's is a buried menu). Ours is a genuine differentiator; it also
  doubles as the command-stream debug view.
- Read-only toolbar (no false affordances), delete-confirm-names-the-target,
  Diagnostics rows click-to-focus, snap-on-by-default grid-native defaults —
  all match the consensus.

## 2. Trust repairs — small items violating unanimous consensus

The literature's one repeated law: **an editor that loses work or lies about
undo is routed around forever.** We currently break it in four small ways.

- **2.1 Undo must survive Save.** `dispatchCreativeDesktopCommands` calls
  `clearEditHistory` on Save/SaveAs (tags `desktop_save`/`desktop_save_as`).
  Unanimous verdict: save is not a history barrier — "saving costs you your
  undo stack" is the canonical trust-killer. Keep clearing on New/Open only.
- **2.2 Unsaved-changes guard.** New/Open discard a dirty document with no
  prompt. Confirm modal that names the document (same doctrine as the
  delete-confirm fix).
- **2.3 Shortcuts are currently decorative.** Menu strings ('Ctrl+S',
  'Ctrl+Z', 'Del') are ImGui display labels; no verified key routing while
  the shell owns input. Real chords must dispatch through the same command
  frame — palette, menus, keys, and tests as four faces of one registry.
- **2.4 Pin the gesture discipline.** TrenchBroom's rule: preview during
  drag, exactly ONE command on release, Escape cancels emitting nothing and
  consuming no history. We believe we do this — pin it with a headless test
  so it can't regress.

## 3. Command-spine dividends — near-free given the registry

- **3.1 Command palette (Cmd+P).** Fuzzy search over the command registry +
  room objects + save slots. Converged across Unity (Quick Search), Blender
  (F3), Godot ("ship it early"), VS Code lineage. With a typed registry it is
  a projection, not a feature; poll-failed entries render grayed **with the
  reason** (Blender's disabled-tooltip guideline).
- **3.2 Adjust Last Operation.** Blender's redo panel: after a command
  commits, render its payload (auto-generated from the payload schema);
  editing a field = undo top-of-stack + re-dispatch. Strict validity rule:
  live only while that command is top of the undo stack — any other mutation
  dismisses it (Blender bug #78171 is the trap to design out).
- **3.3 Repeat + payload persistence.** Last payload becomes the next
  default; Shift+R re-dispatches the last command verbatim. Collapses
  stamp-another-guard / another-crate workflows to one keystroke.
- **3.4 Duplicate-and-translate as one gesture** (TrenchBroom Ctrl+arrow) —
  the single biggest room-furnishing accelerator in the genre; trivially a
  Duplicate + transform-delta composite command.
- **3.5 Command echo log.** Blender's Info editor lesson: print each executed
  command as its serialized payload; a session transcript is literally a
  replayable headless fixture. One artifact, three consumers (user log, test
  authoring, bug repro).

## 4. Safety net — a single dev has no backup colleague

- **4.1 Crash-safety trio** (Unity/Blender/LDtk convergence): timer autosave
  to a sidecar dir with rotating retention (~5), snapshot before bake and
  before Play, unclean-shutdown banner offering Recover on next launch.
  Status bar shows autosave age — users must SEE that autosave exists before
  they trust it.
- **4.2 Open picker.** `OpenDocument` currently has no payload and reloads
  whatever `activeSaveId` already is — there is no save-slot browser in the
  shell. Payload + a minimal picker.

## 5. The stealth seam — where Vaultwright stops being generic

The genre survey's central lesson: **for a stealth game the overlays ARE the
content.** Iteration dies when every tuning question costs a play round-trip.

- **5.1 One path primitive, two consumers.** Quake's `path_corner` served
  monsters AND func_train; LDtk's PointPath fields ditto. Our moving-platform
  path grammar (waypoints, per-point dwell, PingPong/Loop, preview scrub) is
  already the right shape — patrol routes should be the second consumer of
  the SAME system, adding per-point facing/action marker and a route-level
  **phase offset** (Mimimi doctrine: 4–8s desync between guards on similar
  loops is what makes rooms read well; tooling that makes copy-paste easy but
  phase-offset hard actively causes synced-beat rooms).
- **5.2 Overlays panel.** One-key, non-modal viewport toggles: view cones,
  hearing radii, walkable area, routes/paths. Cone rendering per the Shadow
  Tactics standard: hard-edged, zone-coded (hatched crouch-safe, solid
  danger), never gradients — pixel-exact reasoning. Detail is
  selection-scoped (Unreal Gameplay Debugger lesson: unscoped debug is soup
  past ~3 agents). Render overlays in the capture path → cone coverage
  becomes screenshot-testable.
- **5.3 Noise probe.** "If the thief makes noise at this tile, which guards
  hear it" as a command (Mark of the Ninja's ring, evaluated at author time —
  distance + wall occlusion suffices on a grid).
- **5.4 Deterministic sim-step.** "Advance the baked room N ticks" as a typed
  command with overlays live — Unreal Simulate-in-Editor's rung without
  possessing a player, and Visual-Logger-style scrubbing falls out of
  determinism for free. Transport-bar UI later; the command comes first.
- **5.5 Bake receipts.** Genre validators wired into the reserved
  Diagnostics tabs ("Diffs / Stale Outputs / Proof Receipts do not exist
  yet"): unreachable waypoint, route-segment-through-solid, cone-facing-wall,
  orphaned platform path. Fail-closed bake, fail-open save (never refuse to
  save a broken room; refuse to bake it, with click-to-focus rows). The Dark
  Engine's silent room-brush failures are the cautionary tale.

## 6. Feel and infrastructure

- **6.1 Layout persistence.** `io.IniFilename = nullptr` means layout
  amnesia every launch — the literature reads this as "the tool doesn't
  respect my time." Write the ini to app-support (the nullptr was about not
  polluting CWD — right instinct, wrong fix), keep Reset Layout, add at most
  2–3 named presets (Author / Playtest / Capture). Fixed opinionated
  geography beats free docking (TrenchBroom); never auto-switch a panel the
  user is reading (Godot proposal #8264).
- **6.2 Rotate gizmo.** Move-only today; the rotate/scale kernel exists.
  Standard conventions verbatim (W/E/R, RGB=XYZ, arcs) — zero invention
  budget on input conventions (Blender's 20-year right-click-select lesson).
- **6.3 Multi-select batch transform.** `SetObjectTransform` payload is
  single-object; N-selection + one edit should fan out as one command, mixed
  values rendering as a dash. Belongs in the command layer so panels can't
  lose it (Unity's custom-inspector decay).
- **6.4 Outliner ergonomics.** Collapsing tree nodes (`hasChildren` is
  already computed and unused), rename-in-place, context menu.
- **6.5 ImGui polish checklist** (Tracy's lesson): FreeType with light
  hinting, two font weights + an icon font, 4/8px spacing grid. This is the
  lever that moves an ImGui tool from "debug overlay" to "product."
- **6.6 Non-blocking bake.** Known JobSystem gap; every source treats a
  frozen tool as THE iteration-killer. Kernel-gated work (core-spine rules) —
  recorded here for completeness, not sequenced by this doc.

## 7. Skip ledger — explicit non-goals, with reasons

- Behavior-tree/graph editing — guard behavior stays code in `runtime/ai`;
  the editor authors data (routes, cones, markers, spawns) only.
- Per-guard/per-difficulty cone tuning UI — one global detection preset
  table (Shadow Tactics shipped ONE cone spec; mastery transfers).
- Prefab variants / nested overrides — Unity's apply-to-wrong-level failure
  class and TrenchBroom's years of linked-group corruption; plain groups +
  duplicate cover a single dev.
- Workspace/keymap/theming infrastructure — one keymap in a config file;
  Blender's alternative keymaps rot as second-class citizens.
- Asset folder trees, tags, import-pipeline UI — the palette is dozens of
  assets; one filterable grid. Import stays deferred on the renderer seam.
- Navmesh view-flag zoo — one walkable tint + off-mesh-link arcs is the
  whole story for grid rooms.
- Vertex snapping, 3D cursor, custom transform orientations, pie menus,
  quad-ortho layouts — mesh-modeling machinery, not room authoring.
- Checkbox creep — each request passes the why/why-now rubric; solo dev
  makes the discipline easier to skip and thus more necessary.

## 8. Proposed sequencing (Ace cuts the orders; this is the menu)

Already fixed by Ace: **Tool Settings + map/asset-source document tabs**
next; imported-asset UI deferred. After that, in leverage order:

1. **Batch: trust repairs** — §2.1–2.4. Small, self-contained, each a
   one-command-family change with a headless pin.
2. **Batch: palette + repeat** — §3.1, §3.3, §3.4. The command spine's
   dividend; replaces menu design and shortcut sprawl at once.
3. **Batch: safety net** — §4.1–4.2.
4. **Batch: patrol = path unification** — §5.1. First stealth-seam cut;
   opens §5.2 overlays, then §5.3 probe / §5.4 sim-step in later batches.
5. Opportunistic: §3.2 adjust-last-operation (needs its invalidation rule
   designed), §6.1 layout persistence, §6.2 rotate gizmo, §6.3 batch
   transform, §6.4 outliner ergonomics, §6.5 polish pass.

Recurring ritual regardless of batch: **Storm's click audit** — monthly,
count keystrokes for the 5 hottest operations (place, nudge, path-point
edit, waypoint edit, bake+play) and drive toward 1–2.
