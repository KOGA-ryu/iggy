# Research-to-intent ledger

This ledger records which observed fact becomes which authored behavior. It
also records what the sources do *not* justify.

## White-oak growth increments

### Observation

Pace et al. describe `Quercus alba` as ring porous. Vigorous annual increments
may reach 2 mm or more; suppressed increments may be only 120 micrometres and
contain only earlywood.

Source:

- Pace, Dutra, Marcati, Angyalossy, and Evert, “Seasonal Cambial Activity and
  Formation of Secondary Phloem and Xylem in White Oaks,” *Forests* 14(5),
  920 (2023): <https://doi.org/10.3390/f14050920>

### Intent

- Author a nonuniform annual-increment sequence.
- Preserve both slow and vigorous eras.
- Allow suppressed increments to become visually quiet or subpixel.
- Do not use one evenly spaced sine wave.
- Do not make all rings equally dark.

### Implementation

- 64 explicit champion widths;
- 8 deterministic epoch scales;
- 832 compiled increments across the authoring radius;
- one shared boundary table used by side and end evaluation.

### Not justified

- a decorative 6–16 mm ring pitch;
- perfectly periodic rings;
- a centred bullseye in a rectangular cant.

## Earlywood and latewood vessels

### Observation

The same study reports earlywood vessels at `154 ± 28 µm` and latewood vessels
at `15 ± 3 µm`. Earlywood vessels mark the beginning of the annual increment.
Latewood vessels are solitary or occur in radial multiples of two to four in a
dendritic arrangement.

### Intent

- Place earlywood pore response near the beginning of each ring.
- Change spacing, diameter, presence, and phase with explicit cycles.
- Represent latewood primarily as density and value organization at game
  resolution.

### Implementation

- seven pore-spacing values;
- eight measured diameter deviations;
- eleven explicit presence values, including intentional absences;
- eight phase values;
- area-preserving subpixel response.

### Not justified

- enlarged pinholes;
- evenly spaced polka dots;
- pores scattered through latewood with no ring relationship.

## Rays

### Observation

Pace et al. report uniseriate rays and aggregate rays more than ten cells wide.
The USDA Forest Products Laboratory identifies conspicuous rays as part of
oak's grain character.

Sources:

- Pace et al. (2023), above.
- Sander and Rosen, “Oak: An American Wood,” USDA Forest Service FS-247:
  <https://www.fpl.fs.usda.gov/documnts/usda/amwood/247oak.pdf>

### Intent

- Use two visibly different ray families.
- Make broad rays rare and individually authored.
- Keep narrow rays filtered and subordinate.
- Show only a few side-visible ray flecks where the cut geometry permits.

### Implementation

- 15 explicit wide-ray angles and widths;
- narrow rays at a 5.8-degree nominal step with eight authored offsets;
- five finite side flecks.

### Not justified

- radial spokes of identical width;
- dense bright starbursts;
- ray marks on every side face.

## Color authority

### Observation

The accepted structural door material reverse-derived twenty wood colors and
their broad value grouping from the reverse of Metropolitan Museum object
55.61.170.

Source:

- <https://www.metmuseum.org/art/collection/search/468492>
- `../structural_oak_door_v1/profiles/structural_oak_door_v1.json`

### Intent

- Preserve project color continuity.
- Reuse the exact palette rather than invent “oak brown.”
- Build many blended shades inside one timber.
- Keep large quiet passages between accents.

### Implementation

- the generator loads the door profile at build time;
- twenty palette entries are converted from sRGB to linear;
- 17 broad and short color passages cross the four faces;
- annual increments and finite tracks add smaller-scale color movement.

### Not justified

- copying photographed lighting;
- copying photographed wear;
- assigning one palette color to an entire face;
- orange saturation as a substitute for wood.

## Beam geometry authority

### Observation

The approved beam already owns:

- physical dimensions;
- four hewn faces;
- 57 hewing observations;
- two branch knots;
- five end checks;
- two edge losses;
- per-face and per-vertex semantic attributes.

Source:

- `../../architecture/structural/rough_hewn_timber_beam_v1/profiles/rough_hewn_timber_beam_v1.json`

### Intent

- Deflect grain at the two existing knots.
- Select a minority of hewing observations for surface signature.
- Never draw another set of checks or chips in the material.
- Route atlas faces using semantic face ownership.

### Implementation

- front knot: `X=-0.72 m`, cross `0.041 m`, influence `0.23 m`;
- top knot: `X=1.06 m`, cross `-0.058 m`, influence `0.14 m`;
- 17 selected axe events;
- shader reads `sinc_timber_face_id`;
- shader reads `IGGY_TimberUV` as metre coordinates.

## Blender shader method

### Written method

Blender's documentation describes image textures as functions of an input
vector, usually supplied by texture coordinates, and documents the UV Map node
as the way to choose a particular UV map. Blender's geometry attribute system
also supports named attributes and face-corner two-dimensional UV data.

Sources:

- Blender Manual, “Image Texture Node”:
  <https://docs.blender.org/manual/en/4.3/render/shader_nodes/textures/image.html>
- Blender Manual, “Using UV Maps”:
  <https://docs.blender.org/manual/en/4.0/modeling/meshes/uv/applying_image.html>
- Blender Manual, “Attributes”:
  <https://docs.blender.org/manual/en/3.6/modeling/geometry_nodes/attributes_reference.html>

### Intent

- Keep the expensive anatomical authoring in deterministic maps.
- Keep physical routing and material response in a reusable node graph.
- Use simple exportable primitives: image textures, UV coordinates, math,
  normal map, and Principled BSDF.

### Implementation

- side and end images are independent inputs;
- face identity chooses the appropriate atlas;
- constant roughness is a material parameter;
- tangent normal maps carry restrained anatomical relief;
- packed images make the review `.blend` self-contained.

### Unreal translation

The Blender node graph is not expected to transfer verbatim. Unreal receives
the same texture lanes and reconstructs the same mapping contract:

1. timber-local longitudinal metres;
2. face/material identity;
3. side-row or end-column selection;
4. base color and normal sample;
5. uniform roughness parameter.

That is a direct material-function translation, not a screenshot or baked
Blender lighting pass.

