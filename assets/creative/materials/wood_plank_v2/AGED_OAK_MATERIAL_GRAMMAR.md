# Aged Flat-Sawn Oak: Executable Material Grammar

Research and intent dossier, 2026-07-28

## Decision

The first champion material is an aged, flat-sawn white-oak plank floor with a
low-luster amber finish, sparse knots, moderate checking, and restrained
painterly/ink accents. It is not a generic brown wood, reclaimed barn wood, a
fresh polyurethane floor, or a literal copy of an *Arcane* or *Borderlands*
asset.

The physical layer must read as oak before stylization is enabled. The
stylization layer must then make selected structure more legible without
turning every fibre into a black contour.

This document is the specification for the next hand-authored specimen and its
eventual generator. It deliberately freezes generator work until the feature
relationships below can be represented. The current `wood_plank_v2` maps are a
prototype baseline, not the visual target.

## How to read the claims

Four labels separate research from art direction:

- **Observed** is supported by a linked anatomy, flooring, or professional-art
  source.
- **Inferred** is a conservative visual consequence of the observed fact.
- **Directed** is a choice for this material's story and style.
- **Proposed control** is an initial scripting range to be tuned against
  renders. It is not presented as a biological measurement or flooring
  standard.

This separation matters. A procedural material becomes untrustworthy when an
arbitrary noise value is described as if nature required it.

## Reference conclusions

### Material identity

White oak is a ring-porous hardwood. Earlywood and latewood differ markedly,
and oak has conspicuous rays. These are not optional micro-details: they are
the anatomical reasons oak does not read like pine, walnut, or a laminated
print. The USDA Forest Products Laboratory describes hardwood vessels or
pores, ring-porous growth, and the prominence of oak rays in its
[Wood Handbook material](https://www.fpl.fs.usda.gov/documnts/fplgtr/fplgtr113/ch02.pdf).
Its white-oak technical material describes a light-to-dark brown heartwood,
coarse texture, generally straight grain, and strong differences between
earlywood and latewood
([white-oak technical sheet](https://www.fpl.fs.usda.gov/documnts/TechSheets/HardwoodNA/pdf_files/quercuseng.pdf)).

The National Wood Flooring Association describes white oak as having open
grain, longer rays, considerable board-to-board color and grain variation, and
different figures by cut. Plain-sawn faces are plumed or flared; rift-sawn
faces are calmer; quarter-sawn faces expose flake or tiger-ray figure
([NWFA white oak](https://woodfloors.org/white-oak-species/)).

### The saw cut is a shape generator

On a flat- or plain-sawn face, growth rings run roughly parallel to the board
width and form cathedral-like arches. Quarter-sawn faces show straighter grain
and can expose conspicuous ray fleck. This relation is explained and
illustrated by Penn State Extension in
[What Exactly Are Growth Rings?](https://extension.psu.edu/what-exactly-are-growth-rings)
and
[What Are Rays in Wood?](https://extension.psu.edu/what-are-rays-in-wood).

The consequence for scripting is direct: a flat-sawn cathedral cannot be a
sine stripe rotated into the plank. It must be an intersection through a
curved ring field, with an explicit pith offset and saw-plane orientation.

### Defects disturb the anatomy

Knots interrupt the continuity of grain and create localized steep cross-grain.
Checks are separations that commonly arise during seasoning. USDA guidance
also distinguishes checks from splits and describes the distorted-grain zone
around knots
([Wood Handbook, stress grades chapter](https://research.fs.usda.gov/download/treesearch/62252.pdf)).

Oak rays are planes along which checks may form. Penn State notes that checks
often develop beside the large rays visible in oak. This means knots, rays,
and checks cannot be sampled as unrelated decals.

### Form records moisture history

Wood changes dimension differently in tangential and radial directions.
Plain-sawn boards tend to cup toward the bark side. Moisture changes can also
produce gaps or splits, some of which change seasonally. These behaviors are
covered by the
[USDA Forest Products Laboratory FAQ](https://research.fs.usda.gov/fpl/faq),
Penn State's growth-ring explanation, and
[NWFA problem-prevention guidance](https://woodfloors.org/problem-prevention/).

The material does not need to simulate moisture. It does need a consistent
`bark_side` and ring orientation so cup, ring curvature, and some checking tell
the same history.

### Finish and use are separate layers

Floor finish protects the wood. Clear water-based finishes, amber oil-based
finishes, low-luster amber wax, and penetrating natural oils create different
color and reflection stories. Matte finishes reflect the least light, and
lower sheen makes small scratches less obvious
([NWFA finishes](https://woodfloors.org/finishes/)).
Tracked dirt and debris can scratch or wear the finish
([NWFA floor care](https://woodfloors.org/spring-cleaning-top-tips-for-caring-for-wood-floors/3/)).

Our target is a thin, warm, low-luster penetrating finish. Wear should first
change sheen and color, and only deeper events should cut appreciably into
wood height.

### Stylization is a selection system

Mark Foreman's professional Substance workflow separates softened anisotropic
grain, growth rings, knot-driven deformation, directional fibres, middle-scale
noise, colorized grain families, and related-but-distinct roughness
([Adobe Substance 3D tips](https://www.adobe.com/learn/substance-3d-designer/web/mark-foreman-s-substance-3d-designer-tips-and-tricks)).
Adobe's old-plank course likewise treats planks, height, knots, vector warp,
roughness, color, blending, and rendering as dependent stages
([Creating Old Wood Planks](https://www.adobe.com/learn/substance-3d-designer/web/creating-old-wood-planks-in-substance-3d-designer)).

Fortiche identifies organic, hand-crafted imperfection as central to its visual
language ([Fortiche FAQ](https://forticheprod.com/faq/)). Gearbox's
*Borderlands* retrospective describes material, lighting, and shading as a
co-developed route from concept-art character to the game
([Unreal Engine retrospective](https://www.unrealengine.com/blog/borderlands)).

The actionable common ground is selective authorship: large readable shapes,
controlled rest, a few expressive marks, and a material that supports graphic
lighting. Generic grunge is not stylization.

## The material story

This is a cared-for old interior floor, not neglected exterior lumber.

- The boards are white-oak heartwood, primarily flat-sawn.
- Installation uses 150–200 mm wide planks with staggered end joints.
- The floor has had enough moisture history for subtle cup, small gaps, and
  selected checks, but not enough neglect for rotten voids or severe warping.
- A warm penetrating oil or wax-like finish has aged to matte or low satin.
- Traffic has softened some shoulders and altered sheen. Dirt gathers in
  joints and deeper recesses, not as a global black wash.
- Knots and figure are sparse enough that quiet boards remain visible.
- Painterly strokes reinforce broad flow; ink accents reinforce a few
  structural events. Neither layer substitutes for physical form.

The grading vocabulary is inspired by the visible character permitted in
common oak grades—color variation, knots, checks, worm holes, and mineral
streaks—but this asset is not claiming a certified lumber grade
([NWFA NOFMA grade photographs](https://nwfa.org/nofma-grade-photos/)).

### Source-to-feature traceability

| Source | Material facts used | Feature records constrained |
|---|---|---|
| USDA Wood Handbook, structure | hardwood vessels, ring-porous growth, rays, earlywood/latewood | OAK-03–OAK-09 |
| USDA white-oak technical sheet | white-oak color, coarse texture, straight grain, growth character | OAK-02, OAK-04–OAK-09, OAK-28 |
| Penn State, growth rings | flat-sawn cathedral figure, cut-dependent ring appearance, cupping tendency | OAK-03, OAK-06, OAK-16 |
| Penn State, rays | cut-dependent ray appearance, oak ray prominence, checks beside rays | OAK-07, OAK-20 |
| USDA stress-grades chapter | knot-driven cross-grain, checks, splits, seasoning | OAK-12–OAK-15, OAK-20–OAK-21 |
| USDA FPL FAQ | radial/tangential shrinkage and moisture response | OAK-16–OAK-21 |
| NWFA white oak and grade photographs | cut figure, color variation, knots, checks, mineral streak vocabulary | OAK-01–OAK-03, OAK-12, OAK-20, OAK-28–OAK-29 |
| NWFA finishes and floor care | finish families, low sheen, scratching, finish wear | OAK-23–OAK-27, OAK-31 |
| Adobe professional wood workflows | separate ring, fibre, knot-warp, color, roughness, and render stages | OAK-03–OAK-15, OAK-30–OAK-32 |
| Fortiche and Gearbox style discussions | organic imperfection and co-designed material, light, and shading | OAK-11, OAK-33–OAK-36 |

Board dimensions use a directed range inside common plank-floor territory.
NWFA describes plank flooring as 3 inches or wider
([floor style guide](https://woodfloors.org/which-wood-floor-fits-your-lifestyle-part-one/3/)).
The exact 150–200 mm range below is an art-direction control, not a claim that
all white-oak floors use those widths.

## Coordinate and scale contract

The existing tile remains 1.60 m square. Each feature is authored in metres
and evaluated in a local board frame:

- `s`: distance along the board;
- `t`: signed distance across the board, with `t = 0` at its centre;
- `z`: surface height;
- `bark_side`: `-1` or `+1`, defining which long edge lies closer to the bark;
- `growth_flow`: a normalized 2D tangent following the visible ring or fibre;
- `board_id` and `segment_id`: stable ownership, never derived from color;
- `edge_distance` and `end_distance`: independent distances to long and end
  joints.

The delivery target is 2048 px. One texel then covers approximately 0.78 mm.
The 1024 px study target covers approximately 1.56 mm per texel. Anatomical
features smaller than a texel must be represented as an aggregate change in
tone, roughness, or a broken cluster—not as scientifically literal holes.

### Frequency ladder

| Band | Physical span | Carries | Must not carry |
|---|---:|---|---|
| Composition | 0.20–1.60 m | board widths, joint cadence, focal rests | fibre, pores |
| Macro form | 40–400 mm | cup, bow, cathedral sweep, traffic lane | speckle |
| Meso structure | 2–40 mm | ring bands, rays, knot halo, checks, edge wear | global cloudy noise |
| Micro response | 0.2–2 mm | aggregated pores, broken fibres, fine scratches | tile identity |
| Stylized mark | screen-aware | selected ink and brush accents | physical depth by itself |

These bands overlap because one cause can affect several scales. A knot has a
centimetre-scale body, a larger grain-deflection zone, and small internal
checks. They must share an owner even when rendered by separate fields.

## Dependency graph

```text
floor layout
  -> board and segment ownership
    -> local frame + bark side + saw-plane model
      -> annual-ring phase and growth flow
        -> earlywood / latewood bands
        -> ray and pore habitats
        -> knot body and knot-driven ring deformation
          -> board form, edge form, and defect candidates
            -> finish, traffic, scratches, dirt, and exposed wood
              -> final height and normal
              -> base pigment and material-event color
              -> roughness, cavity, and occlusion
              -> ink, highlight strokes, brush direction, detail priority
```

No downstream field may silently invent a second board layout, knot position,
or grain direction. Derived channels may simplify a shared cause, but they may
not contradict it.

## Executable feature records

All numeric values below are proposed first-pass controls. The hand-authored
specimen is expected to select deliberate values inside the ranges; a later
variation script may sample them only after the specimen passes review.

### OAK-01 — Board cadence

**Observed.** Flooring classified as plank is at least 3 inches wide, and oak
floors legitimately contain board-to-board variation. **Directed.** Use nine
visible rows across the tile, with a mixture of calm wide boards and narrower
transitions.

**Representation.** Store ordered physical widths and cyclic end positions.
Normalize only after validation. Each board owns its local coordinate frame.

**Proposed controls.**

- `board_width_m`: 0.15–0.20;
- `visible_segment_length_m`: 0.45–1.45;
- `adjacent_end_separation_m`: at least 0.18 where the composition permits;
- no more than two conspicuously short pieces in the champion tile.

**Constraints.** Adjacent widths must not alternate mechanically. Three or
more end joints must not align into an accidental cross-floor seam. The wrap
joint must be judged as part of the composition.

**Proof.** A flat ID plate at 1× and 4× must read as a plausible laying cadence
with no texture channels enabled.

### OAK-02 — Stable board families

**Observed.** White oak may vary considerably in color and grain. **Inferred.**
That variation belongs to boards grown and cut differently, not to independent
pixels.

**Representation.** Each board receives one `board_family` record containing
heartwood color, ring spacing character, saw offset, roughness bias, cup sign,
and permitted defects. Segments inherit the board family and may add limited
local history.

**Proposed controls.** Author four to six families for the tile: warm tan,
neutral brown, muted honey, slightly cool brown, and at most one pale
transitional board. Keep family value offsets inside roughly ±8% until light
tests justify more.

**Constraints.** Color may not be the only difference between families.
Neighbours may share hue but must not share every ring and wear parameter.

**Proof.** A board-family plate must show recognizable kinship without looking
like a five-color palette strip.

### OAK-03 — Flat-sawn ring scaffold

**Observed.** Flat-sawn faces produce plumed or cathedral figure. **Directed.**
The champion specimen is overwhelmingly flat-sawn; one quiet near-rift face is
allowed for compositional rest, but conspicuous quarter-sawn flake is excluded.

**Representation.** Evaluate nested, slowly varying growth-ring curves in a
virtual log cross-section, then intersect them with each board's saw plane.
Expose `pith_offset`, `saw_angle`, `ring_age_phase`, and a slow longitudinal
drift. The result is a continuous scalar `ring_phase` and vector
`growth_flow`, not a thresholded stripe image.

**Proposed controls.**

- `pith_offset / board_width`: 0.8–3.5;
- `saw_angle_deg`: -10 to +10 for the flat-sawn target;
- `longitudinal_drift_m`: 0.08–0.35 over 1 m;
- adjacent ring spacing changes gradually, not by independent sampling.

**Constraints.** No perfectly periodic spacing; no mirrored cathedral centred
on every board; no discontinuity at a segment joint belonging to the same
physical board family.

**Proof.** The ring-phase plate must show at least three distinct states:
opening arch, closing arch, and calm transition. A line-frequency analysis
must not find one dominant barcode interval across the entire tile.

### OAK-04 — Earlywood band

**Observed.** White oak is ring-porous and earlywood differs visibly and
structurally from latewood. **Inferred.** Earlywood should read as a softer,
more porous transition near each annual boundary.

**Representation.** Derive a broad asymmetric window from `ring_phase`. It
provides the habitat for aggregated pore chains and a slight pigment and
roughness change.

**Proposed controls.**

- `earlywood_fraction`: 0.18–0.36 of a local ring;
- height relief: a shallow 0.02–0.10 mm aggregate recession at delivery scale;
- color: small warm/dark or desaturated shift chosen per board family;
- roughness: +0.02 to +0.08 before finish.

**Constraints.** It must vary with local ring width. It may not become a hard
black contour or an equally strong groove on every ring.

**Proof.** When latewood and fibres are disabled, earlywood still follows the
cathedral and changes width naturally. At room distance it merges into broad
figure rather than aliasing.

### OAK-05 — Latewood body

**Observed.** Latewood is denser than earlywood and contributes the solid body
between porous ring starts. **Inferred.** It should carry most of the calm face
and should not be rendered as an equally dark second stripe.

**Representation.** The complement of earlywood is modulated by a low-frequency
density field tied to the board family. Use it chiefly for broad pigment and
subtle response, not a binary height step.

**Proposed controls.** Preserve 55–75% of each board face as visually quiet
latewood. Pigment variation inside it should stay below the contrast of the
earlywood boundary, knot, or joint.

**Constraints.** Global micro-noise cannot be multiplied uniformly through the
latewood. Quiet latewood is the material's area of rest.

**Proof.** A grayscale detail-density plate must show connected calm regions
large enough to survive at 4× repetition.

### OAK-06 — Ring-spacing history

**Observed.** Annual rings are not equally spaced. **Directed.** The floor
should show periods of slower and faster apparent growth without becoming
chaotic.

**Representation.** Build ring radii by cumulatively integrating a smooth
positive spacing curve. Each board family receives two or three broad growth
epochs; local jitter is subordinate.

**Proposed controls.**

- spacing multiplier: 0.65–1.55 around a family mean;
- broad epoch length: 4–12 rings;
- per-ring residual change: no more than about 12% before smoothing.

**Constraints.** Never add white noise to ring position. Ring order cannot
cross or reverse.

**Proof.** An unwrapped ring-spacing graph must show broad trends plus small
variation, with no metronomic plateau and no random sawtooth.

### OAK-07 — Rays

**Observed.** Oak has conspicuous rays; their face appearance depends on cut,
and checks may occur beside them. **Directed.** On this flat-sawn target, rays
are sparse, slender radial dashes or streaks rather than dominant
quarter-sawn fleck.

**Representation.** Seed rays in the virtual-log frame, project them through
the saw plane, and clip them by visibility. Export `ray_mask`,
`ray_direction`, and a lower-density `check_candidate` mask beside selected
rays.

**Proposed controls.**

- visible ray length: 3–35 mm;
- width after aggregation: 0.5–2.5 mm;
- only 8–20% of candidate rays receive an explicit stylized accent;
- ray contrast remains below knot-core contrast.

**Constraints.** Rays may not be random scratches. Their orientation must be
explained by the ring field and saw plane. They must not appear uniformly on
every board.

**Proof.** A ray-only plate must be distinguishable from the scratch plate by
orientation, habitat, and response.

### OAK-08 — Aggregated vessel and pore texture

**Observed.** Hardwood vessels create pores, with conspicuous earlywood pores
in ring-porous oak. **Inferred.** At this tile resolution, much anatomy is
sub-texel or near-texel and must be aggregated.

**Representation.** Place short broken pore chains inside the earlywood mask,
aligned with `growth_flow`. Render them mainly into roughness and local tone,
with height used only for the largest resolved clusters.

**Proposed controls.**

- cluster length: 2–14 mm;
- cluster duty cycle: 25–60%, leaving broken intervals;
- resolved height recession: 0.01–0.06 mm;
- contrast fades one mip earlier than the ring figure.

**Constraints.** No evenly distributed pinholes. No round pore stamp repeated
at fixed spacing. Pores may not cross from earlywood into unrelated latewood.

**Proof.** At 100% crop, pores enrich the earlywood. At 25% scale, they merge
cleanly without sparkling or changing the silhouette of the ring.

### OAK-09 — Longitudinal fibre bundles

**Observed.** Professional wood construction separates directional fibres
from growth rings. **Inferred.** Fibres carry long-axis motion but inherit
deflection near knots and figure.

**Representation.** Advect broken anisotropic strokes along `growth_flow`.
Create at least three length and width families. Apply a soft directional warp
from board form and knot influence.

**Proposed controls.**

- long bundle: 30–180 mm;
- middle bundle: 8–45 mm;
- fine aggregate: 1–8 mm;
- explicit height: 0.01–0.08 mm for selected bundles only;
- coverage cap: 20–35% of a quiet board, 35–50% near a focal event.

**Constraints.** No full-length parallel hairlines. No equal contrast. Fibres
terminate, merge, fade, and leave rests.

**Proof.** Direction histograms must follow each board's long axis but visibly
bend around the knot influence zone.

### OAK-10 — Planing and sanding memory

**Observed.** A floor surface is manufactured and finished, not a raw
cross-section. **Directed.** Subtle broad planing and sanding should soften
anatomy without erasing it.

**Representation.** Add a very low-amplitude long-axis tool field before
finish. It can attenuate selected pore and fibre height while leaving their
roughness trace.

**Proposed controls.**

- broad tool width: 12–55 mm;
- height amplitude: 0.01–0.05 mm;
- visible pass coverage: 10–25%;
- no more than one clearly readable tool pass in a 200 mm crop.

**Constraints.** This is not brushed or heavily wire-brushed lumber. Circular
sander swirls and repetitive milling ridges are excluded from the champion.

**Proof.** The tool field must disappear when height is exaggerated back to
physically plausible scale; if it reads as corrugation, reject it.

### OAK-11 — Quiet wood

**Observed.** Professional stylized materials preserve areas of rest.
**Directed.** Quietness is authored as a positive field, not whatever remains
after feature placement.

**Representation.** Store `rest_mask` per segment. It suppresses explicit
pores, fibre contrast, damage seeding, and ink, while preserving broad ring
figure and material response.

**Proposed controls.**

- 35–55% of the tile belongs to strong rest;
- every 300 mm crop contains at least one connected rest region;
- at least two boards contain no knot, long check, or focal scratch group.

**Constraints.** Rest is not flat color. It keeps macro form, restrained
pigment, and soft response.

**Proof.** A detail-priority plate must make the focal-to-rest hierarchy
obvious without showing final color.

### OAK-12 — Knot body

**Observed.** A knot is branch material intersected by the saw plane.
**Directed.** Use one focal intergrown knot, one smaller secondary knot, and at
most two pin knots in the 1.60 m tile.

**Representation.** A knot record owns centre, elliptical axes, branch tilt,
life state, core, ring halo, and influence radius. Intersect a simple tapered
branch volume with the board plane; do not stamp a blurred circle.

**Proposed controls.**

- focal visible diameter: 28–58 mm;
- secondary: 12–30 mm;
- pin knot: 3–10 mm;
- aspect ratio: 1.1–2.4;
- influence radius: 1.8–4.0 times visible knot radius.

**Constraints.** Knot aspect and surrounding deflection share the same branch
direction. Knot centres cannot sit at identical normalized positions. No knot
is allowed inside an end-joint bevel.

**Proof.** In silhouette-free base color, the knot must still read as an
embedded branch section rather than a dark dot.

### OAK-13 — Knot-driven grain deflection

**Observed.** Knots create localized cross-grain and interrupt grain
continuity. **Representation.** Warp `ring_phase` and `growth_flow` around the
knot with a smooth, asymmetric influence field. Flow divides before the knot,
wraps it, and rejoins with a small wake.

**Proposed controls.**

- maximum flow deflection: 18–48 degrees near the knot shoulder;
- upstream influence: 1.2–2.5 radii;
- downstream wake: 2–4 radii;
- asymmetry: 10–35% according to branch tilt.

**Constraints.** Grain cannot pass straight under the knot. The warp may not
form a perfect radial star or a symmetric eye at every knot.

**Proof.** A causal toggle must compare the same board with knot influence off
and on. Acceptance requires visible ring compression, wrapping, and recovery
without field folding.

### OAK-14 — Knot rings, boundary, and core

**Observed.** Knot character includes more than a single dark mass.
**Directed.** The focal knot receives a dense core, two to five compressed
boundary arcs, and one broken darker shoulder.

**Representation.** Evaluate concentric branch growth in knot-local
coordinates, clipped and distorted by the branch-plane intersection.
Separate `knot_core`, `knot_ring`, `knot_boundary`, and `knot_exposed` masks.

**Proposed controls.**

- boundary-ring count: 2–5 visible fragments;
- core value reduction: 12–28% relative to its board;
- boundary roughness: +0.03 to +0.12 if finish does not fully fill it;
- height range: -0.05 to +0.12 mm for an intergrown knot.

**Constraints.** Do not outline the complete ellipse. At least one boundary
must merge into surrounding flow.

**Proof.** The knot breakdown must remain legible in height, pigment, and
roughness independently; none may be a copy of another.

### OAK-15 — Knot checking

**Observed.** Grain distortion and seasoning make knots plausible origins for
small checks. **Directed.** Only the focal knot receives a visible check.

**Representation.** Seed one to three tapered paths from the knot core or
boundary, initially aligned with local weakness and then attracted toward
`growth_flow`. Each path owns width, depth, branching limit, and fade.

**Proposed controls.**

- length: 8–45 mm;
- opening width: 0.3–1.8 mm;
- depth: 0.08–0.45 mm;
- branches: zero or one per path.

**Constraints.** Checks may not cross the knot like a uniform starburst.
Width and depth taper toward the tip. They cannot repeat on every knot.

**Proof.** Under grazing light the check opens from the knot; under flat
albedo it remains subordinate to the knot body.

### OAK-16 — Board cup

**Observed.** Plain-sawn boards tend to cup toward the bark side because radial
and tangential movement differ. **Representation.** A low-order across-board
profile uses `bark_side`, with a small longitudinal modulation.

**Proposed controls.**

- centre-to-edge height difference: 0.15–1.20 mm;
- at least two boards nearly flat: below 0.25 mm;
- no adjacent pair shares identical sign and amplitude.

**Constraints.** The cup direction must remain consistent with the saw model.
It cannot reset at an end joint. It must not turn the floor into roof tiles.

**Proof.** A cross-board profile plate names bark side and cup sign for every
board. A grazing render must show variation before grain or damage is enabled.

### OAK-17 — Bow and local plane

**Observed.** Moisture history and installation can leave boards subtly out of
plane. **Directed.** Use long, low-amplitude bow and tiny segment seating
differences.

**Representation.** Each physical board receives one or two longitudinal
Bezier-like height trends. Segment seating is a separate constant or slow
edge-safe field.

**Proposed controls.**

- bow amplitude: 0–0.8 mm over 0.6–1.6 m;
- segment seating difference: -0.25 to +0.35 mm;
- slope limited so no board appears detached.

**Constraints.** Bow must not be generic FBM. The profile is deliberate,
long-range, and periodic where the tile wraps.

**Proof.** A height-only clay render at grazing light must distinguish at least
three board plane behaviours without visible micro-detail.

### OAK-18 — Long-edge shoulder

**Observed.** Installed plank edges may be sharp, bevelled, softened, or worn.
**Directed.** Use a mostly square aged shoulder with slight, non-uniform
rounding.

**Representation.** Long-edge distance creates the structural gap and bevel.
A low-frequency wear field changes the shoulder radius along `s`; isolated
compression or loss modifies only selected stretches.

**Proposed controls.**

- structural gap: 1.2–3.0 mm;
- shoulder width: 0.8–2.5 mm;
- local softened stretches: 25–120 mm;
- edge loss depth: 0.1–0.7 mm, on fewer than 8% of long-edge length.

**Constraints.** Pigment cannot fake the gap. Both adjacent boards contribute
to the profile. Every edge must not receive identical wear.

**Proof.** Cross-sections at quiet, softened, and damaged edges must differ.
Normal, AO, and albedo joint positions must agree within one texel.

### OAK-19 — End-joint history

**Observed.** End joints expose a different cut and are common sites for
opening and checks. **Directed.** End joints are slightly more irregular than
long joints but remain installed-floor scale.

**Representation.** Store end distance independently. Add a small saw-plane
tilt, selected end darkening, and optional end check. The same board on both
sides of the tile wrap keeps its cyclic identity.

**Proposed controls.**

- end gap: 1.5–3.8 mm;
- end height mismatch: 0–0.45 mm;
- darkened contact band: 1–5 mm with broken coverage;
- only 20–40% of visible ends receive a check.

**Constraints.** Do not outline every end. No repeated horizontal dark bar.
End grain is implied at this resolution rather than rendered as a miniature
tree cross-section.

**Proof.** An end-joint strip shows at least one quiet, one slightly open, and
one checked joint. The 4× tile must not reveal a repeated ladder.

### OAK-20 — Ray-guided surface checks

**Observed.** Checks can form beside oak rays and during seasoning.
**Representation.** Select from `ray_mask` and broader drying-stress regions;
grow a tapered longitudinal or oblique path whose initial direction respects
the local ray and ring geometry.

**Proposed controls.**

- visible length: 12–95 mm;
- width: 0.2–1.5 mm;
- depth: 0.05–0.35 mm;
- two to six independent surface checks across the full tile.

**Constraints.** Checks cannot wander like contour noise, tile from edge to
edge, or occur at uniform density. They stop, pinch, and occasionally reopen.

**Proof.** A lineage plate overlays source ray, stress candidate, selected
path, and final check. Every accepted check must have a named origin.

### OAK-21 — End checking and small splits

**Observed.** End checks are seasoning separations beginning at an end.
**Directed.** Use them as rare punctuation, not as a damage border.

**Representation.** Spawn from selected end joints into the board along a
distorted growth-aware path.

**Proposed controls.**

- length: 10–75 mm;
- one main path, with zero or one short fork;
- opening is widest at the end and closes inward;
- no more than three visible end checks in the champion.

**Constraints.** An end check must touch an end joint. A floating interior
crack belongs to OAK-20 or the knot system.

**Proof.** Feature ownership validation rejects any `end_check` whose source
point is outside the end-joint habitat.

### OAK-22 — Splinter and fibre lift

**Observed.** Deeper edge damage can lift or remove fibres along the grain.
**Directed.** The cared-for interior target allows one or two small edge
splinters, never a shredded perimeter.

**Representation.** Begin at a damaged shoulder or check. Grow a tapered
sliver along `growth_flow`, with paired negative height beside a small raised
lip.

**Proposed controls.**

- length: 8–38 mm;
- width: 0.6–3.0 mm;
- recessed depth: 0.15–0.8 mm;
- raised lip: 0.05–0.25 mm.

**Constraints.** No splinter crosses the grain. Raised lips must be clamped to
avoid silhouette-breaking spikes in the normal.

**Proof.** A grazing-light crop must show the paired recess and lift. If it
reads only as a dark painted line, reject it.

### OAK-23 — Dents and compression

**Observed.** Furniture and impact can mark floor faces. **Directed.** Use
small, soft dents with compressed finish, concentrated away from protected
rest.

**Representation.** Elliptical or irregular shallow depressions with a broad
shoulder. Orient some according to plausible drag direction; do not use round
noise dots.

**Proposed controls.**

- diameter: 3–18 mm;
- depth: 0.03–0.35 mm;
- zero to three clusters;
- one cluster may contain two overlapping dents.

**Constraints.** Dents do not darken uniformly. A compressed or polished
centre may be smoother while its rim catches dirt.

**Proof.** Height, roughness, and color plates must show different but causally
related footprints.

### OAK-24 — Scratches and scuffs

**Observed.** Debris, furniture, heels, and ordinary use can scratch a floor.
Low sheen makes small scratches less visually obvious. **Directed.** Scratches
are grouped events with plausible motion, not isotropic grunge.

**Representation.** Each scratch group owns direction, curvature, length
family, pressure profile, and finish penetration. Most affect roughness and
finish; only the deepest affect wood height.

**Proposed controls.**

- group count: 3–7;
- members per group: 1–6;
- length: 8–140 mm;
- width: 0.2–1.2 mm;
- only 10–25% penetrate into wood height.

**Constraints.** Parallel group members share a cause but vary in start and
length. Groups cannot align with every board or cover the entire tile.

**Proof.** A finish-only toggle must reveal most scratches; a height-only
toggle must reveal only the deepest subset.

### OAK-25 — Traffic polish

**Observed.** Repeated use changes the finish, and sheen controls how visible
small wear becomes. **Directed.** A broad, broken traffic lane crosses several
boards without becoming a painted stripe.

**Representation.** Author a world-space `traffic_field` independent of board
ownership. It reduces roughness slightly, softens edge and fibre relief, and
changes dirt retention. Board response modulates its strength.

**Proposed controls.**

- lane width: 0.28–0.65 m;
- roughness reduction at centre: 0.03–0.12;
- boundary transition: 80–220 mm;
- coverage broken by 20–45% so the path is inferred, not diagrammed.

**Constraints.** The lane may cross boards, but it cannot move knots or rings.
It must not bake a highlight or directional scene light into base color.

**Proof.** Under a moving specular sweep, the lane emerges and recedes. Under
flat unlit albedo, it is nearly invisible.

### OAK-26 — Finish film and worn finish

**Observed.** Penetrating oil and wax-like finishes can be warm and low
luster. **Directed.** The target has a thin amber finish with selected wear,
not thick glossy polyurethane.

**Representation.** Store `finish_thickness`, `finish_amber`,
`finish_roughness`, and `finish_loss`. Finish partially fills pores and small
fibre relief. Loss exposes a slightly paler, rougher wood response.

**Proposed controls.**

- broad finish roughness: 0.58–0.76 before engine remapping;
- board-to-board bias: ±0.04;
- finish loss: 2–12% of tile, concentrated at traffic, scratches, and
  shoulders;
- amber color contribution: 2–8%, never a uniform orange overlay.

**Constraints.** Finish loss cannot be independent clouds. It must follow use
or edge exposure. Height does not change unless the wood itself is damaged.

**Proof.** A finish breakdown must show raw wood, finished wood, finish loss,
and final response under the same light.

### OAK-27 — Joint dirt and embedded grime

**Observed.** Dirt is transported by use and retained by recesses.
**Directed.** Dirt is restrained and local; the floor is old, not abandoned.

**Representation.** Derive candidates from cavity, edge distance, traffic, and
low spots. Apply a clumped deposition field with board-dependent adhesion.

**Proposed controls.**

- strong dirt occupies less than 4% of the tile;
- faint accumulation extends 1–6 mm from selected joints;
- deep checks may receive more dirt than shallow scratches;
- hue is neutral/cool brown, not pure black.

**Constraints.** No global grunge multiplication. Convex raised centres should
not receive the same dirt as recesses.

**Proof.** A habitat plate compares candidate recesses with selected deposits.
At least half of the structural joints remain visually quiet.

### OAK-28 — Heartwood and restrained pale transition

**Observed.** White-oak sapwood is pale and heartwood ranges through light to
darker brown. **Directed.** The champion is heartwood-dominant, with at most
one narrow pale transition at a board edge.

**Representation.** Board family controls the heartwood base. If present, a
sapwood-transition field follows the virtual-log geometry rather than a
straight painted strip.

**Proposed controls.**

- palette centred on muted tan, honey, and neutral brown;
- broad luminance variation between boards: roughly 6–16%;
- pale transition coverage: 0–6% of tile;
- saturation kept low enough for colored scene light and graphic ramps.

**Constraints.** Do not use random per-cell brown noise. Knots, earlywood,
finish, and dirt remain separate color causes.

**Proof.** A five-value posterization of base color must retain broad board and
cathedral grouping without becoming camouflage.

### OAK-29 — Mineral or cool streak

**Observed.** Oak grading examples include mineral streaks and natural color
variation. **Directed.** One subtle cool-gray or olive-brown streak may add
history without implying rot.

**Representation.** Author a narrow, softly branching longitudinal field that
follows growth flow and fades across rings. It affects pigment more than
height.

**Proposed controls.**

- length: 80–320 mm;
- width: 4–22 mm;
- value change: 2–7%;
- saturation change: 3–10%;
- zero or one focal streak in the champion.

**Constraints.** It cannot be a straight gradient or repeat on neighbouring
boards. It does not create a crack.

**Proof.** The streak is recognizable in pigment isolation and absent from
height isolation.

### OAK-30 — Height and normal discipline

**Observed.** Physical form must agree across response channels. **Directed.**
Height is an accountable sum of board form, joints, anatomy, and actual
damage—not a receptacle for every visible mark.

**Representation.**

```text
height =
    board_seating
  + cup
  + bow
  + edge_profile
  + resolved_anatomy
  + dents
  + checks_and_splinters
```

Derive the tangent-space normal once, after the final height. Roughness-only
pores and finish scratches do not secretly enter the normal.

**Proposed controls.** Preserve metre-valued intermediate layers. Provide an
art-review gain for proofs, but label it and never bake the exaggerated gain
into delivery maps.

**Constraints.** No independent normal noise. No AO painted into base color.
No height remapping that makes 0.05 mm anatomy as deep as a 2 mm joint.

**Proof.** Every visible normal event can be selected in a height-lineage
legend. Automated checks compare seam and joint positions across height,
normal, AO, and color.

### OAK-31 — Roughness as material history

**Observed.** Finish type, sheen, scratches, pores, and traffic affect
reflection differently. **Representation.** Begin with broad finish
roughness, then apply bounded, named modifiers:

```text
roughness =
    finish_base
  + exposed_wood
  + earlywood_porosity
  + deep_damage
  + embedded_dirt
  - traffic_polish
  + scratch_response
```

Each modifier uses its own remap and may be disabled independently.

**Proposed controls.** Keep most finished wood within a coherent matte band;
use extrema only at exposed wood, polished traffic, or deep dirt. Limit
fine-scale contrast so it does not sparkle at the first mip.

**Constraints.** Roughness cannot be a grayscale copy of height or base color.
It may share causes, never the complete signal.

**Proof.** A roughness-only sphere, flat plane, and grazing strip must reveal
board family, traffic, and damage at different scales.

### OAK-32 — Cavity and occlusion

**Observed.** Deep joints and checks occlude more than shallow pigment events.
**Representation.** Derive cavity from physical height at multiple radii.
Combine only at response time; keep it out of pigment.

**Proposed controls.**

- tight radius: pores, checks, small dents;
- middle radius: bevels, knot recess;
- broad radius: joints and seating;
- broad AO remains subtle on an open floor plane.

**Constraints.** No black halo around every grain line. A ray, mineral streak,
or finish scratch with no depth produces no cavity.

**Proof.** A white-clay response plate must show deep hierarchy without making
the floor look soot-outlined.

### OAK-33 — Structural ink

**Directed.** Ink is an asset-authored semantic lane for graphic rendering. It
selects significant structure; it is not baked lighting.

**Representation.** Build `ink_mask` from named candidates: deepest joint
fragments, focal knot boundary, selected checks, one or two fibre sweeps, and
rare damaged shoulders. Store `ink_class` or separate masks if the renderer
needs different weights.

**Proposed controls.**

- strong ink: less than 3% of tile;
- soft ink: less than 9%;
- at least 60% of joint length receives no strong ink;
- line width varies 2:1 or more within a mark;
- fade, break, and taper are authored in board coordinates.

**Constraints.** No complete outline around every board or knot. No ink from
arbitrary luminance. Ink does not modify height.

**Proof.** The ink-only plate must look intentionally composed at 1× and not
form a cage at 4×. Three light directions must not reveal baked-shadow
contradictions.

### OAK-34 — Highlight strokes

**Directed.** Sparse highlight strokes make selected raised fibres, softened
edges, or knot shoulders readable under a cel-shaded response.

**Representation.** `highlight_stroke_mask` selects physically plausible
convex or tangent-facing structures but remains independent of scene light.
The renderer decides whether current lighting activates it.

**Proposed controls.**

- coverage: 1–5%;
- stroke length: 6–80 mm;
- three width families;
- no more than one dominant highlight gesture per 250 mm crop.

**Constraints.** Do not paint a permanent white rim. Strokes cannot appear in
deep gaps or on every fibre.

**Proof.** With neutral activation, strokes clarify flow. With activation off,
the physical material still reads as oak.

### OAK-35 — Brush direction and pigment gesture

**Directed.** Painterly breakup follows the wood, installation, and wear
story. **Representation.** Export a two-channel `brush_direction` field
derived primarily from `growth_flow`, blended toward traffic direction in
finish-wear regions and toward edge tangents on softened shoulders.

Use it to place broad translucent pigment gestures, not opaque hairlines.

**Proposed controls.**

- broad stroke width: 8–45 mm;
- length: 30–240 mm;
- pigment shift: 1–6% value and 1–8% saturation;
- 40–70% of potential strokes are suppressed by rest and hierarchy masks.

**Constraints.** Brush flow cannot ignore the board frame. The same generic
brush texture cannot cover every board. A stroke may cross boards only when it
represents world-space traffic or finish application, not wood anatomy.

**Proof.** A direction-field plate and a pigment-only plate must make each
stroke's coordinate owner obvious.

### OAK-36 — Detail priority and mip survival

**Directed.** The material states what must survive distance. **Representation.**
Export `detail_priority` with four semantic levels:

1. board/joint composition;
2. cathedral and macro form;
3. focal knots, checks, and wear;
4. pores, fibres, and minor scratches.

The renderer or texture-build step may use this to bias filtering, contrast, or
stylized stroke selection.

**Proposed controls.** Levels 1–2 survive the room view. Level 3 survives the
mid view. Level 4 is close-view reward and must disappear cleanly.

**Constraints.** Priority is not edge strength. A quiet board can have high
priority at its silhouette and low priority inside.

**Proof.** A prescribed mip ladder shows 100%, 50%, 25%, 12.5%, and 6.25%.
Failure includes sparkling pores, vanishing board cadence, or ink thickening
into black bars.

## Variation policy

The first successful asset is authored, not randomized. Variation begins only
after the champion specimen proves the grammar.

### Locked identity

Every valid variation preserves:

- white-oak ring-porous anatomy;
- predominantly flat-sawn figure;
- a warm low-luster penetrating finish;
- the same physical tile scale;
- sparse focal events and large areas of rest;
- the dependency graph between cut, rings, knots, checks, wear, and response.

### Movable composition

A later variation script may change:

- board widths and cyclic joint positions within construction bounds;
- pith offsets, slow ring-spacing history, and bark side;
- board families and their restrained palette;
- the selection, scale, and orientation of pre-authored knot types;
- which ray or end candidates become checks;
- traffic path and scratch groups;
- where ink and painterly accents select existing causes.

### Forbidden randomization

The script may not independently randomize:

- grain direction after a knot has been placed;
- cup sign without changing bark side;
- checks without a knot, ray, end, or drying-stress habitat;
- dirt without recess or use history;
- roughness without finish or material structure;
- ink from final luminance;
- damage density per pixel;
- every board to the same level of busyness.

Use bounded family selection, smooth correlated fields, Poisson-like event
spacing, and explicit exclusions. Never use one seed to create the impression
that thirty unrelated sliders constitute art direction.

## Hand-authored champion specimen

The next implementation should describe nine boards before it synthesizes
pixels:

1. **Quiet warm heartwood:** broad opening cathedral, slight cup, no focal
   damage.
2. **Focal knot board:** one intergrown knot with asymmetric ring deflection
   and one internal check; otherwise restrained.
3. **Calm near-rift transition:** straighter figure, shallow bow, traffic
   polish, no knot.
4. **Aged figured board:** closing cathedral, one ray-guided check, one worn
   shoulder.
5. **Narrow supporting board:** muted value, secondary knot or pin knot, no
   long damage.
6. **Cool-streak board:** subtle mineral/color streak and selected pores, no
   knot.
7. **Rest and wrap board:** quiet figure designed so the tile boundary is not
   a focal event.
8. **Pin-knot transition:** one very small knot punctuates an otherwise calm
   figure without becoming a second focal event.
9. **Boundary rest:** a broad quiet board keeps the final tile edge from
   competing with the focal knot.

For each board, author these values explicitly:

```text
board_id
width_m
cyclic_end_positions_m
board_family
pith_offset
saw_angle_deg
bark_side
ring_spacing_epochs
cup_profile
bow_profile
rest_regions
knot_records
ray_accent_records
check_records
edge_wear_records
finish_loss_records
ink_selections
highlight_selections
```

The JSON recipe should name intentions such as `focal_intergrown_knot` and
`quiet_open_cathedral`, not expose anonymous arrays whose meaning exists only
inside the generator.

## Acceptance proof suite

The proof is adversarial. It is designed to expose a weak material, not decorate
it.

### Plate A — Source-to-field anatomy

Show the saw-plane model, ring phase, earlywood, latewood, rays, pores, fibres,
knot volume, and knot influence. Annotate which source fact motivated each
field. Acceptance requires that disabling one upstream field produces the
expected downstream change.

### Plate B — Causality toggles

Use identical inputs and compare:

- flat stripes versus virtual-log intersection;
- knot body without influence versus knot-driven grain deformation;
- checks sampled freely versus ray/end/knot-guided checks;
- generic noise roughness versus named finish-history modifiers.

The old or deliberately wrong state is part of the proof.

### Plate C — Form under hostile light

Render white clay on:

- a flat floor plane;
- a 30-degree plane;
- a rounded step or cylinder;
- an inside corner.

Use frontal, three-quarter, and grazing light. Show macro form alone, then
anatomy, then damage. Acceptance requires board form to read before
micro-detail.

### Plate D — Color and response independence

Show:

- broad board pigment;
- anatomy pigment;
- finish amber and loss;
- dirt;
- raw roughness modifiers;
- final roughness;
- final unlit base color.

Reject any stage where base color depends on AO or where roughness is a copy of
height.

### Plate E — Stylization lanes

Show ink, highlight strokes, brush direction, pigment gesture, and detail
priority independently. Render the physical material with stylization off,
then add each lane. Acceptance requires every addition to clarify hierarchy
instead of merely increasing contrast.

### Plate F — Repetition and distance

Show 1×, 2×, 4×, 8×, and 16× repetition plus the full mip ladder. Include a
rotated crop so line aliasing cannot hide. Acceptance requires:

- no repeated knot constellation dominating the room view;
- no ladder of end joints;
- no uniform alternating board palette;
- no barcode grain;
- no pore sparkle;
- no black ink cage;
- no seam discontinuity in height, normal, roughness, or stylization lanes.

### Plate G — Matched professional review

Link, but do not redistribute, two professional wood-material examples. Match
crop scale and approximate light direction. Annotate differences in:

- cathedral and knot construction;
- board form;
- middle-frequency anatomy;
- edge and damage history;
- finish response;
- areas of rest;
- stylized mark selection;
- room-scale repetition.

“Looks good” is not an acceptance statement. Every comparison must name what
the candidate still lacks.

## Automatic checks

Automation cannot judge poetry, but it can reject contradictions:

- all output seams match within channel-specific tolerance;
- every feature belongs to a valid board and segment;
- every end check touches an end habitat;
- every ray-guided check names a ray or stress candidate;
- knot influence radius exceeds knot-body radius;
- growth flow remains finite and does not fold;
- ring order is monotonic in the virtual-log model;
- cup sign agrees with `bark_side`;
- normal is derived from final height;
- metallic is zero;
- strong ink and highlight coverage remain below their caps;
- rest coverage remains above its minimum;
- texture values remain finite and inside channel ranges;
- identical recipe and seed produce identical outputs;
- a variation changes selected authored degrees of freedom without changing
  the locked identity.

Passing these checks proves consistency, not visual quality. The hostile-light,
matched-reference, distance, and human art reviews remain mandatory.

## Immediate reject conditions

Reject the specimen before polishing if any of these are visible:

- rings read as equally spaced sine waves;
- knots read as dark stickers over straight grain;
- all boards are equally busy;
- every joint is a complete dark outline;
- scratches are isotropic random lines;
- rays cannot be distinguished from scratches;
- pores are uniform pinholes;
- dirt is a global grunge multiply;
- finish wear changes wood height without physical damage;
- roughness copies height;
- ink substitutes for missing edge form;
- micro-detail survives farther than the board composition;
- the most memorable knot repeats as a grid;
- the seam is technically continuous but compositionally obvious.

## Implementation threshold

The grammar is ready to become code when the next implementation can answer
these questions before drawing a pixel:

1. Which physical board owns this point?
2. What saw-plane and bark-side history does that board have?
3. What is the continuous ring phase and growth direction here?
4. Is this earlywood, latewood, ray habitat, or a protected rest?
5. Is a knot deforming the material here?
6. Which named cause permits this check, dent, scratch, dirt, or finish loss?
7. Does the mark belong in height, pigment, roughness, stylization, or several
   related-but-distinct channels?
8. At what viewing distance should it disappear?

If the script cannot answer one of those, it does not yet have enough intent to
generate that feature.

## Prototype-to-implementation translation

The next pass must replace mechanisms, not rename current masks.

| Current prototype shortcut | Why it fails | Required replacement | First isolating proof |
|---|---|---|---|
| broad cosine grain | creates a dominant repeating interval and barcode read | virtual-log ring intersection with integrated spacing history | ring phase, spacing graph, and 4× repetition |
| fine cosine grain | gives every fibre equal length, spacing, and importance | broken anisotropic fibre families advected through growth flow | fibre-only crop plus direction histogram |
| blurred knot dot | has no branch section, boundary rings, or life history | tapered branch-plane intersection with core, rings, shoulder, and state | knot components in isolation |
| small local phase bend | grain still appears to pass under the knot | asymmetric upstream wrap and downstream wake in ring phase and flow | knot causality toggle |
| generic wandering rift | has no reason to exist where it appears | typed knot, ray-guided, or end-guided check with habitat validation | source-to-path lineage plate |
| uniform distance bevel | gives every board the same untouched manufactured edge | structural profile plus authored softened, compressed, and lost stretches | three labelled edge cross-sections |
| global broad and fine noise | lays one unrelated weather field over all boards | board-owned macro form, anatomy-owned meso structure, finish-owned response | frequency ladder with owner IDs |
| per-segment brown palette | changes hue without changing material history | stable board families carrying cut, rings, cup, finish, and restrained pigment | family plate with color disabled and enabled |
| additive roughness noise | has no finish or traffic story | named raw-wood, finish, pore, damage, dirt, polish, and scratch modifiers | roughness modifier stack under specular sweep |
| dark structural lines in base color | turns every joint into a permanent graphic outline | physical gap plus sparse, separately exported semantic ink | three-light comparison with ink on and off |
| generated chapter images as proof | explains construction but does not challenge quality | hostile-light, causality, mip, matched-reference, and repetition plates | Plates A–G |

The first coding milestone is therefore not “make twelve random woods.” It is:
author the nine-board champion recipe, implement the virtual-log ring and knot
influence fields, and prove the anatomy and causality plates before color,
damage, or stylization is allowed to hide them.
