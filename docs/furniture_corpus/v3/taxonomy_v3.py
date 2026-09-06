"""Furniture Grammar Census v3 — a SOURCE-CORRECT repair of v2.

v1 and v2 are untouched. This file is the single generator for every v3 table, so every
number in the README is reproducible by running it.

WHAT V2 GOT WRONG, AND WHAT CHANGED
-----------------------------------
1. TRUNCATED IDS. v2 cited three candidate IDs that do not exist in the BOM:
       FC-03-30_120_5  -> FC-03-30_120_59   (8 memberships)
       FC-11-10_125_1  -> FC-11-10_125_133  (4 memberships)
       FC-12-2002_298  -> FC-12-2002_298_1  (7 memberships)
   19 of v2's 153 assignments, across 17 of 41 families, pointed at nothing. Anything
   computed by joining to the BOM silently dropped them. Reconciled below.

2. CANDIDATE-LEVEL MEMBERSHIP. v2 assigned whole CANDIDATES to families. A candidate is
   an object, not a part: one object contributes many BOM rows, and those rows are often
   built differently from each other. FC-03-2011_3's foot row states outright that the two
   REAR feet are applied turned buns while the two FRONT feet are the integral turned toe
   of the leg - two incompatible constructions inside one v2 membership.
   v3 assigns BOM ROWS, and splits a row into analytical subrows where the row itself
   records more than one construction. Source facts are never edited, only partitioned.

3. THE FAMILY TEST. v2 grouped by name, role and appearance. v3 requires COMPATIBLE
   CONSTRUCTION LOGIC, carried on five independent axes - geometry generator, host
   interface, joint schedule, assembly role, surface realization. Two parts share a
   family only if a single parameterized recipe could build both.

4. NEGATIVE FACTS RANKED AS ASSETS. v2 ranked `inlay.absent_recorded` (a record that an
   object has NO inlay) and `frame.assembled_from_members` (an anti-duplication note)
   as buildable recipes. Neither is a thing to model. Moved to their own non-asset table.

5. THE DONOR PREMISE WAS FALSE. v2 asserted "only rough_hewn_timber_beam_v1 is a geometry
   asset" and classified the rest as material-only. That was read off DIRECTORY LAYOUT
   (assets/creative/materials/...) rather than inspection. Inspecting the trees shows
   geometry builds living inside the material assets - openwork_strap_hinge_geometry_v1
   and forged_fasteners_v1.blend are both inside forged_iron_v1/, and the joinery and door
   assets each ship a .blend fixture. The premise is replaced by an inspected inventory.
   v2's CONCLUSION (zero verified exact donors) survives, but it now rests on comparison
   rather than on a miscount.

6. SCORING. v2's formula was never published and gave positive scores to families with no
   visible evidence, because a reuse-class prior was added rather than multiplied. v3
   publishes the formula, multiplies through evidence, and marks any zero-evidence family
   NOT_RANKABLE.

EVIDENCE WEIGHTS (fixed, from the brief)
    VISIBLE 1.0 | PARTIALLY VISIBLE 0.5 | INFERRED FROM TYPE 0.0 | NOT DETERMINABLE 0.0
Inferred and not-determinable rows are PRESERVED as research notes. They may never
establish a joint, an interface, family compatibility, or construction authority.

THE RANKING FORMULA, stated once and implemented once (see score()):

    weighted_evidence  W  = sum over member subrows of the evidence weight
    objects_unlocked   N  = count of DISTINCT candidates contributing a subrow with w > 0
    construction_conf  C  = (subrows whose host interface AND joint schedule are
                             source-visible) / (all member subrows)          in [0,1]
    ambiguity_penalty  A  = (member subrows carrying an unresolved-construction flag)
                            / (all member subrows)                          in [0,1]
    reuse_multiplier   R  = 1.00 exact_donor      (none qualify)
                            0.95 toolkit_capability
                            0.90 parameterized_recipe
                            0.75 shared_profile
                            0.60 assembly_grammar
                            0.15 bespoke

    score = W * (0.5 + 0.5*C) * R * (1 - 0.5*A) * (1 + 0.15*(N-1))

    Every term multiplies through W, so W == 0  =>  score == 0  =>  NOT_RANKABLE.
    A reuse prior can never manufacture priority for a family with no seen evidence.
    The (N-1) term rewards breadth without letting one broad family beat a well-evidenced
    narrow one on breadth alone.

LANES. Unlike planning objects are never ranked against each other:
    toolkit_capability  a reusable native/profiling capability, not one asset
    component_recipe    one parameterized part
    assembly_grammar    a rule for combining parts that already exist
    bespoke_hero        a hand-authored one-off with a reusable socket
    non_asset           constraints, negative facts, anti-duplication annotations.
                        NEVER ranked.
"""
import csv, json, os, math
from collections import OrderedDict, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS = os.path.dirname(HERE)

W_EVID = {"VISIBLE": 1.0, "PARTIALLY VISIBLE": 0.5,
          "INFERRED FROM TYPE": 0.0, "NOT DETERMINABLE": 0.0}

REUSE_MULT = {"exact_donor": 1.00, "toolkit_capability": 0.95, "parameterized_recipe": 0.90,
              "shared_profile": 0.75, "assembly_grammar": 0.60, "bespoke": 0.15}

# ---------------------------------------------------------------- 1. SOURCE IDENTITY

ID_FIXES = OrderedDict([
    ("FC-03-30_120_5",  "FC-03-30_120_59"),
    ("FC-11-10_125_1",  "FC-11-10_125_133"),
    ("FC-12-2002_298",  "FC-12-2002_298_1"),
])


def load_bom():
    """BOM rows with a stable bom_row_id = R### in file order. Order is the identity."""
    rows = list(csv.DictReader(open(os.path.join(CORPUS, "component_bom_v1.csv"))))
    for i, r in enumerate(rows):
        r["bom_row_id"] = "R%03d" % i
    return rows


# SUBROWS. A BOM row is split ONLY where the row's own text records more than one
# construction. The quote that forces each split is carried in `because`.
# (parent_row_id, suffix, subpart, evidence_override_or_None, because)
SUBROWS = [
 ("R064","a","rear pair - applied turned bun under each rear post", None,
  "'turned compressed bun APPLIED under each rear post'"),
 ("R064","b","front pair - integral turned ovoid toe of the leg", None,
  "'the two FRONT feet are NOT separate parts - they are the integral turned ovoid toe "
  "terminating the front leg turning'"),
 ("R105","a","claw-and-ball carved foot, three front legs", None,
  "'Ball gripped by four talons, on all three front legs only'"),
 ("R105","b","rear legs - chamfered taper, no foot part", None,
  "'the three rear legs simply taper to a chamfered end with no distinct foot'"),
 ("R137","a","carved hairy paw with acanthus knuckle", None,
  "'carved hairy paw with acanthus knuckle and side scroll boss'"),
 ("R137","b","flat circular disc pad under the paw", None,
  "'the disc pad is separable from the paw and reusable alone'"),
 ("R193","a","front pair - turned foot, square section transitions into it","PARTIALLY VISIBLE",
  "'2 front VISIBLE'; R191 gives the interface: 'the lower chamfer/taper where the square "
  "section transitions into the turned foot'"),
 ("R193","b","rear pair - not shown","NOT DETERMINABLE","'2 rear NOT DETERMINABLE'"),
 ("R322","a","three turned feet with bored castor socket", None,
  "'3 VISIBLE (near headboard foot, both footposts)'"),
 ("R322","b","fourth foot, asserted by symmetry","INFERRED FROM TYPE","'4th inferred by symmetry'"),
 ("R405","a","front pair - turned ball foot dowelled into the plinth","PARTIALLY VISIBLE",
  "'dowel fixing into the plinth'"),
 ("R405","b","rear pair - not shown","NOT DETERMINABLE","'rear feet not shown, total NOT DETERMINABLE'"),
 ("R096","a","two tapered front cone-stump arm posts on the side seat rails", None,
  "'a tapered front cone-stump rising off the side seat rail'"),
 ("R096","b","two wide flat rear posts tying the arm to the back frame", None,
  "'a wide flat rear post tying the arm to the back frame'"),
 ("R096","c","one central back post, crest to rear seat rail", None,
  "'plus one central back post from crest to rear seat rail'"),
 ("R270","a","carcase corner post, masked on the facade","PARTIALLY VISIBLE",
  "'front pair is masked on the facade by the applied pilaster strip'"),
 ("R270","b","applied pilaster face strip", None,
  "'whether the face is left plain or receives an applied pilaster'"),
 ("R095","a","two END members continuous crest-to-floor", None,
  "'The two END members read continuous from crest height down through the seat frame to "
  "the floor (one stick doing stile + leg)'"),
 ("R095","b","centre rear leg, separate raked member", None,
  "'the CENTRE rear leg is a separate raked member joined to the rear rail'"),
 ("R214","a","front corner stiles, continuous leg-and-stile", None,
  "'the outer upright runs unbroken from the tapered foot, past the drawer, up to the "
  "acanthus capital'"),
 ("R214","b","rear members","NOT DETERMINABLE","'rear members NOT DETERMINABLE'"),
 ("R160","a","left rear stile, continuous cornice-to-floor", None,
  "'Left rear stile is visible in both overall views'"),
 ("R160","b","right rear stile, by symmetry","INFERRED FROM TYPE","'right rear is inferred by symmetry'"),
 ("R066","a","front and side seat rails, gilt show faces", None,
  "'Front and side rails are gilt show faces carrying paterae'"),
 ("R066","b","rear seat rail, bare secondary wood", None,
  "'the rear rail is left in bare wood (seen in alt4/alt8)'"),
 ("R052","a","front and two side rails, mitred closed box","PARTIALLY VISIBLE",
  "'Long front rail and both short rails are visible'"),
 ("R052","b","rear long rail, by symmetry","INFERRED FROM TYPE",
  "'the rear long rail is never shown in any published view and is inferred by symmetry'"),
 # R216's variation field records three distinct roles on the front plane; assigning the
 # aggregate to rail.frieze claimed a role for rails the source distinguishes.
 ("R216","a","two arched frieze rails, one per end bay", None,
  "'two arched frieze rails (one per end bay)'"),
 ("R216","b","rails above and below the drawers (2+2)", None,
  "'two rails above and two below the drawers'"),
 ("R216","c","top and bottom rails of the centre section", None,
  "'the top and bottom rails of the centre section'"),
 ("R231","a","main and upper cornice runs with dentil courses", None,
  "'The main cornice runs the full 180 cm with a dentil course; above it sits a separate "
  "raised case with its own smaller cornice and dentil course'"),
 ("R231","b","two mirrored ogee-swept shaped boards", None,
  "'flanked by two mirrored ogee-swept shaped boards with reeded edge-lines'"),
 ("R187","a","inlaid urn centrepiece master", None, "'a fluted, footed urn'"),
 ("R187","b","inlaid colonnaded arcade master", None,
  "'a low colonnaded arcade on a plinth used in place of a keyhole'"),
 ("R357","a","acanthus leaf collar capping every leg turning", None,
  "'the leaf collar that caps every leg turning'"),
 ("R357","b","square foliate rosette die block", None,
  "'the square foliate panel with a central rosette that fills each corner die block'"),
 ("R234","a","upright acanthus/anthemion capital master", None,
  "'Four master ornaments: (a) upright acanthus/anthemion capital'"),
 ("R234","b","husk-and-laurel swag master with anthemion drop at each junction", None,
  "'(b) husk-and-laurel swag with an anthemion drop at each junction'"),
 ("R234","c","dentil block master", None, "'(c) dentil block'"),
 ("R234","d","drilled dot master", None, "'(d) drilled dot'"),
 # R114's own count field NAMES six masters: "6 masters identified: MOP star (approx 120+
 # instances across top slab plus approx 30 per end panel), MOP circular disc, carved
 # rosette/patera, carved flower-in-linked-chain, abalone rectangular plaque, carved
 # palmette. Exact instance totals NOT DETERMINABLE". Six masters, six subrows - v3.1's
 # two-bucket split (one flat motif + one carved boss) collapsed identities the source
 # itself keeps separate.
 ("R114","a","MOP star, flat inlay, 4+ sizes", None, "'MOP star (approx 120+ instances)'"),
 ("R114","b","MOP circular disc, flat inlay", None, "'MOP circular disc'"),
 ("R114","c","carved rosette/patera, host-carved", None, "'carved rosette/patera'"),
 ("R114","d","carved flower-in-linked-chain, host-carved", None,
  "'carved flower-in-linked-chain'"),
 ("R114","e","abalone rectangular plaque, flat inlay", None, "'abalone rectangular plaque'"),
 ("R114","f","carved palmette, host-carved", None, "'carved palmette'"),
 ("R338","a","bowed-and-serpentine sawn rail form", None,
  "'BOWED OUTWARD IN PLAN and serpentine in elevation, sawn from a wide board' - the FORM "
  "is fully visible"),
 ("R338","b","rail end construction - knockdown or fixed", "NOT DETERMINABLE",
  "'whether the ends are knockdown or fixed' - the END JOINT is the open question, not the "
  "form; v3 let one flag depress both"),
 ("R247","a","frieze rails, marquetry face", None,
  "'the frieze rails carry marquetry'"),
 ("R247","b","plinth rails, reeded band", None, "'the plinth rails carry a reeded band'"),
 ("R247","c","back rails, plain secondary cherry", None,
  "'the back rails are plain secondary cherry'"),
 ("R247","d","the single rail above the drawers", None,
  "'1 rail above the drawers' - present in the count field but dropped by v3.1's three-way "
  "split, which covered frieze, plinth and back rails only"),
 ("R303","a","carved wreath ring, applied", None, "'carved wreath cartouche (applied centre'"),
 ("R303","b","inset shield marquetry cell", None,
  "'the inset shield panel is a swappable marquetry cell'"),
]


def build_rows():
    """-> list of analytical subrows, each an assignable unit with a stable id."""
    bom = load_bom()
    by_id = {r["bom_row_id"]: r for r in bom}
    splits = defaultdict(list)
    for parent, suf, subpart, ev, because in SUBROWS:
        splits[parent].append((suf, subpart, ev, because))
    out = []
    for r in bom:
        rid = r["bom_row_id"]
        if rid in splits:
            for suf, subpart, ev, because in splits[rid]:
                s = dict(r)
                s["row_uid"] = rid + "." + suf
                s["subpart"] = subpart
                s["evidence"] = ev or r["evidence"].split("(")[0].strip()
                s["split_because"] = because
                out.append(s)
        else:
            s = dict(r)
            s["row_uid"] = rid
            s["subpart"] = ""
            s["split_because"] = ""
            # the BOM's count field sometimes carries the evidence inline; normalise
            e = r["evidence"].split("(")[0].strip()
            s["evidence"] = e if e in W_EVID else "NOT DETERMINABLE"
            out.append(s)
    return out


# ---------------------------------------------------------------- 2. FAMILY MODEL
# F(fid, lane, broad, invariant, generator, host_interface, joint_schedule, assembly_role,
#   surface, variation, excluded, reuse, members, proven, unresolved, donor, note)
# `proven`    = member subrows whose HOST INTERFACE and JOINT SCHEDULE are source-visible.
# `unresolved`= member subrows carrying an open construction question.

# UNCERTAINTY IS TYPED (v3.1). v3 used ONE `unresolved` set and let it depress construction
# confidence no matter what the uncertainty was ABOUT. That penalised families for facts that
# have nothing to do with how the part is built - R438's glass cabochons are perfectly well
# understood as construction; what is unknown is HOW MANY there are, because only the front
# half of any band is ever photographed. Counting that as construction doubt is wrong.
#   construction  how the part is made / how it meets its host   -> REDUCES confidence
#   quantity      how many there are                             -> recorded, no penalty
#   view_coverage what the photographs happen to show            -> recorded, no penalty
#   surface       material or finish                             -> recorded, no penalty
#   provenance    original vs repair vs later addition           -> recorded, no penalty
UNCERT_TYPES = ("construction", "quantity", "view_coverage", "surface", "provenance")


def F(fid, lane, broad, invariant, generator, host, joints, role, surface, variation,
      excluded, reuse, members, proven=(), unresolved=(), uncert=None,
      status="RANKABLE", donor="AUDIT_REQUIRED", note=""):
    """`unresolved` is the legacy shorthand for CONSTRUCTION uncertainty.
    `uncert` overrides it per row: {row_uid: "quantity"|"view_coverage"|...}.
    `status` "AUDIT_ONLY" marks an analysis bucket that must never be ranked."""
    u = {r: "construction" for r in unresolved}
    for k, v in (uncert or {}).items():
        assert v in UNCERT_TYPES, (fid, k, v)
        u[k] = v
    return dict(fid=fid, lane=lane, broad=broad, invariant=invariant, generator=generator,
                host=host, joints=joints, role=role, surface=surface, variation=variation,
                excluded=excluded, reuse=reuse, members=list(members),
                proven=set(proven), uncert=u,
                unresolved=set(k for k, v in u.items() if v == "construction"),
                status=status, donor=donor, note=note)


FAMILIES = [

# ============================================================ FOOT
F("foot.turned_separate_dowelled", "component_recipe", "foot",
  "A lathe-turned solid of revolution made as its OWN part and attached to the member "
  "above by a dowel or a square block. The joint is proved, not assumed.",
  "revolve of a 2D profile (pad / ball or bun / neck / collar)",
  "underside of plinth, leg blank or case base",
  "dowel into plinth (1984.161, stated); square block above the ball seen in the underside "
  "view (2019.59); applied under the rear post (2011.3)",
  "floor contact, load path into the case",
  "ebonised, gilt or clear-finished turned wood",
  "ball/bun/onion/trumpet silhouette; height; max diameter; ring count and pitch; pad "
  "diameter and thickness; dowel diameter",
  "every integral turned terminal - attaching one of these would mean CUTTING the leg and "
  "changing its joint schedule; every prismatic termination; the spun-metal base",
  "parameterized_recipe",
  ["R294","R405.a","R064.a"], proven=["R294","R405.a","R064.a"],
  donor="AUDIT_REQUIRED - no turned-solid donor located; rough_hewn_timber_beam_v1 is hewn, "
        "shares no lathe logic; silhouette/section/joints NOT compared",
  note="The only foot family where BOTH the revolved geometry and the host interface are "
       "source-visible. This is what v2's foot.turned_separate should have been."),

F("foot.turned_integral_with_leg", "component_recipe", "foot",
  "NOT a separate part. The turned terminal is the bottom of the leg or post turning "
  "itself - one blank, one lathe setup, no joint at all.",
  "continuation of the leg's own revolve profile",
  "none - continuous stock with the member above",
  "none; the interface upward is the seat rail or lower rail mortise",
  "floor contact, continuous with the leg",
  "same finish as the leg it terminates",
  "toe fullness; ring/collar count above the toe; where the taper stops",
  "every dowelled or applied foot - they are a different joint contract entirely",
  "parameterized_recipe",
  ["R023","R064.b"], proven=["R023","R064.b"],
  note="Split from the dowelled family on the joint contract, per the v2 audit. 2011.3 "
       "carries BOTH constructions on one object, which is why R064 is split."),

F("foot.turned_interface_unresolved", "component_recipe", "foot",
  "Revolved foot geometry is clearly visible; whether it is a separate turning or integral "
  "with the member above is NOT resolvable in the published views.",
  "revolve of a 2D profile",
  "UNRESOLVED - the defining open question of this family",
  "UNRESOLVED",
  "floor contact",
  "turned wood",
  "silhouette; height; diameter; ring count",
  "any row where the interface IS proved, in either direction",
  "parameterized_recipe",
  ["R051","R161","R414","R193.a"], proven=[],
  unresolved=["R051","R161","R414","R193.a"], status="AUDIT_ONLY",
  note="AUDIT_ONLY - an analysis bucket, never a production queue entry. A visible "
       "silhouette with an unknown interface cannot authorise a recipe. R193.a arrives here "
       "from foot.turned_integral_with_leg: 69.146.2's post 'transitions into' the turned "
       "foot, which describes a shape change, not a proved continuity of stock. "
       "67.230 says "
       "so outright: 'Whether the foot is integral to the shaft turning or a separate glued "
       "turning is not resolvable from these views.' These rows may not establish a joint."),

F("foot.turned_with_castor_socket", "component_recipe", "foot",
  "Turned foot whose pad is BORED to receive a castor - the socket is part of the recipe.",
  "revolve, then a bored axial socket",
  "leg or post above; castor stem below",
  "bored socket, castor stem",
  "floor contact via a rolling castor",
  "turned wood",
  "height; diameter; ring count above the socket; socket bore",
  "feet with a flat resting face and no castor provision (1999.488 states there are none)",
  "parameterized_recipe",
  ["R322.a"], proven=["R322.a"], unresolved=[],
  note="One evidenced object. The 4th foot (R322.b) is inferred by symmetry and carries "
       "zero weight."),

F("foot.disc_pad_separable", "component_recipe", "foot",
  "A flat circular disc pad that seats a carved foot above it and is explicitly reusable "
  "on its own.",
  "revolve - a simple disc",
  "underside of a carved paw/claw master",
  "glued or dowelled seat under the carved foot",
  "floor contact under a bespoke carving",
  "gilt or finished wood",
  "diameter; thickness",
  "the carved paw above it, which is bespoke sculpture",
  "parameterized_recipe",
  ["R137.b"], proven=["R137.b"],
  note="Extracted from the paw-foot row because the row itself says the pad 'is separable "
       "from the paw and reusable alone'. The pad is a recipe; the paw is not."),

F("foot.spun_metal_base", "component_recipe", "foot",
  "A spun or cast metal shell sitting on the plinth - a metal process, not woodturning.",
  "revolve, thin-shell",
  "top face of the stone plinth",
  "seated shell, no wood joint",
  "base of a stacked pedestal",
  "gilt brass",
  "flare ratio; lappet count round the ring; bead pitch on the upper fillet",
  "all turned-wood feet - different material, process and wall condition",
  "parameterized_recipe",
  ["R429"], proven=["R429"],
  note="Kept separate from wood turning: a spun shell and a solid turning are not one "
       "generator even though both are solids of revolution."),

# ============================================================ TERMINATION (moved out of foot)
F("termination.leg_toe_prismatic", "component_recipe", "foot",
  "A leg blank ending in a prismatic block, pad flare or chamfer - a cut on the leg, not a "
  "solid of revolution and not a separate part.",
  "prismatic cut / chamfer on the leg blank",
  "none - continuous with the leg",
  "none below; apron or rail mortise above",
  "floor contact at the end of a tapered leg",
  "gilt block, reeded cuff, or plain",
  "block height; cuff reed count; chamfer angle; pad flare",
  "all revolved feet",
  "parameterized_recipe",
  ["R240","R105.b"], proven=["R240","R105.b"],
  note="Per the audit: prismatic integral toes belong in leg-termination logic, not in a "
       "foot family. 1969.262 carries an unexplained horizontal seam ~12 cm up which may be "
       "an original cuff line or a spliced repair - recorded, not resolved."),

# ============================================================ POST
F("post.case_corner_structural_joined", "component_recipe", "post",
  "A true structural corner post of a carcase, showing on TWO adjacent faces and mortised "
  "on both. Continuity past the case seams is proved by view, not assumed.",
  "extruded square prism with face treatments",
  "case sides and front/back frames on two adjacent faces",
  "mortise-and-tenon on two ADJACENT faces (this is what separates it from a muntin)",
  "carcase corner, primary load path",
  "reeded, fluted or ebonised show faces",
  "height; square section; reed/flute count; roundel diameter at the head; whether the post "
  "projects above the cornice",
  "panel muntins (grooved on OPPOSITE faces); applied pilasters (no structural role); posts "
  "whose joint schedule is not resolvable",
  "parameterized_recipe",
  ["R242","R191"], proven=["R242","R191"],
  note="69.140ab is the strongest construction evidence in the corpus: 'confirmed in the "
       "rear view where the same black post is unbroken past both case seams.'"),

F("post.carcase_unresolved", "component_recipe", "post",
  "A vertical carcase member whose STRUCTURAL role is not resolvable - it may be a joined "
  "corner post or an applied face pilaster.",
  "extruded prism",
  "UNRESOLVED",
  "UNRESOLVED",
  "carcase front, possibly structural",
  "reeded with carved roundel blocks",
  "height; width; reed count; roundel block count and spacing",
  "posts with a proved joint schedule, in either direction",
  "parameterized_recipe",
  ["R159","R270.a"], proven=[], unresolved=["R159","R270.a"], status="AUDIT_ONLY",
  note="AUDIT_ONLY - a research bucket, not a production recipe. Per the audit: joined structural corner stiles must be split from applied pilasters "
       "AND from unresolved carcase posts. These two are the unresolved bucket."),

F("pilaster.applied_face_strip", "component_recipe", "post",
  "A decorative strip APPLIED to the face of a carcase post. Carries no load and has no "
  "mortise; it masks the post behind it.",
  "extruded profile strip",
  "glued to the face of a post or stile",
  "glued applique only",
  "facade decoration",
  "moulded/ebonised show face",
  "width; profile; length; cross-block count",
  "the structural post underneath, which is a different part with a different joint schedule",
  "parameterized_recipe",
  ["R270.b"], proven=["R270.b"],
  note="Extracted from R270 because the row records the applied strip and the post it masks "
       "as two separate things."),

F("muntin.panel_grooved", "component_recipe", "post",
  "An intermediate upright inside a frame, GROOVED ON TWO OPPOSITE FACES to capture the "
  "panels either side. Opposite-face grooving is the whole distinction from a corner post.",
  "extruded prism with a groove on each of two opposite faces",
  "upper and lower rails of the frame it divides",
  "stub tenons into rail mortises; panels float in the grooves, unglued",
  "divides a frame into bays",
  "plain, moulded, or carrying applied spindles",
  "count (drives bay count); width; section; spacing; whether it carries applied spindles",
  "corner posts, which are mortised on ADJACENT faces; upholstery tacking muntins, which "
  "have no groove at all",
  "parameterized_recipe",
  ["R360","R378"], proven=["R360","R378"],
  note="R320 was filed under 'moulding run' in v1 and refiled here in v3; it is QUARANTINED "
       "in v3.1 because neither the opposite-face grooves nor the rail tenons are visible - "
       "the row records only 'spacing and count across a panel width', which is a layout "
       "fact, not a joint schedule."),

F("muntin.upholstery_tacking", "component_recipe", "post",
  "An intermediate upright that divides an UPHOLSTERED back into webbing bays. It is a "
  "tacking ground - no groove, no panel, and it is invisible when the cover is on.",
  "extruded prism, plain",
  "upper and lower back-frame rails",
  "tenoned or lapped; webbing tacked to its faces",
  "divides an upholstered field into webbing bays",
  "bare secondary wood - never seen in the finished object",
  "bay count (chair 0, this settee 1, longer sofa 2-3); section",
  "grooved panel muntins - a groove would serve no purpose here",
  "parameterized_recipe",
  ["R073"], proven=["R073"],
  note="The audit's split, and the source supports it exactly: 'central vertical member "
       "dividing the upholstered back into two webbing bays; visible only in alt4 with the "
       "outer cover off.'"),

F("armpost.on_seat_rail_short", "component_recipe", "post",
  "A SHORT post standing on the seat rail and carrying the arm above it. Both ends are "
  "joints; it spans only the seat-to-arm gap.",
  "revolve or tapered prism, short",
  "seat rail below, arm rail above",
  "tenon or dowel at both ends",
  "carries the arm off the seat frame",
  "faceted, gilt, or tapered show wood",
  "height; taper; section; facet count; carving presence",
  "posts that tie into the BACK frame (different upper host); continuous arm/back stiles "
  "(no joint at all)",
  "parameterized_recipe",
  ["R002","R096.a"], proven=["R002","R096.a"],
  note="Per the audit, post.arm_support is split three ways by what the post actually "
       "connects. This is the short-post-on-seat-rail case."),

F("armpost.rear_tied_to_back_frame", "component_recipe", "post",
  "A wide FLAT post tying the arm into the back frame. Its upper host is the back frame, "
  "not the arm rail, so its joint schedule differs from a short arm post.",
  "flat sawn board, tapered",
  "side seat rail below, back frame behind",
  "tenon below; lapped or tenoned into the back frame",
  "ties the arm assembly into the back",
  "show wood or covered",
  "width; taper; rake; height",
  "front cone-stump arm posts; continuous arm/back stiles",
  "parameterized_recipe",
  ["R096.b"], proven=["R096.b"]),

F("stile.continuous_curved_arm_back", "assembly_grammar", "post",
  "One continuous curved member doing arm rail AND back stile AND crest junction with NO "
  "joint break. There is nothing to assemble - the modelling error is treating it as parts.",
  "swept section along a compound 3D curve",
  "seat frame below; crest above; both are continuations of the same stick",
  "NONE along the run - that is the defining fact",
  "arm and back outer edge in one member",
  "gilt show wood, reeded-and-inlaid on the inside face only",
  "sweep radius; scoop depth under the elbow; section width; whether the inside face carries "
  "the reeded band",
  "any jointed arm-post-plus-arm-rail construction",
  "assembly_grammar",
  ["R028","R027"], proven=["R028","R027"],
  note="2012.216 states it: 'rises from the front terminal, dips into a shallow scoop under "
       "the elbow, and turns up into the back stile WITHOUT A JOINT BREAK.'"),

F("post.back_intermediate", "component_recipe", "post",
  "An intermediate vertical in the BACK frame running crest to rear seat rail.",
  "extruded prism",
  "crest rail above, rear seat rail below",
  "tenon at both ends",
  "stiffens a wide back; carries medial rails",
  "covered or plain",
  "count as a function of back width (1 here; 2-3 for a longer bench); section",
  "arm posts; muntins that capture panels",
  "parameterized_recipe",
  ["R096.c"], proven=["R096.c"]),

F("post.bed_stacked_block", "assembly_grammar", "post",
  "A post built as a STACK of discrete solids. Adding or dropping a stage rescales the post "
  "without any new geometry - the grammar is the asset, not the post.",
  "ordered stack of pre-existing solids",
  "floor/foot below; tester or finial above",
  "stacked and pinned; each stage is a separate solid",
  "bed post carrying a tester",
  "carved, inlaid, moulded per stage",
  "stack ORDER and stage count; per-stage height and diameter",
  "one-piece turned or fluted posts",
  "assembly_grammar",
  ["R300"], proven=["R300"],
  note="'Each stage is a separate solid; adding or dropping a stage rescales the post "
       "without new geometry.'"),

F("post.bed_square_fluted", "component_recipe", "post",
  "One square fluted post recipe run at two different lengths on the same object - the "
  "object itself proves length is the parameter.",
  "extruded square prism, fluted faces, carved capital block",
  "floor or corner block below; tester/canopy above",
  "tenon or dowel at the cap; die block below",
  "bed corner post",
  "gilt on show faces, bare wood on rear faces",
  "LENGTH (proved: 163.2 cm vs 132.7 cm from one recipe); flute count; flute stop height; "
  "capital leaf density; which faces are gilded",
  "stacked-block posts; turned posts",
  "parameterized_recipe",
  ["R333","R299"], proven=["R333","R299"],
  note="The cleanest parametric proof in the corpus: 'the head-end pair and foot-end pair "
       "are the SAME square fluted post recipe run at two different lengths.'"),

F("post.turned_pivot_gate", "component_recipe", "post",
  "A turned upright that BEARS A PIVOT - the gate swings on it. The bearing is the reason "
  "the part exists and it is not shared with any static post.",
  "revolve with a bored or pinned pivot seat",
  "top and bottom rails of the fixed frame",
  "pivot bearing (pin/socket) top and bottom",
  "hinge axis for a swinging gate leg",
  "turned wood",
  "diameter relative to the corner leg; count along the side; presence at all (a 6-leg "
  "version drops them)",
  "the stacked stone-and-metal pedestal, which is not a recipe and has no pivot; static "
  "turned supports",
  "parameterized_recipe",
  ["R413"], proven=["R413"],
  note="The audit's split of post.turned_support. A pivot-bearing wooden gate post and a "
       "stacked alabaster pedestal share only the word 'support'."),

F("trestle.splayed_board_standard", "component_recipe", "post",
  "A splayed flat BOARD standard at a trestle end, with applied carving that can be removed "
  "to leave a plain trestle.",
  "tapered flat board, splayed in elevation",
  "trestle foot below, top frame above",
  "tenoned into foot and top rail",
  "trestle end support",
  "carved palmette and swag as APPLIED layers",
  "height; splay angle; board width; taper",
  "square-section posts; turned supports",
  "parameterized_recipe",
  ["R124"], proven=["R124"]),

F("pier.midspan_support", "component_recipe", "post",
  "A load-bearing midspan pier under a long carcase, standing on a shelf rather than the "
  "floor.",
  "extruded prism",
  "shelf below, drawer carcase above",
  "UNRESOLVED at the shelf; joined by a cross rail to its pair",
  "midspan support for a long span",
  "plain or carved",
  "pier count along the span (drives arch count); section; height above shelf",
  "trestle standards (which reach the floor)",
  "parameterized_recipe",
  ["R125"], proven=[], unresolved=["R125"],
  note="'whether 2 further piers exist is NOT DETERMINABLE' - the count is open."),

F("support.shelf_stick", "component_recipe", "post",
  "A slender stick supporting a cantilevered shelf and landing on the SHELF BELOW rather "
  "than on the carcase - a staggered-shelf grammar.",
  "slender square prism",
  "shelf above and shelf below (never the carcase)",
  "butted/pinned at both ends",
  "carries a staggered etagere shelf",
  "plain",
  "stick section; length; horizontal offset of the shelf it carries",
  "any post that lands on the carcase or the floor",
  "parameterized_recipe",
  ["R215"], proven=["R215"],
  note="'the single most transferable idea in the object: a staggered shelf held by one "
       "thin stick.'"),

F("partition.interior_vertical", "component_recipe", "post",
  "An interior board partition dividing drawer banks. Its OFFSET sets the bank widths.",
  "flat board, case-depth",
  "case top and bottom; drawer runners fix to it",
  "housed/dadoed into top and bottom; rails tenon into it",
  "divides the interior into drawer banks",
  "secondary wood, unfinished",
  "board width (case depth); height; OFFSET from the case end - the single most retargetable "
  "parameter in the object",
  "show-face posts and muntins",
  "parameterized_recipe",
  ["R192"], proven=[], unresolved=["R192"], status="AUDIT_ONLY",
  note="AUDIT_ONLY per the final audit - zero proved construction members. "
       "v3 marked this row simultaneously proven and unresolved, which is incoherent. The "
       "housing into the case top and bottom is NOT visible in any view; what the row "
       "actually establishes is the partition's existence and its offset. Interface "
       "UNRESOLVED. (1 of 2 visible, 1 inferred by symmetry - PARTIALLY VISIBLE.)"),

# ============================================================ STILE
F("stile.continuous_leg_and_stile", "component_recipe", "rear leg/stile",
  "One stick doing leg AND stile, unbroken from floor to crest or cornice. Proved by view.",
  "extruded prism or turning, full height",
  "floor below; crest/cornice above",
  "NONE along the run; rails mortise into it",
  "corner member of the whole object",
  "reeded/gilt on show faces",
  "total height; section; taper; reed count; where the capital block lands",
  "any object where the rear support is a separate leg under a concealed frame",
  "parameterized_recipe",
  ["R095.a","R160.a","R214.a","R412"], proven=["R095.a","R160.a","R214.a","R412"],
  note="The baseline's defining trait. Note it does NOT hold on 2012.216 or 1954.151 - both "
       "explicitly contradict it, and are in the separate-leg family below."),

F("stile.separate_rear_leg", "component_recipe", "rear leg/stile",
  "The rear support is a SEPARATE leg, applied under a seat frame or below a die block. The "
  "show-wood stile above stops short and never reaches the floor.",
  "revolve or tapered prism",
  "seat frame or corner die block above",
  "dowel or tenon into the frame/die block",
  "rear floor support only",
  "may be plain where the front is decorated",
  "same turning parameters as the front leg; rake angle; whether the rear pair is shortened",
  "continuous leg-and-stile members",
  "parameterized_recipe",
  ["R025","R063","R144","R336","R095.b"],
  proven=["R025","R063","R144","R336","R095.b"],
  note="1954.151 is emphatic: 'CRITICAL DIVERGENCE FROM THE BASELINE: this is NOT a "
       "continuous rear leg-and-stile.' Two objects disprove the baseline pattern."),

F("stile.case_corner_board", "component_recipe", "rear leg/stile",
  "A thin board-like corner stile of a case, thinner than the front members - a show-face "
  "vs back-face economy rule.",
  "flat board prism, slight taper",
  "case sides and back",
  "housed or tenoned; rails mortise in",
  "case corner",
  "plain or ebonised; deliberately cheaper than the front",
  "thickness; taper; whether front and rear sections match; whether it stops at a base "
  "moulding or runs to the floor",
  "full-section structural corner posts",
  "parameterized_recipe",
  ["R359","R377","R376"], proven=["R359","R376"], unresolved=["R377"],
  note="R376 arrives here from the deleted family termination.stile_to_floor. Continuing "
       "past the lower rail to the floor is a TERMINATION SETTING on this recipe - the "
       "variation axis already reads 'whether it stops at a base moulding or runs to the "
       "floor' - not a second component. 1971.281 supplies both the stile (R359) and the "
       "proof of the floor setting (R376), so they are one part."),

F("board.plain_back", "component_recipe", "rear leg/stile",
  "A plain board closing the back, shaped only by a functional notch. Deliberately undecorated "
  "because it is never seen.",
  "flat board with a V-notch",
  "case sides",
  "nailed or housed",
  "closes the back; provides floor bearing at the rear",
  "unfinished secondary wood",
  "board width; notch depth and shape",
  "show-face stiles",
  "parameterized_recipe",
  ["R295"], proven=["R295"],
  note="'deliberately plain because the rear is unseen, in direct contrast to the turned "
       "front feet' - the visibility-economy rule stated outright."),

F("stile.continuity_unresolved", "component_recipe", "rear leg/stile",
  "A rear member that LOOKS continuous but whose junction is masked. Continuity is the "
  "baseline's defining trait and here it is asserted, not seen.",
  "prism above, turning below - alignment visible, junction not",
  "UNRESOLVED",
  "UNRESOLVED - masked by the seat rail and gilding",
  "rear corner member",
  "gilt with a recessed matte field",
  "rake angle; length below seat; recessed field dimensions",
  "members where continuity is proved either way",
  "parameterized_recipe",
  ["R001"], proven=[], unresolved=["R001"], status="AUDIT_ONLY",
  note="AUDIT_ONLY. THE BASELINE OBJECT'S OWN defining trait is unproven on the baseline object: "
       "'continuity is asserted only as plausible, not seen.'"),

# ============================================================ RAIL
F("seatframe.closed_frame_assembly", "assembly_grammar", "rail",
  "A closed rectangle of four rails that must exist for a seat to be framed. This is the "
  "OWNERSHIP rule that says the individual rail recipes below compose into one frame - it "
  "is not itself a part to model.",
  "composition of four rail members",
  "leg or post at each corner",
  "tenon, pin or mitre at four corners - varies by member family",
  "the seat frame as a unit",
  "n/a - the members carry the surface",
  "plan aspect; rail depth; straight vs shaped vs barrel plan",
  "n/a",
  "assembly_grammar",
  ["R066.a","R066.b","R097","R052.a","R003","R004"],
  proven=["R066.a","R066.b","R097","R003"], unresolved=["R052.a","R004"],
  status="RANKABLE",
  note="Per the audit, rail.seat_frame is an assembly grammar, and the leaf recipes (show "
       "face / secondary / mitred / pinned) are separate families below."),

F("rail.seat_tenoned", "component_recipe", "rail",
  "The tenoned seat rail. ONE recipe: front, side and rear are the same member with the same "
  "joint, differing only in which faces get a finish and how deep the rail runs.",
  "extruded rectangular section, optionally scalloped on the lower edge",
  "legs or posts at each corner",
  "tenoned into posts and leg blocks",
  "front / side / rear leaf of a closed seat frame - the ROLE lives in the assembly schedule, "
  "not in a separate recipe",
  "SURFACE IS A PARAMETER: gilt show face with paterae and a bead arris, or bare secondary "
  "wood, or upholstered over",
  "length; depth (deeper where the cover wraps over it); scalloped vs straight lower edge; "
  "paterae pitch; PER-FACE finish flag; role (front/side/rear)",
  "pinned/pegged rails, which are a different joint schedule; mitred box rails, which have no "
  "tenon at all",
  "parameterized_recipe",
  ["R003","R066.a","R066.b","R004"], proven=["R003","R066.a","R066.b"],
  unresolved=["R004"],
  note="MERGED in v3.1. v3 split these into rail.seat_show_face and "
       "rail.seat_secondary_concealed, but that split is FINISH, not construction - the "
       "geometry and the tenon are identical and 2011.3 proves it by carrying both states on "
       "one frame ('Front and side rails are gilt show faces carrying paterae; the rear rail "
       "is left in bare wood'). Decoration is a per-face flag, which is a parameter."),

F("rail.seat_pinned", "component_recipe", "rail",
  "A seat rail whose joints are PINNED with visible round pegs - a different joint schedule "
  "from a glued tenon, and visible on the finished object.",
  "extruded rectangular section, plain",
  "legs or posts at each corner",
  "tenoned AND PINNED with visible round pegs through the joint",
  "leaf of a closed seat frame",
  "plain secondary wood; the pegs read on the show face",
  "length; depth; peg count and position; straight vs serpentine front edge in plan",
  "glued tenoned rails - the peg array is real geometry and changes the joint contract",
  "parameterized_recipe",
  ["R097"], proven=["R097"],
  note="Split from the merged tenoned recipe per the audit: 'joints pinned with visible "
       "round pegs' is a different joint schedule AND a visible surface event."),

F("rail.seat_mitred_box", "component_recipe", "rail",
  "Four flat boards MITRED into a closed rectangular box with no separate apron member. The "
  "mitre is the joint contract and it excludes tenoning.",
  "flat board, mitred at 45 degrees at the corners",
  "column tops or leg dowels at the corners",
  "mitre, reinforced internally (block or leg dowel - not resolvable)",
  "seat/frieze box",
  "relief on the long faces, painted row on the short faces",
  "rail height; plan aspect; ornament assignment PER FACE",
  "tenoned rail frames",
  "parameterized_recipe",
  ["R052.a"], proven=[], unresolved=["R052.a"],
  note="The corner reinforcement is inferred, not seen - see the corresponding unresolved "
       "brace row."),

F("rail.drawer_divider", "component_recipe", "rail",
  "A horizontal rail that separates and carries drawers. Its receiver schedule is what "
  "defines it: it tenons into the corner posts AND into the interior partitions, and it "
  "takes the drawer runners.",
  "extruded rectangular section",
  "corner posts and interior partitions",
  "tenoned into posts and partitions; receives drawer runners; may carry a lock keeper",
  "divides a drawer stack and carries its load",
  "reeded or moulded front edge",
  "length; height; COUNT (drives drawer count); front-edge profile; keeper plate presence",
  "panel-frame stock (spans openings, carries nothing); frieze and plinth rails (no drawer "
  "load, different receiver); back rails (secondary wood, no show face)",
  "parameterized_recipe",
  ["R163","R194","R216.b","R247.d"], proven=["R163","R194"],
  unresolved=["R216.b","R247.d"],
  note="Split out of v3's rail.case_divider, which combined drawer dividers, panel-frame "
       "stock, frieze rails, plinth rails, back rails, gate stock and arched rails into one "
       "family on the strength of all being horizontal. R216.b (rails above and below the "
       "drawers) and R247.d (the single rail above the drawers, restored by the final "
       "audit) have a clear drawer-bounding ROLE but unproved receiver schedules - members, "
       "not proof."),

F("rail.panel_frame_stock", "component_recipe", "rail",
  "Generic frame stock whose job is to SPAN panel openings. It carries no drawer and no "
  "applied show programme - the panel count it spans is the parameter.",
  "extruded rectangular section, grooved on one edge",
  "stiles and muntins of the same frame",
  "tenoned into stiles; grooved to capture the panel",
  "horizontal member of a frame-and-panel assembly",
  "plain; peg pattern visible",
  "length; width; how many panel openings the rail spans; peg pattern",
  "drawer dividers (carry load, different receiver); frieze/plinth rails",
  "parameterized_recipe",
  ["R271"], proven=["R271"]),

F("rail.frieze", "component_recipe", "rail",
  "The upper rail of a case front, under the cornice. Carries the show programme and may be "
  "arched on its lower edge.",
  "extruded section, lower edge straight, arched or scalloped",
  "corner posts or stiles",
  "tenoned into posts; the cornice sits on it",
  "top rail of a case front",
  "marquetry on 69.140ab; arched openings on 1999.79",
  "span; depth; straight vs arched vs scalloped lower edge",
  "drawer dividers; plinth rails at the base; back rails",
  "parameterized_recipe",
  ["R216.a","R247.a"], proven=["R247.a"], unresolved=["R216.a"],
  note="R216 was split by role in the final patch; only its arched frieze rails (.a) belong "
       "here. The drawer-bounding rails went to rail.drawer_divider and the centre-section "
       "rails to the AUDIT_ONLY bucket - assigning the aggregate here claimed a frieze role "
       "for rails the source distinguishes."),

F("rail.plinth", "component_recipe", "rail",
  "The base rail of a case, carrying the reeded band and the load down to the feet.",
  "extruded section",
  "corner posts; feet or plinth blocks below",
  "tenoned into posts; transfers load to the feet",
  "base rail",
  "reeded band",
  "span; depth; band reed count",
  "frieze rails at the top of the same case - different load path and different treatment",
  "parameterized_recipe",
  ["R247.b"], proven=["R247.b"]),

F("rail.back_secondary", "component_recipe", "rail",
  "A plain secondary-wood rail closing the back. No show face, cheaper stock, deliberately "
  "undecorated.",
  "extruded section, plain",
  "case sides",
  "tenoned or nailed",
  "closes the back of a case",
  "plain secondary cherry - never seen",
  "span; depth",
  "any show-face rail",
  "parameterized_recipe",
  ["R247.c"], proven=["R247.c"]),

F("rail.table_frame", "component_recipe", "rail",
  "The fixed frame rails of a table - end rails and long side rails, optionally pierced for "
  "a drawer opening.",
  "extruded section, straight or shaped lower edge",
  "corner legs",
  "tenoned into the legs",
  "fixed frame of a table",
  "primary wood on show faces, secondary behind",
  "length; depth; pierced for a drawer or not; primary vs secondary wood; straight vs shaped "
  "lower edge",
  "case drawer dividers; swinging gate stock",
  "parameterized_recipe",
  ["R415"], proven=[], unresolved=["R415"], status="AUDIT_ONLY",
  note="AUDIT_ONLY per the final audit - the tenon claim is asserted from type; no joint is "
       "visible in the published views."),

F("rail.chest_lock_and_lower", "component_recipe", "rail",
  "The front rails of a chest, where the UPPER rail is deepened to take the lock.",
  "extruded section, upper deeper than lower",
  "corner stiles",
  "tenoned into the stiles; the upper rail is mortised for a lock",
  "front rails of a chest",
  "plain or shaped lower edge",
  "upper (lock) rail depth vs lower rail depth; run length; straight vs curved lower edge",
  "drawer dividers; frieze rails",
  "parameterized_recipe",
  ["R361"], proven=[], unresolved=["R361"], status="AUDIT_ONLY",
  note="AUDIT_ONLY per the final audit - the lock mortise and stile tenons are inferred "
       "from the rail depths, not seen."),

F("rail.shared_stock_multi_role", "component_recipe", "rail",
  "One stock section serving several roles on one object, distinguished only by which inlay "
  "path is run on it. The reusable fact is the STOCK, not the role.",
  "one extruded section, several path programmes",
  "posts or stiles at each end",
  "tenoned",
  "capital under-rail and drawer bottom rail on the same object",
  "different inlay band per role",
  "length; height; which inlay path is applied",
  "rails whose section is driven by a load they carry",
  "parameterized_recipe",
  ["R115"], proven=["R115"],
  note="'the capital under-rail is the same stock as the drawer bottom rail with a different "
       "inlay path' - stated outright."),

F("rail.multi_purpose_unresolved", "component_recipe", "rail",
  "A rail recorded as possibly grooved, possibly rebated, possibly mortised - three "
  "different receiver schedules, none of them settled.",
  "extruded section", "UNRESOLVED", "UNRESOLVED",
  "unclassified case rail", "unresolved",
  "length; height; and the open question of which receiver schedule applies",
  "every rail whose schedule IS known",
  "parameterized_recipe",
  ["R379","R216.c"], proven=[], unresolved=["R379","R216.c"], status="AUDIT_ONLY",
  note="AUDIT_ONLY. R379 lists three mutually exclusive schedules ('whether grooved for "
       "panels, rebated for bottom boards, or mortised for a lock') and settles none. "
       "R216.c - the top and bottom rails of the centre section - records a position but no "
       "receiver schedule at all."),

F("rail.back_frame", "component_recipe", "rail",
  "Top and bottom rails bounding an upholstered back opening between the rear posts.",
  "extruded section; the top rail may be widened to carry a carved frieze",
  "rear posts at each end",
  "tenoned into the rear posts",
  "bounds the back opening",
  "carved frieze on the front face of the top rail; plain behind",
  "opening height; bay count; frieze repeat count",
  "seat rails; crest boards sawn to a silhouette",
  "parameterized_recipe",
  ["R067","R068"], proven=["R067","R068"],
  note="'the same rail with the ornament stripped is a plain crest rail' - ornament is a "
       "separable layer on this member."),

F("rail.upholstery_tacking", "component_recipe", "rail",
  "A rail existing PURELY as a tacking ground for a cover. No show face, no structural span "
  "claim - add more as the back gets taller.",
  "extruded plain section, lapped past the post",
  "back frame posts",
  "lapped past the central back post",
  "tacking ground for the inside-back cover",
  "never seen",
  "length; height on the back frame; count",
  "structural back-frame rails",
  "parameterized_recipe",
  ["R099"], proven=["R099"]),

F("rail.curved_sawn", "component_recipe", "rail",
  "A rail BOWED IN PLAN and serpentine in elevation, sawn from a wide board. Its structural "
  "core is bare; all ornament is applied to the outer face.",
  "sawn from a wide board along two curves (plan bow + elevation serpentine)",
  "posts at each end",
  "OUT OF SCOPE - the end interface is unevidenced and is NOT part of this recipe; a "
  "consumer must supply its own end joint",
  "long side rail of a bed",
  "gilt ornament applied to the outer face only; core left bare",
  "length; bow radius in plan; serpentine sag depth at mid-span",
  "straight rails - a straight member cannot be stretched into this and the stock width "
  "requirement is completely different",
  "parameterized_recipe",
  ["R338.a"], proven=["R338.a"],
  note="GEOMETRY-ONLY per the final audit. R338.a proves the curved SAWN FORM - bowed in "
       "plan, serpentine in elevation, sawn from wide stock - and nothing about the end "
       "joint. 'Knockdown or fixed' was removed as a variation axis: it is not a parameter "
       "of this recipe, it is an unevidenced fact about a different contract (R338.b, "
       "quarantined). What this family delivers is the curve; the interface is external."),

F("rail.straight_visibility_switched", "component_recipe", "rail",
  "A straight rail whose FINISH is switched by visibility: the same member is plain "
  "unfinished wood at the unseen end and fully decorated at the seen end.",
  "extruded straight section",
  "posts at each end",
  "tenoned post to post",
  "closes the base of an end frame; rear rail of a case",
  "plain unfinished OR full gilt apron - a switch, not a different part",
  "length; finished-vs-plain treatment as a visibility-driven flag",
  "curved sawn rails",
  "parameterized_recipe",
  ["R339","R146"], proven=["R339","R146"],
  note="1954.151 proves it on ONE object: head-end version plain on both faces, foot-end "
       "version carries the full gilt apron. Same rail, one boolean."),

F("rail.gallery_spindle_mortised", "component_recipe", "rail",
  "A gallery base rail MORTISED at pitch to receive spindles. The mortise pitch is the "
  "interface and no other gallery rail has it.",
  "extruded section with a mortise array",
  "posts or frame at each end",
  "mortise array at fixed pitch for spindle tenons",
  "seats a spindle gallery",
  "plain or moulded",
  "length; MORTISE PITCH (drives spindle count)",
  "gallery cap rails and inlaid band rails - neither carries a mortise array",
  "parameterized_recipe",
  ["R310"], proven=["R310"],
  note="Per the audit: gallery base, cap and top rails are split because their interface "
       "schedules are NOT proven equivalent. This one's schedule is a mortise array."),

F("rail.gallery_cap", "component_recipe", "rail",
  "The rail capping a gallery. Receives spindle tops; its own face may carry a bead run.",
  "extruded section, optionally with a bead run",
  "spindle tops below",
  "receives spindle tenons from below",
  "caps a gallery",
  "bead run on the face, or plain",
  "length; profile depth; whether it carries a bead run",
  "the mortised base rail; inlaid band rails",
  "parameterized_recipe",
  ["R311","R313"], proven=["R311","R313"]),

F("rail.inlaid_band", "component_recipe", "rail",
  "A rail whose defining feature is a stretchable inlaid band with FIXED-WIDTH terminal "
  "cells - a two-part layout that generalises to any long band.",
  "extruded section; the inlay is a separate path stretched to the rail length",
  "posts or frame at each end",
  "tenoned",
  "show rail of a headboard/footboard or bed side",
  "marquetry band: fixed terminal cells + stretchable centre",
  "length; moulding member count on the edges; whether the centre medallion is present",
  "gallery rails with spindle interfaces",
  "parameterized_recipe",
  ["R309","R312","R308"], proven=["R309","R312","R308"],
  note="'the centre ribbon-knot is a fixed medallion with a stretchable scroll to either "
       "side' - a genuinely reusable layout rule."),

F("arm.round_reeded_rod", "component_recipe", "rail",
  "A reeded CYLINDRICAL rod running fore-aft with a carved terminal. A solid of revolution "
  "with a swept reed - not a sawn board.",
  "revolve + reed sweep; carved terminal master at the front",
  "arm post below at the front; rear post behind",
  "dies into a carved bracket at the rear post",
  "arm",
  "reeded, gilt; ram's-head/lion-head/volute terminal as a swappable master",
  "length; reed count; terminal master choice",
  "flat swept arm boards - a board and a rod share only the centre path",
  "parameterized_recipe",
  ["R069"], proven=["R069"],
  note="Per the audit: split from flat swept-board arms unless only the centre-path "
       "vocabulary is shared. Here only the centre path is shared."),

F("arm.flat_swept_board", "component_recipe", "rail",
  "One sawn, COMPOUND-CURVED board doing the whole arm, terminating in a cyma scroll. "
  "Mirror-symmetric pair from one profile.",
  "sawn board along a compound curve; cyma scroll at the terminal",
  "arm posts below; back frame behind",
  "lapped or tenoned onto the arm posts",
  "arm",
  "show wood",
  "scroll tightness; flare angle; board thickness; arm height above seat",
  "round reeded rods",
  "shared_profile",
  ["R098"], proven=["R098"]),

F("arm.arcaded_assembly", "assembly_grammar", "rail",
  "An arm rail whose length drives ARCH COUNT in lockstep with spindle count - the arcade is "
  "a composition rule, not a single member.",
  "rail + repeated arch/spindle cells",
  "seat rail below via posts",
  "spindles tenon between seat rail and arm rail",
  "arm arcade",
  "gilt; top face padded or bare",
  "length -> arch count (5 arches over 4 spindles + 1 post here); arch springing height; "
  "whether the top face is padded",
  "single-member arms",
  "assembly_grammar",
  ["R006"], proven=["R006"],
  note="'the single most transplantable assembly on the object.'"),

F("flyrail.moving_assembly", "assembly_grammar", "rail",
  "A MOVING assembly: swing rail + pivot + swing leg + a stop that is not resolvable. It is "
  "one mechanism and must be authored as one, with the stop left open.",
  "rail + pivot joint + leg, with a motion range",
  "rear rail of the carcase",
  "PIVOT (knuckle or pin) at the inboard end; the leg is a standard leg blank with an "
  "extended UNFINISHED upper post because it is concealed when closed",
  "supports a drop leaf when open; nests against the rear rail when closed",
  "show wood on the leg; raw saw-marked concealed extension",
  "swing length; pivot offset from the rail end; leg blank parameters; concealed-extension "
  "length",
  "static rails and static legs",
  "assembly_grammar",
  ["R147","R145"], proven=["R147","R145"],
  note="v3 marked R147 BOTH proven and unresolved - the fail-closed check in v3.1 caught "
       "it. The rail and its pivot ARE visible (alt7 shows it extended at 90 degrees "
       "carrying the swing leg); what is unresolved is the STOP, which has no BOM row of its "
       "own and is therefore recorded here as a family-level open question rather than as a "
       "penalty on a row that is actually well evidenced. "
       "Per the audit, authored as rail + pivot + support + UNRESOLVED STOP. No view "
       "resolves what limits the swing at 90 degrees. The 'concealed extension' parameter "
       "generalises to any moving member."),

F("rail.gate_swinging", "component_recipe", "rail",
  "A flat board rail that SWINGS with a gate and carries pins along its face.",
  "flat board with shaped pivot end and a pin array",
  "gate post pivot",
  "pivot at one end; pins along the face",
  "upper member of a swinging gate",
  "plain",
  "length (sets how far under the leaf the gate reaches); thickness; pivot-end shaping; pin "
  "count",
  "fixed frame rails",
  "parameterized_recipe",
  ["R416"], proven=["R416"]),

# ============================================================ BRACE
F("brace.corner_glue_block", "component_recipe", "brace",
  "A triangular or quarter block glued into an inside frame corner. ONLY rows where a block "
  "was actually seen are members.",
  "prism, two mating faces cut to the frame angle",
  "two inside rail faces at a corner",
  "glued; one instance mates to a CURVED end apron, so the recipe needs a "
  "'seat against a swept surface' parameter",
  "stiffens a frame corner",
  "none - never seen in the finished object",
  "block section; leg length; chamfer; flat-vs-swept mating face",
  "every INFERRED block (47, 53, 139, 241) - they carry zero weight and may not establish "
  "this family's joint; the case-base locator, which does a different job",
  "parameterized_recipe",
  ["R102","R149"], proven=["R102","R149"],
  note="Per the audit, exactly the two EVIDENCED rows. v2 carried 7 members of which 4 were "
       "inferred; this was the single worst evidence dilution in v2."),

F("locator.case_base_block", "component_recipe", "brace",
  "A block at the base corners that LOCATES a case sitting loose on top. It positions, it "
  "does not stiffen.",
  "prism at an angle",
  "base top surface; the loose case above",
  "glued to the base; the case above simply sits within them",
  "locates a removable upper case",
  "none",
  "block length and angle",
  "corner glue blocks, which reinforce a joint rather than locate a separate case",
  "parameterized_recipe",
  ["R293"], proven=["R293"],
  note="Moved out of the glue-block family per the audit: 'these sit at the base corners and "
       "appear to locate the case that sits loose on top.'"),

F("stay.metal_quadrant_visible", "component_recipe", "brace",
  "A brass quadrant stay / lopper arm limiting a fall front. Visible, metal, and its upper "
  "end slides in a slot.",
  "swept arc strap, constant section",
  "carcass stile (slot) and fall front (fixed end)",
  "slides in a slot in the carcass stile, or is pinned",
  "limits and supports a falling front",
  "brass",
  "arc radius and chord length (set by fall depth); strap width; sliding vs pinned upper end",
  "unseen axial tie rods - opposite evidence status and opposite construction",
  "parameterized_recipe",
  ["R168"], proven=["R168"],
  note="Per the audit: visible quadrant metal stays split from unseen axial tie rods."),

F("bracket.shaped_sawn_console", "component_recipe", "brace",
  "A shaped sawn bracket/console whose 2D profile is swept to a constant thickness. One "
  "curve asset, several length settings.",
  "2D cyma/ogee profile swept to constant thickness",
  "post and the member it braces",
  "let in, screwed, or pegged; 1954.151 adds a let-in iron keeper plate",
  "ties a crest or canopy down to a post; gusset between arm rail and stile",
  "gilt or matched to the frame; may be pierced",
  "arc radius; span; rise; thickness; hole count and position; piercing radius",
  "glue blocks (unseen, structural-only)",
  "shared_profile",
  ["R341","R305","R010"], proven=["R341","R305","R010"],
  note="'The same curve appears again at the headboard ends, so it is ONE curve asset with "
       "two length settings.'"),

F("cleat.lid_batten", "component_recipe", "brace",
  "An end cleat battened across a lid underside to resist cupping. Fixing type doubles as an "
  "age/repair storytelling axis.",
  "tapered batten with chamfered ends",
  "lid underside",
  "pegged, nailed or screwed - the choice is the period/repair signal",
  "stiffens a lid",
  "may be moulded to match the lid edge; or pale unpatinated modern wood",
  "cleat width; taper of the chamfered ends; fixing pattern",
  "structural frame braces",
  "parameterized_recipe",
  ["R370","R387"], proven=["R370","R387"],
  note="1971.281's cleats are explicitly a LATER REPAIR in pale unpatinated wood with modern "
       "slotted screws - a ready-made 'this has been repaired' variant."),

F("plate.pegged_repair", "component_recipe", "brace",
  "A flat plate laid over a failed joint and pegged at its corners - a shop-level repair, "
  "not original construction.",
  "flat rectangular plate with a peg array",
  "outside face of a rail over a leg tenon",
  "four round pegs at the corners",
  "reinforces a failed tenon",
  "bare wood, contrasting with the finished frame",
  "plate outline; peg count and pattern",
  "original structural members",
  "parameterized_recipe",
  ["R103"], proven=["R103"],
  note="'Extremely valuable as a this-frame-has-been-repaired decal for aged furniture "
       "variants.'"),

F("brace.seat_internal_grid", "assembly_grammar", "brace",
  "An internal grid inside a seat rectangle: one longitudinal member crossed by front-to-back "
  "sticks. Spacing is a function of seat length.",
  "longitudinal member + crossing sticks, some straight and some sawn to a shallow arc",
  "inside faces of the seat rails",
  "lapped over or tenoned into the rails",
  "supports the seat platform over a long span",
  "never seen",
  "brace spacing/count as a function of seat length; straight vs arced stick; lapped vs "
  "tenoned",
  "corner blocks",
  "assembly_grammar",
  ["R101"], proven=["R101"]),

# ============================================================ FRAME
F("frame.stile_rail_muntin_grid", "assembly_grammar", "panel frame",
  "Structural joinery: stiles + rails + muntins, grooved to CAPTURE a floating panel. The "
  "grammar is the asset, not any one frame.",
  "grid of grooved members",
  "carcase or door opening",
  "mortise-and-tenon, often PEGGED; panels float in grooves, never glued",
  "structural side, back or door of a case",
  "moulded lip on the inner edge; may receive applied ornament",
  "bay count; bay aspect ratio; stile/rail widths; groove depth; muntin count",
  "applied moulded surrounds, which are trim on a board and hold nothing; the unresolved "
  "69.140ab case sides",
  "assembly_grammar",
  ["R197","R362","R380","R164","R272","R130"],
  proven=["R197","R362","R272"], unresolved=["R380","R164","R130"],
  note="'the grammar (stiles + rails + muntins + captured panels) is the reusable object, "
       "not the specific frame.'"),

F("frame.applied_moulded_surround", "component_recipe", "panel frame",
  "Applied trim MITRED around an aperture on a board. It captures nothing and carries no "
  "load - the opposite construction to a joined frame.",
  "moulded profile mitred into a closed loop",
  "the face of a board or carcase front",
  "MITRE at the corners; glued to the ground",
  "dresses an aperture",
  "concentric stepped/reeded margins; ovolo/cove stacks",
  "aperture size; number of concentric steps; reed pitch; step count in the profile stack",
  "structural stile-rail-muntin grids",
  "parameterized_recipe",
  ["R218","R316","R071"], proven=["R218","R316","R071"],
  note="Kept separate from the joined grid per the audit, membership corrected. The mitre is "
       "the discriminator: joined frames tenon, applied surrounds mitre."),

F("enclosure.rail_and_bracket", "assembly_grammar", "panel frame",
  "A four-sided enclosure made of rail / rail / bracket / bracket around a pierced field - "
  "NOT a stile-and-rail frame and not applied trim.",
  "two horizontal rails closed at each end by a shaped bracket",
  "arm assembly of a chair",
  "rails tenon to the brackets; the pierced field is captured between",
  "encloses a pierced lattice field under an arm",
  "gilt; winged monopodium brackets",
  "field length; field height; bracket master; lattice cell count",
  "both frame families - it has no stiles and no mitred trim",
  "assembly_grammar",
  ["R030"], proven=["R030"],
  note="Created per the audit as its own enclosure for FC-01-2012_216. The source calls it "
       "'the single most transferable assembly rule on the object'."),

# ============================================================ PANEL
F("panel.floating_wood_in_groove", "component_recipe", "floating panel",
  "A wooden board floating free in the surrounding frame's grooves - never glued, so it can "
  "move. The BLANK is separable from whatever is done to its face.",
  "flat board blank, optionally bevel-raised (fielded)",
  "grooves of the surrounding frame",
  "floats in the groove; NO glue - that is the defining constraint",
  "fills a frame opening",
  "plain, carved, marquetried, fielded or glazed - all are face programs on one blank",
  "aspect ratio; thickness; bevel/fielding depth; rectangular vs shouldered field; face "
  "program",
  "upholstered fields, which are a padded volume with no groove interface",
  "parameterized_recipe",
  ["R363","R198"], proven=["R363","R198"],
  note="REPAIRED TWICE. v3 admitted 11 members on asserted grooves; v3.1 kept four; the "
       "final audit cut two more. Only TWO rows prove the exact groove/captured-panel "
       "contract: R363 GROOVE STATED ('a plain, a carved, and a pierced variant all share "
       "one GROOVE-SET blank') and R198, whose companion R197 records the full grammar WITH "
       "groove depth ('stiles + rails + muntins + captured panels', 'groove depth'). R273 "
       "and R381 are quarantined as frame_layout_only: their companions (R272, R380) prove "
       "frame layout, panel count and peg pattern - never the concealed capture. Weighted "
       "evidence 10.0 -> 4.0 -> 2.0 across the three passes. Each drop is a correction."),

F("panel.upholstered_field", "component_recipe", "floating panel",
  "A padded volume dropped into a frame rebate. Same SLOT as a wooden panel, entirely "
  "different construction - no groove, no wood movement, a crown instead of a thickness.",
  "padded volume with a crowned surface",
  "frame rebate; tacked to the frame or to webbing",
  "tacked, not captured - there is no groove",
  "fills a frame opening with textile",
  "silk, gimp, fringe",
  "field crown/loft; edge radius; cover material",
  "wooden floating panels",
  "parameterized_recipe",
  ["R012","R041"], proven=["R012","R041"],
  note="Preserved from v2 - the audit confirms this distinction was correct. 'a clean "
       "demonstration that the frame and the field are separable slots.'"),

# ============================================================ CREST / CORNICE
F("cornice.stacked_profile_run", "component_recipe", "crest/cornice",
  "A cornice built as a STACK of swept profile members, mitred at the corners and returning "
  "on the visible sides.",
  "stacked swept profiles along a rectangular path",
  "top of a case or tester",
  "mitred returns at the corners; dies into the posts",
  "crowns a case",
  "fillet/cove/ogee stack, optionally with a dentil course",
  "stack depth; fillet count; projection; whether it returns on the sides only or wraps; "
  "dentil pitch",
  "shaped sawn crest boards, which are a silhouette cut from a board, not a swept stack",
  "shared_profile",
  ["R158","R190","R246","R282","R302","R231.a"],
  proven=["R158","R190","R246","R282","R302","R231.a"],
  note="Preserved from v2; the audit confirms stacked cornice vs shaped sawn crest is a real "
       "distinction."),

F("crest.shaped_sawn_board", "component_recipe", "crest/cornice",
  "A single board SAWN to a silhouette, carved on the front face only. The rear proves it: "
  "a plain sawn board with a screw and peg line.",
  "2D silhouette curve cut from a board",
  "top rail of the back or case",
  "screwed and pegged to the top rail",
  "crowns a chair back or case front",
  "relief carving on the FRONT ONLY",
  "pitch/span; lobe count (scales with seat count); lobe amplitude; end sweep; relief content",
  "stacked cornice runs; laminated curved crests",
  "shared_profile",
  ["R070","R100","R231.b"], proven=["R070","R100","R231.b"],
  note="'the rear views prove the back face is a plain sawn board with a screw and peg line "
       "where it fixes to the top rail.' The top-edge curve is the transferable asset."),

F("crest.laminated_curved", "component_recipe", "crest/cornice",
  "An arched crest BUILT UP from laminated curved segments - lamination lines are visible. "
  "It cannot be cut from one board, which is the whole point.",
  "laminated curved segments following an arc",
  "post caps at each end",
  "stepped down through an ogee shoulder to the post cap",
  "arched cresting of a bed end",
  "applied garland on the face",
  "arch span; arch rise; shoulder step height; garland density",
  "single-board sawn crests - the stock requirement is completely different",
  "parameterized_recipe",
  ["R340"], proven=["R340"],
  note="Split from the sawn-board family on construction: 'built up from laminated curved "
       "segments (lamination lines visible in alt9/alt10/alt11)'."),

F("crest.rolled_carved", "component_recipe", "crest/cornice",
  "A crest that ROLLS backward over the top of the back and terminates in a carved spiral at "
  "each corner - a swept roll, not a flat silhouette.",
  "swept roll section along the back's top curve, with a carved volute terminal master",
  "top of the back frame",
  "continuous with the back frame stiles",
  "crowns a chair back",
  "gilt, carved spirals",
  "roll radius; spiral tightness at each end",
  "flat sawn crest boards",
  "shared_profile",
  ["R013"], proven=["R013"]),

F("crest.bent_moulded_rail", "component_recipe", "crest/cornice",
  "A moulded top rail bent to a flattened D-curve in plan, with ASYMMETRIC ornament: plain "
  "outside, decorated inside.",
  "moulded section swept along a flattened D-curve",
  "back frame stiles",
  "continuous with the stiles",
  "wraps and crowns a barrel-plan back",
  "plain outside face; reeded and disc-inlaid inside face",
  "corner radius; crown height; arc width; the outside/inside ornament asymmetry",
  "sawn silhouette crests",
  "shared_profile",
  ["R029"], proven=["R029"],
  note="The inside/outside asymmetry rule is itself reusable and is recorded as a variation "
       "axis rather than a separate family."),

F("capital.metal_assembly", "assembly_grammar", "crest/cornice",
  "A metal capital that is itself a small assembly grammar - a bell plus swap-in ornament "
  "slots - explicitly 'not one blob'.",
  "bell profile + corner volute masses + central mask slot + cresting slot",
  "top of the shaft; abacus above",
  "seated/threaded on the shaft",
  "crowns a pedestal",
  "gilt brass, fluted/reeded bell, cabled torus",
  "bell profile; which ornament fills each slot; volute mass scale",
  "wooden carved capitals",
  "assembly_grammar",
  ["R441"], proven=["R441"]),

# ============================================================ MOULDING
F("moulding.profile_toolkit", "toolkit_capability", "moulding run",
  "NOT one moulding asset. A PROFILING TOOLKIT: a library of profile atoms, the stacks built "
  "from them, the paths they sweep along, and the rules for mitres, stopped ends and "
  "junctions. Objects state repeatedly that ONE section serves several roles at several "
  "scales, which is what makes this a capability rather than a part.",
  "sweep of a 2D section along an arbitrary path; sections stack",
  "any edge, aperture or run on any object",
  "mitre at corners; stopped ends; dies into posts; returns on visible faces",
  "all linear detail",
  "the section IS the surface event",
  "PROFILE ATOMS (observed: fillet, ovolo, cavetto, ogee, astragal/bead, thumbnail, "
  "chamfer, reed, flute, bolection); STACK depth and member count; PATH (straight, "
  "rectangular loop, serpentine, arc); MITRE vs STOPPED END; junction/return rule; scale",
  "repeated-cell runs, where a discrete cell is instanced rather than a section swept - "
  "that is a different generator",
  "toolkit_capability",
  ["R014","R081","R112","R155","R182","R195","R232","R250","R284","R344","R365","R384",
   "R409","R036"],
  proven=["R014","R081","R112","R182","R195","R232","R250","R284","R344","R384","R409","R036"],
  unresolved=["R155","R365"],
  note="Re-described as a toolkit per the audit. The corpus states the reuse explicitly: "
       "'one profile serves drawer surrounds, side-panel frames and the plinth at different "
       "scales' (69.146.2); 'the same few sections repeat as door-panel bolection, pilaster "
       "cross-blocks, base cap, and base plinth' (2019.59); 'Sectioning the profiles once "
       "and sweeping them along arbitrary rectangles covers essentially all of this object's "
       "linear detail' (69.146.3)."),

F("moulding.repeating_cell_run", "component_recipe", "moulding run",
  "A discrete carved or cast CELL instanced along a path at a pitch. The generator is "
  "instancing, not sweeping - a constant-section sweep cannot produce it.",
  "instance a cell master along a path at a pitch",
  "any edge or drum",
  "carved integral or applied as a sleeve",
  "enriched edge treatment",
  "bead-and-reel, rosette-chain, lappet registers",
  "run length; cell pitch; cell master choice; instance count around a circumference",
  "constant-profile sweeps",
  "parameterized_recipe",
  ["R319","R433"], proven=["R319","R433"],
  note="Preserved from v2 - the audit confirms constant-profile sweep vs repeated-cell run "
       "is a real distinction. 2002.298.1 gives the strongest exact-reuse evidence in the "
       "whole corpus: 'The upper and lower sleeves are the SAME part used twice.'"),

F("ring.revolved_collar", "component_recipe", "moulding run",
  "A plain revolved ring clamping the edge of a band or sleeve. One shape, one parameter.",
  "revolve of a small section",
  "drum rims and band edges",
  "clamped/seated on the drum",
  "closes a joint between stacked drums",
  "gilt",
  "diameter",
  "swept runs along open paths",
  "parameterized_recipe",
  ["R434"], proven=["R434"], uncert={"R434": "quantity"},
  note="'At least 8 are visible, but individual rings merge visually with the adjacent drum "
       "rims at every published resolution, so NO HONEST COUNT EXISTS.'"),

F("collar.cut_in_solid_stone", "component_recipe", "moulding run",
  "A reeded collar cut IN THE STONE itself, with no metal ring at all. It proves the jewel "
  "bands are not all metal, so it is a construction fact as well as a part.",
  "reeds cut into the stone drum",
  "the shaft itself - subtractive, not applied",
  "none - it is the shaft",
  "mid-shaft register",
  "polished alabaster, sunken jewelled register",
  "reed count; register depth",
  "applied gilt collars - opposite construction (subtractive vs applied)",
  "parameterized_recipe",
  ["R435"], proven=["R435"],
  note="'Important construction fact: it proves the jewel bands are not all metal.'"),

# ============================================================ ORNAMENT
F("ornament.placement_system", "assembly_grammar", "repeated ornament master",
  "The PLACEMENT AND INSTANCING SYSTEM ONLY - path pitch, ring array, mirroring and graded "
  "scatter. It owns no geometry. Every actual relief master is its own part identity in the "
  "relief.* families generated below.",
  "a placement rule applied to a master supplied from elsewhere",
  "any show face", "n/a - the hosted master carries its own interface",
  "distributes masters over a surface or run", "n/a - the master carries the surface",
  "path pitch; ring count; instance scale ramp along a run; mirroring; GRADED FALL (dense at "
  "the top, thinning downward, lower third empty on tall panels - 69.140ab)",
  "all relief geometry - v3 merged 17 unrelated masters into this family purely because they "
  "all use transforms, which is not a shared construction",
  "assembly_grammar",
  ["R078","R079","R383"], proven=["R078","R079","R383"],
  note="REBUILT in v3.1. v3's ornament.instanced_relief_master held 17 members and was the "
       "assembly-lane leader at score 22.59 on the strength of an argument that amounts to "
       "'these all get placed somewhere'. The three members kept here are the rows that "
       "actually record a PLACEMENT RULE as the reusable thing (paterae counted per rail "
       "face; palmettes at a measured ~71px pitch; blocks swapped within one layout slot). "
       "Everything else moved to its own master."),

F("ornament.applied_screwed_garland", "component_recipe", "repeated ornament master",
  "A separately carved garland SCREWED to a plain sawn backing. The interface is proved by a "
  "photograph of the back - the clearest applied-ornament evidence in the corpus.",
  "carved swag master, hung along a path",
  "a plain sawn backing board",
  "SCREWED - seen from behind in alt6",
  "applied surface enrichment",
  "carved and gilt rose-and-leaf",
  "swag length; sag depth; flower density; hung as a drop, a spiral or a continuous run",
  "ornament carved integral to its ground",
  "parameterized_recipe",
  ["R356"], proven=["R356"],
  note="'alt6 shows one from behind: a separately carved element SCREWED to a plain sawn "
       "backing, which is exactly the applied-ornament workflow to copy.'"),

F("ornament.carved_in_solid_face", "component_recipe", "repeated ornament master",
  "Ornament cut INTO the parent member's own face. There is no separate part and no "
  "interface - modelling it as an applied master is the error.",
  "subtractive carving on the parent face",
  "the parent member itself",
  "none - it is the parent",
  "surface enrichment integral to a panel or block",
  "linenfold folds; wreath rings",
  "fold width; fold count per panel; terminal motif; bead pitch; density from austere 2-fold "
  "to crowded 5-fold",
  "applied masters with a glue or screw interface",
  "parameterized_recipe",
  ["R364"], proven=["R364"],
  note="v3 merged three incompatible things here. R303.a is an APPLIED carved wreath (glued "
       "on, not cut into the host) and R303.b is an INSET MARQUETRY CELL (a veneer cut, not "
       "geometry at all). Both are now their own families. Only R364's linenfold is actually "
       "carved into the host face."),

F("ornament.marquetry_motif_set", "component_recipe", "repeated ornament master",
  "A small SET of flat masters (blossom, leaf, stem) re-scattered to fill any field. "
  "Retargeting is by re-scattering, never by new art.",
  "flat 2D masters placed by a scatter rule with rotation and scale jitter",
  "a veneered field",
  "let into the veneer - a surface program, not a solid",
  "fills a panel or band with pattern",
  "contrasting veneers, engraved detail, dot accents",
  "instance rotation, scale and packing density; GRADED FALL (dense at top, thinning "
  "downward); field aspect ratio",
  "carved relief masters, which are solids",
  "parameterized_recipe",
  ["R184","R211","R269","R331"], proven=["R184","R211","R269","R331"],
  note="NARROWED in v3.1 to genuine reusable BOTANICAL motif sets. Removed: R154 (pure "
       "closed line-figures, no botanical master at all - the row says so), R186 (a keyhole "
       "cartouche, a functional marker not a field filler), and R114 (mixed - split into a "
       "veneer-cut star and a host-carved boss, which cannot share a recipe). "
       "69.140ab records a placement rule worth having: 'every field is a graded fall - "
       "dense at the top, thinning downward, with the lower third left empty on the tall "
       "door and side panels, while the short drawer and frieze bands run at even high "
       "density throughout.'"),

# --- bespoke heroes, split into individual masters per the audit
F("bespoke.lion_grotesque_head", "bespoke_hero", "repeated ornament master",
  "A carved lion/grotesque head, hand-authored, instanced mirror-left and mirror-right.",
  "hand-authored sculpture", "arm terminal or knee", "carved integral or applied",
  "figural accent", "carved and gilt", "scale; mirror only",
  "every other figural master - they are unrelated sculptures, not one family",
  "bespoke", ["R016"], proven=["R016"]),

F("bespoke.female_mask", "bespoke_hero", "repeated ornament master",
  "A classicising female head with fillet headdress and pendant collar on a capital face.",
  "hand-authored sculpture", "capital face", "cast integral with the capital",
  "figural accent", "gilt brass", "scale only",
  "the leonine corner mask on the SAME capital - a different sculpture",
  "bespoke", ["R442"], proven=["R442"], uncert={"R442": "view_coverage"},
  note="'Every published view is the same frontal aspect, so only ONE mask has ever been "
       "seen; the square abacus implies four, one per face, but that is not observed.'"),

F("bespoke.leonine_corner_mask", "bespoke_hero", "repeated ornament master",
  "A maned leonine/grotesque head at a capital corner, terminating in a scroll volute.",
  "hand-authored sculpture", "capital corner", "cast integral with the capital",
  "figural accent", "gilt brass", "scale only",
  "the female mask on the same capital",
  "bespoke", ["R443"], proven=["R443"], uncert={"R443": "view_coverage"},
  note="'Two are visible on the one photographed face; four corners are implied but not seen.'"),

F("bespoke.winged_sun_disk", "bespoke_hero", "repeated ornament master",
  "A winged sun disk with flanking uraei - drops unchanged onto any long horizontal ground.",
  "hand-authored sculpture with a span parameter", "a long horizontal ground",
  "UNRESOLVED - carved integral to the rail or applied is not stated",
  "figural frieze centrepiece", "carved/painted",
  "wingspan scales to rail length; feather-row count; disk diameter",
  "the lotus rows and petal capitals on the same object",
  "bespoke", ["R054"], proven=[], unresolved=["R054"],
  note="'the single highest-value donor element in the object' - but its interface is "
       "explicitly unresolved, so it cannot be admitted as a donor on that claim."),

F("ornament.radial_petal_array", "component_recipe", "repeated ornament master",
  "An overlapping lotus-petal bell capital. The petal profile is the generative unit and "
  "could be re-swept onto vase turnings and urn bases.",
  "one petal master arrayed radially around the capital bell", "column head",
  "bell/host attachment UNRESOLVED - the petal geometry on the bell is visible; how the "
  "bell meets the column is not evidenced", "capital", "carved",
  "petal count around the bell; petal length relative to bell height; tip sharpness",
  "the winged sun disk and painted lotus rows",
  "parameterized_recipe", ["R056"], proven=[], uncert={"R056": "construction"},
  note="Reclassified from bespoke_hero in v3.1: ONE petal master arrayed radially around a "
       "bell is a recipe, not a hero carving - 'The same petal profile is the generative "
       "unit for the capital and could be re-swept onto vase turnings, urn bases, or column "
       "feet.' Final-audit corrections: the petals are NOT claimed carved integral to the "
       "column (attachment unresolved -> construction uncertainty), and the unseen total "
       "petal count ('petals wrap the bell; only the near faces are visible') is QUANTITY "
       "doubt only - recorded here rather than as a penalty."),

F("bespoke.inlaid_urn", "bespoke_hero", "repeated ornament master",
  "A fluted, footed inlaid urn used as a composition anchor at panel centres.",
  "hand-authored 2D marquetry master", "a veneered panel centre",
  "let into the veneer", "composition anchor", "marquetry",
  "scale only - it is a signature mark, not a kit part",
  "the arcade master on the same object; the blossom motif sets",
  "bespoke", ["R187.a"], proven=["R187.a"]),

F("bespoke.inlaid_arcade", "bespoke_hero", "repeated ornament master",
  "A low colonnaded arcade on a plinth, used in place of a keyhole on the bottom drawer.",
  "hand-authored 2D marquetry master", "a drawer front",
  "let into the veneer", "signature centrepiece", "marquetry", "scale only",
  "the urn master on the same object",
  "bespoke", ["R187.b"], proven=["R187.b"],
  note="Split from the urn per the audit: 'Split unrelated figural carvings into individual "
       "bespoke masters.' The source itself calls them two one-off masters."),

F("bespoke.anthemion_cartouche", "bespoke_hero", "repeated ornament master",
  "A central carved palmette cartouche with fluted crown and four flanking S-scroll tendrils.",
  "hand-authored sculpture", "centre of an arm lattice", "applied",
  "figural centrepiece", "carved and gilt",
  "cartouche height; tendril count and curvature",
  "the disc-in-bezel master it sits among",
  "bespoke", ["R038"], proven=[], unresolved=["R038"],
  note="'Directly seen on one arm only; the second is asserted from the object's bilateral "
       "symmetry, not observed.'"),

# ============================================================ INLAY
F("inlay.linear_stringing_path", "component_recipe", "inlay path",
  "A let-in line of contrasting wood following an OFFSET of the silhouette of the face it "
  "sits on. It is an offset-inset generator, not drawn art.",
  "offset-inset of a face silhouette, swept at a constant line width",
  "a veneered or solid show face",
  "LET IN - a groove cut and filled, so it is real geometry",
  "outlines a field or panel",
  "contrasting light wood",
  "line width; inset distance from the part edge; corner radius; path outline (plain "
  "rectangle vs cusped cartouche); PATH CURVATURE (bent around ogee braces on 69.146.1); "
  "pass count per face - rear faces get zero, show faces one to two",
  "painted line-work, which cuts no geometry; engraved-and-filled stone lines",
  "parameterized_recipe",
  ["R153","R183","R210","R268","R113","R329"],
  proven=["R153","R183","R210","R268","R113","R329"],
  note="1969.262 states the generator: 'Every figure here is a single-width light line "
       "offsetting the silhouette of the face it sits on - an offset-inset generator, not "
       "drawn art.'"),

F("inlay.painted_line_path", "component_recipe", "inlay path",
  "The SAME path vocabulary executed in paint or gilding. No groove, no let-in element - "
  "the geometry is untouched and only the shader differs.",
  "the same path generator, resolved to a texture rather than geometry",
  "a painted or gilt ground",
  "NONE - it is a surface treatment",
  "outlines a field",
  "painted line, zigzag, dot banding, gilt striping",
  "line weight; colour pair; zigzag amplitude and wavelength; stripe width and gap",
  "let-in stringing, which is geometry",
  "shared_profile",
  ["R057","R235"], proven=["R057","R235"],
  note="Split from let-in stringing on construction, not on appearance. The sources "
       "themselves flag the shared vocabulary: 'the path generator is shared and only the "
       "surface shader differs.' Only the centre-path vocabulary is shared, which is exactly "
       "the audit's stated test."),

F("inlay.engraved_filled_line", "component_recipe", "inlay path",
  "Line ornament CUT INTO STONE and filled with pigment - subtractive on a mineral ground.",
  "engraved groove following a path, pigment-filled",
  "polished alabaster",
  "incised and filled",
  "wraps a drum in registers",
  "red-brown pigment in alabaster",
  "register count; motif per register; wrap circumference",
  "wood stringing and painted line-work",
  "parameterized_recipe",
  ["R437"], proven=["R437"], uncert={"R437": "quantity"},
  note="'At least 4 distinct registers, but each wraps 360 degrees and only ~half is ever "
       "visible' - the count is bounded below only."),

F("inlay.node_punctuation", "component_recipe", "inlay path",
  "A discrete cell used as PUNCTUATION wherever a linear path turns a corner or terminates. "
  "It is a placement rule as much as a shape.",
  "a cell master + a rule keyed to path corners and terminations",
  "the ends and corners of an inlay path",
  "let in with the path",
  "punctuates a linear run",
  "framed rosette square; disc-in-bezel",
  "cell size; spacing pitch along the run; the corner/termination rule itself",
  "the continuous line the cells punctuate",
  "parameterized_recipe",
  ["R035","R330"], proven=["R035","R330"],
  note="'It is the punctuation mark used wherever a linear path turns a corner or "
       "terminates - a PLACEMENT RULE as much as a shape.'"),

F("inlay.repeated_cell_run", "component_recipe", "inlay path",
  "A row of discrete inlaid cells at a pitch - instancing, not a continuous line.",
  "instance a cell along a path at a pitch",
  "a show face",
  "let in",
  "vertical accent band",
  "dark inlaid triangles",
  "triangle size; spacing; run length",
  "continuous stringing lines",
  "parameterized_recipe",
  ["R296"], proven=["R296"]),
F("bespoke.claw_and_ball_foot", "bespoke_hero", "foot",
  "A ball gripped by four talons - a fixed carved master, not a recipe. The source says so: "
  "'This is a fixed donor mesh, not a recipe - swap it whole for pad foot, spade foot, or trifid.'",
  "hand-authored sculpture", "ankle of a cabriole leg", "carved integral with the leg blank",
  "front-leg floor contact", "carved and finished wood",
  "uniform scale; talon spread; ball diameter relative to the ankle - and WHOLE-MASTER "
  "substitution, which is the real variation axis",
  "the paw foot on 1972.47, an unrelated carving; every turned foot",
  "bespoke", ["R105.a"], proven=["R105.a"],
  note="Split into its own master per the audit. The object carries it on the three FRONT "
       "legs only; the rear legs have no foot at all and sit in termination.leg_toe_prismatic."),

F("bespoke.paw_foot", "bespoke_hero", "foot",
  "A carved hairy paw with an acanthus knuckle and a side scroll boss, seated on a separate "
  "disc pad.",
  "hand-authored sculpture", "leg above; disc pad below",
  "carved integral with the leg; seats on the separable disc pad",
  "floor contact", "carved and gilt",
  "scale; toe count; forward rake",
  "the claw-and-ball master, a different sculpture; the disc pad beneath it, which IS a "
  "recipe and is its own family",
  "bespoke", ["R137.a"], proven=["R137.a"],
  note="The disc pad was extracted to foot.disc_pad_separable because the source states it "
       "is 'separable from the paw and reusable alone'. The paw itself is not reusable."),

F("ornament.painted_repeating_row", "component_recipe", "repeated ornament master",
  "A repeating ornament row executed in PAINT on a flat ground. Same placement grammar as an "
  "instanced relief run, but no geometry is added at all.",
  "a 2D cell repeated along a path, resolved to texture",
  "a flat painted ground", "NONE - it is a surface treatment",
  "enriched band", "painted flat",
  "unit pitch; repeat count; blossom-to-bud ratio; whether the row is painted (as here) or "
  "carved - the source names that switch explicitly",
  "carved relief masters, which are solids with a glue or carve interface",
  "parameterized_recipe", ["R055"], proven=["R055"],
  note="Same construction split as inlay.painted_line_path: shared placement vocabulary, "
       "different realization. The row itself flags the switch: 'whether the row is painted "
       "(as here) or carved'."),

]

# ---------------------------------------------------------------- 2b. RELIEF MASTERS
# v3 merged 17 unrelated relief masters into one family because they all get placed by a
# transform. Placement is not construction. Each master below is its own PART IDENTITY, with
# its own attachment and its own material process. They are generated from a table only to
# keep the axes uniform - they are not a merged family.
# (fid, member, what it is, attachment, material process, variation, reuse, uncert)
RELIEF_MASTERS = [
 ("relief.spiral_volute", "R017", "a carved spiral volute",
  "carved integral or applied - not separated in the row", "carved wood, gilt",
  "scale; turn count", "parameterized_recipe", None),
 ("relief.pendant_palmette", "R080", "a heart-shaped palmette-and-scroll drop",
  "applied to the front face of a post", "carved wood",
  "length to suit post height", "parameterized_recipe", None),
 ("relief.knee_acanthus_rocaille", "R106",
  "an acanthus-and-shell cartouche filling a knee face and RETURNING ONTO THE RAIL",
  "carved in the solid, continuous across leg and rail", "carved wood",
  "instance scale; mirroring; presence (a plainer bench drops it)", "parameterized_recipe", None),
 ("relief.carved_roundel", "R185", "a disc-within-a-ring set in a squarish block",
  "repeated in vertical strings interrupting pilaster reeding", "carved wood",
  "diameter; count per string (tunes block length)", "parameterized_recipe", None),
 ("relief.anthemion_capital", "R234.a", "an upright acanthus/anthemion capital master",
  "instanced along a path", "carved and gilded wood",
  "instance pitch; scale ramp along the run", "parameterized_recipe", None),
 ("relief.husk_laurel_swag", "R234.b",
  "a husk-and-laurel swag with an anthemion drop at each junction",
  "hung between junction points along a run", "carved and gilded wood",
  "swag sag depth; span between junctions; husk count", "parameterized_recipe", None),
 ("relief.dentil_block", "R234.c", "a dentil block",
  "instanced along a cornice bed at a pitch", "carved wood",
  "block section; PITCH; run length", "parameterized_recipe", None),
 ("relief.drilled_dot", "R234.d", "a drilled dot",
  "drilled into the host face - SUBTRACTIVE, unlike every other master here", "drilled wood",
  "diameter; depth; spacing along a band", "parameterized_recipe", None),
 ("relief.bolection_field", "R297",
  "an applied bolection-moulded field, outline rectangle or lozenge",
  "applied to a panel or door face", "moulded wood",
  "outline (rectangle vs lozenge); field size; moulding step count", "parameterized_recipe", None),
 ("relief.floral_block", "R332", "a carved floral relief block for a bed post",
  "the same carved face repeated around a block - ONE tile drives the component",
  "carved wood", "block height; face width; relief depth", "parameterized_recipe", None),
 ("relief.pierced_rosette_ring", "R355",
  "a pierced ring enclosing a many-petalled rosette - the apron's atomic unit",
  "pierced THROUGH the ground or applied solid - both states recorded", "carved wood",
  "ring diameter; petal count; pierce-through vs solid ground; linear pitch",
  "parameterized_recipe", None),
 ("relief.acanthus_collar", "R357.a", "a leaf collar capping a leg turning",
  "seated around the top of a turning", "carved wood",
  "leaf count; collar height", "parameterized_recipe", None),
 ("relief.rosette_die_panel", "R357.b",
  "a square foliate panel with a central rosette filling a corner die block",
  "fills the face of a die block", "carved wood",
  "panel size; rosette petal count", "parameterized_recipe", None),
 ("relief.beaded_teardrop_lappet", "R439",
  "a swollen teardrop bounded by a beaded ogee outline - the unit of the gilt drum sleeves",
  "instanced around a drum register", "cast/chased gilt brass",
  "lappet width; bead pitch; teardrop taper; count around the circumference",
  "parameterized_recipe", None),
 ("relief.heart_lappet", "R440",
  "a broad heart/lotus leaf with a small spearhead between each pair, run around a foot flare",
  "instanced around the flare of a base ring", "cast/chased gilt brass",
  "leaf width; interstitial motif; count around the ring", "parameterized_recipe", None),
 ("relief.glass_cabochon", "R438",
  "a hemispherical red glass boss, instanced in rows and strung along engraved swag lines",
  "SET into a metal or stone seat - a glazier's job, not a carver's", "moulded red glass",
  "diameter (about two sizes); row pitch; stringing along a curve", "parameterized_recipe",
  "quantity"),
 ("relief.mop_disc_in_bezel", "R034",
  "a flat circular mother-of-pearl disc seated in a raised gilt ring bezel",
  "the BEZEL is the interface - a raised metal ring that captures the shell disc",
  "shell disc + gilt metal bezel - two materials, two processes",
  "disc diameter (two scales recorded); bezel ring section", "parameterized_recipe",
  "quantity"),
 ("relief.carved_rosette_patera", "R114.c",
  "a carved rosette/patera - host-carved relief",
  "carved into or applied onto the host - method not separated in the row", "carved wood",
  "scale; petal count; relief depth", "parameterized_recipe", "quantity"),
 ("relief.flower_chain_run", "R114.d",
  "a carved flower-in-linked-chain - a linked repeating carved band",
  "carved along a run on the host", "carved wood",
  "chain pitch; flower scale; run length", "parameterized_recipe", "quantity"),
 ("relief.carved_palmette", "R114.f",
  "a carved palmette - host-carved relief",
  "carved into or applied onto the host - method not separated in the row", "carved wood",
  "scale; frond count", "parameterized_recipe", "quantity"),
]

for _fid, _m, _what, _att, _mat, _var, _reuse, _unc in RELIEF_MASTERS:
    FAMILIES.append(F(
        _fid, "component_recipe", "repeated ornament master",
        "A single relief master: " + _what + ". Its identity is the geometry, not the "
        "transform that places it.",
        "one authored master solid", _att, _att,
        "instanced surface enrichment", _mat, _var,
        "every other relief master - they are unrelated geometry that merely shares a "
        "placement system (see ornament.placement_system)",
        _reuse, [_m], proven=[_m],
        uncert=({_m: _unc} if _unc else None),
        note="Given its own part identity in v3.1. v3 merged this into "
             "ornament.instanced_relief_master with 16 others."))

FAMILIES += [

F("relief.applied_wreath_ring", "component_recipe", "repeated ornament master",
  "A single relief master: a carved wreath ring APPLIED as a centre ornament, hosting a "
  "swappable inset. Its identity is the geometry, not the transform that places it.",
  "one authored master solid",
  "applied to the ground - ATTACHMENT METHOD UNRESOLVED (glue, screws or pins; the source "
  "shows the applied state, never the fixing)",
  "applied; fixing method unresolved",
  "instanced surface enrichment", "carved wood",
  "ring diameter; leaf/blossom density; which inset it hosts",
  "every other relief master; ornament carved into its ground",
  "parameterized_recipe", ["R303.a"], proven=[], uncert={"R303.a": "construction"},
  note="The final audit removed the 'glued' claim: the source term says 'applied centre "
       "ornament' and nothing about adhesive. Applied-ness is visible; the fixing is not - "
       "unlike R356's garland, whose screws are photographed from behind. Construction "
       "uncertainty, honestly carried."),

F("inlay.inset_swappable_cell", "component_recipe", "repeated ornament master",
  "An inset marquetry cell that DROPS INTO a carved host - the same wreath can host different "
  "inlay. A veneer cut, not relief geometry.",
  "flat 2D marquetry cell", "a recess in a carved applied ornament",
  "let into the recess", "swappable centre of a cartouche", "marquetry veneer",
  "cell size; which motif fills it",
  "the carved ring that hosts it, which is wood relief; host-carved linenfold",
  "parameterized_recipe", ["R303.b"], proven=["R303.b"],
  note="Split from ornament.carved_in_solid_face, where v3 had put a marquetry cell alongside "
       "carved wood. 'the inset shield panel is a swappable marquetry cell, so the same "
       "carved wreath can host different inlay.'"),

F("inlay.closed_line_outline_master", "component_recipe", "repeated ornament master",
  "A pure CLOSED LINE-FIGURE master with no botanical or figural content. It retargets by "
  "STRETCHING rather than by re-cutting, which no botanical motif can do.",
  "a closed 2D outline with end caps", "a veneered face", "let into the veneer",
  "repeated geometric accent", "contrast-wood veneer",
  "aspect ratio; end-cap radius - stretch parameters, not density parameters",
  "botanical motif sets, which must be re-scattered rather than stretched",
  "parameterized_recipe", ["R154"], proven=["R154"],
  note="Removed from ornament.marquetry_motif_set per the audit. The row states the "
       "divergence itself: 'there is no figural or botanical marquetry element anywhere on "
       "this object - the repeated masters are pure closed line-figures, so they retarget by "
       "stretching rather than by re-cutting.'"),

F("inlay.keyhole_cartouche", "component_recipe", "repeated ornament master",
  "An inlaid trefoil ribbon knot FRAMING A KEYHOLE - a functional marker keyed to a hardware "
  "position, not a field filler.",
  "a 2D master placed on a keyhole centre", "a drawer front, centred on the lock",
  "let into the veneer, aligned to the keyhole",
  "marks an opening", "marquetry veneer",
  "scale; whether it wraps a brass plate or a bare keyhole",
  "botanical field motifs, which are placed by density rather than by a hardware position",
  "parameterized_recipe", ["R186"], proven=["R186"],
  note="Removed from ornament.marquetry_motif_set per the audit. 'a ready-made this-opens "
       "visual marker for any drawer asset.'"),

F("inlay.star_scatter_master", "component_recipe", "repeated ornament master",
  "A flat inlaid MOP star run at 4+ sizes and scattered over a field - veneer-cut.",
  "a flat 2D master, scattered", "a veneered field", "let into the veneer",
  "scattered field enrichment", "mother-of-pearl",
  "scale (4+ sizes on the top alone); scatter density",
  "every carved master from the same source row, which is host geometry; the MOP disc and "
  "abalone plaque, which are different figures",
  "parameterized_recipe", ["R114.a"], proven=["R114.a"], uncert={"R114.a": "quantity"},
  note="R114 names SIX masters and the final audit split all six. This is the MOP star "
       "('approx 120+ instances across top slab plus approx 30 per end panel' - approx, "
       "hence quantity uncertainty). Its five siblings each have their own family."),

F("inlay.mop_disc_master", "component_recipe", "repeated ornament master",
  "A flat inlaid mother-of-pearl circular disc - veneer/shell-cut, set flush. NOT the "
  "bezel-captured disc of 2012.216, which is a different interface on a different object.",
  "a flat 2D shell disc, scattered or run", "a veneered/inlaid field",
  "let in flush - no bezel", "field enrichment", "mother-of-pearl",
  "diameter; scatter density",
  "relief.mop_disc_in_bezel - bezel capture is a different interface; every carved master "
  "from the same source row",
  "parameterized_recipe", ["R114.b"], proven=["R114.b"], uncert={"R114.b": "quantity"},
  note="One of R114's six stated masters."),

F("inlay.abalone_plaque", "component_recipe", "repeated ornament master",
  "A flat rectangular abalone plaque - shell-cut, set flush.",
  "a flat 2D shell rectangle", "a veneered/inlaid field",
  "let in flush", "field enrichment", "abalone shell",
  "plaque aspect ratio; scale; placement density",
  "the MOP masters (different shell, different figure); every carved master from the row",
  "parameterized_recipe", ["R114.e"], proven=["R114.e"], uncert={"R114.e": "quantity"},
  note="One of R114's six stated masters."),

]

# ---------------------------------------------------------------- 2c. QUARANTINE
# Rows deliberately held OUT of every production family, with the reason and the kind of
# doubt. This is an EXPLICIT PERMITTED STATE: the fail-closed check below accepts a row as
# accounted-for only if it is assigned, quarantined, a non-asset, or zero-weight.
QUARANTINE = [
 ("R243", "construction",
  "termination.stile_to_floor (deleted) claimed this as a proved continuous termination. "
  "'Whether it is glued-on or integral is not resolvable under the ebonizing.' The two "
  "readings are opposite joint contracts."),
 ("R156", "construction",
  "'A horizontal seam crosses each leg roughly 12 cm above the floor where the stringing "
  "also stops - I cannot tell whether this is an original cuff line or a spliced repair.' "
  "A splice IS a joint, so this is construction doubt as well as provenance doubt."),
 ("R320", "construction",
  "Removed from muntin.panel_grooved: neither the opposite-face grooves nor the rail tenons "
  "are visible. The row records only 'spacing and count across a panel width'."),
 ("R338.b", "construction",
  "'whether the ends are knockdown or fixed' - the rail FORM is fully visible (R338.a, in "
  "production) but its end joint is not."),
 ("R072", "construction",
  "panel.floating_wood_in_groove: inferred from type. The row records a carved show face and "
  "'a plain UNGILDED board with a visible horizontal glue joint' - a glued-up board, which "
  "is if anything evidence AGAINST a free-floating panel."),
 ("R131", "construction", "panel: inferred from type - no groove or frame evidence in the row."),
 ("R219", "construction",
  "panel: inferred from type. 'Flat, flush, unornamented fields sitting inside the reeded "
  "margins' describes APPLIED margins, which is not a groove."),
 ("R317", "construction", "panel: inferred from type - the row records the marquetry, not the edge."),
 ("R165", "construction", "panel: unresolved. 'recess depth' is recorded, the capture is not."),
 ("R318", "construction", "panel: unresolved. 'whether bays are flush or recessed' is open."),
 ("R273", "construction",
  "panel.floating_wood_in_groove, final audit: companion row R272 proves frame layout, "
  "panel-opening count and peg pattern - never the concealed groove or the floating "
  "capture. frame_layout_only is not an admissible interface class."),
 ("R381", "construction",
  "panel.floating_wood_in_groove, final audit: companion row R380 records 'joined frame "
  "(stiles + rails + muntins)' and whether ornament is applied - frame layout again, not "
  "the capture contract."),
 ("R249", "construction",
  "panel: unresolved, and its own frame row (R248) states the case-side construction 'cannot "
  "be settled'. Admitting the panel would decide by the back door what the frame refuses."),
]


# ---------------------------------------------------------------- 3. DONOR INVENTORY
# Statuses (only these five):
#   VERIFIED_COMPATIBLE   silhouette, section, topology, construction, joints, interfaces
#                         and editability all compared and all matching
#   RECIPE_PRECEDENT_ONLY the approach is instructive; the asset itself is not a donor
#   MATERIAL_ONLY         donates surface, not construction
#   INCOMPATIBLE          compared and rejected
#   AUDIT_REQUIRED        not yet compared - the DEFAULT, and never an implicit pass
#
# INSPECTED, not inferred from directory layout. v2 claimed "only rough_hewn_timber_beam_v1
# is a geometry asset" by reading assets/creative/materials/... as material-only. That was
# wrong: forged_iron_v1/ ships openwork_strap_hinge_geometry_v1 (build script, manifest,
# clay-proof .blend) AND forged_fasteners_v1.blend, and both the joinery and door assets
# ship .blend fixtures. Geometry and material ownership are recorded separately below.

SINC = "/Users/kogaryu/Documents/Codex/2026-07-25/sinc"

# STATUS IS PER DONOR-TO-FAMILY RELATIONSHIP. One package can be a legitimate precedent for
# one family and flatly incompatible with another; a single status per package cannot say
# that. `relations` carries the per-family verdicts where they differ from the headline.
DONORS = [
 dict(donor="iggy3d rough_hewn_timber_beam_v1", kind="geometry",
      located="assets/creative/architecture/structural/rough_hewn_timber_beam_v1",
      status="RECIPE_PRECEDENT_ONLY",
      compared="silhouette NO / section NO / topology NO / construction PARTIAL / joints NO / "
               "interfaces NO / editability NO",
      relations="stile.continuous_leg_and_stile=RECIPE_PRECEDENT_ONLY; "
                "stile.case_corner_board=RECIPE_PRECEDENT_ONLY",
      finding="Continuous-square-stock logic is a legitimate precedent for single-stick "
              "members with no joint along the run. Adze-hewn surface, architectural scale, "
              "no furniture joint schedule. Not a donor for any family."),
 dict(donor="iggy3d structural_oak_joinery_v1 - geometry", kind="geometry",
      located="assets/creative/materials/structural_oak_joinery_v1/output/"
              "structural_oak_joinery_v1.blend",
      status="RECIPE_PRECEDENT_ONLY",
      compared="construction PARTIAL - joinery approach only; silhouette/section/topology/"
               "joints/interfaces/editability NOT compared against corpus members",
      relations="frame.stile_rail_muntin_grid=RECIPE_PRECEDENT_ONLY; "
                "rail.seat_tenoned=RECIPE_PRECEDENT_ONLY",
      finding="Resolved from AUDIT_REQUIRED. Its mortise-and-tenon approach is a genuine "
              "precedent for joined frames and tenoned rails. It is NOT an exact donor: "
              "structural oak joinery is sized for building frames, not furniture."),
 dict(donor="iggy3d structural_oak_joinery_v1 - materials", kind="material",
      located="assets/creative/materials/structural_oak_joinery_v1/{patterns,profiles,output}",
      status="MATERIAL_ONLY", compared="n/a - material assets are not construction donors",
      relations="all wooden families=MATERIAL_ONLY",
      finding="Donates oak surface. Recorded separately from the geometry above so the "
              "package can never be cited as a geometry donor by association."),
 dict(donor="iggy3d structural_oak_door_v1 - materials", kind="material",
      located="assets/creative/materials/structural_oak_door_v1/{patterns,output}",
      status="MATERIAL_ONLY", compared="n/a",
      relations="all wooden families=MATERIAL_ONLY", finding="Surface only."),
 dict(donor="iggy3d structural_oak_door_v1 - door fixture", kind="geometry",
      located="assets/creative/materials/structural_oak_door_v1/output/"
              "structural_oak_door_v1.blend (externally imported Sinc asset)",
      status="INCOMPATIBLE",
      compared="construction YES - and it fails there",
      relations="frame.stile_rail_muntin_grid=INCOMPATIBLE; "
                "panel.floating_wood_in_groove=INCOMPATIBLE; "
                "frame.applied_moulded_surround=INCOMPATIBLE",
      finding="RESOLVED AND REJECTED. v3 flagged this as the most plausible frame-and-panel "
              "precedent AND the most dangerous name-resemblance trap. The trap was real: it "
              "is an externally imported Sinc LEDGED-AND-BRACED PLANK DOOR - vertical planks "
              "on ledges with a diagonal brace. That is the opposite construction to "
              "frame-and-panel: no stiles, no rails, no muntins, no grooves, no floating "
              "panel. INCOMPATIBLE with every frame and panel family."),
 dict(donor="iggy3d forged_iron_v1 - material", kind="material",
      located="assets/creative/materials/forged_iron_v1/{patterns,output}",
      status="MATERIAL_ONLY", compared="n/a",
      relations="stay.metal_quadrant_visible=MATERIAL_ONLY; "
                "relief.beaded_teardrop_lappet=MATERIAL_ONLY",
      finding="Donates iron surface only."),
 dict(donor="iggy3d openwork_strap_hinge_geometry_v1", kind="geometry",
      located="assets/creative/materials/forged_iron_v1/"
              "build_openwork_strap_hinge_geometry_v1.py + output manifest + clay proof",
      status="INCOMPATIBLE",
      compared="construction YES / topology YES - both fail against the quadrant stay",
      relations="stay.metal_quadrant_visible=INCOMPATIBLE; "
                "flyrail.moving_assembly=RECIPE_PRECEDENT_ONLY (generic pivot only)",
      finding="RESOLVED. A strap hinge leaf is a flat pierced plate rotating about a fixed "
              "pintle. A quadrant stay is an ARC that SLIDES IN A SLOT while its other end "
              "is fixed - a different mechanism, a different section and a different motion. "
              "INCOMPATIBLE with the quadrant stay. Retains value as a generic pivot "
              "precedent for the fly-rail assembly and nothing more."),
 dict(donor="iggy3d forged_fasteners_v1", kind="geometry",
      located="assets/creative/materials/forged_iron_v1/output/forged_fasteners_v1.blend",
      status="RECIPE_PRECEDENT_ONLY",
      compared="construction PARTIAL - generation and placement approach only",
      relations="plate.pegged_repair=RECIPE_PRECEDENT_ONLY; "
                "cleat.lid_batten=RECIPE_PRECEDENT_ONLY; "
                "rail.seat_pinned=RECIPE_PRECEDENT_ONLY",
      finding="RESOLVED. Precedent for fastener GENERATION AND PLACEMENT - arrays, spacing, "
              "seating into a host. NOT an exact donor for wooden pegs or round furniture "
              "pins: forged iron fasteners and turned wooden pegs differ in material, "
              "silhouette and how they seat."),
 dict(donor="Sinc GH-001 hand-hewn structural beam system", kind="geometry",
      located=SINC + "/docs/asset-library/MASTER_ASSET_REGISTRY.md (GH-001 row); "
              + SINC + "/work/giant_house_hand_hewn_timber_beam_system_v1_SELECTION.md; "
              + SINC + "/outputs/giant-house-hand-hewn-timber-beam-system-v1/"
                       "giant_house_hand_hewn_timber_beam_system_v1.blend",
      status="RECIPE_PRECEDENT_ONLY", compared="construction PARTIAL",
      relations="stile.continuous_leg_and_stile=RECIPE_PRECEDENT_ONLY; "
                "stile.case_corner_board=RECIPE_PRECEDENT_ONLY",
      finding="Registry: 'Hand-hewn structural beam system', donor accepted. Its "
              "continuous-stock member logic is precedent for the single-stick stile "
              "families - and nothing more: hewn architectural beams share no furniture "
              "silhouette, section or joint schedule."),
 dict(donor="Sinc GH-002 pegged mortise-and-tenon joint", kind="geometry",
      located=SINC + "/docs/asset-library/MASTER_ASSET_REGISTRY.md (GH-002 row); "
              + SINC + "/work/giant_house_pegged_mortise_tenon_joint_v1_SELECTION.md; "
              + SINC + "/outputs/giant-house-pegged-mortise-tenon-joint-v1/"
                       "giant_house_pegged_mortise_tenon_joint_v1.blend",
      status="RECIPE_PRECEDENT_ONLY", compared="construction PARTIAL",
      relations="frame.stile_rail_muntin_grid=RECIPE_PRECEDENT_ONLY; "
                "rail.seat_tenoned=RECIPE_PRECEDENT_ONLY",
      finding="Registry: 'Pegged mortise-and-tenon joint', donor accepted. The joint TYPE "
              "matches the corpus's pegged frames and tenoned seat rails, so it is a "
              "legitimate method precedent - at building scale, so not an exact donor for "
              "furniture sections."),
 dict(donor="Sinc GH-009 raised-panel and frame/panel candidates", kind="geometry",
      located=SINC + "/docs/asset-library/GH009_FURNITURE_COMPONENT_LEDGER.md; "
              + SINC + "/outputs/blender-toolkit-pilot/gh009-furniture-raised-panel-door-a/; "
              + SINC + "/outputs/blender-toolkit-pilot/gh009-furniture-frame-panel-a/"
                       "gh009_furniture_frame_panel_a.blend; "
              + SINC + "/outputs/blender-toolkit-pilot/gh009-furniture-joined-stock-a/"
                       "gh009_furniture_joined_stock_a.blend",
      status="AUDIT_REQUIRED",
      compared="none - exact corpus compatibility comparison outstanding",
      relations="panel.floating_wood_in_groove=AUDIT_REQUIRED; "
                "frame.stile_rail_muntin_grid=AUDIT_REQUIRED",
      finding="THE OPEN ITEM. Bears directly on the repaired panel family and on the joined "
              "frame grammar. Requires an exact compatibility comparison - silhouette, "
              "section, topology, construction, joints, interfaces, editability - against "
              "the TWO proved panel members (R363, R198) before any claim. Registry "
              "note: raised-panel door A and the frame/panel components are visually "
              "accepted on the Sinc side - acceptance there is not compatibility here."),
 dict(donor="Sinc GH-011 furniture pins", kind="geometry",
      located=SINC + "/docs/asset-library/GH011_WROUGHT_FASTENER_COMPONENT_LEDGER.md; "
              + SINC + "/outputs/giant-house-furniture-pin-short-v1/"
                       "giant_house_furniture_pin_short_v1.blend; "
              + SINC + "/outputs/giant-house-furniture-pin-long-v1/"
                       "giant_house_furniture_pin_long_v1.blend",
      status="RECIPE_PRECEDENT_ONLY", compared="construction PARTIAL",
      relations="rail.seat_pinned=RECIPE_PRECEDENT_ONLY; "
                "plate.pegged_repair=RECIPE_PRECEDENT_ONLY",
      finding="Resolved. Precedent for pin generation and placement. The corpus's pins are "
              "visible round wooden pegs; compatibility of the actual pin geometry is not "
              "established."),
 dict(donor="Sinc GH-018 moving strap leaf", kind="geometry",
      located=SINC + "/docs/asset-library/GH018_STRAP_HINGE_COMPONENT_LEDGER.md; "
              + SINC + "/outputs/giant-house-strap-hinge-leaf-a-v1/"
                       "giant_house_strap_hinge_leaf_a_v1.blend",
      status="INCOMPATIBLE",
      compared="construction YES - fails against the quadrant stay",
      relations="stay.metal_quadrant_visible=INCOMPATIBLE; "
                "flyrail.moving_assembly=RECIPE_PRECEDENT_ONLY (generic moving eye only)",
      finding="Resolved. A moving strap leaf is not a sliding quadrant. At most a generic "
              "moving-eye precedent."),
 dict(donor="Sinc GH-019 components", kind="geometry",
      located=SINC + "/docs/asset-library/GH019_HERTER_CHAIR_COMPONENT_LEDGER.md; "
              + SINC + "/docs/asset-library/GH019_HERTER_CHAIR_BUILDER_BLUEPRINT.md; "
              + SINC + "/outputs/gh019-herter-chair-v1/ (WIP .blends only - the registry "
                       "records 'Builder forbidden until complete blueprint review')",
      status="AUDIT_REQUIRED", compared="none - and blocked before comparison",
      relations="", finding="DONOR-INELIGIBLE UNTIL P39. GH-019 components remain blocked by "
              "their own donor-admission gate. Accepted status is not compatibility, and no "
              "comparison may be credited until P39 clears."),
 dict(donor="Sinc GOK-001 profile kernel", kind="geometry",
      located=SINC + "/docs/asset-library/GOK001_GOTHIC_ORNAMENT_KERNEL_LEDGER.md; "
              + SINC + "/outputs/sinc-gothic-ornament-kernel-v1/"
                       "sinc_gothic_ornament_kernel_v1.blend; "
              + SINC + "/outputs/sinc-gothic-ornament-kernel-v1/review_manifest.json",
      status="RECIPE_PRECEDENT_ONLY", compared="construction PARTIAL",
      relations="moulding.profile_toolkit=RECIPE_PRECEDENT_ONLY",
      finding="Resolved, and it lands on the top-ranked lane leader. A profile kernel is "
              "precedent for the profiling toolkit's atoms-and-sweeps approach. It is NOT a "
              "verified exact donor: the corpus's ten observed profile atoms and their "
              "mitre/stopped-end/junction rules have not been matched against it. This is "
              "the natural first target once the census is frozen."),
]

# ---------------------------------------------------------------- 4. NON-ASSETS
# Constraints, negative facts and anti-duplication annotations. NEVER ranked, never scored,
# and never counted as evidence for a buildable family.
NON_ASSETS = [
 dict(item="inlay_present=false", kind="candidate negative fact",
      rows=["R092"], candidate="FC-03-2011_3",
      statement="'RECORDED AS ABSENT. All ornament here is carved in the solid and "
                "water-gilt; there is no linear or circular inlay and no marquetry anywhere "
                "on the object. The paterae that read as the baseline's circular inlay are "
                "carved discs.'",
      disposition="REMOVED from the component taxonomy and from all ranking (v2 carried it as "
                  "the family `inlay.absent_recorded`, reuse class bespoke, and RANKED it). "
                  "Stored as a boolean property of the candidate. It is real and useful - it "
                  "is the corpus's clearest proof that carved discs and inlaid discs are "
                  "different constructions that read alike - but it is not a thing to model."),
 dict(item="frame.assembled_from_members", kind="anti-duplication annotation",
      rows=["R011","R342"], candidate="FC-01-1999_488; FC-09-1954_151",
      statement="'The frame is not a joinery unit of its own - it is the ASSEMBLY of post + "
                "post + bottom rail + arched crest, enclosing one arch-topped field.'",
      disposition="Demoted from a ranked recipe to an OWNERSHIP annotation. Its purpose is to "
                  "stop a modeller authoring a 'frame' part that does not exist - the members "
                  "are already owned by their own families. Authoring it would duplicate "
                  "geometry. v2 ranked it; that was a category error."),
 dict(item="pedestal parent-assembly annotation", kind="anti-duplication annotation",
      rows=["R430"], candidate="FC-12-2002_298_1",
      statement="'Parent assembly of the two turned spindle entries below - NOT an extra "
                "piece of stone. The whole design is a stack grammar: plinth / foot / gilt "
                "drum / jewel band / torsade drum / collar / jewel band / plain drum / jewel "
                "band / gilt drum / capital / abacus.'",
      disposition="Recorded as ownership, not as a part. The stack ORDER is the reusable "
                  "idea and it is captured by post.bed_stacked_block's grammar; the shaft "
                  "itself is not separate geometry."),
 dict(item="axial tie rod through the stacked pedestal", kind="inferred construction constraint",
      rows=["R446"], candidate="FC-12-2002_298_1",
      statement="'An internal threaded rod or tie through the stacked elements. ASSERTED, NOT "
                "SEEN: alabaster at 163 cm tall on a 28 cm base cannot survive as a "
                "freestanding butt-jointed stack without a core in tension, and every "
                "plausible joint plane is deliberately hidden under a gilt sleeve. No view "
                "shows it.'",
      disposition="Split from visible metal stays per the audit. INFERRED FROM TYPE, weight "
                  "0. It is a sound engineering argument and a useful modelling note, and it "
                  "may NOT establish a joint, an interface or a donor claim. Kept as a "
                  "constraint so a modeller knows the stack needs a core - not as an asset."),
 dict(item="frame construction of 69.140ab case sides", kind="unresolved construction",
      rows=["R248"], candidate="FC-08-69_140ab",
      statement="'Whether the case sides are true frame-and-panel or solid boards with "
                "applied mouldings cannot be settled - the interior faces read as flat "
                "boards.'",
      disposition="REMOVED from BOTH frame families per the audit. v2 had it in "
                  "frame.stile_rail_muntin_grid, where it silently voted for a construction "
                  "the source explicitly refuses to settle. The two candidate constructions "
                  "are opposites, so membership in either would be a fabricated fact."),
]

# ---------------------------------------------------------------- 4b. PANEL INTERFACE
# How each panel row's edge condition is ACTUALLY known. Only the first three classes may
# establish the production recipe; the rest are quarantined.
PANEL_IFACE = {
 "R363": "groove_stated_by_source",
 "R198": "frame_and_panel_stated_by_source",
 "R273": "frame_layout_only",
 "R381": "frame_layout_only",
 "R072": "inferred_from_type",
 "R131": "inferred_from_type",
 "R219": "inferred_from_type",
 "R317": "inferred_from_type",
 "R165": "unresolved",
 "R318": "unresolved",
 "R249": "unresolved",
}
PANEL_IFACE_ADMISSIBLE = ("groove_visibly_proved", "groove_exposed_construction_view",
                          "groove_stated_by_source", "frame_and_panel_stated_by_source")


# ---------------------------------------------------------------- 5. SCORING
# v3.1 changes:
#   - ONLY construction-typed uncertainty depresses confidence or penalises. Quantity,
#     view-coverage, surface and provenance doubt are recorded and cost nothing.
#   - status AUDIT_ONLY forces NOT_RANKABLE regardless of evidence. A visible silhouette
#     with an unknown interface is research, not a production queue entry.
#   - every factor is emitted at FULL PRECISION with its numerator and denominator, so a
#     reader can reproduce final_score from the printed row alone.

def score(fam, rows_by_uid):
    subs = [rows_by_uid[u] for u in fam["members"] if u in rows_by_uid]
    n = len(subs)
    Wt = sum(W_EVID.get(s["evidence"], 0.0) for s in subs)
    N = len(set(s["candidate_id"] for s in subs
                if W_EVID.get(s["evidence"], 0.0) > 0))
    mem = set(fam["members"])
    n_proven = len(fam["proven"] & mem)
    n_constr = len([u for u, t in fam["uncert"].items()
                    if t == "construction" and u in mem])
    C = (n_proven / n) if n else 0.0
    A = (n_constr / n) if n else 0.0
    conf_f = 0.5 + 0.5 * C
    amb_f = 1.0 - 0.5 * A
    breadth_f = 1.0 + 0.15 * max(0, N - 1)
    R = REUSE_MULT[fam["reuse"]]
    rank_f = 0.0 if fam["status"] == "AUDIT_ONLY" else 1.0
    final = Wt * conf_f * R * amb_f * breadth_f * rank_f
    if fam["status"] == "AUDIT_ONLY":
        rank = "NOT_RANKABLE_AUDIT_ONLY"
    elif Wt <= 0:
        rank = "NOT_RANKABLE_NO_EVIDENCE"
    else:
        rank = "RANKABLE"
    other = {}
    for u, t in fam["uncert"].items():
        if t != "construction" and u in mem:
            other[t] = other.get(t, 0) + 1
    return dict(n_subrows=n, weighted_evidence=Wt, objects_unlocked=N,
                n_proven=n_proven, n_construction_uncertain=n_constr,
                construction_confidence=C, ambiguity_penalty=A,
                confidence_factor=conf_f, ambiguity_factor=amb_f,
                breadth_factor=breadth_f, rankability_factor=rank_f,
                reuse_class=fam["reuse"],
                reuse_multiplier=R, final_score=final, rankability=rank,
                other_uncertainty=";".join("%s=%d" % kv for kv in sorted(other.items())))


# ---------------------------------------------------------------- 5b. FAIL CLOSED
class CorpusError(Exception):
    pass


def validate(subs, by_uid):
    """Stop the build rather than emit a table with a hole in it. v2 lost 19 memberships
    silently; nothing in this generator may fail quietly again."""
    errs = []
    quar = set(q[0] for q in QUARANTINE)
    nonasset = set(r for x in NON_ASSETS for r in x["rows"])
    seen_fids = set()
    declared = 0
    for f in FAMILIES:
        if f["fid"] in seen_fids:
            errs.append("duplicate family id: %s" % f["fid"])
        seen_fids.add(f["fid"])
        declared += len(f["members"])
        for u in f["members"]:
            if u not in by_uid:
                errs.append("%s cites row UID %s which does not exist" % (f["fid"], u))
            if u in quar:
                errs.append("%s cites QUARANTINED row %s" % (f["fid"], u))
            if u in nonasset:
                errs.append("%s cites NON-ASSET row %s" % (f["fid"], u))
        for u in f["proven"]:
            if u not in set(f["members"]):
                errs.append("%s marks %s proven but does not declare it" % (f["fid"], u))
        for u in f["uncert"]:
            if u not in set(f["members"]):
                errs.append("%s marks %s uncertain but does not declare it" % (f["fid"], u))
        both = f["proven"] & set(k for k, v in f["uncert"].items() if v == "construction")
        if both:
            errs.append("%s marks %s BOTH construction-proven and construction-uncertain"
                        % (f["fid"], sorted(both)))
    for u in quar:
        if u not in by_uid:
            errs.append("quarantine cites row UID %s which does not exist" % u)
    for u in nonasset:
        if u not in by_uid:
            errs.append("non-asset cites row UID %s which does not exist" % u)
    if errs:
        raise CorpusError("FAIL-CLOSED: %d problem(s)\n  - %s" % (len(errs),
                                                                   "\n  - ".join(errs)))
    return declared


# ---------------------------------------------------------------- 6. COVERAGE
# CORRECTION (v3.1). v3 asserted that v1 "promises to list the gap candidates and then
# doesn't". THAT WAS WRONG - I read v1's tally section and stopped short of the section
# below it. v1's README, under "Known gaps, stated rather than hidden", says plainly:
#   "2 BOMs missing: FC-02-60_4_14 (Phyfe footstool) and FC-06-27_57_1 (chest of drawers)
#    - both agents died on API errors mid-response."
# So the two API failures ARE identified. The other four were not analysed for different or
# unrecorded reasons, and v1 dispatched 20 agents for 24 candidates without recording why.
#
# RIGHTS CLASSES. v3 said the V&A object was "the only non-open-access candidate". Also
# wrong: Brooklyn's stored rightsType is "Creative Commons 3D", which is a noncommercial
# licence family, not unrestricted reuse. Rights are now an explicit class per candidate.
RIGHTS_CLASS = {
 "unrestricted": "unrestricted / open production use",
 "attribution": "attribution required",
 "noncommercial": "noncommercial or otherwise restricted - NOT equivalent to open reuse",
 "conflict_or_restricted": "conflicting rights statements - treated as restricted until an "
                           "authoritative licence resolves the contradiction",
 "reference_only": "page-visible reference only",
 "unknown": "unknown",
}


# EXPLICIT, not sniffed. The first attempt pattern-matched the rights prose and got all 24
# wrong in the same direction: the V&A string contains the phrase "NOT open access and NOT
# CC0", so a substring test for "cc0" classed the one reference-only object as unrestricted.
# Rights are too consequential to infer from prose. Stated per candidate, with the verbatim
# field kept in candidates_v1.csv for anyone who wants to check the call.
# ALL 24 candidates, stated explicitly. No substring detection remains anywhere - the
# previous fallback still pattern-matched "CC0" in the prose, and prose matching is how the
# V&A negation slipped through the first time. The verbatim rights field stays in
# candidates_v1.csv for anyone auditing these calls.
_CC0 = ("unrestricted", "")
RIGHTS_BY_CANDIDATE = {
 "FC-01-1999_488": _CC0, "FC-01-2012_216": _CC0, "FC-02-67_230": _CC0,
 "FC-02-60_4_14": _CC0, "FC-03-2011_3": _CC0, "FC-03-30_120_59": _CC0,
 "FC-04-1972_47": _CC0, "FC-04-1969_262": _CC0, "FC-05-69_146_3": _CC0,
 "FC-06-69_146_2": _CC0, "FC-06-27_57_1": _CC0, "FC-07-1999_79": _CC0,
 "FC-08-69_140ab": _CC0, "FC-08-2019_59": _CC0, "FC-09-69_146_1": _CC0,
 "FC-09-1954_151": _CC0, "FC-10-1971_281": _CC0, "FC-10-1984_161": _CC0,
 "FC-11-10_125_133": _CC0, "FC-12-2002_298_1": _CC0, "FC-12-2002_298_2": _CC0,
 "FC-05-1974-224-1ab": ("conflict_or_restricted",
   'Philadelphia Museum of Art. The object record says "RightsType":"Public Domain" while '
   'the site-wide notice limits use to educational purposes and requests permission '
   'contact. That is a CONFLICT between two authority statements, not an attribution '
   'licence - classed restricted until an authoritative licence resolves it.'),
 "FC-07-76_63a-f": ("noncommercial",
   'Brooklyn Museum. Stored rightsType is "Creative Commons 3D" - a NONCOMMERCIAL licence '
   'family. NOT equivalent to the CC0 objects that make up most of the corpus.'),
 "FC-11-236:12-1869": ("reference_only",
   'V&A. "(c) Victoria and Albert Museum, London" on all 7 image assets. Rights are STATED, '
   'which satisfies the literal admission gate, but the images are neither open access nor '
   'CC0.'),
}
assert len(RIGHTS_BY_CANDIDATE) == 24


def rights_class(c):
    return RIGHTS_BY_CANDIDATE[c["candidate_id"]][0]


def rights_note(c):
    return RIGHTS_BY_CANDIDATE[c["candidate_id"]][1]


COVERAGE_NOTES = {
 "FC-02-60_4_14": ("api failure",
   "Met 60.4.14 footstool. IDENTIFIED BY v1 as one of the two BOM agents that died on API "
   "errors mid-response: 'FC-02-60_4_14 (Phyfe footstool)'. Candidate verified; only its "
   "component analysis is absent. 4 published views."),
 "FC-05-1974-224-1ab": ("page-only evidence",
   "Philadelphia Museum of Art 1974-224-1a,b. No open API; PAGE-VERIFIED ONLY, view count "
   "counted by eye. Never dispatched - an API-path BOM agent had nothing to call."),
 "FC-06-27_57_1": ("api failure",
   "Met 27.57.1 chest of drawers. IDENTIFIED BY v1 as the second BOM agent that died on API "
   "errors: 'FC-06-27_57_1 (chest of drawers)'. 13 published views, the second-highest in "
   "the corpus - it is analysable and its absence is purely the agent failure."),
 "FC-07-76_63a-f": ("page-only evidence",
   "Brooklyn Museum 76.63a-f. No open API; PAGE-VERIFIED ONLY. Never dispatched. NOTE its "
   "rights class: stored rightsType is 'Creative Commons 3D', a NONCOMMERCIAL family - not "
   "equivalent to the CC0 objects that make up most of the corpus."),
 "FC-11-236:12-1869": ("page-only evidence",
   "V&A 236:1, 2-1869. No open API; PAGE-VERIFIED ONLY. Never dispatched. Rights: "
   "'(c) Victoria and Albert Museum, London' on all 7 image assets - reference only."),
 "FC-12-2002_298_2": ("not dispatched - reason unrecorded",
   "Met 2002.298.2, the SECOND of a matched PAIR of Vanderbilt House pedestals; .1 was "
   "analysed and contributes 19 BOM rows. Open API, re-verified, gate PASS, 8 views. v1 "
   "dispatched 20 agents for 24 candidates and did not record which four were left out or "
   "why; this is one of them. IMPORTANT: a matched pair is NOT two independent objects. If "
   ".2 is analysed later its rows must not raise any family's objects_unlocked, or one "
   "design gets counted twice as corroboration."),
}


# ---------------------------------------------------------------- 7. EMIT
def W(path, header, rows):
    with open(os.path.join(HERE, path), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)
    return len(rows)


def main():
    subs = build_rows()
    by_uid = {s["row_uid"]: s for s in subs}
    bom_all = load_bom()
    declared = validate(subs, by_uid)          # fail closed before anything is written
    out = {}

    # -- source-correct BOM
    out["bom_v3.csv"] = W("bom_v3.csv",
      ["bom_row_id","row_uid","candidate_id","accession","broad_component","source_term",
       "subpart","count_as_recorded","evidence_class","evidence_weight","v1_reuse_class",
       "split_because","variation_axis"],
      [[s["bom_row_id"], s["row_uid"], s["candidate_id"], s["accession"], s["component"],
        s["source_term"], s["subpart"], s["count"], s["evidence"],
        W_EVID.get(s["evidence"], 0.0), s["reuse_class"], s["split_because"],
        s["variation_axis"]] for s in subs])

    # -- taxonomy
    out["refined_taxonomy_v3.csv"] = W("refined_taxonomy_v3.csv",
      ["family_id","lane","status","broad_component","reuse_class","n_member_subrows",
       "construction_invariant","geometry_generator","host_interface","joint_schedule",
       "assembly_role","surface_realization","permitted_variation","incompatible_excluded",
       "member_row_uids","proven_construction_rows","typed_uncertainty","donor_status",
       "note"],
      [[f["fid"], f["lane"], f["status"], f["broad"], f["reuse"], len(f["members"]),
        f["invariant"],
        f["generator"], f["host"], f["joints"], f["role"], f["surface"], f["variation"],
        f["excluded"], ";".join(f["members"]),
        ";".join(sorted(f["proven"] & set(f["members"]))),
        ";".join("%s=%s" % (k, v) for k, v in sorted(f["uncert"].items())),
        f["donor"], f["note"]]
       for f in FAMILIES])

    # -- BOM-row -> family assignment (the identity table the audit demanded)
    arows = []
    for f in FAMILIES:
        for u in f["members"]:
            s = by_uid.get(u)
            if not s:
                continue
            ut = f["uncert"].get(u, "")
            conf = ("proven" if u in f["proven"] else
                    ("unresolved" if ut == "construction" else "asserted"))
            arows.append([s["bom_row_id"], u, s["candidate_id"], s["component"],
                          s["source_term"], s["subpart"], s["evidence"],
                          W_EVID.get(s["evidence"], 0.0), f["fid"], f["status"], conf,
                          "yes" if u in f["proven"] else "no", ut,
                          PANEL_IFACE.get(u, "")])
    out["bom_row_family_assignments_v3.csv"] = W("bom_row_family_assignments_v3.csv",
      ["bom_row_id","row_uid","candidate_id","broad_component","source_term","subpart",
       "evidence_class","evidence_weight","family_assignment","family_status",
       "assignment_confidence","source_visible_construction","uncertainty_type",
       "panel_interface_class"], arows)

    # -- matrix (families x candidates)
    cands = sorted(set(s["candidate_id"] for s in subs))
    mrows = []
    for f in FAMILIES:
        cell = {}
        for u in f["members"]:
            s = by_uid.get(u)
            if not s:
                continue
            w = W_EVID.get(s["evidence"], 0.0)
            c = s["candidate_id"]
            if w == 0.0:
                cell[c] = cell.get(c) or "i"
            else:
                cell[c] = max(cell.get(c, 0) if isinstance(cell.get(c), float) else 0.0, w)
        mrows.append([f["fid"]] + [("" if c not in cell else
                                    (cell[c] if isinstance(cell[c], str)
                                     else ("1" if cell[c] == 1.0 else "0.5")))
                                   for c in cands])
    out["refined_component_matrix_v3.csv"] = W("refined_component_matrix_v3.csv",
      ["family_id"] + cands, mrows)

    # -- tally
    trows = []
    for f in FAMILIES:
        sc = score(f, by_uid)
        ec = defaultdict(int)
        for u in f["members"]:
            s = by_uid.get(u)
            if s:
                ec[s["evidence"]] += 1
        trows.append([f["fid"], f["lane"], f["status"], sc["n_subrows"], ec["VISIBLE"],
                      ec["PARTIALLY VISIBLE"], ec["INFERRED FROM TYPE"],
                      ec["NOT DETERMINABLE"], sc["weighted_evidence"],
                      sc["objects_unlocked"], sc["rankability"]])
    trows.sort(key=lambda r: -r[8])
    out["evidence_weighted_tally_v3.csv"] = W("evidence_weighted_tally_v3.csv",
      ["family_id","lane","status","n_member_subrows","n_visible","n_partially_visible",
       "n_inferred","n_not_determinable","weighted_evidence","objects_unlocked",
       "rankability"], trows)

    # -- lane-separated ranking with full decomposition
    rrows = []
    for f in FAMILIES:
        sc = score(f, by_uid)
        rrows.append([f["lane"], f["fid"], f["status"], repr(sc["weighted_evidence"]),
                      sc["objects_unlocked"], sc["n_proven"], sc["n_subrows"],
                      sc["n_construction_uncertain"], repr(sc["construction_confidence"]),
                      repr(sc["ambiguity_penalty"]), sc["reuse_class"],
                      repr(sc["reuse_multiplier"]), repr(sc["confidence_factor"]),
                      repr(sc["ambiguity_factor"]), repr(sc["breadth_factor"]),
                      repr(sc["rankability_factor"]), repr(sc["final_score"]),
                      sc["rankability"],
                      sc["other_uncertainty"]])
    LANE_ORDER = {"toolkit_capability": 0, "component_recipe": 1, "assembly_grammar": 2,
                  "bespoke_hero": 3}
    rrows.sort(key=lambda r: (LANE_ORDER.get(r[0], 9), -float(r[16])))
    out["ranking_v3.csv"] = W("ranking_v3.csv",
      ["lane","family_id","status","weighted_evidence","objects_unlocked",
       "n_proven","n_member_subrows","n_construction_uncertain","construction_confidence",
       "ambiguity_penalty","reuse_class","reuse_multiplier","confidence_factor",
       "ambiguity_factor","breadth_factor","rankability_factor","final_score",
       "rankability","non_construction_uncertainty"], rrows)

    # -- donors
    out["donor_compatibility_matrix_v3.csv"] = W("donor_compatibility_matrix_v3.csv",
      ["donor","kind","located_at","headline_status","per_family_relations",
       "axes_compared","finding"],
      [[d["donor"], d["kind"], d["located"], d["status"], d.get("relations", ""),
        d["compared"], d["finding"]] for d in DONORS])

    # -- non-assets
    out["non_asset_constraints_v3.csv"] = W("non_asset_constraints_v3.csv",
      ["item","kind","candidate","source_rows","source_statement","disposition"],
      [[x["item"], x["kind"], x["candidate"], ";".join(x["rows"]), x["statement"],
        x["disposition"]] for x in NON_ASSETS])

    # -- coverage
    cand_rows = list(csv.DictReader(open(os.path.join(CORPUS, "candidates_v1.csv"))))
    bomids = set(r["candidate_id"] for r in bom_all)
    crows = []
    for c in cand_rows:
        cid = c["candidate_id"]
        if cid in bomids:
            n = sum(1 for r in bom_all if r["candidate_id"] == cid)
            ns = sum(1 for s in subs if s["candidate_id"] == cid)
            crows.append([c["slot"], cid, c["accession"], c["museum"], "analysed", n, ns,
                          c["view_count"], c["verification"], rights_class(c),
                          RIGHTS_CLASS[rights_class(c)], rights_note(c), ""])
        else:
            st, why = COVERAGE_NOTES[cid]
            crows.append([c["slot"], cid, c["accession"], c["museum"], st, 0, 0,
                          c["view_count"], c["verification"], rights_class(c),
                          RIGHTS_CLASS[rights_class(c)], rights_note(c), why])
    out["candidate_coverage_report_v3.csv"] = W("candidate_coverage_report_v3.csv",
      ["slot","candidate_id","accession","museum","coverage_status","n_bom_rows",
       "n_analytical_subrows","view_count","verification_method","rights_class",
       "rights_class_meaning","note"], crows)

    # -- unresolved construction report
    urows = []
    seen = set()
    for f in FAMILIES:
        for u in sorted(f["unresolved"] & set(f["members"])):
            s = by_uid.get(u)
            if not s:
                continue
            urows.append([u, s["candidate_id"], s["component"], f["fid"], s["evidence"],
                          W_EVID.get(s["evidence"], 0.0), f["host"], f["joints"]])
            seen.add(u)
    for u, t, why in QUARANTINE:
        if u in seen:
            continue
        sq = by_uid[u]
        urows.append([u, sq["candidate_id"], sq["component"], "QUARANTINED (%s)" % t,
                      sq["evidence"], W_EVID.get(sq["evidence"], 0.0), "held out", why[:160]])
        seen.add(u)
    for s in subs:
        if s["row_uid"] in seen:
            continue
        if W_EVID.get(s["evidence"], 0.0) == 0.0:
            urows.append([s["row_uid"], s["candidate_id"], s["component"],
                          "UNASSIGNED - zero-weight evidence", s["evidence"], 0.0,
                          "n/a", "n/a"])
    out["unresolved_construction_report_v3.csv"] = W("unresolved_construction_report_v3.csv",
      ["row_uid","candidate_id","broad_component","family_or_status","evidence_class",
       "evidence_weight","family_host_interface","family_joint_schedule"], urows)

    # -- quarantine
    out["quarantined_rows_v3.csv"] = W("quarantined_rows_v3.csv",
      ["row_uid","candidate_id","broad_component","evidence_class","evidence_weight",
       "uncertainty_type","panel_interface_class","reason_held_out_of_production"],
      [[u, by_uid[u]["candidate_id"], by_uid[u]["component"], by_uid[u]["evidence"],
        W_EVID.get(by_uid[u]["evidence"], 0.0), t, PANEL_IFACE.get(u, ""), why]
       for u, t, why in QUARANTINE])

    # -- v2 -> v3 correction ledger + reconciliation
    import importlib.util
    spec = importlib.util.spec_from_file_location("t2", os.path.join(CORPUS, "v2",
                                                                     "taxonomy_v2.py"))
    m2 = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(m2)
    except SystemExit:
        pass
    v2 = m2.FAMILIES
    led = []
    for f in v2:
        broken = [c for k in ("members", "inferred_members") for c in f.get(k, [])
                  if c in ID_FIXES]
        for c in sorted(set(broken)):
            led.append(["TRUNCATED_ID", f["name"], c, ID_FIXES[c],
                        "%d membership(s) in this family cited an ID absent from the BOM; any "
                        "BOM join dropped them silently" % broken.count(c)])
    v2names = set(f["name"] for f in v2)
    v3names = set(f["fid"] for f in FAMILIES)
    for n in sorted(v2names - v3names):
        led.append(["FAMILY_RETIRED_OR_SPLIT", n, "", "",
                    "not carried into v3 under this name - split, renamed, or demoted to a "
                    "non-asset; see refined_taxonomy_v3.csv and non_asset_constraints_v3.csv"])
    for n in sorted(v3names - v2names):
        led.append(["FAMILY_NEW_IN_V3", "", n, "",
                    "created by a construction-compatibility split or by row-level "
                    "reassignment"])
    led.append(["GRANULARITY", "candidate-level membership", "BOM-row-level membership", "",
                "v2 made %d candidate-level assignments; v3 makes %d subrow-level assignments"
                % (sum(len(f.get("members", [])) + len(f.get("inferred_members", []))
                       for f in v2),
                   sum(len(f["members"]) for f in FAMILIES))])
    led.append(["DONOR_PREMISE", "only rough_hewn_timber_beam_v1 is geometry",
                "inspected inventory of %d entries" % len(DONORS),
                "v2 read directory layout instead of inspecting; geometry builds exist inside "
                "the material assets"])
    out["v2_to_v3_correction_ledger.csv"] = W("v2_to_v3_correction_ledger.csv",
      ["correction_type","v2_value","v3_value","extra","reason"], led)

    return out, subs, by_uid


if __name__ == "__main__":
    out, subs, by_uid = main()
    for k, v in sorted(out.items()):
        print("%-46s %4d rows" % (k, v))

    BROAD = {"foot", "post", "rail", "rear leg/stile", "brace", "panel frame",
             "floating panel", "crest/cornice", "moulding run",
             "repeated ornament master", "inlay path"}
    assigned = set(u for f in FAMILIES for u in f["members"])
    quar = set(q[0] for q in QUARANTINE)
    nonasset = set(r for x in NON_ASSETS for r in x["rows"])
    tgt = [x for x in subs if x["component"] in BROAD]

    # EXCLUSIVE partition, in priority order, so the four buckets sum to the total.
    # v3 published overlapping counts (R446 was both zero-weight and non-asset) and
    # presented them as a partition. They are exclusive now.
    b_assigned = [x for x in tgt if x["row_uid"] in assigned]
    rest = [x for x in tgt if x["row_uid"] not in assigned]
    b_nonasset = [x for x in rest if x["row_uid"] in nonasset]
    rest = [x for x in rest if x["row_uid"] not in nonasset]
    b_quar = [x for x in rest if x["row_uid"] in quar]
    rest = [x for x in rest if x["row_uid"] not in quar]
    b_zero = [x for x in rest if W_EVID.get(x["evidence"], 0.0) == 0.0]
    b_unex = [x for x in rest if W_EVID.get(x["evidence"], 0.0) > 0.0]

    print("\n--- disposition of the 11 broad categories (EXCLUSIVE partition) ---")
    print("  assigned to a family      %4d" % len(b_assigned))
    print("  non-assets                %4d" % len(b_nonasset))
    print("  quarantined               %4d" % len(b_quar))
    print("  zero-weight notes only    %4d" % len(b_zero))
    print("  UNEXPLAINED               %4d %s" % (len(b_unex),
                                                  [x["row_uid"] for x in b_unex]))
    print("  ---------------------------------")
    print("  total target subrows      %4d" % len(tgt))
    assert len(b_assigned) + len(b_nonasset) + len(b_quar) + len(b_zero) + len(b_unex) \
        == len(tgt)
    assert not b_unex, "unexplained rows present"

    # split-count vocabulary, stated exactly (v3 conflated these)
    from collections import Counter as _C
    par = _C(p for p, _s, _sp, _e, _b in SUBROWS)
    print("\n--- split accounting ---")
    print("  parent rows given suffixed identities : %d" % len(par))
    print("  parents expanded into >1 subrow       : %d"
          % sum(1 for k, v in par.items() if v > 1))
    print("  parents renamed to a single subrow    : %d"
          % sum(1 for k, v in par.items() if v == 1))
    print("  analytical subrows created            : %d" % sum(par.values()))
    print("  total analytical subrows              : %d" % len(subs))

    tot = _C()
    for f in FAMILIES:
        for u in f["members"]:
            if u in by_uid:
                tot[by_uid[u]["evidence"]] += 1
    print("\nfamilies: %d   assignments: %d   %s" % (len(FAMILIES), sum(tot.values()),
                                                      dict(tot)))
    print("weighted evidence total:",
          round(sum(W_EVID.get(by_uid[u]["evidence"], 0.0)
                    for f in FAMILIES for u in f["members"] if u in by_uid), 2))
    ao = [f["fid"] for f in FAMILIES if f["status"] == "AUDIT_ONLY"]
    print("AUDIT_ONLY families: %d  %s" % (len(ao), ao))
