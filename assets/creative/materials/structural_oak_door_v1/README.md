# Structural Oak Door v1

This is a door-specific structural-timber colour system. Its source of truth is
the real fourteenth-century European door used during the model work:
[The Met, object 55.61.170](https://www.metmuseum.org/art/collection/search/468492).
The reverse view is the primary colour and fibre-direction reference.

It does not use the `wood_plank_v2` floor pattern, a synthetic cathedral-grain
formula, random noise, or AI-generated imagery.

The acceptance fixture is the existing double-braced giant-house door from the
SINC Blender node library. The fixture remains a native Geometry Nodes
assembly. Its stored UV map and timber seed place a different reference-derived
field on each plank, ledge, and brace after those parts have been rotated into
construction position.

## Reference-to-texture workflow

1. Inkblotter eyedropper data records twenty observed wood colours from the
   Met reverse, with the source pixel position and descriptive label retained.
2. The generator reads selected wood-only regions from the real reverse image.
   Each crop is reduced to a coarse grid using median colour, rather than
   copied as a photograph. This rejects cracks, holes, hardware, and
   micro-surface detail while preserving the door's irregular colour
   relationships.
3. Every reduced colour cell is pulled toward one of the twenty measured wood
   samples. This prevents background, iron, and photographed lighting outliers
   from silently becoming new material colours.
4. All twenty measured samples also form a smooth spatial base field. Twelve
   board variants transform that field differently and emphasize different
   observed regions: charcoal upper wood, pale lower wood, cool centre wood,
   ochre edge wood, and the dark or pale ledges.
5. Each of the twenty measured colours is laid again as a narrow, softly
   blended longitudinal glaze. The glazes use the sample positions to drift
   and terminate at different points, so a single timber contains many related
   shades without becoming an evenly spaced stripe pattern.
6. Three broad colour passages are hand-authored over every variant. These
   retain large quiet areas and prevent the sampled image from becoming the
   design by itself.
7. Five longitudinal boundaries are selected from coherent transverse changes
   in each reduced real-door crop. Four additional fibre segments are authored
   per variant. Both are colour/ink only: they are not cracks, height, normal,
   or roughness.
8. The generator compiles the twelve variants into colour and ink atlases. The
   Blender material hashes `sinc_seed` to select a tile, maps the tile through
   the timber's construction UV, and packs both generated atlases into the
   resulting `.blend`.

The real photograph is therefore a build-time authority, not a photo texture
projected onto the model. The saved acceptance asset has no runtime dependency
on the donor repository or the reference image.

## Included in this pass

- the exact twenty-colour observed reference palette;
- broad cool/warm and dark/pale fields from the real door;
- twelve independently seeded layouts;
- large quiet passages between changes;
- restrained longitudinal fibre colour;
- a separate sparse fibre-ink lane;
- neutral-light colour, linework, and combined proofs.

## Explicitly excluded

- the previous hardwood-floor pattern;
- periodic growth rings, waves, or zebra grain;
- random procedural noise;
- roughness maps or roughness variation;
- normal, bump, or height detail;
- damage, wear, scratches, dings, scuffs, cracks, chips, stains, and
  photographic weathering.

The proof material uses a uniform diffuse shader. Geometry and neutral lighting
describe the door's form; the wood system contributes only colour and
linework.

## Build

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  --background \
  --python assets/creative/materials/structural_oak_door_v1/build_structural_oak_door_v1.py \
  -- \
  --source /absolute/path/to/sinc_blender_node_library_v1.blend
```

The builder writes a self-contained Blender file, the compiled atlases, a
twenty-swatch reference sheet, three proof renders, and validation manifests
under `output/`.
