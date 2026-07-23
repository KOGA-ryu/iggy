# Creative Terrain Contract

## Purpose

Terrain is authored as an exact height field plus material field derived from a
bounded ordered operation stack. Sparse vertical control rods remain one direct
authoring source: a rod's X/Z coordinate identifies its control point, Height
sets elevation, and Radius sets its influence disk. Generated operations,
regions, grades, paths, and future reusable stamps share the same durable stack;
render and collision output remain disposable derived data.

## Interaction

Equip **Terrain Rod** from the catalog or assign it to a tool-wheel sector.

| Input | Operation |
|---|---|
| Hold right mouse / PS5 X | Paint or update rods, or commit the selected rod's draft |
| Hold left mouse / PS5 Circle | Cancel a selected draft; otherwise erase rods |
| Middle mouse / PS5 Square | Select the highlighted rod and sample its Height and Radius |
| Up/down / D-pad up/down | Raise or lower Height directly |
| Left/right / D-pad left/right | Shrink or widen Radius directly |

The cyan vertical guides are stored rods. Their dim ground boxes show influence.
The nearest aimed rod turns green, a selected rod turns pink, and the yellow
guide plus stepped cell-edge outline show the live draft and its exact circular
influence membership. D-pad edits remain revision-free until X commits; Circle
restores the sampled values without adding history. Guides are editor overlays;
they never enter saved room geometry or change document revision.

Paint and erase gestures act immediately, then repeat every 200 ms while held.
Each gesture tracks at most 256 unique X/Z coordinates and never mutates one
coordinate twice, even if the aim revisits it. Press-hold-release creates one
lazy history transaction: accepted mutations join one undo record, while an
empty or rejected gesture records nothing. If Circle begins while a rod draft
is selected, that complete hold is cancel-only and cannot fall through into
erasing the same rod. Focus loss, modal entry, commands, hotbar changes, capture
mode, and shutdown finalize an active gesture.

Terrain Rod also exposes `ROD STAMP` in Tool Options. `SINGLE` retains the
one-rod behavior above. `SEED` turns the same X/Circle gesture into a bounded
area operation:

- X seeds only missing rods on a circular lattice. Existing rods are preserved.
- Circle clears every authored rod in the circular seed footprint.
- `SEED RADIUS` selects 1, 2, 4, or 8 cells; `SEED SPACING` selects 1, 2, or 4.
- New rods sample the pre-edit derived terrain height where terrain exists and
  otherwise use the HUD Height. Their influence uses the HUD Radius.
- The largest radius with one-cell spacing plans at most 197 positions. If the
  resulting document would exceed 256 controls, the entire stamp rejects before
  history or document mutation.

The green seed preview shows every missing rod that X will add; holding Circle
turns the same exact preview red for rods that will be removed. Preview planning
is fixed-capacity and revision-free. Overlapping or stationary stamps preserve
existing controls and remain grouped into the gesture's single undo record.

Equip **Terrain Grade** to author one reopenable, straight terrain corridor
between two grid cells. It uses the shared world-action vocabulary without
adding bindings:

| Input | Operation |
|---|---|
| Middle mouse / PS5 Touchpad | Start a grade, or reopen the newest enabled manual grade handle under the cursor |
| Right mouse / PS5 X | Apply the previewed grade as one atomic edit and one undo record |
| Left mouse / PS5 Circle | Cancel the current grade without history |
| Up/down or PS5 Square / D-pad up/down | Cycle start height, end height, width, cross-slope, and falloff |
| Left/right / D-pad left/right | Decrease or increase the selected value |

The HUD reports Width as the full corridor diameter in cells (`2r+1`), while
the durable recipe stores its canonical half-width. Heights are bounded to the
terrain range, half-width and falloff to 0-64 cells, and cross-slope to
-100.0% through +100.0% in exact 1.0% steps.

Starting a new grade samples the dense authored terrain first, then legacy
terrain, and otherwise keeps the current tool height. Aim to move the selected
handle. The green center line, start/end handles, corridor bounds, candidate
terrain surface, and slope colors come from the same immutable operation plan
that X submits. Invalid or degenerate corridors turn red and cannot mutate the
document.

`CreativeTerrainGradeRecipe` is the single source for preview, commit,
operation-stack replay, save/load, room bake, collision, and the live movement
readout. The kernel interpolates longitudinal elevation, signed cross-slope,
constant corridor width, and bounded falloff over at most 8,192 terrain cells.
The readout samples the generated collision triangles through the runtime slope
policy instead of estimating walkability from authored parameters. Applying a
new grade or updating an existing stable operation creates one history record;
a rejected plan emits no partial field.

World Layout roads and site approaches keep one durable Path owner. Every Road
segment is validated through the shared Grade adapter. A `BUILDING_PAD` endpoint
must be an exact linear, flat, constant-width Grade segment from exterior
terrain to exactly one building footprint perimeter. A `BRIDGE` endpoint must
participate in a two-sided pair on opposite ends of one integral bridge
footprint; both terminal segments must exactly equal the two recipes produced
by the bridge adapter. Curved, crowned, variable-width, unmatched, one-sided,
or ambiguous approaches reject before any document mutation. The compiler does
not generate hidden duplicate Grade operations underneath the authored Path.

Equip **Terrain Sculpt** to reshape existing authored rods without creating new
terrain controls:

| Input | Operation |
|---|---|
| Hold right mouse / PS5 X | Apply Raise, Lower, Flatten, or Smooth immediately, then repeat every 200 ms |
| Left mouse / PS5 Circle | End and cancel the active sculpt gesture |
| Middle mouse / PS5 Square | Sample the exact derived surface height as the Flatten target |
| Up/down / D-pad up/down | Increase or decrease Strength through 1, 2, 4, and 8 cells |
| Left/right / D-pad left/right | Shrink or widen Radius through 1, 2, 4, and 8 cells |

Raise adds Strength cells and Lower subtracts Strength cells from every existing
rod in the selected Circle or Square mask, clamped to the authored 1-64 cell
height range.
Flatten moves each rod toward the sampled target by at most Strength cells.
Smooth computes each affected rod's neighborhood average from the same pre-edit
snapshot using the selected mask, then moves toward that average by at most
Strength cells. No mode creates rods, changes rod radii, or partially applies a
rejected batch. An empty brush reports `NO RODS`; use Terrain Rod when the
authored field needs more controls.

Tool Options exposes `MASK`, `FALLOFF`, and, in Flatten mode, a numeric `TARGET`
without adding world-action branches. `CIRCLE` measures radial distance;
`SQUARE` measures Chebyshev distance and includes the complete square footprint.
`UNIFORM` preserves full Strength across either mask. `LINEAR` tapers Strength
by that mask's distance metric, and `SMOOTH` applies a smoothstep taper for
rounded shoulders. Both tapered modes reach zero at the exact brush boundary.
Their weights and integer height steps use deterministic fixed-point math;
center, preview, and mutation therefore agree across platforms. `UNIFORM` and
`CIRCLE` are the defaults so existing terrain gestures retain their behavior.

One press-hold-release sculpt gesture is one lazy history transaction. Repeating
over a stationary brush is intentional: each 200 ms step continues moving the
same rods toward the target. Release, tool/modal changes, focus loss, capture,
undo/redo, document replacement, and shutdown finalize the gesture; a gesture
with no accepted mutation records no history.

The preview is cached by document ID/revision, aim cell, mode, mask, radius,
strength, falloff, target height, and contour settings. Preview and commit share
one immutable sculpt plan. Its exact dirty region is the union of changed
control influence bounds plus one patch cell for corner-height sharing. The
regional renderer samples only that region and its one-cell support border;
it does not rebuild the complete terrain. The maximum-control proof edits 256
radius-16 controls while bounding preview work to 2,500 candidate patches and
2,704 sampled support coordinates, with bit-exact parity against the equivalent
full-render subset.

Candidate contours are rebuilt from a staged field only inside that region, and
preview triangles use runtime movement slope policy: green is normally
walkable, yellow requires careful footing, and red is rejected by the current
maximum-walkable-slope rule. The selected mask and mode-specific target are
repeated in the sculpt HUD. Aiming alone never mutates the document, records
history, or uploads room geometry.

Terrain Sculpt is explicitly **destructive**. Each accepted gesture changes the
canonical terrain controls directly and remains editable through ordinary
terrain tools plus undo/redo; it does not create or pretend to own a reopenable
parametric source operation. Use Terrain Profile, Path, Grade, Region, or Stamp
when source parameters and operation ordering must remain durable.

Equip **Terrain Profile** to stamp a complete mathematical elevation in one
action. It is a catalog/hotbar tool and does not replace a default tool-wheel
sector.

| Input | Operation |
|---|---|
| Right mouse / PS5 X | Add the exact preview, or update the selected profile |
| Middle mouse / PS5 Touchpad | Select a stored profile center, otherwise lock the sampled base height |
| Left mouse / PS5 Circle | Return a locked base to automatic sampling |
| Up/down / D-pad up/down / PS5 Square | Select the previous/next exact profile control |
| Left/right / D-pad left/right | Decrease/increase the selected control by one unit |

Profile stamps are intentionally one-shot. Holding X or right mouse does not
repeat after 200 ms. This prevents accidental overlapping Add stamps and avoids
unbounded scene rebuilds while a large brush is held.

Tool Options and the viewport quick-edit readout expose these bounded settings:

- `PROFILE`: Hill, Basin, Ring, Crater, Ridge, Wave, or Ripple.
- `BLEND`: Set computes `base + delta`; Add computes `pre-edit height + delta`.
- `ROD POLICY`: Fill adds a canonical lattice where rods are missing; Existing
  edits only authored rods already inside the footprint.
- `BASE`: 1-64 cells, either sampled automatically or locked.
- `RADIUS`: 1-64 cells.
- `AMPLITUDE`: 1-64 cells.
- `SPACING`: 1-16 cells.
- `DIRECTION`: eight X/Z axis and diagonal directions for Ridge and Wave.
- `FREQUENCY`: 1-8 cycles for Wave and Ripple.
- `SEED`: an exact unsigned 64-bit oscillatory phase seed.

Fill is the default, so a Hill works on an empty map. It materializes the dense
bounded profile footprint; Existing modifies only already-authored height cells.
Set is idempotent at unchanged settings. Add intentionally compounds. Touchpad
samples the same derived terrain height used by rendering; the lock persists
while aiming elsewhere, and Circle restores automatic per-aim sampling. If no
terrain is present at the aim point, automatic base uses the Terrain Rod HUD
height.

`buildCreativeTerrainProfileRecipe` is the dense geometry owner. The versioned
recipe records center, base, kind, blend, source policy, direction, radius,
amplitude, spacing, frequency, and seed. It evaluates the bounded footprint in
canonical Z/X order with checked coordinates and a maximum 8,192-cell output;
every rejection publishes zero partial output. The editor previews the exact
`CreativeTerrainOperationMutationPlan` later sent through `Facade`, then renders
its candidate height field. Green footprint and slope-colored triangles therefore
match commit bit-for-bit; invalid combinations retain a red non-mutating preview
and publish the exact failure reason.

An accepted profile is one durable `Profile` operation and one history record,
not a destructive pile of control edits. A stored manual operation keeps its id
when its center or settings change. Touchpad reopens the topmost manual profile
at an aimed center. The Operations panel can select, duplicate, reorder,
enable/disable, delete, and bake profiles through the same generic stack used by
the other procedural terrain sources; every mutation is undoable. Save format 25
round-trips all profile fields. World Layout-owned profiles remain reconciler
owned and are intentionally not opened by the manual profile editor.

Profile math is deterministic integer fixed point. A checked-in 65-entry Q15
quarter-wave table supplies sine; cosine is a quarter-turn phase offset. Radial
distance and profile coordinates use Q16, 64-bit intermediates, and explicit
symmetric nearest rounding. Runtime profile planning does not call `std::sin`
or `std::cos`. Secant, cosecant, and cotangent are deliberately excluded because
their poles conflict with finite bounded terrain output.

Wave requires `2 * frequency * spacing <= radius`; Ripple requires
`4 * frequency * spacing <= radius`. Both also require Fill so sampling spacing
is defined. Moving or editing an accepted source changes its stable height hash;
the scene cache rebuilds terrain once even when the replayed field's local
revision happens to match the previous field, then reuses it on idle frames.

Equip **Terrain Path** to connect terrain features as a road, riverbed, ridge,
or trench. The tool uses locked bends plus one live endpoint; it does not add a
new controller binding.

| Input | Operation |
|---|---|
| Middle mouse / PS5 Square | Lock the aimed start or bend point |
| Right mouse / PS5 X | Commit the locked route plus its current live endpoint |
| Left mouse / PS5 Circle | Remove the latest locked point; an empty route is cancelled |
| Up/down / D-pad up/down | Increase or decrease rise for Road/Ridge or depth for River/Trench |
| Left/right / D-pad left/right | Shrink or widen the path through 1, 3, 5, and 7 cells |

A straight path takes Square once, aim at the endpoint, then X. Square can lock
additional bends before X. Cyan guides are locked points, yellow is the live
endpoint, green is an accepted exact route, and red is a rejected route. Tool
options choose `PATH TYPE` (Road, River, Ridge, Trench), `ELEVATION` (Follow,
Level, Grade), `WIDTH`, and `RISE / DEPTH` (1, 2, 4, 8 cells).

- Road is a flat raised corridor across its selected width.
- River is a smooth depressed bowl returning to base at its banks.
- Ridge is the positive smooth cross-section.
- Trench is a flat full-depth cut across its selected width.
- Follow samples the pre-edit derived terrain along the centerline.
- Level holds the first locked point's elevation.
- Grade interpolates from the first point to the live/final endpoint by route
  progress.

`buildCreativeTerrainPathPlan` owns centerline rasterization, cross-section
geometry, final preview controls, and mutation edits. It walks each segment with
deterministic integer Bresenham traversal, deduplicates joins and overlapping
corridor cells, sorts output by Z then X, and computes smooth cross-sections
with the shared fixed-point terrain kernel. The input is capped at 32 path
points, each segment at 255 cells, and the final authored field at 256 controls.
Coordinate, traversal, or capacity failure returns zero edits; paths never stamp
a partial prefix.

Preview applies the same plan to a copied terrain field and caches the result by
document ID, terrain revision, effective point list, and path settings. Aiming
within one grid cell reuses the cache and never mutates room geometry. X submits
the plan through one Facade terrain batch, producing at most one document
revision, scene refresh, and undo record. One undo restores the complete
pre-path terrain field.
Catalogs, tool options, controls, transforms, and capture mode hide the overlay.

Equip **Terrain Region** for bounded changes to authored rods. It reuses the
selection-box interaction rather than reserving another controller button:

| Input | Operation |
|---|---|
| Right mouse / PS5 X | Set corner 1, set corner 2, then apply the complete region |
| Left mouse / PS5 Circle | Clear the region without document history |
| Middle mouse / PS5 Square | Sample the aimed derived height when Flatten is selected |
| Up/down / D-pad up/down | Adjust Amount for Raise/Lower/Smooth, or Target for Flatten |
| Left/right / D-pad left/right | Cycle Raise, Lower, Flatten, Smooth, and Erase |

Raise and Lower add or subtract 1, 2, 4, or 8 cells. Flatten sets every selected
rod to the sampled or tuned target. Smooth averages authored neighbors from one
unchanged pre-edit snapshot and retains each rod's authored radius. Erase removes
the selected rods. All operations affect existing rods only; an empty region is
rejected instead of densifying terrain implicitly.

The normal selection box shows the inclusive X/Z footprint. Green rod and slope
guides show an accepted result, orange marks Erase, and red marks a rejected
plan. The preview is cached by document ID, terrain revision, bounds, operation,
amount, and target height. Aiming and D-pad edits do not mutate the document.
`buildCreativeTerrainRegionPlan` scans the canonical terrain field into fixed
256-entry storage, preserves canonical order, and emits no partial prefix on
invalid input or capacity failure. X submits that exact edit span through one
Facade batch, so the complete region produces at most one document revision,
scene refresh, and undo record.

### Terrain Stamps

A complete volume selection can be captured as exact reusable terrain. In the
desktop **Terrain Generator**, enter a name under **Terrain Stamps** and choose
**Save Selected Region**. The catalog shows a deterministic 16 x 16 material
and elevation thumbnail, source dimensions, present-cell count, and asset
version. **Place** copies that immutable source into the normal terrain-stamp
preview; **Delete** removes only the catalog source.

Named sources live under
`~/.iggy3d/creative_standalone/terrain_stamps/*.igts`. The library holds at most
64 current-format assets. Its binary codec is little-endian and validates its
magic, codec version, dense cell count, identity lengths, exact material
weights, content signature, and trailing size before installing an asset. Load
is deterministic by filename. Invalid, oversized, duplicate, filename-mismatched,
or corrupt assets are rejected independently without hiding the valid catalog.
Writes use a temporary sibling followed by rename; in-memory state changes only
after the durable write succeeds.

| Stamp input | Operation |
|---|---|
| Right mouse / PS5 X | Apply the preview as one terrain batch; the stamp stays active |
| Left mouse / PS5 Circle | Cancel the stamp preview without changing terrain |
| Up/down / D-pad up/down | Select Rotation, Mirror X, Mirror Z, or Height |
| Left/right / D-pad left/right | Rotate 90 degrees, toggle the selected mirror, or adjust height by one cell |

`CreativeTerrainStamp` stores up to 8,192 source-local dense cells, including
holes, exact heights, and exact four-material weights. It also retains asset ID,
label, asset version, source document/revision, source minimum, dimensions,
minimum present height, and a content signature. Capturing reads the composed
visible terrain, not only legacy control rods. `buildCreativeTerrainStampPlan`
mirrors before applying a normalized quarter-turn rotation and returns the exact
final height and material fields used by both preview and commit. `MERGE` writes
present stamp cells while preserving destination terrain at stamp holes.
`REPLACE` also clears destination height and material at those holes. Coordinate,
height, union-capacity, invalid-source, and output failure reject atomically.

Tool Options also owns `STAMP HEIGHT`. `SURFACE` is the default: when authored
terrain exists under the crosshair, the copied stamp's lowest rod aligns to that
derived surface height. Empty terrain preserves the copied elevation instead of
inventing a floor height. `ABSOLUTE` always preserves copied heights. The
selected Height quick-edit channel applies a signed manual offset after either
policy, from -63 to +63 cells. If any final rod would leave the authored 1-64
height range, the complete plan is rejected and remains a red non-mutating
footprint.

The crosshair anchors the transformed footprint's lower X/Z corner. The valid
green surface and invalid red footprint are cached by document revision, stamp
signature, target, transform, merge mode, elevation mode, and manual height
offset. Aiming does not mutate or rebuild room geometry.
Every X/right-click adds one durable `Stamp` operation through `Facade` and
creates one undo record; moving the crosshair and pressing again creates another
independently undoable operation.

Every operation embeds the complete baked stamp and separately records its
source asset identity, version, and content signature. Replay and map loading
therefore never depend on a mutable or missing catalog file. The operation list
classifies its source as `Available`, `Missing`, `VersionMismatch`,
`ContentMismatch`, or `Incompatible`. **Repair Source** restores a missing
catalog asset from the embedded baked payload; **Restore Baked Source** replaces
an explicitly drifted source. Deleting a source cannot change existing terrain.
Duplicating an operation copies its complete stamp recipe. Document save schema
v24 round-trips the ordered operation, exact dense payload, identity, transforms,
mode, and elevation policy. **Bake Terrain Stack** remains the explicit,
undoable destructive route that preserves the current height/material result
while removing all parametric operations.

## Procedural Terrain Operations

The desktop Terrain Generator creates or edits one `GeneratedTerrain` operation
in the same ordered stack used by region, grade, and path recipes. Its visible
parameters are Seed, Bounds, Base Height, Relief, Horizontal Scale, Octaves,
Persistence, Lacunarity, Slope Damping, Mask, Feather, and composition mode.
Surface Intent adds a material-output toggle, named biome intent, explicit
lowland/highland materials, and a transition height. Named intents are only
presets: their concrete material fields are saved with the recipe, and direct
material edits switch intent to Custom.

Preview is transient. It replays the candidate stack against current authored
height and material truth, then renders that exact candidate without changing
document revision, history, save data, room signature, or the committed scene
cache. The preview cache keys document identity/revision, source-scene refresh,
grid transform, candidate height hash, and candidate material hash. A material-
only change therefore redraws the staged colors, while unchanged idle frames do
no generation, room bake, or scene upload work.

Apply commits one operation through `Facade` and records one undo snapshot.
Regenerate advances the seed once and rebuilds the same bounded recipe. A
successful operation may protect up to 16 rectangle or ellipse regions; every
protected cell retains both its pre-operation height and material. Protection
is part of the recipe and round-trips through the document save section.

`Paint materials` distinguishes height-only generation from height plus surface
intent. It is a durable semantic switch, not a UI-only compatibility branch.
Legacy operation stacks restore with it disabled because those formats never
authored generator materials.

`Bake Terrain Stack` is deliberately destructive and undoable. It preserves the
exact current height and material result as the stack base, removes every
operation, and leaves no hidden parametric layer. Undo restores the complete
ordered stack. Individual detach/rebase is not offered because removing one
layer while preserving every later layer would require a separately specified
provenance rewrite rather than an honest local edit.

Generation accepts at most 8,192 cells and eight octaves. Receipts expose exact
cell and octave evaluations, material overrides, protected cells, modified
cells, and stable height/material hashes. Invalid versions, kinds, bounds,
non-finite or out-of-range values, operation overflow, malformed protection,
and field replacement failure reject atomically.

## Authored Data

`CreativeTerrainField` owns a canonical vector sorted by Z then X. Coordinates
are unique. A mutation batch is atomic and advances both terrain and document
revision once when it changes content.

- Maximum controls: 256
- Height: 1-64 grid cells
- Radius: 1-16 grid cells
- Undo/redo: full document history snapshot
- Save section: optional `creativeDocument.terrainControl.*` records
- Old saves: absence of the control block means an empty valid terrain field

Terrain controls are independent of authored objects and voxel cells. Do not
materialize rods as hidden `TerrainPatch` objects or write generated cells into
`CreativeVoxelField`.

## Surface Kernel

`buildCreativeTerrainSurfacePlan` is a pure deterministic rebuild:

1. Enumerate only cells inside each rod's circular influence disk.
2. Give each contribution integer weight `(radius^2 - distance^2 + 1)^2`.
3. Sort contributions by Z then X and reduce overlaps with a rounded weighted
   average of rod heights.
4. Merge consecutive equal-height cells across each X row into cuboids.
5. Emit `TerrainPatch` cuboids from document grid Y=0 to the resolved height.

There is no floating-point interpolation and no scan of the controls' global
bounding rectangle. Work scales with the sum of influence-disk areas. A single
rod creates a flat circular patch; overlapping rods blend where their disks
intersect.

`buildCreativeTerrainRenderPlan` consumes that canonical column plan and emits
one center-plus-four-corner visual patch per occupied cell. The center preserves
the resolved column height; each corner averages the same neighboring column
set, so adjacent patches share identical edge vertices and bend without cracks.
The plan is capped at 8,192 patches and emits no partial mesh when the cap or
coordinate conversion fails.

`sampleCreativeTerrainHeight` applies the same integer reduction to one X/Z
cell. `raycastCreativeTerrainField` uses that sampler in a bounded X/Z DDA, so
the center ray can target terrain tops and cliff sides without rebuilding or
scanning generated cuboids. Authored voxels and objects win equal-distance ties
over derived terrain; failed or over-budget terrain traversal does not fabricate
a ground-plane target.

## Render And Runtime

`CreativeEditorSceneCache` caches generated fallback cuboids plus the shared
center-and-corner terrain patches by document ID, terrain revision, grid origin,
and grid cell size. Aim movement and guides reuse the cache. An accepted rod
mutation or grid transform change rebuilds both derived views once. Vulkan
batches admitted patches into one terrain draw, includes their vertices in the
room geometry signature, and frustum-culls them with the rest of the room. If
the patch cap or the room's 16-bit vertex budget is exceeded, the renderer
automatically falls back to the stepped terrain planes instead of dropping the
surface.

Rejected, repeated, and revisited stroke samples do not advance document
revision, so they do not rebuild or upload terrain scene data.

An admitted render plan also enters `RoomAsset` as fixed five-point
`HeightPatch` walkable surfaces. Runtime height and segment queries evaluate the
same four center-fan triangles as the renderer, including their exact normals.
Player grounding and movement therefore follow the visible height, enforce the
configured maximum walkable slope, and keep the top projectile-solid. The
physics AABB bake deliberately skips these query-owned top patches; only exposed
outer terrain edges and holes emit actor/projectile cliff boxes, avoiding hidden
stair-step colliders beneath a smooth surface.

This contract is fail-safe rather than all-or-nothing. A missing, invalid,
conversion-failed, or over-budget patch plan does not emit partial smooth
collision. The existing terrain cuboids retain their flat walkable tops and
full actor/projectile blocker columns for that bake, matching the renderer's
stepped fallback.

## Future Algorithm Gates

Keep these as explicit later work, with profiling and visual tests before use:

- AI navigation over smooth terrain must consume the bounded height-patch query
  and prove route/slope parity before replacing its current footprint model.
- Material layers, erosion, spline ridges, caves, and overhangs are separate
  authored capabilities. A heightfield must not be stretched to represent them.
- Future grid mutation paths must preserve grid origin and cell size as terrain
  scene-cache keys.
- Neighborhood smoothing remains the bounded O(n^2) reference kernel for at
  most 256 rods. Replace it only after profiling demonstrates a bottleneck and
  an indexed implementation proves bit-identical plans in tests.
