"""Refined furniture-component taxonomy v2.

Broad v1 categories split into families that could HONESTLY share one parameterized construction
recipe. Splits are derived from the v1 BOM evidence (source_term, variation_axis, evidence class),
not from a supplied example list.

REUSE CLASSES, applied strictly:
  exact_donor            compatible silhouette, section, topology, joints, interfaces, editability
  parameterized_recipe   controlled change generated from the SAME construction logic
  shared_profile         shares only the authored path/profile vocabulary
  assembly_grammar       shares component RELATIONSHIPS, not necessarily geometry
  bespoke                cannot honestly be produced by an existing family

EVIDENCE WEIGHTS for production priority: VISIBLE 1.0, PARTIALLY VISIBLE 0.5,
INFERRED FROM TYPE 0.0, NOT DETERMINABLE 0.0. Inferred rows are PRESERVED as research notes but
contribute ZERO construction authority.

SINC DONORS: every entry is AUDIT_REQUIRED. Only ONE asset in the library is a geometry donor at
all - rough_hewn_timber_beam_v1. structural_oak_joinery_v1, structural_oak_door_v1 and
forged_iron_v1 are MATERIAL assets; they donate surface, not construction, and naming them as
component donors would be exactly the name-resemblance error the brief forbids.
"""

FAMILIES = [
 # ---------------------------------------------------------------- FOOT
 dict(parent="foot", name="foot.turned_separate",
   members=["FC-01-1999_488","FC-02-67_230","FC-05-69_146_3","FC-06-69_146_2","FC-08-2019_59",
            "FC-10-1984_161","FC-11-10_125_1","FC-09-69_146_1"],
   inferred_members=["FC-09-1954_151"],
   invariant="A lathe-turned solid of revolution, authored as a stacked profile (pad / ball or bun / "
             "neck / collar) and joined to the leg or plinth above by a dowel or by being turned in "
             "one with the leg blank. Silhouette is fully described by one revolved curve.",
   variation="ball vs bun vs onion vs trumpet vs toupie silhouette; overall height; max diameter; "
             "ring count and pitch in the collar; pad/disc diameter and thickness; dowel diameter",
   joints="dowel into plinth or leg (1984.161 explicit); or turned integral with the leg blank "
          "(1954.151, unresolved at web resolution); castor socket bored in the pad (69.146.1)",
   excluded="FC-08-69_140ab and FC-10-1971_281 - their 'foot' is the stile running past the lower "
            "rail to the floor, NOT a turned solid. FC-04-1972_47 and FC-03-30_120_5 - carved "
            "figural. FC-12-2002_298 - spun metal.",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - no turned-solid donor exists in the library; rough_hewn_timber_beam_v1 "
         "is hewn, not turned, and shares no lathe logic",
   unlocks="chair, stool, chest, bed, table, cabinet - the widest-reaching single recipe in the corpus",
   blockers="No underside view on any member; dowel vs integral turning unresolved on 1954.151 "
            "(recorded INFERRED and therefore zero production weight)"),

 dict(parent="foot", name="foot.integral_stile_termination",
   members=["FC-08-69_140ab","FC-10-1971_281","FC-03-2011_3","FC-04-1969_262"],
   inferred_members=[],
   invariant="NOT a separate part. The vertical member (stile, post or leg) continues past the "
             "lowest rail and terminates on the floor. Authoring this as a foot at all is the error; "
             "it is a termination treatment on the post recipe.",
   variation="length below the lowest rail; toe radius or wear rounding; splay/cant angle; optional "
             "slight pad flare (1969.262)",
   joints="none - continuous stock. Interface is the lower rail mortise above it.",
   excluded="every turned-foot member; attaching a turned foot here would require cutting the stile, "
            "changing its joint schedule",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - candidate rough_hewn_timber_beam_v1 for the continuous-stock logic only; "
         "silhouette, section and interfaces NOT compared",
   unlocks="wardrobe, chest, settee, side table",
   blockers="2011.3 front feet described as integral but the seam is not resolvable in published views"),

 dict(parent="foot", name="foot.carved_figural",
   members=["FC-03-30_120_5","FC-04-1972_47"],
   inferred_members=[],
   invariant="A sculpted animal terminal (claw-and-ball, paw) carved in the solid at the leg foot. "
             "Not generable from a revolved profile.",
   variation="scale; toe/talon count; disc pad diameter under a paw; forward rake",
   joints="carved integral with the leg blank",
   excluded="all turned and all integral-termination members",
   reuse="bespoke",
   donor="AUDIT_REQUIRED - none",
   unlocks="settee, side table (2 of 18 objects)",
   blockers="1972.47 notes the disc pad IS separable and reusable alone - a possible sub-part rescue"),

 dict(parent="foot", name="foot.block",
   members=["FC-07-1999_79"],
   inferred_members=[],
   invariant="A short rectangular block below a cuff at the bottom of a tapered leg. Prismatic, not revolved.",
   variation="block height; cuff reed count; taper match to the leg above",
   joints="continuous with the leg; cuff is an applied moulding run",
   excluded="turned family (different generative primitive)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="sideboard",
   blockers="single-member family - the split may be an artefact of one object"),

 dict(parent="foot", name="foot.spun_metal_base",
   members=["FC-12-2002_298"],
   inferred_members=[],
   invariant="A spun gilt-brass ring/torus seated on a stone plinth. Metal fabrication, not joinery.",
   variation="lathe profile of the spreading torus; flare ratio; lappet count; bead pitch",
   joints="sits on the plinth as a separate spun element",
   excluded="all wood families - different material AND different process",
   reuse="bespoke",
   donor="AUDIT_REQUIRED - forged_iron_v1 is a MATERIAL, not a spun-metal geometry donor",
   unlocks="console/pedestal",
   blockers="individual ring boundaries not countable in published views"),

 # ---------------------------------------------------------------- POST
 dict(parent="post", name="post.case_corner_structural",
   members=["FC-06-69_146_2","FC-08-69_140ab","FC-08-2019_59","FC-05-69_146_3"],
   inferred_members=[],
   invariant="Square-section vertical carrying the case; mortised on two adjacent faces to receive "
             "rails; runs full height corner to corner; face may be reeded, fluted or left plain.",
   variation="height; square section width; flute/reed count on exposed faces; roundel-block count "
             "interrupting the reeding; whether the post projects above the cornice",
   joints="mortise-and-tenon to rails on two faces; groove for panels where frame-and-panel",
   excluded="post.panel_muntin (non-structural, no rail mortises on two faces); post.bed_stacked_block "
            "(a stacked assembly, not one stick)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - candidate rough_hewn_timber_beam_v1 for square-section stock; section, "
         "joints and editability NOT compared",
   unlocks="chest of drawers, wardrobe, desk, cabinet",
   blockers="rear faces unphotographed on 69.146.2 and 2019.59"),

 dict(parent="post", name="post.panel_muntin",
   members=["FC-10-1971_281","FC-10-1984_161","FC-03-2011_3"],
   inferred_members=[],
   invariant="Intermediate vertical dividing a framed opening into bays. Grooved on BOTH long edges "
             "for panels. Carries no case load; its count is the design parameter.",
   variation="count (0 muntins = single-panel front, 2 = three bays, 3 = four bays); width; section; "
             "whether it carries applied spindles or ornament",
   joints="stub tenon into upper and lower rails; groove both edges",
   excluded="post.case_corner_structural - a corner post is grooved/mortised on adjacent faces, a "
            "muntin on opposite faces. Different joint schedule, not a size change.",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="chest, wardrobe, settee back",
   blockers="none material"),

 dict(parent="post", name="post.bed_stacked_block",
   members=["FC-09-69_146_1","FC-09-1954_151"],
   inferred_members=[],
   invariant="A tall post built as an ORDERED STACK of discrete blocks and turnings (plinth block, "
             "carved floral block, stepped ring, bead-and-reel band, inlaid rosette square, shaft). "
             "The stack ORDER is the recipe; the segments are separate authored parts.",
   variation="stack order and segment count; total height (1954.151 proves head and foot posts are "
             "the same recipe differing ONLY in height); face width; rosette-cell placement",
   joints="segments stacked and presumably pinned or threaded; bed-bolt interface to the side rails",
   excluded="post.case_corner_structural - that is one stick with applied treatment; this is an "
            "assembly of separately authored solids",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - none",
   unlocks="bedstead (both candidates)",
   blockers="internal fixing between segments never visible; bed-bolt detail unresolved"),

 dict(parent="post", name="post.turned_support",
   members=["FC-11-10_125_1","FC-12-2002_298"],
   inferred_members=[],
   invariant="A free-standing turned upright whose whole silhouette is one revolved profile, carrying "
             "a rail or slab above.",
   variation="diameter relative to neighbouring legs; count along a side; presence at all (a 6-leg "
             "table drops the medial posts); segment order for the pedestal shaft",
   joints="pivot bearing (gate-leg medial post); stacked on plinth (pedestal)",
   excluded="post.case_corner_structural (prismatic)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="long table, console/pedestal",
   blockers="pedestal internal tie rod ASSERTED not seen - recorded INFERRED, zero weight"),

 dict(parent="post", name="post.board_standard",
   members=["FC-04-1972_47"],
   inferred_members=[],
   invariant="A wide flat splayed board acting as a trestle end support, with applied carved layers "
             "that can be removed to leave a plain standard.",
   variation="height; splay angle; board width; taper; applied-ornament on/off",
   joints="housed to the stretcher; carries the frieze carcase above",
   excluded="all square-section and all turned posts",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="side table / library table",
   blockers="single-member family"),

 dict(parent="post", name="post.arm_support",
   members=["FC-03-30_120_5","FC-01-1999_488","FC-01-2012_216"],
   inferred_members=[],
   invariant="A short upright rising off the seat rail to carry an arm rail. Two sub-shapes co-occur "
             "in one object (30.120.59: a tapered front cone-stump AND a wide flat rear post), so the "
             "family is defined by ROLE plus a shape parameter, not by one silhouette.",
   variation="front-vs-rear shape selector; height; taper; facet count; whether it is continuous with "
             "the back frame above (2012.216) or terminates at the arm rail",
   joints="tenon into seat rail below, into arm rail above",
   excluded="post.case_corner_structural",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, settee",
   blockers="2012.216's rear support is explicitly NOT continuous with the back stile - a divergence "
            "from the baseline that must not be smoothed over"),

 # ---------------------------------------------------------------- RAIL
 dict(parent="rail", name="rail.seat_frame",
   members=["FC-01-1999_488","FC-03-2011_3","FC-03-30_120_5"],
   inferred_members=["FC-01-2012_216"],
   invariant="Four rails tenoned into legs/posts forming a CLOSED rectangle that carries upholstery. "
             "Front and side rails may be show faces; the rear rail is routinely secondary wood.",
   variation="length and depth per side; show vs secondary finish per rail (2011.3 proves the rear "
             "rail differs from the front on the SAME object); whether the lower edge is straight or "
             "cut to an apron profile; peg count",
   joints="mortise-and-tenon, pinned with round pegs (30.120.59 visible); corner glue blocks inside",
   excluded="rail.drawer_divider (spans a case, not a seat); rail.frame_and_panel (grooved for panels)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, settee, stool",
   blockers="FC-01-1999_488 rear seat rail is NOT DETERMINABLE - no rear or underside view exists. "
            "FC-01-2012_216 seat rails are INFERRED FROM TYPE and carry zero production weight."),

 dict(parent="rail", name="rail.drawer_divider",
   members=["FC-05-69_146_3","FC-06-69_146_2","FC-10-1984_161","FC-07-1999_79"],
   inferred_members=[],
   invariant="A horizontal blade spanning the case front between corner posts or partitions, "
             "separating drawer openings. Front edge carries a reed or moulding; count drives drawer count.",
   variation="count (drives drawer count); length; height; front-edge profile; whether grooved for "
             "bottom boards or mortised for a lock",
   joints="tenoned into corner posts and interior partitions",
   excluded="rail.seat_frame; rail.frame_and_panel",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="chest of drawers, desk, sideboard, chest",
   blockers="runner/guide/stop geometry behind the blade never visible on any member"),

 dict(parent="rail", name="rail.frame_and_panel",
   members=["FC-08-2019_59","FC-10-1971_281","FC-08-69_140ab"],
   inferred_members=[],
   invariant="Upper and lower rails GROOVED on the inner edge to receive a floating panel, tenoned "
             "to stiles and muntins. The groove is the defining feature.",
   variation="length; width; how many panel openings the rail spans; groove depth",
   joints="mortise-and-tenon to stiles, pegged; groove for panel",
   excluded="rail.seat_frame (no groove); rail.drawer_divider (no groove)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - structural_oak_door_v1 is a MATERIAL asset, not a frame-and-panel "
         "geometry donor; do not treat the name as evidence",
   unlocks="wardrobe, chest, cabinet, desk",
   blockers="groove depth not directly measurable in any published view"),

 dict(parent="rail", name="rail.swept_show_wood_arm",
   members=["FC-01-2012_216","FC-03-30_120_5","FC-03-2011_3","FC-01-1999_488"],
   inferred_members=[],
   invariant="A single continuous compound-curved member forming the arm. Sawn from a board or bent; "
             "its silhouette is a 3D curve, not a straight extrusion.",
   variation="sweep curve; whether it terminates in a scroll, a ram's head or an arcade; arch count "
             "in lockstep with spindle count (1999.488); section (round reeded rod vs flat board)",
   joints="into the back frame at one end, onto the arm post at the other",
   excluded="all straight rail families",
   reuse="shared_profile",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, settee",
   blockers="none material - well photographed across four members"),

 dict(parent="rail", name="rail.bed_side",
   members=["FC-09-1954_151","FC-09-69_146_1"],
   inferred_members=[],
   invariant="A long rail spanning head to foot post, thick, sawn from a wide board; may be bowed in "
             "plan and serpentine in elevation. Structural core is bare wood with ornament applied.",
   variation="length (227 cm on 69.146.1); height; bow amount in plan; count of moulding members on "
             "top and bottom edges; ornament applied vs plain",
   joints="bed-bolt or hook into the posts (never visible in any view)",
   excluded="rail.seat_frame (short, closed rectangle)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="bedstead",
   blockers="the post-to-rail fixing is the single most important unresolved joint in the corpus"),

 dict(parent="rail", name="rail.gallery_cap",
   members=["FC-09-69_146_1"],
   inferred_members=[],
   invariant="Small-section rail mortised at a regular pitch to receive a spindle gallery above it, "
             "or capping that gallery.",
   variation="length; mortise pitch for the spindles; whether the face carries a bead run",
   joints="mortised for spindles; tenoned to posts",
   excluded="all load-bearing rail families",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="bedstead; would extend to settee and cabinet galleries",
   blockers="single-object family (three rails on one bed)"),

 dict(parent="rail", name="rail.moving_fly",
   members=["FC-04-1969_262"],
   inferred_members=[],
   invariant="A rail that PIVOTS, swinging out to carry a fly leg and support a leaf. Requires a "
             "hinge/knuckle interface absent from every other rail family.",
   variation="swing length; depth; pivot offset from the rail end",
   joints="knuckle joint or pivot pin at one end",
   excluded="every fixed rail family - the moving interface is a construction difference, not a parameter",
   reuse="bespoke",
   donor="AUDIT_REQUIRED - none",
   unlocks="side table (game table); would extend to any drop-leaf",
   blockers="knuckle geometry not resolvable; alt7 shows it extended but not the joint"),

 # ---------------------------------------------------------------- REAR LEG/STILE
 dict(parent="rear leg/stile", name="stile.continuous_leg_and_stile",
   members=["FC-03-30_120_5","FC-05-69_146_3","FC-07-1999_79","FC-10-1971_281","FC-10-1984_161"],
   inferred_members=["FC-01-1999_488"],
   invariant="ONE stick doing both stile and leg: runs unbroken from crest or cornice height down "
             "through the seat/case frame to the floor. This is the baseline chair's defining grammar.",
   variation="rake angle of the upper portion; section (square vs board-like); taper amount; whether "
             "it terminates in its own square foot or continues to a base moulding",
   joints="mortised on two faces for rails at multiple heights",
   excluded="FC-01-2012_216 - its rear support is explicitly a SHORT SEPARATE TURNED LEG, not "
            "continuous. FC-09-1954_151 - all four corners are the same turned leg master.",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - candidate rough_hewn_timber_beam_v1 for continuous stock; NOT compared",
   unlocks="settee, desk, sideboard, chest x2",
   blockers="FC-01-1999_488 - the junction is masked by the seat rail and gilding, so continuity is "
            "PARTIALLY VISIBLE only and recorded at half weight"),

 dict(parent="rear leg/stile", name="stile.separate_rear_leg",
   members=["FC-01-2012_216","FC-09-1954_151","FC-04-1969_262","FC-11-10_125_1"],
   inferred_members=[],
   invariant="The rear support is a SEPARATE member, not continuous with anything above. Often the "
             "same blank as the front leg with decoration suppressed (1969.262: identical blank, "
             "inlay omitted on the unseen face).",
   variation="whether ornament is suppressed on the rear elevation; block schedule; turning profile "
             "shared with the front leg",
   joints="tenon into seat/case rails only",
   excluded="stile.continuous_leg_and_stile - a genuine construction opposite",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, bedstead, side table, long table",
   blockers="none material - this family is unusually well evidenced (rear elevations published)"),

 dict(parent="rear leg/stile", name="stile.plain_back_board",
   members=["FC-08-2019_59","FC-09-69_146_1"],
   inferred_members=[],
   invariant="Deliberately plain rear member, board-like, notched rather than moulded, because the "
             "rear is unseen. Its plainness is the design intent, not a loss of evidence.",
   variation="board width; notch depth and shape; total length (300 cm floor-to-tester on 69.146.1)",
   joints="housed or nailed to the case; canopy support at the head",
   excluded="all show-face stile families",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="wardrobe, bedstead",
   blockers="none material"),

 # ---------------------------------------------------------------- BRACE
 dict(parent="brace", name="brace.corner_glue_block",
   members=["FC-03-30_120_5","FC-04-1969_262","FC-08-2019_59"],
   inferred_members=["FC-02-67_230","FC-04-1972_47","FC-07-1999_79","FC-01-2012_216"],
   invariant="A triangular or quarter block glued into the inside corner of a rail frame. Two mating "
             "faces set by the angle of the members it joins.",
   variation="block depth; the two mating face angles (1969.262 mates to a CURVED apron, so the recipe "
             "needs a curved-face variant)",
   joints="glued, sometimes screwed, to two rail faces",
   excluded="brace.structural_gusset (a show member); brace.lid_cleat (a board, not a block)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="stool, settee, side table, sideboard, wardrobe",
   blockers="CRITICAL - 4 of 7 members are INFERRED FROM TYPE and carry ZERO construction authority. "
            "Only 3 members are actually evidenced. No underside view exists on any inferred member."),

 dict(parent="brace", name="brace.shaped_show_bracket",
   members=["FC-01-1999_488","FC-09-69_146_1","FC-09-1954_151"],
   inferred_members=[],
   invariant="A flat sawn board cut to a curve (arch-pierced gusset, cyma bracket, shaped console), "
             "left visible and finished to match. A 2D profile swept to constant thickness.",
   variation="triangle or cyma proportions; piercing radius and count; span, rise and thickness all free",
   joints="glued and pegged between two members; on 1954.151 drilled with weight-reduction holes",
   excluded="brace.corner_glue_block (hidden, prismatic)",
   reuse="shared_profile",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, bedstead x2",
   blockers="none material"),

 dict(parent="brace", name="brace.lid_cleat",
   members=["FC-10-1971_281","FC-10-1984_161"],
   inferred_members=[],
   invariant="A batten fixed across the underside of a lid to resist cupping and locate the lid on "
             "the carcase.",
   variation="cleat width; fixing pattern (pegs vs screws); whether moulded to match the lid edge",
   joints="pegged or screwed to the lid underside",
   excluded="all in-frame braces",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="chest x2",
   blockers="1971.281's cleats are a LATER REPAIR in pale unpatinated wood with modern slotted "
            "screws - explicitly not original, and must not be treated as period evidence"),

 dict(parent="brace", name="brace.metal_stay",
   members=["FC-05-69_146_3"],
   inferred_members=["FC-12-2002_298"],
   invariant="A metal arm controlling or supporting a moving element (quadrant stay for a fall front; "
             "an internal tie rod).",
   variation="arc radius and chord (set by fall depth); strap width; sliding vs fixed upper end",
   joints="pivots on the carcase stile; slides in a slot",
   excluded="all timber braces",
   reuse="bespoke",
   donor="AUDIT_REQUIRED - forged_iron_v1 is a MATERIAL, not a stay geometry donor",
   unlocks="desk",
   blockers="FC-12-2002_298's internal rod is ASSERTED NOT SEEN and carries zero weight"),

 # ---------------------------------------------------------------- PANEL FRAME
 dict(parent="panel frame", name="frame.stile_rail_muntin_grid",
   members=["FC-06-69_146_2","FC-10-1971_281","FC-10-1984_161","FC-08-2019_59","FC-05-69_146_3",
            "FC-08-69_140ab"],
   inferred_members=[],
   invariant="The joined grammar stile-muntin-...-stile crossed by upper and lower rails, all pegged, "
             "grooved throughout to receive floating panels. Bay count is the parameter; the JOINT "
             "SCHEDULE is the invariant.",
   variation="bay count and proportion; stile/rail widths; muntin count; groove depth; whether the "
             "frame receives applied ornament or is left plain",
   joints="mortise-and-tenon, pegged; continuous groove",
   excluded="frame.applied_moulded_surround (an applied margin, no groove, carries no panel structurally)",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - structural_oak_door_v1 is a MATERIAL asset. Do NOT treat the name as a "
         "frame-and-panel donor.",
   unlocks="chest x2, wardrobe x2, chest of drawers, desk",
   blockers="groove depth unmeasurable; back frames unphotographed on several members"),

 dict(parent="panel frame", name="frame.applied_moulded_surround",
   members=["FC-07-1999_79","FC-09-69_146_1","FC-04-1972_47","FC-01-2012_216"],
   inferred_members=[],
   invariant="Concentric stepped/reeded margins applied to a flat carcase face, shrinking inward to a "
             "flat field. Not a joined frame - a moulding layout on a board.",
   variation="aperture width and height; number of steps in the ovolo/cove stack; corner treatment "
             "(plain mitre throughout on 69.146.1)",
   joints="glued/pinned to the carcase face; mitred at corners",
   excluded="frame.stile_rail_muntin_grid - that is structural joinery, this is applied trim. "
            "Confusing them would produce a frame that cannot hold a panel.",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="sideboard, bedstead, side table, armchair",
   blockers="none material"),

 dict(parent="panel frame", name="frame.assembled_from_members",
   members=["FC-09-1954_151","FC-03-2011_3","FC-01-1999_488"],
   inferred_members=[],
   invariant="There is NO frame part. What reads as a frame is the ASSEMBLY of post + post + bottom "
             "rail + crest. Authoring a frame object here would duplicate members already modelled.",
   variation="n/a - the parameters belong to the member recipes",
   joints="the members' own joints",
   excluded="both real frame families",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - none",
   unlocks="bedstead, settee, armchair",
   blockers="none - this family exists to PREVENT a modelling error, not to be built"),

 # ---------------------------------------------------------------- FLOATING PANEL
 dict(parent="floating panel", name="panel.floating_board_in_groove",
   members=["FC-06-69_146_2","FC-08-2019_59","FC-10-1971_281","FC-10-1984_161","FC-05-69_146_3",
            "FC-08-69_140ab"],
   inferred_members=[],
   invariant="A board seated loose in the surrounding frame groove, free to move. Blank is separable "
             "from its face treatment: a plain, a carved and a marquetry panel are the SAME blank.",
   variation="aspect ratio; thickness; flat vs raised/fielded; bevel depth; shouldered vs rectangular "
             "raised field; face treatment swapped independently of the blank",
   joints="loose in groove - no glue, no fixing. This is the defining interface.",
   excluded="panel.upholstered_field (textile, not a board); panel.applied_flat_field (glued flush)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="chest x2, wardrobe x2, chest of drawers, desk",
   blockers="groove engagement depth not visible on any member"),

 dict(parent="floating panel", name="panel.upholstered_field",
   members=["FC-01-1999_488","FC-01-2012_216","FC-03-2011_3"],
   inferred_members=[],
   invariant="A textile field set within a show-wood lip: foundation + padding + cover, crowned. "
             "Occupies the panel SLOT but shares no construction with a wooden panel.",
   variation="field size; padding crown height; edge roll; whether both faces are upholstered "
             "(1999.488 is upholstered on BOTH faces of the back frame)",
   joints="tacked to the frame rebate; trimmed with gimp",
   excluded="panel.floating_board_in_groove - a clean demonstration that the frame slot and the fill "
            "are separable, but they are NOT interchangeable recipes",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair x2, settee",
   blockers="webbing beneath is INFERRED on every member - zero construction authority"),

 # ---------------------------------------------------------------- CREST/CORNICE
 dict(parent="crest/cornice", name="cornice.stacked_fillet_moulding",
   members=["FC-05-69_146_3","FC-06-69_146_2","FC-08-69_140ab","FC-08-2019_59","FC-09-69_146_1",
            "FC-07-1999_79"],
   inferred_members=[],
   invariant="A stack of fillet/cove/ogee members swept along a 3-sided return path at the top of a "
             "case. Profile stack depth and projection are the parameters.",
   variation="projection depth; number of steps/fillets in the stack; profile section; whether it "
             "returns on the sides only or wraps; optional dentil course",
   joints="applied to the carcase top; mitred at the front corners",
   excluded="crest.shaped_sawn_board (a silhouette, not a swept profile)",
   reuse="shared_profile",
   donor="AUDIT_REQUIRED - none",
   unlocks="desk, chest of drawers, wardrobe x2, bedstead, sideboard",
   blockers="none material - the best-evidenced family in the corpus"),

 dict(parent="crest/cornice", name="crest.shaped_sawn_board",
   members=["FC-03-30_120_5","FC-03-2011_3","FC-09-1954_151","FC-01-1999_488","FC-01-2012_216"],
   inferred_members=[],
   invariant="A board sawn to a decorative silhouette at the top of a back or head: serpentine "
             "camel-back, low pediment, arched cresting, rolled crest. Defined by a 2D outline curve.",
   variation="silhouette curve (double-ogee, triangular pediment, arch, roll); whether carved on the "
             "front face only (2011.3: rear is plain); lamination for tight curves (1954.151 shows "
             "visible lamination lines)",
   joints="tenoned between the two rear posts",
   excluded="cornice.stacked_fillet_moulding",
   reuse="shared_profile",
   donor="AUDIT_REQUIRED - none",
   unlocks="settee x2, bedstead, armchair x2",
   blockers="none material"),

 dict(parent="crest/cornice", name="crest.metal_capital",
   members=["FC-12-2002_298"],
   inferred_members=[],
   invariant="A cast/spun gilt-brass Corinthian-type bell with corner volutes and applied masks.",
   variation="bell flute count; volute mass; mask selection",
   joints="seated on the stone shaft",
   excluded="all timber families",
   reuse="bespoke",
   donor="AUDIT_REQUIRED - none",
   unlocks="console/pedestal",
   blockers="single-member family"),

 # ---------------------------------------------------------------- MOULDING RUN
 dict(parent="moulding run", name="moulding.profile_library",
   members=["FC-01-1999_488","FC-04-1972_47","FC-05-69_146_3","FC-06-69_146_2","FC-07-1999_79",
            "FC-08-69_140ab","FC-08-2019_59","FC-10-1971_281","FC-10-1984_161","FC-11-10_125_1",
            "FC-01-2012_216"],
   inferred_members=[],
   invariant="A constant section swept along a path. The SECTION is the authored asset; the path is "
             "free. Multiple objects state explicitly that one section serves several roles at "
             "different scales (69.146.2: one profile serves drawer surrounds, side-panel frames and "
             "the plinth).",
   variation="section swap (chamfer / ovolo / cavetto / roll / astragal / thumbnail / multi-reed / "
             "stepped ogee); run length; mitre vs stopped ends; scale",
   joints="applied and mitred",
   excluded="moulding.repeating_cell_run - that instances a discrete carved unit along a path, which "
            "is a different generator",
   reuse="shared_profile",
   donor="AUDIT_REQUIRED - none",
   unlocks="ELEVEN of eighteen objects - the widest-reaching shared_profile family",
   blockers="none material"),

 dict(parent="moulding run", name="moulding.repeating_cell_run",
   members=["FC-03-2011_3","FC-09-69_146_1","FC-09-1954_151","FC-12-2002_298"],
   inferred_members=[],
   invariant="A discrete carved or cast CELL instanced at a regular pitch along a path "
             "(bead-and-reel, rosette chain, wave scroll, lappet register). Generator is "
             "instance-along-curve, not sweep.",
   variation="cell pitch; cell selection; run length; register count; whether the run is straight or "
             "follows a curve",
   joints="applied along an edge",
   excluded="moulding.profile_library",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="settee, bedstead x2, console",
   blockers="individual cell boundaries not countable on 2002.298"),

 # ---------------------------------------------------------------- ORNAMENT
 dict(parent="repeated ornament master", name="ornament.instanced_relief_master",
   members=["FC-01-1999_488","FC-02-67_230","FC-03-2011_3","FC-05-69_146_3","FC-07-1999_79",
            "FC-09-69_146_1","FC-09-1954_151","FC-12-2002_298","FC-10-1984_161"],
   inferred_members=[],
   invariant="ONE hand-authored relief master instanced many times with only placement transforms. "
             "The master itself is bespoke; the INSTANCING is the recipe.",
   variation="instance count; pitch; scale; rotation; mirror; scatter density (69.140ab states the "
             "placement is a GRADED FALL - dense at top, thinning down - not random)",
   joints="applied, screwed (1954.151 shows a garland screwed to a plain sawn backing) or carved in",
   excluded="ornament.inlay_path_master (2D, flush, no relief)",
   reuse="assembly_grammar",
   donor="AUDIT_REQUIRED - none",
   unlocks="nine of eighteen objects",
   blockers="the masters themselves must be hand-modelled; only their distribution is generable"),

 dict(parent="repeated ornament master", name="ornament.marquetry_motif_set",
   members=["FC-05-69_146_3","FC-06-69_146_2","FC-08-69_140ab","FC-09-69_146_1"],
   inferred_members=[],
   invariant="A small set of flat cut-veneer masters (blossom, serrated leaf, stem) composed into a "
             "scrolling field. 69.146.3 states the field is built from roughly THREE masters.",
   variation="scatter density; mirror axis; field aspect ratio; instance count (narrow drawers use "
             "the same masters at lower count)",
   joints="veneer laid flush into the ground",
   excluded="ornament.instanced_relief_master (3D relief)",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="desk, chest of drawers, wardrobe, bedstead - and it is the BASELINE's own botanical "
           "marquetry grammar",
   blockers="none material"),

 dict(parent="repeated ornament master", name="ornament.figural_bespoke",
   members=["FC-01-1999_488","FC-03-30_120_5","FC-12-2002_298"],
   inferred_members=[],
   invariant="A sculpted head, mask or figural cartouche. Mirror is the only honest transform.",
   variation="scale and mirror ONLY",
   joints="carved integral or applied",
   excluded="both instanced families - these cannot be generated",
   reuse="bespoke",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, settee, console",
   blockers="hand-modelling required; no parametric route"),

 # ---------------------------------------------------------------- INLAY PATH
 dict(parent="inlay path", name="inlay.linear_stringing_path",
   members=["FC-04-1969_262","FC-05-69_146_3","FC-06-69_146_2","FC-08-69_140ab","FC-02-67_230",
            "FC-08-2019_59","FC-07-1999_79"],
   inferred_members=[],
   invariant="A line of contrasting material inset at a fixed distance from a part edge, following a "
             "closed rectangular or cusped path. The PATH is the asset; the material is a parameter.",
   variation="line width; contrast material (wood / brass / paint / gilt); corner radius; inset "
             "distance from the edge; plain rectangle vs cusped cartouche silhouette",
   joints="inlaid flush into the ground, or painted on (1999.79 - the same PATH grammar realised as "
          "paint rather than inlay, which is a material swap on one recipe)",
   excluded="inlay.node_and_cell_punctuation",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="side table, desk, chest of drawers, wardrobe x2, stool, sideboard",
   blockers="none material"),

 dict(parent="inlay path", name="inlay.node_and_cell_punctuation",
   members=["FC-01-2012_216","FC-09-69_146_1","FC-04-1972_47"],
   inferred_members=[],
   invariant="A discrete cell (disc-in-bezel, framed rosette square) placed where a linear path turns "
             "a corner or terminates. 69.146.1 calls it explicitly 'the punctuation mark used wherever "
             "a linear path turns a corner'.",
   variation="cell size; spacing along the run; whether the cell is a fixed-width cap with a "
             "STRETCHABLE band between (69.146.1's two-part layout generalises to any long band)",
   joints="inlaid flush",
   excluded="inlay.linear_stringing_path - complementary, not interchangeable",
   reuse="parameterized_recipe",
   donor="AUDIT_REQUIRED - none",
   unlocks="armchair, bedstead, side table",
   blockers="none material"),

 dict(parent="inlay path", name="inlay.absent_recorded",
   members=["FC-03-2011_3"],
   inferred_members=[],
   invariant="RECORDED AS ABSENT. All ornament carved in the solid and water-gilt; no linear or "
             "circular inlay anywhere. Kept as a family so the absence is visible in the matrix "
             "rather than reading as missing data.",
   variation="n/a", joints="n/a",
   excluded="n/a", reuse="bespoke",
   donor="n/a",
   unlocks="none - this is a negative record",
   blockers="none"),
]
