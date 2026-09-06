# Stage 1: Reference Research

This stage converts outside evidence into a bounded material brief. It does not
authorize production maps, guessed geometry, copied images, or a celebratory
reference board.

## Research questions

Answer all questions relevant to the selected capability:

1. How is the material physically made or formed?
2. Which anatomical, mineral, fibre, grain, aggregate, scale, coating, or
   construction layers are visible?
3. Which direction does each structure follow?
4. What are the real dimensions, spacing, density, depth, and variation
   envelopes?
5. How do cut orientation, tooling, fabrication, age, use, and environment
   change the surface?
6. What remains visually quiet?
7. How does the material respond under frontal, neutral, and grazing light?
8. Which details survive at hero, gameplay, and distant viewing ranges?
9. How do excellent production assets simplify or exaggerate the same facts?
10. Which claims cannot be supported by the available sources?

## Source ladder

Prefer sources in this order:

1. standards, catalogues, surveys, laboratory papers, conservation manuals,
   manufacturer technical sheets, and museum measurements;
2. official Blender, Unreal, or tool documentation for implementation
   behavior;
3. detailed written professional breakdowns that show intermediate layers and
   scene use;
4. high-resolution professional asset presentations for visual comparison;
5. photographs with known object identity, scale, license, and lighting
   limitations;
6. video only when its transcript provides unique procedural detail.

Search broadly enough to avoid building the complete brief around one
convenient source. Prefer two independent authorities for a critical
measurement or transfer claim when possible.

## Written-source rule

Research through text. If a useful result is a video:

- obtain its transcript;
- identify the exact procedural statements;
- separate the artist's measured facts from preferences or shortcuts;
- record only what the transcript supports.

If a useful result is an image:

- identify the object, material, date, view, scale cue, and lighting;
- translate visible structure into text;
- list ambiguities such as shadow versus pigment, metal versus wood, damage
  versus construction joint, or geometry versus normal response;
- do not leave the image as unexplained inspiration.

## Reference ledger

Each retained source entry should record:

- stable source identifier;
- title and publisher or creator;
- URL or repository path;
- publication or access date when useful;
- license and whether pixels may be retained;
- material, object, and construction family;
- exact measurements or observations used;
- evidence class;
- transfer limit;
- implementation decision enabled by the source;
- unsupported interpretations that remain prohibited.

Use SI units in the authored contract. Preserve the source unit in the ledger
when conversion errors would be possible.

## Evidence classes

Use explicit language:

- `surveyed`: measured from the stated historical or physical specimen;
- `catalogued`: supplied by a collection or manufacturer;
- `laboratory_characterized`: measured composition or surface property;
- `technical_guidance`: a recommended working value, not a specimen survey;
- `qualitative_authority`: reliable construction or appearance description
  without a numeric value;
- `comparative_finish`: a measured or demonstrated process transferred from a
  related material or period;
- `proxy`: a deliberately limited substitute for one property;
- `authored_translation`: a project choice constrained by evidence;
- `unknown`: not authorized.

Never replace these terms with the vague word "reference".

## Professional quality comparison

Select at least two high-quality assets or shipped-use examples of the same
material role. At least one should show the material in a scene, modular kit,
trim sheet, or reusable library rather than only a sphere.

For each benchmark, write concrete comparisons in:

- construction and silhouette;
- macro value grouping;
- medium material structure;
- edge treatment;
- color layering;
- reflection and roughness;
- stylized authorship;
- repetition strategy;
- world-context use.

Name at least five visible gaps between the current candidate and the
benchmark. "Needs more detail" is not a comparison.

Do not copy benchmark maps or embed unlicensed imagery. Store links and textual
analysis.

## Color research

Do not sample arbitrary shadowed pixels and call them material color.

When a licensed photograph is used for relative color evidence:

- sample only identified material interiors;
- exclude sky, cast shadow, highlights, mortar, damage, repairs, dirt, and
  adjacent materials unless one of those is the explicit research subject;
- normalize or de-light only for relative grouping;
- retain numeric palette evidence and attribution;
- state that the capture does not establish PBR roughness, relief, or absolute
  color under neutral light;
- discard or isolate source pixels according to license and package policy.

Research color as relationships:

- warm versus cool families;
- light, mid, and dark value groups;
- within-element transitions;
- layer-specific hue shifts;
- areas of rest;
- event colors such as fresh cut, oxidation, compression, mineral inclusion,
  or exposed substrate.

## Implementation research

Research how Blender or the target engine represents the required operation:

- local and object coordinates;
- curve length and deformation-stable frames;
- UV and face-corner attributes;
- named geometry attributes;
- texture color spaces and channel packing;
- normal combination;
- material layer blending;
- Boolean or fracture face classification;
- packed images and save/reopen behavior.

Use written official documentation where available. A node screenshot without
socket-level reasoning is not an implementation plan.

## Research exit gate

Do not call this stage ready until the package can state:

- one coherent material or construction family;
- real scale envelopes for the important visible structures;
- physical layer anatomy;
- at least two professional benchmarks;
- known color relationships;
- a written Blender and engine method;
- explicit unknowns and proxy limits;
- current candidate gaps;
- no unresolved confusion between geometry, lighting, and texture.

If one unsupported value would materially change the result, continue
research. If it can safely be omitted, record it as unknown and continue.

