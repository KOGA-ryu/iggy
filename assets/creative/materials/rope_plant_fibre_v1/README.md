# Rope and plant-fibre material v1

This directory contains a deterministic three-strand regular-lay rope source.
It produces fine-lashing, utility-line, and heavy-hawser presets with four
authored surface variations each, plus one curve-native Blender Geometry Nodes
system shared by all three physical sizes.

The source has two deliberate owners:

- `generate_rope_plant_fibre_v1.py` authors the tiling colour, fibre, response,
  identity, and stylization atlases;
- `build_rope_plant_fibre_v1.py` authors structural relief as real geometry:
  three compacted strand hulls, seven counter-laid yarn bundles per strand,
  metre-based twist, curve-local data, and optional sparse flyaways.

It does not generate knots, broken ends, dirt, or damage.

Generated channels:

- base colour;
- tangent-space normal;
- 16-bit signed-range height;
- ORM (ambient occlusion, roughness, metallic);
- identity masks (strand body, yarn groups, fibre ribbons);
- RGB primary-strand identities;
- long-fibre, short-fibre, and optional flyaway-spawn masks;
- large-cavity, cavity-core, and selective-sheen response masks;
- bundle-event diagnostics for event coverage and merge/split inspection;
- stylization masks (separator ink, crown lift, quiet field);
- a proof sheet showing the layers separately and combined;
- a neutral analytic three-lobed cylinder proof.

The Blender build adds:

- `IGGY_GN_RopePlantFibre_v001`, a reusable modifier group;
- `IGGY_MAT_RopePlantFibre_v001`, with broad colour, authored finite fibre
  tracks, independent roughness, bump, and restrained fibre sheen;
- 12 mm, 32 mm, and 70 mm proof objects driven by the same node group;
- a hidden straight 32 mm validation object;
- packed source atlases and four neutral-light proof renders;
- a manifest with hashes, dimensions, hierarchy, packing, coordinates, and
  render provenance.

The authored construction data is split into three files:

- `three_strand_regular_lay_v1.json` owns physical size, lay, colour passages,
  quiet fields, and the four-variation contract;
- `rope_bundle_tracks_champion_v2.json` owns asymmetric strand crowns and
  5/8/11 coarse bundle tracks per visible strand, including width classes,
  counter-twist rates, sub-ridges, merges, splits, burial, fade, and flatten;
- `rope_fibre_tracks_champion_v2.json` owns finite long blades, their authored
  companions, short staple fragments, colour, height, sheen, valley bridging,
  and optional flyaway candidates.

Coordinates are curve-local: longitudinal distance is expressed in metres and
the second coordinate is a normalized turn around the rope circumference.
Geometry and shader authors must preserve those coordinates through curve
deformation.

The structural source resamples by physical length before it calculates twist.
It never uses point index as distance. Three strand hulls occupy the compacted
mass; the 21 yarn curves only interrupt their crowns, preventing the material
from reading as a bundle of independent plastic wires. Yarn turn opposes rope
turn. Two low-amplitude, incommensurate phase bands keep manufactured regularity
without producing a perfect procedural stamp.

Production generation streams one preset at a time and fills each four-row
atlas variation-by-variation. The default 1536 by 384 tile build therefore
does not retain three complete preset atlases and twelve source variations in
memory at once.

Run:

```sh
python3 assets/creative/materials/rope_plant_fibre_v1/generate_rope_plant_fibre_v1.py
```

Then build the reusable Blender source and proofs:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  --background --factory-startup \
  --python assets/creative/materials/rope_plant_fibre_v1/build_rope_plant_fibre_v1.py
```

The build writes:

- `output/rope_plant_fibre_v1.blend`;
- `output/rope_plant_fibre_v1_blender_manifest.json`;
- `output/rope_plant_fibre_v1_{hero_oblique,construction_side,grazing_response,scale_family}.png`.

Targeted verification:

```sh
python3 -m unittest \
  tests.unit.rope_plant_fibre_v1_generator_tests \
  tests.unit.rope_plant_fibre_v1_blend_tests
```

Research and dimensional provenance are recorded in
`RESEARCH_INTENT.md` and `profiles/rope_plant_fibre_v1.json`.
