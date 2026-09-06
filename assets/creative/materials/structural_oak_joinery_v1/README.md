# Structural Oak Joinery v1

This asset extends the accepted `structural_oak_timber_v1` growth volume into
clean construction cuts. It is not a second wood material painted over the
beam. Mortise walls, housing beds, tenon cheeks, shoulders, and drawbore walls
are evaluated at their real positions inside the same tree represented by the
outer timber.

The acceptance fixture is one fully housed, blind mortise-and-tenon joint with
an offset drawbore and a separate tapered octagonal peg. It deliberately
contains no damage, checking, edge wear, dirt, repair, or authored roughness
map. Those remain later, separately controlled systems.

## Reference decisions translated into dimensions

Written reference was converted into explicit metre-scale rules before any
geometry was built:

- The [Timber Frame Guild discussion of tenon sizing](https://forums.tfguild.net/ubbthreads.php?Number=33583&ubb=showflat)
  describes the common rule of a tenon one quarter of stock thickness and a peg
  about half the tenon thickness. On the `0.300 m` receiving timber, the
  mortise is therefore `0.075 m` wide and the riven peg is `0.038 m` across.
- The [Timber Frame HQ fully housed joint example](https://timberframehq.com/fully-house-mortise-and-tenon-joint/)
  gives a housing range of roughly three quarters to one and a half inches.
  This fixture uses a restrained `0.0254 m` housing: deep enough to establish a
  real bearing seat without removing an implausible amount of the beam.
- The [Timber Frame Guild drawbore discussion](https://forums.tfguild.net/ubbthreads.php?Number=90&ubb=showflat)
  describes a draw of roughly one sixteenth to one eighth inch and traditional
  tapered, often octagonal pins. The tenon bore is offset `0.0024 m` toward its
  shoulder, and the peg has eight riven faces plus a `0.055 m` taper.
- The [Blender Boolean modifier manual](https://docs.blender.org/manual/en/dev/modeling/modifiers/generate/booleans.html)
  defines the Exact solver and its material-transfer option. Both are used
  here so generated bearing and cavity faces can be identified before the
  temporary cutter is deleted.

The locked fixture dimensions are:

| Part | Dimension |
| --- | --- |
| Receiving timber | `4.200 × 0.300 × 0.340 m` |
| Housing | `0.260 × 0.240 × 0.0254 m` |
| Blind mortise | `0.200 × 0.075 × 0.155 m` |
| Entering member | `1.200 × 0.240 × 0.240 m` |
| Tenon | `0.160 × 0.071 × 0.196 m` |
| Mortise/tenon cheek clearance | `0.002 m` each side |
| Mortise end clearance | `0.004 m` |
| Drawbore | `0.040 m` bore, `0.038 m` peg |
| Draw offset | `0.0024 m` toward the shoulder |

## Wood continuity rather than a bag of patterns

The atlas generator loads the approved structural-timber profile, pattern, and
generator. It then reuses the same pith path and all `832` annual-ring
boundaries. Every texel starts from a physical coordinate:

- a wall perpendicular to the beam uses a `YZ` slice;
- a longitudinal mortise or housing wall uses an `XZ` slice;
- a horizontal bearing face uses an `XY` slice;
- a peg hole unwraps physical distance along its axis against angle around the
  real bore centre.

The entering member receives its own local length but the same tree-growth
rules. Its shoulder is a crosscut slice, its cheeks are longitudinal slices,
and its offset bore is a cylindrical sample of that member. A feature does not
switch to an arbitrary “end-grain texture” merely because it is a cut.

The eight atlas tiles are physical projections, not visual variants:

1. mortise cross wall;
2. mortise long wall;
3. mortise floor and housing bearing bed;
4. receiving-member drawbore cylinder;
5. tenon shoulder;
6. tenon cheek across local Y;
7. tenon cheek across local Z;
8. entering-member drawbore cylinder.

Each tile has base colour, normal, restrained height, anatomical identity, and
semantic proof lanes. There is no random source. The atlas does not contain
damage, checks, chips, grime, repair, or a roughness texture.

## Boolean-safe face ownership

The workflow does not infer new faces from normals, proximity, or triangle
interpolation after the fact.

1. The target starts with its approved timber material and the clean-cut
   material.
2. A closed cutter receives a temporary magenta marker material.
3. An Exact Difference Boolean runs with material transfer enabled.
4. Faces carrying the transferred marker are visited immediately.
5. Every corner of each generated face receives both a semantic integer and a
   direct atlas UV chosen from the face orientation and physical vertex
   position.
6. The marker is replaced by the cut material, the cutter object and its mesh
   datablock are deleted, and the unused marker material is removed.

This makes the generated-face boundary discrete. Original timber corners keep
their original outer or end-face classification; a triangle cannot blend a
mortise wall into a neighboring outer face.

The corner-domain taxonomy is:

| ID | Meaning |
| ---: | --- |
| 0 | original outer timber |
| 1 | original end |
| 2 | fresh crosscut |
| 3 | mortise wall |
| 4 | mortise floor |
| 5 | housing wall |
| 6 | housing bearing bed |
| 7 | peg-bore wall |
| 8 | tenon cheek |
| 9 | tenon shoulder |
| 10 | riven peg side |
| 11 | peg end |

The saved names are `sinc_wood_surface_class` for the integer taxonomy and
`sinc_joinery_uv` for the direct cut-atlas coordinate. Both live on the
`CORNER` domain.

## Geometry construction

The receiving timber is a data-copy of the approved clean structural beam, so
its bow, hewn silhouette, chamfers, growth coordinates, and longitudinal grain
remain intact. Three applied Boolean cuts make the housing, blind mortise, and
receiving bore.

The entering member is one closed manifold mesh. Its outer body, shoulder ring,
and reduced tenon share vertices; there are no overlapping boxes or internal
faces. The fourth applied Boolean makes its deliberately offset drawbore.

The pin is separate geometry with eight flat riven sides, a full-diameter
driving section, a finite taper, and a small tip. It is not a smooth cylinder
with a wood colour.

## Shader contract

The outer faces continue to use `IGGY_MAT_StructuralOakTimberV1`. Clean cuts
use `IGGY_MAT_StructuralOakJoineryCutsV1`, whose graph is intentionally small:

- explicit `sinc_joinery_uv`;
- packed sRGB base-colour atlas;
- packed non-colour normal atlas;
- Principled BSDF with metallic `0` and constant roughness `0.76`.

The height and semantic maps are exported for downstream authoring and Unreal
reconstruction but are not used to counterfeit geometry in this Blender
acceptance proof. No baked lighting or ambient occlusion is in base colour.

## Build and acceptance

Generate the final `3072 × 1536` atlas, then build the packed Blender asset:

```sh
/Applications/Blender.app/Contents/Resources/5.1/python/bin/python3.13 \
  assets/creative/materials/structural_oak_joinery_v1/generate_structural_oak_endgrain_v1.py

/Applications/Blender.app/Contents/MacOS/Blender \
  --background --factory-startup \
  --python assets/creative/materials/structural_oak_joinery_v1/build_structural_oak_joinery_v1.py
```

The final validation reopens the saved file and checks all three meshes for
manifold topology, confirms all twelve semantic classes, verifies packed
images, proves the temporary material is gone, and checks all four renders.

The current proof establishes clean construction logic and material continuity.
It does not claim final joinery wear. Compression, bruised arrises, tool
overrun, end checks, peg polish, and repair history belong to the separate
damage library after this clean master passes.
