"""Generator for english_vernacular_dimensions_v1.tsv — Addy, *The Evolution of the
English House* (1898), IA `evolutionenglis00addygoog`. Cycle 48.

WHY A GENERATOR AND NOT A HAND-TYPED TSV. Every metre value here is computed from the
feet/inches pair printed in the book, so a transcription slip shows up as a wrong FOOT
value next to its own quote rather than as a plausible metre value with no witness.
Cycle 40 lost two numbers to hand-carried metres; this closes that route.

OFFSET. len(pageindex) == imagecount == 263, so leaf N is image N — the trusted branch
of the iabook.py rule. Printed page = leaf - 34, checked at five widely separated leaves
(n59/25, n101/67, n137/103, n164/130, n192/158).

THE FRACTION TRAP ON THIS SCAN (cycle 48). The OCR layer of this item contains ZERO
occurrences of the glyphs (half) (quarter) (three-quarters) across all 263 leaves, yet the
book demonstrably prints them — leaf n166 carries three of them in two lines. So every
vulgar fraction is destroyed in the text layer, arriving as junk ("33^", "2\\", "5^",
"6i", "9!"). engdim.DIM requires the number to sit immediately against its unit, so
"33^ feet" matches NOTHING: the failure mode is SILENT OMISSION, not silent corruption.
That is the safer of the two, but it means the density scan UNDER-counts, and a hall
printed as "20 ft by 15 (half) ft" comes back as one dimension rather than a pair.
Fractions lost with no junk left behind are undetectable from the text layer at all.
Consequence for this manifest: any row whose value carries a fraction is either
`verified_full_res=yes` (read off the page image) or evidence `ocr-uncertain`.

EVIDENCE CLASSES used here, beyond the corpus vocabulary:
    documentary-survey  a contemporary written survey of a building that no longer
                        stands — here the twelfth-century *Domesday of St Paul's*, which
                        states Kensworth and Walton in Roman numerals and Latin. These
                        are not Addy's measurements and not a reconstruction; they are a
                        medieval clerk's. Kept separate because their error mode is
                        scribal, not metrological.
    secondary           Addy quoting another author on a building he did not himself see.
"""
FT, IN = 0.3048, 0.0254

HEADER = ("ia_id\tsource\tbuilding\tplace\tperiod\telement\tvalue\tunit\tvalue_m\t"
          "leaf\tprinted_page\tevidence\tverified_full_res\tquote")

IA = "evolutionenglis00addygoog"
SRC = "Addy, The Evolution of the English House (1898)"

# (building, place, period, [(element, ft, in, unit_note, evidence, verified, quote)], leaf)
ROWS = []


def add(building, place, period, leaf, evidence, quote, dims, verified="no"):
    for el, ft, inch, unit in dims:
        ROWS.append((building, place, period, el, ft, inch, unit, leaf, evidence,
                     verified, quote))


# ---------------------------------------------------------------- Kensworth (n164-165)
K_Q = ("consisted of a hall 35 feet long, 30 feet broad, and 22 feet high, viz., 11 feet "
       "to the tie-beams, and 11 feet from the tie-beams to the ridge-tree. ... The "
       "\"house\" was 12 feet long and 17 feet broad; it was 17 feet high, viz, 10 feet to "
       "the tie-beams, and 7 feet from the tie-beams to the ridge-tree. The bower was 22 "
       "feet long and 16 broad; it was 18 feet high, viz., 9 feet to the tie-beams, and 9 "
       "feet from the tie-beams to the ridge-tree.")
K_LAT = ("Halla hujus manerii habet xxxv. pedes in longitud', xxx. ped' in latitud', et "
         "xxii. in altit', xi. sub trabibus. et xi. desuper. Domus, que est inter hallam "
         "et talamum, habet xii. pedes in longitud', xvii. in latit' et xvii. in "
         "altitudine, x. sub trabibus et vii. de'super. Thalamus habet xxii. pedes in "
         "longit', xvi. in latitud', xviii. in altitud', ix. sub trabibus et ix. desuper. "
         "— Domesday of St. Paul's, p. 129")

add("Kensworth manor house — hall", "Kensworth, Bedfordshire (hist. Hertfordshire)",
    "XIIc", 164, "documentary-survey", K_Q + "  [Latin: " + K_LAT + "]",
    [("internal length", 35, 0, "ft"),
     ("internal breadth", 30, 0, "ft"),
     ("height, floor to ridge-tree", 22, 0, "ft"),
     ("height, floor to tie-beams", 11, 0, "ft"),
     ("height, tie-beams to ridge-tree", 11, 0, "ft")], verified="yes")

add("Kensworth manor house — \"house\" (domus, the entry between hall and bower)",
    "Kensworth, Bedfordshire (hist. Hertfordshire)", "XIIc", 164, "documentary-survey",
    K_Q + "  [Latin: " + K_LAT + "]",
    [("internal length", 12, 0, "ft"),
     ("internal breadth", 17, 0, "ft"),
     ("height, floor to ridge-tree", 17, 0, "ft"),
     ("height, floor to tie-beams", 10, 0, "ft"),
     ("height, tie-beams to ridge-tree", 7, 0, "ft")], verified="yes")

add("Kensworth manor house — bower (thalamus, women's apartment)",
    "Kensworth, Bedfordshire (hist. Hertfordshire)", "XIIc", 164, "documentary-survey",
    K_Q + "  [Latin: " + K_LAT + "]",
    [("internal length", 22, 0, "ft"),
     ("internal breadth", 16, 0, "ft"),
     ("height, floor to ridge-tree", 18, 0, "ft"),
     ("height, floor to tie-beams", 9, 0, "ft"),
     ("height, tie-beams to ridge-tree", 9, 0, "ft")], verified="yes")

add("Kensworth manor house — ox-house", "Kensworth, Bedfordshire (hist. Hertfordshire)",
    "XIIc", 164, "documentary-survey",
    "Besides these rooms there was an ox-house 33 feet long, 12 feet broad, and 13 feet high.",
    [("internal length", 33, 0, "ft"),
     ("internal breadth", 12, 0, "ft"),
     ("height", 13, 0, "ft")], verified="yes")

add("Kensworth manor house — sheep-cote", "Kensworth, Bedfordshire (hist. Hertfordshire)",
    "XIIc", 165, "documentary-survey",
    "also a sheep-cote, 39 feet long, 12 feet broad, and 22 feet high, with a lamb-cote, "
    "24 feet long, 12 feet broad, and 12 feet high.",
    [("internal length", 39, 0, "ft"),
     ("internal breadth", 12, 0, "ft"),
     ("height", 22, 0, "ft")])

add("Kensworth manor house — lamb-cote", "Kensworth, Bedfordshire (hist. Hertfordshire)",
    "XIIc", 165, "documentary-survey",
    "also a sheep-cote, 39 feet long, 12 feet broad, and 22 feet high, with a lamb-cote, "
    "24 feet long, 12 feet broad, and 12 feet high.",
    [("internal length", 24, 0, "ft"),
     ("internal breadth", 12, 0, "ft"),
     ("height", 12, 0, "ft")])

# ---------------------------------------------------------------- Walton great barn (n166)
W_Q = ("the great barn of the manor house at Walton was 168 feet long, 53 feet wide, and "
       "33 1/2 feet high, viz., 21 1/2 feet to the tie-beams, and 12 feet from them to the "
       "ridge-tree.  [Latin: Magnum orreum Walentonie habet x. perticas et dimid' in "
       "longitudine (et pertica est de xvi. pedibus) et in latitudine iii. perticas et v. "
       "pedes, et in altitudine sub trabe xxi. ped' et dimid', et desursum trabe xii. ped'. "
       "— Domesday of St. Paul's, p. 130]")
add("Walton manor house — great barn", "Walton (manor of St Paul's)", "XIIc", 166,
    "documentary-survey", W_Q,
    [("internal length", 168, 0, "ft (= 10 1/2 perches of 16 ft)"),
     ("internal breadth", 53, 0, "ft (= 3 perches + 5 ft)"),
     ("height, floor to ridge-tree", 33, 6, "ft in"),
     ("height, floor to tie-beams", 21, 6, "ft in"),
     ("height, tie-beams to ridge-tree", 12, 0, "ft")], verified="yes")

# ---------------------------------------------------------------- Gunthwaite barn (n166)
G_Q = ("A large barn with a nave and two aisles adjoining Gunthwaite Hall, near Penistone, "
       "still remains. It is 165 feet long, 43 feet broad, and 30 feet high, viz., 15 feet "
       "to the tie-beams, and 15 feet from them to the ridge-tree. It consists of 11 bays "
       "of 15 feet each in length. It has two rows of wooden pillars, each measuring 14 "
       "inches by 9, and standing on stone pedestals. The length of the tie-beams from one "
       "pillar to another is 23 feet. The roof is in a single span extending across the "
       "whole breadth. The building is of timber framework filled up with stonework to the "
       "height of 8 ft 9 in. There are six barn doors.")
add("Gunthwaite Hall barn — aisled timber barn (nave + two aisles)",
    "Gunthwaite, near Penistone, South Yorkshire", "medieval, extant", 166, "measured", G_Q,
    [("internal length", 165, 0, "ft"),
     ("internal breadth", 43, 0, "ft"),
     ("height, floor to ridge-tree", 30, 0, "ft"),
     ("height, floor to tie-beams", 15, 0, "ft"),
     ("height, tie-beams to ridge-tree", 15, 0, "ft"),
     ("structural bay length", 15, 0, "ft"),
     ("arcade pillar section, larger face", 0, 14, "in"),
     ("arcade pillar section, smaller face", 0, 9, "in"),
     ("tie-beam length, pillar to pillar (aisle-post spacing across)", 23, 0, "ft"),
     ("stonework infill height in timber frame (plinth)", 8, 9, "ft in")], verified="yes")

# ---------------------------------------------------------------- Cholsey barn (n166, secondary)
add("Cholsey barn (demolished)", "Cholsey, Berkshire", "medieval", 166, "secondary",
    "There was formerly a barn at Cholsey, in Berkshire, 303 feet long and 51 feet high. "
    "The pillars were four yards in circumference. — Parker's Glossary of Arch., 1850, p. 241 "
    "[FLAG: '51 feet high' is what Addy prints; the usual published figure for Cholsey is a "
    "WIDTH of about 54 ft, so this may be Parker's or Addy's slip for 'wide'. Recorded as "
    "printed, not corrected.]",
    [("internal length", 303, 0, "ft"),
     ("height (as printed — see flag)", 51, 0, "ft"),
     ("pillar circumference", 12, 0, "ft (= 4 yards)")])

# ---------------------------------------------------------------- mud house (n72)
M_Q = ("A good example of a mud house, not now occupied, may be seen at Great Hatfield, "
       "Mappleton, in East Yorkshire. The outside length is 28 ft, and the inside breadth "
       "15 ft. 2 in. The height to the eaves, which project ten inches, is 6 ft. 2 in. The "
       "mud walls are 1 ft. 7 in. thick. The house has one door, 3 ft 3 in. wide, facing "
       "south... The length of the \"speer\" is four feet, so that it just covers the door. "
       "Its height is 6 ft. 2 in.")
add("Great Hatfield mud house (single-storey cot, open hearth)",
    "Great Hatfield, Mappleton, East Yorkshire", "vernacular, extant 1898", 72, "measured", M_Q,
    [("external length", 28, 0, "ft"),
     ("internal breadth", 15, 2, "ft in"),
     ("height to eaves", 6, 2, "ft in"),
     ("eaves projection", 0, 10, "in"),
     ("mud wall thickness", 1, 7, "ft in"),
     ("door width", 3, 3, "ft in"),
     ("\"speer\" (hearth screen) length", 4, 0, "ft"),
     ("\"speer\" (hearth screen) height", 6, 2, "ft in")])

# ---------------------------------------------------------------- coit (n104)
C_Q = ("At Rushy Lee... is one of those numerous buildings known in Yorkshire as \"coits\"... "
       "The larger door, 6 ft. high and 4 ft. 8 in. wide, is the main entrance to the "
       "building... Its length is forty-four, and its breadth thirty-seven feet. Four cows "
       "stood between each pair of wooden pillars, two in a stall.  [and n103: 'is built in "
       "bays of approximately fifteen feet in length']")
add("\"Coit\" at Rushy Lee — combined dwelling + shippon, basilical, aisled",
    "Rushy Lee, near Midhope, South Yorkshire", "vernacular, extant 1898", 104, "measured", C_Q,
    [("external length", 44, 0, "ft"),
     ("external breadth", 37, 0, "ft"),
     ("main (shippon) door height", 6, 0, "ft"),
     ("main (shippon) door width", 4, 8, "ft in"),
     ("structural bay length", 15, 0, "ft (approximately)")])

# ---------------------------------------------------------------- Padley (n171, n174)
P_BUT = ("[buttery] measures 15 by 17 feet, and, measured up to the original beams of the "
         "floor, is 12 feet high. It contains one window, 2 ft. 7 in. by 2 ft. 5 in., "
         "looking into the quadrangle... Within the buttress, about two feet from the "
         "ground, is an opening 2 ft. 4 in. square, finished by dressed stones, which may "
         "have served for an ambry... surrounded on its outer sides by walls nearly three "
         "feet thick")
add("Padley Hall — buttery (ground floor)", "Padley, Derbyshire", "medieval, ruin", 171,
    "measured", P_BUT,
    [("internal plan dimension A (order as printed)", 15, 0, "ft"),
     ("internal plan dimension B (order as printed)", 17, 0, "ft"),
     ("internal height to original floor beams", 12, 0, "ft"),
     ("window opening height", 2, 7, "ft in"),
     ("window opening width", 2, 5, "ft in"),
     ("ambry opening (square)", 2, 4, "ft in"),
     ("ambry sill height above floor", 2, 0, "ft (about)"),
     ("external wall thickness", 3, 0, "ft (nearly)")])

P_HALL = ("This room is 32 feet in length and 17 feet in breadth. Like the buttery it was "
          "12 feet high. It has a square-headed window in each of its three outer walls. "
          "The largest window, which faces inwards, is 2 ft. 7 in. by 2 ft. 4 in. The two "
          "other windows are only 2 ft. 6 in. by 1 ft.")
add("Padley Hall — hall (ground floor, chapel over)", "Padley, Derbyshire", "medieval, ruin",
    174, "measured", P_HALL,
    [("internal length", 32, 0, "ft"),
     ("internal breadth", 17, 0, "ft"),
     ("internal height", 12, 0, "ft"),
     ("principal window height", 2, 7, "ft in"),
     ("principal window width", 2, 4, "ft in"),
     ("secondary window height", 2, 6, "ft in"),
     ("secondary window width", 1, 0, "ft")])

# ---------------------------------------------------------------- Charney Bassett (n180)
CB_Q = ("A house at Charney-Basset near Wantage, in Berkshire, has a hall and buttery on "
        "the ground-floor, with a chamber and chapel on the floor above them... the chapel, "
        "which is only 12 ft. 5 in. by 9 ft. 10 in., is over the buttery... The hall "
        "beneath the chamber is 30 feet by 16")
add("Charney Bassett manor house — first-floor chapel", "Charney Bassett, Berkshire",
    "late XIIIc", 180, "measured", CB_Q,
    [("internal length", 12, 5, "ft in"),
     ("internal breadth", 9, 10, "ft in")])
add("Charney Bassett manor house — ground-floor hall", "Charney Bassett, Berkshire",
    "late XIIIc", 180, "measured", CB_Q,
    [("internal length", 30, 0, "ft"),
     ("internal breadth", 16, 0, "ft")])

# ---------------------------------------------------------------- Peveril keep (n192, n197, n198)
PK_Q = ("The height of this room was 17 feet to the \"square,\" and 27 feet to the ridge. "
        "The length is 22 feet, and the breadth 19 feet... This doorway, 4 ft. 9 in. wide, "
        "is surmounted on the outside by a relieving arch and tympanum. It is 8 ft. 6 in. "
        "above the present level of the ground outside.")
add("Peak (Peveril) Castle keep — upper apartment / living room", "Castleton, Derbyshire",
    "XIIc, extant", 192, "measured", PK_Q,
    [("internal length", 22, 0, "ft"),
     ("internal breadth", 19, 0, "ft"),
     ("height, floor to the \"square\" (wall-head)", 17, 0, "ft"),
     ("height, floor to ridge", 27, 0, "ft"),
     ("first-floor entrance doorway width", 4, 9, "ft in"),
     ("entrance doorway sill height above external ground", 8, 6, "ft in")])

add("Peak (Peveril) Castle keep — basement / ground-floor store", "Castleton, Derbyshire",
    "XIIc, extant", 198, "measured",
    "The height of this room, measured from the highest part of the ground within to the "
    "ledges which supported the floor above, was 12 feet, and 17 feet measured from the "
    "lowest part of the ground.",
    [("height, highest internal ground to floor-ledges", 12, 0, "ft"),
     ("height, lowest internal ground to floor-ledges", 17, 0, "ft")])

add("Peak (Peveril) Castle keep — \"sentry\" aperture above the roof", "Castleton, Derbyshire",
    "XIIc, extant", 197, "measured",
    "It is about 6 ft. 5 in. in depth and 4 ft 1 in. in breadth. The narrow loop-hole at "
    "the outer end of the aperture has been crossed horizontally by two iron bars, one at "
    "the height of 4 ft 7 in. above the floor, and the other a little above the floor.",
    [("aperture depth (through wall)", 6, 5, "ft in"),
     ("aperture breadth", 4, 1, "ft in"),
     ("upper iron bar height above aperture floor", 4, 7, "ft in")])

# ---------------------------------------------------------------- Irish oratory (n59)
O_Q = ("The \"oratory\" is composed of dry rubble masonry, and consists of a single "
       "rectangular chamber 15 ft. 3 in. long by 10 ft. wide inside. \"It has a flat-headed "
       "western doorway with inclining jambs, 5 ft. 10 in. high by 1 ft. 11 in. wide at the "
       "top, and 2 ft. 5 in. wide at the bottom inside... The outside aperture is 1 ft. 3 "
       "in. high by 9 1/2 in. wide at the top, and 10 in. at the bottom. The window measures "
       "on the inside 3 ft. 3 in. high by 1 ft 6 in. wide at the top, and 1 ft. 9 in. wide "
       "at the bottom. On the inside of the doorway, at a height of eight inches above the "
       "bottom of the lintel, is a projecting stone on each side, with a hole three inches "
       "square through it to receive the door-frame... The flags below these are 1 ft. 4 in. "
       "wide.\" — Arch. Cambr., vol. ix. (5th S.), 148")
add("\"Oratory\" of Gallarus — dry-rubble corbelled cell", "Gallarus, near Dingle, Co. Kerry, Ireland",
    "early medieval, extant", 59, "measured", O_Q,
    [("internal length", 15, 3, "ft in"),
     ("internal width", 10, 0, "ft"),
     ("doorway height", 5, 10, "ft in"),
     ("doorway width at head (inclining jambs)", 1, 11, "ft in"),
     ("doorway width at sill (inclining jambs)", 2, 5, "ft in"),
     ("window, external aperture height", 1, 3, "ft in"),
     ("window, external width at head", 0, 10, "in [OCR '9!' — printed value is probably "
                                                "9 1/2 in; 10 in recorded is the BOTTOM figure, "
                                                "see ocr-uncertain row]"),
     ("window, external width at sill", 0, 10, "in"),
     ("window, internal height", 3, 3, "ft in"),
     ("window, internal width at head", 1, 6, "ft in"),
     ("window, internal width at sill", 1, 9, "ft in"),
     ("door-frame socket (square section)", 0, 3, "in"),
     ("socket height above underside of lintel", 0, 8, "in"),
     ("ridge flag width", 1, 4, "ft in")])
ROWS.append(("\"Oratory\" of Gallarus — dry-rubble corbelled cell",
             "Gallarus, near Dingle, Co. Kerry, Ireland", "early medieval, extant",
             "window, external width at head", 0, 9.5, "in", 59, "ocr-uncertain", "no",
             "The outside aperture is 1 ft. 3 in. high by 9 1/2 in. wide at the top, and 10 in. "
             "at the bottom. [OCR renders '9! in.'; this scan destroys every fraction glyph, "
             "so 9 1/2 is the reading but it was NOT checked on the page image.]"))

# ---------------------------------------------------------------- booth + village forge (n56)
# The unit that Gallarus and the cruck houses are all copies of. Note the second sentence:
# the VILLAGE FORGE is stated at the same size, which is the only smithy footprint the
# corpus has from a contemporary document.
B_Q = ("c. 1350. \"Johannes Flesshewer ten. j botham de novo edificatam super vastum, "
       "longitudinis xx pedum et latitudinis xviij pedum.\" — Bishop Hatfield's Survey "
       "(Surtees Soc), p. 32. Another booth, as well as the village forge, is described as "
       "being of the same size.")
add("Booth (single-cell cruck cot) — newly built on the waste", "County Durham", "c. 1350",
    56, "documentary-survey", B_Q,
    [("length", 20, 0, "ft"),
     ("breadth", 18, 0, "ft")])
add("Village forge / smithy", "County Durham", "c. 1350", 56, "documentary-survey", B_Q,
    [("length", 20, 0, "ft (stated as the same size as the booth)"),
     ("breadth", 18, 0, "ft (stated as the same size as the booth)")])

# ---------------------------------------------------------------- Flannan Isles (n60)
F_Q = ("Teampull Beannachadh... \"composed of rough stones joggled compactly together "
       "without mortar, built in the form of a squared oblong, but irregular on the "
       "ground-plan, the lengths of the side walls externally being respectively 11 ft. 11 "
       "in. and 12 ft 2 in., and the lengths of the end walls 10 ft. 3 in. and 9 ft 2 in. "
       "The walls vary in thickness from 2 ft. 5 in. to 2 ft. 11 in... The chamber measures "
       "about 7 ft. long by 5 ft wide, and 5 ft 9 in. high. The doorway in the west end is "
       "but 3 ft. high, and there is no window or other opening of any kind in the "
       "building.\" — Anderson, Scotland in Early Christian Times, 1882, p. 121")
add("Teampull Beannachadh — drystone corbelled cell", "Eilean Mor, Flannan Isles",
    "early medieval, extant", 60, "measured", F_Q,
    [("external side wall length (1 of 2)", 11, 11, "ft in"),
     ("external side wall length (2 of 2)", 12, 2, "ft in"),
     ("external end wall length (1 of 2)", 10, 3, "ft in"),
     ("external end wall length (2 of 2)", 9, 2, "ft in"),
     ("wall thickness, minimum", 2, 5, "ft in"),
     ("wall thickness, maximum", 2, 11, "ft in"),
     ("internal chamber length", 7, 0, "ft (about)"),
     ("internal chamber width", 5, 0, "ft (about)"),
     ("internal chamber height", 5, 9, "ft in"),
     ("doorway height", 3, 0, "ft")])

# ---------------------------------------------------------------- Hornsea crypt (n228)
H_Q = ("A crypt under the east end of the chancel of Hornsea church in East Yorkshire "
       "differs from that at Repton in having a fire-place, 6 ft. 2 in. wide and 3 ft. 2 in. "
       "high, on its north side... In shape the crypt approaches to a square measuring 15 ft. "
       "5 in. from north to south, and, in the north compartment, 14 ft. 2 in. from east to "
       "west... In the east wall is a window splayed inwardly to a width of nearly 4 feet, "
       "and diminishing to 1 ft. 8 in. in the narrowest part.")
add("Hornsea church crypt (undercroft beneath chancel)", "Hornsea, East Yorkshire",
    "medieval, extant", 228, "measured", H_Q,
    [("internal, north-south", 15, 5, "ft in"),
     ("internal, east-west (north compartment)", 14, 2, "ft in"),
     ("fireplace opening width", 6, 2, "ft in"),
     ("fireplace opening height", 3, 2, "ft in"),
     ("window internal splay width", 4, 0, "ft (nearly)"),
     ("window external (narrowest) width", 1, 8, "ft in")])

# ---------------------------------------------------------------- London party wall (n137)
L_Q = ("The document known as \"Fitz-Alwyne's Assize,\" dated A.D. 1189, shows that the "
       "party-walls of London houses \"were of freestone, 3 feet thick and 16 feet high, "
       "from which the roof (whether covered with tiles or thatch)... \"  and: \"From a deed "
       "bearing date 1217 or 1218, it appears that the corbels or joists for supporting the "
       "upper floor were inserted at a height of eight feet from the ground.\"")
add("London town house — party wall, by statute (Fitz-Alwyne's Assize)", "London",
    "A.D. 1189", 137, "author-rule", L_Q,
    [("party wall thickness (freestone)", 3, 0, "ft"),
     ("party wall height", 16, 0, "ft"),
     ("upper-floor joist bearing height above ground (deed 1217/18)", 8, 0, "ft")])
add("Small English house — common room height, XVIc-XVIIc", "England", "XVIc-XVIIc", 137,
    "author-estimate",
    "It is a common thing nowadays to find in houses of the sixteenth and seventeenth "
    "centuries rooms not more than 6 feet high, and this is especially the case in the "
    "smaller houses.",
    [("internal room height (upper bound observed)", 6, 0, "ft")])

# ---------------------------------------------------------------- the bay rule (n101, n103, n245)
BAY_Q = ("In the twelfth century English buildings were measured by the linear perch of 16 "
         "ft. Thus \"the great barn of Walton was 10 1/2 perches in length (a perch being 16 "
         "ft.) and 3 perches and 5 ft. in breadth.\"")
add("English structural bay — farm buildings", "England", "medieval", 101, "author-rule",
    BAY_Q + "  [and: 'whilst the length of the English bay is rigidly fixed in farm "
            "buildings by the standing room required for two pairs of oxen, considerable "
            "variation occurs in the breadth.']",
    [("linear (building) perch = bay length", 16, 0, "ft")])

add("English land measure — rod / rood", "England", "medieval", 103, "author-rule",
    "the length of the bay, viz. sixteen feet, corresponds to the breadth of a rod or rood "
    "of land, the acre being composed of four roods, each 16 feet broad and 640 feet long, "
    "lying side by side... [footnote] Strictly the English land measure is 16 1/2, and not 16, "
    "feet.",
    [("rood breadth", 16, 0, "ft"),
     ("rood length", 640, 0, "ft"),
     ("land perch / rod (statute)", 16, 6, "ft in (= 16 1/2 ft)")])

add("Roman ox-house — Vitruvius", "Rome (De Architectura 6.9)", "c. B.C. 10", 101,
    "author-rule",
    "the breadth of ox-stalls should be not less than 10 ft. or more than 15 ft. As regards "
    "length, the standing room for each pair of oxen should not be less than 7 ft. "
    "[Latin: Bubilium autem debent esse latitudines nec minores pedum denum, nec majores "
    "quindenum. Longitudo, uti singula iuga, ne minus occupent pedes septenos.]",
    [("ox-stall breadth, minimum", 10, 0, "ft (Roman)"),
     ("ox-stall breadth, maximum", 15, 0, "ft (Roman)"),
     ("standing room per pair of oxen, minimum", 7, 0, "ft (Roman)")])

add("Roman ox-house — Palladius", "Rome (De Re Rustica i.21)", "c. A.D. 210", 101,
    "author-rule",
    "8 ft. are more than sufficient standing room for each pair of oxen, and 15 ft. for the "
    "breadth [of the ox-house.] [Latin: Octo pedes ad spatium standi singulis boum paribus "
    "abundant, et in porrectione xv.]",
    [("standing room per pair of oxen", 8, 0, "ft (Roman)"),
     ("ox-house breadth", 15, 0, "ft (Roman)")])

add("Roman cow-house — Columella", "Rome (i.6.6)", "c. A.D. 60", 101, "author-rule",
    "See also Columella, i. 6. 6. This author fixes the breadth at 10, or at least 9, feet.",
    [("cow-house breadth, preferred", 10, 0, "ft (Roman)"),
     ("cow-house breadth, minimum", 9, 0, "ft (Roman)")])

add("Cow-house at Bolsterstone", "Bolsterstone, South Yorkshire", "vernacular", 101,
    "measured",
    "The breadth of a cow-house at Bolsterstone, described hereafter, is 10 ft. 8 in.",
    [("internal breadth", 10, 8, "ft in")])

add("Bay spacing measured by the author — Roman building, Bailgate", "Lincoln", "Roman",
    103, "measured",
    "The author has measured the bays of the great Roman building lately found in Bailgate, "
    "on the west side of Lincoln Cathedral, and found them, when measured from the centre "
    "of each pillar, 14 ft. 6 in. apart.",
    [("bay spacing, pillar centre to centre", 14, 6, "ft in")])

add("Bay spacing measured by the author — parish churches", "England", "medieval", 103,
    "author-estimate",
    "He has also measured the bays of a few churches and found them about fifteen feet "
    "apart. In cathedrals they are much wider.",
    [("bay spacing", 15, 0, "ft (about)")])

Y_Q = ("According to the Welsh Laws there were 8 feet in the field yoke and 16 feet in the "
       "long yoke. (i. 187.) \"There are sixteen feet in the length of the long yoke.\" "
       "(i. 539.) \"Sexdecim pedes et dimidium iugum faciunt longum, id est, hyryeu.\" "
       "(ii. 784.) \"Pedes XV. et dimidium faciunt longum jugum.\" (ii. 852.)")
add("Welsh Laws — yoke lengths (the unit the bay derives from)", "Wales", "medieval", 245,
    "author-rule", Y_Q + "  [CONFLICT: the four citations give the LONG yoke as 16, 16 1/2 and "
                         "15 1/2 feet. Addy prints all of them without resolving them. All four "
                         "rows are recorded; do not average them.]",
    [("field yoke", 8, 0, "ft"),
     ("long yoke (Welsh Laws i.187, i.539)", 16, 0, "ft"),
     ("long yoke (Welsh Laws ii.784)", 16, 6, "ft in (= 16 1/2 ft)"),
     ("long yoke (Welsh Laws ii.852)", 15, 6, "ft in (= 15 1/2 ft)")])

add("Cow-house (domus vaccariae) at Felsa", "Felsa (Meaux Abbey estate), Yorkshire",
    "1396-9", 245, "documentary-survey",
    "1396-9. \"In vaccaria insuper de Felsa unam novam domum vaccariae, lxxx pedum in "
    "longitudine; fecit de novo aedificari.\" Chron. Monast. de Melsa (Rolls Series), iii. "
    "242. This was exactly 5 bays of 16 feet each.",
    [("internal length", 80, 0, "ft (= 5 bays of 16 ft)")])

add("Berchary (sheep-house), Meaux Abbey estate", "Yorkshire", "1396-9", 245,
    "documentary-survey",
    "On the next page we have a berchary 160 feet, i.e., ten bays, in length.",
    [("internal length", 160, 0, "ft (= 10 bays of 16 ft)")])

# ---------------------------------------------------------------- Pompeii basilica (n224, secondary)
PB_Q = ("\"At the west end,\" says Dr. Lange, \"of the great hall\" of the Pompeian "
        "basilica, \"are three rooms. The middlemost of these is the tribunal, which is "
        "raised 5 1/2 feet above the floor of the great hall, and is 32 feet broad and 18 ft. 2 "
        "in. deep. On the sides of the tribunal are two side rooms about 18 feet broad... "
        "stairs... lead into a subterranean chamber lying 11 feet beneath the tribunal.\"")
add("Basilica at Pompeii — tribunal and undercroft", "Pompeii", "Roman", 224, "secondary",
    PB_Q,
    [("tribunal breadth", 32, 0, "ft"),
     ("tribunal depth", 18, 2, "ft in"),
     ("tribunal floor height above hall floor", 5, 6, "ft in (= 5 1/2 ft)"),
     ("flanking room breadth", 18, 0, "ft (about)"),
     ("subterranean chamber depth below tribunal", 11, 0, "ft")])


def emit(path="english_vernacular_dimensions_v1.tsv"):
    lines = [HEADER]
    for (building, place, period, el, ft, inch, unit, leaf, ev, ver, quote) in ROWS:
        m = ft * FT + inch * IN
        if inch:
            val = f"{ft} ft {inch} in" if ft else f"{inch} in"
        else:
            val = f"{ft} ft"
        printed = leaf - 34
        q = " ".join(quote.split())
        lines.append("\t".join([
            IA, SRC, building, place, period, el, val, unit, f"{m:.3f}",
            f"n{leaf}", str(printed), ev, ver, q]))
    open(path, "w").write("\n".join(lines) + "\n")
    return len(lines) - 1


if __name__ == "__main__":
    print(emit(), "rows")
