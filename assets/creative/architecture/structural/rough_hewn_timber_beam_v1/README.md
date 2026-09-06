# World Asset 001 — rough-hewn timber beam

This directory owns exactly one polished master beam. It is a reusable,
joinery-ready structural timber, not a wall kit, prop arrangement, or material
demo. Its first acceptance gate is neutral clay.

The asset is 4.2 m long. Its nominal section tapers from 300 by 340 mm at the
left end to 288 by 329 mm at the right end. Those dimensions deliberately sit
between the historically common 8 by 8 and 10 by 10 inch hewn sections while
matching the larger stock required by the giant-house kit.

## What the geometry owns

- a 12 mm natural bow and 7 mm lateral crook that return to the joinery-ready
  end centres;
- 1.15 degrees of total twist;
- four independently hewn faces;
- four independently varying arrises, with 6–19 mm surviving chamfers;
- 13 broad target-plane anchors per face with independent diagonal sweeps;
- two knots with recessed knot bodies and visible socket edges;
- five seasoning checks that originate on exposed end grain and narrow inward;
- two restrained edge losses;
- three protected joinery zones;
- stable metre-valued longitudinal, face identity, end-distance, hewing-depth,
  joinery, and UV lanes.

Fine grain, growth rings, pores, colour, roughness, and scene damage do not
belong to this build. Those are material-only details after the clay geometry
passes.

## Blender construction

`build_rough_hewn_timber_beam_v1.py` builds a deterministic 145-section
quad-strip. Each section has 17 samples for each of the four hewn faces plus
the four connecting arrises. The face samples follow fitted broad target
planes, not sampled noise or repeated tool-shaped cavities. The profile retains
57 measured broad-axe observations for the later fine-normal striation and
tool-signature pass; clay comparisons explicitly rejected using them as local
geometry displacement.

The script keeps two objects:

- `IGGY_WA001_RoughHewnTimberBeam_CleanSource` is the hidden, modifier-free
  semantic source;
- `IGGY_WA001_RoughHewnTimberBeam_Master` is the visible evaluated asset.

The master uses nine closed manifold Boolean cutters in a fixed order:

1. five end-check cutters;
2. two knot-socket cutters;
3. two edge-loss cutters;
4. a 0.45 mm selective bevel;
5. face-influenced weighted normals.

The Boolean stack is editable. The bevel and weighted-normal tail preserves the
broad hewn faces rather than rounding the entire timber.

## Build

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  --background --factory-startup \
  --python \
  assets/creative/architecture/structural/rough_hewn_timber_beam_v1/build_rough_hewn_timber_beam_v1.py
```

For a fast geometry-only diagnostic:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  --background --factory-startup \
  --python \
  assets/creative/architecture/structural/rough_hewn_timber_beam_v1/build_rough_hewn_timber_beam_v1.py \
  -- --skip-renders
```

The full build writes:

- `output/rough_hewn_timber_beam_v1.blend`;
- `output/rough_hewn_timber_beam_v1_manifest.json`;
- front, back, top, bottom, and both end proofs;
- two perspective proofs;
- wireframe and silhouette proofs;
- a grazing-light tool-mark proof;
- an end-check close-up.

## Verification

```sh
python3 -m unittest \
  tests.unit.rough_hewn_timber_beam_v1_blend_tests
```

The test reopens the saved Blender file and evaluates the live modifier stack.
It verifies physical dimensions, manifold output, the clean-source boundary,
modifier order, semantic attributes, joinery zones, anatomy counts, material
scope, and every required review image.
