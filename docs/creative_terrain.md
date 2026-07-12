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
| Right mouse / PS5 X | Place a rod or update the rod at the aimed grid point |
| Left mouse / PS5 Circle | Remove the rod at the aimed grid point |
| Middle mouse / PS5 Square | Sample that rod's Height and Radius |
| Up/down / D-pad up/down | Select Height or Radius |
| Left/right / D-pad left/right | Decrease or increase the selected value |

The cyan vertical guides are stored rods. Their dim ground boxes show influence.
The yellow guide is the current target and settings. Guides are editor overlays;
they never enter saved room geometry or change the document revision.

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

## Render And Runtime

`CreativeEditorSceneCache` caches the generated cuboids by terrain revision.
Aim movement and guides reuse the cache. An accepted rod mutation changes the
document revision, rebuilds the surface once, and feeds the cuboids through the
existing room bake. The result is visible and supplies the same walkable
surface path used by voxel-backed floor geometry. Terrain columns additionally
emit actor and projectile blocker boxes so rendered cliffs are physically solid.

The initial representation is intentionally stepped. It uses existing bounded
box rendering and physics instead of introducing a second terrain renderer.

## Future Algorithm Gates

Keep these as explicit later work, with profiling and visual tests before use:

- Smooth triangulation may consume the same control field, but must preserve
  exact rod heights, deterministic topology, bounded triangle counts, and a
  collision/nav parity test.
- A terrain-specific center-ray query should replace ground-plane targeting if
  editing steep walls from a horizontal view becomes necessary.
- Material layers, erosion, spline ridges, caves, and overhangs are separate
  authored capabilities. A heightfield must not be stretched to represent them.
- Any mutable grid origin or cell-size feature must invalidate the terrain scene
  cache explicitly.
