# Forged-Iron Research to Material Intent

## Surface sentence

Clean hot-worked architectural iron retains a compact adherent blue-black
oxide skin whose broad heat and scale variation crosses each component, while
finite hammer-plane changes and restrained local worked-face direction alter
reflection without turning every impact into exposed silver metal.

## Source ledger

| ID | Authority | Evidence | Used for | Transfer limit |
| --- | --- | --- | --- | --- |
| S01 | Metropolitan Museum of Art, door 55.61.170 | catalogue object, wood and iron, 1740 x 1092 x 165 mm | construction/value segmentation and iron-on-oak hierarchy | photograph contains age, shadow, wood, and oxidation; pixels are not runtime albedo |
| S02 | Met hinges 55.61.58 and 55.61.63 | catalogue envelopes 406 x 41 mm and 349 x 38 mm | human-scale strap length/width | thickness, taper, holes, and finish are not published |
| S03 | Colonial Williamsburg forging hammer 2020-20,16 | face 41.275 x 31.75 mm; cross-peen 36.5125 x 12.7 mm | maximum finite tool envelopes | later period; a complete face print is not assumed |
| S04 | Canadian Conservation Institute, Care and Cleaning of Iron | stable compact/adherent iron ranges from silver-gray through blue-black and red-brown; orange crystal/flaking states are active corrosion | intact state and exclusions | colour prose is not a measured spectral albedo |
| S05 | Historic England, *An Investigation of Hammerscale* | high-temperature scale phase order; ordinary-forging oxide-thickness proxy around 50 micrometres, welding proxy around 300 micrometres | physical oxide-layer model and relief ceiling warning | oxide thickness is not automatically visible surface-height amplitude |
| S06 | National Heritage Ironwork Group commissioning guidance | charcoal iron is forged and slightly uneven; later rolled material is smoother | broad planar irregularity and rejection of rolled-sheet smoothness | period and manufacturing family must stay explicit |
| S07 | Doug Bracken, written hot-iron finish method | hot scale is wire brushed; fine hand-wrought finish is subtle, preserves joinery, and brushing changes luster | comparative worked-face method | modern finish method, not proof of medieval wire brushing |
| S08 | PFERD and Osborn written brush specifications | modern steel wire diameters 0.30-0.35 mm | lower physical reference for a comparative brush response | wire diameter is not visible bundle spacing |
| S09 | B. Gerin et al., hot-forged C70 surface study | Ra 6.4 micrometres, 2 micrometre standard deviation, 2.5 mm cutoff | modern measured micro-topography proxy | same Ra can hide different topographies; not medieval metrology |
| S10 | Tim Wilmsen, BG3 Iron Throne modular kit | shipped-use imagery: broad dark plate groups, construction edges/rivets first, subordinate streaking and mottling | scene-use value and distance hierarchy | no dimensions or pixels transfer |
| S11 | User-supplied Adrien Roose brushing/metal articles | written image translation: directional filtering, transformed passes, normal-channel detail, curvature/dirt masks, local manual correction | ordered surface-method stack | legacy gloss blend modes translate to physical roughness and masks, not literal node names |
| S12 | Blender 5.1 local API and official manual | Tangent node can derive anisotropic tangent from a named UV map; Principled exposes anisotropy and rotation | executable shader method | Blender proof does not establish Unreal parity |

## Observation, intent, implementation, prohibition

| Observation | Intent | Implementation | Prohibition |
| --- | --- | --- | --- |
| intact scale is a physical dielectric layer | keep a continuous dark skin | dielectric Principled response is the default and metalness map is zero | hammer activity may not expose conductor |
| wrought surfaces are slightly uneven, not melted | broad planes stay legible and flat | finite signed planishing/cross-peen height and normal | no global lumpy noise |
| the surface contains cool and warm dark passages | multiple shades blend inside one part | sixteen-shade palette plus ten broad finite colour fields | no one colour per component and no saturated orange |
| finish direction changes the highlight | local directional response exists only where worked | finite worked passes create normal, mask, direction, roughness change, anisotropy, and rotation | no candy-cane global lines or direction that crosses unrelated components |
| micro-roughness is close-detail evidence | retain bounded high-frequency response | world-metre micro coordinate and camera fade | microdetail may not carry the gameplay read |
| giant hardware was authored at ten-times fabrication scale | tool vocabulary scales with the imagined making process | macro coordinate divides by `sinc_iron_fabrication_scale`; macro displacement multiplies by it | micro and brush spacing may not become ten times larger |
| actual hinge geometry already exists | prove the material on the real consumer | import the approved runtime hinge objects and assign the canonical material | generic sphere/strap cannot be the final acceptance asset |

## Material anatomy

### 1. Construction

The leaf outline, openwork cells, knuckles, barrel clearance, pintle, plate
thickness, and arrises are geometry. The material does not draw apertures,
joint gaps, or silhouette chips.

### 2. Broad compact-scale colour

The quiet ground is a blend of cool blue-black, neutral charcoal, and restrained
warm-black families. It uses at least sixteen palette anchors and ten finite,
overlapping broad fields. The surface must show several shades inside one leaf,
not assign one shade to each disconnected object.

The broad field is intrinsic colour. It excludes AO, photographed shadow,
edge-darkening, corrosion, and a baked light direction.

### 3. Compact scale plate tone

Finite overlapping plate-tone regions interrupt the broad ground. They are
low contrast and dielectric. Their boundaries can affect roughness and
sub-20-micrometre local height, but they may not resemble scattered black
spots, exposed silver islands, or rust flakes.

### 4. Planishing and cross-peen planes

The planishing vocabulary uses a 41.275 x 31.75 mm maximum envelope at
fabrication scale 1. Cross-peen uses 36.5125 x 12.7 mm. Events are finite,
clustered, and separated by long quiet regions. Strength changes slope,
sub-millimetre signed height, and roughness. It does not automatically change
metal identity.

At fabrication scale 10 the footprints and signed macro amplitude become ten
times larger. This is a deliberate giant-house making model. Scale plates,
micro-topography, and worked finishing retain their world-metre response.

### 5. Worked-face brushing

Twelve finite passes form the first clean finish vocabulary inside a separate
2.436 by 0.492 m field. Each owns:

- a centre, length, width, and rotation;
- a 2.4-4.8 mm visible bundle spacing;
- a bounded 0.45-0.85 response;
- a phase distinct from neighboring passes;
- quiet termination instead of covering the entire tile.

The 2.4-4.8 mm rhythm is an authored filtered bundle signal chosen to remain
stable at the declared atlas resolution. The modern 0.30-0.35 mm wire
diameter remains research context; it is not mislabelled as rendered spacing.

Worked response produces:

- R: finite coverage;
- G: signed local angle encoded from -90 to +90 degrees;
- B: protected rest/detail-priority;
- an OpenGL tangent normal;
- a small roughness decrease that reads as changed luster;
- live Principled anisotropy and anisotropic rotation.

It produces no scratches, grooves, exposed metal, or damage height.

### 6. Micro surface

The modern hot-forged Ra proxy is 6.4 micrometres. It is an amplitude guard,
not a procedural identity. The signal is band-limited and subordinate. It is
full until 0.65 m and absent after 4.5 m.

### 7. Optional physical layers

The conductor branch remains valid for future geometry-authored contact polish,
but its default mask is zero. Brown oxidation also remains a default-off
optional physical layer. Neither participates in this candidate's visible
proof.

## Coordinate contract

Every mesh supplies:

- `sinc_iron_u_m`: component-local longitudinal metres;
- `sinc_iron_v_m`: component-local cross-stock metres;
- `sinc_seed`: deterministic atlas row;
- `sinc_iron_edge_mask`: later contact eligibility, unused at default;
- `sinc_iron_fabrication_scale`: 1 for human-size fixtures and 10 for the
  giant openwork hinge;
- `IGGY_IronUV`: face-corner tangent frame aligned to component work direction.

The shader reconstructs three sample vectors:

1. macro vector: U and V divide by fabrication scale before the physical tile;
2. surface vector: raw metre U and V.
3. worked vector: raw metre U and V across a field twice the base length and
   three times its width.

Tool masks, macro normal, and macro height use the fabrication vector. Broad
compact-scale colour, scale response/normal/height, and micro normal/height use
the raw surface vector. Worked normal/response use the larger raw-metre worked
vector. A giant hammer therefore
grows with the imagined maker while oxide plates, colour movement, and finish
retain world-metre material scale, and finish does not repeat in neat
cross-stock rows. Variant selection changes only the atlas row.

## Frequency and response ladder

| Band | Span | Amplitude/response | Distance role |
| --- | --- | --- | --- |
| geometry | 0.05-3.112 m | real silhouette and thickness | all |
| broad colour | 0.12-1.218 m at fabrication scale | sixteen related dark shades | distant/gameplay |
| planishing | max 41.275 x 31.75 mm times fabrication scale | signed 85 micrometres times fabrication scale | gameplay/close |
| cross-peen | max 36.5125 x 12.7 mm times fabrication scale | same bounded macro lane | gameplay/close |
| scale plates | 26-166 mm authored field | 0-20 micrometre positive lip | close/gameplay |
| worked bundles | 2.4-4.8 mm | normal plus max 0.05 roughness shift; no height claim | close |
| micro | 2.5 mm cutoff context | 6.4 micrometre Ra proxy | close, faded |

## Colour order

1. quiet compact-scale palette anchor;
2. ten broad warm/cool finite fields;
3. restrained scale-plate tone;
4. optional brown oxidation, multiplied by zero in this slice;
5. optional contact-exposed conductor, multiplied by zero in this slice.

Hammer and worked-face masks do not create bright colour islands.

## Roughness causality

- broad intact scale: 0.66-0.82;
- finite planishing: up to 0.035 lower;
- cross-peen: up to 0.025 higher;
- scale plates/lips: up to 0.08 higher in combination;
- worked-face luster: up to 0.05 lower inside finite passes;
- micro: small signed modulation;
- no direct height inversion.

## Rejection conditions

Reject the build if it reads as:

- glossy black plastic;
- smooth neutral gray plastic;
- bright silver hammer stamps;
- a uniformly rusty plate;
- one noisy procedural field;
- one colour per object;
- brushed aluminum;
- parallel candy-cane lines;
- micro-speckle at gameplay distance;
- a disconnected swatch that was never assigned to the openwork hinge.
