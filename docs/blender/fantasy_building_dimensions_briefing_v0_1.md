# Building dimensions for a fantasy setting — briefing v0.1

_Generated 2026-07-30 from a 22-agent sweep of Viollet-le-Duc's Dictionnaire (533 articles) and the 86-book public-domain shelf. Backing data: `reference_manifests/fantasy_building_dimensions_v1.tsv` (317 rows) and `reference_manifests/building_dimensions_v1.tsv` (686 consolidated earlier rows)._

# Building Dimensions: Coverage Briefing

**Corpus:** Viollet-le-Duc *Dictionnaire* (French, metric) + an 86-title English shelf (Gwilt, Turner/Parker, Gotch, Garner & Stratton, Dollman, Brandon, Addy, Bond). 317 mined rows across 14 families, on top of ~700 rows already banked in `fortification_dimensions_v1.tsv`, `viollet_dimensions_v1.tsv`, `domestic_dimensions_v1.tsv`, `timber_roof_scantlings_v2.tsv`.

---

## 1. What is well served, what is thin

**BUILDABLE NOW (shell + enough interior to close a section)**

| Family | State |
|---|---|
| **stair** | Best-parameterised family in the corpus. Gwilt gives a tread/riser *table* and a constant-product rule; Viollet gives the medieval vis. Only headroom is missing. |
| **tower** | Shell is complete and includes a working parametric algorithm for arrow-loop placement. Interiors are empty. |
| **curtain_rampart** | Viollet states an explicit anthropometric merlon/crenel formula plus a fully dimensioned timber hoarding grid. |
| **hall_manor** | The English shelf gives length × width × **height** triples for ~15 named halls, plus wall thicknesses and window sill heights. |
| **timber_frame** | The only family where a full facade grid was recovered — plate-scaled off Châteaudun fig. 4 against a 5 m scale bar, cross-checked three ways. |
| **town_house** | Element-level (doors, windows, joists, hearths, shopfronts) is rich. Building-level (frontage) is absent. |
| **keep_donjon** | ~85 rows, but they are *besieger's* dimensions: diameter, height, base thickness, ditch. Interiors near-empty. |

**THIN — usable fragments only:** `bridge` (~17 statements; deck width clusters hard at 4.90–5.60 m, everything else single-instance), `town_gate` (passage sizes excellent, machinery zero), `church_chapel` (parish plan data from Bond; Viollet's church articles are numerically silent), `undercroft` (one coherent building, Southwark, Turner leaf n163 — everything else is isolated sentences).

**GENUINELY EMPTY — this is a real finding, not a search failure:**

- **workshop_mill / forges.** Not one measured dimension for a forge anywhere in the corpus. No hearth size, no anvil clearance, no bellows bay, no chimney bore. Viollet has no *Forge* article at all.
- **Mill machinery clearances.** Zero. No wheel diameter, no millstone diameter, no headroom over the stones, no shaft height — despite *Moulin* being 12k chars and describing the mechanism at length.
- **Cloister walk width and bay spacing.** Printed **nowhere**, in 370k chars of monastic articles. Viollet gives bay *counts* obsessively (Le Thoronet 8/gallery, Sémur 2/side) and never a clear span. No chapter house article exists.
- **Roof pitch for any hall.** Zero degrees-of-pitch statements across four English books. Back-derivable from Hampton Court (40 ft span / 41 ft to hammer-beams / 60 ft apex) and nowhere stated.
- **Dais height.** The vocabulary is everywhere; the number is nowhere in the corpus.
- **Stair headroom.** Gwilt para. 2805 *defines* headway and prints no figure — verified on the page image, so it is absent from the book, not lost to OCR. Viollet never states it either.
- **Named-but-empty articles** (do not re-fetch): *Cave* (1595 ch, 0 numbers), *Église* (99k, 0), *Chapelle* (85k, 0), *Bretèche*, *Courtine*, *Enceinte*, *Barbacane*, *Herse*, *Vantail*, *Citerne*, *Conduite*, *Cul-de-basse-fosse*, *Perron*, *Grange*, *Moulin*.

---

## 2. The dimensional spine

Ranges below are what a modeller can commit to today. Full provenance and quotes are in the mined-rows table.

### Stair — the strongest spine in the corpus
- **Tread × rise = 66 (inches).** Canonical pair 12 in × 5.5 in. Full pairing table runs 5:9 → 14:4.5, i.e. **pitch 60.9° down to 17.8°**.
- Common house stair tread **10 in**; best staircases **12–18 in** (0.30–0.46 m). Two-tier rule, directly separates service from principal stairs.
- Clear width for two to pass: **≥ 4 ft (1.22 m)**.
- Medieval vis (Viollet): cage internal Ø **~1.90 m** (six pieds), emmarchement **≤ 1.00 m**, giron **0.28–0.30 m**, rise **0.15–0.20 m**, **~16 steps/turn**.
- Newel: solid pillar up to **0.76 m** Ø, above that a thin shell wall. Stair-tower shell as thin as **0.23 m** (Saint-Merri — a freak, cited as such).
- Stone step thickness = **half the step length in feet, expressed in inches** (5 ft step → 2.5 in); 1 in joggle behind each riser.
- Mural stair rock-cut risers **0.30–0.40 m** (Château-Gaillard); mural stairs need a wall **≥ 4 m** thick (Étampes).

### Tower
- Small flanking tower **6.00 m** OD above talus, walls **1.20 m**, three storeys + crenellated stage. Large town tower **~20 m** OD (Nuremberg, measured 5 m up a battered wall).
- **Arrow loop (complete parametric rule):** strike arc **2.20 m** inside the circumference → divide into **16**; divide outer circumference into **8**; sight-line origins offset **0.30 m** from the curtain face; loops staggered *pleins sur vides* storey to storey. Residual wall at the niche **0.70 m**; external slit **0.06 m**; horizontal field of fire **35°**.
- Hoarding: sockets **0.30 × 0.30 m** at **5 pieds ≈ 1.62 m** centres, **0.30 m** square oak, projection **≤ 1.95 m** (one toise), two-deck spacing **1.80 m**. Stone-corbel variant (Coucy): 48 corbels, **1.07 m** projection × **0.30 m** thick.
- Merlon thickness **0.38–0.58 m**, height **1.60–1.80 m** (Carcassonne — soft, see §3).

### Curtain / rampart
- Curtain thickness **2–3 m** (pre-gunpowder). Wall-head parapet **0.50–0.70 m** (peacetime stone).
- **13th-c merlon formula:** height **2.00 m**, width **1.70–3.30 m**, thickness **0.45 m**; crenel sill **1.00 m** above the walk; crenel **0.70 m** wide.
- **Wall-walk width = wall thickness − merlon thickness** (merlons sit flush on the outer face) → **1.55–2.55 m**, matching the banked "2 m maximum". Hard functional floor: **two men abreast**, achieved by corbelling the paving inward on thin walls.
- Machicolation: parapet **0.33–0.40 m** thick × **2 m** high; drop-hole **0.33–0.40 m** square; corbels **0.70–1.20 m** axis-to-axis.
- Postern **2.00 × 0.90 m**; gate passage progression by century **3.0 → 3.5 → 4.2 m**.
- Access control (typology, no metres): Carolingian = direct steps from the terreplein; from the 12th c. the walk is reachable **only through the towers**, ironbound doors between.

### Hall / manor (English, feet)
- **Plan:** 29 × 20 ft (Wilderhope) → 106 × 40 ft (Hampton Court). Cluster of real great halls **50–71 ft × 27–39 ft**.
- **Height:** ceiled manor hall **13 ft 9 in**; open hall to wall-plate/springing **20–26 ft**; cathedral-scale **41 ft to hammer-beams, 60 ft to apex**.
- **Wall thickness:** ordinary domestic **2 ft – 2 ft 6 in**; priory refectory **3 ft 6 in**; vaulted undercroft **3 ft 3 in**; tower-house **8–20 ft**.
- **Window sill 10–12 ft above the floor** as a stated rule (7 ft at Hoghton, where the wall is glazed down to panelling). This is the single most game-relevant number in the family — it governs whether a thief reaches a sill.
- **Screens stop 10–12 ft up**, with a gallery floor over them.
- Newel stair Ø **5 ft 8 in / 10 ft / 11 ft**; step width **5 ft**; newel post **7 in**.
- Cellar headroom **7 ft 3 in**; mural chamber **8 ft 6 in × 7 ft**.
- French vertical stack (Viollet, *Manoir*, the only closed section in the corpus): hall **4.30 m** + beams **0.60 m**; entresols **2.30 m** each + floor **0.30 m**; ground storey **≤ 2.65 m**; joists **0.30 m**.

### Timber frame (plate-scaled, ±0.05 m)
- **Post grid 0.90 m**, and the window light (0.74 m) + meneau post (0.19 m) = the same 0.91 m module. A facade is a string of 0.90 m bays flagged panel / window / door.
- Storeys **3.35 / 3.35 / 2.80 m**; clear headroom **2.70 m**.
- Door **1.00 × 2.15 m**; window light **0.74 m × 1.27–1.84 m** on a **0.80–0.90 m** sill.
- Posts **0.16–0.19 m**; joists at **0.60–0.90 m** centres; sablière **0.30 × 0.24 m**; chevrons **0.45–0.63 m**; tile-roof rafter entre-axe **0.22 m**.
- Roof pitch shift **40–50° → 60–65°** across the medieval period (Viollet, *Charpente* — the only pitch data in the whole corpus, and it is French roofs, not English halls).

### Town house / urban fabric
- Ground storey **3–4 m**; storeys under **3 m** condemned by Viollet as too low. English ground storeys **9 ft 9 in – 11 ft 1 in**.
- Door **1.0–1.5 m** wide; window splay **0.60 m** deep (a body-sized reveal); glazing bay capped near **1.00 m** so a mullion must be broken or squeezed past.
- Hearth **2.57 × 1.66 m** (Jacques Cœur) — big enough to stand inside. Palace scale **10.00 m × 2.30 m under the mantel**.
- Street **10 m**, rear ruelle **3 m** (Montpazier); plot 100 m² with 49 m² built (Vitteaux) → back-solves to ~7 × 7 m.
- London Assize 1189: party wall **3 ft**, masonry to **16 ft** before the timber gable.

---

## 3. Conflicts and suspicious ranges

**Same author, two articles, two answers — a modeller must pick:**
- **Coucy donjon:** 30.50 m Ø / 55 m high (*Donjon*) vs **31 m Ø / 64 m** (*Château*), with a footnote in the same article saying **65 m**.
- **Louvre donjon:** 20 m Ø × **~40 m** (*Donjon*) vs 20 m Ø × **30 m** (*Château*).
- **Poitiers grand'salle width:** 16.00 m (*Salle*) vs 16.30 m (*Cheminée*). Treat as 16.0–16.3.

**Geometrically impossible as stated:** the 13th-c formula gives a 2.00 m merlon with the crenel sill at 1.00 m — ~1.00 m of merlon above the sill — yet Viollet also says the hoarding floor sits level with the sill and men walk **through the crenels as through doors**. Both cannot be literally true. Carcassonne's measured merlons (1.60–1.80 m) sit with sockets "a little below the crenel sill". Pick one and flag it.

**Suspiciously wide, and the width means something:**
- **Merlon width 1.70–3.30 m** — a factor of ~2, and no repeat-pitch rule is ever given. A generator must sum merlon + crenel and guess.
- **Jetty 0.28 m (Châteaudun, plate-scaled) vs 1.65–2.00 m (Reims, Paris, Normandy).** These are *different systems* — a plain jetty vs an encorbellement carried on potences. Do not average them; branch on system. English comparison: Lavenham **18 in**.
- **Merlon thickness 0.38–0.58 / 0.45 / 0.50 / 0.50–0.70 m** across four articles — these are consistent, not contradictory. **0.40–0.60 m is the safe default**, 0.70 m for a fortified-town wall head.

**Internal contradictions already flagged in the manifests, still standing:** Viollet's "comfortable 22° pitch" is arithmetically inconsistent with his own 0.15–0.20 / 0.28–0.30 pair (which gives ~27–36°) — **use the linear pair**. The joinery vis "12 steps per circumference" is contradicted by his own plate (~16). The Louvre giron is measured near the newel (Sauval's convention), contradicting Viollet's own *Giron* article, which measures at mid-length.

**Almost certainly corrupt text — do not admit without checking the plate:** *Porte* fig. 50 postern at "0 m,50" wide (not passable by a person; fig. 46 gives 1 m for the same type). *Porche*, Saint-Front de Périgueux, a porch "de forme carrée" given as "10 m,30 × 0 m,65".

**Definition collision, do not dedupe:** 0.33–0.40 m appears in two manifests meaning different things — machicolation *parapet thickness* vs machicolation *drop-hole square*.

**Term collision:** "bay" is not one thing. Eltham = 17 ft (structural principals); Crosby Place = 54 ft in eight bays ≈ 6.75 ft (roof panels). Never mix them in one generator.

**Evidence-class caution, corpus-wide:** Carcassonne and Pierrefonds were Viollet's **own restoration sites**. Any datum taken from a crenel sill, machicolation walk, or hoarding floor there measures *his* work. The Carcassonne merlon heights and curtain heights are the specific rows to distrust. The Meurtrière loop-layout rule is datum'd on the talus top and the tower circumference — both surviving fabric — which is why it survives the objection and is the most trustworthy generator input in the whole set.

**Two unit traps that cost a factor of 100 / silently drop half the data:** `0,45 c.` means 0.45 m (c. = centimètres *of a metre*), not 0.45 cm. And the metre regex must be `\d+\s*m\s*[,.]\s*\d+` — HTML stripping leaves a space before the comma, so `\d+\s*m[,.]\d+` matches nothing. This under-reported *Manoir* from 8 hits to 0 on the first pass.

---

## 4. What is missing, and where it would have to come from

| Gap | Route |
|---|---|
| **Stair headroom** | Not in the corpus at all. Import from a modern code, label as derived. The corpus's only lever is Gwilt's theatre rule ("thirteen for headway"), implying ~2.6 m/turn at a 0.20 m riser. |
| **Tower & keep storey heights** | The largest single hole. Zero floor-to-floor for any tower or round-keep interior. Requires an acquisition: **G. T. Clark, *Mediaeval Military Architecture in England* (1884)**, which prints keep plans dimensioned in feet *with wall thickness per storey* — exactly Viollet's blind spot. Not on the shelf. |
| **Arrow-loop heights, sill, niche height** | Figure-only. Viollet says explicitly the slit length varies with height above external ground and refers you to the plate. Scale off the plates or pick. |
| **Crenel opening width / head form** | *Créneau* was mined for the formula but never gives a measured crenel opening; the coping profile after the Gallo-Roman capping slab was abandoned is never drawn in numbers. |
| **Portcullis groove, drawbridge pit** | Checked every occurrence of *rainure* (×7) and *pont-levis* (×35) in the 265k-char *Porte* — **not one number**. Recoverable only from the plates, which carry printed scales (1:500, 1:400, 1:100, 1:10). |
| **Cloister walk width & bay spacing** | Not printed. But Viollet **states the drawn scale** of the monastic plates in the text: Sémur cloister plan **1:200**, Cluny general plan **1:2000**, arcade elevation 1:40, pier sections 1:20/1:50. Plate-measurable. |
| **Church nave / aisle / bay plan** | Same route: *Chapelle* plans at **1:400 and 1:200**, *Cathédrale* at **1:1000**. Bowman & Crowther (`gri_33125016336048`) returned **zero** ft-in in OCR across 246 leaves because the dimensions are lettered on the drawings. |
| **Échauguette interiors** | Given as **occupancy, not geometry**: 1 man (Provins), 2–3 (Villeneuve), 4 (Chambois). 27 plates remain the only route to real geometry. |
| **Undercrofts** | Turner & Parker **vols 2–4** + Garner & Stratton, keyed on undercroft/vaulted/groined. Dollman is measured drawings — plate extraction, not OCR (a full OCR sweep of 386 pages returned one usable hit). |
| **Forges, mill machinery, granary loading** | Outside this shelf entirely. Would need an engineering source, or scaling off Viollet's Bagas plates (he gives figure refs A–P). |
| **Town-house frontage width** | No source gives it. Only handle is the Vitteaux 49 m² footprint. A modeller must pick by fiat. |
| **Infill panel depth** | Viollet names both breakable fillings (torchis; mortar-and-moellon) and dimensions neither. Panel *face* size is on the 0.90 m grid; panel *depth* is unrecoverable. |
| **Hall roof pitch, dais height, screens-doorway width** | Absent from four English books. Roof pitch is back-derivable from section triples; the other two are not recoverable from this corpus. |

**The named unlock is plate measurement.** It is the only method that has already converted a "thin" family to buildable: Châteaudun fig. 4 carried a 5 m scale bar, calibrated three independent ways (staffage man = 1.67 m; section posts 0.16–0.19 m against Viollet's printed "17 à 19 centimètres"; door = a round 1.00 m), and produced the entire timber-frame facade grid that the text withholds. The same route is sitting unused on *Porte* (149 figures with printed scales), *Échauguette* (27), *Cloître* (43), *Chapelle*, and the whole Dollman/Bowman & Crowther measured-drawing corpus. Prioritise it over any further text mining — Viollet's text is close to exhausted for towers, keeps and town houses.

---

## 5. Start here — three building types

**1. Round flanking tower (6 m OD class).** The only family with a *complete parametric algorithm* rather than a bag of facts: shell diameter, wall thickness, and a working arrow-loop setting-out rule that closes and is datum'd on surviving fabric. The wall-head is fully specified (merlon 0.40–0.60 × 1.60–2.00 m, hoarding on a 1.62 m socket grid projecting ≤1.95 m). Vertical circulation is borrowable wholesale from the banked vis-stair rows (Ø 1.90 m cage, 0.28–0.30 m giron, 0.15–0.20 m rise). **One decision required:** storey height, which the corpus does not supply — derive it as risers × steps-per-turn and label it derived.

**2. English great hall + screens + undercroft.** The only family where length, width **and** height arrive together, for ~15 named buildings, with wall thicknesses and a stated window-sill rule. It generates the archetypal explorable interior in one go: a 50–70 ft clear volume, screens stopping at 10–12 ft with a gallery over, a 7 ft 3 in undercroft below, mural chambers at 8 ft 6 in × 7 ft, and windows sitting 10–12 ft off the floor — a printed, sourced number that directly decides whether a thief reaches a sill. Take roof pitch from the Hampton Court section triple and flag it as derived.

**3. Jettied timber-frame town house.** The only family with a *proven* full facade grid: a 0.90 m post module in which windows, panels and doors all sit, closed vertically at 3.35 / 3.35 / 2.80 m with 2.70 m clear headroom, and openings dimensioned (door 1.00 × 2.15 m; window light 0.74 × 1.27–1.84 m on a 0.80–0.90 m sill). Breakable infill, a climbable frame, a 0.60–0.90 m joist grid for ceiling voids, and jetties that narrow the street overhead. Branch the jetty on system (0.28 m plain vs 1.65–2.00 m on potences) and pick a frontage — the corpus will not give you one.

**Do not start on:** forge, mill, cloister, or church interior. The first two have no numbers at all; the second two have numbers only behind an unexecuted plate-measuring pass.