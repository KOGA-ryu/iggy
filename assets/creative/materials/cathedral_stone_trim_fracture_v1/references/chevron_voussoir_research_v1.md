# Chevron Voussoir Portal Research Ledger V1

Accessed: 2026-07-29

This ledger supports one intact chevron-voussoir portal order. It does not
authorize fracture, erosion, soot, limewash, damage, or a complete historical
reconstruction. Source images were inspected and translated into text for
comparison. No third-party pixels are retained by the package or used in
runtime textures.

## Evidence vocabulary

- `surveyed`: a stated measurement from an identified physical specimen.
- `technical_guidance`: a working recommendation rather than a specimen
  measurement.
- `qualitative_authority`: a reliable construction or terminology statement
  without a numeric measurement.
- `comparative_finish`: a professional asset or process used to judge finish,
  not historical fact.
- `authored_translation`: a project choice constrained by the sources.
- `unknown`: not authorized and not silently replaced.

## Retained sources

### S01 — Old Sarum architectural and sculptured stone catalogue

- Title: *Architectural and Sculptured Stonework*
- Authors: Allan Brodie and David Algar
- Repository: Bournemouth University Research Online
- URL:
  `https://eprints.bournemouth.ac.uk/37564/3/Arch%20%20Sculp%20Stone%2021%2004%2010.pdf`
- Relevant pages: PDF pages 20–21, catalogue entries 54–72
- Evidence class: `surveyed` dimensions and `qualitative_authority`
  construction description
- Rights: no reusable-pixel license was established in the document; retain
  citations and textual measurements only
- Object family: reused twelfth-century chevron voussoirs and related
  rectangular chevron blocks associated with Old Sarum material

Surveyed voussoirs:

| Catalogue | Height | Tapered width | Depth | Special note |
| --- | ---: | ---: | ---: | --- |
| 54, stone 14 | 190 mm | 250–190 mm | 350 mm | chevron moulding |
| 55, stone 15 | 200 mm | 180–140 mm | 270 mm | chevron moulding |
| 56, stone 16 | 200 mm | 175–130 mm | 290 mm | chevron moulding |
| 57, stone 17 | 200 mm | 210–160 mm | 200 mm | chevron moulding |
| 58, stone 18 | 200 mm | 285–220 mm | 210 mm | rear slot about 130 x 130 mm |
| 59, stone 19 | 200 mm | 190–140 mm | 240 mm | chevron moulding |
| 60, stone 20 | 200 mm | 145–115 mm | 280 mm | junction between paired arches |
| 61 | 230 mm | 220–165 mm | 260 mm | accession 2002.55 |

The source states that the group:

- came from arches;
- used a chevron pattern consisting of a roll, a hollow, and a roll;
- included underside projections that connected to a chevron on a separate
  block;
- is consistent with the decorated outer order of a substantial arch,
  probably a gallery rather than the main arcade.

Catalogue 62 is superficially similar but has a different chevron pattern and
is not used as authority for this version.

Related rectangular blocks, catalogue 63–71, measure 200–370 mm high,
125–300 mm wide, and 150–300 mm deep. The source explicitly says their taper
is too slight for voussoirs and that they likely came from a flat surface
rather than a pier. They therefore do not authorize a matching portal jamb.

Transfer limits:

- The catalogue does not label which of each two widths is inner or outer.
  Treating the larger width as the broad outer end is a necessary geometric
  interpretation of a wedge, not an additional surveyed statement.
- It does not publish the complete arch radius, voussoir count, joint width,
  chevron direction, roll diameter, hollow depth, arris radius, or relief
  depth.
- The soot and possible limewash described on reused stones are excluded from
  the intact capability.
- The group contains stones from arches in the plural. Do not claim that all
  eight measurements belong to one reconstructed arch.

Decision enabled:

- use catalogue 55 as the dimensional champion for a clean, ordinary wedge:
  200 mm radial height, 140 mm inner chord, 180 mm outer chord, and 270 mm
  wall depth;
- retain `roll / hollow / roll` as the only source-backed moulding sequence;
- reject the old 360 mm-wide smooth ribbon and its 83.3 mm independent
  diamond repeat.

### S02 — Gloucester Cathedral south transept archaeology

- Title: *The archaeology of the south transept of Gloucester Cathedral,
  2002–3*
- Publisher: Bristol and Gloucestershire Archaeological Society
- URL: `https://www.bgas.org.uk/publications/gcar/gcar-02-c.pdf`
- Relevant pages: report pages 5–10, especially PDF lines corresponding to
  worked stones 400–402
- Evidence class: `surveyed` dimensions and `qualitative_authority` finish
- Rights: no reusable-pixel license established; text citations only

Relevant observations:

- Romanesque ashlar tooling survives as vertical or diagonal striated marks.
- Worked stone 401 measures 350 x 160 x 210 mm.
- Its chevron profile is described as
  `fillet / half-roll / angle fillet / lozenge / angle fillet / half-roll`,
  probably symmetric with a final fillet.
- It preserves soffit and therefore functioned as a voussoir.
- The source estimates its former arch at only 300–400 mm diameter.
- Its dag crosses the joint, while later Gloucester chevron centers the dag in
  the stone. Pattern-to-joint phase is therefore an intentional construction
  choice, not an arbitrary texture repeat.
- Worked stone 400 measures 250 x 200 x 190 mm and preserves part of the same
  profile. Its flat bottom shows it was not a voussoir; the report says it
  could have been a jamb stone.
- Worked stone 402 retains a 140 mm roll moulding and was cut about five
  degrees out of perpendicular.
- Romanesque rolls retain vertical striated tooling while flat surfaces retain
  diagonal striated axe marks. Later fourteenth-century work is much finer,
  with faint claw marks where any tooling survives.

Transfer limits:

- This is a different building, stone group, profile, and arch scale from S01.
- Do not transfer the 140 mm roll diameter, the 300–400 mm arch diameter, or
  worked-stone-400 jamb dimensions into the Old Sarum champion.
- The source proves that joint phase, soffit treatment, roll/fillet anatomy,
  and tool direction require explicit decisions.

Decision enabled:

- store pattern phase per voussoir;
- identify front face, soffit, roll, hollow, and joint separately;
- keep tooling direction attached to the carved surface rather than applying
  one world-aligned noise field;
- do not claim that an Old Sarum jamb has been measured.

### S03 — Corpus of Romanesque Sculpture Chevron Guide

- Title: *The Chevron Guide*
- Publisher: Corpus of Romanesque Sculpture in Britain and Ireland
- URL: `https://www.crsbi.ac.uk/resources/the-chevron-guide`
- Relevant sections: introduction; position and direction; carving chevron
  voussoirs; moulding profiles; treatment of an order edge
- Evidence class: `qualitative_authority`
- Rights: site copyright; links and textual findings only

Construction grammar:

- Chevron is three-dimensional zigzag ornament formed by one or more rolls.
  Two-dimensional ornament without rolls is only zigzag.
- A row of chevrons may occupy the face, soffit, or edge of an arch order.
- Lateral chevrons lie parallel to the decorated surface; frontal chevrons
  project at right angles.
- A sculptor carving lateral face chevron normally, though not always, carves
  one chevron on each voussoir.
- The point may face the broad outer end (`centrifugal`) or the narrow inner
  end (`centripetal`).
- Chevron orders commonly sequence roll and hollow mouldings.
- An edge may remain plain, carry a roll, be undercut into a serrated edge, or
  receive another explicit ornament. These are distinct designs.

Decision enabled:

- this capability uses one chevron unit per stone;
- it uses one row of centripetal lateral face chevron as an
  `authored_translation`;
- it does not invent a soffit or edge chevron;
- each stone owns its phase, so a repeat cannot drift across construction
  joints;
- the roll/hollow/roll pattern must produce real depth under neutral light,
  not merely a dark diamond in base color.

### S04 — Blender written geometry and shader guidance

Official sources:

- BMesh API:
  `https://docs.blender.org/api/current/bmesh.html`
- Spin tool:
  `https://docs.blender.org/manual/en/latest/modeling/meshes/tools/spin.html`
- Bevel modifier:
  `https://docs.blender.org/manual/en/latest/modeling/modifiers/generate/bevel.html`
- Normal Map node:
  `https://docs.blender.org/manual/en/dev/render/shader_nodes/vector/normal_map.html`
- Image Texture node:
  `https://docs.blender.org/manual/en/4.3/render/shader_nodes/textures/image.html`
- Attribute node:
  `https://docs.blender.org/manual/en/latest/render/shader_nodes/input/attribute.html`
- Color management:
  `https://docs.blender.org/manual/en/latest/render/color_management.html`
- Packed data:
  `https://docs.blender.org/manual/en/latest/files/blend/packed_data.html`
- Evidence class: `technical_guidance`
- License: Blender documentation is CC-BY-SA

Socket-level and geometry consequences:

- Spin creates a connected rotational extrusion. It is suitable for a lathed
  arch profile, but it does not by itself create separate load-bearing
  voussoir stones or joints. The rejected ribbon is exactly the failure that
  would result from stopping at this operation.
- Build each wedge from its own radial bounds, depth bounds, and face grid.
  Use `Mesh.from_pydata` or BMesh, validate the mesh, and retain one object or
  one explicitly identified mesh island per stone.
- Use explicit face-corner UV data or named geometry attributes. Do not infer
  stone identity from a triangle's interpolated position.
- The Bevel modifier's width is a real distance and its segment count adds
  edge loops. Since no arris radius is measured for S01, no global Bevel
  modifier is authorized for the champion stones.
- An Image Texture node needs the same UV mapping used by the tangent-space
  Normal Map node.
- Normal, height, roughness, masks, and other numeric texture data must be
  marked `Non-Color`.
- A tangent-space normal image must feed a `ShaderNodeNormalMap`; merely
  packing an unused image does not produce normal response.
- Named geometry data may be read by an Attribute node. A selector node that
  is not linked to material behavior is not an implementation.
- Eligible external images can be packed into the `.blend`; packing occurs
  with the save. Reopen validation must inspect the packed data rather than
  trusting in-memory nodes.

Implementation method selected:

1. calculate one exact radial stone body for each authored angular pitch;
2. sample an explicit front-face relief grid for the two roll crests and
   intervening hollow;
3. close the back, radial ends, inner face, and outer face into one manifold
   stone volume;
4. create real 3 mm joint gaps between independent stones;
5. write face and point attributes for stone ID, local phase, front, roll,
   hollow, quiet field, joint-adjacent linework, and relief priority;
6. write a stable local UV in metres;
7. sample the intact stone-body textures with per-stone phase/rotation
   variation;
8. pass the body normal through a real Normal Map node;
9. apply selective ink and highlight only through explicit trim masks;
10. pack, save, reopen, and inspect objects, attributes, nodes, images, and
    proof manifest.

### S05 — Max Kutsenko, Church Wall Trim

- Title: *Church Wall Trim*
- Author: Max Kutsenko, texture artist
- Publisher: Games Artist
- URL:
  `https://gamesartist.co.uk/church-wall-trim-substance-material-creation-max-kutsenko/`
- Evidence class: `comparative_finish` and written professional workflow
- Rights: all third-party images remain on the source site; no pixels retained

Written workflow:

- block the complete trim sheet first from simple strips at a few deliberate
  heights;
- combine addition and subtraction masks until the large arrangement works;
- select a finite ornament vocabulary instead of generating unrelated detail;
- author a single motif, then repeat, mirror, transform, and combine it;
- retain different grayscale levels where later edge detection and beveling
  need to separate shapes;
- assemble final height before color;
- layer multiple color washes and make them conform to height, rather than
  assigning one flat color to each band;
- avoid sampling lit reference photography as albedo authority;
- straighten arch UV chunks, align their borders to the trim, and inspect
  edge misalignment in the renderer;
- demonstrate the material in an actual archway, not only on a sphere.

Image translation:

- The photographed church portal has multiple nested orders with clear quiet
  mouldings separating dense floral bands. Stone joints remain visible across
  both plain and decorated orders.
- Ornament density changes by order: small repeated leaves, large scalloped
  floral units, rope-like rolls, roundels, and broad undecorated bands do not
  all compete at one frequency.
- The height sheet separates broad profile steps, major floral volume,
  narrow bead/rope bands, and plain rest fields.
- The rendered arch preserves radial joints and the complete nested-order
  silhouette. Its dense surface is useful as a construction comparison but is
  too uniformly aged and too high-frequency for this intact gameplay asset.

Transfer decision:

- retain blockout-first, finite-motif, height-first, layered-color, aligned-UV,
  and actual-asset proof methods;
- reject its damage/grunge recipe for this intact pass;
- do not copy its ornament.

### S06 — Mike Means, Ornamental Stone Trim Sheet

- Title: *Ornamental Stone Trim Sheet*
- Author: Mike Means, Principal Environment/Material Artist
- URL: `https://thedoombutton.artstation.com/projects/Po4W2Z`
- Evidence class: `comparative_finish`
- Rights: ArtStation page states all rights reserved; textual comparison only

Image translation:

- The sheet is organized as a hierarchy of large moulded rails, medium
  ornamental bands, and fine surface response. Quiet rails separate dense
  leaf, egg-and-dart, arcade, foliage, and quatrefoil bands.
- Broad profile changes survive in the grayscale height view before fine
  pitting appears.
- The normal view shows that nested profile changes, motif facets, and fine
  surface response occupy different frequency bands.
- The final material is not one gray per band: cool gray body, warmer
  shoulder values, pale granular accents, and darker recess families move
  within each element.
- Recess darkness is supported by actual height/normal response. It is not a
  universal black outline.
- Roughness variation is visible across broad faces and selected deposits,
  but the stone remains predominantly diffuse.
- The material is shown both as a flat trim and wrapped around a curved
  architectural volume.

Transfer decision:

- separate macro profile, medium motif, fine stone body, selective linework,
  and roughness;
- preserve quiet bands;
- use the portfolio only as a finish benchmark, never as measurement or
  source imagery.

### S07 — Baldur's Gate 3 Temple Tileset

- Title: *Baldur's Gate 3 — Temple Tileset*
- Author: Gert-Jan van de Put, Lead 3D Environment Artist at Larian
- URL: `https://gjvandeput.artstation.com/projects/rJB8lL`
- Evidence class: shipped-use `comparative_finish`
- Rights: all rights reserved; textual comparison only
- Stated production method: high- and low-poly modular tileset including
  trims, coordinated textures, then many modular pieces assembled in Larian's
  editor

Image translation:

- Portal silhouettes are built from multiple nested orders, shafts, capitals,
  spandrels, and trim bands. Surface information does not substitute for these
  forms.
- Individual radial stone joints remain legible in the broad inner portal
  orders. They interrupt the ring at construction intervals without turning
  every edge into a black outline.
- Reused trim rhythms are broken by columns, capitals, panel fields, windows,
  projecting cornices, gold inserts, and different opening sizes.
- Macro color grouping separates pale warm limestone, cooler blue-gray
  recesses and columns, muted red-brown carved bands, blue patterned panels,
  and sparse gold focal accents.
- Within the pale stone family, faces carry several warm, cool, light, and
  dark passages. No block reads as a single flat swatch.
- Reflection hierarchy is material-specific: stone is broad and diffuse,
  recesses remain readable through value and occlusion, and metal accents take
  the sharper response.
- At wide view, portal hierarchy and opening silhouette dominate. Fine surface
  breakup supports them instead of becoming screen-space noise.

Transfer decision:

- use a modular individual-stone arch with explicit nested anatomy and quiet
  fields;
- preserve broad warm/cool grouping and selective ink/highlight;
- keep metal and damage outside the current capability;
- prove the result at a gameplay-like camera distance as well as close.

### S08 — Gothic modular-kit written breakdown

- Title: *Recreating Gothic Architecture in Substance 3D & Unreal Engine 5*
- Artist: Mika Kuwilsky
- Publisher: 80 Level
- URL:
  `https://80.lv/articles/recreating-gothic-architecture-in-substance-3d-unreal-engine-5`
- Evidence class: written professional workflow
- Rights: all rights reserved; textual findings only

Relevant workflow:

- begin with blockout parts to establish dimensions and fit;
- reduce a cathedral's total detail to its characteristic forms;
- build typical ornaments from a small set of baked assets;
- create simple curve shapes in Blender, then sculpt and bake;
- use protruding stones and columns to carry silhouettes and cover modular
  transitions;
- test compatible combinations;
- reduce dirt for the reusable base and add scene-specific history later;
- solve repetition with additional kit pieces, buttresses, supports, and
  structural variation rather than more universal texture noise.

Transfer decision:

- finish one measured arch order first;
- keep the intact reusable base clean;
- allow later portal families and overlays to be separate capabilities.

## Champion reconstruction

The champion is not a stone-for-stone reconstruction of a recorded complete
arch. It is an `authored_translation` that closes a semicircle using the
catalogued dimensions of S01 catalogue 55.

### Source-bounded values

- radial stone height: `0.200 m`;
- inner stone chord: `0.140 m`;
- catalogued outer stone chord: `0.180 m`;
- stone depth into the wall: `0.270 m`;
- moulding sequence: `roll / hollow / roll`;
- construction type: individual wedge-shaped voussoir.

### Authored closure values

- count: `16` equal centreline pitches over `180 degrees`;
- pitch: `11.25 degrees`;
- technical joint gap: `0.003 m`, transferred from the accepted intact
  cathedral ashlar joint contract, not measured on the S01 arch;
- body angle after the mid-radius joint gap: approximately
  `11.042294 degrees`;
- inner radius derived from the 140 mm chord: approximately
  `0.727551 m`;
- outer radius: approximately `0.927551 m`;
- resulting outer chord: approximately `0.178485 m`, 1.515 mm below the
  catalogue's rounded 180 mm value;
- clear opening diameter at the arch: approximately `1.455103 m`;
- exterior diameter: approximately `1.855103 m`;
- chevron direction: one centripetal lateral face unit per stone;
- left and right springers use the same dimensional module; any jamb beneath
  them is acceptance context, not a claimed Old Sarum reconstruction.

Closure formula:

```text
pitch = pi / 16
body_angle = pitch - joint_gap / (inner_radius + radial_height / 2)
inner_radius = inner_chord / (2 * sin(body_angle / 2))
outer_radius = inner_radius + radial_height
outer_chord = 2 * outer_radius * sin(body_angle / 2)
```

The two equations are solved iteratively until radius change is below
`1e-12 m`.

### Explicitly unknown values

- measured complete-arch radius and stone count;
- measured joint width and mortar profile for S01;
- roll widths and diameters;
- hollow width and depth;
- chevron relief amplitude;
- arris radius;
- point truncation or apex rounding;
- face and soffit phase relationship;
- exact stone species and neutral PBR roughness for the S01 fragments;
- original color, paint, and finish before reuse.

### Bounded authored profile

The capability must exist under light even though S01 does not publish a
section drawing. The profile therefore uses an explicitly authored,
replaceable proportion inside the measured 200 mm radial band:

- inner quiet margin: `25 mm`;
- first roll zone: `50 mm`;
- hollow zone: `50 mm`;
- second roll zone: `50 mm`;
- outer quiet margin: `25 mm`;
- roll crest relief: authored `12 mm`;
- hollow depression: authored `6 mm`;
- stone-front grid sampling: fine enough to resolve the two crests without
  letting tessellation become the visible design.

These values are not historic measurements. They are versioned implementation
parameters whose total width is bounded by the surveyed stone height. Neutral
clay proof must expose them, and later measured profile evidence may replace
them without changing the voussoir construction contract.

## Current-candidate gaps implied by research

1. Replace the smooth annular ribbon with sixteen individually identifiable,
   closed, radial stone volumes and real 3 mm gaps.
2. Replace the independent 83.3 mm diamond repeat with one chevron unit per
   stone, phase-locked to the voussoir.
3. Replace the flat diamond stamp with source-bounded roll/hollow/roll relief
   that survives neutral clay and grazing light.
4. Replace the dormant normal image with an actual Non-Color Image Texture
   through a tangent-space Normal Map node using the same UV.
5. Replace proof-only identity and stylization outputs with live selective
   masks that visibly affect color while remaining optically authored rather
   than baked light.
6. Replace one broad cloudy atlas field with the accepted intact-stone
   material body, per-stone phase/rotation variation, and multiple related
   warm/cool/value passages inside each stone.
7. Preserve quiet stone between roll crests and prevent fine stone body or
   linework from competing at gameplay distance.
8. Prove the material on the actual radial assembly, under clay, unlit color,
   neutral, grazing, close, gameplay-distance, and repetition views.
9. Pack every consumed image and prove the same graph and attributes after
   reopening the saved file.
10. Keep fracture, erosion, soot, limewash, lichen, damp, chips, and repairs
    disabled and outside the candidate.

## Prohibited inferences

- Darkness in a photograph is not relief depth, pigment, or roughness.
- A catalogue width pair does not establish a complete arch radius without an
  explicit authored closure rule.
- A related Gloucester profile does not measure the Old Sarum profile.
- A straight chevron block from S01 is not automatically a portal jamb.
- A professional trim sheet is not historical dimensional authority.
- A packed image is not a live shader lane.
- A shader selector node is not functional unless its output reaches visible
  behavior.
- A saved file reopening does not prove source reproduction.
- A stylized dark recess is not permission to bake illumination into base
  color.
