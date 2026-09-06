# Wood Plank V2 Material Book

This book follows the floor from a hand-authored layout to a complete material
response. It is not a list of texture buffers. Each chapter isolates one
decision, shows the fields that carry it, and explains how that decision
survives into the final image. The selected recipe is variation
0 of `aged_oak_champion_v1` across a physical tile of
1.60 metres.

## Chapter 1 — Authored Layout

The floor begins as joinery, not noise. 9 boards cross
the tile and are divided into 18 cyclic pieces. Their
widths, lengths, and offsets were authored to create a cadence of long rests
and short interruptions. The seam wraps, but the eye should read a laid floor
rather than a diagram designed merely to pass a seam test.

Each segment owns a local coordinate frame. One axis travels across the growth
rings and the other travels along the fibres. Every later feature inherits
that frame. Grain follows the board after its width changes; a knot belongs to
its piece; an end accent remains near a saw joint. This is the quiet structural
promise on which all decorative richness depends.

Variation changes the performance without rewriting the score. Width, length,
and stagger move inside bounded ranges, preserving the recognizable family
while preventing a single arrangement from becoming wallpaper.

## Chapter 2 — Boards and Joints

The spaces between boards are physical recesses. Long edges and end joints
are measured independently, then merged through the nearest distance. That
single construction yields a 0.0012 m half-gap
and a 0.0018 m shoulder. At a corner, color, height,
normal, and occlusion therefore agree about where the wood ends.

A painted black line can suggest a joint from one camera, but it has no answer
for grazing light. Here the plank rises from the gap, crosses a rounded bevel,
and settles into its face. Individual segments receive a slight raise so the
floor catches light with the irregularity of boards planed, installed, and
worn by different hands.

The profile panels are deliberately unromantic evidence. One crosses the
boards; the other travels along them. They reveal whether the mask, distance,
and height describe the same cut.

## Chapter 3 — Grain Anatomy

Wood grain is a hierarchy of motion. Broad bands carry the slow record of
growth. Fine fibres interrupt those bands with a quicker rhythm. If both are
collapsed into one frequency, the surface becomes striped plastic; if they are
unrelated, it becomes television static. The material gives each family its
own warp while keeping both subordinate to the local plank frame.

The broad field owns the recognizable sweep. The fine field supplies broken
edges and small catches. Their combined mask removes up to
0.00012 m from height and gently darkens pigment.
Relief and color share a cause, but their strength differs: the eye can feel a
fibre in light without seeing every fibre painted as a black line.

Near a knot, the broad phase bends. That deformation is important because a
knot pasted over straight grain reads as a decal. The wood must remember the
branch before the artist adds the dark mark.

## Chapter 4 — Character Marks

Character is assigned sparingly at the layout level:
4 knot segments,
14 clear segments. Empty boards are part of the
composition. Without quiet pieces, every special mark competes for attention
and the floor loses scale.

Damage is authored as 1 knot check,
2 end checks,
1 edge splinter, and
3 use scratches. Each record names a source,
segment, tapered path, physical width, depth, and optional raised lip.

The deepest damage removes 0.00062 m. Knot checks
begin inside a named branch, end checks begin at a segment end, and the
splinter begins at a long edge. Character reads as history held inside the
plank rather than trenches stamped on top.

## Chapter 5 — Height Stack

Height is built as an accountable stack. The wood begins
0.0042 m above the gap. Per-segment raise changes
the plane. A broad undulation keeps the face from becoming mathematically
flat; a much smaller surface field removes sterile perfection. Grain then
scores the face, and character marks make their deeper, rarer cuts.

Every amount is expressed in metres. A 512-pixel study and a 2048-pixel
delivery can therefore describe the same physical floor instead of changing
the apparent depth whenever resolution changes.

The normal is derived only after the complete height has been assembled.
There is no independent normal noise with a different opinion about the
surface. This is both a technical contract and an artistic discipline: every
bend in light must be able to name the form that caused it.

## Chapter 6 — Color Script

Color begins with a weighted family of browns assigned per segment. That
choice gives neighbouring boards individual pigment before any grain,
lighting, or damage appears. A quiet warm wash moves through the macro field,
connecting the pieces without erasing their identities.

The passes then arrive in order. Grain receives translucent ink. Knots and
checks receive stronger ink because they expose denser or deeper material.
End accents remain restrained, suggesting contact without outlining every
joint like a comic panel border. The sequence matters because it preserves
control: an artist can soften the grain without washing out the knots.

No directional shadow is baked into base color. The map describes wood,
pigment, and age, leaving scene light free to become graphic. This restraint
is essential for a cel-shaded treatment, where broad illumination ramps need
a surface that enriches the light rather than contradicting it.

## Chapter 7 — Material Response

The completed height is searched for local cavities. Those recesses inform
ambient occlusion, darkening gaps and cuts as a response channel while keeping
the base-color pigment clean. The tangent-space normal carries the same form
into direct light.

Roughness begins as a per-segment property, because boards do not all accept
wear and finish identically. Surface variation, grain, knots, checks,
splinters, and scratches each make a named change. The result is broken
reflection with a readable cause, not arbitrary noise sprinkled into the green
channel.

The packed ORM map keeps ambient occlusion in red, roughness in green, and
metallic at zero in blue. Wood does not become more interesting by pretending
to be metal. Its richness comes from the agreement of form, fibre, pigment,
and restrained reflection.

## Chapter 8 — Scale, Seams, and Final Read

The tile spans 1.60 m, and each pixel represents
0.000781 m. Periodic fields protect the mathematical
border. Gold seam frames make that contract visible, while two-by-two repeats
ask a different question: does a memorable knot or board rhythm announce the
tile from across the room?

The variation family is the beginning of a placement system. Compatible
layouts can be selected, rotated, or distributed so repetition becomes a
designed rhythm rather than a hidden flaw. Even so, the chosen tile must stand
on its own; variation cannot rescue weak joinery or uncontrolled landmarks.

The final close study gathers every promise. A joint should feel cut into the
floor. Grain should bend with the board. A knot should feel grown rather than
pasted. Pigment should leave enough silence for bold cel light. At distance,
the boards form a readable cadence; up close, the surface rewards attention
without dissolving into static.
