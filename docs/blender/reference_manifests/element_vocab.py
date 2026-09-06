"""Controlled element vocabulary for the building-dimension corpus.

WHY. Cycle 39 left ~1450 rows across six manifests with 1039 distinct free-text `element`
strings, half of them occurring exactly once. Nothing can be queried across that. Worse, a naive
roll-up mis-buckets: a "window sill height above floor" read as a room height gave Hoghton Tower a
2.1 m great hall.

HOW. A MAPPING TABLE, not a rewrite. The source manifests keep their own wording - that wording is
evidence and several rows carry qualifications inside it ("wall thickness at the point a workman
pierced it in 27 days"). This module adds a canonical column alongside.

ORDER MATTERS. Rules are tried in sequence and the first match wins, so the specific ones must
come before the general. "sill height above floor" must beat "height"; "wall thickness" must beat
"thickness"; "roof span" must beat "span".
"""
import re

# canonical -> ordered list of patterns. First canonical whose pattern matches, wins.
RULES = [
 # --- openings and sills: MUST precede the generic height/width rules
 ("sill_height_above_floor", r"sill.*(above|from|off).*(floor|ground)|window.*sill height|"
                             r"appui|sill height|lowered sill|windows? .*(above the floor|from the floor)"),
 ("opening_height",  r"(door|gate|portal|passage|postern|window|light|loop|archer|meurtri|crenel|embrasure)"
                     r".{0,26}(height|high)|height.{0,20}(of the )?(door|gate|opening|window)"),
 ("opening_width",   r"(door|gate|portal|passage|postern|window|light|loop|archer|meurtri|crenel|embrasure|"
                     r"gallery|slit|rainure)\b.{0,26}(width|wide|opening|clear)|"
                     r"(width|opening).{0,20}(of the )?(door|gate|window|light|crenel|loop)"),
 # --- stair members
 ("tread_depth",     r"\btread\b|giron|emmarch|breadth of (the )?steps?|step (breadth|width)"),
 ("riser_height",    r"\briser\b|rise of (a|each|the) step|step (height|rise)"),
 ("stair_width",     r"stair(case)?.{0,24}(width|wide|clear width)|width.{0,16}stair|flight width"),
 ("newel_diameter",  r"newel|noyau|\bvis\b.{0,20}(diameter|cage)"),
 ("headroom",        r"headway|headroom"),
 # --- structure
 ("wall_thickness",  r"wall.{0,18}thick|thickness.{0,18}wall|epaisseur|reveal depth|parapet.{0,16}thick|"
                     r"merlon.{0,16}thick|shell wall"),
 ("member_section",  r"scantling|equarrissage|équarrissage|squared section|\bmoise|solive|beam.{0,16}section"),
 ("roof_span",       r"roof span|span of (the )?roof|truss span"),
 ("roof_pitch",      r"pitch|inclination|slope of (the )?roof"),
 ("bay_spacing",     r"spacing|bay\b|entraxe|centres|between towers|per bay|apart\b"),
 ("vault_span",      r"vault.{0,16}(span|width)|span.{0,16}vault|berceau.{0,16}larg"),
 ("projection",      r"projection|saillie|overhang|jetty|corbel.{0,16}project|encorbell"),
 ("diameter",        r"diameter|diam[eè]tre|\bround\b.{0,14}(size)?"),
 ("storey_height",   r"storey|stor(y|ey) height|floor.to.(floor|ceiling)|floor to plafond|"
                     r"height of the (ground|first|upper|timber)"),
 # --- room envelope: generic, so LAST among the dimensional rules
 ("internal_length", r"internal length|inside length|length .{0,14}(dans oeuvre|internal|inside|clear)|"
                     r"^length$|length long|\blong\b.{0,10}$|hall length|nave length"),
 ("internal_width",  r"internal width|inside width|width .{0,14}(dans oeuvre|internal|inside|clear)|"
                     r"^width$|hall width|nave width|aisle width"),
 ("internal_height", r"height to (the )?(ceiling|wall.?plate|springing|plafond|arch|roof)|internal height|"
                     r"floor to ridge|height to ridge|clear height|ceiling height|^height$"),
 ("external_length", r"external length|overall length|outside length"),
 ("external_width",  r"external width|overall width|outside width"),
 ("external_height", r"external height|overall height|total height|height above (the )?(ground|ditch|"
                     r"counterscarp|fossé)"),
 ("depth",           r"\bdepth\b|\bdeep\b|well depth"),
 ("thickness",       r"thick"),
 ("length",          r"length|\blong\b"),
 ("width",           r"width|\bwide\b|largeur"),
 ("height",          r"height|\bhigh\b|hauteur"),
 # --- forms found in the unmapped tail (cycle 40), each a real group not an oddity
 # "purlin at A, larger profile, across" / "octagonal shaft, across the flats" - a member's
 # cross-dimension, measured across the face. Same class as a scantling.
 ("member_section", r",\s*across\b|across the flats|\bacross$"),
 # "floor to top of wall-plate" - my first pattern demanded "height to", which this does not say.
 ("internal_height", r"floor to (the )?(top of|underside of|springing|wall.?plate|plate|ridge|apex|"
                     r"crown|soffit)"),
 ("gap",             r"clear gap|gap between|between paired|clear distance between"),
 # The source itself declares these unresolved. That is data about the source, not a mapping failure,
 # so it gets a name rather than being left to look like a gap in this table.
 ("unresolved_in_source", r"unassigned|not resolved|to be re-checked|broken line|terminations|"
                          r"leader assignment"),
 # A bare room name in the element column, with the size in the value ("audit room" = 20 x 15 ft).
 ("room_footprint",  r"^\s*(the )?(audit room|quadrangle|refectory|dorter|frater|cloister|kitchen|"
                     r"buttery|pantry|larder|chapel|oratory|parlour|parlor|solar|garderobe|court|"
                     r"courtyard|undercroft|cellar|crypt|barn|granary|stable|hall|room|chamber|"
                     r"gallery|porch|lodge|study|closet)s?\s*$"),
 ("putlog_hole",     r"putlog|trou de hourd|hoarding.{0,16}(hole|socket)|socket"),
 ("gable_rise",      r"gable rise|wall.?plate to ridge|ridge down to"),
 ("drop_hole",       r"drop.?hole|mâchicoulis.{0,16}(hole|trou)|machicolation.{0,16}hole"),
 ("distance",        r"^distance|clear distance"),
 ("span",            r"\bspan\b"),
 # THE LONG TAIL, and it is one thing: individual members measured on a plate. Dollman and Brandon
 # dimension every board, post, jamb and boss separately, e.g. "barge board of Gable A, second
 # figure" or "jamb of Window F on Plate 1". These are NOT building-envelope dimensions and should
 # not sit in the same bucket as a hall width - a generator wants them, but for detailing, not for
 # massing. Kept as a named class so the count is visible rather than looking like a mapping gap.
 ("member_detail",   r"barge ?board|\bpost\b|\bjamb\b|\bcill\b|\bsill\b|baluster|\brib\b|"
                     r"mullion|transom|corbel|\bboss\b|finial|crocket|label|bracket|shaft|"
                     r"\bmember\b|moulding|\bmould\b|nosing|string.?course|hearth|standard|"
                     r"\bfigures?\b|detail|\bplate\b|\bstep stone|purlin|rafter|\bbeam\b|"
                     r"\btie\b|strut|brace|\bplate\b"),
 # VERTICAL RELATIONSHIPS between two parts, not a dimension of either. A real class and a useful
 # one for a game about climbing and falling: "drop from the machicolation walk to the projectile
 # floor", "relief of the inner enceinte above the outer chemin de ronde", "level difference
 # between two adjoining curtain walks", "command over the neighbouring parapet crest".
 ("level_difference", r"\bdrop\b|relief of|level difference|command over|\brise of\b|"
                      r"above the (outside|neighbouring|courtyard|adjoining)|below the (courtyard|"
                      r"sill|ground)|from the .{0,30} to the |sunk below|raised above"),
 ("perimeter",        r"perimeter|circuit|round the ring|across the ring"),
 ("weight",           r"weight|kilog|\blbs?\b|pesait"),
 ("room_footprint",   r"\b(sitting room|bed ?room|school ?room|dining hall|central court|audit room|"
                      r"great chamber|withdrawing room|long gallery)\b|"
                      r"\b(room|hall|court|chamber|parlour|parlor|schoolroom)\s*$"),
 ("external_length",  r"whole pile.{0,20}(north.south|east.west|long)|overall.{0,12}(north|east)"),
 # --- non-dimensional
 ("count",           r"\bnumber\b|count|per curtain|storeys?\b|steps? per|courses? per|holes? per|"
                     r"^\d+ \+ \d+$|divided into"),
 ("ratio_or_rule",   r"ratio|proportion|rule|parity|principle|constant|product|formula|:\s?1\b"),
]
COMPILED = [(c, re.compile(p, re.I)) for c, p in RULES]
CANON = [c for c, _ in RULES]

def canon(element):
    """Free-text element string -> canonical name, or None if nothing matches."""
    e = (element or "").strip()
    if not e:
        return None
    for c, rx in COMPILED:
        if rx.search(e):
            return c
    return None
