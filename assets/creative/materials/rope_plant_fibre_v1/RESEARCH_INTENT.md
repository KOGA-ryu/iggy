# Three-strand plant-fibre rope: research to script intent

This material is not a diagonal stripe generator. It reconstructs the nested
construction of a regular-lay plant-fibre rope and keeps each visual scale in
its own lane.

## Physical construction

The United States Bureau of Standards describes commercial regular-lay Manila
rope as three strands whose yarns twist in the opposite direction to the
strands around the rope axis. It defines lay as the axial distance occupied by
one complete turn. Its sample log records approximately 3.3 to 4.0 rope turns
per foot for specimens around 1 1/8 to 1 1/4 inch diameter. This is an axial
lay of roughly 76 to 92 millimetres, or about 2.4 to 3.2 rope diameters.

The three presets use a 2.8-diameter medium-lay target:

| Preset | Diameter | Axial lay | Use |
| --- | ---: | ---: | --- |
| fine lashing | 12 mm | 34 mm | ties and small lashings |
| utility line | 32 mm | 90 mm | pulleys, traps, bindings |
| heavy hawser | 70 mm | 196 mm | bridges and hoists |

These sizes stay inside the 8–80 mm rope family requested for the giant-house
asset library. ISO 2307 supplies the measurement rule: measure complete turns
of the same strand under reference tension and divide their axial length by
the number of turns.

## High-quality material method

Ishan Verma's rope breakdown separates the large rope pattern, threadwork, and
loose strands before assembling the final height. It then combines several
normal contributions instead of deriving one normal from one undifferentiated
height field. Colour is built from multiple gradients and masks; roughness
removes high frequencies before adding a separate variation layer; AO combines
multiple depth scales.

The Python translation is:

1. Build three independent primary crowns with different widths, flattening,
   left/right contact rolloff, packing bulge, colour tendency, and centre
   wander.
2. Give every coarse yarn bundle an explicit counter-twist rate that opposes
   the primary lay; a small curve perturbation is not accepted as
   counter-twist.
3. Represent 5, 8, or 11 coarse bundle tracks per visible strand according to
   preset, with five width classes and track-owned sub-ridges.
4. Apply merge, split, burial, fade, and flatten events with quintic entry and
   exit tapers rather than angular path jumps.
5. Give every bundle a rounded crown, edge lane, groove lane, dominant colour,
   roughness bias, and diameter-relative height.
6. Attach finite long fibre families to named bundle tracks. Each family owns
   start, length, taper, width, slope, bend, height, colour, sheen, quiet-field
   eligibility, valley-bridge eligibility, and one or two authored companions.
7. Attach a separate short-staple library with over a 10:1 length range and a
   small explicit subset eligible for silhouette geometry.
8. Band-limit ribbon coverage and ridge width against the cross-strand pixel
   footprint so a diagonal fibre cannot turn into a one-pixel dot chain merely
   because its centre crosses texel rows.
9. Combine rope-lobe, yarn, long-ribbon, and short-fibre normals separately.
10. Build colour from rope body, soft per-strand tendencies, authored broad
    passages, dominant bundle colour, sub-ridge tones, fibre colour, and
    modest cavity material darkening.
11. Build roughness from absorbency, bundle bias, sheltered cavities, lifted
    fibres, selective sheen, and crown compaction instead of copying height.
12. Keep separator ink, flyaway spawn, compression, wetness, and scene grime
    optional. None is baked into the intact base material.

The default build is streamed: one preset is filled variation-by-variation,
written, and released before the next preset. This keeps the 1536 by 384
production tiles practical on the target Mac without reducing authored detail.

## Geometry translation

The hero source does not inflate a cylinder with the texture height. The
builder reconstructs the nested rope hierarchy:

1. Resample the input curve by a metre-valued step no larger than one
   thirty-fourth of a lay or one ninth of the diameter.
2. Store accumulated spline length as `sinc_rope_u_m`.
3. Duplicate the path three times for compacted strand hulls and 21 times for
   seven yarn bundles per strand.
4. For rope diameter `D`, place equal strand centres at
   `0.2679491924 D` from the rope axis. The ideal equal-circle strand radius is
   `0.2320508076 D`; the visible compacted hull uses `0.215 D`.
5. Pack one core and six outer yarn bundles in every strand. Outer yarn
   centres sit at `0.72` of the ideal strand radius and yarn radius is `0.23`
   of that radius. Their crowns therefore break the hull without turning the
   rope into 21 separate cables.
6. Calculate rope phase from physical distance,
   `theta = 2 pi u / lay + 2 pi strand / 3`.
7. Counter-rotate yarn packing at 1.14 times the rope turn. This produces
   opposite yarn and rope hands rather than another same-direction helix.
8. Add two bounded phase-drift bands, 0.038 and 0.014 radians, at
   incommensurate rates. They perturb rhythm without changing the specified
   average lay length.
9. Offset through the curve normal and tangent-cross-normal binormal, then
   convert strand and yarn curves to meshes.
10. Store metre U, circumference turn V, diameter, strand identity, yarn
    identity, and face-corner `rope_uv` on the evaluated mesh.

The four-row authored atlases are not sampled as one tall texture. Strand and
yarn identity select one row; V is remapped into that quarter, and U remains
one physical lay per tile. The long- and short-fibre channels drive finite
colour and normal accents. Structural strand/yarn relief remains geometry.

The source intentionally keeps three levels of representation:

- hero: compacted strand hulls, 21 yarn crown curves, and optional sparse
  flyaways;
- standard: bake the hero source to a three-lobed mesh while retaining the
  same metre and identity lanes;
- distant: one tube using the baked tangent normal, roughness, colour, and
  identity maps.

## Art-direction limits

- No AI-generated imagery.
- No photographic noise.
- No universal fuzz or fray.
- No baked light direction in base colour.
- No perfectly repeated candy-cane striping.
- Fine flyaway silhouettes belong to optional geometry, not noisy alpha.
- The heavy preset may require real strand silhouette geometry at hero range;
  this texture never claims to replace it.
