# Plant-fibre rope material anatomy dossier

## Status

The original `rope_plant_fibre_v1` output was a structural prototype. The
reference-driven v2 construction now implements asymmetric strand crowns,
explicit opposing counter-twist, authored coarse bundle tracks, rounded
sub-ridges, finite long-fibre families, authored companion blades, short staple
fragments, persistent construction-owned colour, separate height/normal
scales, cavity response, selective sheen, and optional flyaway-spawn masks.

The structural tests are green and the default production build completes at
1536 by 384 pixels per variation. Visual acceptance is still open. The current
licensed-reference comparison shows that the script proof remains too orderly
and too sparse in the crown interiors compared with R2, and its neutral
analytic renderer does not yet reproduce the dense anisotropic highlight field
of the photograph. Those are explicit remaining defects, not accepted quality.

## Reference authority

### R1 — hemp-rope macro

- File: `references/hemp_rope_travis_isaacs_cc_by_2.jpg`
- Source:
  https://commons.wikimedia.org/wiki/File:Hemp-rope.jpg
- Author: Travis Isaacs
- License: Creative Commons Attribution 2.0
- Original size: 3892 by 2586 pixels
- Use: aged fibre colour, strand transition anatomy, loose fibre density,
  irregular yarn grouping, compression, and highlight behaviour.
- Limits: shallow depth of field, unknown physical diameter, unknown lighting,
  and use-related wear make it unsuitable as direct albedo.

### R2 — brown rope close-up

- File: `references/closeup_ropes_procsilas_moscas_cc_by_2.jpg`
- Source:
  https://commons.wikimedia.org/wiki/File:Closeup_ropes.jpg
- Author: Procsilas Moscas
- License: Creative Commons Attribution 2.0
- Original size: 2560 by 1920 pixels
- Use: intact surface hierarchy, fibre-ribbon density, strand valleys, colour
  range inside one rope, short fibre ends, and material response.
- Limits: direct sunlight creates clipped highlights and deep photographic
  shadows; these must not be copied into base colour.

### R3 — construction diagram

- File: `references/rope_construction_verrill_1919_public_domain.png`
- Source:
  https://commons.wikimedia.org/wiki/File:Constriction_of_rope.png
- Author: Alpheus Hyatt Verrill
- Date: 1919
- License: public domain
- Use: explicit fibre to yarn to strand to rope hierarchy.

### R4 — measured Manila-rope tests

- Source:
  https://nvlpubs.nist.gov/nistpubs/nbstechnologic/nbstechnologicpaperT198.pdf
- Authors: A. H. Stang and L. R. Strickenberg
- Institution: United States Bureau of Standards
- Date: 1921
- Use: three-strand regular-lay construction, opposing twist directions,
  turns-per-foot observations, and empirical yarn-count scaling.

### R5 — professional material construction breakdown

- Source: https://gamesartist.co.uk/woven-rope/
- Artist: Ishan Verma
- Use: separate generators for rope pattern, threadwork, and loose strands;
  layered normal construction; masked colour buildup; independently treated
  roughness and ambient occlusion.

## Hierarchy: what one rope actually contains

The rope must be authored as a nested construction, not as frequencies placed
on top of one another.

### Level 0 — complete rope body

The complete rope has a nominal diameter, an axial lay length, a handedness,
and an overall compression state. At this level the shader must establish:

- three-lobed cross-sectional suggestion;
- silhouette responsibility split between material and geometry;
- the large contact troughs where neighbouring strands meet;
- the apparent pitch of a complete strand turn;
- local changes in apparent diameter caused by strand packing;
- broad absorbency and age-neutral colour fields;
- curve-stable coordinates in metres.

The material may suggest the three lobes in normal and height, but a hero
heavy hawser needs geometry if the camera can read the silhouette.

### Level 1 — the three primary strands

The strands are not equal graphic ribbons. In the references:

- one crown may appear broad while the next is pinched;
- the contact edge may flatten before entering a valley;
- the visible width changes gradually along the lay;
- the crown is not centered perfectly between adjacent troughs;
- one strand can be slightly darker or greyer without becoming a solid colour
  strip;
- the transition into the next strand contains overlapping fibre bundles,
  rather than a clean sine-wave boundary.

Required authored properties per strand:

- phase offset;
- crown-width curve;
- left-contact rolloff;
- right-contact rolloff;
- crown flattening;
- local packing bulge;
- broad colour tendency;
- valley depth contribution;
- compression response;
- linework eligibility.

### Level 2 — yarn and coarse bundle structure

The prototype's evenly spaced comb pattern is wrong. R1 and R2 show coarse
bundles with unequal width, spacing, height, and persistence.

Observed behaviours:

- neighbouring bundles merge for part of their path;
- a bundle can lose contrast and disappear into the crown;
- two narrow bundles can occupy the space of one broad neighbour;
- spacing increases near a contact trough;
- local bundle direction bends as the strand rolls into a valley;
- a bundle may stay bright while the adjacent bundle is dark because their
  exposed fibre orientations differ;
- bundle ridges are rounded or flattened, not identical cosine crests.

Required authored properties per bundle track:

- owning primary strand;
- local cross-strand offset;
- width as a fraction of strand width;
- height as a fraction of rope diameter;
- longitudinal persistence;
- local phase lead or lag;
- curvature;
- merge target and merge window;
- split child and split window;
- crown/valley visibility bias;
- colour-family bias;
- roughness bias;
- anisotropic highlight bias.

The fine, utility, and heavy presets should not merely increase the frequency.
The number of represented coarse bundles must change with viewing role:

- fine lashing: 4–6 readable bundles per visible strand;
- utility line: 6–9 readable bundles per visible strand;
- heavy hawser: 8–13 readable bundles per visible strand.

These are render-representation counts, not claims about literal yarn count.
The Bureau of Standards data shows that physical ropes can contain many more
yarns than can survive a game texture's mip chain.

### Level 3 — long fibre ribbons

Natural Manila and hemp surfaces are built from long, blade-like fibres.
The dominant fine detail is not isotropic fuzz.

R2 image-space observations:

- the central sharp rope spans roughly 260–340 pixels across its visible
  diameter depending on position;
- major visible fibre ribbons are commonly about 2–9 pixels wide;
- the corresponding visible width is approximately 0.6–3.5 percent of the
  apparent rope diameter;
- many ribbons remain visible for 0.25–1.5 rope diameters before merging,
  crossing, or disappearing;
- a smaller population extends for several diameters;
- ribbons vary from dark ochre through medium copper-brown to pale straw;
- some pale ribbons form narrow specular streaks without the entire rope
  becoming glossy.

These observations are image-space ratios, not physical metrology.

Required authored properties per ribbon family:

- owning bundle track;
- width range;
- length range;
- offset jitter;
- direction jitter;
- waviness;
- taper at both ends;
- overlap order;
- colour-family distribution;
- height range;
- normal-only option;
- roughness and anisotropy response;
- quiet-field eligibility.

Long fibres need segmented persistence. A global high-frequency sine cannot
represent where a ribbon starts, narrows, disappears beneath another ribbon,
and later re-emerges.

### Level 4 — short fibre ends and staple fragments

R1 and R2 both contain short protruding fibres. They are not damage decals.
They are part of the normal material construction.

Observed behaviours:

- most lie close to the surface;
- directions cluster around the local yarn direction but include strong
  departures;
- lengths vary by more than an order of magnitude;
- some appear as pale transverse stubs;
- others form dark embedded slivers;
- clusters occur, but large quiet stretches remain;
- only a small subset should affect silhouette.

Required lanes:

- embedded short-fibre colour mask;
- tangent-space normal-only sliver mask;
- optional opacity/geometry spawn mask;
- root position;
- length;
- width;
- bend direction;
- lift;
- colour family;
- roughness;
- visibility by distance preset.

No evenly distributed hair noise is allowed.

### Level 5 — contact troughs and internal shadow responsibility

Primary strand troughs produce the strongest graphic line in the material.
They require several separate contributions:

- real geometric recession or height;
- AO derived from recession;
- roughness increase from sheltered fibre orientation;
- colour variation from denser and less-exposed material;
- optional stylized ink;
- compression flattening where the rope bears against another surface.

The base colour may contain a modest material-darkening lane. It may not bake
the deep directional shadows visible in the photographs.

The trough is not a constant-width black line. It should:

- widen and narrow with strand packing;
- soften across a flattened contact;
- be interrupted by crossing fibres;
- contain at least two depth scales;
- vary independently from optional ink;
- remain readable in the normal map when the ink lane is disabled.

### Level 6 — colour anatomy

One rope strand contains many colours. The new colour system requires:

1. rope-wide neutral body;
2. broad warm/cool age-neutral fields;
3. per-strand tendency;
4. per-bundle colour family;
5. long-ribbon colour;
6. pale dry fibre;
7. dark embedded fibre;
8. contact-trough material darkening;
9. compression desaturation;
10. optional weathered-grey variant.

R2 contains copper, ochre, straw, honey, tan, muted brown, deep umber, and
small desaturated grey-brown passages. R1 adds cooler grey, compact dark brown,
and pale exposed fibres.

The palette must therefore contain at least:

- 5 broad body colours;
- 6 medium bundle colours;
- 5 pale fibre colours;
- 4 dark embedded/cavity colours;
- 4 weathered-neutral colours.

Colour selection must be spatially persistent along fibres. Per-pixel colour
noise is prohibited.

### Level 7 — material response

Natural fibre response is directional and scale-dependent.

Required response components:

- broad diffuse rope body;
- slightly smoother compacted bundle crowns;
- rougher lifted and broken fibres;
- selective narrow fibre highlights;
- sheltered trough roughness;
- compression/contact polish only where supplied by geometry;
- optional wetness as a scene overlay;
- no metallic response.

The normal map must be layered:

1. rope-lobe normal;
2. strand-contact normal;
3. bundle-track normal;
4. long-ribbon normal;
5. short-fibre normal.

The short-fibre and long-ribbon normals must not be reconstructed from the
same combined height as the large rope lobe. They have different amplitude,
filtering, and mip requirements.

## Direct comparison: reference versus current prototype

| Feature | References | Current prototype | Required correction |
| --- | --- | --- | --- |
| Strand boundary | changing width, rolled contact, fibre overlap | smooth repeated cosine trough | asymmetric contact profiles plus crossing-ribbon interruptions |
| Strand crown | uneven, locally flat or packed | equal analytic lobe | strand-owned crown-width and flattening curves |
| Coarse bundles | unequal, merging, disappearing | evenly spaced comb | authored track table with width, persistence, merge, and split windows |
| Long fibres | tapered ribbons with start/end points | infinite periodic micro-lines | segmented ribbon families with length and overlap |
| Short fibres | clustered stubs and slivers | almost absent | authored sparse event library and geometry mask |
| Colour | many persistent colours inside one strand | soft tan field with limited directional persistence | bundle- and ribbon-owned colour assignments |
| Troughs | layered material and geometric depth | one smooth sinusoidal valley | large recess, narrow core, fibre bridges, AO, roughness, optional ink |
| Roughness | directional and locally selective | broad scalar variation | bundle crown, ribbon sheen, trough, and lifted-fibre lanes |
| Variation | construction irregularity | harmonic phase warp | authored structural variants plus bounded perturbation |
| Proof | photographed rope reads cylindrical | flat atlas only | neutral cylinder and curved-rope comparison under two lights |

## Script architecture

The replacement generator should expose these intermediate products:

- `rope_body`;
- `strand_identity[3]`;
- `strand_contact_left[3]`;
- `strand_contact_right[3]`;
- `strand_crown[3]`;
- `bundle_track_masks[N]`;
- `bundle_merge_masks[N]`;
- `bundle_split_masks[N]`;
- `long_ribbon_masks[N]`;
- `short_fibre_embedded`;
- `short_fibre_geometry_spawn`;
- `cavity_large`;
- `cavity_core`;
- `fibre_bridge_mask`;
- `colour_body`;
- `colour_bundle`;
- `colour_ribbon`;
- `roughness_body`;
- `roughness_bundle`;
- `roughness_ribbon`;
- `roughness_lifted_fibre`;
- `normal_rope`;
- `normal_bundle`;
- `normal_ribbon`;
- `normal_short_fibre`;
- `optional_ink`;
- `quiet_field`;
- `compression_input`;
- `wetness_input`.

Every intermediate lane must be inspectable in a proof. If a lane cannot be
viewed independently, it will be too easy to hide a poor generator inside the
combined output.

## Acceptance tests

The detailed pass is not accepted unless:

- each axial cross-section still contains three primary crowns;
- neighbouring crowns have measurably different width profiles;
- bundle widths have at least four distinct authored classes;
- no preset has a single dominant bundle spacing frequency;
- at least 20 percent of bundle tracks contain a merge, split, fade, or burial
  event;
- long-ribbon length spans at least a 6:1 range;
- short-fibre lengths span at least a 10:1 range;
- short fibres occupy less than 12 percent of the intact surface;
- at least 30 percent of the surface remains a quiet field at fibre scale;
- strand, bundle, ribbon, and trough colour contributions are independently
  non-zero;
- roughness-to-height correlation remains below 0.72;
- optional ink can be disabled without losing strand readability;
- base colour contains no directional-light gradient;
- both atlas axes remain seam-safe;
- the first and fourth variations cannot be aligned by a simple phase shift;
- the cylinder proof reads as rope at gameplay distance;
- the close proof exposes ribbon hierarchy without becoming photographic
  noise;
- the heavy preset declares when silhouette geometry is required.

## Delivery sequence

1. Author strand contact and crown profiles.
2. Author bundle-track tables for one champion variation.
3. Prove bundle merges, splits, fades, and unequal spacing.
4. Add long-ribbon families.
5. Add embedded short fibres and optional geometry spawn mask.
6. Rebuild colour by construction ownership.
7. Rebuild roughness and anisotropic highlight masks.
8. Layer normals by scale.
9. Derive three bounded structural variations from the champion.
10. Produce flat, cylinder, curved, grazing, distance, and channel-isolation
    proofs.
