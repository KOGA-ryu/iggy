# Creative Tool Completion Backlog

Status: canonical product-completion backlog

This document answers one question: what must exist before a Creative feature
may be called a finished tool?

It deliberately does not count settings, buttons, commands, glyphs, templates,
or internal kernels as tools. A tool is a creator-facing interaction mode that
solves a complete authoring problem. The 3D viewport and 2D drafting canvas are
frontends over the same product capability; implementing both does not create
two tools.

## Product Truth

- No currently exposed tool is assumed complete merely because it has an enum,
  UI entry, preview kernel, receipt, or unit test.
- A deterministic geometry operation is a foundation, not automatically a
  useful authoring workflow.
- A tool remains incomplete when its output cannot be selected, inspected,
  revised, saved, loaded, rendered correctly, collided with correctly, or
  operated with the supported keyboard, touchpad, and PS5 controller inputs.
- A deferred tool still needs an explicit completion contract. "Later" must not
  mean "undefined."
- Incomplete tools should be hidden from the normal toolbox. They may remain in
  an opt-in experimental/lab surface while their kernels are developed.

## Inventory Boundaries

The current registries expose several overlapping kinds of things:

- `CreativeHeldItemKind`: 23 held interaction modes in the 3D viewport.
- `CreativeEditorWorldLayoutTool`: 16 drafting-canvas modes.
- `CreativeEditorToolDescriptor`: 37 product capabilities projected into the
  held-item catalog, drafting toolbox, options, hints, and help surfaces.
- `CreativeToolOptionId`: 68 settings. These are not 68 tools.
- `creative::Tool`: Select, Move, Measure, and Navigate facade modes.

This backlog normalizes those surfaces into the product capabilities below.
Room, selection, terrain region, placement, and building operations must
eventually share one semantic implementation across their 2D and 3D frontends.

## Maturity Ladder

- **M0 - Name only:** enum, button, marker, or command exists.
- **M1 - Prototype:** an interaction can produce output under narrow conditions.
- **M2 - Stable recipe:** one deterministic, validated request produces one
  exact preview/commit plan with explicit provenance and bounded failure.
- **M3 - Product tool:** the complete creator workflow satisfies the universal
  definition below on every supported input surface.
- **M4 - Integrated:** the tool interoperates with the rest of Creative and its
  output survives render, collision, navigation, save/load, and re-editing.

Only M3 and M4 tools belong in the default toolbox. M0-M2 capabilities are
internal or experimental even when their code remains useful.

## Universal Definition Of Done

Every tool must satisfy every applicable item. Any exception must be explicit
in the tool's own section and must be visible to the user.

### Purpose And Ownership

- [ ] The tool has one sentence describing the creator problem it solves.
- [ ] Its output semantics are owned by one shared recipe/planner, not duplicated
  between UI, 3D interaction, 2D drafting, preview, and commit paths.
- [ ] The tool declares whether it is parametric, destructive, or observational.
- [ ] A destructive tool warns through its workflow and leaves normally editable
  output; it does not masquerade as a reopenable parametric operation.
- [ ] Settings and submodes are data in the tool descriptor, not additional
  hardcoded tool branches on each UI surface.

### Discovery And Input

- [ ] The tool is discoverable by category, searchable by name and aliases, and
  has one stable semantic action ID.
- [ ] Keyboard/mouse, MacBook touchpad, and PS5 controller can perform the full
  common workflow without an unavailable button such as middle mouse.
- [ ] Input context, button consumption, simultaneous presses, hold/repeat
  behavior, focus loss, modal interruption, and cancellation are defined.
- [ ] Contextual controls and current values are visible while the tool is active.
- [ ] Rebinding cannot create an unreported conflict or remove the user's escape
  route from the tool.

### Configuration And Targeting

- [ ] Meaningful parameters can be configured before commit.
- [ ] Common parameters have direct manipulation and precise numeric entry.
- [ ] Units, limits, defaults, and snapping rules are visible and physically
  consistent with the document grid and runtime world scale.
- [ ] Targeting identifies the affected object, cell, surface, level, host, or
  path before the user commits.
- [ ] Snap modes and attachment hosts are explicit, previewed, and reversible.

### Preview And Failure

- [ ] Preview and commit consume the same immutable plan; they cannot drift.
- [ ] The preview shows exact bounds, orientation, material, affected members,
  and valid/invalid status in both relevant 2D and 3D views.
- [ ] Invalid targets explain why they are invalid and how to recover.
- [ ] Capacity, overflow, non-finite input, missing assets, stale provenance,
  locked objects, and conflicting edits fail atomically.
- [ ] Preview-only state changes no document revision, save data, runtime ID,
  room signature, collision bake, or history entry.

### Commit, History, And Re-Editing

- [ ] Confirm commits one semantic operation; cancel restores the exact prior
  state; an empty gesture records nothing.
- [ ] One gesture or explicit Apply action creates one undo record regardless of
  internal object count.
- [ ] Undo and redo restore source parameters, generated output, selection, and
  dependent derived data together.
- [ ] Parametric output retains stable provenance and can be reopened with its
  original parameters.
- [ ] Post-commit output supports applicable select, move, rotate, scale, snap,
  duplicate, group, material, replace, and delete operations.
- [ ] Changing a source recipe updates its owned generated output without
  silently overwriting user-refined or unrelated objects.

### Engine Integration

- [ ] Save/load round-trips source intent, stable IDs, and editable parameters.
- [ ] Missing or version-mismatched dependencies produce repairable diagnostics.
- [ ] Render geometry preserves the authored type, dimensions, material, pivot,
  and orientation instead of collapsing to a generic proxy.
- [ ] Collision matches visible geometry closely enough for the player movement
  envelope, including steps, slopes, openings, headroom, and ledges.
- [ ] Navigation, line of sight, cover, logic, and gameplay semantics rebuild
  when the authored result affects them.
- [ ] The tool declares cache invalidation and derived-data rebuild domains.

### Performance And Proof

- [ ] The algorithm states complexity, deterministic ordering, and hard capacity
  behavior; large work is chunked or cancellable where necessary.
- [ ] Idle aiming does not rebuild or upload unchanged scene geometry.
- [ ] A stress fixture proves the supported upper bound without freezing input.
- [ ] Pure planner tests cover every option, invalid case, and preview/commit
  parity.
- [ ] One headless workflow test crosses UI command, recipe, document, history,
  save/load, render bake, and collision/nav outputs as applicable.
- [ ] One visual regression fixture proves readable preview and final output.
- [ ] One manual keyboard/touchpad test and one PS5 controller test prove the
  complete workflow from discovery through re-editing.
- [ ] User-facing controls and limitations are recorded in the relevant contract.

## Shared Completion Work

These are prerequisites for all individual tools. Reimplementing them per tool
is forbidden.

### G0.1 Shared Semantic Recipe Boundary - Complete

- [x] Extend the recipe layer beyond Building and ObjectLibrary so terrain,
  patterns, structural edits, and authored paths have durable source records.
- [x] Route 2D and 3D frontends through the same recipe/planner and provenance.
- [x] Define explicit destructive-operation records for operations that should
  not remain parametric.
- [x] Provide one apply/reconcile/history contract for every recipe family.

Milestone note (2026-07-22): `AuthoringContract.hpp` is the exhaustive family
boundary for Building, ObjectLibrary, Road, Watercourse, Bridge,
RetainingEdge, Terrain, Volume, Pattern, AssetScatter, and Prefab. Each family
declares its lifecycle, durable source store, and supported plan, preview,
apply, reconcile, history, destructive-record, and frontend-neutral
capabilities. Validation rejects missing families and illegal capability
combinations.

Every semantic edit stored in history now carries a versioned
`CreativeAuthoringOperationRecord`: family, operation kind (`Apply`,
`Reconcile`, or `Destructive`), derived lifecycle, stable action,
deterministic request fingerprint, and exact affected-member count. Operation
validation prevents a destructive family from claiming reconciliation and
prevents a parametric family from recording an unadvertised destructive edit.
Pattern, asset-scatter, and prefab detach/removal therefore remain associated
with their source family while being recorded honestly as destructive. Undo
and redo preserve this record unchanged.

Building and authored path families reconcile through the fingerprinted World
Layout plan consumed by both the 2D source canvas and 3D generated output.
Terrain consumes the same exact edit vectors for preview and apply. Pattern,
scatter, and prefab operations fingerprint their durable recipe or authored
source; volume operations explicitly remain destructive. Durable authored
asset file writes are intentionally outside document undo, while the
document-side source acknowledgement is a named reconcile operation so undo
correctly restores the source-changed state instead of claiming to roll back
the file. Eleven focused recipe, terrain, world-layout, pattern, scatter,
prefab, volume, placement, group, codec, and source-history tests pass.

### G0.2 Unified Tool Descriptor - Complete

- [x] Replace presentation-only records with a descriptor that owns category,
  availability, settings, actions, input hints, lifecycle, and maturity.
- [x] Generate toolbox, catalog, options, controller hints, and help surfaces
  from that descriptor.
- [x] Add a release gate that rejects a default-visible tool below M3.
- [x] Keep validation switches exhaustive, but remove repeated business-rule
  `if` chains from canvas and interaction surfaces.

Milestone note (2026-07-22): the canonical descriptor now owns orthogonal
drafting input, preview-geometry, preview-fill, prompt, and volume-settings
profiles in addition to category, availability, settings, actions, hints,
lifecycle, and maturity. A consteval direct map requires every World Layout tool
to have exactly one descriptor; validation rejects missing, duplicate, invalid,
and internally inconsistent bindings before a tool can ship.

The Plan canvas resolves selection mode once per frame and consumes descriptor
profiles for gesture admission and previews. Interaction, overlay, and placement
surfaces consume held-item registry frame/interaction policies instead of
re-identifying Object Move, Terrain Region, Logic Link, material strokes, or the
five volume operations. Exhaustive switches remain where they choose distinct
geometry or edit algorithms; they no longer duplicate user-facing capability
rosters. The `i3dc` build and focused toolbox, World Layout, placement, and
interaction tests pass, and a negative source gate confirms the retired tool
rosters are absent from the canvas and interaction owners.

### G0.3 Synchronized 2D And 3D Inspection - Complete

- [x] Provide Plan, Elevation, and 3D views with synchronized selection.
- [x] Permit a split 2D/3D workspace so editing never hides inspection.
- [x] Preserve camera and panel state while switching views.
- [x] Show the same preview validity and authored source in every view.

Milestone note (2026-07-22): one read-only inspection resolver now owns the
layout source, authored-source identity, preview validity, content revision, and
volatile-candidate status consumed by Plan, Elevation, and the live 3D viewport.
An accepted exact preview always wins source ownership. Local candidates without
a compilable 3D preview are visibly invalid rather than appearing valid in 2D
while 3D displays authored output. The shared status is shown in both drafting
views and the 3D toolbar.

Plan and Elevation caches include exact-preview content revision and bypass
reuse for volatile candidates. Repeated identical building transforms preserve
and reuse the existing exact preview instead of invalidating or recompiling it.
The dockspace is built only on first use or explicit Reset Layout, so opening,
closing, and switching World Layout views preserves the user's panel geometry;
Plan and Elevation retain independent pan and zoom state, and source-history
restores preserve both inspection cameras and the active view. Building
transform, building-template placement, and room-drag command paths are pinned
to the same compiled preview source used by 3D. The `i3dc` build and six focused
desktop, layout, Plan, toolbox, source-history, and editor tests pass.

### G0.4 Input Parity - Complete

- [x] Define touchpad pan/orbit/zoom without middle mouse.
- [x] Make every default-visible tool reachable and fully operable on PS5.
- [x] Reserve X for accept/advance and Circle for cancel/remove consistently.
- [x] Add contextual D-pad and radial focus behavior without stealing camera
  control or creative flight.
- [x] Add automated binding-conflict and unreachable-action checks.

Milestone note (2026-07-22): the input router retains one context-owned PS5
grammar: X accepts or advances, Circle rejects, cancels, removes, or clears;
R3 owns the radial wheel; D-pad input is contextual; and camera look is blocked
while the radial context owns the right stick, then rearmed only after the
stick returns to neutral. Creative flight remains available in viewport,
replacement-preview, and transform-preview contexts. Touchpad navigation uses
the shared focus-based pan, orbit, and dolly kernel without requiring a middle
mouse button.

The only default-visible tools are Select and Transform. Both are default wheel
entries with live descriptor hints and valid held-item definitions. Select's
previously dishonest Circle hint now maps through the registry to a semantic
`ClearSelection` operation that clears both Facade selection and the synchronized
World Layout source selection. The fixed-capacity release audit covers 80
required gamepad action/context/activation tuples across the viewport, catalog,
tool wheel, tool options, asset replacement, asset library, authored-asset edit,
transform preview, transform controls, Controls, and runtime play. It rejects
missing, wrong-device, wrong-activation, and invalid requirements independently
of the existing chord-conflict audit. `i3dc` and seven focused input, selection,
placement, hint, viewport, and toolbox test targets pass.

### G0.5 Physical Scale And Render Semantics - Complete

- [x] Establish one meter/grid/storey/player-envelope authority.
- [x] Eliminate template-local scale inventions such as exceptional storey
  heights unless explicitly named as a style profile.
- [x] Preserve architectural roles and material identity through room bake and
  Vulkan rendering.
- [x] Render actual slabs, wall thickness, openings, stairs, ramps, roofs, and
  asset meshes rather than generic boxes or top planes.

Milestone note (2026-07-21): authored documents own meters and the exact grid;
architectural profiles own floor-to-floor and structural-layer policy; object
descriptors own structural layer thickness; and the narrow runtime
`MovementDefaults.hpp` contract now owns the standing player body, step, ground
snap, skin, speed, and slope defaults. Opening clearance, building usability,
physical traversal, clamber thresholds, and the architectural human scale
marker derive from that runtime contract instead of copying numeric literals.
One shared profile-to-grid resolver is consumed by normalization, blockout UI,
and built-in templates. Builder Estate no longer uses 22-metre viewport-tuned
storeys: it declares the Grand profile and resolves to two equal 5-metre
storeys, one 10-metre exterior facade envelope, and profile-owned slab layers.
The two built-in map templates now resolve architectural scale through named
profiles: Builder Estate uses `Grand`, while Ditch House declares its explicit
three-metre custom profile and derives slab and wall thickness from descriptor
and wall-geometry authorities. The Map Demo's remaining dimensions are named
kit measurements or explicit gameplay/traversal proof thresholds, not hidden
storey defaults.

Room bake now carries exact source meaning separately from coarse batching
roles, and preserves material ids and imported material variants through scene
projection, geometry signatures, floor/wall optimization, and Vulkan CPU
geometry. Floors, ceilings, and flat roofs render as full-thickness boxes;
only actual floors receive the editing grid. Building openings remain physical
wall cutouts assembled from side, sill, and lintel pieces. Stairs, ramps,
open-frame arches, sloped roof panels, tapered hip-roof panels, and imported
asset meshes retain their dedicated geometry paths. Tree assets collide through
trunk-only compound bounds, while shrubs, ferns, reeds, branch piles, and other
soft ground cover explicitly remain non-colliding. `i3dc` and ten focused
room-bake, projection, material, building, architecture, template, opening,
roof, and vertical-connector tests pass, including exact semantic/material
propagation and full slab volume.

### G0.6 Product Validation - Complete

- [x] Keep structural/topology validators as advisory guardrails.
- [x] Do not label topology-only results as complete usability proof.
- [x] Add actual player-motor traversal, doorway, stair, ramp, headroom, and
  collision tests for authored buildings.
- [x] Add canonical creator tasks for a building, terrain corridor, and asset
  composition; each task must include save, reopen, modify, and undo.

Milestone note (2026-07-21): `creative_creator_task_workflow_tests` now drives
three complete creator tasks through production seams. A two-room building is
staged, generated, saved, reopened, property-edited, and undone with its source
and generated output synchronized. A road corridor preserves authored height
and material through save/reopen, then extends and undoes atomically. A grouped
pair of asset-backed props publishes as a durable authored asset, survives both
document and library reload, then modifies and undoes without losing hierarchy
or asset identity. `i3dc` plus the new workflow and five adjacent building,
terrain, authored-asset, and save-section suites pass.

## Core Workspace Tools

### C1. Navigate And Inspect - Current M2 - Finish Now

Product outcome: move through the workspace and inspect authored output without
accidentally editing it or losing the desktop UI.

- [x] Make viewport capture/release deterministic across mouse and touchpad.
- [x] Provide orbit, pan, fly, frame selection, frame all, plan, elevation, and
  3D perspective paths.
- [x] Preserve one explicit camera scale and speed model across input devices.
- [x] Keep selection, overlays, and UI visible when entering/exiting navigation.
- [ ] Add controller and touchpad manual acceptance tests.

Milestone note (2026-07-21): World Layout no longer occupies the only central
viewport. Its Plan/Elevation canvas uses a resizable sibling dock while the
remaining central passthrough node stays a live 3D view, so drafting and visual
inspection remain side by side. The desktop toolbar now exposes icon actions
for framing the visible selection and all visible document objects through the
shared camera-bounds planner. Empty inputs fail visibly and neither action
touches document revision or history. Pointer capture, UI visibility, and the
split-layout contract are headless-pinned. A single allocation-free camera
kernel now owns focus distance, orbit, perspective-scaled pan, and exponential
dolly. In desktop mode, Option+drag orbits, Shift+Option+drag pans, and
Option+two-finger scroll dollies; ordinary click/place and scroll/hotbar input
remain unchanged. Free-cursor motion cannot rotate the camera. Plain viewport
click still enters persistent fly-look, while orbit/pan release with the drag.
Frame Selection and Frame All establish the same reusable camera focus, and
the PS5 Create button invokes one contextual semantic action that frames the
selection or falls back to the scene without displacing X, Circle, L2/R2, R3,
touchpad pick, or D-pad ownership. Camera state is also retained across authored
asset workspace entry/exit. The app and eight focused input/navigation tests
pass. Custom-layout preservation and manual keyboard/touchpad/PS5 acceptance
remain open before C1 can reach M3.

### C2. Select - Current M2 - Finish Now

Product outcome: identify any editable authored source or generated result and
understand what is selected.

- [x] Support click, toggle, marquee/region, hierarchy, parent, child, and
  generated-source selection consistently.
- [x] Define occlusion, cycling, locked/hidden, overlapping, and empty-space
  behavior in both 2D and 3D.
- [x] Synchronize Outliner, Inspector, 2D canvas, elevation, and 3D viewport.
- [x] Resolve generated geometry back to its semantic source recipe.
- [x] Support selection sets and large-selection performance bounds.

Milestone note (2026-07-21): one 4,096-target ordered selection state now owns
the primary object and complete selection set. Object clicks, Outliner rows,
Plan symbols, Elevation items, and generated-source Inspector actions all route
through that state and then resolve back to the nearest editable Pattern or
World Layout source. Hidden and locked objects remain selectable from semantic
views for repair; hidden objects are excluded from direct 3D picking, while
locked objects can be inspected without making them editable. Empty unmodified
clicks clear selection and modified clicks preserve it. 3D, Plan, and Elevation
overlap selection use deterministic topmost-first 64-source stacks with explicit
truncation and repeated-click cycling; duplicate glyphs owned by one source use
one cycle slot. Plan left-drag uses Crossing selection, right-drag uses Window
selection, and Shift/Command compose add/toggle sets atomically. Hierarchy
commands select a parent, direct children, or a complete bounded subtree. Stale
provenance, missing objects, and capacity overflow fail without partially
changing selection. Seven focused selection, desktop, Plan, and Elevation tests
plus the application build pass. Manual touchpad and PS5 acceptance remains a
cross-cutting G0.4/C1 gate before promoting the tool from M2 to M3.

### C3. Measure - Current M2 - Product Path Complete

Product outcome: inspect distance, height, slope, area, and dimensions without
mutating the map.

- [x] Add point-to-point, axis-projected, vertical, slope, perimeter, and area
  measurements.
- [x] Snap endpoints to grid, vertices, surfaces, openings, and levels.
- [x] Display units in 2D, 3D, and the Inspector with copyable numeric values.
- [x] Keep measurements transient by default and offer explicit annotation save.
- [x] Prove no measurement action changes document history or generated output.

The shared bounded planner now owns all six measurement modes and exact snap
provenance. Transient measurements render from the same canonical geometry in
3D, Plan, Elevation, the HUD, and the Inspector without changing document
revision, dirty domains, generated-source revision, or history. Explicit Save
creates a stable-ID document annotation; Remove, Undo, Redo, save/load, and the
v15-to-v16 migration preserve it. Saved annotations remain visible after the
transient ruler clears and are excluded from room geometry and capture output.
Eight focused measurement, Facade, command, projection, and persistence tests
plus the application build pass. Promotion to M3 remains blocked only by the
cross-cutting G0.4 manual touchpad and PS5 acceptance gate.

### C4. Transform - Current M2 - Core Path Complete

Product outcome: move, rotate, scale, mirror, and precisely nudge applicable
content with one predictable interaction model.

- [x] Provide visible 3D gizmos and equivalent 2D handles.
- [x] Support local/world axes, pivot/origin choice, free and constrained motion,
  numeric entry, snap increments, reset, and duplicate-as-transform.
- [x] Define non-uniform scaling support per object type instead of silently
  accepting unsupported transforms.
- [x] Preserve child relationships, attachments, provenance, collision, and
  generated-source ownership.
- [x] Reuse the same transform session for authored objects, groups, imported
  assets, and generated buildings where legal.
- [x] Route editable PatternRecipe and durable terrain-recipe transforms through
  the same session after their source schemas can represent the operation; never
  transform only generated output or silently detach source ownership.

Milestone note (2026-07-21): one `CreativeEditorSelectionTransformState` now
owns preview, pointer gesture, numeric editing, exact planning, clearance,
commit, and history for ordinary authored objects, complete group hierarchies,
imported asset proxies, and complete generated buildings. Visible X/Y/Z gizmo
handles use the real content viewport and local or world bases; the same session
supports free movement, constrained movement, arbitrary-axis rotation where the
descriptor permits it, quarter-turn-only bounds objects, per-axis scaling,
selection/active-object/custom pivots, snap and fine nudge, reset, mirror, and
copy-as-transform. Descriptor-owned capabilities disable unsupported controls
and the planner rejects unrepresentable requests before mutation.

Move clearance ignores only the frozen source set, while copy clearance treats
the source as an obstacle. Both use exact candidate geometry against authored
objects, voxels, and terrain and report the blocking object before commit. A
generated building routes to its `WorldLayout` source and regenerates source and
owned output in one history step; partial or stale generated selections fail
closed. Pattern-generated output now expands to its complete recipe closure and
uses that same session for rigid translation: source members, generated members,
radial pivots, scatter paint centers, and exclusions publish atomically while
recipe identity and membership remain intact. Partial, nested/dependent, stale,
locked, or World Layout-backed recipes fail closed instead of detaching output.

Manual Region, Grade, Profile, Path, Stamp, and Landform operations also use the
shared transform session through a transient bounds proxy and a grid-exact
translation plan. Commit replays the complete terrain stack and publishes the
durable recipe, height field, materials, and hard edges in one document revision
and one undo record. GeneratedTerrain and World Layout-owned operations remain
with their source editors. The desktop operation list exposes this route through
one semantic Transform command; unsupported owners are visibly unavailable.
The Plan canvas retains its source-level building move and rotate/mirror handles
over the same world-layout kernels. Focused transform, pattern, terrain,
dispatcher, toolbox, persistence, and application gates pass. Promotion to M3
still waits on the consolidated G0.4 manual touchpad and PS5 acceptance gate.

### C5. Group And Ungroup - Current M2 - Core Path Complete

Product outcome: manipulate related objects as one assembly without destroying
their individual identities.

- [x] Add visible group membership and group-local pivot editing.
- [x] Define nested-group depth, cycle rejection, lock/visibility inheritance,
  and selection behavior.
- [x] Preserve world transforms exactly through group and ungroup.
- [x] Save/load, duplicate, delete, and asset-update groups atomically.

Milestone note (2026-07-21): Groups are bounded world-space assemblies with a
persistent editable pivot, visible nested membership in the Outliner, one-root
selection, and explicit focus for editing members. Pivot edits change only the
Group root record; member transforms remain byte-for-byte unchanged. Move,
rotate, scale, mirror, array, clipboard, paste, duplicate, cut, delete, ungroup,
undo, and redo expand a selected Group to its complete hierarchy and commit
atomically while selecting only the resulting root where appropriate.

Hierarchy depth is capped at eight parent edges. Creation, grouping, mutation,
and restore reject missing parents, unsupported parent owners, cycles, and
depth overflow before partial work. Visibility and lock state inherit through
the full chain and fail closed when hierarchy truth cannot be resolved. The same
effective state now gates selection mutation, transform, picking, wireframes,
preview proxies, placement clearance, spatial projection, room bake, runtime
interactables, map validation, generated-source overlays, and authored-asset
refresh/update. The document resolver follows the indexed object lookup per
ancestor, so normal hierarchy checks are bounded by depth rather than document
size.

Save/load preserves Group kind, pivot, flags, and parent ids exactly. Complete
hierarchies survive clipboard and duplicate id remapping; group deletion and
asset refresh roll back as a unit when a descendant or ancestor is locked.
Fourteen focused group, persistence, tools, asset, rendering, projection,
selection, desktop, and clearance tests plus the application build pass. Groups
intentionally remain organizational world-space assemblies, not local-transform
stacks: changing a pivot never silently rewrites members, and transformations
of members occur only through an explicit whole-hierarchy command. Promotion to
M3 remains blocked by the shared manual touchpad and PS5 acceptance gate.

### C6. Clipboard, Duplicate, Delete, Undo, And Redo - Support System - Now

These are commands, not independent tools.

- [x] Make them work for every capability that passes M3.
- [x] Preserve semantic recipes instead of copying only generated geometry.
- [x] Define external references, stable-ID remapping, attachment repair, and
  cross-document paste behavior.
- [x] Expose bounded, named history entries and a usable History panel.
- [x] Ensure delete offers source-level deletion when generated output is picked.

Milestone note (2026-07-21): clipboard operations now carry complete hierarchy
closures, internal logic links, and editable PatternRecipe records. Paste assigns
fresh object and recipe ids, repairs internal parents and attachments, remaps
logic-link endpoints, preserves translated pattern pivots, and rejects recipe
transforms its schema cannot represent. Copying an independent source remains an
independent copy; picking generated Pattern output expands to the complete
editable recipe closure. Authored-asset capture and placement use the same
semantic clipboard representation, so reusable assets do not bake arrays into
anonymous geometry.

Cut and Delete fail atomically when a logic link or another recipe crosses the
selected closure. Whole groups and attachment hierarchies remove and restore in
one named history step; locked descendants reject the transaction. Picking
Pattern output deletes its Pattern source closure, while picking World Layout
output edits its 2D source first and leaves compiled geometry intact until the
normal regeneration boundary. Multi-selection Delete no longer removes only the
primary object.

One ImGui-free history model now merges the bounded World Layout source history
and document snapshot history in exact router priority. The docked History panel
shows newest-first named Undo/Redo rows, capacities, source domains, and disables
stale document entries until World Layout source and generated revisions agree.
Fifteen focused clipboard, pattern, group, attachment, authored-asset, logic,
history, desktop-command, persistence, and Facade tests plus the application
build pass. No capability is promoted to M3 yet because the shared manual
touchpad/PS5 gate remains open; this support layer is complete across the current
M2 product paths and is required for every future M3 promotion.

## Placement And Asset Tools

### P1. Material Or Block Placement - Current M2 - Core Path Complete

- [x] Keep held-object, exact target preview, snap plane, depth, anchor, yaw, and
  material visible before commit.
- [x] Support single placement and bounded continuous strokes with deduplication.
- [x] Make placed output immediately selectable and editable.
- [x] Resolve overlap, replacement, occupied cell, unsupported shape, and asset
  collision failures explicitly.
- [x] Preserve actual material and object role in final rendering.

Milestone note (2026-07-21): one immutable `CreativeBrushPlacementPlan` owns
target geometry, held/target preview geometry, storage policy, transform,
surface contact, anchor, yaw, and commit data. The viewport shows a stable held
proxy plus the exact admitted target, active snap plane/depth/grid layer, anchor,
and orientation before mutation; transient aiming changes neither the room
signature nor static-mesh upload signature. Voxel, authored-object, imported
asset, ramp, stair, open-frame, structural-span, and attachment previews retain
their real geometry profiles rather than falling back to one box.

Single placement and 200 ms continuous place/remove gestures share one bounded
repeat owner, a fixed 256-key visited set, primary-action precedence, lazy
history, interruption finalization, and one undo record per changed gesture.
Accepted authored output can be selected and transformed immediately; accepted
voxel output is addressable and editable immediately. The revision-keyed scene
cache performs no rebuild across 300 idle frames or aim-only movement, refreshes
only changed voxel chunks, and updates once after an accepted mutation.

Admission and mutation status now reach creator-visible feedback for missing
targets, unsupported shapes or policies, disallowed faces, incompatible hosts,
occupied sockets/cells, exact collision blockers, invalid plans, capacity, and
history failures. Material, authored-asset, and scatter erasers all route through
semantic deletion: pattern output removes its recipe closure, direct World
Layout output says `Edit generated source`, external references say
`Referenced elsewhere`, and prefab roots remove their complete child hierarchy
atomically. Final voxel meshes retain both exact material identity and their
independent floor/wall/prop geometry role, so optimization cannot merge distinct
materials merely because their role matches.

Eleven focused placement, compatibility, interaction, scatter, authored-asset,
room-bake, static-mesh, frame-input, command-recording, and vertex-format tests
plus the application build pass. P1 remains M2 until its shared manual MacBook
touchpad and PS5 controller acceptance pass is performed; no automated evidence
is being relabeled as that manual product gate.

### P2. Catalog Asset Placement - Completed M2 Product Slice

- [x] Provide searchable categories, thumbnails, dimensions, pivot, collision,
  sockets, material variants, and missing-asset status before placement.
- [x] Support grid, floor, wall, surface-normal, and free placement modes.
- [x] Preview exact imported mesh bounds and collision rather than a generic box.
- [x] Place an instance with stable asset/version identity and editable transform.
- [x] Support replace/update while preserving authored placement and overrides.

### P3. Attachment And Socket Placement - Completed M2 Product Slice

- [x] Display compatible sockets, orientation, occupancy, and rejection reason.
- [x] Define best-match selection, manual socket choice, detach, and reattach.
- [x] Preserve hierarchy and local transform through save/load and asset update.
- [x] Validate collision and avoid silently snapping through walls or hosts.

### P4. Asset Scatter - Completed M2 Product Slice

- [x] Preview every accepted/rejected candidate with density, spacing, yaw,
  scale, slope, collision, and deterministic seed controls.
- [x] Support paint, erase, regenerate, mask, exclusion, and selection filtering.
- [x] Store one editable scatter recipe rather than only unrelated instances.
- [x] Permit bake-to-instances explicitly when the user wants destructive output.
- [x] Prove large scatters stay responsive and rebuild only their dirty region.

Scatter output is now one versioned, saveable recipe with bounded paint centers,
exclusions, selection filters, deterministic seed, asset identity, and all
placement parameters. The viewport distinguishes admitted candidates from
density, spacing, mask, exclusion, slope, collision, occupancy, and capacity
rejections. A held stroke previews and plans only its pending region; 300
unchanged frames reuse that result. Paint appends only new outputs, erase removes
only the targeted output, survivor IDs remain stable, and release records one
history entry. Full rebuilding occurs only through explicit Regenerate; Bake to
Instances detaches the recipe while retaining its authored objects.

### P5. Prefab And Building-Template Placement - Completed M2 Product Slice

- [x] Treat Estate House and later templates as catalog recipes, not unique tools.
- [x] Show footprint, entrances, levels, terrain impact, and conflicts before
  placement.
- [x] Retain template ID/version and expose parameters and local overrides.
- [x] Refresh, detach, or upgrade instances explicitly without deleting refinements.

Estate House is installed into the same durable building-template library as
captured buildings and uses the generic catalog placement path. Placement now
reports the exact transformed envelope, footprint, exterior entrance count,
level count, content-fingerprint version, terrain grounding impact, and first
building conflict before commit. Blocked candidates stay positioned and render
as invalid previews; overflow, missing terrain, excessive relief, and occupied
envelopes fail without mutating the layout. Terrain preparation is cached for
the placement session and stages pending World Layout terrain through the real
compiler plan, so foundation preview and final generation use the same surface.
Instances retain template id, source fingerprint, anchor, orientation, and a
local-content fingerprint. The Inspector exposes current, source-changed,
locally-modified, conflict, source-missing, and unlinked states. Updating the
template from a selected refinement, upgrading only safe instances, explicitly
forcing a destructive upgrade, and detaching while preserving geometry and
stable identity are separate commands. Placement, sync, persistence, map
template, editor, and dispatcher tests pin these contracts.

## Voxel And Shape Construction Tools

### V1. Material Brush - Current M1 - Finish Later

- [ ] Support continuous size input, clear shape/axis/fill/falloff/symmetry/mask
  controls, and an exact affected-cell preview.
- [ ] Define paint versus geometry creation semantics unambiguously.
- [ ] Add surface-only, replace-filtered, connected, and depth-limited modes
  through shared settings rather than separate hidden behaviors.
- [ ] Keep one stroke per undo record and remain responsive at maximum radius.

### V2. Volume Selection - M2 Foundation Complete

- [x] Support two-corner, face-expand, move/resize handles, numeric bounds, and
  selection from existing content.
- [x] Show inclusive/exclusive bounds and exact affected counts.
- [x] Preserve a reusable named region while the user switches volume operations.
- [x] Work in all orthographic planes and the 3D viewport.

Milestone note (2026-07-21): one canonical `CreativeVolumeSelection` now owns
min-inclusive/max-exclusive grid bounds, overflow-safe axis moves and face
resizes, existing-object fitting, and exact operation preflight. The editor
retains a named region across compatible volume tools and caches preview receipts
by document identity, revision, and complete operation request. The Inspector
provides numeric bounds, dimensions, cell count, existing-selection fitting,
face expansion, axis nudging, and exact changed-member counts. The 3D viewport
projects six face handles and three move axes from the same world-space region;
visible shafts are pickable with the center crosshair, movement snaps by whole
cells, and cancellation or modal/focus interruption restores the prior region.
Projection tests cover front, plan, side, and 3D views while keeping terrain
region behavior and existing volume-outline counts unchanged. This completes
the shared M2 selection foundation for V3-V7; manual touchpad/PS5 acceptance and
final visual polish remain part of the universal M3 gate.

### V3. Volume Fill - Current M1 - Finish After V2

- [x] Preview exact solid shape and material before commit.
- [x] Support box, ellipsoid, cylinder, line, arbitrary bounds, and orientation.
- [x] Define overlap/merge/replace policy and maximum generated count.
- [x] Leave normally selectable output or an explicit editable shape recipe.

V3 completion note: Fill now uses one bounded shape planner for box,
ellipsoid, line, and X/Y/Z cylinders over arbitrary signed grid bounds. The
three-action gesture records corner 1, freezes an exact staged-document scene,
then applies that same result as one history transaction. Preserve Existing and
Replace Existing are explicit settings; Replace also migrates tagged legacy
volume cells atomically. The editor rejects plans above 512 affected cells.
The staged scene uses the normal room bake and selected material, does not
mutate the source document, and reuses both operation and scene caches for 300
unchanged frames. Output is intentionally voxel-native rather than a false
parametric recipe: the completed region remains selected and the result is
immediately revisable through Volume Select, Material Brush, Replace, and
Erase. Headless coverage lives in `creative_volume_tests`,
`creative_tools_tests`, and `creative_editor_placement_tests`; live touchpad
and PS5 acceptance remains part of the universal M3 gate.

### V4. Volume Hollow - Current M1 - Finish After V2

- [x] Expose shell thickness, inward/outward alignment, openings, and corner rules.
- [x] Reject dimensions that cannot contain the requested shell.
- [x] Preview interior and exterior bounds and preserve editable parameters.

V4 completion note: Hollow now shares Fill's bounded shape planner and exact
staged-document preview while adding explicit 1/2/4-cell shell thickness,
Inward/Outward alignment, axis-relative end openings, and framed or full-cut
corner behavior. Inward selection denotes the exterior; Outward selection
denotes the cavity and uses checked expansion. Lines, undersized inward shells,
invalid enum values, coordinate overflow, and plans above the interactive
512-cell limit reject before mutation with stable reason codes. The viewport
draws independently counted exterior and interior envelopes from the same
bounds used by commit. Every Hollow parameter participates in the operation
preview cache key, survives in request/receipt facts and held-tool status, and
commits as one undoable voxel transaction. Output remains intentionally
voxel-native and revisable through the retained region and existing volume or
material tools. Headless coverage lives in `creative_shape_brush_tests`,
`creative_volume_tests`, `creative_tools_tests`, and
`creative_editor_placement_tests`; live touchpad/PS5 acceptance and final visual
polish remain part of the universal M3 gate.

### V5. Volume Replace - Current M1 - Finish After V2

- [x] Expose source-material/object filters, target material, include/exclude
  masks, and replacement counts.
- [x] Preview changed versus unchanged members.
- [x] Preserve object identity where only material replacement is intended.

V5 completion note: Replace now owns an explicit target material, Any or exact
source-kind filter, and Voxels/Objects/Both member mask. Its bounded preflight
classifies matched, changed, unchanged, and excluded members in both storage
domains and rejects against the complete matched-member count before mutation.
The viewport draws exact changed members in mint and unchanged members in gray
inside the operation-colored selection envelope; all filter values participate
in the preview cache key and appear in held-tool status and operation logs.
Document-object replacement is deliberately restricted to exact one-cell
objects carrying the legacy volume-cell tag. It changes their material kind in
place, preserving ID, name, transform, bounds, layer, visibility, tags, and
other metadata; ordinary props, recipe children, relationships, and locked
objects cannot be recolored through this tool. Mixed object and voxel changes
advance the document once and produce one undo record. Headless coverage lives
in `creative_volume_tests`, `creative_tools_tests`, and
`creative_editor_placement_tests`; live touchpad/PS5 acceptance and final visual
polish remain part of the universal M3 gate.

### V6. Volume Erase - Current M1 - Finish After V2

- [x] Preview exact deletion and dependent-source consequences.
- [x] Prevent generated-child deletion from corrupting its owning recipe.
- [x] Support mask/filter and one-step restoration through undo.

V6 completion note: Erase now owns an independent Any or exact source-kind
filter plus Voxels/Objects/Both member mask. Its bounded preflight classifies
the complete contained set before mutation and publishes exact voxel cells and
object IDs that will be removed. World Layout generated output and every member
owned by a Pattern recipe are protected rather than silently detached; their
amber outlines remain in place, while related pattern sources are exposed in
yellow as dependency facts. Ordinary eligible members render red. Locked
objects, parents with children outside the region, and logic links crossing the
region reject the entire operation before mutation and expose both the blocked
member and external dependency.

Mixed object and voxel deletion stages against an unchanged document and uses
the shared staged-document commit boundary, so one user action publishes one
document revision and one undo record. Undo restores both storage domains and
leaves protected semantic sources untouched. Filter/mask values participate in
the preview cache key and appear in Tool Options, held-tool status, HUD text,
and operation logs. Headless coverage lives in `creative_volume_tests`,
`creative_tools_tests`, `creative_interaction_tests`, and
`creative_editor_placement_tests`; live touchpad/PS5 acceptance and final visual
polish remain part of the universal M3 gate.

### V7. Volume Clone - Current M1 - Finish After V2

- [x] Provide a movable preview with axis, offset, rotation, mirror, and overlap
  policy.
- [x] Remap hierarchy and references deterministically.
- [x] Preserve recipe identity where cloning a parametric source is supported.

V7 completion note: Clone now owns six signed offset directions, positive
distance, quarter-turn rotation, X/Z mirror state, Voxels/Objects/Both member
mask, and Reject/Preserve/Replace voxel-overlap policy. The bounded preflight
expands a selected Pattern member to its complete source-and-generated closure,
then transforms exact voxel cells and document objects around the region's
minimum anchor. Its staged document is also the scene preview source, so the
viewport shows actual target geometry before apply: changed members in mint,
unchanged preserved voxels in gray, protected generated members in amber,
dependent sources in yellow, and blockers in red. Every setting participates in
the preview cache key and appears in Tool Options, held-tool status, HUD text,
and operation logs.

Apply deterministically remaps hierarchy parents, internal logic links, and
Pattern recipe source/generated IDs while detaching external parents. A
hierarchy child or logic link crossing the cloned closure rejects the complete
operation rather than leaving a dangling relationship. World Layout output is
protected from direct cloning. Pattern recipes preserve editable identity for
translation clones; quarter-turn or mirrored Pattern clones reject explicitly
because the current recipe schema cannot represent those transforms without
flattening the relationship. Mixed object and voxel clones publish one document
revision and one undo record. Headless coverage lives in
`creative_volume_tests`, `creative_tools_tests`,
`creative_interaction_tests`, and `creative_editor_placement_tests`; live
touchpad/PS5 acceptance and final visual polish remain part of the universal M3
gate.

### V8. Connected Fill - Current M2 Kernel, M1 Product - Finish Later

- [ ] Show connectivity rule, boundary, source filter, limit, and affected count.
- [ ] Support paint and erase without ambiguous primary/secondary behavior.
- [ ] Explain capacity rejection before commit and avoid partial fills.

### V9. Surface Extrude And Inset - Current M2 Kernel, M1 Product - Finish Later

- [ ] Select and preview the exact connected face set and normal direction.
- [ ] Expose signed depth, inset boundary behavior, material, limits, and collision.
- [ ] Handle corners, holes, thin regions, non-manifold boundaries, and zero-area
  results explicitly.
- [ ] Preserve an editable operation when applied to authored-object geometry;
  destructive voxel use may remain explicitly destructive.

### V10. Linear And Radial Array - M2 Stable Recipe, M1 Product

- [x] Preview count, spacing, axis, sweep, pivot, collision, and final bounds.
- [x] Store one editable array relationship with source identity.
- [x] Permit reselecting the array to change count/spacing without rebuilding it
  manually.
- [x] Support explicit bake/detach to independent instances.
- [x] Bound instance count and avoid full-scene rebuild on parameter adjustment.

Milestone note (2026-07-21): `CreativePatternRecipeStore` now preserves stable
linear/radial relationship IDs, source IDs, generated IDs, and exact parameters
through save/load. Selecting a generated member reopens the relationship;
Apply replaces only its owned generated hierarchy in one history step, while
Detach removes only provenance and keeps the baked copies. The bounded preview
receipt is shared by rendering and headless tests, reads the live options draft,
uses the placement broadphase plus strict OBB overlap checks, colors blocked
copies red, and reports aggregate final bounds without changing document,
history, collision-cache, or scene-cache keys. Collision remains advisory to
preserve the existing overlap workflow. V10 remains M1 as a product until its
full keyboard/touchpad/PS5 workflow and cross-tool interoperability receive
manual acceptance; no separate 2D array frontend currently exists.

## Terrain Tools

### T1. Terrain Rod - Shelved M1 - Deferred

Rods remain a possible low-level control-point editor, not the primary terrain
workflow.

- [x] Hide from the default catalog until a control-point workflow is chosen.
- [ ] If retained, show only the nearest active layer and remove guides after an
  explicit bake only when durable source ownership has moved elsewhere.
- [ ] Provide height, radius, falloff, move, multi-select, numeric editing, and
  contour context.
- [ ] Define whether rods are durable terrain source data or temporary handles;
  never support both meanings simultaneously.

T1 deferral note: the existing low-level terrain-control kernel remains
available only through Experimental surfaces. Its canonical descriptor is M1,
is not default-visible, is not default-wheel eligible, and projects the same
experimental status into the catalog. The remaining control-point, guide
lifecycle, and durable-source decisions stay intentionally unresolved; no new
terrain feature may depend on rods until one meaning is chosen.

### T2. Terrain Paint - Current M1 - Finish Next

- [x] Paint terrain material/layer without changing geometry unless selected.
- [x] Provide radius, hardness, opacity, source filter, blend, and layer preview.
- [x] Support erase/restore, masks, slope/height filters, and material sampling.
- [x] Preserve bounded layer data through save/load and rendering.

Completed with one bounded four-layer material field whose byte weights always
sum to 255, deterministic Replace/Additive quantization, exact per-cell preview
swatches, shared Brush/Connected/Region planning, grouped history, dominant-
layer sampling, sparse Grass restoration, blended renderer tint, and save
section v18 with v17 one-hot compatibility. Focused terrain paint, editor,
renderer, save-section, and save/load tests pin geometry reuse, filters,
rounding, invalid-input atomicity, persistence, and Vulkan CPU vertex color.

### T3. Terrain Grade - Current M2 Product - Stable Recipe Complete

- [x] Support start/end handles, target elevations, width, cross-slope, falloff,
  and live slope/walkability readout.
- [x] Reopen and edit the grade as a durable terrain recipe.
- [x] Integrate with roads, paths, building pads, and bridge approaches.
- [x] Prove player traversal on the generated collision surface.

Milestone note (2026-07-21): Terrain Grade is now a typed, versioned operation
in the bounded terrain stack rather than a destructive edit inferred from rod
anchors. Start and end handles reopen the same stable operation ID; Square
cycles endpoint elevation, width, cross-slope, and falloff controls; preview
and commit consume the same immutable mutation plan; Apply creates one history
entry; and save section v19 preserves the source recipe. The live readout uses
the actual rendered collision triangles and the runtime slope policy.

Path, building-pad, and two-ended bridge adapters translate their domain
requests into the shared Grade recipe with bounded validation and overflow
failure. World Layout now consumes those contracts without adding a hidden
second terrain owner: the durable Path remains authoritative. Building-pad
joins require one exact exterior-to-perimeter terminal segment. Bridge joins
require two exact, opposing terminal segments whose geometry, deck elevation,
width, signed cross-slope, and falloff equal the bridge adapter's recipes.
Curved or profiled terminal segments reject rather than approximating a seam.

A committed Grade operation has also been baked through the room surface path
and crossed end-to-end by the real player movement system. That proof exposed
and removed artificial 45-degree terrain faces: render-patch centers use the
geometric mean of their four crack-free shared corners instead of creating a
spike at each integer column transition. The current descriptor reports Grade
as an experimental M2 parametric tool, and the user guide now describes the
live handle/operation workflow rather than the retired rod-stamping prototype.
The application and focused terrain, room-bake, preview, world-layout, codec,
save, and movement tests pass.

This remains M2 rather than M3/M4 until universal discovery, complete
keyboard/touchpad/PS5 operation, stress and visual-regression coverage, and
manual acceptance are satisfied. The recipe and site-work integration contract
are complete; repo-wide product certification is not.

### T4. Terrain Sculpt - Current M2 Product - Destructive Contract Complete

- [x] Complete Raise, Lower, Flatten, Smooth, radius, strength, falloff, mask,
  and target-height controls with direct and numeric interaction.
- [x] Add local dirty-region updates and high-radius performance proof.
- [x] Make the destructive nature explicit or retain a bounded editable sculpt
  operation stack.
- [x] Display contours and slope classification during sculpting.

Milestone note (2026-07-21): Terrain Sculpt now owns one deterministic bounded
plan for Circle and Square Raise/Lower/Flatten/Smooth edits, including numeric
target height, fixed-point falloff, atomic overflow/capacity failure, and an
exact dirty patch region. Preview and commit consume that plan; the regional
preview is bit-exact with the equivalent full-render subset and the maximum
256-control/radius-16 proof bounds work to 2,500 candidate patches and 2,704
sampled support coordinates. Candidate contours and runtime slope classes are
shown before commit. One gesture remains one lazy history record. The descriptor
now states the honest `Destructive` lifecycle and keeps the M2 tool experimental:
it is not M3 until shared touchpad/PS5 discovery, visual-regression, and complete
cross-engine workflow gates are satisfied.

### T5. Terrain Profile - Current M2 Product - Stable Recipe Complete

- [x] Preview and edit hill, basin, ring, crater, ridge, wave, and ripple with
  handles for radius, amplitude, direction, frequency, spacing, and base.
- [x] Store a durable profile recipe with seed and blend policy.
- [x] Show invalid mathematical combinations before commit.
- [x] Support move, duplicate, reorder, enable/disable, and bake.

Milestone note (2026-07-21): Terrain Profile now authors one versioned dense
`Profile` operation shared by preview, commit, operation replay, save/load, and
the scene cache. Hill, Basin, Ring, Crater, Ridge, Wave, and Ripple retain exact
numeric base/radius/amplitude/direction/frequency/spacing/seed controls rather
than preset-only values. The viewport shows the live bounded footprint, center
handle, slope-classified candidate surface, current selected control, and the
exact rejection reason before mutation; Touchpad reopens a stored manual center
or locks its derived base, and D-pad/Square quick editing remains bounded.

Preview owns the exact mutation plan later submitted through `Facade`, accepted
adds and updates preserve one stable operation identity and one undo record, and
the generic Operations panel now selects, duplicates, reorders, enables,
disables, deletes, and bakes profile sources. Save format 25 carries every
recipe field. Terrain scene caching uses stable height/material content hashes
on document changes, preventing replayed local-revision collisions while still
reusing idle and object-only frames.

The tool remains Experimental M2 rather than M3/M4 until the universal
discovery, complete remappable keyboard/touchpad/PS5 workflow, visual-regression,
stress, and manual acceptance gates are satisfied. The durable recipe and
product-editing contract are complete; repo-wide product certification is not.

### T6. Terrain Path - Current M2 Product - Site-Work Contract Complete

- [x] Replace the two-point drafting shortcut with editable multi-point splines.
- [x] Add point insert/delete/reorder, tangent or curve policy, width profile,
  elevation mode, bank/cross-section, and endpoint joins.
- [x] Store one durable Road/River/Ridge/Trench recipe.
- [x] Recompute only affected segments and expose generated control count.
- [x] Integrate intersections, bridges, terrain materials, collision, and nav.

Milestone note (2026-07-21): Terrain Path now has one versioned durable source
recipe shared by the 3D editor, World Layout drafting, preview, commit,
operation-stack replay, save/load, and generated-source reconciliation. Stable
point ids survive insertion, deletion, and reordering; Linear and Catmull-Rom
curves carry per-point elevation, width, amplitude, and bank into explicit
cross-sections, falloff, materials, and Open/Blend/Intersection/Bridge endpoint
joins. Both authoring surfaces provide exact transient 3D preview and commit one
source plus one undo record rather than destructive per-cell edits.

The preview path now retains validated raw samples per source segment. A middle
point edit rebuilds only the Catmull tangent-adjacent interval, reuses unaffected
segments by stable endpoint ids, and publishes generated-control, rebuilt, and
reused counts through the path and operation replay receipts. Tests compare the
incremental dense height field, material edits, and segment receipts against a
clean rebuild; allocator-only changes reuse every segment, and failed output
composition preserves the last good cache atomically. World Layout validates
matching intersection heights and real bridge-footprint endpoints, while room
bake, reachability, reasoning, and real player movement consume the generated
surface. The application and eleven focused path, operation, editor, layout,
codec/save, room-bake, and reasoning tests pass.

This remains M2 rather than M3/M4 until the universal discovery, complete
keyboard/touchpad/PS5 workflow, stress, visual-regression, and manual acceptance
gates are satisfied; the site-work recipe and engine-integration contract is
complete, not the repo-wide product certification.

### T7. Terrain Region - Current M2 - Product Gates Pending

- [x] Unify 3D region and 2D drafting selection over one recipe.
- [x] Complete move/resize handles, numeric bounds, rectangle/ellipse masks,
  feather, operation options, and exact preview.
- [x] Reopen applied regions in an operation stack instead of losing parameters.
- [x] Define overlap ordering and conflict behavior with other terrain recipes.

Milestone note (2026-07-21): 2D drafting and the in-world 3D tool now author
one `CreativeTerrainRegionRecipe`; no parallel UI recipe remains. Raise, Lower,
Flatten, Smooth, Noise, and Erase share bounds, rectangle/ellipse masking,
feather, deterministic seed/scale, exact preview, durable apply, save/load, and
undo. The plan canvas owns numeric bounds plus edge, corner, and body handles;
the 3D surface reuses the canonical volume selection and option model. Both
surfaces reopen a stable operation id and update it instead of baking a second
anonymous result.

Operation replay is authoritative and ordered: later enabled operations compose
over earlier ones. In-world picking searches that order in reverse, respects
the actual mask footprint, and may edit only enabled Manual regions; disabled
or World Layout-owned sources are never detached by the generic tool. Focused
tests prove cross-frontend recipe and preview parity, overlap fallthrough,
stable-id updates, one-record undo, schema persistence, room bake, collision,
and reasoning-route compatibility.

This remains M2 rather than M3/M4 until universal discovery, complete
keyboard/touchpad/PS5 operation, stress and visual-regression coverage, manual
acceptance, and the full road/river/bridge corridor workflow are complete.

### T8. Terrain Stamp - Current M2 Stable Recipe + Durable Catalog

- [x] Save a selected terrain region as a named reusable stamp.
- [x] Preview rotation, mirror, height mode, merge/replace, and surface alignment.
- [x] Retain source identity and version or explicitly bake the result.
- [x] Add catalog thumbnail, dimensions, compatibility, and missing-source repair.

Milestone note (2026-07-22): exact composed height/material regions now save as
bounded `.igts` assets with stable identity, validated binary persistence,
deterministic thumbnails, dimensions, source-drift diagnostics, and explicit
repair from the baked payload retained by every `Stamp` operation. Placement
rotation, both mirrors, Surface/Absolute elevation, signed offset, Merge/Replace,
exact preview/commit parity, one-record history, operation duplication, save v24
round-trip, and explicit Bake All are pinned headlessly. The capability remains
M2 until the universal M3 gates, especially complete catalog operation across
every supported input surface and manual visual acceptance, are satisfied.

### T9. Procedural Terrain Generator - Current M2 - Product Gates Pending

- [x] Make seed, bounds, scale, relief, masks, biome/material intent, and output
  ownership explicit.
- [x] Preview without replacing authored terrain; apply through a durable recipe.
- [x] Support regenerate, lock selected regions, blend with authored edits, and
  detach/bake deliberately.
- [x] Prove deterministic output and bounded generation/upload time.

Milestone note (2026-07-22): the ordered terrain-operation stack now owns one
versioned slope-damped FBM recipe with explicit seed, finite bounds, base,
relief, horizontal scale, octave count, persistence, lacunarity, slope damping,
mask, blend mode, biome intent, low/high materials, transition height, and an
explicit height-only/material-output switch. Named biome presets write concrete
material fields; Custom preserves manual values. Legacy stacks migrate as
height-only so opening an old map cannot repaint it.

Preview and Apply consume the same immutable replay result, including exact
height and material fields. The staged scene cache keys both hashes, so a
material-only change recolors the preview while 300 unchanged frames add zero
scene builds. Apply creates one durable operation and one undo record. Regenerate
advances the seed deliberately. Up to 16 rectangle/ellipse protected regions
preserve authored height and material together during composition. `Bake Terrain
Stack` is an explicit destructive, undoable operation that retains the exact
current result while removing all parametric provenance; no misleading
single-layer detach is offered.

The generator is capped at 8,192 cells and eight octaves, exposing exact cell,
octave, material, protected-cell, and modified-cell counts. Focused tests prove
deterministic height/material hashes, invalid-input and capacity failure,
protected-region behavior, old-save migration, save/load, preview/apply parity,
one-build scene invalidation, dispatcher history, and exact bake undo/redo.

This remains M2 rather than M3/M4 until it is discoverable through the common
tool model, has complete keyboard/touchpad/PS5 operation, visual regression and
stress fixtures, manual acceptance, and downstream gameplay/nav validation for
large generated maps.

### T10. Contours And Elevation Editing - Current M2 - Product Gates Pending

- [x] Render configurable contour intervals, index contours, elevation labels,
  slope bands, and cut/fill feedback in plan view.
- [x] Select a contour or height handle and edit terrain through existing terrain
  recipes rather than directly moving render lines.
- [x] Synchronize contour edits with 3D preview and collision walkability.

Milestone note (2026-07-21): one bounded `CreativeTerrainAnalysisPlan` now
derives configurable minor/index contours, deterministic index labels, local
gradient and slope bands, and signed cut/fill cells from canonical composed
terrain. Plan drafting resolves contour clicks and explicit height-handle clicks
through typed hit modes into the existing durable `CreativeTerrainRegionRecipe`;
the canvas never mutates a rendered line or height patch directly. Drag-region
editing and its move/resize handles remain available beside the analysis targets.

Candidate terrain is compared against committed terrain for cut/fill, reused by
the 2D analysis cache, and supplied to the existing 3D contour overlay. Apply
preserves that exact candidate in the ordered operation stack. Focused proofs
cover deterministic labels and gradients, holes and capacity failure, explicit
contour/height precedence, shared recipe adoption, exact preview/apply parity,
3D contour parity, and room-baked walkable collision matching the same smoothed
render patch before and after the edit.

This remains M2 rather than M3/M4 until universal discovery, complete
keyboard/touchpad/PS5 operation, stress and visual-regression coverage, manual
acceptance, and the complete site-work corridor workflow are satisfied.

### T11. Hydrology And Erosion - Missing - Deferred

- [ ] Research and define drainage, flow accumulation, river routing, erosion,
  deposition, and water-surface ownership before implementation.
- [ ] Keep algorithms deterministic, bounded, seedable, and non-destructive.
- [ ] Require explicit bake/update behavior and visual/collision/nav contracts.

## Building And Floor-Plan Tools

### B1. Building Blockout And Template - Current M2 - Product Gates Pending

- [x] Replace the four-pattern/four-room ceiling with an arbitrary editable floor
  plan and explicit capacity behavior.
- [x] Configure footprint, storeys, floor-to-floor height, wall profile, slab,
  roof, entrance, windows, room connections, and style before generation.
- [x] Preview the complete building in synchronized 2D/elevation/3D views.
- [x] Retain one BuildingRecipe with source provenance and generated ownership.
- [x] Reopen, modify, regenerate, undo, save/load, and preserve refinements.

Milestone note (2026-07-21): the four blockout patterns are starter layouts,
not a room-count ceiling. Any accepted starter opens into the canonical
orthogonal topology editor; focused proof splits a Grid2x2 source into a fifth
room while retaining shared-edge ownership and exact 3D generation. The create
surface now configures physical-grid-aware Residential, Grand, or Custom
floor-to-floor profiles; floor, ceiling, and roof layers; entrance and window
generation; room connections and vertical circulation; roof form; and separate
exterior/interior structural materials before staging.

Blockout recipe version 3 owns those choices, provenance, fingerprinting,
generated ownership, exact preview, regeneration, and v1/v2 migration defaults.
Plan projection, elevation projection, and the exact generated-document preview
all consume the same world-layout source. A dispatcher-level workflow proves
create, preview, confirm, semantic refinement, generated-output adoption,
three-way conflict resolution, undo, two save/reopen boundaries, regeneration,
detach, and physical traversal without losing accepted refinements. This is M2,
not M3, until the universal discovery, input-parity, stress, visual-regression,
and manual product gates are satisfied.

### B2. Building Shell - Folded Into B1 - Current M2

- [x] Treat shell as a building recipe component, not a disconnected box tool.
- [x] Generate one coherent exterior wall envelope per facade/storey range where
  appropriate, with explicit corners, thickness, base, top, and material.
- [x] Support footprint edits and regenerate dependent facades/openings.
- [x] Keep exterior and interior wall ownership distinct.

Milestone note (2026-07-21): shell generation is now a component of the
building blockout recipe. Compatible exterior runs merge vertically into one
facade envelope with one base/top/thickness/material contract, while partitions
remain level-owned. Upper-storey opening cutouts are rebased into the merged
facade frame. Legacy blockouts materialize the recipe's exterior/interior styles
when they enter canonical topology; explicit topology and later user refinements
remain authoritative. Footprint and room-boundary edits rebuild dependent
facades and hosted openings transactionally, with focused multi-storey facade,
material, opening-offset, history, preview, and persistence coverage.

### B3. Room And Floor Plan - Current M1 - Finish Now

- [x] Draw arbitrary orthogonal rooms first; define non-orthogonal support later.
- [x] Provide vertex/edge/room handles, dimensions, adjacency, naming, room type,
  floor/ceiling elevation, and overlap diagnostics.
- [x] Split/merge rooms and move shared boundaries without duplicating walls.
- [x] Generate floors, ceilings, partitions, and openings from one topology graph.
- [x] Make 3D BuildingRoom and 2D Room invoke this same recipe.

Milestone status: the canonical orthogonal topology recipe and both editor
frontends are implemented. This is an M2 stable recipe, not yet an M3 product
tool; the universal input, stress, visual, and manual acceptance gates still
apply.

### B4. Levels, Floors, And Ceilings - Partial - Finish Now

- [x] Add a level/storey manager with reorder, duplicate, height, visibility,
  active level, and lower/upper context controls.
- [x] Define floor top, slab thickness, ceiling height, and inter-storey ownership
  once, in physical meters/grid units.
- [x] Render full slab thickness and preserve walkable collision surfaces.
- [x] Keep each storey's ceiling/floor relationship consistent by default while
  allowing explicit exceptions.

Milestone status: atomic level operations, context visibility, physical slab
ownership, generated output, persistence, and history are implemented. This
remains M2 until the universal product-tool gates are satisfied.

### B5. Walls And Partitions - Current M1 - Finish Now

- [x] Represent walls as relational segments between topology vertices, not
  unrelated generated boxes.
- [x] Support exterior/interior profiles, height, thickness, material, joins,
  corners, curved-wall deferral, and direct length editing.
- [x] Move/split/merge walls while preserving hosted doors and windows.
- [x] Generate visible wall thickness, correct collision, and opening cuts.

Milestone status: cardinal straight walls have one stable topology-edge identity
through 2D/3D selection, preview, commit, history, save/load, templates,
whole-building transforms, room bake, collision, and rendering. This is M2,
not M3, until the universal product-tool gates are satisfied.

Curved walls are explicitly deferred. They require a canonical line/arc curve
primitive, endpoint and tangent ownership, offset/join tessellation, arc-length
opening coordinates, split/merge and transform laws, exact plan projection,
visible mesh/collision parity, persistence migration, and performance bounds.
Do not emulate a curve with hidden unrelated straight walls or assign one
stable identity to an opaque segment chain.

### B6. Openings - Partial Infrastructure - Finish With Doors/Windows

- [x] Define one hosted-opening contract for wall edge, offset, width, height,
  sill, lintel, insert asset, facing, and clearance.
- [x] Slot an opening anywhere valid on an existing wall without manual cleanup.
- [x] Preserve attachment when the host wall moves or resizes.
- [x] Diagnose overlaps, edge clearance, missing host, and movement obstruction.

Milestone status: hosted openings now resolve one canonical explicit-wall or
topology-edge frame, validate cutout/insert/facing/clearance semantics, retain
attachment through wall and room operations, round-trip versioned source, and
project their front side in plan view. Player-envelope failures feed the
creator diagnostics. This is M2 infrastructure for B7/B8, not a claim that
interactive doors or complete window gameplay are finished.

### B7. Doors - Current M2 - Product Gates Pending

- [x] Produce a real wall cut plus frame, leaf, hinge, handle, and collision state.
- [x] Support left/right hinge, inward/outward swing, width/height, single/double,
  locked/open/closed variants, and procedural fallback.
- [x] Preview swing clearance and reject collisions with walls or critical paths.
- [x] Integrate interaction logic, navigation portals, sound/LOS behavior, save
  state, and runtime animation.

Milestone status: hosted door openings now compile through one bounded procedural
recipe into a real cut, frame, one or two leaf assemblies, hinges, handles,
render geometry, and collision. Canonical hinge/swing/initial/lock/transition
settings round-trip through world-layout and Creative-document persistence and
participate in recipe fingerprints. Plan projection previews leaf pose and swing
clearance; wall and player-critical-path conflicts fail before commit. Creative
Play owns deterministic fixed-tick animation with grouped leaves, tight live
targeting bounds, occupancy-safe closing, lock feedback, transition sound, and
atomic collision/LOS/reasoning rebuilds. The Play sandbox remains intentionally
ephemeral; the durable state is the authored initial/runtime configuration. This
remains M2 until the universal discovery, input-parity, stress, visual-regression,
and manual keyboard/touchpad/PS5 product gates are satisfied.

### B8. Windows - Current M2 - Product Gates Pending

- [x] Produce a real wall cut plus sill, lintel, frame, glazing/shutter insert,
  width, height, and sill elevation.
- [x] Preserve host relationship through wall edits.
- [x] Integrate collision, LOS, cover behavior, and missing-asset fallback.
- [ ] Add breakable glazing and openable shutter state after Creative Play owns
  a durable window-damage and shutter-interaction contract.

Milestone status: hosted windows now compile through one bounded procedural
recipe into a real wall cut, four-part frame, inset glazing, or paired shutter
assembly. Window treatment round-trips through world-layout and Creative-
document persistence, participates in fingerprints, survives wall/building/
template edits, and projects distinctly in plan view. Glazing is physically
blocking but sight-transparent; shutters block actors, projectiles, and sight.
Both treatments retain semantic collision and LOS under missing-asset fallback,
with a repairable missing-metadata diagnostic. Render materials distinguish
glass from shutters. The inserts are intentionally static: Creative Play has no
window damage state or shutter hinge/state owner to reuse, so break/open behavior
is deferred rather than hidden in the recipe or emulated through door state.
This remains M2 until that runtime contract and the universal product-tool gates
are satisfied.

### B9. Stairs - Current M2 - Product Gates Pending

- [x] Generate actual treads, risers, landing, stringer/rail sockets, headroom,
  width, rise, run, direction, and floor cut.
- [x] Fit between selected levels and reject impossible geometry before commit.
- [x] Match visible mesh, compound collision, and player step/clamber policy.
- [x] Produce navigation connectivity and pass an actual motor traversal test.

Milestone status: one fixed-layout stair recipe now owns the bounded step
schedule, landings, rail/stringer sockets, headroom, dimensions, and direction.
World-layout compilation cuts the intervening slabs and emits the same semantic
Stair consumed by preview, generated mesh, compound collision, attachment
targets, room-bake navigation anchors, and reasoning. An authored two-storey
fixture traverses all twelve risers through the real player motor and reaches
the upper landing grounded. The bounded 96-step capacity also covers the
reference estate's deliberate 88-step scale without restoring the old silent
clamp; unsafe tread/run, headroom, transform, and over-capacity requests reject
atomically. This remains M2 until stair discovery, direct plan/elevation/3D
manipulation, refinement controls, keyboard/touchpad/PS5 parity, stress and
visual-regression coverage, and manual product acceptance satisfy the universal
tool gates.

### B10. Ramps - Current M2 - Product Gates Pending

- [x] Generate a visible sloped surface with width, run, rise, landing, side edge,
  and material.
- [x] Show slope angle and walkability while editing.
- [x] Match render, collision normal, movement policy, and navigation.
- [x] Reject excessive slope or insufficient headroom atomically.

Milestone note (2026-07-21): one fixed-layout ramp recipe now owns width, run,
rise, surface length and normal, slope angle against the shared runtime movement
limit, headroom, two landings, two side edges and attachment sockets, and
structural material. World-layout source, schema/codec migration, compiler,
direct and generated-source inspectors, renderer, exact height-patch collision,
attachment snapping, navigation anchors, reasoning nodes, and the gameplay
movement system consume that contract. Focused tests prove a complete authored
ramp traversal and fail-closed slope, headroom, material, and malformed-input
paths. This remains M2 until ramp discovery, direct plan/elevation/3D
manipulation, refinement controls, keyboard/touchpad/PS5 parity, stress and
visual-regression coverage, and manual product acceptance satisfy the universal
tool gates.

### B11. Roofs - Current M2 - Product Gates Pending

- [x] Add flat, shed, gable, and hip roof recipes with ridge direction, pitch,
  overhang, thickness, material, eaves, openings, and drainage sockets.
- [x] Provide plan/elevation/3D handles and exact building-footprint attachment.
- [x] Preserve one editable roof source and generated closure pieces.
- [x] Defer complex intersections/dormers until simple roofs pass M4.

Milestone note (2026-07-21): one fixed-layout structural roof recipe now owns
Flat, Shed, Gable, and Hip geometry; ridge axis or shed downhill direction;
pitch; overhang; thin layer thickness; structural material; perimeter
eave/verge facts; and bounded drainage sockets. World-layout source, schema-19
codec with schema-18 defaults, blockout-provenance recipe version 2 with
version-1 defaults, transforms, compiler ownership, plan projection, elevation
profiles, generated stable keys, material tags, renderer geometry, collision,
and attachment receivers consume that contract. Flat keeps the exact
per-room-surface compatibility path for irregular footprints; authored
rectangular Flat roofs and all pitched styles use the canonical whole-roof
plan. Gable collision follows the visible weather faces and Hip renders four
tapered panels. Hip rendering and collision queries now share one tapered
eight-corner layout, so traversal and segment tests reject the empty ridge-end
space that the old box envelope falsely occupied. Pitched weather faces remain
query-only height patches rather than AABB-baked volumes.

Direct-manipulation milestone note (2026-07-22): one level-owned roof edit
kernel now derives four eave handles and the optional ridge-height handle from
the canonical whole-roof plan. Plan view exposes quarter-cell symmetric
overhang drags on the exact roof perimeter; elevation routes ridge height
through the same rise/run pitch calculation; and the 3D viewport projects and
picks the same five world-space handles at the center reticle. All three views
share one typed Begin/Update/Commit/Cancel session, transient exact generated
preview, repeated-update suppression, stale/topmost-level rejection, and one
source-plus-document history entry on release. Cancel, capture mode, focus
loss, and interaction lifecycle changes discard only the transient preview.
Focused tests pin exact footprint coordinates, plan and 3D picking, elevation
math parity, payload mismatch, cancel, stale revision, undo, overlay gating,
and both desktop-dispatch and direct-viewport commit paths. B11 remains M2
pending the universal touchpad/PS5, stress, visual-regression, and manual
product-acceptance gates; roof-aperture 3D refinement and complex roof
intersections remain separate deferred work.

Roof-aperture milestone note (2026-07-21): schema 20 adds a durable bounded
source record for Skylight and Chimney clearance apertures, with schema-19
loads defaulting to none. One allocation-free planner cuts exact closure pieces
for rectangular Flat, Shed, and Gable roofs, produces a fitted Window insert
for a skylight, leaves a clearance intentionally open, and rejects non-finite,
outside, overlapping, cross-panel, over-capacity, and unrepresentable requests
atomically. Hip apertures fail closed with an explicit polygon-panel-storage
reason instead of approximating the opening. Building move, rotate, duplicate,
delete, template capture/stamp/update, and level duplicate/reorder/delete all
preserve aperture ownership. Plan and elevation projections, hierarchy,
diagnostics, synchronized 2D/3D selection, generated-source scope, source
inspection, save/load, room bake, rendering, and collision consume the same
record and closure plan. Create, edit, and delete each synchronize source and
generated output in one undoable dispatcher transaction. Selected plan
apertures now expose move and eight resize handles with quarter-cell snapping,
exact live 3D preview, red invalid-preview feedback, cancel without mutation,
stale-source rejection, and one source-plus-scene undo entry on release. The
focused seam gate passes 23 tests across recipe, compiler, codec, save,
templates, levels, projection, history, room bake, rendering, and collision.

This remains M2: Hip cutouts, volumetric side/underside and AABB occlusion
semantics, dormers and intersecting roofs, elevation and direct 3D handles,
stress and visual-regression fixtures, touchpad and PS5 parity, and manual
product acceptance are not complete.

### B12. Building Refinement And Validation - Partial - Finish After B1-B11

- [x] Expose structural/topology diagnostics beside direct repair actions.
- [x] Validate entrance, room connectivity, vertical circulation, headroom,
  opening clearance, generated ownership, collision, and actual traversal.
- [x] Allow generated pieces to be intentionally refined with explicit adopt,
  override, detach, or regenerate behavior.
- [x] Prove one multi-storey building can be created, entered, traversed, saved,
  reopened, changed, and regenerated without losing refinements.

Building diagnostics now expose explicit source focus and planner-backed repair
actions. The conservative repair owner can add a valid exterior entrance,
connect one disconnected room through a shared boundary, or fit an undersized
door to the runtime player envelope. Every action revalidates the exact issue,
rejects stale or impossible candidates, synchronizes source and generated 3D in
one history transaction, and remains undoable. Ambiguous roomless, connector,
and ownership failures intentionally remain focus-only instead of fabricating
architecture.

Building generation now also runs a cached physical proof over the exact
generated candidate. It room-bakes only source-owned structural surfaces,
removes complete door-leaf assemblies to test the openable passage, checks a
runtime player body against representative room floor/clearance samples, sweeps
both directions through every door, and drives the real movement system both
ways across every stair and ramp. Physical failures block generation and remain
source-addressable; arbitrary furniture and terrain are intentionally reserved
for whole-map validation rather than being mislabeled as structural defects.
The same work repaired blockout ramp sizing so its run is derived from the
shared runtime slope limit instead of emitting an unbuildable 45-degree ramp.

Generated refinement now has one explicit policy instead of permissive raw
object editing. One-to-one Object and Box outputs may be transformed in 3D and
adopted back into their invertible source. Condensed room, wall, opening,
connector, level, and roof output remains locked to semantic source editing;
the editor does not invent a one-object inverse for a many-piece recipe. When
both source and generated output change, the existing state-bound three-way
review requires an explicit Use 2D or Keep 3D decision. Removed refined output
requires Remove or Detach, while malformed groups require Regenerate or Detach;
no option wins silently, and every accepted resolution is one undoable install.

The product workflow proof creates a two-storey building with an entrance,
stair, and building-owned one-to-one plinth; adopts a representable 3D edit;
keeps a later 3D override through an unrelated building regeneration; saves and
reopens with stable object identity; explicitly regenerates the conflicted
member from 2D; detaches a removed refined member; saves and reopens again; and
runs the real player traversal proof before and after each durable boundary.
The detached object remains authored 3D output, while the building source and
managed structure remain synchronized and traversable.

## Site And Infrastructure Tools

### S1. Plateau, Terrace, And Cliff - Current M2 - Stable Recipe Complete

- [x] Replace the fixed click macro with editable bounds/path, target elevation,
  side slope or retaining edge, feather, material, and erosion policy.
- [x] Add terrace and cliff variants through one terrain recipe family.
- [x] Integrate building pads, roads, collision, contours, and nav.

Milestone note (2026-07-21): one bounded landform recipe now owns Plateau,
Terrace, and Cliff; exact editable tile bounds; base and target elevations;
direction and terrace count; slope or retaining-edge treatment; feather;
optional surface material; deterministic clean or weathered transition policy;
and seed. The original click is only a starter placement. The selected source
can be reopened in the World Layout Inspector, changed through the same recipe,
reconciled against generated output, saved, loaded, removed, and regenerated
without changing its terrain-operation identity.

Landforms replay before road and site-path recipes, while building grounding
now consumes that same staged candidate rather than the previously committed
height field. Intentional terrace and retaining-cliff discontinuities are
stored as canonical hard edges: preview geometry splits the shared vertices and
draws material-matched vertical faces; RoomBake emits exact actor/projectile
blockers; runtime collision queries stop at the face; and the reasoning graph
does not fabricate a walkable cross-edge connection. Sloped variants retain
smooth shared vertices and no wall blocker. Contours and terrain analysis keep
using the same composed height source. Focused proofs cover deterministic
recipe output, capacity/failure behavior, operation ordering and stable
identity, source reconciliation, save migration, staged building grounding,
scene-cache invalidation, render topology, collision, and reasoning.

This remains M2 rather than M3/M4 until universal discovery, direct manipulation
in every relevant view, complete keyboard/touchpad/PS5 operation, stress and
visual-regression coverage, and manual product acceptance are complete.

### S2. Road And Path - Current M2 - Finish After T6

- [x] Use the shared editable terrain-path spline.
- [x] Add width profile, shoulders, camber, grade limits, surface material, curb
  or edge kit, intersections, endpoints, and bridge approaches.
- [x] Retain one road recipe with generated terrain and asset ownership.
- [x] Prove traversal and deterministic intersection generation.

S2 now reuses `CreativeTerrainPathSourceRecipe` as the only durable centerline
and profile source. Per-point half width and bank remain the width/camber
profile; `CreativeTerrainRoadSettings` adds shoulder width, an optional positive
grade cap (`0` deliberately means unlimited for behavior-preserving v1
migration), and a bounded structural curb treatment. The public path sampler is
the exact joined spline consumed by both dense terrain replay and
`CreativeRoadRecipe`, so generated edge members cannot drift onto a second
curve implementation.

World Layout stages the path operation first, then reconciles deterministic
Road-owned curb members against the composed final height field. Intersection,
bridge, and building-pad endpoint weights suppress curb pieces at shared seams;
short joins may intentionally produce no structural member. Save section v28
and World Layout codec/schema v23 retain every road setting, while older roads
upgrade to zero-shoulder, unlimited-grade, no-edge defaults. The World Layout
inspector exposes these settings through the existing source-edit transaction.

Focused proof covers shoulder grading/painting, exact and rejected grade limits,
shared-sampler determinism, generated-member provenance and capacity failure,
join suppression, save and layout migration, stable reconciliation/removal,
RoomBake collision clearance down the travel surface, and a preserved reasoning
edge. `i3dc` and the nine road-adjacent recipe, save, RoomBake, and World Layout
targets build and pass headlessly.

This remains M2 rather than M3/M4 until road discovery and direct manipulation
are available in every relevant 2D/3D view, keyboard/touchpad/PS5 workflows are
complete, and stress plus manual visual acceptance cover long curved networks,
curb kits, intersections, and bridge approaches.

### S3. Ditch, Trench, Riverbed, And Watercourse - Current M2

- [x] Use the shared path spline with cross-section, depth, bank slope, feather,
  material, and drainage direction.
- [x] Separate terrain cut from future water surface and flow semantics.
- [x] Support crossings and bridge attachment without hand alignment.
- [x] Defer simulated water until water rendering/physics has an explicit owner.

S3 extends the shared `CreativeTerrainPathSourceRecipe` rather than introducing a
second spline. Per-point amplitude remains the exact cut depth, falloff remains
the feather, and the existing cross-section and material policies remain the
terrain source of truth. Version 3 adds bounded bank slope, explicit authored
drainage direction, an optional reserved-surface policy, and stable crossings
attached to path-point ids. A zero bank slope and unspecified drainage preserve
all older River and Trench output exactly; newly drafted Ditches opt into a
one-cell bank slope and start-to-end drainage.

`CreativeWatercoursePlan` consumes the same sampled path after the final terrain
operation stack has been staged. It keeps bed samples separate from reserved
future-surface elevations, carries explicit River/Trench, flow-direction, and
surface-policy metadata, owns a deterministic fingerprint, and derives each
crossing's axis, banks, approaches, deck elevation, transform, and span from one
stable control point. World Layout retains these transient plans for S4 and a
future water owner, but does not create anonymous water or bridge objects.

Save section v29 and World Layout codec/schema v24 retain every watercourse and
crossing field. Version 28/v23 paths migrate to inert watercourse defaults. The
World Layout Inspector edits the shared source transaction, removes a crossing
when its host point is deleted, and exposes clearly that a reserved surface is
not rendered or simulated water. Focused proof covers bank/bed geometry,
drainage rejection, reversed flow, exact attachment frames, deterministic
replay, current and legacy persistence, source reconciliation, no-change
rebuilds, editor drafting, and the absence of fabricated water or bridge output.
`i3dc` and eleven adjacent recipe, terrain, save, World Layout, projection,
properties, and editor tests build and pass headlessly.

This remains M2 rather than M3/M4 until discovery and direct manipulation are
complete across relevant 2D/3D views, keyboard/touchpad/PS5 workflows are
accepted manually, stress and visual-regression coverage exercise long curved
watercourses, and water rendering, physics, and simulation receive an explicit
owner. Those systems are deliberately not hidden inside this terrain recipe.

### S4. Bridge - Current M2

- [x] Select or infer both banks/approaches and span a real gap.
- [x] Configure deck width, elevation, supports, rails, material kit, clearance,
  and maximum span.
- [x] Generate actual bridge structure, collision, nav, and approach grade.
- [x] Store one editable bridge recipe attached to a stable path crossing while
  retaining the existing exact road-endpoint seam contract.

S4 keeps legacy free-standing Bridge boxes intact and adds one explicit
parametric mode to the existing World Layout object row. Its durable
`CreativeBridgeSourceRecipe` owns a stable River/Trench path key, crossing id,
deck dimensions and elevation, maximum span and minimum clearance, bounded pier
policy, optional rails, approach-grade limits, falloff, and a structural
material kit. The Bridge canvas tool infers an authored S3 crossing from its
source point; zero crossings retains the legacy box workflow, multiple
crossings reject as ambiguous, and an already claimed crossing rejects so one
stable crossing has one bridge owner.

The pure `planCreativeBridge` kernel consumes S3's canonical crossing frame and
emits one bounded output: a walkable deck, paired supports where required,
optional rails, and exactly two terrain-grade approach recipes. Members reuse
the existing Bridge, Column, and Railing kinds and the shared recipe
materialization/reconciliation path rather than introducing a second geometry
engine. Structure identity remains independently useful for stable member
patching, while the complete bridge receipt fingerprint also covers both
approach recipes. Extreme spans, support counts, grade coordinates, capacity,
clearance, grade, missing attachments, and duplicate ownership fail before
mutation or unsafe numeric conversion.

World Layout owns bridge structures and approach operations under stable source
keys. Crossing derivation removes prior owned approach grades from its
temporary terrain source, preventing self-feedback across rebuilds. Unchanged
sources converge to no change; settings patch retained member ids; disabled
rails are removed; deletion removes all owned members and approach grades.
World Layout codec/schema v25 persists the source and migrates v24 generic
Bridge rows without changing their meaning. The same encoded source survives
atomic world save/open, and the Inspector edits settings through the existing
single source-history transaction while generated placement follows the
crossing.

Focused proof covers deterministic structure and complete-output fingerprints,
source inference and ambiguity, one-owner enforcement, span/clearance/grade and
overflow rejection, material and optional-member policy, codec migration,
atomic persistence, stable reconciliation/removal, RoomBake and physics
collision, a clear center travel lane, and a preserved reasoning-graph edge.
`i3dc` and the nine bridge-adjacent recipe, World Layout, persistence, editor,
and projection tests build and pass headlessly.

This remains M2 rather than M3/M4 until the generated bridge has exact preview
and direct retargeting in every relevant 2D/3D view, discovery and complete
keyboard/touchpad/PS5 workflows are manually accepted, long and diagonal
crossings receive stress and visual-regression coverage, authored bridge asset
kits replace the canonical structural boxes where desired, and road endpoints
can attach directly to the same parametric source instead of only using the
preserved legacy seam validation.

### S5. Retaining Wall And Edge Kit - Current M2 - Stable Recipe Complete

- [x] Add spline or segment placement with height-following, corners, caps,
  stairs/ramps, terrain cut/fill, and asset-kit selection.
- [x] Reuse road/terrain path and attachment contracts rather than creating an
  independent placement engine.

S5 is a segment-derived attachment to one exact bounded S1 landform, not a
second terrain sculptor or freehand wall engine. The landform remains the sole
owner of cut/fill and canonical hard-edge topology. Its optional durable
`CreativeRetainingEdgeSourceRecipe` selects all, internal, or perimeter seams;
configures thickness, maximum height, corners, end caps, structural material,
and a procedural or infrastructure-stone kit; and owns at most 16 exact
stair/ramp seam replacements. This keeps terrain shape, wall structure, and
traversal openings editable from one source without duplicating road/path or
terrain composition rules.

The pure `planCreativeRetainingEdge` kernel consumes the composed height field
and only the final seams owned by that profile's landform operation. It emits
deterministically keyed height-following wall segments, exact procedural corner
and cap posts, and validated Stair/Ramp members. Transition seams omit the wall
rather than layering traversal geometry through it. The infrastructure kit uses
the checked-in retaining-wall and terrain-step assets; exact closure posts stay
procedural so asset fitting cannot distort small joints. Invalid versions,
grids, settings, seam topology, duplicate or unmatched transitions, unsafe
stairs/ramps, excessive height, non-finite values, and the 4,096-member output
capacity fail before mutation.

World Layout derives hard-edge ownership from the final bounded 64-operation
terrain stack: because landforms are the only operations that add hard edges
and each later landform erases touched seams first, a reverse operation scan
assigns every surviving seam to one exact source. This prevents an overlapping
later landform from being decorated by the wrong retaining recipe. Generated
members use normal recipe reconciliation, retain ids through material changes,
and disappear together when disabled. Codec/schema v26 persists the complete
source and migrates v25 landforms with retaining output disabled. Atomic world
save/open preserves the same encoded source.

The existing terrain-profile Inspector edits the attached source through the
typed property/history command, including bounded numeric transition rows.
Plan projection keeps the landform footprint as the owning symbol and marks
each authored transition on its exact shared-cell boundary with the existing
Stair/Ramp drafting roles. Focused proof covers deterministic geometry,
selection policies, corners/caps, both asset kits, stair/ramp replacement,
invalid/capacity behavior, v25 migration, atomic persistence, stable
reconciliation/removal, overlapping-source ownership, RoomBake, actor-blocking
collision, and a preserved reasoning-graph edge through the stair opening.
`i3dc` and the 11 retaining-, terrain-, stair/ramp-, recipe-, World Layout-,
persistence-, editor-, and projection-adjacent tests build and pass headlessly.

This remains M2 rather than M3/M4 until retaining output is discoverable through
the common tool model, seams and transitions have direct manipulation and exact
preview in every relevant 2D/3D view, keyboard/touchpad/PS5 workflows are
manually accepted, long/overlapping sites receive stress and visual-regression
coverage, and authored asset kits cover corners, caps, ramps, and material
families without relying on procedural closure pieces.

## Gameplay Authoring Tools

### G1. Player Spawn - Current M2 - Stable Recipe And Runtime Integration Complete

- [x] Configure facing, player profile, spawn group, validation radius, and
  fallback priority.
- [x] Preview the exact standing envelope, floor contact, clearance, and initial
  camera direction.
- [x] Reject collision, out-of-bounds, unsupported floor, and unreachable starts.
- [x] Prove save/load and game-launch consumption.

Player Spawn is one durable point-object capability rather than a marker that
runtime code reinterprets. `CreativePlayerSpawnSettings` owns the profile,
group, validation radius, and deterministic fallback priority; Rotation Y owns
facing. Direct-object and World Layout inspectors edit those values through
typed mutation/history boundaries. Document creation, object-library recipes,
clipboard, generated World Layout output, document save schema v30, and World
Layout codec/schema v27 preserve the same settings, with explicit migration
defaults for older sources.

`PlayerSpawn` is the sole physical owner. It resolves the baked spawn anchor,
samples complete floor support, grounds the authored point, applies the exact
standing overlap volume and effective clearance radius, checks configured world
bounds and walkable reachability, then resolves group candidates by ascending
priority and object id. Validation, play preparation, runtime sandbox
activation, initial camera position, and camera yaw consume that same accepted
plan; failed or absent bakes never fabricate a usable start.

The drafting canvas shows a finite body/clearance symbol and authored facing.
The 3D viewport shows the exact overlap envelope, floor contact, clearance,
camera height, and facing ray in green or red, plus an actionable projected
status label. Its fixed-layout geometry is cached by document revision,
selection, and room-preview revision, so 300 unchanged frames perform no spawn
replanning and add no document revision, history, bake, or room geometry.
Structured map diagnostics identify unsupported profiles, map bounds, floor
support, obstruction, reachability, and unavailable groups. The common desktop
Diagnostics panel now runs that whole-map validator explicitly, caches its
result by document revision and asset-catalog signature, marks changed results
stale without hidden rebakes, separates full-map and logic-only reports, and
provides actionable source-addressable repair rows plus an honest pass status.

Focused proof covers typed command/no-change/undo/redo behavior, invalid and
non-spawn targets, exact grounding/envelope/camera/yaw geometry, finite invalid
2D symbols, cache invalidation, every physical rejection, fallback selection,
map diagnostic ownership, document and World Layout migration, projection,
play preparation, editor play startup, and runtime activation. The 13 focused
document, mutation, codec, save, editor, projection, validation, preview, play,
and runtime tests build and pass headlessly; the shared diagnostics descriptor
and on-demand cache add a fourteenth focused proof.

This remains M2 rather than M3/M4 until the complete settings workflow is
manually accepted on keyboard/mouse, MacBook touchpad, and PS5 controller, a
visual regression proves the plan/3D symbols at production scale, and G5
provides the separate explicit game/playtest lane. Those are productization
requirements, not reasons to fork or reinterpret the stable spawn recipe.

### G2. NPC Spawn And Patrol - Current M2 Durable Authoring Slice - Continue

- [x] Configure actor archetype through object kind, optional behavior profile,
  team, authored facing, explicit PatrolRoute ownership, health override,
  normalized initial alert, and play-start/disabled policy.
- [x] Preserve the actor policy through document creation and mutation,
  clipboard duplication, object-library recipes and fingerprints, World Layout
  source editing/compilation/adoption, versioned World Layout and save codecs,
  undo/redo, play preparation, runtime seed creation, and Session startup.
- [ ] Add equipment, schedules, and conditional spawn rules only after those
  concepts have explicit runtime owners; do not add inert editor-only fields.
- [ ] Author patrol points/paths with previewed nav reachability.
- [ ] Validate collision, reasoning graph, LOS/cover relevance, and stuck risk.
- [ ] Provide simulation preview without mutating the authored map.

The M2 checkpoint stores NPC settings only on NPC/Enemy spawn objects and
rejects invalid cross-kind payloads. Empty profile, actor-default team, and zero
health retain the prior actor-kind defaults; disabled actors remain authored but
are omitted from play. Save section v31 and World Layout v28 migrate older
actors to those defaults. The 3D Inspector and World Layout source Inspector
edit the same durable settings, while Rotation Y and a PatrolRoute parent remain
the sole facing and patrol authorities. This is not M3/M4 until patrol
reachability and risk diagnostics exist, simulation preview is non-mutating and
useful, and the workflow receives manual visual/input acceptance.

### G3. Logic Link - Current M2 Typed Link Graph - Continue

- [x] Define typed source events, target actions, compatibility, direction, and
  visible graph ownership through one document-layer endpoint descriptor table.
- [x] Keep authored cycles impossible while source and target object roles are
  disjoint; any future relay/gate role must add explicit bounded cycle semantics.
- [x] Persist canonical links through save/load and remap complete internal link
  closures through clipboard, volume clone, and authored-asset placement.
- [ ] Add conditions, delays, one-shot/repeat, and failure policy only with a
  runtime execution owner and bounded deterministic receipts.
- [ ] Add a bounded live event trace; the current Play overlay shows source
  occupancy, directed links, invalid targets, and target active state but not
  historical signal flow.
- [ ] Preserve external links through authored-asset source replacement after
  asset members gain stable local endpoint identities. Refresh currently rejects
  crossing references rather than guessing a replacement endpoint.

The M2 checkpoint makes source behavior an authored contract rather than a
runtime/editor convention. TriggerZone emits a pulse on entry, PressurePlate
holds while occupied, and Switch/Lever/Button use manual activation. Door,
Platform, and MovingPlatform each expose an ordered, target-specific action
roster from the same fixed descriptor table consumed by document validation,
the Inspector, editor action cycling, runtime catalog construction, and runtime
action validation. The Inspector presents product-facing source-event labels
without importing the play/runtime layer.

Directed 3D authoring arrows, source/target diagnostics, undoable semantic
commands, save/load, clipboard remapping, automatic-source execution, and the
Play occupancy/target-state overlay were already operational and are now
recorded here instead of being mislabeled M1. Compile-time and focused unit
proofs pin unique endpoint descriptors, complete target action order, and the
disjoint source/target role law. This remains M2 until replacement-stable
endpoint identity, bounded event history, and manually accepted authoring/play
workflows exist; speculative delay or condition fields would not advance it.

### G4. Triggers, Objectives, Interactables, And Encounters - Current M2 Authored Objective Slice - Continue

- [x] Give LootPoint and ExitPoint durable typed settings with matching-kind
  mutation ownership, undo/redo, direct-object and World Layout inspectors,
  versioned save/layout migration, recipe/fingerprint transport, and
  relationship-aware clipboard remapping.
- [x] Run authored loot quantity and exit requirements through the real Play
  interaction, inventory, objective, outcome, prompt, and feedback owners
  without mutating the authored document.
- [x] Diagnose exits whose finite requirement cannot be met by visible authored
  loot; persistent loot is correctly treated as a repeatable source.
- [x] Keep door, switch/button/lever, trigger-zone/pressure-plate, platform, and
  moving-platform behavior on the typed G3 source/action graph rather than
  creating parallel objective wiring.
- [ ] Add checkpoints and encounter/spawn-wave orchestration only after their
  lifecycle, reset, persistence, and deterministic runtime receipts have named
  owners.
- [ ] Add sound and hazard authoring only after their payloads and runtime
  effects are semantic contracts rather than editor-only fields.
- [ ] Add richer conditional, delayed, one-shot/repeat, and failure behavior
  through G3's bounded execution model, then expose dependency and live event
  traces here.

The M2 checkpoint makes LootPoint and ExitPoint product capabilities rather
than generic markers with hidden conventions. Loot owns an optional shared item
identifier, quantity, and one-shot versus persistent collection policy. Empty
loot identifiers retain deterministic per-object identity. Exit owns an
optional item identifier and quantity requirement plus a deterministic
per-object objective identity. The same settings survive direct creation,
typed mutation, object-library and Creative recipes, authored-asset
fingerprints, clipboard paste, World Layout compile/adopt/codec v29, and
creative-document save v32; older data migrates to the prior no-requirement and
single-removable-loot defaults.

The direct Inspector can bind an exit to any visible document loot source or
enter a semantic identifier manually. The World Layout source Inspector offers
only explicit identifiers because automatic object-derived identities do not
exist until compile. Map validation totals visible finite loot and treats
persistent loot as repeatable, producing an object-addressable error when an
exit requirement is impossible. Play converts loot into real inventory
interactions and exits into real objectives; satisfying the requirement
publishes objective-complete feedback, deactivates the exit, and resolves the
existing `exit_` outcome rule to Victory. Separate presses can collect
persistent loot repeatedly, while held input cannot duplicate either one-shot
pickup or objective completion.

Focused proof covers kind ownership, invalid/no-change mutation behavior,
undo/redo, save and World Layout migration, recipe/fingerprint and clipboard
identity, diagnostic quantities, targetability, repeatable pickups, authored
inventory quantity, exit prompts, objective completion, Victory, and authored
document isolation. This remains M2 rather than M3/M4 until keyboard/mouse,
touchpad, and PS5 workflows are manually accepted, the marker presentation is
visually polished, multiple-objective/reset behavior is exercised in production
maps, and the deferred checkpoint, encounter, sound, hazard, and conditional
logic contracts exist.

### G5. Playtest And Validation - Support System - Finish Before Gameplay Tools

- [ ] Launch a separate game/playtest lane from an explicit saved or staged map.
- [ ] Report spawn, collision, nav, reasoning, objective, and logic diagnostics.
- [ ] Return from playtest without corrupting editor state or history.
- [ ] Capture deterministic replay/receipts for failed canonical scenarios.

Milestone note (2026-07-21): the editor now has an explicit on-demand whole-map
validation surface over the existing `MapValidation` kernel. It reports current
spawn, collision, room-bake, asset, navigation/reachability, and logic failures;
object-owned rows focus the source through the semantic desktop dispatcher.
Results are cached by document revision and a stable asset-catalog signature,
remain visible but marked stale after changes, and never trigger hidden
frame-time room bakes. The existing bounded logic report remains a separate
focused tab instead of being mislabeled as whole-map pass status. This completes
the editor-side foundation for the listed diagnostic classes that currently
exist; reasoning, objective, and deterministic playtest/replay diagnostics are
still required before G5 can be checked complete.

## Future Custom Object Authoring Workspace

Disposition: deliberately deferred. It is a separate workspace, not another map
toolbox section. Promote it only after map building, terrain, and asset placement
reach M3/M4.

### O1. Primitive And Component Creation

- [ ] Create box, cylinder, sphere/ellipsoid, wedge, plane, and authored asset
  components with dimensions and materials.
- [ ] Add, select, move, rotate, scale, duplicate, and delete child components.
- [ ] Preserve a bounded deterministic hierarchy and reject cycles.

### O2. Modifier And Pattern Stack

- [ ] Reuse transform, mirror, linear/radial array, and attachment recipes as
  editable modifiers.
- [ ] Define ordering, enable/disable, reordering, limits, and bake behavior.
- [ ] Defer boolean union/subtract until topology, collision, and failure
  semantics are researched and testable.

### O3. Shape Refinement

- [ ] Define whether refinement means parameter editing, lattice/deform, bevel,
  boolean modeling, or sculpting; do not call a primitive scaler a modeler.
- [ ] Add only operations with deterministic topology and undo/save contracts.
- [ ] Defer freeform mesh sculpting unless the project explicitly chooses that
  complexity.

### O4. Pivot, Sockets, Collision, Materials, And LOD

- [ ] Edit origin/pivot and placement anchor.
- [ ] Author named typed sockets with orientation and compatibility.
- [ ] Preview or author collision independently from render mesh.
- [ ] Assign bounded material slots and variants.
- [ ] Generate or import distance variants with measurable performance budgets.

### O5. Save, Version, Publish, And Update

- [ ] Save a stable definition ID, schema version, hierarchy, local transforms,
  materials, bounds, sockets, render proxy, and collision proxy.
- [ ] Publish one catalog entry with thumbnail and metadata.
- [ ] Place map instances without flattening the definition by default.
- [ ] Define explicit update, pin-version, detach, missing-definition, migration,
  and bake/export workflows.

## Deliberately Not Counted As Tools

The following remain required product systems but must not inflate the tool list:

- Tool options such as radius, axis, falloff, material, count, spacing, and seed.
- Commands such as Save, New, Load, Undo, Redo, Copy, Paste, Duplicate, and Delete.
- View modes such as Plan, Elevation, 3D, fit selection, roof visibility, lower
  level context, contours, dimensions, and snap display.
- Templates such as Estate House.
- Catalog assets and asset categories.
- Preview, diagnostics, reconciliation, cache, render, collision, and nav kernels.
- Pick/sample, Confirm, Cancel, Primary, Secondary, and D-pad adjustments.

These systems are mandatory dependencies of finished tools, but a new enum or
button in one of these systems is not a new product capability.

## Current Registry Coverage Check

Every current `CreativeHeldItemKind` is covered by this backlog:

- Material -> P1
- MaterialBrush -> V1
- ObjectSelect -> C2
- ObjectMove -> C4
- VolumeSelect -> V2
- VolumeFill -> V3
- VolumeHollow -> V4
- VolumeReplace -> V5
- VolumeErase -> V6
- VolumeClone -> V7
- LinearArray -> V10
- ConnectedFill -> V8
- SurfaceExtrude -> V9
- TerrainControl -> T1
- TerrainPaint -> T2
- TerrainGrade -> T3
- TerrainSculpt -> T4
- TerrainProfile -> T5
- TerrainPath -> T6
- TerrainRegion -> T7/T8
- ObjectGroup -> C5
- LogicLink -> G3
- BuildingRoom -> B3

Every current `CreativeEditorWorldLayoutTool` is covered:

- Select -> C2
- Room -> B3
- Floor -> B4
- Wall -> B5
- Door -> B6/B7
- Window -> B6/B8
- Stair -> B9
- Ramp -> B10
- Plateau -> S1
- Road -> S2
- Ditch -> S3
- Bridge -> S4
- CatalogAsset -> P2
- PlayerSpawn -> G1
- NpcSpawn -> G2
- BuildingShell -> B1/B2

The remaining facade modes are covered by C1 Navigate, C3 Measure, C2 Select,
and C4 Transform.

## Delivery Order

### Wave 0 - Stop False Breadth

- [x] Add maturity and visibility to the unified tool descriptor.
- [x] Hide every M0-M2 capability from the default toolbox.
- [x] Keep an explicit Experimental Tools surface for kernel testing.
- [x] Stop counting options, templates, and commands as tools.

The descriptor-backed catalog, drafting toolbox, tool wheel, and held-item
hotbar now enforce this boundary. Released workspace tools are generated from
the descriptor; remaining hotbar slots are explicit assignable empties rather
than advertisements for unfinished capabilities.

### Wave 1 - Complete One Useful Building Workflow

- [ ] Finish G0.1-G0.6.
- [ ] Finish C1, C2, C4, C6, P2, and B1-B12 to the extent required by one
  multi-storey building workflow.
- [ ] Prove: draw plan, generate, inspect in 3D, enter, traverse every level,
  save, reopen, change a wall/opening/storey, regenerate, undo, and playtest.

### Wave 2 - Complete Terrain And Site Workflow

- [ ] Finish T2-T7, T9-T10 and S1-S4 around one road/river/bridge corridor.
- [ ] Prove: generate base terrain, refine contours, grade a road, cut a riverbed,
  place a bridge, inspect slopes, traverse, save, reopen, and modify the spline.

### Wave 3 - Complete Asset And Gameplay Placement

- [ ] Finish P2-P5, G1-G3, and G5.
- [ ] Prove: place/update assets, scatter dressing, author player/NPC spawns and a
  patrol, link one interactable, validate, save, reopen, and playtest.

### Wave 4 - Advanced Construction

- [ ] Finish V1-V10 where they solve refinement tasks the first three workflows
  actually expose.
- [ ] Retire redundant voxel tools instead of polishing operations creators do
  not need.

### Wave 5 - Deferred Systems

- [ ] Re-evaluate T1 Terrain Rod, T11 hydrology/erosion, S5 retaining walls, G4
  gameplay graph breadth, and O1-O5 custom object authoring.
- [ ] Promote only capabilities with a named creator problem and an M2 recipe.

## Release Rule

A feature is not complete when it runs. It is complete when a creator can find
it, understand it, configure it, preview it, commit it, undo it, reopen it,
change it, combine it with the other finished tools, save it, load it, render it,
collide with it, and operate it with the supported input devices without hidden
knowledge or manual repair.
