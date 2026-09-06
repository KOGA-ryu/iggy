# Lime Plaster and Giant Rubble: Research-to-Intent Contract

This dossier replaces the old AI-comparison capture as the canonical source of
material intent. No image—generated or photographed—is sampled, reduced,
projected, or tiled by the production build. Written conservation guidance
establishes construction; the project brief establishes giant-house scale; the
authored recipe establishes composition.

## Lime plaster is a construction stack

The U.S. National Park Service's *Preservation Brief 21: Repairing Historic
Flat Plaster* describes traditional lime plaster as lime, sand, water, and
animal hair in the coarse work. It distinguishes:

1. a coarse, keyed scratch coat;
2. a second coarse brown coat that establishes the wall plane;
3. a lime-rich finish coat with little aggregate and no fibre.

The same brief gives `3/8 in` for each coarse coat and `1/8 in` for the finish.
The production metric equivalents are therefore:

| Layer | Source thickness | Authored thickness |
|---|---:|---:|
| scratch coat | `3/8 in` | `9.525 mm` |
| brown coat | `3/8 in` | `9.525 mm` |
| finish coat | `1/8 in` | `3.175 mm` |
| complete three-coat build | `7/8 in` | `22.225 mm` |

Lime Green's written lath-plaster guide independently describes a diamond
scratch around `45°` and a fine finish skim often no more than `3 mm`. The
scratch reveal therefore needs directional grooves; it must not be a darker
copy of the finish texture.

Historic England documents surviving external vernacular finishes that can be
less than `10 mm` total. That is a separate thin-render preset. It does not
invalidate the NPS interior three-coat specimen and must not be silently
averaged into it.

## Aggregate changes with depth

The finish coat uses a fine aggregate and reads as a quiet lime body with:

- broad carbonation and drying fields;
- overlapping trowel compression arcs;
- sparse sub-millimetre grains;
- occasional shallow pores;
- no universal dirt or photo grain.

The brown coat contains more visible sand and float drag. The scratch coat is
coarser, warmer, more open, and owns the `45°` key grooves. Each layer has its
own palette, roughness range, height range, normal response, and line logic.

## Rubble masonry is laid, not scattered

The project scale contract requires exposed stones approximately
`0.30–1.20 m` across. The champion tile is `4 × 4 m`, containing seven
contiguous courses and thirty-six authored stones. Each course owns its joints;
variation may mirror a champion outline or change phase and pigment, but may
not generate a new bond.

Historic England's *Repointing Brick and Stone Walls* states that mortar-joint
character contributes as much to masonry appearance as the units. It documents
flush rubble pointing finished with an open-grained bristle-brush surface.
Mortar therefore owns:

- a continuous body colour independent of stone colour;
- a real recess below stone faces;
- sparse sand aggregate;
- directional brush response;
- contact occlusion at stone boundaries;
- no black outline painted into every joint.

The authored giant-house joint widths are `38 mm` perpends and `50 mm` beds.
They are intentionally broader than fine dressed-stone work so the construction
remains readable against `0.38–1.14 m` units at gameplay distance.

## Champion stone vocabulary

Five reusable outlines replace the rounded-rectangle pattern:

1. quiet bedded slab — long, stable bearing edges and minimal crown;
2. broad limestone plane — one dominant face with a displaced shoulder;
3. laminated bedding face — compressed height and explicit strata;
4. fractured cool face — asymmetric missing corner and opposing planes;
5. projecting traversal ledge — strong lower bearing plane and readable top.

Each champion is a hand-authored normalized polygon. Stones derive controlled
siblings by mirroring, modest course shear, and phase changes. Random blobs,
Voronoi cells, and identical rounded boxes are prohibited.

## Graphic surface hierarchy

The surface is read in this order:

1. wall opening, reveal patch, projecting traversal silhouettes;
2. course rhythm, stone outlines, mortar territories;
3. dominant planes and restrained colour grouping per stone;
4. selective arris fragments and bedding lines;
5. sparse aggregate, pores, and micro-normal response.

Linework is caused by form: a facing arris, bedding seam, fracture plane, or
trowel drag. It is not a universal dark edge filter. Quiet regions must remain
between accents.

## Geometry and shader boundary

Geometry owns categorical construction state on the face domain:

- masonry;
- flush lime mortar;
- scratch coat;
- brown coat;
- lime finish.

Face-domain state prevents a categorical layer identity from becoming a
triangle-interpolated gradient. Continuous transition-edge emphasis and
optional damp/soot masks remain separate float lanes. Boolean or topology
changes must regenerate the attributes.

The shader owns light response, not damage placement. It samples independently
authored plaster and masonry maps, reconstructs three dielectric responses
(plaster, mortar, stone), combines their normals and metre heights, and blends
only from the geometry-owned state.

## Written sources

- U.S. National Park Service, *Preservation Brief 21: Repairing Historic Flat
  Plaster—Walls and Ceilings*:
  https://www.nps.gov/orgs/1739/upload/preservation-brief-21-flat-plaster.pdf
- Historic England, *Repointing Brick and Stone Walls: Guidelines for Best
  Practice*:
  https://historicengland.org.uk/images-books/publications/repointing-brick-and-stone-walls/
- Historic England, *Lime Finishes in a Changing Climate* symposium, including
  the documented thin historic plaster evidence:
  https://historicengland.org.uk/advice/technical-advice/buildings/maintenance-and-repair-of-older-buildings/what-s-the-point-2-lime-finishes-in-a-changing-climate-symposium/
- Lime Green, *Plastering onto Laths*:
  https://www.lime-green.co.uk/support/knowledgebase/plastering_onto_laths
- Blender Manual, *Displacement*:
  https://docs.blender.org/manual/en/4.5/render/materials/components/displacement.html

