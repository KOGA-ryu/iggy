# Forged-Iron Component Response Audit

## Purpose

This audit determines why the selected v004.1 intact forged-iron response
remains too smooth on the actual hinge. It does not design a replacement
pattern. Its job is to establish which construction event owns each visible
surface, measure the current broad-normal field, and reject any coordinate or
recipe that contradicts how the hinge was made.

The consumer remains the saved eight-object openwork hinge. Colour, oxide
coverage, roughness, shader topology, geometry, damage, and condition are
frozen during this audit.

## Construction anatomy

### Moving leaf stock

`SM_GH018_MovingLeaf_Openwork`, `MovingKnuckle_01`,
`MovingKnuckle_03`, and `MovingKnuckle_05` are one material lineage. The
knuckles are tabs or projections of the parent flat strap that have been
formed into the eye. They are not independent pieces of cylindrical stock.

### Fixed leaf stock

`SM_GH018_FixedLeaf`, `FixedKnuckle_02`, and `FixedKnuckle_04` are the second
material lineage. Their response must likewise remain continuous from the
unrolled leaf into its formed eye tabs.

### Pintle stock

`SM_GH018_Pintle` is separate forged round pin stock. Its manufacturing frame
and finish do not inherit either leaf field.

## Written evidence ledger

### 1. William L. Ilgen, *Forge Work* (1912)

- Authority: public-domain manual by a forging instructor, preserved as a
  [Project Gutenberg text](https://www.gutenberg.org/files/53854/53854-h/53854-h.htm).
- Observation: upright blows are used both to draw flat material and make
  smooth surfaces. Round work is best drawn in swages; when swages are absent,
  the work is progressively made square, octagonal, and then round with light
  blows while being revolved continuously.
- Eligible transfer: flat strap should read as broad, overlapping, mostly
  smoothed forging influence. A forged pintle may retain very restrained axial
  or progressive-facet response, but not periodic rings.
- Prohibited inference: the manual supplies operations, not a universal hammer
  diameter, mark spacing, residual depth, or fixed visible facet count for this
  giant hinge.

### 2. Thomas F. Googerty, *Practical Forging and Art Smithing* (1915)

- Authority: public-domain wrought-metal manual from a master craftsman,
  available through the
  [Blacksmiths Association of Missouri archive](https://www.bamsite.org/books/cu31924003588534.pdf).
- Observation: the eye is the first hinge operation; it may be loose or welded,
  a sizing eye-pin trues the hole, and the weld lap belongs on the back. For a
  heavy joint the projections are marked while the stock is flat, the bar is
  split lengthwise, the eye is formed and welded, and alternating projections
  are cut and fitted.
- Eligible transfer: the openwork model's alternating knuckles belong to their
  parent leaf in an unrolled stock coordinate. A rear weld-seam identity and
  bearing-face identity may later be separate semantic masks.
- Prohibited inference: the text does not prove that every surviving museum
  hinge was welded, nor does it authorize a universal dark seam, crack, ring,
  or repeated knuckle decoration.

### 3. J. B. Stokes, FAO *Basic Blacksmithing: A Training Manual* (1992)

- Authority: written training manual published by the
  [Food and Agriculture Organization of the United Nations](https://www.fao.org/4/ah637e/ah637e00.htm).
- Observation: the strap-hinge job begins with 25 or 30 by 6 mm flat strap. The
  end is chamfered, bent over the rounded anvil edge into an eye, fitted to a
  12 mm drift, and flattened in an 18 mm bottom swage before punching and final
  wire brushing. The paired hinge-pin job uses separate round stock and bottom
  swages.
- Eligible transfer: eye response must deform with the original flat strap;
  drift and bottom-swage work can justify a low-amplitude eye-local smoothing
  or compression owner after stock continuity exists. Pintle response is a
  separate recipe and coordinate.
- Prohibited inference: these human-scale job dimensions are evidence for
  construction order and tool relationships only. They are not literal
  dimensions for the giant-house hinge and may not be multiplied into texture
  sizes without an explicit scale argument.

### 4. Field's Blacksmith Shop at The Farmers' Museum

- Authority: practicing historical-reproduction shop documenting replacement
  barn hardware in
  [Hinges, Pintles and Gudgeons](https://ruralblacksmith.blogspot.com/2009/09/fields-blacksmith-shop-is-making.html).
- Observation: the hinge begins as one 18-inch bar; its barrel or eye is forged
  and rolled, a sizing pin establishes the gudgeon, then the bar is drawn out at
  hammer and anvil. The pintle is forged as the separate mating component.
- Eligible transfer: the causal sequence is eye/roll, size, then draw the strap;
  leaf and eye share stock history while pintle does not.
- Prohibited inference: one shop example cannot establish a universal visual
  finish, number of blows, oxide colour, or wear state.

### 5. Cloverfields Preservation Foundation reconstruction

- Authority: documented late-seventeenth-century door reconstruction by an
  architectural historian, blacksmith, carpenter, and millwork specialist,
  described in
  [Recreating and Installing a Seventeenth-Century Door](https://www.cloverfieldspreservationfoundation.org/newsletters/cellar-door-hardware).
- Observation: the wrought-iron strap is wrapped around the eye and forge
  welded on the rear. Careful work leaves a slight feathered seam; cruder work
  may leave the wrap unwelded. The pintles are separately hand forged.
- Eligible transfer: a weld seam is sparse construction evidence restricted to
  the rear eye overlap, and it must be separately addressable rather than
  painted around every barrel. Pintle and leaf are distinct stock lineages.
- Prohibited inference: the reconstruction does not prove the exact seam
  location or welding state of the current fantasy hinge. Until geometry or a
  named reference establishes that state, the seam remains an optional later
  semantic lane.

## Evidence synthesis

The written sources agree on the causal hierarchy:

1. Start with flat strap stock.
2. Mark, split, chamfer, or otherwise prepare the eye region while accessible.
3. Bend or roll that parent stock around a drift, sizing pin, or pintle.
4. Weld and feather the rear overlap where the construction calls for it.
5. True and smooth the eye with the drift, swage, and controlled blows.
6. Forge the pintle from separate stock and finish its round bearing form while
   rotating it or using an appropriate swage.

This hierarchy rules out the current shape-only ownership model. A cylindrical
knuckle does not automatically receive a generic cylinder pattern. It first
inherits the flat strap's rest-stock field, deformed through the roll. Only
then may eye-local sizing, weld, and bearing responses be added in separate
semantic lanes.

## Current implementation hypothesis

The v004.1 generator currently gives each Blender object its own normalized
field frame and object-name seed. It also invokes the same three-rail,
sixteen-knot broad forging recipe independently on leaf, pintle, and every
knuckle. This creates two opposed errors:

- the normalized recipe restarts and can repeat the same mathematical gesture
  on all eight objects;
- fields that should continue from a parent leaf into its rolled knuckles lose
  phase, scale, and seed continuity at the object boundary.

The existing `circumferential_barrel` tangent is locally plausible for the
rolled section, but it does not preserve the unrolled stock coordinate or
parent lineage. The existing `axial_pin` mode is a plausible starting frame for
the pintle, but the pintle still receives the leaf's normalized broad-rail
recipe and amplitude rule.

## Audit decision rule

Reject the current broad-normal owner if the measurements show any of the
following:

- one standardized height gesture is highly correlated across unrelated
  components;
- the coordinate frame restarts across a leaf and its knuckles;
- selected-C normal angles remain below a useful actual-asset response despite
  the 1.75 multiplier;
- feature wavelengths are defined mainly by whole-component normalization
  instead of metre-scale manufacturing evidence;
- the pintle inherits a leaf-oriented recipe rather than a separate
  round-finishing construction owner.

Passing this audit does not select a new field. It only establishes the inputs
for the next reduced decision board.

## Measured v004.1 result — 2026-08-02

The deterministic audit is stored at
`output/forged_iron_v004_1_component_response_audit_v1/manifest.json`, with the
four-panel board at
`output/forged_iron_v004_1_component_response_audit_v1/forged_iron_v004_1_component_response_audit_board.png`.
The board legend is:

1. stock lineage;
2. current per-object normalized U/V frame;
3. integrated broad height on one global metre scale;
4. integrated normal angle on one fixed zero-to-two-degree scale.

### Correlation result

All eight standardized broad-height fields were resampled to the same 128 by
64 diagnostic grid. Off-diagonal Pearson correlation ranges from
`0.9999968312` to `1.0`, with a median indistinguishable from `1.0`. The
generator is not producing related but distinct construction evidence. It is
restarting essentially one identical three-rail, sixteen-knot gesture at every
object boundary.

The three moving knuckles are numerically identical to one another. The two
fixed knuckles are likewise identical to one another. This is an explicit
repeated-motif failure even though its final normal amplitude is subtle.

### Height and wavelength result

The integrated broad-height peak-to-peak values are:

| Host | Peak-to-peak height | Dominant local-U wavelength |
| --- | ---: | ---: |
| Fixed leaf, 0.852 m long | 9.348 mm | 0.428 m |
| Moving leaf, 3.112 m long | 9.432 mm | 1.558 m |
| Pintle, 0.410 m axial span | 3.675 mm | 0.207 m |
| Moving knuckles | 2.609 mm | 0.139 m |
| Fixed knuckles | 2.609 mm | 0.126 m |

Each dominant U wavelength is approximately half its host span, while the
dominant V wavelength is approximately the entire second-axis span. The field
therefore scales with object dimensions rather than keeping a manufacturing
feature size in metres. A longer hinge receives a longer wave; a shorter
knuckle receives the same wave compressed into its own frame.

### Normal-angle result

The selected 1.75 multiplier is not failing because it produces zero slope.
Median normal angles range from 0.96 to 1.47 degrees; 95th-percentile angles
range from 1.84 to 2.84 degrees, and maxima range from 2.10 to 3.14 degrees.
Those values are already substantial for a rough intact dielectric surface.

The visual proof still reads smooth because the angles belong to one or two
whole-host lobes. Increasing strength again would deepen a large smooth warp
and expose the repeated rail gesture. It would not create a missing
broad/medium/micro manufacturing hierarchy.

### Ownership result

- `circumferential_barrel` is retained only as a secondary local frame for
  eye-specific operations. It cannot own the base stock field because it loses
  parent-leaf phase and absolute cross-stock position.
- `axial_pin` is retained as the pintle's local construction frame, but the
  pintle must stop reusing the leaf's three-rail recipe.
- object-name seeds are rejected for any field shared by a leaf and its rolled
  knuckles;
- the two leaf lineages require a rest-stock coordinate that survives the roll;
- eye drift/swage response, rear weld seam, and bearing face remain separate,
  optional later owners rather than excuses for a generic cylinder pattern.

Decision: `reject_shape_only_global_normal_owner`. No material candidate was
created and the v004.1 source hash remained unchanged.
