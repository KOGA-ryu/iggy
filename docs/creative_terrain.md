# Creative Terrain Contract

## Purpose

Terrain is authored as sparse vertical control rods on the Creative document
grid. A rod's X/Z coordinate identifies its control point, Height sets the
terrain elevation, and Radius sets the horizontal influence disk. Rods are the
durable source data; generated terrain is disposable derived data.

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

Equip **Terrain Grade** when a deliberate ramp is needed between authored
rods. This tool uses the same world-action vocabulary without adding bindings:

| Input | Operation |
|---|---|
| Middle mouse / PS5 Square | Set the starting authored rod and sample its exact height |
| Right mouse / PS5 X | Apply the previewed grade as one atomic edit and one undo record |
| Left mouse / PS5 Circle | Cancel the current grade without history |
| Up/down / D-pad up/down | Raise or lower the endpoint height |
| Left/right / D-pad left/right | Shrink or widen every grade rod's influence |

The HUD reports Width as the full influence diameter in cells (`2r+1`), while
the authored control continues to store the canonical radius.

The grade starts only from an authored rod; selecting an interpolated surface
cell cannot silently rewrite an unseen control. After anchoring, aim at any grid
cell to preview the path. Green rod boxes and a connected top line are the exact
control batch that X will submit. Cyan and green footprint outlines show the
start and endpoint influence widths. An over-capacity plan turns red and cannot
mutate the document.

`buildCreativeTerrainGradePlan` owns both preview and mutation geometry. It
walks the X/Z grid with deterministic Bresenham traversal, interpolates heights
with explicit symmetric integer nearest rounding, and emits at most 256 unique
upserts in fixed storage. A rejected plan emits no partial edits. Facade applies
an accepted plan as one batch, so document revision, scene-cache refresh, and
history each advance at most once.

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
