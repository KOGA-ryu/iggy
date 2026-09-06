# Structural oak timber v1 — quality review

## Acceptance scope

This pass judges one material on one already-approved beam. It does not judge a
wall kit, furniture set, damage library, aging system, or environmental
overlay.

## What is materially stronger than the previous wood work

### Side/end continuity

The side and end maps share one digest-identical ring table. This removes the
most common procedural shortcut: longitudinal lines on the side and unrelated
concentric decoration on the end.

### Measured scale

Ring, pore, and ray dimensions are authored in metres from white-oak anatomy.
Subpixel structures are filtered rather than inflated.

### Layer ownership

Color, growth identity, selected grain, knot influence, ray response, tool
response, quiet fields, and normal relief can be reviewed independently.

### Authored variation

The champion has:

- 64 explicit annual increments;
- 832 compiled increments across the cant;
- 27 finite grain tracks;
- 9 track events;
- 17 broad/short color passages;
- 15 wide rays;
- 5 side ray flecks;
- 17 selected tool signatures;
- 5 quiet fields.

That variation is structured. None of those counts comes from a random seed.

## Repair passes

### Pass 1 — measured profile

Replaced inherited decorative millimetre-to-centimetre anatomy assumptions
with measured ring and vessel dimensions.

### Pass 2 — common growth volume

Made side and end evaluation share pith controls and the annual-ring boundary
table.

### Pass 3 — source validation

A focused test found a front tool-line event targeting mark 15 in a zero-based
fifteen-mark list. Repaired the champion to target valid mark 14 before render.

### Pass 4 — first material book

Confirmed the layers were present but found the flat proof could not expose
the narrow arris mapping path.

### Pass 5 — real-beam application

The first beam render exposed black rails on face class 6. The shader had four
side rows and two end columns but no explicit route for the hewn chamfer class.

### Pass 6 — explicit arris lane

Added a quiet accepted-palette chamfer color and flat tangent normal selected
by semantic face identity. This removed the invalid out-of-bounds texture
sample without stretching a planar face across the arris.

### Pass 7 — knot color space

Converted the recessed knot body's accepted sRGB palette entry into scene
linear before sending it to the shader.

### Pass 8 — intra-face color

Added eleven shorter warm/cool/grey/umber passages so each face contains
multiple interacting value fields rather than one shade per block.

### Pass 9 — neutral proof lighting

Reduced the warm/cool cast of the stage lights so the proof describes material
color rather than theatrical lighting.

## Honest limits

- Pores are intentionally subtle at 400 mm per 2048 pixels. A close hero asset
  could use a higher-resolution end trim without changing the growth model.
- The narrow hewn arrises use a quiet color lane, not true adjacent-face
  triplanar blending. This is stable and honest, but a future Geometry Nodes
  mesh can store adjacent face ownership for a richer arris transition.
- Boolean-generated internal bearing faces inherit the nearest available
  semantic route. A dedicated `cut_face_id` is still the cleaner long-term
  solution for fresh cuts, mortises, and tenons.
- The two recessed knot bodies use a restrained separate material rather than
  a dedicated knot atlas.
- No Unreal material function has been built in this pass. The lane and
  coordinate contract is ready for it.

## Remaining work that must stay separate

- end-grain/joinery overlay atlas;
- fresh cut versus weathered outer-face state;
- damage/check library;
- age and stain;
- soot and wetness;
- distance-aware simplification;
- Unreal material-function parity proof.

None of those should be folded into the base timber color generator.

