# Creative Mode — The Two Faces (product vision)

> Planner-authored 2026-07-05 from direct user design conversation, AFTER the
> user actually ran the editor and found it unusable. This supersedes the
> "generic select/move/measure/create palette" interaction model in
> `creative_tooling_v1.md` §3 (that layer's FOUNDATION survives; its
> single-muddled-mode UI does not). The foundation (world_foundation_plan_v0_1
> D1–D8; TV1-A..H: document, objects, mutations, snap, drag-to-world math, fly
> camera, save) all carries. What changes is the AUTHORING FACE on top.

## The core insight (user's words): "two faces to the problem"

Creative mode has TWO distinct interaction paradigms, chosen by context, and
they complement each other — rough it out on one, refine on the other:

- **FACE 1 — IN-WORLD BUILD ("like Minecraft"):** active while FLYING (the
  Navigate/fly camera). A ghost of the current object sits on the grid cell the
  camera is aiming at (camera ray → grid cell); click to place. Spatial,
  immediate, approximate. You block out shapes fast by flying and dropping.
- **FACE 2 — DRAFTING ("like drafting/CAD"):** active in the creative UI. Pick
  an object type, TYPE its dimensions (L×W×H), and click-and-drag to size/place
  exactly. Precise, planned, dimension-first. "Make a floor by setting 20×20 and
  dropping it."

Speed on Face 1, precision on Face 2. Both write the same CreativeDocument.

## What the user explicitly asked for

1. UI for selecting WHICH object type to make (palette — both faces; hotbar
   in-world, panel in drafting).
2. UI for HOW to manipulate them (select/move/adjust — shared).
3. UI to SET DIMENSIONS — type L×W×H — the heart of Face 2. "Quickly make a
   floor by setting in dimensions."
4. A way to CLICK AND DRAG — to size/place (Face 2 precise, Face 1 spatial).
5. Axis + dimension building: drop a point, pick an axis/direction, give a
   length (type or drag), a piece appears; new point, repeat — stack elevations
   fast (SketchUp/CAD extrusion feel). This lives primarily on Face 2 but is the
   "build fast" spine.
6. Start in a COMPLETELY BLANK map and build fast WITHOUT fumbling with
   controls. Speed and fluidity are first-class requirements.

Dimension input = BOTH typed (fast/precise) AND click-drag (tactile) — the user
confirmed both.

## Why the current build failed the user (diagnosed from the real running app)

- **No stage of its own:** entering creative mode calls `createProductSession`
  → loads the `first_room` demo ("Loop Keep") → creative overlays an empty doc +
  wireframes ON TOP of a finished gameplay room. User was standing in a built
  level, not a blank canvas. (Operations.cpp:1041; foundation plan flagged "no
  deliberate stage — only accidental.")
- **Create spawns at world origin, not where you point** (create-at-origin +
  count offset; click-to-place was deferred). User: "I didn't get to say where."
- **Objects render as wireframe over the gameplay scene** — light-blue
  (Structural style {0.42,0.78,0.86} = Room) boxes lost among real room meshes.
- **New World writes a permanent save every time, no cleanup** → 160+ saves
  (151 identical Loop Keep starters). Archived 2026-07-05 to
  `~/.iggy3d/saves_archive_*`; the CAUSE (persist-on-create, no prune) still
  needs a fix.

## Reprioritized roadmap (supersedes the TV1-I/J/K ordering)

The remaining generic slices (hygiene, selection highlight, QA harness) drop
below making the editor actually usable. New spine, build-small-then-RUN-WITH-
USER at each step (the lesson: no more building blind):

- **F0 — Blank stage.** Entering creative mode = an empty world: ground plane +
  visible grid, NOT the first_room demo. Own package/scenario or a synthesized
  empty stage; camera framed on the origin. Prereq for everything.
  - **FIXED 2026-07-05 (32d13f38):** the blank stage rendered an empty void
    ("creative doesn't launch") because the ground grid was pinned to
    floor(camera.y)=6m (eye height) — off-screen under the downward camera. Now
    the creative-stage grid stays at Y=0 (legacy map-maker still follows
    elevation): `config.planeY = mapMakerLive ? floor(anchor.y) : 0` in
    ProjectionRefresh.cpp:buildMapMakerGridForFrame. Verified headless (grid at
    Y=0, visible, reaches draw list); camera (0,6,10) pitch -30 frames Y=0 at
    the origin. STILL PENDING a real windowed look with the user.
- **F1 — Drafting face MVP.** Object palette (Floor/Room/Crate) + dimension
  fields (type L×W×H) + click-drag to place a sized object. Deliver "pick Floor,
  type 20×20, drop it."
- **F2 — In-world build face.** On the fly camera: object hotbar, camera-ray →
  grid-cell ghost, click to place, Minecraft-style.
- **F3 — Axis+dimension extrusion.** Point → axis → length (type/drag) → piece →
  repeat; stack elevations fast.
- **Fx — Save hygiene** (fold in): a new world does not become a permanent save
  until the user chooses to save; prune/cap; a clean discard path.

CAMERA IS PART OF THE FEEL: drafting click-drag wants a plan/top-down view; the
in-world face wants the fly camera raycasting to a cell; current projection is
flat front-on (screen=XY). Each face needs camera work to feel right — surface
it per step, do not let it block F0.

## Open design question (still to settle, non-blocking for F0)

- Object containment: flat document vs rooms-contain-their-contents. Deferred;
  F0/F1 can proceed flat, revisit when multi-room authoring arrives.

## Render + menu→stage switch model (mapped 2026-07-05, verified)

How a frame is produced and how creative "launches" visually — so we stop
guessing:
- **Loop:** `runProductWindowLoop` (window/Loop.cpp) is a CONTINUOUS ~60fps
  loop. Per iteration, in order: poll SDL events → `processProductWindowInputFrame`
  (Loop.cpp:228, where a menu click launches) → build projection frame →
  `presentProductWindowFrame` (Loop.cpp:271). Input runs BEFORE present on the
  SAME live window/frontend refs, so a click shows in the same frame.
- **Menu-vs-stage gate:** the opening menu draws iff
  `frontend.screen == FrontendScreen::Starter` (FramePresenter.cpp:858).
- **The switch:** click → routeProductOpeningMenuInput(MenuConfirm) →
  confirmStarterCreativeNewWorld → launchProductCreativeNewWorld →
  enterProductGameplayTransition → enterFrontendGameplay sets
  `frontend.screen = Gameplay` UNCONDITIONALLY (FrontendState.cpp). Next present
  drops the menu, draws the stage. The automation path
  (routeAutomationInput) converges on the SAME routeProductOpeningMenuInput, so
  headless `--automation-control` faithfully exercises the click path.
- **Title:** productWindowTitle now keys on `frontend.screen` FIRST (Loop.cpp,
  commit 08f76e55) so it can't disagree with the drawn body.

**STALE-BUILD TRAP (cost a whole session, 2026-07-05):** a user screenshot showed
title "iggy3d - Creative" OVER a still-visible starter menu. That state is
IMPOSSIBLE in correct source (title requires interactionMode=Creative, set one
line AFTER the screen transition). Root cause was a partially/stale-rebuilt
windowed binary: title (window.gameplayActive, window-side) and menu gate
(frontend.screen, frontend-side) read different truth, so a not-fully-relinked
binary split them. LESSON: when windowed behavior contradicts the source, force a
clean rebuild of the EXACT binary before chasing a code bug. Also: the headless
`--print-render-receipt` `window_title` field is a crude formula
(ReceiptBuilder.cpp:505-506, never emits "Creative") — use `interaction_mode`/
`frontend_screen`/`opening_menu_visible`, not `window_title`, as the proxy.

## Discipline for this lane going forward

Build ONE small step, then RUN THE APP AND LOOK (with the user), then iterate.
Unit-green is necessary but NOT sufficient — the editor must be seen working.
Keep the full-all-targets build + forced-clean-rebuild gate. Local commits only.
