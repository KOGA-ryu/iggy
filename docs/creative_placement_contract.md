# Creative Placement Contract

This document owns the engineering contract behind material placement and
removal. `docs/creative_controls.md` owns the player-facing control vocabulary;
this file owns how a world action becomes a target, a plan, a mutation, visible
feedback, and one history record.

## Public Reference Evidence

The implementation is original iggy3d code. These official Mojang sources are
behavior and API references:

- [Minecraft Editor](https://github.com/Mojang/minecraft-editor) establishes an
  in-engine tool environment with extensions layered over native editor tools.
- [Editor extension samples](https://github.com/Mojang/minecraft-editor-extension-samples)
  register semantic actions against tool contexts instead of polling raw input
  in every tool.
- The sample [dye brush](https://github.com/Mojang/minecraft-editor-extension-samples/blob/main/dye-brush/dye-brush.ts)
  separates button-down, drag, and button-up, deduplicates repeated bounds, owns
  transient preview volume, and restores cursor state when its modal tool exits.
- The [portal generator](https://github.com/Mojang/minecraft-editor-extension-samples/blob/main/portal-generator/portal-generator.ts)
  opens a transaction, validates orientation, tracks the affected area before
  mutation, and either discards or commits the operation.
- The [tree generator](https://github.com/Mojang/minecraft-editor-extension-samples/blob/main/tree-generator/tree-generator.ts)
  tracks an explicit block-change list and reports partially invalid locations.
- [goto-mark](https://github.com/Mojang/minecraft-editor-extension-samples/blob/main/goto-mark/goto-mark.ts)
  records a compact user-defined payload with separate undo and redo handlers.
- [Bedrock block schemas](https://github.com/Mojang/bedrock-samples/blob/main/documentation/Blocks.html)
  keep placement faces, replaceability, selection bounds, support, transforms,
  and state permutations in data rather than input branches.
- [Brigadier](https://github.com/Mojang/brigadier/blob/master/src/main/java/com/mojang/brigadier/CommandDispatcher.java)
  separates expensive parsing from execution and carries structured failure
  context. Creative placement adopts the same plan-then-execute shape.
- [DataFixerUpper](https://github.com/Mojang/DataFixerUpper) is reserved as a
  future reference for versioned map migration. It is not part of placement or
  any frame-time path.
- [WorldEdit `EllipsoidRegion`](https://github.com/EngineHub/WorldEdit/blob/version/7.4.x/worldedit-core/src/main/java/com/sk89q/worldedit/regions/EllipsoidRegion.java),
  [`CylinderRegion`](https://github.com/EngineHub/WorldEdit/blob/version/7.4.x/worldedit-core/src/main/java/com/sk89q/worldedit/regions/CylinderRegion.java),
  and [`EditSession`](https://github.com/EngineHub/WorldEdit/blob/version/7.4.x/worldedit-core/src/main/java/com/sk89q/worldedit/EditSession.java)
  demonstrate cell-centered curved regions, filled/shell separation, directional
  cylinder extrusion, and explicit maximum-change failures.
- [TrenchBroom `DrawShapeTool`](https://github.com/TrenchBroom/TrenchBroom/blob/master/lib/TbUiLib/src/DrawShapeTool.cpp)
  and [`Transaction`](https://github.com/TrenchBroom/TrenchBroom/blob/master/lib/TbMdlLib/include/mdl/Transaction.h)
  keep transient shape updates separate from final document transactions and
  make cancel/rollback explicit.
- [FastAsyncWorldEdit](https://github.com/IntellectualSites/FastAsyncWorldEdit)
  treats changes, iterations, memory, and region counts as separate resource
  budgets. iggy3d adopts the preflight/bounded-work principle, not its async
  implementation.

The samples expose useful seams but are not treated as production-quality
failure policy. iggy3d requires explicit receipts and fail-closed validation.

## Ownership Map

| Stage | Owner | Contract |
|---|---|---|
| Raw device input | `src/app/platform/SdlWindow.*` | Produce device facts only |
| Semantic routing | `src/app/iggy3d/creative/input/InputRouter.*` and `Interaction.*` | Produce pressed/down/released world actions |
| Center-ray target | `apps/iggy3d_creative/EditorInteraction.*` | Resolve object hit, face, placer heading, target cell, adjacent cell, and anchor |
| Object policy | `src/app/iggy3d/creative/document/ObjectDescriptor.*` | Own storage, occupancy, orientation, and whether an object kind may enter placement |
| Geometry plan and admission | `apps/iggy3d_creative/EditorPlacement.*` | Produce one validated fixed-layout plan and structured admission status |
| Selection transform plan | `src/app/iggy3d/creative/tools/SelectionPlacement.*` | Transform copied object records once for preview, Move, and clipboard Copy |
| Transform interaction | `apps/iggy3d_creative/EditorTransform.*` | Own transient source snapshot, contextual controls, preview, and one history commit |
| Shape-cell plan | `src/app/iggy3d/creative/tools/ShapeBrush.*` | Produce deterministic bounded Box, Line, Ellipsoid, and Cylinder cells |
| Bulk cell storage | `src/app/iggy3d/creative/document/VoxelField.*` | Own sorted 16-cubed chunks, atomic edits, greedy cuboids, and grid DDA |
| Mutation gesture | `apps/iggy3d_creative/EditorInteraction.*` | Deduplicate targets, execute due plans, and group history |
| Visual preview | `apps/iggy3d_creative/EditorPreviewFrame.*` | Render held and target views from the admitted plan without document mutation |
| World mutation | `src/app/iggy3d/creative/Facade.*` | Apply requests and return receipts |
| History | `src/app/iggy3d/creative/history/History.*` | Record one changed gesture as one undo snapshot |
| Rendering boundary | `src/render/FrameInput.*` | Carry bounded transient preview data only |
| Room conversion | `src/app/iggy3d/creative/adapters/RoomBake.*` | Convert objects plus cached voxel cuboids into meshes and collision surfaces |
| Save conversion | `src/app/iggy3d/creative/world/DocumentSection.*` | Persist sparse voxel chunks in Creative document section v3 |

## Required Pipeline

Every material placement follows this order:

1. Route raw input to `Primary`, `Secondary`, or `Pick` in the active context.
2. Resolve one `CreativeGridTarget` from the center ray.
3. Read `CreativeObjectPlacementPolicy` from the selected object descriptor.
4. Produce `CreativeBrushPlacementAdmission` and its embedded geometry plan.
5. Use that same admitted plan for target preview and mutation generation.
6. Apply the object-create or voxel-edit request through `Facade` only when
   admission is `Ready`.
7. Use the mutation receipt for feedback and history decisions.
8. Refresh scene geometry only when document identity or revision changes.

Preview code must not recompute different geometry, and execution must not
reinterpret an admitted plan.

## Selection Transform Contract

- `CreativeSelectionPlacementPlan` is the only geometry source for selection
  Move and clipboard Copy. It receives a source anchor, target anchor, a bounded
  quarter-turn value `0..3`, mirror-X, mirror-Z, and the explicit Copy/Move mode.
- For each world point, planning subtracts the source anchor, applies mirrors,
  applies the cardinal Y rotation, and adds the target anchor. Quarter turns use
  exact component permutations rather than trigonometry, so grid-aligned values
  do not accumulate residual drift. Object pivots, bounds-only corner envelopes,
  and every stored path point use this same kernel.
- Transform-backed objects move their pivot and authored bounds by the same
  delta, then update yaw. Bounds-only objects receive the axis-aligned envelope
  of all eight transformed corners. Copy request generation consumes the planned
  object records directly; it cannot reinterpret the offset later.
- Move admission additionally rejects locked objects and any required `Move`,
  `Rotate`, `SetBounds`, or stored-path mutation unsupported by the descriptor.
  All mutations publish through one `applyDocumentMutationsAtomically` batch, so
  one accepted Move advances document revision once.
- A Move source is valid only while document ID and revision still match its
  snapshot and every source object remains present. A stale source stays visible
  as invalid output but cannot mutate. Copy may use a foreign clipboard because
  it creates independent objects and remaps internal parents.
- Planning is `O(objects + path points)` with `O(objects + path points)` output.
  Preview detail is bounded at 512 objects per source/destination side; larger
  selections render aggregate extents. Preview changes never invalidate
  `CreativeEditorSceneCache` because they do not change document revision.

## Current Placement Policy

Current placeable descriptors explicitly use:

- Target: adjacent grid cell.
- Allowed faces: all six cardinal faces.
- Storage: `Wall`, `Floor`, `Ceiling`, and `Roof` use `VoxelCell`; each manual
  placement writes exactly one adjacent grid cell. All other placeable
  descriptors use `AuthoredObject`.
- Grid scale: voxel-backed targeting, volume selection, and mutation use the
  document grid's origin and cell size. Tool snap increments continue to
  position authored objects but cannot resize or offset stored voxel coordinates.
- Orientation: voxel cells are unrotated. Ordinary authored objects use the
  descriptor default.
- Cardinal orientation: `Door`, `Window`, `Arch`, `Fence`, `Railing`, `Ladder`,
  `WallRunSurface`, `Sign`, and `Banner` align their descriptor-owned local
  forward axis with an aimed X/Z face. On top or bottom faces, their front turns
  toward the placer using the opposite of the player's horizontal facing.
- Occupancy: a voxel-backed material rejects any occupied destination cell.
  Distinct authored objects may overlap, but an identical brush kind, transform,
  bounds, and path at the same target is already occupied. Either rejection
  occurs before document revision changes.

These values are intentional compatibility settings, not permanent limits.
Future support requirements, replaceable destinations, and additional
orientation modes must be introduced as descriptor data plus admission tests.
They must not be added as object-kind branches in input or preview code.

`CreativeTransform::rotationEulerRadians` is the only in-memory rotation unit.
Degree-valued UI commands convert once at the transform-command boundary. The
existing serialized `transform.rotation` key is retained and stores the same
Euler-radian values.

`CreativeObject::bounds` describe the object's identity-rotation, identity-scale
shape in document coordinates. `resolveCreativeObjectBounds` applies scale and
intrinsic X-then-Y-then-Z Euler rotation around `transform.position`, returning
fixed-layout corners, exact oriented center/size, and a world AABB. Preview,
picking, document wireframes, spatial projection, volume containment, clipboard
extents, UI summaries, and RoomBake consume that shared result.

Room meshes carry exact Euler radians into CPU vertex generation. Cardinal walls
retain the axis-aligned wall-segment optimization. Arbitrary yaw bypasses the
incompatible wall/floor merge path. Physics still consumes AABB colliders, so a
non-cardinal object receives the conservative world AABB of its oriented shape;
cardinal placement is exact.

## Shape Brush Contract

- Fill/Hollow own a direct two-corner gesture: Primary starts or replaces corner
  1, the current aim supplies the transient second corner, and Secondary fixes
  corner 2 and commits exactly once. The selection wand can supply the same two
  inclusive grid cells for other region operations. Box, ellipsoid, and cylinder
  enumeration is canonical `z/y/x`; reversing the two corners cannot alter their
  generated order.
- Line uses integer 3D Bresenham traversal. Its endpoints are canonicalized and
  included, so reversing corner order produces the same ordered plan.
- Ellipsoid and cylinder classify grid-cell centers against the normalized
  selection envelope. Even-sized selections remain symmetric around the plane
  between their two middle cells.
- Cylinder extrusion uses an explicit X, Y, or Z axis. A one-cell extent on that
  axis is a disk. A one-cell Box extent is a wall or plane without another shape
  kind.
- Hollow means the one-cell six-neighbor boundary of the filled shape. It is a
  closed shell, including cylinder end caps. Line has no removable interior.
- Planning validates enum values and preflights candidate work before iteration.
  Candidate and generated counts are independently bounded; the default limit is
  16,384 cells. A limit or validation failure returns no partial cell list.
- The reusable planner retains its 16,384-cell algorithm ceiling. The interactive
  Creative application currently admits at most 512 candidate/generated cells
  per Fill/Hollow commit as a deliberate interaction safety budget. Preview uses
  the same limit and turns red before a rejected commit. Cells no longer become
  document objects, so future budget changes must be justified by measured
  planning, history-copy, bake, and upload costs rather than object count.
- Fill/Hollow execute only the planned cells. Converting an existing generated
  solid to Hollow removes only generated interior cells; cells elsewhere in the
  selection remain untouched. Replace, Erase, and Clone retain their existing
  rectangular-selection semantics.
- Shape planning is `O(candidate cells)` and Line is `O(longest axis)`. Document
  application remains atomic through a staged document and one history record.
- Preview derives its status and cell count from the same planner. Rendering is
  bounded: Box uses 12 edges, Line uses one centerline plus endpoint cells,
  Ellipsoid uses three fixed 48-segment loops, and Cylinder uses two loops plus
  four rails. Invalid or over-limit plans show a red box and cannot mutate.
- Catalog shape selection is a bounded six-row preset table: Box, Line,
  Ellipsoid, Cylinder X, Cylinder Y, and Cylinder Z. Catalog navigation edits a
  draft and copies it into `CreativeToolSettings` only when the held tool is
  equipped; the planner never reads UI state directly.

## Voxel Storage And Rendering

- `CreativeDocument` owns one `CreativeVoxelField` beside ordinary authored
  objects. Manual `Wall`, `Floor`, `Ceiling`, and `Roof` placement plus
  Fill/Hollow write voxels; Replace, Erase, and Clone operate on both voxels and
  ordinary objects where their rectangular selection contract calls for it.
  Retired `iggy3d.volume_cell.v1` objects remain readable and removable, but new
  structural-cell and bulk operations never create them.
- Volume material cycling admits only descriptors whose storage policy is
  `VoxelCell`; authored props and attachments cannot enter voxel storage through
  Fill, Hollow, or Replace.
- A chunk is exactly `16 x 16 x 16` cells. Chunks are sorted in canonical
  `z/y/x` coordinate order, cells use X-fastest local indices, and negative
  coordinates use floor division. Empty chunks are removed.
- One edit batch is sorted, rejects duplicate cells or invalid materials, stages
  atomically, and advances field and document revision once. Receipts separate
  created, removed, and replaced cells and name every dirty chunk.
- Greedy cuboids never cross chunk boundaries. `CreativeEditorSceneCache`
  caches them by chunk coordinate and monotonic chunk revision, so a document
  refresh remeshes only new or changed chunks before RoomBake combines the
  cached plans with ordinary object meshes.
- Center-ray voxel picking uses grid DDA and compares its distance with the
  nearest ordinary object hit. It allocates no per-cell candidate list. Primary
  removes the exact hit voxel; Pick samples its material; Secondary still uses
  the resolved adjacent grid cell.
- Creative document save section v3 stores sparse chunk coordinates plus
  canonical local indices and serialized material IDs. Section v2 remains
  readable and restores an empty voxel field.
- Existing history remains document-snapshot based. This preserves exact
  undo/redo now that bulk cells are compact, but very large-world delta history
  remains a separately measured optimization.

## Admission Statuses

| Status | Meaning | Mutation |
|---|---|---|
| `Ready` | Target, descriptor policy, and geometry plan agree | Allowed |
| `InvalidTarget` | Missing, non-finite, or degenerate target facts | Rejected |
| `UnsupportedBrush` | Object descriptor cannot produce placement geometry | Rejected |
| `InvalidGeometry` | Planned bounds or transforms are invalid | Rejected |
| `UnsupportedPolicy` | Policy requests semantics this runtime does not implement | Rejected |
| `FaceDisallowed` | Hit face is absent or excluded by descriptor policy | Rejected |

Unknown enum values and non-finite inputs fail closed. Rejections may render a
red positionable ghost but never mutate the document.

## Gesture And History Laws

- Secondary places and Primary removes. Primary wins simultaneous presses.
- The first action is immediate; held input repeats every 200 milliseconds.
- One target cell or object may mutate at most once per gesture.
- The visited set is fixed at 256 entries. Reaching capacity rejects further
  edits without allocating.
- The history transaction opens lazily. Release or interruption commits one
  record only when at least one mutation changed the document.
- Tool changes, hotbar changes, modals, capture mode, transform preview, undo,
  redo, new, load, focus loss, and shutdown finalize the active gesture.
- Empty or wholly rejected gestures cancel without creating history.
- Occupied voxel cells and identical authored-object plans are rejected across
  gesture boundaries. This prevents repeated taps or touchpad edge events from
  stacking invisible work that forces another document bake.
- A Fill/Hollow corner pair applies one atomic volume operation and records at
  most one history entry. Secondary without an armed first corner fails closed,
  and a completed pair cannot be reapplied until Primary starts a new pair.

## Visual And Cache Laws

- Held preview is view-space state. Target preview is world-space state.
- Valid target is green; invalid but positionable target is red.
- Preview items never enter document geometry, room signatures, saves, or undo.
- An accepted mutation hides the target ghost in the same frame and uses the
  live object bounds or exact voxel-cell bounds for receipt feedback.
- `CreativeEditorSceneCache` is keyed by document ID and revision. Aim motion
  does not rebuild or upload room geometry.
- Voxel cuboid plans are additionally cached by chunk coordinate and revision;
  only dirty chunks are remeshed on a document refresh.

## Deferred Algorithms

The following require explicit design and randomized or performance testing
before implementation:

- Replaceable-cell and support-neighbor queries.
- Directional state permutations for stairs, ramps, doors, decals, logs, and
  similar object classes whose gameplay state changes with orientation.
- Exact oriented collision primitives for arbitrary non-cardinal yaw. Current
  physics intentionally uses a conservative transformed AABB.
- Edge/corner ray tie-breaking for rotated and non-uniform bounds.
- Atomic rollback and compact diffs for very large operations.
- Client/server sequence IDs and reconciliation if editing becomes networked.
- Controller disconnect, reconnect, deadzone, and trigger-threshold behavior on
  physical hardware.

## Proof Targets

- `creative_object_descriptor_tests`: storage and occupancy ownership, local
  forward axes, and face filtering.
- `creative_shape_brush_tests`: lattice counts, symmetry, canonical line order,
  axes, disks, hollow boundaries, enum rejection, and both operation limits.
- `creative_voxel_field_tests`: negative chunk coordinates, atomic edits,
  chunk-local greedy cuboids, grid DDA, and one document revision per batch.
- `creative_interaction_tests`: semantic action edges and repeat timing.
- `creative_editor_placement_tests`: object/voxel mutation parity, cardinal
  admission, oriented preview/picking/bake/render parity, exact voxel feedback,
  bounded shape outlines, gesture deduplication, interruption, history grouping,
  and scene-cache reuse.
- `creative_tools_tests`: degree-command to stored-radian conversion and
  transform/scale geometry, shared selection-transform geometry, atomic Move,
  locked-source rejection, and invalid quarter turns.
- `creative_pattern_tests` and `creative_editor_pattern_tests`: transformed
  clipboard parity, non-mutating Copy/Move previews, contextual control updates,
  one-step history, red invalid output, cancellation, and aggregate fallback.
- `creative_spatial_projection_tests` and `creative_document_wireframe_tests`:
  transformed world bounds and exact oriented box edges.
- `render_projection_input_tests`: bounded preview frame validation.
- `render_command_recording_tests`: target-before-held draw ordering and depth
  policy.
