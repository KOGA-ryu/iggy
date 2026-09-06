# Butterfly-knife manual modeling — video extract for the weapons node lane

Extraction of: **"Modeling a CS:GO Butterfly Knife in Blender"** — Aryan (@Aryan3D, ~149K subs), 17:29, published 2026-07-28 (captured same day at ~715 views), https://www.youtube.com/watch?v=a9w6BuJkAQ4. Source material: full auto-generated English transcript (140 timestamped blocks) plus spot-checked frames at 2:10, 14:57, and 16:52. Auto-captions garble a few product names ("masterass" = masterclass); techniques are unambiguous. The handle section is time-lapsed with the author stating it reuses the blade techniques, so its step detail is thinner than the blade's. Texturing is not covered — the final frame shows a CS:GO-skin material that the video never explains.

**How to read this against [sword_geometry_nodes_handoff_v0_1.md](sword_geometry_nodes_handoff_v0_1.md):** this video is the **direct-geometry side** of that handoff's GN-vs-direct table — a hand-topology practitioner doing subdiv-clean hero modeling of a bladed weapon. It is not a Geometry Nodes video and proposes no automation. Its value to us is threefold: a concrete technique inventory for the direct-mesh escape hatches, hard cost calibration for manual hero work, and independent confirmation of several rules the handoff asserts.

## 1. Workflow as performed (timestamped, paraphrased)

- **0:24–1:06 — References.** Google Images for the subject plus the word "blueprint" to find an ortho background reference; separately one high-resolution beauty shot chosen *because surface shading is visible* (the shading reveals the blade's planes — the author picks references the way our art gate uses grazing light). Beauty shots go into PureRef beside Blender. He deliberately stops collecting after two images — most candidates are near-duplicates and add nothing.
- **1:06–1:19 — Setup.** Background reference image loaded into the viewport and aligned; modeling happens over it.
- **1:19–2:45 — Feature-first start.** He starts with the blade face that carries the carved channel (the CS:GO balisong's groove — our fuller analog), explicitly *because* the geometry must be arranged to support that line before anything else exists. Not outline-first: crease-first.
- **1:27–2:45 — Seeding topology.** Plane, subdivided twice; the two center quads become a circle via LoopTools Circle (the channel's rounded terminus); a vertex rip (V) opens the channel mouth. First attempt to fit the channel curve with LoopTools Curve through placed vertices is **rejected on visual bend quality** — he deletes it and instead collapses a plane to a single vertex and traces the line as a low-poly polyline under a Subdivision Surface modifier, a "French curve" he can shape with few control points.
- **3:05–4:34 — Density reconciliation.** The traced curve is re-spaced (LoopTools Space), subdivided to approach the neighboring boundary's count, then vertices are randomly dissolved and re-spaced until both sides carry ~35 edges. Edge counts are read from the Statistics overlay. Only then are the objects joined, strips filled (F), an edge duplicated and flattened (Shift+D, scale-to-zero on X about the 3D cursor), and the interior closed with **Grid Fill**.
- **4:34–7:41 — Merging feature curves.** The outer silhouette curve and the channel curve must merge into one continuous flow line that runs to the tip. Tools: separate-by-selection so proportional editing (sharp falloff) can push a region without dragging neighbors; LoopTools Relax (linear, 25%, repeated with Shift+R); extruded vertex chains with small bevels to redirect a curve's trajectory into another; split-edges-and-faces to plant a tip vertex; more count-matching (12 vs 9 → add 3 loop cuts) before joining.
- **7:41–8:10 — Big-surface close.** Frame the region, Insert with boundary toggled off (I, then B), verify the frame arithmetic (4 + 10 + 10 + 4 edges), separate the frame, Grid Fill with the corner as active element.
- **8:10–12:03 — Second face and spine.** Same toolkit. Notable: the whole upper curve is dropped in Z with **LoopTools G-Stretch with X and Y locked**, so the plan-view shape is untouched while the profile descends — and later the spine is straightened the same way. Axis-locked straightening is his enforcement mechanism for blade straightness.
- **12:03–12:51 — Blade close-out.** Second edge bridged (Bridge Edge Loops), tip closed by scaling the boundary to zero at the 3D cursor. **The back edge is explicitly not left knife-sharp: he extrudes it down to give the spine a small flat** before aligning it with the rear surface.
- **13:15–13:57 — The ridge and the dent.** The channel region is inset for width, extruded down, and its bottom edge beveled with **two segments**; edge slides place the creases; diagonal triangles are dissolved back to quads. A small dent at the blade base comes from two vertex slides and a J-connect.
- **13:57–15:06 — Flats as time-lapse.** The remaining flat regions and the circular lightening holes (insert → LoopTools Circle → delete face) are time-lapsed; he tells viewers to pause and copy vertex arrangements directly, and points at his free topology guide for the principles.
- **15:06–16:40 — Handles and the confession.** Both skeletonized handles ≈ **1.5 hours** of manual vertex work (he watched half of *Heat* while doing it), asserted to use no new techniques. The pivot screw's star recess was built as all-quad topology **on principle, ~25 minutes for one tiny detail**, while admitting a Boolean modifier would have looked just fine — "we probably could have just used a boolean modifier."
- **16:40–17:29 — Outro.** Final textured render; pointers to his masterclass course, Pro Tips ebook, free topology guide, and Discord (discord.gg/gvSHbfEemp). Channel also runs a "Tips for Modeling with Perfect Topology" series (parts 5–6 current) if we want deeper cuts of the same doctrine.

## 2. Technique inventory (the tooling)

| Technique | Blender mechanism | Where it fits for us |
|---|---|---|
| Blueprint + shading-rich beauty ref | Background image + PureRef | Reference discipline for hero/damage tier; shading-visible refs are the static analog of our grazing-light gate |
| Crease-first topology planning | Start modeling at the feature line | Manual twin of the handoff's feature-aligned rings: creases get topology reserved for them *first* |
| Circle seeding in a grid | LoopTools **Circle** on interior quads | Round terminals of grooves; circular holes |
| Guide-curve tracing | Collapse to vertex → low-poly polyline + Subsurf | Cheap "French curve" for feature lines; candidate scripted utility for direct ops |
| Even spacing | LoopTools **Space** | Post-edit ring redistribution |
| Curve smoothing | LoopTools **Relax** (linear, 25%, repeat) | De-kinking silhouettes without proportional-editing drift |
| Axis-locked straighten/descend | LoopTools **G-Stretch** with axes locked | Manual enforcement of the straightness invariant; useful in damage-tier repair |
| Count-matched boundaries | Statistics overlay + dissolve + re-space | Manual version of what GN gets by construction; our validate scripts assert it automatically |
| Big quad closes | **Grid Fill** with corner as active element | Fast clean caps once opposite counts match |
| Region isolation for falloff edits | Separate-by-selection → proportional edit → rejoin | Keeps sharp/smooth falloff pushes local |
| Flatten/align an edge | Scale-to-zero on one axis about the 3D cursor | Planar end cuts, tip closes |
| Crisp ridge | Inset → extrude → **bevel (2 segments)** → edge slides → dissolve diagonals | The manual channel/fuller; ours is analytic, the damage tier still needs this |
| Curve merging | Extruded vertex chains + mini-bevels redirecting flow | Direct-mesh joinery of feature lines (clip points, ornate spines) |
| Tiny detail pragmatism | Boolean modifier instead of quad purity | His own verdict after 25 min on a screw recess |

The LoopTools verbs (Circle, Space, Relax, Curve, G-Stretch, plus Bridge) ship with Blender as the `mesh_looptools` add-on and are scriptable. When the direct-geometry semantic ops from the GH-007/WPN-001 handoffs get built (AdzePlane, notch, splinter), these are the algorithms to wrap or imitate — bounded, validated versions of exactly the moves this practitioner reaches for by hand.

## 3. Calibration data for the lane split

- Skeletonized handle pair, competent specialist, hand topology: **~1.5 h**.
- One star screw recess at quad purity: **~25 min**, self-described as principle over sense; Boolean alternative: seconds.
- Narrated blade face with channel: **~15 min** of dense, decision-heavy work — reconciling edge counts consumed much of it.

This is the cost curve the WPN-001 node library amortizes: everything he did to get *clean stock* (tapers, flats, ridge creases, matched counts, straight spine) is exactly what the analytic blade field produces for free, per variant, per seed. What his workflow uniquely buys — and what stays manual in our doctrine — is judgment at merges, silhouette taste, and hero details. The video is a good argument that the boundary in our GN-vs-direct table is drawn in the right place.

## 4. Doctrine confirmations and one tension

Independent confirmations from a hand-modeling practitioner who has never seen our docs:

- **Feature-first = feature-aligned rings.** He orders all topology around the carved line before the outline exists; our blade reserves rings for ridge/fuller/bevel/edge stations before deformation. Same principle, opposite tooling.
- **Spines carry a flat.** His back edge gets deliberate thickness; our `Edge Land` is clamped nonzero. Zero-thickness edges are wrong in both worlds.
- **Straightness is enforced, not hoped for.** His axis-locked G-Stretch is a manual invariant pass; our checklist asserts centerline deviation ≈ 0.
- **Count matching before joining.** His dissolve-and-respace loop is the manual tax for what GN gives by construction; our validate scripts make it an assertion rather than a chore.
- **Booleans for tiny details are fine.** His screw confession supports the handoff's stance: sparse, small, well-separated Boolean cuts are acceptable; quad purity there is ideology.

The tension worth keeping in view: his all-quad discipline exists **for subdivision surfaces** — hero/source meshes that must survive subsurf and close-up shading. Our export path is triangulated game meshes with baked materials, where quad purity below the silhouette is invisible. So adopt his crease-first planning and his cost data; do not adopt quad-purity-everywhere as a gate for runtime assets. It is a source-mesh standard, and only for the direct-mesh hero tier.

## 5. Balisong decomposition (future kit note)

From the blueprint frame and final render, a butterfly-knife family for the node library would decompose as: blade (single-edge section, carved channel, pivot hole, thickened spine flat, choil/dent at base), **two** skeletonized channel-bar handles (circular lightening holes, pivot bosses at one end, tang channel between bars), latch, and machine screws with star recesses. The blade reuses `SINC_GN_SwordBlade`'s single-edge v2 field family nearly unchanged; the handles are extruded-profile bars with hole arrays (clean GN); the pivot screws want a `SINC_GN_MachineScrew` group distinct from the forged-rivet fasteners — machined, not forged, so a different mask/shader contract. Not scheduled; recorded so the decomposition isn't re-derived later.

## 6. Channel references

From the video description: Blender Masterclass course, Blender Pro Tips ebook, free topology guide (all shorturl links in the description of the video URL above), Discord discord.gg/gvSHbfEemp, contact askaryan3d@gmail.com. The free topology guide and the "Perfect Topology" series are the likely next extractions if we want the principles behind §2 rather than the moves.
