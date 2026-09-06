# Rough-hewn structural timber: research translated into Blender intent

This document is the causal specification for World Asset 001. It separates
what a real timber is, what hand tools do to it, what Blender must represent as
geometry, and what later belongs to a material. It is intentionally stricter
than a visual mood board.

## 1. The object before the marks

A hewn beam begins as a log. Its faces are not four independent decorative
panels. Each face is a removal from the same growth volume, so every large
decision must remain mutually consistent:

- the longitudinal axis is the tree axis;
- knots are branch intersections and therefore affect a surrounding volume;
- end checks begin at exposed ends and travel lengthwise;
- bow, crook, and twist act on the whole piece;
- broad-axe marks remove material toward a snapped target plane;
- the surviving arrises are the boundaries between four separate hewing
  operations.

This is why a displaced cube with generic noise fails. It has no shared causal
volume. It cannot explain why a mark points in a direction, why a knot owns a
grain influence field, or why a check begins at the end.

The master is 4.2 m long with a 300 by 340 mm starting section and a 288 by
329 mm ending section. Historic hewn-building references commonly discuss
finished 8 by 8 and 10 by 10 inch timbers, approximately 203 and 254 mm
square. The giant-house kit needs larger stock, so the selected section is a
measured extrapolation rather than a claim that every historic timber was this
size.

## 2. Hewing sequence, not surface noise

The researched hand-hewing sequence is:

1. Square lines are laid out on the log ends and connected along its length.
2. A scoring axe makes repeated notches down to, but not through, the intended
   face.
3. Waste between the score notches is removed.
4. A broad axe joggles or flattens the remaining surface toward the line.
5. An adze may refine a face, but it is not a license to cover the timber with
   uniform scallops.

Preserved broad-axe surfaces read as large flattened removals. Many carry a
diagonal or transverse terminal line where the cutting edge stopped. The marks
have a long entry, a flatter central field, and a comparatively sharp exit.
They are not symmetrical dents.

The profile records 57 authored observations split across four faces. Each
observation owns:

- longitudinal centre;
- longitudinal reach;
- cross-face centre;
- cross-face reach;
- observed removal depth;
- blade-profile angle.

Those observations are deliberately **not** sampled as source-mesh
displacement. Clay comparisons demonstrated two distinct failures. A sampled
depth field made soft isolated dents; closed Boolean wedges made repeated
parallelogram stamps. Neither matched the written craft evidence. The
observations remain measured input for the later fine-normal striation and
tool-signature lane, where diagonal slicing traces and the blade profile can be
drawn without pretending that every trace is a separate cavity.

The front and back faces emphasize diagonal broad-axe stops in opposing
directions. The top is quieter and represents broad-axe work followed by light
adze refinement. The bottom carries stronger scoring remnants. The four faces
are intentionally related but not mirrored.

The accepted geometry is the larger dressed surface. Each face passes through
13 explicitly measured target-plane anchors. Their irregular 0.25–0.45 m
spacing records the scoring/joggling sequence. Linear interpolation between
anchors creates broad 1–3 mm plane offsets. Each face also owns a different
85–130 mm cross-face sweep, so pass transitions travel diagonally instead of
forming vertical panel seams. Front and back sweep in opposite directions.

Every semantic region is fitted to a literal plane in object space. This is
the deterministic counterpart of Scrape or Multiplane Scrape followed by
Flatten in Blender Sculpt Mode. The fit retains longitudinal and cross-face
coordinates and adjusts only the face-normal coordinate. It is 94 percent
applied and limited to 3.2 mm of correction so it cannot erase measured taper,
bow, crook, or anatomy.

The written Blender sculpt guidance places sharp Crease or Draw Sharp accents
after the broad planes exist. This build follows that boundary more strictly:
blade-stop linework is material-only until a hand-authored close-up proves
which stops deserve geometry.

### Why random adze marks are rejected

Random adze coverage produces the familiar early-2000s educational-game read:
every patch is equally important, every mark has the same cause, and the beam
looks embossed rather than hewn. It also destroys the broad quiet fields that
stylized lighting needs. Random adze noise is not used anywhere in the geometry
or material contract.

## 3. Primary form measurements

The large-form deviations are small in metric terms but important under a
straight highlight:

| Form | Maximum | Placement rule |
| --- | ---: | --- |
| taper | 12 mm width, 11 mm height | linear end to end |
| vertical bow | 12 mm | zero at ends, broad middle envelope |
| lateral crook | 7 mm | zero at ends, asymmetric middle envelope |
| twist | 1.15 degrees | distributed along the full length |
| arris/chamfer | 6–19 mm | four separate slow variations |
| broad-axe depth | about 2–8 mm | authored by face and mark |

The deformation returns to the end centres because the ends are future joinery
interfaces. A beam that wanders at its attachment plane becomes difficult to
reuse and makes later mortises, tenons, straps, and rope wrapping unstable.

Three protected zones reduce hewing depth to 24 percent:

- 360 mm at the left end;
- 620 mm around the central bearing;
- 370 mm at the right end.

These zones are not empty placeholders. They are quieter, structurally
plausible surfaces where a future joint can be cut without crossing a hero axe
facet.

## 4. Knots are local anatomy

A knot is not a dark circular decal. It is the remaining branch base inside the
trunk. The USDA Wood Handbook describes knots as a source of distorted cross
grain whose effect depends on size, type, and location.

This master uses two restrained examples:

- a 52 by 36 mm live-knot socket on the front face at X = -0.72 m;
- a 34 by 25 mm pin-knot socket on the top face at X = 1.06 m.

The geometry owns the recessed socket edge and a separate editable knot body.
The future wood material owns ring colour, pore density, and grain deflection
within the stored 230 and 140 mm influence radii. Making the entire grain
deflection geometry would be wasteful and difficult to resize; making the
socket only a texture would lose the contact shadow that identifies a branch
intersection.

The socket outlines use two low-amplitude harmonics rather than a perfect
ellipse. The result remains one deliberate anatomical feature, not a noisy
circle.

## 5. End checks and splits

Wood-science references distinguish checks from arbitrary surface scratches.
Checks are seasoning separations that run across or through annual rings,
generally along the length of the piece. Their most legible origin is exposed
end grain. A through split is a more severe condition.

The clean master therefore uses five checks:

- three on the left end;
- two on the right end;
- 3–6.5 mm mouth widths;
- 110–240 mm penetration depths;
- different radial angles and reaches.

Each cutter is a closed tapered prism. It begins slightly outside the end
plane, opens to the specified mouth, follows a radial path in the end face, and
narrows to roughly ten percent of its mouth width as it travels inward. The
checks do not appear halfway down the beam, do not run at decorative random,
and do not become black painted lines.

This build treats the checks as restrained intrinsic anatomy, not as the future
damage library. Broken corners, impact scars, repair sockets, rot, scorch, and
scene-specific wear remain outside this asset.

## 6. Edge hierarchy

A useful timber does not have four identical rounded edges. Every arris tells
which faces survived, where the log was close to the target square, and where
handling removed a little more stock.

The cross-section therefore has four independently varying chamfers. Their
slow change over length sits between 6 and 19 mm. Two small geometry losses are
allowed:

- one 61 mm chip at the left-front-top arris;
- one 44 mm chip at the right-back-bottom arris.

These are local interruptions of an otherwise readable edge. They do not
establish a universal damage frequency.

## 7. Blender topology method

### 7.1 Deterministic quad strip

The core mesh has 145 longitudinal sections. Every section has:

- 16 interior samples for each of the four hewn faces;
- one connecting sample at each arris;
- 36 total perimeter samples.

Neighbouring rings form quads. Each end is closed with a triangle fan. This
gives the authored broad-axe events enough spatial support while preserving
continuous longitudinal topology and stable semantic face ownership.

This method is the scripted equivalent of adding deliberate loop cuts and
using connected proportional editing for broad deformation. Blender's
Proportional Editing documentation explains that connected falloff follows
topological relationships; here the same idea is encoded explicitly in the
bow, crook, taper, twist, and face-event functions so the result is repeatable.

### 7.2 Why sculpt-remesh is not the source of truth

Sculpting can be an excellent manual discovery tool. A voxel or uniform remesh
is not the best canonical source for this reusable system because it rebuilds
topology and makes the longitudinal, face, end-distance, joinery, and UV lanes
harder to preserve. The production asset needs to survive resizing and later
Geometry Nodes transforms. Deterministic topology wins that trade.

The correct hand-authoring loop remains available: adjust the measured JSON
events or edit a duplicate in Blender, study the change, then promote the
accepted measurement back into the profile. The script is not a substitute for
judgment; it is the repeatable record of that judgment.

### 7.3 Boolean anatomy

Blender's Boolean modifier documentation identifies Exact as the slower solver
that handles overlapping or coplanar geometry more reliably. It also warns that
manifold input is the guaranteed case. Every cutter in this asset is therefore
a closed manifold volume and every modifier uses `DIFFERENCE` with `EXACT`.

The stack order is causal:

1. left and right seasoning checks;
2. knot sockets;
3. restrained edge losses;
4. selective bevel;
5. weighted normals.

The cutters remain editable in `IGGY_WA001_ConstructionCutters`; they are hidden
from final renders but are not destructively applied. The hidden
`IGGY_WA001_RoughHewnTimberBeam_CleanSource` has no modifiers and preserves the
pre-anatomy source.

### 7.4 Bevel and normals

The final bevel is only 0.45 mm with two segments and an angle limit. Its job
is to prevent razor aliasing at Boolean anatomy. The 6–19 mm arris hierarchy is
already authored in the source cross-sections; a 2.6 mm modifier bevel was
rejected after it outlined shallow tool experiments like decorative panels.

Blender's Bevel documentation states that Harden Normals keeps surrounding
faces flat while the bevel shades into them. It explicitly describes following
the bevel with a Weighted Normal modifier using Face Influence. The master uses
that pairing and gives broad face area and angle a high weight. The result
preserves the planar hierarchy needed by a painterly shader.

### 7.5 UV and geometry data

The canonical material coordinate is longitudinal distance in metres, not a
0–1 square stretched over whatever length the object happens to be.

The clean mesh writes:

- `sinc_timber_u_m`: 0–4.2 m longitudinal coordinate;
- `sinc_timber_face_id`: front, back, top, bottom, left end, right end, or
  chamfer;
- `sinc_joinery_reserved`: binary protected-zone ownership;
- `sinc_hewing_depth_m`: measured authored removal;
- `sinc_end_distance_m`: distance to the closest cut end;
- `IGGY_TimberUV`: face-corner UVs using physical longitudinal distance.

The end faces have separate identity values. This is mandatory: end grain
cannot be inferred safely from a triangular interpolation mask after a Boolean
cut.

Blender's UV documentation recommends finalizing most geometry before unwrap
because newly added faces can need additional mapping. Here the semantic source
UV is written before non-destructive anatomy, and later export/bake tooling
must explicitly inspect new Boolean faces. The face identity attribute is the
authoritative routing lane; the UV is a production convenience, not a hidden
substitute for face ownership.

## 8. Material boundary

Neutral clay is not the final art direction. It is the honesty test.

After geometry approval, the material may add:

- approximately twenty related oak colour shades within each face;
- broad warm and cool longitudinal passages;
- latewood and earlywood rhythm;
- white-oak pore structure;
- knot-driven grain deflection;
- independent end-grain rings and pores;
- fine sharp tool-edge linework;
- fine fibre normal;
- roughness authored independently from colour and height.

The material may not add:

- new arbitrary dents;
- random damage;
- baked directional lighting;
- one flat hardwood-floor colour;
- square tiling that ignores the 4.2 m axis;
- black cracks;
- photographic noise everywhere.

The existing structural-oak texture family will be evaluated only after the
clay proofs pass. Its job will be to reveal the geometry, not rescue it.

## 9. Review package and acceptance questions

The build renders front, back, top, bottom, both ends, two perspectives,
wireframe, silhouette, grazing light, and an end-check close-up.

The asset passes geometry review only if:

1. the silhouette reads as a subtly tapered whole timber;
2. all four faces differ without looking independently randomized;
3. broad-axe work reads first as a dressed, gently undulating face;
4. the quiet fields remain larger than the marks;
5. the knots read as branch anatomy before colour;
6. the checks visibly originate from the ends and narrow inward;
7. the arrises keep long surviving runs;
8. joinery zones remain usable;
9. the beam is credible under neutral clay;
10. the evaluated modifier result is closed and manifold.

## 10. Sources and exact use

- National Park Service, Fort Scott, *Broad Axe and Adze*: tool identity and
  the distinction between a broad axe used for hewing and an adze used for
  trimming or smoothing.
- Olde Wood Limited, *How They Hewed It*: end layout, scoring at roughly one-
  or two-foot intervals, waste removal, and broad-axe joggling sequence.
- Handmade Houses, *The Broad Axe vs. the Adze*: visual distinction between
  preserved broad flat hewn faces and modern random adze scalloping.
- Gränsfors Bruk, *Broad Axes – How to Choose the Right Broad Axe*: the flat
  side produces a smoother dressed surface while the ground side can leave a
  wave-pattern finish.
- *Hand Hewn: The Traditions, Tools, and Enduring Beauty of Timber Framing*,
  “Axe Clues”: vertical scoring marks, diagonal broad-axe slicing striations,
  and the terminal profile of the cutting edge are distinct evidence.
- 80 Level, *Using Blender & Substance 3D Designer to Make a Stylized Diorama
  in Unreal Engine 5*: loop-cut silhouette authorship, two-segment Angle
  Bevel, Harden Normals, straight UVs, and consistent texel density.
- 80 Level, *Making a Stylized Woodcutter's Hut in Blender, ZBrush & Unreal
  Engine 5*: preserve flat surfaces, avoid universal surface noise, keep the
  workflow non-destructive, and test assets early in engine lighting.
- USDA Forest Products Laboratory, *Wood Handbook, General Technical Report
  FPL-GTR-190*: knots, distorted cross grain, checks, splits, and seasoning
  behavior.
- USDA Forest Service, *Timber Bridges: Design, Construction, Inspection, and
  Maintenance*: unequal shrinkage as a source of bow, twist, crook, and cup;
  checks as lengthwise separations across annual rings.
- Blender Manual, *Boolean Modifier*: manifold input and Exact solver behavior.
- Blender Manual, *Bevel Modifier*: Harden Normals, Face Strength, and the
  Weighted Normal follow-up.
- Blender Manual, *Weighted Normal Modifier*: face-area/angle weighting, Keep
  Sharp, and Face Influence.
- Blender Manual, *Proportional Editing*: connected topological falloff as the
  manual analogue for broad form editing.
- Blender Manual, *UV Unwrapping Introduction*: face-corner UV ownership and
  the need to revisit mapping after geometry changes.
