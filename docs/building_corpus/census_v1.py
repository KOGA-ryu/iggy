"""Fantasy Homes Grammar Census — Phase-1 generator (census_v1).

Governed by PHASE1_BRIEF_v1.md (blessed 2026-07-31). Emits every Phase-1 CSV; the README is
hand-maintained and subordinate to these tables. Fail-closed from birth: a dangling source
reference, duplicate ID, or incomplete slot stops the build.

CYCLE 1 (2026-07-31) — Mitchell + Ellis loaded and verified, registry + candidates seeded.

SOURCE TRAPS FOUND THIS CYCLE (also documented in iabook.py where the recipe lives):
  * DERIVATIVE PREFIX != IDENTIFIER. india.history.resource.100246 stores its hOCR as
    100246_hocr_*.gz, not <identifier>_hocr_*.gz. Constructing derivative URLs from the
    identifier fails silently (HTML error page); read the item's files list.
  * ELLIS'S OFFSET DRIFTS. leaf n141 = printed 124 (offset 17) but leaf n300 = printed 268
    (offset 32): plate leaves are uncounted by the pagination. imagecount is ABSENT from the
    item metadata, so the len(pageindex)==imagecount rule cannot even be evaluated. Every
    Ellis citation must read the printed number off the page image.
  * ALL THREE BOOKS DESTROY VULGAR FRACTIONS (0 x half/quarter/three-quarter glyphs across
    1,050 leaves) - and joinery dimensions are MOSTLY fractional (4 1/2 x 3 in stock). So:
  * THE NUMBERS LIVE ON THE FIGURES, NOT IN THE PROSE. engdim density is anomalously low on
    all three books because dimensions are drawn on the plates. Consequence: the extraction
    route for this content class is PLATE-FIRST (dimline/contour at full resolution), with
    prose supplying rules and part names. This inverts the Addy workflow.
"""
import csv, os

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- sources registry
# status: loaded | cached | located | mined
# offset_rule: how leaf maps to printed page, with confirmation points
SOURCES = [
 dict(source_id="mitchell1898", ia_id="buildingconstruc00mitc",
      title="Mitchell, Building Construction and Drawing: First Stage (1898)",
      leaves=336, status="loaded",
      offset_rule="printed = leaf - 13; CONFIRMED 2 points (n206=193, n300=287); "
                  "len(pageindex)==imagecount==336, trusted branch",
      fraction_trap="YES - zero fraction glyphs in OCR; figures carry them",
      other_traps="running heads absent from OCR layer - page confirmation needs the image",
      verified_leaves="n206 (printed 193: ledged and braced doors, dimensioned elevations + "
                      "sill sections; the ledged-and-braced type is the construction of the "
                      "Sinc door fixture)",
      rights="pre-1931 scan, University of Toronto contribution; public domain",
      rights_class="unrestricted",
      role="joinery/section atlas - windows (sash leaves n192-n323 cluster), doors, roofs"),
 dict(source_id="mitchell1894", ia_id="buildingconstru01mitcgoog",
      title="Mitchell, Building Construction and Drawing (1894)",
      leaves=293, status="loaded",
      offset_rule="printed = leaf - 14; CONFIRMED 2 points from OCR heads (n60=46, n250=236); "
                  "len(pageindex)==imagecount==293, trusted branch",
      fraction_trap="YES",
      other_traps="Google scan; OCR quality visibly worse than the 1898 Toronto scan "
                  "('EJCAldlKAlriOK' class garbage) - prefer mitchell1898 where both cover "
                  "a topic",
      verified_leaves="",
      rights="Google scan marked NOT_IN_COPYRIGHT", rights_class="unrestricted",
      role="joinery/section atlas, earlier edition; sash cluster n179-n284"),
 dict(source_id="ellis1902", ia_id="india.history.resource.100246",
      title="Ellis, Modern Practical Joinery (1902)",
      leaves=421, status="loaded",
      offset_rule="DRIFTS: printed = leaf - 17 at n141 but leaf - 32 at n300 (uncounted "
                  "plate leaves); imagecount ABSENT from metadata so the trusted-branch "
                  "test cannot be evaluated. READ THE PRINTED NUMBER PER CITATION.",
      fraction_trap="YES - and Ellis prose is dense with fractional rules (throat >= 1/4 in, "
                    "water bar 1 x 3/16 in) so prose numbers need page-image confirmation too",
      other_traps="DERIVATIVE PREFIX trap: hOCR files are 100246_hocr_*, not "
                  "<identifier>_hocr_* - fetch via the files list, or pre-place in cache",
      verified_leaves="n141 (printed 124: sash-frame construction - pulley stile fixing, "
                      "pocket cutting, head joint, oak sill sinkings; dimensioned isometrics "
                      "AND quantified prose rules on one page)",
      rights="1902; public domain", rights_class="unrestricted",
      role="THE joinery treatise: 102 sash leaves, 21-22 casement/mullion leaves, "
           "12 meeting-rail leaves, stairs, doors, skylights"),
 dict(source_id="addy1898", ia_id="evolutionenglis00addygoog",
      title="Addy, The Evolution of the English House (1898)",
      leaves=263, status="mined",
      offset_rule="printed = leaf - 34; CONFIRMED 5 points + Padley plan; trusted branch",
      fraction_trap="YES (the book that discovered the trap)",
      other_traps="", verified_leaves="n109 Bolsterstone barn section; n164 Kensworth plan; "
                                     "n164/n166 dimension passages",
      rights="pre-1931 scan; public domain", rights_class="unrestricted",
      role="already mined: english_vernacular_dimensions_v1.tsv, 149 rows / 37 buildings"),
 dict(source_id="turnerparker", ia_id="someaccountofdom00turn; someaccountofdom00park_0; "
      "someaccountofdom02park",
      title="Turner/Parker, Some Account of Domestic Architecture in England (3 vols cached)",
      leaves=438, status="loaded",
      offset_rule="someaccountofdom02park (Richard II to Henry VIII, the XVc volume): 438 "
                  "leaves, len(pageindex)==imagecount, trusted branch; pagination CONTINUES "
                  "from the earlier part (leaf 228 = printed ~326 per OCR running head "
                  "'32G EXISTING REMAINS' - printed number not yet confirmed on the image). "
                  "Other two volumes unestablished.",
      fraction_trap="unchecked", other_traps="THE WRONG-BERWICK TRAP (census cycle 2): "
      "searching the two obvious volumes for 'Berwick' finds Berwick-upon-Tweed in a castle "
      "list. The Berwick St Leonard barn lives in someaccountofdom02park n228, exactly "
      "where the cycle-48 citation said - the citation was right, the volume identity was "
      "the missing fact.",
      verified_leaves="", rights="pre-1931; public domain", rights_class="unrestricted",
      role="XVc vernacular: Berwick St Leonard barn (n228), Beetham Hall hall-as-barn "
           "(n40-41), Great Chalfield front (n228)"),
 dict(source_id="garner", ia_id="gri_33125010766489",
      title="Garner & Stratton, Domestic Architecture of England during the Tudor Period v.1",
      leaves=364, status="mined",
      offset_rule="leaf N is image N (364==364, confirmed cycle 39)",
      fraction_trap="unchecked", other_traps="",
      verified_leaves="", rights="pre-1931; public domain", rights_class="unrestricted",
      role="Tudor halls (351-row english_hall_dimensions corpus); the n125 cellar"),
 dict(source_id="viollet", ia_id="fr.wikisource (Dictionnaire raisonne)",
      title="Viollet-le-Duc, Dictionnaire raisonne de l'architecture",
      leaves=0, status="mined",
      offset_rule="n/a (wikisource, action=parse required for ProofreadPage transclusion)",
      fraction_trap="n/a", other_traps="restoration provenance: Carcassonne/Pierrefonds "
      "rows are Viollet's own work; datum conflicts (Coucy 64 vs 65 m)",
      verified_leaves="", rights="public domain", rights_class="unrestricted",
      role="castle/tower/window/interior manifests; crenel formula; window rule"),
 dict(source_id="gwilt", ia_id="encyclopaediaofa00gwil",
      title="Gwilt, Encyclopaedia of Architecture", leaves=1474, status="mined",
      offset_rule="DRIFTS (1478 pageindex vs 1474 images; offsets 3 AND 4 measured in one "
                  "book) - the item that produced the drift rule in cycle 39",
      fraction_trap="YES (Newland's table OCRs as '9 8^ 8 H 7 6 H 5' - read off the image)",
      other_traps="", verified_leaves="n689 (printed 668: Newland pairing table, verified "
      "on the page image in cycle 38)",
      rights="pre-1931; public domain", rights_class="unrestricted",
      role="stair rules (27 rows in stair_rules_v1.tsv), incl. the same-page contradiction"),
 # tier anchors, located but not loaded
 dict(source_id="roubo", ia_id="bub_gb_UFuVi47ZglYC (+2 sibling vols)",
      title="Roubo, L'Art du Menuisier (1828 ed.)", leaves=0, status="located",
      offset_rule="unestablished", fraction_trap="unchecked",
      other_traps="pied/pouce units - conversion machinery exists (vldsource/vldnum)",
      verified_leaves="", rights="public domain", rights_class="unrestricted",
      role="French joinery sections at treatise depth"),
 dict(source_id="habs", ia_id="loc.gov HABS/HAER collection",
      title="Historic American Buildings Survey (Library of Congress)", leaves=0,
      status="located",
      offset_rule="n/a (per-sheet measured drawings)", fraction_trap="n/a",
      other_traps="", verified_leaves="",
      rights="US government; public domain", rights_class="unrestricted",
      role="45,882 surveyed structures (API-verified count 2026-07-31); measured-fabric "
           "window/door/moulding sheet details"),
 dict(source_id="millwork", ia_id="Radfordsashdoorsblindsmouldings0001; officialcatalogu00fost; "
      "catalogueoffarle00farl",
      title="Millwork trade catalogues 1880-1930 (Radford 1904, Foster-Munger 1895, "
            "Farley & Loetscher 1898)", leaves=0, status="located",
      offset_rule="unestablished", fraction_trap="unchecked", other_traps="",
      verified_leaves="", rights="pre-1931; public domain", rights_class="unrestricted",
      role="standard sizes at industrial scale - the statistical backbone"),
]

# ---------------------------------------------------------------- candidates
# gate: PASS | PENDING | NONE. NONE rows document a slot hole; they are not candidates.
# All dimension claims live in building_bom_v1 (next cycle); here only identity + gates.
CANDIDATES = [
 dict(cid="BC-00-gunthwaite", slot="00_baseline",
      name="Gunthwaite Hall barn (BUILDING ZERO)", place="Gunthwaite, South Yorkshire",
      period="medieval, extant", source_id="addy1898", leaf_ref="n166 (printed 132)",
      evidence="measured", gate="PASS",
      gate_note="self-checking (11 bays x 15 ft = 165 ft); full parametric set incl. arcade "
                "sections and plinth height; aisled - carries the breadth != span warning",
      verification="quote-bearing manifest rows; source page partially read at full res "
                   "(cycle 48)", fantasy_role="baseline grammar reference"),
 dict(cid="BC-01A-kensworth", slot="01_hall_house", name="Kensworth manor house",
      place="Kensworth, Beds.", period="XIIc", source_id="addy1898",
      leaf_ref="n164-165 (printed 130-131)", evidence="documentary-survey", gate="PASS",
      gate_note="three dimensioned rooms + roof splits; Latin self-check; conjectural plan "
                "is topology-only (measured ~approximate, cycle 49)",
      verification="full-res page read (cycle 48)", fantasy_role="manor hall, 3-room plan"),
 dict(cid="BC-01B-chalfield", slot="01_hall_house", name="Great Chalfield Manor great hall",
      place="Great Chalfield, Wilts.", period="Tudor", source_id="garner",
      leaf_ref="n39", evidence="author-estimate", gate="PASS",
      gate_note="40 x 20 x 20 ft triple in halls corpus; 'about' - author-estimate class",
      verification="quote-bearing manifest row (english_hall_dimensions_v1)",
      fantasy_role="wealthy hall interior"),
 dict(cid="BC-02A-hatfield-mud", slot="02_cot_booth", name="Great Hatfield mud house",
      place="Mappleton, E. Yorks.", period="vernacular, extant 1898", source_id="addy1898",
      leaf_ref="n72 (printed 38)", evidence="measured", gate="PASS",
      gate_note="walls, eaves, door, speer all dimensioned",
      verification="quote-bearing manifest rows", fantasy_role="poor cottage, open hearth"),
 dict(cid="BC-02B-booth1350", slot="02_cot_booth", name="Booth, Bishop Hatfield's Survey",
      place="Co. Durham", period="c. 1350", source_id="addy1898",
      leaf_ref="n56 (printed 22)", evidence="documentary-survey", gate="PASS",
      gate_note="20 x 18 ft; Latin quote", verification="quote-bearing manifest rows",
      fantasy_role="single-cell cot / market booth"),
 dict(cid="BC-03A-walton", slot="03_barn", name="Walton great barn",
      place="Walton (St Paul's manor)", period="XIIc", source_id="addy1898",
      leaf_ref="n166 (printed 132)", evidence="documentary-survey", gate="PASS",
      gate_note="perch arithmetic self-checks both dimensions; aisling unstated -> derived "
                "pitch flagged UNSAFE", verification="full-res page read (cycle 48)",
      fantasy_role="great barn, documentary"),
 dict(cid="BC-03B-berwick", slot="03_barn", name="Berwick St Leonard barn",
      place="Berwick St Leonard, Wiltshire", period="XVc", source_id="turnerparker",
      leaf_ref="someaccountofdom02park n228 (printed ~326, OCR running head)",
      evidence="measured", gate="PASS",
      gate_note="90 x 25 ft, 50 across the transept; consolidated into building_bom_v1 in "
                "census cycle 2; printed page number still needs image confirmation",
      verification="verbatim OCR quote captured; page image not yet inspected",
      fantasy_role="aisled barn with transept"),
 dict(cid="BC-04A-rushy-lee", slot="04_byre_shippon", name="'Coit' at Rushy Lee",
      place="near Midhope, S. Yorks.", period="vernacular, extant 1898",
      source_id="addy1898", leaf_ref="n103-104 (printed 69-70)", evidence="measured",
      gate="PASS", gate_note="44 x 37 ft; aisled; bay grammar stated",
      verification="quote-bearing manifest rows",
      fantasy_role="combined dwelling + byre"),
 dict(cid="BC-04B-bolsterstone", slot="04_byre_shippon", name="Bolsterstone ox-house/barn",
      place="Bolsterstone, S. Yorks.", period="vernacular", source_id="addy1898",
      leaf_ref="n109 (printed 75) + n101", evidence="measured", gate="PASS",
      gate_note="section VERIFIED at full res (cruck + outshuts); 8 oxen, 4/bay; 10 ft 8 in "
                "breadth; 1688 datestone on later work",
      verification="plate inspected at full resolution (cycle 49)",
      fantasy_role="cruck barn - the section reference"),
 dict(cid="BC-05A-forge1350", slot="05_forge_workshop", name="Village forge",
      place="Co. Durham", period="c. 1350", source_id="addy1898",
      leaf_ref="n56 (printed 22)", evidence="documentary-survey", gate="PASS",
      gate_note="20 x 18 ft, stated same size as the booth - the corpus's only smithy "
                "footprint", verification="quote-bearing manifest rows",
      fantasy_role="smithy"),
 dict(cid="BC-05B-NONE", slot="05_forge_workshop", name="NONE - second candidate unsourced",
      place="", period="", source_id="", leaf_ref="", evidence="", gate="NONE",
      gate_note="no second workshop source in the mined corpus; sourcing decision is Ace's",
      verification="", fantasy_role=""),
 dict(cid="BC-06A-peveril", slot="06_keep_tower", name="Peak (Peveril) Castle keep",
      place="Castleton, Derbys.", period="XIIc, extant", source_id="addy1898",
      leaf_ref="n192/n197/n198 (printed 158/163/164)", evidence="measured", gate="PASS",
      gate_note="room triple, door sill height, sentry aperture; roof concealed behind "
                "parapet (design reason recorded); section A.B. plate cached UNREAD",
      verification="quote-bearing manifest rows", fantasy_role="small keep / watchtower"),
 dict(cid="BC-06B-coucy", slot="06_keep_tower", name="Coucy donjon (Viollet)",
      place="Coucy, France", period="XIIIc", source_id="viollet", leaf_ref="Donjon; Chateau",
      evidence="documented-reconstruction", gate="PASS",
      gate_note="DATUM CONFLICT recorded (64 m from ditch bottom vs 65 m unstated); 55 m in "
                "Donjon still needs its datum checked (open queue item)",
      verification="quote-bearing manifest rows (viollet_castle_tower_v1)",
      fantasy_role="great donjon"),
 dict(cid="BC-07A-crenel-set", slot="07_gate_fortification",
      name="Crenel/merlon parametric set", place="France (Viollet)", period="XII-XIVc",
      source_id="viollet", leaf_ref="Creneau (cycle 36 formula)", evidence="author-rule",
      gate="PASS", gate_note="RULES-LANE CROSSOVER: enters rules_v1 as the primary record; "
                "candidate row exists for slot coverage",
      verification="mined formula", fantasy_role="battlements everywhere"),
 dict(cid="BC-07B-carcassonne", slot="07_gate_fortification", name="Carcassonne rows",
      place="Carcassonne, France", period="restored XIXc", source_id="viollet",
      leaf_ref="fortification_geometry_v1 rows", evidence="documented-reconstruction",
      gate="PASS", gate_note="PROVENANCE FLAG: Viollet's own restoration site; older-manifest "
                "re-class is an open queue item", verification="manifest rows",
      fantasy_role="city gate + curtain"),
 dict(cid="BC-08A-fitzalwyne", slot="08_town_house", name="London party-wall code",
      place="London", period="A.D. 1189", source_id="addy1898",
      leaf_ref="n137 (printed 103)", evidence="author-rule", gate="PASS",
      gate_note="RULES-LANE CROSSOVER (Fitz-Alwyne's Assize): 3 ft x 16 ft freestone walls, "
                "joists at 8 ft", verification="quote-bearing manifest rows",
      fantasy_role="urban terrace construction law"),
 dict(cid="BC-08B-viollet-town", slot="08_town_house", name="Viollet town-fabric rows",
      place="France", period="XII-XVc", source_id="viollet",
      leaf_ref="viollet_town_fabric_v1 (16 rows)", evidence="mixed", gate="PASS",
      gate_note="FRONTAGE WIDTH MISSING from entire corpus - the slot's open number",
      verification="manifest rows", fantasy_role="town house"),
 dict(cid="BC-09A-padley", slot="09_chapel_first_floor", name="Padley Hall",
      place="Padley, Derbys.", period="medieval, ruin", source_id="addy1898",
      leaf_ref="n170-174 (printed 136-140)", evidence="measured", gate="PASS",
      gate_note="hall + buttery + chapel-over, window schedule down to 2ft6 x 1ft",
      verification="quote-bearing manifest rows", fantasy_role="chapel-over-hall"),
 dict(cid="BC-09B-charney", slot="09_chapel_first_floor", name="Charney Bassett manor",
      place="Charney Bassett, Berks.", period="late XIIIc", source_id="addy1898",
      leaf_ref="n180 (printed 146)", evidence="measured", gate="PASS",
      gate_note="chapel 12ft5 x 9ft10 over buttery; hall 30 x 16",
      verification="quote-bearing manifest rows", fantasy_role="chapel-over-hall variant"),
 dict(cid="BC-10A-hornsea", slot="10_cellar_crypt", name="Hornsea church crypt",
      place="Hornsea, E. Yorks.", period="medieval, extant", source_id="addy1898",
      leaf_ref="n228 (printed 194)", evidence="measured", gate="PASS",
      gate_note="near-square crypt with FIREPLACE (6ft2 x 3ft2) - rare feature",
      verification="quote-bearing manifest rows", fantasy_role="undercroft / hideout"),
 dict(cid="BC-10B-lavenham", slot="10_cellar_crypt",
      name="Lavenham Guildhall - cellar (and hall)",
      place="Lavenham, Suffolk", period="early XVIc", source_id="garner",
      leaf_ref="n125 (printed 108)", evidence="author-estimate", gate="PASS",
      gate_note="cellar 'about' 31ft9 x 16ft10 x 7ft3, brick stair beneath the staircase, "
                "iron-barred windows; bonus: main hall about 30 x 17 ft, 18 in jetty, "
                "'doors were ledged and boarded, hung on massive hand hinges'; building "
                "identified from the page itself (Fig. 126 caption) - the two-column OCR "
                "had jumbled it",
      verification="page read at full resolution (census cycle 2)",
      fantasy_role="guild hall over a secure cellar"),
 dict(cid="BC-11A-NONE", slot="11_mill", name="NONE - documented negative",
      place="", period="", source_id="", leaf_ref="", evidence="", gate="NONE",
      gate_note="no mill dimension exists in Viollet or Addy (negative_facts); slot held "
                "open to force the sourcing decision", verification="", fantasy_role=""),
 dict(cid="BC-11B-NONE", slot="11_mill", name="NONE", place="", period="", source_id="",
      leaf_ref="", evidence="", gate="NONE", gate_note="as 11A", verification="",
      fantasy_role=""),
 dict(cid="BC-12A-gallarus", slot="12_stone_cell", name="Gallarus Oratory",
      place="Gallarus, Co. Kerry", period="early medieval, extant", source_id="addy1898",
      leaf_ref="n59 (printed 25)", evidence="measured", gate="PASS",
      gate_note="full openings schedule incl. inclining jambs and door-frame sockets",
      verification="quote-bearing manifest rows", fantasy_role="hermit cell / shrine"),
 dict(cid="BC-12B-teampull", slot="12_stone_cell", name="Teampull Beannachadh",
      place="Eilean Mor, Flannan Isles", period="early medieval, extant",
      source_id="addy1898", leaf_ref="n60 (printed 26)", evidence="measured", gate="PASS",
      gate_note="all four wall lengths + thickness range; no window at all",
      verification="quote-bearing manifest rows", fantasy_role="remote drystone cell"),
]

SLOTS = ["00_baseline"] + ["%02d_%s" % (i, s) for i, s in enumerate(
    ["hall_house","cot_booth","barn","byre_shippon","forge_workshop","keep_tower",
     "gate_fortification","town_house","chapel_first_floor","cellar_crypt","mill",
     "stone_cell"], start=1)]


# ---------------------------------------------------------------- BOM (cycle 2)
# building_bom_v1: row-level element instances for census candidates, consolidated from the
# already-mined manifests. IDs B### are POSITIONAL over the ingestion order below - the
# order and the source manifests are FROZEN; new rows append at the end only.
#
# Disposition is exclusive and asserted: every manifest row is assigned / rules-deferred /
# reserve / out-of-scope. Rules-deferred rows go to rules_v1 (cycle 3); nothing vanishes.

MANIFESTS = os.path.join(os.path.dirname(HERE), "blender", "reference_manifests")

# vernacular manifest building -> candidate. Buildings absent from this map are classified
# below, never silently dropped.
VERN_MAP = {
 "Gunthwaite Hall barn — aisled timber barn (nave + two aisles)": "BC-00-gunthwaite",
 "Kensworth manor house — hall": "BC-01A-kensworth",
 'Kensworth manor house — "house" (domus, the entry between hall and bower)': "BC-01A-kensworth",
 "Kensworth manor house — bower (thalamus, women's apartment)": "BC-01A-kensworth",
 "Kensworth manor house — ox-house": "BC-01A-kensworth",
 "Kensworth manor house — sheep-cote": "BC-01A-kensworth",
 "Kensworth manor house — lamb-cote": "BC-01A-kensworth",
 "Great Hatfield mud house (single-storey cot, open hearth)": "BC-02A-hatfield-mud",
 "Booth (single-cell cruck cot) — newly built on the waste": "BC-02B-booth1350",
 "Walton manor house — great barn": "BC-03A-walton",
 '"Coit" at Rushy Lee — combined dwelling + shippon, basilical, aisled': "BC-04A-rushy-lee",
 "Cow-house at Bolsterstone": "BC-04B-bolsterstone",
 "Village forge / smithy": "BC-05A-forge1350",
 "Peak (Peveril) Castle keep — upper apartment / living room": "BC-06A-peveril",
 "Peak (Peveril) Castle keep — basement / ground-floor store": "BC-06A-peveril",
 'Peak (Peveril) Castle keep — "sentry" aperture above the roof': "BC-06A-peveril",
 "Padley Hall — buttery (ground floor)": "BC-09A-padley",
 "Padley Hall — hall (ground floor, chapel over)": "BC-09A-padley",
 "Charney Bassett manor house — first-floor chapel": "BC-09B-charney",
 "Charney Bassett manor house — ground-floor hall": "BC-09B-charney",
 "Hornsea church crypt (undercroft beneath chancel)": "BC-10A-hornsea",
 '"Oratory" of Gallarus — dry-rubble corbelled cell': "BC-12A-gallarus",
 "Teampull Beannachadh — drystone corbelled cell": "BC-12B-teampull",
}
# rules-lane rows (cycle 3 ingests these into rules_v1)
VERN_RULES = {
 "English structural bay — farm buildings", "English land measure — rod / rood",
 "Roman ox-house — Vitruvius", "Roman ox-house — Palladius", "Roman cow-house — Columella",
 "Welsh Laws — yoke lengths (the unit the bay derives from)",
 "Cow-house (domus vaccariae) at Felsa", "Berchary (sheep-house), Meaux Abbey estate",
 "Bay spacing measured by the author — Roman building, Bailgate",
 "Bay spacing measured by the author — parish churches",
 "London town house — party wall, by statute (Fitz-Alwyne's Assize)",
 "Small English house — common room height, XVIc-XVIIc",
}
# reserve pool: real buildings with real quotes, no census slot - preserved, not ranked
VERN_RESERVE = {"Cholsey barn (demolished)": "RESERVE-cholsey"}
VERN_OUT = {"Basilica at Pompeii — tribunal and undercroft"}  # out of scope (not English/fantasy fit)

RULES_CROSSOVER = {"BC-07A-crenel-set", "BC-08A-fitzalwyne"}  # candidates whose content is rules_v1

FT, IN = 0.3048, 0.0254

# hand-consolidated orphans; every metre value computed from the printed feet/inches
# (candidate, building, element, ft, inch, unit_note, datum, evidence, uncert, verified,
#  source_id, leaf_ref, quote)
ORPHANS = [
 ("BC-03B-berwick", "Berwick St Leonard barn", "internal length", 90, 0, "ft",
  "unstated", "measured", "", "no", "turnerparker", "someaccountofdom02park n228",
  "At Berwick St. Leonard's there is also a fine barn of the fifteenth century, 90 feet "
  "long by 25 wide, and 50 across the transept."),
 ("BC-03B-berwick", "Berwick St Leonard barn", "internal width", 25, 0, "ft",
  "unstated", "measured", "", "no", "turnerparker", "someaccountofdom02park n228",
  "At Berwick St. Leonard's there is also a fine barn of the fifteenth century, 90 feet "
  "long by 25 wide, and 50 across the transept."),
 ("BC-03B-berwick", "Berwick St Leonard barn", "width across the transept", 50, 0, "ft",
  "unstated", "measured", "", "no", "turnerparker", "someaccountofdom02park n228",
  "At Berwick St. Leonard's there is also a fine barn of the fifteenth century, 90 feet "
  "long by 25 wide, and 50 across the transept."),
 ("RESERVE-beetham", "Beetham Hall — fortified hall (now used as a barn)",
  "hall internal length", 39, 0, "ft", "unstated", "measured", "", "no",
  "turnerparker", "someaccountofdom02park n40-n41",
  "[n40:] Betham, or Bytham Hall, a seat of the Earl of Derby, is of various d[ates] "
  "[n41:] The castle itself consists of the hall, of the fourteenth century, 39 feet by "
  "26, now used as a barn, and two wings: the windows are small, and high from the "
  "ground, for the sake of defence."),
 ("RESERVE-beetham", "Beetham Hall — fortified hall (now used as a barn)",
  "hall internal width", 26, 0, "ft", "unstated", "measured", "", "no",
  "turnerparker", "someaccountofdom02park n40-n41",
  "The castle itself consists of the hall, of the fourteenth century, 39 feet by 26, now "
  "used as a barn, and two wings."),
 ("BC-10B-lavenham", "Lavenham Guildhall — cellar", "cellar length", 31, 9, "ft in",
  "unstated", "author-estimate", "", "yes", "garner", "n125 (printed 108)",
  "The cellar, which is about 31 feet 9 inches by 16 feet 10 inches, and 7 feet 3 inches "
  "high, is reached by a brick stair beneath the staircase leading to the first floor: "
  "the windows are small and iron barred."),
 ("BC-10B-lavenham", "Lavenham Guildhall — cellar", "cellar width", 16, 10, "ft in",
  "unstated", "author-estimate", "", "yes", "garner", "n125 (printed 108)",
  "The cellar, which is about 31 feet 9 inches by 16 feet 10 inches, and 7 feet 3 inches "
  "high, is reached by a brick stair beneath the staircase leading to the first floor."),
 ("BC-10B-lavenham", "Lavenham Guildhall — cellar", "cellar height", 7, 3, "ft in",
  "floor to ceiling", "author-estimate", "", "yes", "garner", "n125 (printed 108)",
  "The cellar, which is about 31 feet 9 inches by 16 feet 10 inches, and 7 feet 3 inches "
  "high, is reached by a brick stair beneath the staircase leading to the first floor."),
 ("BC-10B-lavenham", "Lavenham Guildhall — main hall", "internal length", 30, 0,
  "ft (about)", "unstated", "author-estimate", "", "yes", "garner", "n125 (printed 108)",
  "The main hall, which was the meeting place for the merchants, is entered through the "
  "porch, and measures about 30 feet by 17 feet."),
 ("BC-10B-lavenham", "Lavenham Guildhall — main hall", "internal width", 17, 0,
  "ft (about)", "unstated", "author-estimate", "", "yes", "garner", "n125 (printed 108)",
  "The main hall, which was the meeting place for the merchants, is entered through the "
  "porch, and measures about 30 feet by 17 feet."),
 ("BC-10B-lavenham", "Lavenham Guildhall — jetty", "first-floor overhang", 0, 18,
  "in (about; as printed - the audit witness check flagged the earlier 1 ft 6 in "
  "normalisation as diverging from the quote's own form)", "face of ground floor",
  "author-estimate", "", "yes",
  "garner", "n125 (printed 108)",
  "The first floor overhangs the face of the ground floor about 18 inches, and this "
  "projection is continued on both frontages by means of a diagonal beam and angle post."),
 ("BC-04B-bolsterstone", "Bolsterstone ox-house", "structural bay length", 15, 0,
  "ft (nearly)", "unstated", "measured", "", "yes", "addy1898", "n109 (printed 75)",
  "Four oxen stood in a bay, a length of nearly 15 ft., two in each stall."),
 ("BC-04B-bolsterstone", "Bolsterstone ox-house", "loft floor height above ox-house floor",
  6, 0, "ft (rather more than)", "ground floor of the ox-house", "measured", "", "yes",
  "addy1898", "n109 (printed 75)",
  "Rather more than 6 ft. above the ground floor of the ox-house are the remains of an "
  "upper floor, showing that there was a loft or gallery above."),
]

DATUM_KEYS = ("tie-beam", "ridge", "eaves", "springing", "square", "ground", "floor",
              "sill", "lintel", "ditch", "plinth")


def _datum(element):
    e = element.lower()
    for k in DATUM_KEYS:
        if k in e:
            return element  # the element text itself states the datum
    return "unstated"


def build_bom():
    import csv as _csv
    rows, dispo = [], {"assigned": 0, "rules-deferred": 0, "reserve": 0, "out-of-scope": 0}
    # 1. vernacular manifest (order frozen)
    vp = os.path.join(MANIFESTS, "english_vernacular_dimensions_v1.tsv")
    # QUOTE_NONE: the source TSVs are raw tab-joined; default csv quoting silently
    # STRIPS the literal double-quotes in names like '"Coit" at Rushy Lee' - the very
    # first fail-closed run caught exactly that.
    for r in _csv.DictReader(open(vp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        b = r["building"]
        if b in VERN_MAP or b in VERN_RESERVE:
            cid = VERN_MAP.get(b) or VERN_RESERVE[b]
            dispo["assigned" if b in VERN_MAP else "reserve"] += 1
            rows.append(dict(candidate_id=cid, building=b, element=r["element"],
                value=r["value"], unit=r["unit"], value_m=r["value_m"],
                datum=_datum(r["element"]),
                evidence=r["evidence"],
                uncertainty_type=("measurement-ocr" if r["evidence"] == "ocr-uncertain" else ""),
                verified_full_res=r["verified_full_res"], source_id="addy1898",
                leaf_ref="%s (printed %s)" % (r["leaf"], r["printed_page"]),
                quote=r["quote"]))
        elif b in VERN_RULES:
            dispo["rules-deferred"] += 1
        elif b in VERN_OUT:
            dispo["out-of-scope"] += 1
        else:
            raise CensusError("vernacular building unclassified: %r" % b)
    # 2. Great Chalfield from the halls manifest
    hp = os.path.join(MANIFESTS, "english_hall_dimensions_v1.tsv")
    for r in _csv.DictReader(open(hp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        if "Chalfield" not in r["building"]:
            continue
        dispo["assigned"] += 1
        rows.append(dict(candidate_id="BC-01B-chalfield", building=r["building"],
            element=r["element"], value=r["value"], unit=r["unit"], value_m=r["value_m"],
            datum=_datum(r["element"]), evidence=r["confidence"], uncertainty_type="",
            verified_full_res="no", source_id="garner",
            leaf_ref="n%s" % r["leaf"], quote=r["quote"]))
    # 3. Viollet rows for the three Viollet candidates
    def viollet(path, match, cid):
        n = 0
        for r in _csv.DictReader(open(os.path.join(MANIFESTS, path)), delimiter="\t",
                                 quoting=_csv.QUOTE_NONE):
            if match and match not in r["subject"]:
                continue
            v = r["value"]
            try:
                vm = "%.3f" % float(v) if r["unit"] == "m" else ""
            except ValueError:
                vm = ""
            rows.append(dict(candidate_id=cid, building=r["subject"], element=r["element"],
                value=v, unit=r["unit"], value_m=vm, datum=_datum(r["element"]),
                evidence=r["evidence_class"],
                uncertainty_type=("provenance" if "carcassonne" in r["subject"].lower()
                                  or r["evidence_class"] == "documented-reconstruction"
                                  else ""),
                verified_full_res="n/a (text source)", source_id="viollet",
                leaf_ref="Dictionnaire, %s" % r["source_article"], quote=r["french_quote"]))
            n += 1
        return n
    dispo["assigned"] += viollet("viollet_castle_tower_v1.tsv", "Coucy", "BC-06B-coucy")
    dispo["assigned"] += viollet("fortification_geometry_v1.tsv", "Carcassonne",
                                 "BC-07B-carcassonne")
    dispo["assigned"] += viollet("viollet_town_fabric_v1.tsv", "", "BC-08B-viollet-town")
    # 4. orphans
    for (cid, bld, el, ft, inch, unit, datum, ev, unc, ver, src, leaf, quote) in ORPHANS:
        m = ft * FT + inch * IN
        val = (("%d ft %d in" % (ft, inch)) if (ft and inch) else
               ("%d in" % inch) if inch else ("%d ft" % ft))
        rows.append(dict(candidate_id=cid, building=bld, element=el, value=val, unit=unit,
            value_m="%.3f" % m, datum=datum, evidence=ev, uncertainty_type=unc,
            verified_full_res=ver, source_id=src, leaf_ref=leaf, quote=quote))
        dispo["assigned" if not cid.startswith("RESERVE") else "reserve"] += 1
    for i, r in enumerate(rows):
        r["bom_row_id"] = "B%03d" % i
    return rows, dispo


def validate_bom(rows):
    errs = []
    cids = set(c["cid"] for c in CANDIDATES)
    reserve = {"RESERVE-beetham", "RESERVE-cholsey"}
    for r in rows:
        if r["candidate_id"] not in cids and r["candidate_id"] not in reserve:
            errs.append("%s: dangling candidate %s" % (r["bom_row_id"], r["candidate_id"]))
        if not r["quote"].strip():
            errs.append("%s: NO QUOTE" % r["bom_row_id"])
        if not r["source_id"]:
            errs.append("%s: no source" % r["bom_row_id"])
    covered = set(r["candidate_id"] for r in rows)
    for c in CANDIDATES:
        if c["gate"] == "PASS" and c["cid"] not in covered \
                and c["cid"] not in RULES_CROSSOVER:
            errs.append("PASS candidate %s has zero BOM rows" % c["cid"])
    if errs:
        raise CensusError("FAIL-CLOSED (BOM): " + "; ".join(errs[:12]))


# ---------------------------------------------------------------- RULES LANE (cycle 3)
# Parametric rules are first-class census objects (brief section 6). They receive no
# production rank in Phase 1. Conflicts are carried UNMERGED under a conflict_group -
# the Welsh yoke values and Gwilt's two same-page stair rules are never averaged or
# reconciled; the conflict itself is the recorded fact.

# Ellis weathering/throating rules, transcribed from the page IMAGE at full resolution
# (leaf n141 = printed 124, verified census cycle 1) because the OCR layer destroys every
# fraction. (element, value, unit, value_m, quote)
ELLIS_RULES = [
 ("window sill throat, minimum width", "1/4 in", "in", 0.25 * IN,
  "To be effective the Throat (see Fig. 404) should not be less than 1/4 in. wide, but "
  "its depth is of less importance."),
 ("sill water bar, width", "1 in", "in", 1 * IN,
  "Sills should be grooved at the bottom to receive a water bar of 1 in. by 3/16 in. "
  "galvanised iron, bedded with white lead in a similar groove in the stone sill."),
 ("sill water bar, thickness", "3/16 in", "in", 3.0 / 16 * IN,
  "Sills should be grooved at the bottom to receive a water bar of 1 in. by 3/16 in. "
  "galvanised iron, bedded with white lead in a similar groove in the stone sill."),
 ("sill bench allowance beyond the frame face", "1/8 in", "in", 0.125 * IN,
  "The sill should always be left from the bench 1/8 in. wider than the rest of the frame "
  "on the outside, to compensate for its greater shrinkage."),
 ("pulley-stile sinking below the sill weathering", "1/4 in", "in", 0.25 * IN,
  "This sinking, which should be made as small as possible, consistent with providing for "
  "sufficient substance in the wedge to drive without breaking, is taken down until it is "
  "1/4 in. below the weathering at the outer edge, as shown in the sketch, Fig. 409."),
]

# bulk material tables: registered as pointers, not duplicated row-by-row
TABLE_POINTERS = [
 ("masonry strength table", "masonry_strength_v1.tsv", 11,
  "crushing/shearing strengths per material tier"),
 ("timber roof scantlings", "timber_roof_scantlings_v2.tsv", 62,
  "measured member sections per church roof, with spans and truss spacing"),
 ("Paley moulding profiles (parametric)", "moulding_profiles_v1.json", 0,
  "compass-and-square profile constructions, already parametric via moulding_profile.py"),
]


def _yoke_conflict(element):
    return "long-yoke" if "long yoke" in element else ""


def build_rules():
    import csv as _csv
    rules = []
    # 1. the 23 rules-deferred vernacular rows
    vp = os.path.join(MANIFESTS, "english_vernacular_dimensions_v1.tsv")
    n_def = 0
    for r in _csv.DictReader(open(vp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        if r["building"] not in VERN_RULES:
            continue
        n_def += 1
        b = r["building"]
        group = ("building_code" if "Fitz-Alwyne" in b else
                 "unit_system" if ("yoke" in b or "rod / rood" in b) else
                 "bay_module")
        rules.append(dict(rule_group=group, subject=b, element=r["element"],
            value=r["value"], unit=r["unit"], value_m=r["value_m"],
            conflict_group=_yoke_conflict(r["element"]), rule_type="rule",
            source_id="addy1898",
            ref="%s (printed %s)" % (r["leaf"], r["printed_page"]),
            evidence=r["evidence"], verified_full_res=r["verified_full_res"],
            quote=r["quote"]))
    assert n_def == 23, "expected 23 rules-deferred vernacular rows, got %d" % n_def
    # 2. Gwilt stair rules - all 27, with the same-page contradiction marked
    sp = os.path.join(MANIFESTS, "stair_rules_v1.tsv")
    for r in _csv.DictReader(open(sp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        subj = r["subject"]
        cg = ("gwilt-stair-pitch" if ("constant-product" in subj or "Newland" in subj)
              else "")
        rules.append(dict(rule_group="stair", subject=subj, element=r["element"],
            value=r["value"], unit=r["unit"], value_m=r["value_m"], conflict_group=cg,
            rule_type="rule", source_id="gwilt", ref=r["source"],
            evidence=r["evidence_class"], verified_full_res="see source ref",
            quote=r["quote"]))
    # 3. Viollet window + bridge author-rules
    wp = os.path.join(MANIFESTS, "viollet_window_bridge_v1.tsv")
    for r in _csv.DictReader(open(wp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        if r["evidence_class"] != "author-rule":
            continue
        try:
            vm = "%.3f" % float(r["value"]) if r["unit"] == "m" else ""
        except ValueError:
            vm = ""
        rules.append(dict(rule_group=r["family"], subject=r["subject"],
            element=r["element"], value=r["value"], unit=r["unit"], value_m=vm,
            conflict_group="", rule_type="rule", source_id="viollet",
            ref="Dictionnaire, %s" % r["source_article"], evidence="author-rule",
            verified_full_res="n/a (text source)", quote=r["french_quote"]))
    # 4. the crenellation formula + general fortification rules
    fp = os.path.join(MANIFESTS, "fortification_geometry_v1.tsv")
    for r in _csv.DictReader(open(fp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        if not any(k in r["subject"] for k in ("formula", "general", "principle")):
            continue
        try:
            vm = "%.3f" % float(r["value"]) if r["unit"] == "m" else ""
        except ValueError:
            vm = ""
        rules.append(dict(rule_group="fortification", subject=r["subject"],
            element=r["element"], value=r["value"], unit=r["unit"], value_m=vm,
            conflict_group="", rule_type="rule", source_id="viollet",
            ref="Dictionnaire, %s" % r["source_article"], evidence=r["evidence_class"],
            verified_full_res="n/a (text source)", quote=r["french_quote"]))
    # 5. Ellis joinery weathering rules (fractions read off the page image)
    for el, v, u, vm, q in ELLIS_RULES:
        rules.append(dict(rule_group="joinery_weathering", subject="Ellis, sash windows",
            element=el, value=v, unit=u, value_m="%.5f" % vm, conflict_group="",
            rule_type="rule", source_id="ellis1902", ref="n141 (printed 124)",
            evidence="author-rule", verified_full_res="yes", quote=q))
    # 6. table pointers
    for name, path, nrows, what in TABLE_POINTERS:
        full = os.path.join(MANIFESTS, path)
        if not os.path.exists(full):
            raise CensusError("table pointer target missing: %s" % path)
        rules.append(dict(rule_group="material_table", subject=name, element=what,
            value="(table: %d data rows)" % nrows if nrows else "(parametric file)",
            unit="", value_m="", conflict_group="", rule_type="table_pointer",
            source_id="manifests", ref=path, evidence="see table",
            verified_full_res="see table", quote=""))
    for i, r in enumerate(rules):
        r["rule_id"] = "RL%03d" % i
    return rules


# ---------------------------------------------------------------- ELEMENT TALLY (cycle 4)
# PROVISIONAL - a coverage instrument, not a production queue (brief section 10). Broad
# classes come from the frozen element_vocab.py (41 classes, mining lane). Census-side
# gaps are patched by an OVERLAY of PROPOSED rules, recorded here and in the README; the
# vocabulary freeze is a review decision, so element_vocab.py itself is untouched.
#
# What the dry run found (166 BOM rows): 137 mapped, 29 unmapped - and 21 of the 29 are one
# dialect fact: ADDY SAYS 'BREADTH' WHERE THE VOCAB SAYS 'WIDTH'. The vocab was built on
# Viollet and the halls corpus; the vernacular shelf speaks differently. The rest: member
# sections named by face, a circumference, axis-labelled plan dims, an ambry, two site
# features, a dovecote capacity.

# tried BEFORE the vocab - census-proposed classes for elements the vocab genuinely lacks
OVERLAY_PRE = [
 ("hearth_screen", r'speer|hearth.screen'),   # the brief's own proposal; Great Hatfield rows
]
# tried AFTER the vocab returns None - pattern extensions and two proposed classes
OVERLAY_POST = [
 ("internal_width",  r"internal breadth"),
 ("external_width",  r"external breadth"),
 ("opening_width",   r"aperture breadth|ambry|aumbry"),
 ("width",           r"\bbreadth\b"),
 ("member_section",  r"pillar (section|circumference)|arcade pillar"),
 ("plan_dimension",  r"plan dimension|internal, (north|south|east|west)"),  # PROPOSED CLASS:
    # a plan dimension whose axis labelling does not follow the length/width convention -
    # Padley's 'order as printed' pair and Hornsea's compass-labelled pair. Assigning these
    # to length/width would invent an axis the source does not state.
 ("level_difference", r"escarpment above"),
 ("gap",             r"ditch separating"),
 ("count",           r"capacity"),
]
PROPOSED_CLASSES = {"hearth_screen", "plan_dimension"}

# PROVISIONAL evidence weights for the coverage tally. Full weight for classes whose error
# mode is metrological or scribal but whose NUMBER is first-hand; half for estimates,
# reconstructions and second-hand; zero for OCR-doubtful numbers.
TALLY_W = {"measured": 1.0, "measured-fabric": 1.0, "documentary-survey": 1.0,
           "author-estimate": 0.5, "documented-reconstruction": 0.5, "secondary": 0.5,
           "author-rule": 0.5, "ocr-uncertain": 0.0}


def classify_element(element):
    import re as _re
    for c, p in OVERLAY_PRE:
        if _re.search(p, element, _re.I):
            return c, "census-proposed"
    import sys as _sys
    if MANIFESTS not in _sys.path:
        _sys.path.insert(0, MANIFESTS)
    import element_vocab as _ev
    k = _ev.canon(element)
    if k:
        return k, "vocab"
    for c, p in OVERLAY_POST:
        if _re.search(p, element, _re.I):
            return c, ("census-proposed" if c in PROPOSED_CLASSES
                       else "census-overlay-pattern")
    return None, None


def build_tally(bom):
    from collections import defaultdict
    cells = defaultdict(dict)   # element -> candidate -> max weight
    meta = defaultdict(lambda: dict(rows=0, layers=set(), examples=[]))
    unmapped = []
    for r in bom:
        k, layer = classify_element(r["element"])
        if k is None:
            unmapped.append(r)
            continue
        w = TALLY_W.get(r["evidence"], 0.0)
        m = meta[k]
        m["rows"] += 1
        m["layers"].add(layer)
        if len(m["examples"]) < 2 and r["element"] not in m["examples"]:
            m["examples"].append(r["element"])
        cid = r["candidate_id"]
        prev = cells[k].get(cid)
        if prev is None or (isinstance(prev, float) and w > prev):
            cells[k][cid] = w if w > 0 else (prev if prev is not None else 0.0)
    if unmapped:
        raise CensusError("FAIL-CLOSED (tally): %d unmapped elements: %s"
                          % (len(unmapped), [r["element"][:40] for r in unmapped][:8]))
    return cells, meta


# ---------------------------------------------------------------- SECTION INVENTORY (cycle 5)
# The Phase-2 decomposition worklist: window/door figures in Ellis + Mitchell 1898,
# extracted from cached OCR by section_inventory_extract.py into section_inventory_raw.tsv
# (committed; this generator never touches the network). Everything is MAPPED except
# figures on the two leaves inspected at full resolution in cycle 1. 'dimensioned' is
# UNKNOWN throughout - the books carry dimensions on the figures and the OCR destroys
# fractions, so only full-resolution inspection can answer it (Phase 2's job).
INSPECTED_LEAVES = {("mitchell1898", 206), ("ellis1902", 141),
                    ("ellis1902", 153)}  # plate 451A, decomposed in cycle 6
PRIORITY_1 = {"sash_window", "casement", "skylight", "glazing", "shutter"}


def build_sections():
    import csv as _csv
    p = os.path.join(HERE, "section_inventory_raw.tsv")
    if not os.path.exists(p):
        raise CensusError("section_inventory_raw.tsv missing - run "
                          "section_inventory_extract.py first")
    sids = set(x["source_id"] for x in SOURCES)
    leaves_by_src = {x["source_id"]: x["leaves"] for x in SOURCES}
    rows = []
    for r in _csv.DictReader(open(p), delimiter="\t", quoting=_csv.QUOTE_NONE):
        src, leaf = r["source_id"], int(r["leaf"])
        if src not in sids:
            raise CensusError("section row cites unknown source %r" % src)
        if not (0 <= leaf < (leaves_by_src.get(src) or 10**6)):
            raise CensusError("section row leaf %d out of range for %s" % (leaf, src))
        tags = set(r["topic_tags"].split(";"))
        pri = "1" if tags & PRIORITY_1 else "2"
        status = "VERIFIED-LEAF" if (src, leaf) in INSPECTED_LEAVES else "MAPPED"
        rows.append([f"S{len(rows):03d}", src, "n%d" % leaf, r["printed_page"],
                     r["figure_no"], r["caption"], r["topic_tags"],
                     r["section_keyword"], "unknown", pri, status])
    return rows


NEGATIVE_FACTS = [
 ("NF00", "documented absence", "the MILL",
  "No mill dimension exists anywhere in the mined corpus - not in Viollet's Dictionnaire "
  "(cycle 42 sweep) and not in Addy. Slot 11 is held open to force the sourcing decision; "
  "any mill asset built now would have NO dimensional authority.",
  "findings doc sections 50 and 56; census candidates BC-11A/B"),
 ("NF01", "construction constraint", "breadth != roof span for aisled/cruck buildings",
  "The Bolsterstone section (inspected at full resolution) is a cruck truss with outshuts: "
  "the main roof spans between the blades, not the walls, and no single pitch describes "
  "the section. Any pitch derived from breadth/2 on an aisled building is UNSAFE.",
  "addy1898 n109 (printed 75), VERIFIED; roof_pitch_derived safety flags"),
 ("NF02", "unresolved conflict - do not average", "the long yoke",
  "The Welsh Laws give the long yoke as 16 ft (i.187, i.539), 16 1/2 ft (ii.784) and "
  "15 1/2 ft (ii.852). Addy prints all without resolving; all rows carried under "
  "conflict_group=long-yoke.",
  "rules_v1 conflict_group long-yoke"),
 ("NF03", "unresolved conflict - do not reconcile", "Gwilt's stair pitch",
  "The same printed page (668) carries a constant tread x riser product of 66 AND "
  "Newland's pairing table, which violates it by up to 47% at the extremes. Both carried "
  "under conflict_group=gwilt-stair-pitch.",
  "rules_v1 conflict_group gwilt-stair-pitch"),
 ("NF04", "recorded-as-printed flag", "Cholsey barn '51 feet high'",
  "Addy (quoting Parker) prints 303 ft long and 51 ft HIGH; the usual published figure for "
  "Cholsey is a WIDTH of about 54 ft. Probably a slip for 'wide' - recorded as printed, "
  "never silently corrected.",
  "building_bom RESERVE-cholsey rows"),
 ("NF05", "datum conflict", "Coucy donjon height",
  "64 m (from the ditch bottom, stated), 65 m (same article's footnote, 'environ'), 55 m "
  "(the Donjon article, datum unchecked - open queue item). Rows carried verbatim; no "
  "canonical height may be chosen until the datums are resolved.",
  "building_bom BC-06B-coucy rows"),
 ("NF06", "provenance constraint", "Carcassonne / Pierrefonds dimensions",
  "Both are Viollet's own restoration sites: any crenel sill, hoarding or embrasure "
  "measured there is his WORK, not medieval fabric. Rows flagged uncertainty_type="
  "provenance; the older-manifest re-class remains an open queue item.",
  "building_bom BC-07B rows; findings queue item 3"),
 ("NF07", "plan reliability constraint", "the Kensworth conjectural plan",
  "Addy's 'drawn to scale' plan measures only approximately to scale (lengths ~4%, "
  "breadths ~8%, axes differing ~12%); it yields NO number the text does not already "
  "state. Topology reference only.",
  "findings doc section 56 cycle 49; BC-01A"),
]


def build_derived():
    import csv as _csv
    dp = os.path.join(MANIFESTS, "roof_pitch_derived_v1.tsv")
    NAME2CID = {"Kensworth manor house — hall": "BC-01A-kensworth",
                'Kensworth manor house — "house" (domus)': "BC-01A-kensworth",
                "Kensworth manor house — bower (thalamus)": "BC-01A-kensworth",
                "Walton manor house — great barn": "BC-03A-walton",
                "Gunthwaite Hall barn": "BC-00-gunthwaite",
                "Peak (Peveril) Castle keep — upper room": "BC-06A-peveril"}
    rows = []
    for r in _csv.DictReader(open(dp), delimiter="\t", quoting=_csv.QUOTE_NONE):
        cid = NAME2CID.get(r["building"])
        if cid is None:
            raise CensusError("derived row unmapped: %r" % r["building"])
        rows.append([f"D{len(rows):03d}", cid, r["building"], "roof pitch",
                     r["pitch_deg"], "deg", "DERIVED",
                     "atan2(rise, breadth/2); rise and breadth from BOM rows",
                     r["safety"], r["span_assumption"], "addy1898", "n%s" % r["leaf"]])
    return rows


class CensusError(Exception):
    pass


def validate():
    errs = []
    sids = [s["source_id"] for s in SOURCES]
    if len(sids) != len(set(sids)):
        errs.append("duplicate source_id")
    cids = [c["cid"] for c in CANDIDATES]
    if len(cids) != len(set(cids)):
        errs.append("duplicate candidate id")
    for c in CANDIDATES:
        if c["slot"] not in SLOTS:
            errs.append("%s: unknown slot %s" % (c["cid"], c["slot"]))
        if c["gate"] not in ("PASS", "PENDING", "NONE"):
            errs.append("%s: bad gate %s" % (c["cid"], c["gate"]))
        if c["gate"] != "NONE" and c["source_id"] not in sids:
            errs.append("%s: dangling source %r" % (c["cid"], c["source_id"]))
        if c["gate"] != "NONE" and not c["leaf_ref"]:
            errs.append("%s: no leaf/source ref" % c["cid"])
    for s in SLOTS[1:]:
        n = sum(1 for c in CANDIDATES if c["slot"] == s)
        if n != 2:
            errs.append("slot %s has %d rows, needs exactly 2 (real or NONE)" % (s, n))
    if sum(1 for c in CANDIDATES if c["slot"] == "00_baseline") != 1:
        errs.append("baseline must be exactly one row")
    if errs:
        raise CensusError("FAIL-CLOSED: " + "; ".join(errs))


def W(path, header, rows):
    with open(os.path.join(HERE, path), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)
    return len(rows)


def main():
    validate()
    out = {}
    out["sources_v1.csv"] = W("sources_v1.csv",
      ["source_id","ia_id","title","leaves","status","offset_rule","fraction_trap",
       "other_traps","verified_leaves","rights","rights_class","role"],
      [[s["source_id"], s["ia_id"], s["title"], s["leaves"], s["status"], s["offset_rule"],
        s["fraction_trap"], s["other_traps"], s["verified_leaves"], s["rights"],
        s["rights_class"], s["role"]] for s in SOURCES])
    out["candidates_v1.csv"] = W("candidates_v1.csv",
      ["candidate_id","slot","name","place","period","source_id","leaf_ref",
       "evidence_class","gate","gate_note","verification","fantasy_role"],
      [[c["cid"], c["slot"], c["name"], c["place"], c["period"], c["source_id"],
        c["leaf_ref"], c["evidence"], c["gate"], c["gate_note"], c["verification"],
        c["fantasy_role"]] for c in CANDIDATES])
    bom, dispo = build_bom()
    validate_bom(bom)
    rules = build_rules()
    for r in rules:
        if r["rule_type"] == "rule" and not r["quote"].strip():
            raise CensusError("rule %s has no quote" % r["rule_id"])
    from collections import Counter as _C
    cg = _C(r["conflict_group"] for r in rules if r["conflict_group"])
    for g, n in cg.items():
        if n < 2:
            raise CensusError("conflict_group %s has only %d member" % (g, n))
    out["rules_v1.csv"] = W("rules_v1.csv",
      ["rule_id","rule_group","subject","element","value","unit","value_m",
       "conflict_group","rule_type","source_id","ref","evidence_class",
       "verified_full_res","quote"],
      [[r["rule_id"], r["rule_group"], r["subject"], r["element"], r["value"], r["unit"],
        r["value_m"], r["conflict_group"], r["rule_type"], r["source_id"], r["ref"],
        r["evidence"], r["verified_full_res"], r["quote"]] for r in rules])
    out["negative_facts_v1.csv"] = W("negative_facts_v1.csv",
      ["fact_id","kind","subject","statement","where_recorded"],
      [list(x) for x in NEGATIVE_FACTS])
    out["derived_v1.csv"] = W("derived_v1.csv",
      ["derived_id","candidate_id","building","quantity","value","unit","evidence_class",
       "formula","safety","assumption","source_id","leaf_ref"], build_derived())
    cells, meta = build_tally(bom)
    cand_order = [c["cid"] for c in CANDIDATES if c["gate"] != "NONE"] + \
                 ["RESERVE-beetham", "RESERVE-cholsey"]
    elems = sorted(cells, key=lambda k: -sum(v for v in cells[k].values()
                                             if isinstance(v, float)))
    out["element_matrix_v1.csv"] = W("element_matrix_v1.csv",
      ["canonical_element"] + cand_order,
      [[e] + [("" if c not in cells[e] else
               ("u" if cells[e][c] == 0.0 else
                ("1" if cells[e][c] == 1.0 else "0.5"))) for c in cand_order]
       for e in elems])
    out["element_tally_v1.csv"] = W("element_tally_v1.csv",
      ["canonical_element","mapping_layer","n_rows","n_candidates_weighted",
       "weighted_evidence","status","example_elements"],
      [[e, ";".join(sorted(meta[e]["layers"])), meta[e]["rows"],
        sum(1 for v in cells[e].values() if isinstance(v, float) and v > 0),
        round(sum(v for v in cells[e].values() if isinstance(v, float)), 1),
        "PROVISIONAL - coverage instrument, not a production queue",
        "; ".join(meta[e]["examples"])] for e in elems])
    out["section_inventory_v1.csv"] = W("section_inventory_v1.csv",
      ["entry_id","source_id","leaf","printed_page","figure_no","caption","topic_tags",
       "section_keyword_in_caption","dimensioned","priority","status"], build_sections())
    out["building_bom_v1.csv"] = W("building_bom_v1.csv",
      ["bom_row_id","candidate_id","building","element","value","unit","value_m","datum",
       "evidence_class","uncertainty_type","verified_full_res","source_id","leaf_ref",
       "quote"],
      [[r["bom_row_id"], r["candidate_id"], r["building"], r["element"], r["value"],
        r["unit"], r["value_m"], r["datum"], r["evidence"], r["uncertainty_type"],
        r["verified_full_res"], r["source_id"], r["leaf_ref"], r["quote"]] for r in bom])
    return out, bom, dispo


if __name__ == "__main__":
    out, bom, dispo = main()
    for k, v in out.items():
        print("%-24s %3d rows" % (k, v))
    from collections import Counter
    print("gates:", dict(Counter(c["gate"] for c in CANDIDATES)))
    print("slots covered:", len(set(c["slot"] for c in CANDIDATES)), "of", len(SLOTS))
    print("\n--- vernacular manifest disposition (exclusive):", dispo,
          " total:", sum(dispo.values()))
    print("--- BOM per candidate:")
    cc = Counter(r["candidate_id"] for r in bom)
    for cid, n in sorted(cc.items()):
        print("   %-22s %3d rows" % (cid, n))
    print("--- evidence:", dict(Counter(r["evidence"] for r in bom)))
    print("--- verified_full_res:", dict(Counter(r["verified_full_res"] for r in bom)))
    print("--- PASS candidates without rows (crossovers expected):",
          [c["cid"] for c in CANDIDATES if c["gate"] == "PASS"
           and c["cid"] not in set(r["candidate_id"] for r in bom)])
    import csv as _csv2
    rl = list(_csv2.DictReader(open(os.path.join(HERE, "rules_v1.csv"))))
    print("--- rules by group:", dict(Counter(r["rule_group"] for r in rl)))
    print("--- conflict groups:", dict(Counter(r["conflict_group"] for r in rl
                                               if r["conflict_group"])))
    dv = list(_csv2.DictReader(open(os.path.join(HERE, "derived_v1.csv"))))
    print("--- derived safety:", dict(Counter(r["safety"] for r in dv)))
    si = list(_csv2.DictReader(open(os.path.join(HERE, "section_inventory_v1.csv"))))
    print("--- sections: %d entries (%s); priority: %s; status: %s" % (len(si),
          dict(Counter(r["source_id"] for r in si)),
          dict(Counter(r["priority"] for r in si)),
          dict(Counter(r["status"] for r in si))))
    tl = list(_csv2.DictReader(open(os.path.join(HERE, "element_tally_v1.csv"))))
    print("--- tally: %d canonical elements; layers: %s" % (len(tl),
          dict(Counter(r["mapping_layer"] for r in tl))))
    print("--- top coverage:")
    for r in tl[:8]:
        print("   %-24s rows=%-3s cands=%-3s W=%s" % (r["canonical_element"],
              r["n_rows"], r["n_candidates_weighted"], r["weighted_evidence"]))
