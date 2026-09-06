# Architecture books — mining findings v0.1

2026-07-29 · Companion to [architecture_books_library_v0_1.md](architecture_books_library_v0_1.md) (the shelf) — this is the **record of what is actually inside**, built the way the scout lane treats museum objects: nothing counts until it has been opened and looked at. Plate-level index: [reference_manifests/architecture_book_plates_v1.tsv](reference_manifests/architecture_book_plates_v1.tsv). Status tags follow lane convention — `VERIFIED` = image inspected at full resolution, `PLATE` = located and mapped but not yet eyeballed.

**Cycle 1 covers: F. A. Paley, *A Manual of Gothic Mouldings*, 1845** (IA `manualofgothicmo00pale_0`, Getty Research Institute scan, 158 leaves). Chosen first because molding profiles are the shortest path from a book to factory geometry — a profile is already a sweep curve.

## 1. The Internet Archive mining recipe (new — the lane only had the Commons one)

Cycle 11 established page-thumbnail extraction for Wikimedia Commons. IA needs a different and **cheaper** approach, because IA exposes an OCR layer: **triage by text first, fetch images only for confirmed targets.** Tool: `books/iabook.py` in the session scratchpad (rebuildable; ~120 lines).

```
1. https://archive.org/metadata/<id>            -> imagecount, dir, server, rights fields
2. <id>_hocr_pageindex.json.gz                  -> per-page offsets
3. <id>_hocr_searchtext.txt.gz                  -> full OCR text, sliced by those offsets
4. regex over per-page text                     -> candidate leaves
5. https://archive.org/download/<id>/page/n<N>_w<W>.jpg   -> page image
```

Three traps found, all of which silently produce wrong answers rather than errors:

- **The page index has two offset pairs.** Entries are `[searchtextStart, searchtextEnd, hocrStart, hocrEnd]`. Elements 2/3 index the hOCR *HTML* (millions of bytes); using them against the searchtext yields empty or garbage slices with no error. **Always slice with elements 0/1.** Sanity check: the last entry's element 1 should be within a few hundred bytes of the searchtext length.
- **`_djvu.txt` has no page separators on this scan** (zero form feeds), so the obvious "split the plain text into pages" approach silently returns one giant page. The pageindex route is the reliable one. Also note plain `curl` without `-L` returns 0 bytes on IA downloads — **follow redirects**.
- **On engraved-plate books, OCR emptiness is the PLATE signal, not the blank signal.** Engraved running heads and figure numbers are invisible to OCR, so plates and true blanks both read as empty; my first heuristic ("sparse text = plate") selected the blank *versos* instead and produced a page of foxing. What works: **JPEG file size on cheap thumbnails.** Fetch the whole candidate range at `w400` and histogram the bytes — line-art plates land 2× the blank median with no overlap at all. On Paley: blanks 14–17 KB, plates 25–41 KB, and the plate list fell out with gaps of exactly 4.

Two more practical notes: the `_w<N>` width suffix is **advisory** — IA returned the full 1930 px image for `w1100` and `w1400` alike, so budget bandwidth by leaf count, not requested width. And for reading many plates at once without any image library (no PIL, no ImageMagick on this Mac), **crop in CSS**: put each image in a fixed-height `overflow:hidden` box with a negative `margin-top`, and one screenshot reads all 16 running heads as a stack.

## 2. Paley 1845 — structure

158 leaves: front matter to n13, **treatise text n14–n87**, **plate section n88–n148**, publisher's advertisements n150–n154, covers n155–n157.

**The plates sit at `leaf = 88 + 4×(plate−1)`, exactly 16 of them**, Plate I at n88 through Plate XVI at n148 — a perfectly regular tipped-in rhythm (plate recto, blank verso, two blanks). The one leaf that breaks the size histogram (n150, 40 KB) is not a 17th plate but the publisher's advertisement page, dense with type.

| Plates | Subject |
|---|---|
| I–III | General moulding sections — serve the introductory, principles, and copying chapters |
| IV–V | Early English mouldings |
| VI–VII | Decorated mouldings (VII is the most-cited plate in the book, 20 references) |
| VIII–IX | Perpendicular mouldings |
| X–XIII | Capitals, one plate per period (X Early English, XI Decorated, XII continued, XIII Perpendicular) |
| XIV–XV | Bases |
| XVI | Strings and labels (string courses and hood moulds) |

Per-plate detail, figure counts, and text cross-references are in the TSV.

## 3. What the plates actually contain (VERIFIED)

**Plate I (n88)** — 25 numbered figures, each a moulding cross-section. **Plate X (n124)** — 40 figures of capitals.

The drawing conventions are consistent and, unusually for a period book, **stated in the text rather than left implicit**:

- **A profile is defined as a true section**: "the appearance it would present if cut through in a line at right angles to its bearing." That is a swept profile, defined the way we would define it.
- **Fixed orientation.** "Draw the outer wall-line parallel with the bottom of the page, and the soffit parallel to the side," and always take the same side of a doorway. So every profile in the book shares one coordinate frame — wall-plane horizontal, soffit-plane vertical — which means the plates can be traced into profile curves without per-figure guessing about which way is up. This is the same kind of declared convention our own component docs open with.
- **Hatched solid = stone, white = air.** The boundary between them *is* the profile curve.
- **The dashed rectangle behind many figures is the uncut stock block.** This is the best single find of the cycle: the plates draw both the finished section and the square of stone it was cut from, so each figure reads as a stock-removal operation rather than a decorative outline. That is exactly the framing the sword handoff arrived at for ground geometry ("a sword needs direct geometry only to look *used*"), and it means a moulding generator should start from the block and subtract, not extrude an outline.
- **Real dimensions are present on a subset of figures**, in feet and inches (Plate I figs 6, 12, 19; Plate X figs 29, 30 — e.g. 1 ft, 11 in, 10½ in, 6¾ in, 7½ in). These are measured medieval mouldings, so they feed the dimension dataset the same way CMA measurement strings do.
- **Capitals are drawn as revolve profiles.** Plate X gives small elevations (fig 5 keyed A = abacus, B = bell, C = astragal) alongside ~30 profile curves and three large detailed elevations. Since a capital on a circular shaft is a surface of revolution, those curves are directly a lathe library. The text even specifies the two numbers to record: depth from top of abacus to underside of astragal, and projection of the abacus over the shaft.
- **Setting-out geometry is shown, not just results** — Plate I fig 25 carries a 45° grid with cross marks, and the text describes hollows "of three-quarters of a circle, accurately formed with the compasses." Chamfers are "generally (not invariably)" 45°, with a named test for when they are not. So the grammar is compass-and-square constructible: arcs of stated fractions struck from grid points, with a default chamfer angle and an explicit exception rate.

**Every figure is provenanced to a named building** — Louth, Wittering, Little Casterton, Attleborough, Trumpington, Tintern Abbey, Over, Histon, St John's College Cambridge. Each profile is a measured survey of a real surviving structure, which gives these the same evidential standing as an accession number, and makes the period attributions checkable.

## 4. Why this matters to the factory

Paley is not a picture book; it is **a parametric grammar of Gothic mouldings with a dating function attached**. Three usable consequences:

1. **A profile library with a declared coordinate frame.** 16 plates × 25–40 figures ≈ 500 sections, all drawn in one orientation, a subset dimensioned. That is a sweep-profile and revolve-profile catalogue we can trace into curve data, and the capitals plates alone would populate a column kit.
2. **Stock-then-subtract is the authentic construction model** for moulded stone, confirmed by the dashed stock boxes. Worth carrying into any moulding node group as the operation order.
3. **The period grammar is a dating function**, which is exactly the "readers" idea from the target-first doctrine: the same doorway silhouette reads as 1200 or 1400 purely by which profile members are used. A `Period` enum (Early English / Decorated / Perpendicular) driving member selection would give three legible architectural eras out of one generator — and the book supplies the discriminating features per period.

## 5. Leads discovered inside the book

The publisher's advertisement leaves (n150–n154) turn out to be a **period bibliography of the same genre** — a discovery channel worth checking at the back of every book we mine. From Paley's:

- **Paley, *Baptismal Fonts*** — "a series of 125 engravings, examples of the different periods, with descriptions." A font monograph; fonts are already a family in the Mason's Yard gallery.
- **Cambridge Camden Society, *Instrumenta Ecclesiastica*** — "a series of **working designs** for the furniture, fittings, and decorations of churches and their precincts," issued in parts of six quarto plates. Working designs for church furniture is a kit source the shelf does not yet have.
- Edmund Sharpe, *Decorated Windows* (already on the shelf) — advertised here in parts, which explains why its scans are split.

Neither of the two new titles has been checked for a digitized scan yet; queued below.

## 6. Cycle 2 — Brandon, Shaw, Dollman surveyed; Ungewitter scoped

Swept 664 leaves across three books at `w400` and mapped every plate by the size histogram. Each book turned out to have a **different plate rhythm**, which is itself the useful generalisation: the rhythm is a fingerprint of how the book was bound, and you must measure it per book rather than assume Paley's.

| Book | Leaves | Rhythm | Plates |
|---|---|---|---|
| Brandon, *Open Timber Roofs* (1849) | 200 | plate → description page → 2 blanks; **some plates in pairs** (24 four-gaps, 14 two-gaps) | ~40 |
| Shaw, *Ornamental Metal Work* (1836) | 122 | plate every **2** leaves, n10–n108 | 50 |
| Dollman, *Ancient Domestic Architecture* (1861–63) | 386 | plate every **4** leaves + dense description runs | ~90 |

### Brandon — the timber source, confirmed (VERIFIED Plate 9)

**Plate 9, "Roof over the Chancel of St Martin's Church, Leicester,"** signed *Measured & Drawn by Raphael & J. A. Brandon*. One sheet carries the truss section (arch-braced, tracery-filled spandrel, embattled panelled cornice, carved corbel head, wall post), a longitudinal view of the boarded and ribbed ceiling, **a printed scale bar in feet**, and — the part that matters most — **member cross-sections at a stated 2-inch scale**: "Section of foot of Rafter and Cornice," "Section of Tie-beam," and the cornice moulding on line A.B. So the book gives truss geometry, member profiles, *and* a scale to read both against.

The description leaves carry **scantling blocks** — timber sizes in inches. OCR recovered only 12 values because these are small engraved annotation blocks, so a full table needs a visual read per plate (queued). What did come through, and is worth having now:

- Common rafters **6 × 3**; purlins **8 × 5** to **10 × 10**; ridge **10 × 9** to **12 × 10**; cornice **10 × 8** to **12 × 9**; principal rafters **12 × 10**
- **Truss bay spacing 12 ft 6 in** (Trinity Chapel, Cirencester)
- A plate note that "all the details are drawn ½ full [size]"

That is already enough to give the structural-oak kit believable member proportions instead of invented ones.

### Shaw — a correction to our own catalogue (VERIFIED)

The shelf entry claimed "hinges, knockers, grilles, keys, candlesticks; literal ironmongery modeling sheets." **That is wrong, and the manifest is now fixed.** Plate n40 is a *Railing in Front of the Montesquieu Baths, at Paris* — Empire/neoclassical, with entwined dolphins and anthemion shells. Plate n64's caption reads "from the Mansion of Thos. Hope Esq… designs by Wm. D. Batley and H. Shaw," proving the book mixes surveyed examples with **new designs by Shaw and his contemporaries**. It is a working designer's pattern book of the 1830s, not a medieval survey. Shaw's medieval material is in his *other* titles, which the shelf lists separately. **For medieval ironwork the right book remains Starkie Gardner's *Ironwork* Part I**, not yet mined.

What Shaw is genuinely good for, and it is a real find: **each plate pairs a dimensioned outline drawing with the same object hand-coloured.** The railing is grey-green *painted* iron with **selectively gilded ornament** — so the plate is simultaneously a measured drawing and a finish specification. Architectural ironwork was painted and parcel-gilt, not bare metal; that is paint-scheme authority for the forged-iron kit whatever the period.

### Dollman — the best whole-building pattern found so far (VERIFIED)

**"St Cross' Hospital — The Refectory & its Details"** (the Hospital of St Cross, Winchester). A single sheet carries: transverse section showing the arch-braced roof with its wind-brace lattice, wall thickness in section and stone coursing drawn; longitudinal section with **room dimensions 32′0½″ × 26′3¾″**; a plan at B.C.; a scale bar in feet; and **seven named, individually dimensioned mouldings** — refectory window, porch archway jamb, inner porch door jamb, entrance gateway jamb, upper string course, string course over the gateway, parapet section.

This is the ideal reference shape for a factory: **the parametric envelope and the detail profiles that dress it, from one real building at one known scale.** Where Paley gives a profile catalogue divorced from buildings, Dollman gives profiles *attached* to a measured room — which is what a building generator actually needs.

**Scan trap:** Dollman's landscape folios are rotated 90° in the scan. Anything automated over these plates needs an orientation check.

### Ungewitter — scoped, not mined

The Ricker translation is **two volumes of 912 + 954 leaves, and it is a typescript**, not typeset. Its OCR carries a trap worth naming: roughly half of every page is **mirror-image bleed-through from the reverse of the thin carbon paper** — `"etluey to aeioege"` is `"vaults of species"` reversed — on top of ordinary typescript confusions (`b`↔`t`, so "building" reads "tuilding"). Fuzzy patterns are mandatory; the measured English-word ratio is a median of 0.22 with the worst decile near 0.09.

Even so, real content surfaced, and it is the most factory-relevant thing in any of these books:

- **A table of vault thrust against vault weight, indexed by rise ratio** (n390), with a stated rule of thumb: for pointed cross vaults of average height, expect a thrust about **⅓ of the half-vault's weight**, entering the wall at about **¼ of the height of the rise**. Worked examples are given for **4 × 4 m and 8 × 8 m bays**.
- **A table of edge pressures on rectangular masonry** by position of the resultant (n417) — middle-third-rule territory.
- The reviser's preface states the third edition added ~400 figures, an entirely rewritten vault section, a new section on abutments, and **tables for vault thrust and for the thickness of walls and buttresses**.

For a game none of this is structural, but it is exactly what makes procedural Gothic read as *built* rather than arbitrary: **buttress mass and wall thickness derived from vault span**, so a generated cathedral's abutment scales with what it is pretending to carry. That is the same "standards-derived" evidence tier the cask ladder used — a rule, not a measurement, but an exact one. Ungewitter deserves its own dedicated cycle; content clusters at n379–n490 (vaults, thrust, tables) and n740–n851 (buttresses, wall thickness).

## 7. Cycle 3 — Ungewitter: one table won, and the scan's real defect found

Targeted the vault-thrust and buttress clusters. Digit-density triage over n360–520 pointed hard at **n395 (38% digits)** and n438 (35%).

### The scan defect — and it changes the method for this book

**n396 is printed page 196 photographed mirror-reversed.** Not bleed-through of a neighbour: the whole page is a reversed ghost, page number and all. This typescript was typed on one side and the scan captured **both faces of every sheet**, so a large share of the 912 leaves are mirror ghosts of their own recto.

Worse for automation, the contamination runs both ways: **n395 reads perfectly by eye yet scores an English-word ratio of 0.083**, because its OCR is dominated by the facing page's mirrored show-through bleeding through thin carbon paper. So:

- The English-ratio metric **cannot** separate readable pages from ghosts — readable pages score like ghosts.
- Digit density still works as a *pointer* to the right neighbourhood, but **a hit may belong to an adjacent leaf**, because the digits OCR'd on n395 were the facing table's.
- Therefore **Ungewitter is a visual mine, not a text mine.** OCR locates a neighbourhood; only an eye confirms what is on the page. At ~900 leaves per volume, full coverage is impractical — targeted reads around known clusters is the only sane strategy, and any future cycle should budget accordingly.

This is worth carrying to other typescript-era scans: a low English ratio is evidence of *show-through*, not of an unreadable page.

### What was actually won (VERIFIED, printed p.195)

A complete **masonry strength table**, read at full resolution and recorded in [reference_manifests/masonry_strength_v1.tsv](reference_manifests/masonry_strength_v1.tsv) — crushing and shearing strength in kg/cm² for nine materials, with the shear direction distinguished relative to the stone's bed:

| | crushing | shearing |
|---|---|---|
| Granite, diorite | 500–1800 | 60–100 |
| Limestone, dolomite | 300–1000 | 30–50 (with bed) / 50–70 (across) |
| Sandstone | 180–900 | 13–40 / 15–40 |
| Limestone tufa, light | 80–200 | 30 |
| Clinker brick | 250–700 | 40–60 |
| Good wall brick | 100–200 | 15–30 |
| Porous/hollow brick | 40–100 | — |
| Cement mortar | 100–200 | 18–30 |
| Hard lime mortar | 50–90 | — |

Why this is useful to us despite being an engineering table: it is a **material capability ladder with a 45× spread from hard lime mortar to granite**, which is the physical reason different stones get used for different jobs — granite for the loaded pier, tufa for the light vault web, brick for filling. A generator that picks material per member can pick it *consistently with what the member has to do*. The bed-direction distinction is also a texture note: stone is anisotropic, and the bedding plane is a visible, orientable feature, not just a strength number.

The accompanying text supplies the failure vocabulary: overturning is checked at pivots — the footing, each widening, and **each abrupt recession of the cross section** — and the author warns that mortar tension and wall-to-footing adhesion "cannot be safely counted on." That is why real Gothic abutment is *stepped*: each recession is a checked pivot.

### The thrust tables: located, not yet read

They sit around **n438**, organised **by rise ratio with a stated span**, e.g. "IV. Rise 2/3 (u = 4.50 m)", "V. Rise 5/6 (u = 5.70 m)", with values in metres. The stability criterion is stated in plain words next to them:

> "Resultant at edge causes overthrow. Resultant at edge of kern ensures abutment if not too great."

That is the middle-third rule as a buttress-sizing test, and it is the rule a generator would apply. The tables themselves need a visual read; a pointer to their own explanation page (printed p.163) is noted in the text but the leaf was not located this cycle.

## 8. Cycle 4 — the buttress tables, read (VERIFIED)

Contact-sheeted n436–n445 first, which confirmed cycle 3's parity finding cleanly: **odd leaves readable, even leaves mirror ghosts**. The two readable tables are n437 and n439. Both read at full resolution. Extracted data: [reference_manifests/buttress_proportions_v1.tsv](reference_manifests/buttress_proportions_v1.tsv).

**Table IV (printed p.216, leaf n437) — "Depth of trapezoidal diminished Buttress", vault A = 4 × 4 m.**
**Table V (printed p.217, leaf n439) — same, vault B = 8 × 8 m.**

Both are rotated 90° in the scan. Structure: for a given vault bay, rise class, and buttress height `t`, the table gives **buttress depth at bottom in metres**, across four load columns (A′ A″ B′ B″) grouped by vault-height case.

Three rules are stated outright, which makes them directly implementable rather than inferred:

- **Taper: depth at top = 5/11 of depth at bottom** (≈0.4545) — hence "trapezoidal diminished."
- **Buttress height is measured to the keystone of the vault.**
- **The sizing criterion, verbatim:** *"Resultant at edge causes overthrow! Resultant at border of kern ensures safety of buttress, if pressure at edge is not too great."* The middle-third rule, used as a buttress test. A cross-reference points to the fuller explanation at printed p.158 (leaf ≈358) — cycle 3's OCR had misread this as p.163.

### The derivation worth having

The two tables together let a real scaling law be checked rather than guessed, because vault A and vault B are the same vault at two sizes:

- **Vault rise scales linearly with span.** The tabulated rises are u = 1.60 / 2.20 / 2.80 m at 4 m span and 3.30 / 4.50 / 5.70 m at 8 m — every pair a factor of 2.03–2.05 for a factor-2 span. So `u/span` is constant per rise class at **0.40 / 0.55 / 0.70**, and those are the numbers to use; the labels "Rise ½ / ⅔ / ⅚" are nominal and do *not* equal the actual rise-to-span ratio.
- **Buttress depth scales sub-linearly with span** — matched cells give roughly **1.2–1.6× depth for a 2× span**, not 2×. Column alignment on the shallowest-rise row is uncertain, so treat the low end as indicative.
- **A steeper vault needs a shallower buttress.** At fixed span and height, depth falls as rise rises: 1.15 → 1.00 → 0.90 m across the three rise classes (4 m vault, row a, t = 10 m). This is the useful design coupling — pitch and abutment trade against each other, so a generator that raises the vault should slim the buttress, not thicken it.

For the factory this is exactly the missing link flagged in cycle 2: **buttress mass can now be derived from vault span, rise and height instead of invented**, with an authored taper ratio and an explicit stability criterion. Combined with the stepped-recession rule from p.195 (every recession is a checked pivot) and the [masonry strength ladder](reference_manifests/masonry_strength_v1.tsv), the abutment side of a Gothic generator has a real basis.

Full cell-by-cell transcription of both tables was **not** attempted — reading ~200 numbers off a rotated typescript invites transcription error, and the rules plus ranges carry most of the value. A careful transcription pass is queued.

## 9. Cycle 5 — Brandon scantlings: the source located, one roof fully read

The scantling hunt turned into a lesson about where data actually lives in a plate folio, and it inverts the assumption cycle 2 made.

### Where the numbers are, and the trap that hid them

Cycle 2 found scantling fragments in the OCR of leaves like n80 and n86 and assumed they were annotation blocks engraved on the plates. **Both assumptions were wrong.** Reading the leaves settled it:

- **n86 is a blank leaf.** Its OCR text was **show-through of neighbouring ink**, not content. So the same show-through trap found in Ungewitter's typescript (cycle 3) recurs here in a fine engraved folio — it is a property of scanned books generally, not of bad typescripts. **Rule: OCR text on a near-blank leaf is a pointer to leaf ± 1, never content.**
- The scantlings are **not on the plates at all.** They are on the **letterpress description pages** (~40–50 KB leaves), one per roof.
- And the sting: **those description pages OCR as completely empty.** `n82`'s OCR is the empty string, yet the page is clean, perfectly legible letterpress. Its content reached the OCR only as show-through onto the adjacent blank. **The ghost was more informative than the original** — which is why the fragments looked like plate annotations.

Practical consequence for this book: **the blanks' OCR is a usable index to what the descriptions contain, but every value must be read visually off the description page itself.**

### St Martin's Church, Leicester — chancel roof (VERIFIED, printed p.47)

n82 is the description for **Plates IX and X**, which also confirms the pairing directly: **Plate IX = n79 (measured working drawing), Plate X = n81 (perspective view)** of the same roof. Cycle 2 inferred the pair from the size rhythm; it is now stated in the book.

Complete measured data, now in [reference_manifests/timber_roof_scantlings_v1.tsv](reference_manifests/timber_roof_scantlings_v1.tsv):

| | |
|---|---|
| Span of roof | **23 ft 0 in** |
| Space between trusses | **12 ft 6 in** (0.54 of span) |
| Tie-beam | **1 ft 8 in × 1 ft 2 in** (20 × 14 in) |
| Purlin | 10 × 10 in |
| Ridge | 10 × 9 in |
| Cornice | 10 × 8 in |
| Common rafter | 6½ × 4½ in |
| Ridge template | 3 ft 6 in long |
| Purlin templates | 2 ft 6 in long |

The prose is as valuable as the numbers, because it gives the **load path and therefore the assembly order**: this is a tie-beam roof with **no principal rafters**. One massive tie-beam crosses the chancel and carries everything — the ridge-piece on a short template resting on a strut tenoned into the tie-beam, the purlins on shorter templates notched onto it. The **cornice is framed into the tie-beam and does not rest on the wall at all**; wall-pieces drop from the tie-beam's underside, and arched braces spring from those. So the entire roof weight is borne by the tie-beam, kept from sagging by the curved braces, with only a wall plate taking the rafter feet.

For the structural-oak kit that is a complete, buildable recipe: one member does the work, everything else is carried on it, and the decorative arch-braces are *structural* (anti-sag), not applied. The generator's dependency graph follows the load path.

One caveat recorded honestly on the plate: this roof was **reconstructed in English oak** to the old model, with carved cornice braces and **angels with outspread wings** replacing the wall-piece corbels (the stone corbels being modern). So the scantlings are a Victorian restoration's, faithful to but not identical with the medieval original — a tier below a surviving-fabric measurement.

### Roof index

Nineteen roof titles were recovered from show-through OCR, overwhelmingly **Norfolk and Suffolk** — the great region for medieval church roofs: Heckington (Lincs, south porch), Holy Trinity Cirencester, Little Welnetham, Capel St Mary, Trunch, Palgrave, Brinton, Starston, Fressingfield, Bacton, **Knapton**, Mattishall (north aisle), Aldenham, Bramford (south aisle), plus St Martin's Leicester. Each has a description page carrying its own span and scantlings, recoverable by the same visual read.

Four further OCR-partial scantling sets are recorded in the TSV as `OCR-PARTIAL` with attribution explicitly unconfirmed — notably a Cirencester set whose values coincide exactly with Leicester's, which is suspicious enough that it should not be trusted until read.

## 10. Cycle 6 — eight roofs read, and a rule falls out of the data

Method note worth keeping: instead of reading 29 full pages, I cropped each description page to the **band from 55% to 97% of page height** with `sips` and read only that. The span and scantling table always live there, and each crop costs roughly a quarter of a full-page read. `sips -c <h> <w> --cropOffset <top> <left>` is the whole trick, and it works with no imaging library installed.

Two filters that did **not** work, recorded so they aren't retried: band file size does not identify which pages carry tables (a sparse table has less ink than a paragraph of prose), and the earlier ink-density approach is equally useless here. Description pages simply have to be opened.

Eight roofs now read at full resolution → [reference_manifests/timber_roof_scantlings_v2.tsv](reference_manifests/timber_roof_scantlings_v2.tsv) (v1 deleted; see below).

| Roof | Span | Truss spacing | Type |
|---|---|---|---|
| St Martin's, Leicester (chancel) | 23′0″ | 12′6″ | tie-beam, no principals |
| Holy Trinity, Cirencester (chapel) | 18′0″ | 11′0″ | tie-beam |
| Brinton, Norfolk (nave) | 17′0″ | 12′0″ | principals, **ridge angle 100°** |
| Fressingfield, Suffolk (nave) | 19′8″ | 7′10″ | hammer-beam |
| Capel St Mary, Suffolk (nave) | 18′3″ | 6′0″ | hammer-beam |
| Palgrave, Suffolk (nave) | 20′4″ | 5′9″ | hammer-beam |
| Trunch, Norfolk (nave) | 19′0″ | 5′6″ | hammer-beam |
| St Andrew's (nave) | ~21′0″ | — | no scantling table |

### The rule the table exposes

Sorting by truss spacing separates the roof types cleanly, and it is the useful generator constraint:

- **Tie-beam roofs: trusses 11′0″–12′6″ apart.** A few heavy trusses, widely spaced, with common rafters spanning between.
- **Hammer-beam roofs: trusses 5′6″–7′10″ apart** — roughly *half* the spacing. Many lighter trusses, closely ranked.

So truss spacing is not a free parameter: it is **determined by roof type**, and the two families sit in non-overlapping bands. A generator that picks "hammer-beam" and then spaces trusses at 12 ft produces something that never existed. Brinton is the informative edge case — it has principals but no hammer-beams, and its spacing (12′0″) sits with the tie-beam family, which suggests the real driver is *whether each truss carries the roof or shares the load*.

Two supporting patterns:

- **Spans cluster tightly at 17′0″–23′0″** across eight buildings in four counties. English parish naves are ~17–21 ft wide, and that is a hard scale datum for building layout — the nave width is nearly a constant of the type.
- **Section sizes are reused across members within a roof.** At Capel St Mary the principal rafters, hammer-beam and collar-beam are all 10 × 8; at Palgrave the principals and hammer-beam are both 1′1″ × 10″. One stock size serves several members — a fabrication economy, and a reason a generator should draw member sections from a small per-building set rather than sizing each independently.
- **Common rafters are almost invariant**: 6–7 in deep × 3–4 in thick in every roof measured. Ridge angle where stated is 100° included, i.e. a **40° pitch**.

### A correction carried out

Cycle 5's four `OCR-PARTIAL` rows are **deleted**, and the suspicion recorded there is confirmed: the "Cirencester" scantlings (cornice 10 × 8, purlin 10 × 10, ridge 10 × 9) were **Leicester's values showing through** — Cirencester's real figures are a 2′8″ × 11″ tie-beam and a 1′9″ × 11″ cornice, nothing like them. Flagging that row as untrustworthy rather than recording it was the right call. `timber_roof_scantlings_v1.tsv` is removed; v2 supersedes it.

Also confirmed in print: **plate N → leaf 2N + 61**. St Andrew's description cites "St Stephen's Church, Plates 32 and 33", which the formula puts at leaves 125 and 127 — both in the plate list from cycle 2.

Progress on this queue item: **8 of ~15** description pages read. Remaining leaves to open: 50, 140, 158, 166, 169, 170, 174, 178, 182, 186 (plus 54–76 for the introductory roofs).

## 11. Cycle 7 — sweep part 2: the spacing rule survives, but not in the form I gave it

Five more description pages read (leaves 50, 140, 158, 170, 186) → **13 roofs** now in [reference_manifests/timber_roof_scantlings_v2.tsv](reference_manifests/timber_roof_scantlings_v2.tsv).

Sorted by spacing, the full set now reads:

| Spacing | Roof type | Building | Span |
|---|---|---|---|
| **1′2″** | common-rafter, *no trusses* | Heckington, south porch | 10′9″ |
| 5′6″ | hammer-beam | Trunch | 19′0″ |
| 6′0″ | hammer-beam | Capel St Mary | 18′3″ |
| 6′4½″ | hammer-beam | Bacton | 19′4″ |
| 5′9″ | hammer-beam | Palgrave | 20′4″ |
| 7′10″ | hammer-beam | Fressingfield | 19′8″ |
| 11′0″ | tie-beam | Cirencester chapel | 18′0″ |
| 12′0″ | principals | Brinton | 17′0″ |
| 12′6″ | tie-beam, no principals | St Martin's Leicester | 23′0″ |
| **14′0″** | curved-brace truss | north aisle (leaf 186) | **11′0½″** |

### The correction

Cycle 6's rule — hammer-beam roofs at 5′6″–7′10″, tie-beam at 11′–12′6″ — **holds for every roof added** (Bacton at 6′4½″ landed inside the hammer-beam band). But two new cases show the rule I *stated* was too narrow:

- **A third family exists below both**: Heckington's porch has **no trusses at all**, just rafters at **1 ft 2 in centres**. That is an order of magnitude tighter than the hammer-beam band and needs its own branch in any generator.
- **The aisle roof breaks the span correlation outright.** It has the **narrowest span in the set (11′0½″) and the widest spacing (14 ft)**. So spacing is emphatically *not* a function of span — I never claimed it was, but it would have been the natural next inference, and this case forecloses it.

What actually governs spacing is **how much work each truss does**, and the aisle roof pays for its 14-foot bay in two places: stout principals (11 × 10½ in, the largest principals in the set) and **the deepest common rafters in the set at 8 × 4 in**. Across the whole dataset common rafter depth tracks spacing at the extremes — 5½ in at 1′2″ centres, 6 in through the hammer-beam band, 6½ in at 12′6″, 8 in at 14′0″ — with scatter in the middle (Brinton is 6 × 4 at 12′0″). So the honest generator rule is: **pick roof type → spacing follows from type → common rafter depth follows from spacing**, and treat the middle of the range as loose.

### Two non-dimensional findings worth as much as the numbers

**Polychromy (leaf 170).** Coloured roofs used **"a spiral band of black on a white ground"**, which the authors call **invariable in all coloured work**, and Aldenham Church's nave roof is "richly coloured" throughout. This matters for the structural-oak kit: a church roof was not bare oak. Spiral-banded members on a white ground is a specific, citable scheme. New lead recorded: Blackburn's work on *Decorative Painting* carries a coloured illustration of that roof.

**The alignment member (leaf 50).** At Heckington a plate projects from the wall face into the porch, moulded to form a cornice, and **both struts and wall-beams tenon into it** — described as "of considerable importance in keeping all the trusses in their proper positions." So the cornice is not decoration hung on the finished frame; it is the **longitudinal member that positions the trusses during assembly**. That is an assembly-order fact a generator should respect: cornice before rafters, not after. The porch roof is also boarded longitudinally and covered with lead.

Two further cross-references found: Bacton cites Brandon's *Parish Churches*, and the aisle roof cites their own *Analysis of Gothick Architecture* Section I Decorated plate 33 — a book already on our shelf, so the two sources interlock.

Progress: **13 of ~15** roof descriptions read. Remaining leaves: 178, 182, plus the introductory run 54–76 and the odd ones at 114, 122, 134, 137, 152, 166, 169, 174.

## 12. Cycle 8 — Brandon closed at 15 roofs, and one of my own claims retracted

Leaves 178 and 182 read. Both are **aisle** roofs, which with leaf 186 gives three, and that is enough to separate the span classes properly. **15 roofs** now in the dataset.

### Span is strongly classed by what the roof covers

| Class | Spans measured | n |
|---|---|---|
| Porch | 10′9″ | 1 |
| **Aisle** | **11′0½″, 11′5″, 11′10½″** | 3 |
| Nave | 17′0″, 18′0″, 18′3″, 19′0″, 19′4″, 19′8″, 20′4″, 21′10″, 23′0″ | 9 |
| Chancel | 23′0″; another "nearly twenty feet" | 2 |

The aisle cluster is the striking one — **three aisles in three counties within 10 inches of each other**, all ~11–12 ft. Combined with naves at 17–23 ft, that gives a generator two hard width bands rather than a free parameter, and the ratio aisle:nave ≈ 0.55–0.6 is itself a plan rule.

### Retraction

Cycle 7 proposed that **common rafter depth tracks truss spacing**. **That does not survive the new data and I am withdrawing it.** The three aisles have nearly identical spans but rafters of 7 × 3, 6 × 4½ and 8 × 4 at spacings of 13′9″, 9′7″ and 14′0″ — no ordering. Fressingfield has the same 7 × 3 rafter at 7′10″ spacing that Bramford has at 13′9″. The extremes I cited in cycle 7 were real but the middle is noise, and three more points show it is noise all the way through. Common rafters are simply **near-invariant at 5½–8 in deep × 3–4½ in thick** across every roof and every span in the book; that invariance is the finding, not a correlation.

The **spacing-by-type** rule from cycle 6 does still hold, with the aisle family added as its own band:

| Roof type | Truss/rafter spacing | n |
|---|---|---|
| Common-rafter, no trusses | 1′2″ | 1 |
| Hammer-beam | 5′6″ – 7′10″ | 5 |
| Truss over aisle | 9′7″ – 14′0″ | 3 |
| Tie-beam / principals over nave | 11′0″ – 12′6″ | 3 |

Note the aisle band overlaps the nave band and extends past it, so "aisle" is not simply "wider spacing" — but every aisle sits at or above 9′7″, well clear of the hammer-beam band.

One more consistency worth having: **aisle principals are as stout as nave principals** (11 × 10½, 11½ × 10, 12 × 10½) despite covering half the span, because they carry wide bays. So member section follows **bay width, not span** — which is the defensible version of what cycle 7 was reaching for.

### Generator spec (from 15 measured roofs)

```
1. Pick what the roof covers      -> span band
     porch  ~10-11 ft | aisle 11-12 ft | nave 17-23 ft | chancel 20-23 ft
2. Pick roof type                 -> spacing band (NOT derived from span)
     common-rafter (no truss) 1'2" | hammer-beam 5'6"-7'10"
     tie-beam/principals 11'-12'6" | aisle truss 9'7"-14'0"
3. Members
     common rafters   5.5-8 deep x 3-4.5 thick   (near-invariant; do not scale with span)
     principals       9.5-12 x 8-10.5            (follows BAY width, not span)
     purlins          6.5-11.5 x 5-10
     tie-beam         1'8"x1'2" to 2'8"x11"      (tie-beam roofs only)
     hammer-beam      10x8 to 1'1"x10"           (hammer-beam roofs only)
     cornice          10x8 to 1'9"x11"
     -> REUSE one section across 2-3 members per roof (observed at Capel St Mary, Palgrave)
4. Pitch: 40 deg (only explicit datum: Brinton, 100 deg included at ridge)
5. Assembly order: wall plate -> cornice (positions the trusses) -> trusses -> common rafters
     -> longitudinal boarding -> lead
6. Finish: not bare oak in a church. Coloured work uses a spiral black band on white ground.
```

Brandon is now **closed as a data source** at 15 roofs. The remaining unread leaves (the introductory run 54–76 and the scattered 114/122/134/137/152/166/169/174) are the book's general essay and secondary examples; they may add roofs but will not change the shape of the rules above, so the marginal return is low. Recording that as a decision rather than exhaustion, per lane practice.

## 13. Cycle 9 — the wall table found, and the leaf/page formula solved

### The scan's page mapping, solved

Cycle 3 guessed a "+200 offset" from a single anchor. With four anchors it resolves exactly:

```
leaf = 2 × printed_page + 5
```

Verified on p.158→leaf 321, p.195→leaf 395, p.216→leaf 437, p.217→leaf 439. The **factor of two is the mirror-ghost**: every printed page contributes two leaves, a readable recto and its reversed show-through. This turns navigation from guesswork into arithmetic, and it explains why cycle 3's hunt for "p.163 at leaf 363" failed — leaf 363 is printed page 179.

### Printed p.158 — not the kern note, but a vault-coursing procedure (VERIFIED)

The cross-reference led to a page about **how vault courses and beds are set out**, which is more useful to us than the kern note would have been:

- **Joint plane rule:** the plane of the joints is perpendicular to the vertical plane of the diagonal groin and passes through its middle, so **in the diagonal section the joint plane appears as a radial straight line**. Courses may be straight or "swelled".
- **Ridge-line rule:** the crowns of the side arches (K, L) may sit at the same or different height as the middle crown (C). If all three are level and the courses unswelled, the ridge lines are straight; otherwise convex. Where the middle is higher, **draw a circular arc through K, C, L as the ridge line**. Strongly swelled courses get a more convex ridge so load intersecting at the groin transfers safely.
- **A stated clamp:** "in any case the curvature of the ridge must not be carried too far, since otherwise an angle projecting downward might be formed in the compartment." That is a parameter limit with a stated failure mode.
- **Bed layout:** divide the **diagonal arch** into courses (e.g. brick courses), draw radii from those division points to the centre, and the prolongations of those radii give the bed elevations.

And the sentence that matters most for our doctrine: the execution **"is not after such a drawing but is done according to the eye of a skilled master, and consequently there are always shown slight variations and irregularities"** — while still following the principle. That is period authority for exactly the approach the factory takes: build to the analytic principle, then apply bounded deviation, because bounded deviation is how the real thing was made.

### Printed p.212 — Table II, "Thickness of straight abutment wall" (VERIFIED)

The wall counterpart to Table IV's buttress depth — the other half of what the preface promised. Data appended to [reference_manifests/buttress_proportions_v1.tsv](reference_manifests/buttress_proportions_v1.tsv).

- **A design assumption is baked into every value: "Length half occupied by openings."** The wall is sized assuming **50% of its length is window**. That is a strong and very usable fact — Gothic abutment walls are calculated as half-absent.
- Height is again measured **to the vault keystone**.
- **Five rise classes appear here**, where cycle 4 saw only three: labelled 1/8, 1/3, 1/2, 2/3, 5/6, with u = 0.60, 1.25, 1.60, 2.20, 2.80 m for the 4 × 4 m vault.
- Wall thickness for that vault ranges **0.35 m to 4.55 m**.

**The coupling, now much stronger than cycle 4 could show:** at matched height and column, wall thickness falls from about **1.00 m at rise 1/8 to about 0.35 m at rise 2/3 — a factor near three.** Cycle 4 saw a mild version of this in the buttress table (1.15 → 0.90 across three classes); with the flat 1/8 class included, the effect is dramatic. **This is the structural reason Gothic went steep: raising the vault buys back wall mass.** For a generator it is the single most valuable coupling found in the book — vault pitch and wall thickness are one decision, not two.

### Two things left honestly unresolved

- **The row letters a–f.** Rise classes I–III print only rows a, d, f while class IV prints a–f, so a–f are **sub-cases within a rise class**, not vault types as cycle 4 speculated. Most likely load variants. The legend was not found on p.212–215.
- **What "rise 1/8" actually denotes.** With five classes, u/span = 0.15, 0.31, 0.40, 0.55, 0.70 against labels of 0.125, 0.333, 0.5, 0.667, 0.833 — no single constant factor relates them. Cycle 4's "use 0.40/0.55/0.70" stands as *data* for those three classes, but I withdraw the implication that the labels are simply nominal-with-a-factor. **Use the tabulated (label, u) pairs; do not compute u from the label.** Both are marked `UNRESOLVED` in the TSV rather than guessed.

## 14. Cycle 10 — the legend found; row letters resolved

The leaf formula made this cheap. Scanning **odd leaves only** (`p = (leaf−5)/2`) for pages carrying `=` signs plus the words *table*, *wind* and *kern* pointed straight at **leaf 425 = printed p.210**, which opens: *"Explanations of Tables II, III and IV for calculating the depths of abutments."* Its continuation, **leaf 427 = printed p.211**, carries the legend itself.

### Rows a–f: RESOLVED — they are the vault web

Read directly off p.211, fully legible:

| | Kind of vault |
|---|---|
| **a** | ½ porous brick vault |
| **b** | ½ hard or ¾ porous brick |
| **c** | ¾ hard or 1 porous brick |
| **d** | 1 brick or 20 cm sandstone |
| **e** | 30 cm rubble vault |
| **f** | brick vault with filling and floor above |

So a→f is a **ladder of increasing vault dead weight** — from a half-brick porous web up to a brick vault carrying filling and a floor above it. That explains two things cycle 9 could only note: why the tabulated depth grows steadily down the rows, and why rise classes I–III print only **a, d, f** (three representative weights) while class IV prints all six.

Cycle 9 guessed "most likely load variants." That was close but imprecise — they are **construction** variants, and the load follows from the construction. Corrected in the TSV.

### The column letters: partly resolved, and flagged as such

`B″ = brick. Resultant at edge of kern.` is definitively readable. So the columns pair a **material** with a **stability criterion** (resultant at edge of *section* versus at edge of *kern*). The companion lines assigning cut stone and sandstone to A′ and A″ are legible **only in mirror show-through**, so I am recording the scheme but **not** the individual A′/A″/B′ assignments. Marked `partly-verified`.

### What the legend page adds beyond the legend

- **The table series is now fully mapped:** Table II = continuous walls, **Table III = vertical (un-diminished) buttresses**, Table IV = buttresses diminished upwards. Cycle 9 inferred III by elimination; it is now stated.
- **Table I (printed p.135) is the primary data** — weights and thrusts — and II–IV are *computed* from it. Everything mined so far is downstream of a table not yet opened.
- **The four axes, in the author's words:** depths vary "according to **rise, kind of vault, span and height of the resistance to the vault**."
- **An author's caveat worth honouring:** the tables "afford a **preliminary value only** for the designer" and do not replace case-by-case work with the line of support. So these are good defaults, not truth — which is exactly the right status for generator parameters.
- **Wind is excluded.** "They give only the depths required by the vaults… the perhaps existing wind pressure against high roofs may make advisable a relatively small addition to the depth of the abutment." A tall-roofed building earns a modest abutment bonus over the tabulated figure.
- **Method:** the moment equation reduces to a **cubic**, solved by Cardan's formula or by iterating a guess — with an offered shortcut of assuming the resultant crosses at x/2 from the middle, or about 0.30 m from the outer edge.

Two further items read from **mirror show-through only** and marked `ghost-read`, so they are usable as leads but not as data: a rule that **pier widths in Tables III and IV reduce by 10–20%** when windows do *not* run buttress-to-buttress (this complements Table II's "half occupied by openings" assumption), and a six-step **maximum-compression ladder** of roughly 0–4 / 4–7 / 7–11 / 11–14 / 14–21 / 21–70 kg per cm², which appears to be what the small figures prefixed to cells in Tables IV and V encode — resolving the "prefixed figures" reference cycle 4 could not explain.

## 15. Cycle 11 — Table I not found, but the reason is itself the finding

### Correction: the leaf formula is only locally valid

Cycle 9 stated `leaf = 2 × printed + 5` as if global. **It is not, and I am qualifying it.** The predicted leaf for p.135 (275) turned out to be prose, and the reason is that **the readable/ghost parity flips repeatedly through the volume**:

| Leaf range | Readable side |
|---|---|
| 160–239 | odd |
| 240–359 | **even** |
| 360–479 | odd |
| 480–519 | **even** |

Roughly every ~120 leaves, which is about 60 printed pages — consistent with **binding gatherings**, each scanned starting from a different face. Every page verified in cycles 9 and 10 (p.158, 194, 195, 213, 215, 216, 217) happened to sit in odd-readable stretches, which is why the formula looked global. **The rule now is: test parity locally, then jump.** Recorded as a `CORRECTION` row in the TSV.

A related observation worth keeping: leaf 274 shows **readable text overlaid with its own mirror in the same band** — so show-through is not confined to ghost leaves; a readable page can carry the facing page's reversed ink across its own lines. That is the mechanism behind every OCR contamination seen since cycle 3.

### Printed p.194 — the failure criteria (VERIFIED)

Section 3, *"Obtaining the Line of Support and Stresses in the Buttresses — security against sliding, overturning and crushing."* Table I is referenced here in a way that confirms its content: *"When by calculation, construction or by Table I the thrust of a vault has been found, **or which is the same, both its components H and V**"* — so Table I tabulates horizontal thrust and vertical weight per half-vault.

**Sliding**, with numbers:
- with **soft mortar**, sliding may occur if the angle between the direction of the pressure and the bed is **less than 45°–60°**
- with **hardened mortar**, if that angle is under **30°–45°**
- the remedy is to **re-orient the bed** toward perpendicular to the thrust — "or less effectively by dowels"
- a named hazard: never form the damp-proof bed of soft pitchy material, since "this has already permitted the sliding of **an entire mass of masonry**." Isolating beds only where compression acts nearly perpendicular to the bed.

**Overturning**: moments about the outer dangerous angle, with the inequality **G₁a₁ + G₂a₂ + W₂n > W₁m**, and it "can most easily occur at the bottom of the foundation" — which sits directly on top of the pivot list already recorded from p.195.

### The loop that closes

This is the most satisfying result of the cycle. Printed p.158 (cycle 9) requires that **the joint plane be perpendicular to the diagonal groin's vertical plane** — radial in section. Printed p.194 now shows **why**: sliding begins when the pressure-to-bed angle falls below 30–60°, so a radial joint is the geometry that keeps that angle steep. **Vault coursing is a consequence of the sliding limit, not an aesthetic convention.**

That matters for us beyond trivia. It means a generator that lays courses radially is not copying a look, it is satisfying a constraint — and a generator that lays them otherwise is producing masonry that would have slid. Directed emergence of exactly the kind the doctrine asks for: the rule lives in the reader (here, the mortar), not in the ornament.

### Table I: still unfound, honestly

Six separate cross-references all read "Table I (p. 135)". Volume 2 contains essentially no Table I references, so it is in volume 1. The formula-predicted leaf was invalid because that region is even-readable. **Next step is mechanical:** read a page number off an even leaf somewhere in leaves 240–359 to re-anchor the mapping for that gathering, then jump to p.135. Marked `UNRESOLVED` rather than guessed.

## 16. Cycle 12 — Table I: a firm negative result, and Ungewitter closed

### The recipe inversion that settled it

The naive way to find a numeric table is to look for digit-rich OCR. **That is backwards here.** Calibrating against the tables already read:

| Leaf | What it is | digits in OCR |
|---|---|---|
| 431, 433, 435, 437, 439 | Tables II–V themselves | **0** |
| 438, 440, 442, 444 | their ghost neighbours | 158–466 |

**The mirror show-through of a numeric table OCRs its figures far better than the table itself.** So tables are found by scanning for **digit-rich ghosts**, then reading leaf ± 1. Recorded as a reusable rule.

Applying it volume-wide gives a complete table census for vol 1: clusters exist **only** at leaf ~395 (the materials strength table, p.195) and leaves ~431–449 (Tables II–V plus the edge-pressure table). **There is no numeric table anywhere in leaves 240–360.**

### Table I is not in this translation — closed

Three independent lines of evidence:

1. Leaves 240–360, where printed p.135 must fall under *either* parity, are uniformly dense prose (every single even leaf >1500 chars).
2. The volume-wide digit census finds no table cluster in that range.
3. Volume 2 contains essentially no Table I references, so it is not there either.

Yet six separate cross-references all read "Table I (p. 135)". The most probable explanation is that **those citations retain the German original's pagination**, which this 1920 student typescript translation never renumbered — consistent with a translation that also left its figures inconveniently separated and its cross-references untouched.

This costs us little, because **Table I's content is already known** from p.194: it tabulates the vault thrust as its two components, horizontal thrust **H** and vertical weight **V**, per half-vault. What is lost is the numbers themselves, and the derived Tables II–V — which we have read — already encode their consequences.

Recorded as a `NEGATIVE-RESULT`, in the lane's tradition of writing down what is *not* there (as with "Cleveland holds no coopered cask"). Not a gap to keep hunting.

Two smaller navigation facts recorded alongside: **some leaves carry no page number at all** in the top margin (274, 300), so page-number anchoring is not always available; and both of those leaves show readable text **overlaid with its own mirror in the same band**, confirming the mechanism from cycle 11.

### One last substantive find (leaf 300)

The passage *"Vaults with only compressile stresses"* states the governing material fact plainly: with the scarcely elastic properties of all stone and mortar, it is **"always risky to count on their unbroken resistance"** — masonry cannot be relied on in tension. It adds that "not without reason the practical Renaissance supported the heavy vaults by very powerful abutments," and warns against too trustful use of wide flat ceilings.

That is the one-line justification for everything the buttress tables do: because the material takes no tension, stability has to come from geometry and mass. For a generator it is the reason abutment scales with vault, rather than being a style choice.

**Ungewitter is now closed** as a data source. Read and recorded across cycles 3, 4, 9, 10, 11 and 12: the masonry strength ladder, Tables II–V with their taper and kern rules, the full legend, the vault-coursing procedure, the sliding and overturning criteria, and the causal link between them. Table I is absent; the remaining unread mass is figures and worked examples that would refine numbers we already have in usable form.

## 17. Cycle 13 — Starkie Gardner, *Ironwork* Part I (1893): the medieval iron source, opened

New book, and it needed a recipe extension before anything could be read.

### Recipe: a fallback when the hOCR derivatives are missing

This item (`cu31924004684902`, Cornell scan, 172 leaves) **has no `_hocr_pageindex` or `_hocr_searchtext`** — the download returns an HTML error page, so `gzip.open` fails with "Not a gzipped file". Its `_djvu.txt` also has **zero form feeds**, so that route is useless too (the same trap as cycle 1).

The reliable fallback is **`_djvu.xml`**, which always carries one `<OBJECT>` element per page with `<WORD>` children — a guaranteed page splitter. Parsed 172 pages, exactly matching `imagecount`. This is now built into [reference_manifests/iabook.py](reference_manifests/iabook.py) as an automatic fallback, so the tool degrades gracefully instead of returning zero pages.

### The book's own periodisation — a dating grammar for ironwork

| Chapter | Period |
|---|---|
| III | **The Age of the Blacksmith** — ninth to fourteenth century (p.38) |
| IV | **The Transition**, due to Oriental influence in the fourteenth century (p.93) |
| V | **The Age of the Locksmith** — fifteenth and sixteenth centuries (p.115) |

That is structurally the same gift Paley gave for mouldings: a **period enum whose members are distinguishable by feature**, so one generator can produce legibly different eras. Here the axis is *what trade dominated* — the smith's drawn-and-scrolled work giving way to the locksmith's cut, filed and cased work.

Navigation is simple in this book — a normally printed volume with no ghost leaves: **leaf = printed page + 13** (verified at p.49→62, p.71→84, p.129→142).

### 57 figures, provenanced — and three more books

The List of Illustrations gives all 57 figures with page numbers, and the families are exactly what the forged-iron kit needs:

- **Hinges** (the largest run, ~18 figures): Stillingfleet, Hormead, Willingale Spain, Eastwood, Haddiscoe, St Albans, Vanga, Faabergs, Pontigny, Montreal, Durham, Sempringham, Market Deeping, Rouen, Notre Dame Paris, Liège
- **Grilles**: Winchester, Ourscamp, Lincoln, **the Eleanor grille at Westminster Abbey**, Siena, Bourges
- **Locks**: Windsor, Klagenfurt, Styria, Augsburg, Amerling — the Age-of-the-Locksmith material
- **Door furniture**: knocker (Stockbury), handles (Stogumber, Westcott Barton, Rouen, Evreux, Styria), door-linings (Cracow, Bruck, Prague, Krems), tabernacle doors
- **Roman/Gallic antecedents**: window-frames and guards, hasps, escutcheons, clamps, andirons, a candelabrum, a folding chair

The credits line is a bibliography in disguise — **three further sources to shelve**: Liger's *La Ferronnerie* (figs 1–16), **Raymond Bordeaux's *La Serrurerie du Moyen Âge*** (figs 25, 26, 33, 39–45), and Du Chaillu's *Viking Age* (figs 23–24), plus Vienna Government Printing Office publications (figs 48–57). The rest were engraved for this work by J. D. Cooper. Same discovery channel as Paley's advertisement leaves, in a different guise.

### Two figures read at full resolution (VERIFIED)

**Fig. 17, Stillingfleet Church (p.49)** — a full-page **halftone photograph**, not a line engraving, of a Norman doorway with its original ironwork in situ. Chevron arch orders over a **vertically planked door** carrying: two great C-scroll straps springing from the hinge side and curling into coiled spiral terminals with leaf/serpent-head finials; a horizontal strap band at mid height; a **braided rope-motif band** across the lower third; a ring handle with key and escutcheon at left.

The insight that matters: **the straps span the plank joints.** Holding the boards together is their structural job, and the decorative curl is that same member elaborated — not applied ornament. This is the third independent instance of the lane's recurring pattern (cornice-as-alignment-member in Brandon, enamel confined to the pricket's quiet zones in the lighting round). Being a photograph, it also carries real rust-on-weathered-oak tone, which a line engraving cannot give.

**Fig. 36, the Eleanor grille or *herse*, Westminster Abbey (p.85)** — full-page wood engraving, rotated 90° in the scan. It decomposes cleanly into a generator:

1. plain flat-bar rectangular frame
2. vertical division bars at regular intervals
3. panel fill = **one C-scroll unit repeated** and collared to the stems
4. each scroll terminating in a small **stamped rosette**
5. top rail carrying evenly spaced **trifurcated spike finials** (*herse* = harrow, hence the name)
6. standing on a tomb chest with its own gabled cinquefoil arcade and heraldic shields

**The density is instancing, not sculpting** — the visual richness comes from repeating a single forged element, which makes this an ideal Geometry Nodes target rather than a hand-modelled piece.

## 18. Cycle 14 — the strap-hinge typology, and the author states it himself

Swept all 17 hinge figures as contact sheets, then read the key donor drawing at full resolution. Typology: [reference_manifests/ironwork_hinge_typology_v1.tsv](reference_manifests/ironwork_hinge_typology_v1.tsv) · 12 new plate rows in the plate index.

Method note: **the browser pane times out reliably on `scroll`** (same failure as cycle 4). Navigation works, so **paginate by generating separate HTML sheets and navigating between them** rather than scrolling one long page.

### The base type, in Gardner's own words (p.47)

> "One of the most resisting, and therefore prevalent forms, appears to have been a **triple strap, the centre straight, and the lateral curved like the horns of a crescent**."

And the reason is **security, not style**: all three straps spring from *behind* the stonework when the door is closed, which "made it particularly difficult to wrench off" — written against a background of Northern raiders where "the church door might at any moment be thundered at by hordes intent on pillage and slaughter." The triple count was *also* read as symbolic, but the form is defensive first. That is the fourth independent instance of the lane's recurring pattern: **the decoration is the functional member elaborated.**

### The dating rule is precise, and it is not the silhouette

> "The ends of these straps are often beaten into scrolls and foliage, **whose fashion is an indication of age, which the form alone fails to convey**."

So **date the hinge by its terminal, not its outline** — the exact inverse of how one would naïvely parameterise it. Named terminal vocabulary from the text: *rudimentary leaves*, *dragons' heads in high relief*, *profile heads with distended jaws*, and "a usual **Frankish tongue** between two scrolls." Hard anchors: the crescent-with-split-and-scrolled-ends type is dated to **very early 11th century** via a Saxon carving from Selsey (a *carving* used to date ironwork), and two doors are given as **c.1160 and c.1190**.

Gardner also supplies an honest **dating caveat** worth carrying: "it is impossible, without great local knowledge, to predicate the date of any work from its style, where the style is not indigenous" — provincial fashion lags, and he cites Icelandic carving still resembling the Bayeux Tapestry within living memory.

### Sub-styles, and one technique spec

- **Detached-piece style** — small detached crescents and straps, much pierced, arranged in patterns. Finest at **Pontigny**; spread to Montreal, Chablis and Cologne; densest in Aquitaine and Auvergne. On the cathedral doors of Angers the pieces are placed in **random** patterns.
- **Band-and-circle style** — three bands per leaf, each a crescent plus **a circle of broad iron with upturned edges** enclosing filigree, explicitly compared to "contemporary goldsmiths' work" (Montreal, Yonne).
- **13th-century foliage, with a surface spec**: "the lobes of the leaves are **sunk**, and the divisions representing the larger veins and the periphery are **raised**, and the stems usually **grooved**." Typical vine, sometimes a trefoil or cinquefoil variant, always mingled with rosette-like flowers and sometimes grapes. That is directly a normal-map/relief recipe.
- **Production**: stamped work "lavishly used" at St Denis — matching the Eleanor grille's repeated-unit construction from cycle 13. Richness by **repetition of forged elements**, not unique freehand work.
- **A non-date style axis**: the rise of lay as against monastic architecture "led, perhaps, to a more simple and restrained treatment of the hinge." Patronage, not period.

### Fig. 22, St Albans Abbey — the donor drawing (VERIFIED)

Read at full resolution, this one figure gives a complete parametric anatomy:

1. **Pintle assembly** — a vertical bar with spear/leaf finials at *both* ends, gripped by two C-hook knuckles
2. **Strap** — flat bar carrying **incised lozenge/zigzag venation along its whole length**
3. **Collar clamp** where the branches spring
4. **Three pairs of scrolls**, springing symmetrically above and below the strap axis
5. Each terminating in a **tight closed spiral of ~2–2.5 turns**
6. **Serrated fan leaves** at the springing points
7. A **profile dragon head with distended jaws**

Crucially it is **all in one plane** — flat strapwork. So a hinge is a **2D curve network swept with a rectangular profile**, which makes the whole family a natural Geometry Nodes target rather than sculpted geometry. Combined with cycle 13's Eleanor grille decomposition, both major ironwork families now have generator-shaped descriptions.

## 19. Cycle 15 — Gardner chapters IV and V, mined by fan-out; the finish spec is the prize

Run as a five-reader fan-out (locks · door-linings · small fittings · chapter V text · chapter IV text) followed by an adversarial verification pass over the fourteen strongest claims. **Nothing in the verified set was refuted**; four claims were tightened and the tightenings are what got recorded. **All 21 figures in chapters IV–V (figs 37–57) were opened and read at full resolution.** Deep-dive record: [gardner_ironwork_chs4_5_v0_1.md](gardner_ironwork_chs4_5_v0_1.md) · 21 plate rows and 85 typology rows added to the manifests.

### The single most useful finding is a material spec, not a form

> "the iron was brightly tinned and laid over red cloth or paper." — p.137

Bright white metal openwork over a saturated red ground: a two-material shader, applying to the locks, hinges and handles of the German pierced-thistle family. Reinforced for the door-linings at p.140 — "illuminated in black and white, red and blue, and profusely gilded" — and at Bruck, p.142: "The ground of the lozenges was painted alternately red and blue, so that the general effect was like gold lace on a scarlet-and-blue chequer."

Gilding is the default across both chapters (seven separate pages), and a keyword scan of every leaf in chapter V found **no mention of etching and none of cast iron at all** — the only finishing processes named are gilding, tinning and painting. **Shipping this asset class as dark bare metal is wrong on the book's own evidence.** That corroborates and sharpens cycle 2's Shaw finding (painted and parcel-gilt) from an entirely independent direction.

### Construction is a layer stack, and layer count is a date parameter

The era rule, p.115: "Heat was now applied only in the preliminary stages, and the greater part of the work was accomplished by the file and saw, or by embossing the iron." Note the hedge — *the greater part*. The book never says "cold"; that inference was raised by a reader and **withdrawn in verification**.

Build order for the mid tier, p.116: hinges made "of several thicknesses of sheet iron, pierced to represent tracery, and riveted together in strong frames", escalating from one thickness to "three or four sheets of piercing superimposed". So **layer count is simultaneously richness and date**. But the top tier is *not* laminated sheet — p.117: "the crockets, pinnacles, and leading lines of the tracery are chiselled and filed from the solid iron in full relief, and the pierced sheet-work plays but a very subordinate part."

The assembly model is stated outright, p.128: the parts "are chased out of the solid, and **tenoned, morticed, and riveted together as in joinery**." Iron built like woodwork.

### Sheathed doors: four lattice systems, one generator

All four read at resolution. Proportions are scan-pixel ratios, not millimetres.

| Fig | Door | Strap angle | Nails at crossings | Cell filler |
|---|---|---|---|---|
| 53 | Cracow Rathhaus | 45° exactly | **bare** | pierced plate, four-fold cruciform |
| 54 | Bruck on the Mur | 50.9° | rosette | pierced plate, nearly all different |
| 55 | Karlstein | 46° | whirl | **painted on the wood — no iron** |
| 56 | Krems | 54.3° | large fluted boss | embossed iron figure plate |

Two parts, in the book's words (p.139): "lining entire doors with pierced and embossed plates and straps of iron" — so lattice member and cell filler are **separate fabricated parts**, and should be emitted as separate meshes with separate materials. **Nail placement is diagnostic**: Cracow leaves crossings bare and puts its nails at plate centres and strap midpoints, while Bruck and Karlstein nail every crossing. Krems covers a whole door with four figure plates and one boss; Bruck repeats almost nothing ("few of the designs being repeated", p.140) and so needs a grammar rather than a library.

### Iron copies stone — one ornament library serves both

The small-fittings cluster answered its question affirmatively and specifically. Fig 47, the Ottoburg tabernacle grille, is a free-standing iron cupboard built exactly like a stone tabernacle-house: a **canted three-faced (half-hexagon) plan**, three *wimperg* gables with crocketed rakes and apex finials, tympanum tracery, pinnacles at roughly 0.55 of body height, and a 45° trellis. Fig 57's Krems tabernacle door has a coffin-headed outline over an orthogonal bar armature of three columns by six registers with appliqué in three Z-layers. **Gables, crockets, finials, pinnacles, tracery and buttresses all appear in iron at small scale**, so the stone ornament library and the ironwork library can share assets.

### The Victorian "Oriental influence" thesis — assessed, not repeated

Chapter IV's title asserts causation, so it cannot be quietly cited around. Gardner claims the Western smith learned file, saw, graving, inlaying, damascening and embossing from the East (p.94), and that the earliest grilles are iron copies of Saracenic pierced marble (p.95). His evidence is **resemblance judged by eye** ("It is unquestionably an Eastern design", p.105), a trade-route narrative with no object or craftsman in transmission, one genuinely evidential item (a Rendcombe plate with Arabic numerals, and even that hedged as "supposed"), and one piece of hearsay.

The strongest reason to distrust it is Gardner himself. At p.89, the one place he reasons about a mechanism, he explains an "entirely novel and rather Oriental effect" by a mundane local cause: "It is just the sort of rendering we might get from a smith, set to work from a drawing without sections, and unacquainted with the process of stamping." A misread drawing, not a trade route — and he does not notice it undercuts his chapter title. This is 1893 diffusionism; it is a finding about Gardner, not about medieval ironwork. **Keep the forms as design-family labels, drop the causation, and do not name assets "Saracenic" on this book's authority.**

### Honest limits

Quotes from pages not re-read against the image (pp.94, 109, 111, 120–126, 135, 138–145) are OCR-derived and marked as such. Three flagged uncertainties are recorded rather than resolved: fig 48's curled centre-axis element cannot be identified even at 4× upscale (keyhole cover, keyhole in a leaf mount, or pendant), fig 50's fine vertical striation could be file-cut ground, the weave of the red backing, or a halftone artefact, and fig 45's corner monograms are low-confidence. **No figure in either chapter shows a bolt, ward, tumbler or spring** — the book says why (p.118: "their mechanism is careful, concealing bolts and key-holes with great skill"), so internal lock mechanism must come from another source.

One process correction worth carrying: the synthesis agent received **truncated copies of three of five reader payloads**, honestly flagged its own gap, and consequently reported five figures as never opened. Recovering the full returns from the run journal showed all five *were* read. Lesson recorded — when fanning out, size the consolidation input to the payloads or pass a file path instead of inlining JSON.

## 20. Cycle 16 — Dollman indexed and mined; a dated period ladder for the whole corpus

Five-reader fan-out (index/chronology · Penshurst · metalwork & joinery · half-timber · great houses) plus an adversarial verify pass. **All eight verification targets came back refuted — and in every case the core claim survived while the specifics were corrected.** That is the pass earning its cost, and the full corrections are in the deep-dive: [dollman_domestic_architecture_v0_1.md](dollman_domestic_architecture_v0_1.md). Data added: **80 plate rows** to the plate index and a new **194-row dimensions manifest**, [domestic_dimensions_v1.tsv](reference_manifests/domestic_dimensions_v1.tsv) — 150 read off plates, 44 from descriptive text, **no pixel estimates admitted**.

**The corpus is 161 plates**: vol 1 = 81 in 28 subjects, vol 2 = 80 in 37 subjects, both totals printed and re-read off the images.

### The chronological list exists, and it dates everything else (VERIFIED)

Not in the front matter where the preface implies — it sits at `dollman_v2` **leaves 26 (English) and 27 (Scottish)**, after the introductory essay. 81 entries in named typological classes. Every date column is headed `DATE.` over an italic *Circa.*, so **these are two architects' stylistic attributions in 1863, not documentary dates** — good as an ordering, which is exactly what a generator needs.

| Style band | Dated exemplars |
|---|---|
| Transition (1180) | Oakham window — an isolated outlier, nothing between it and 1250 |
| Early English (1250–1300) | Lambeth chapel, Apthorpe fireplace, St John's Northampton, Wells chapel |
| Middle Pointed (1320–1380) | Mayfield Palace, Guesten Hall, Battle Abbey gateway, **Penshurst 1335**, Great Malvern timber hall |
| Perpendicular (1430–1500) | Higham Ferrers, Ewelme, St Cross, Sudeley, Colston's, Lyddington |
| Late 16th c (1506–1582) | Bablake, Ford's Hospital, Lavenham, Commandery, Haddon bay, Holcombe Court, Prestbury |
| Post-medieval survival (1600–1637) | Ludlow, Blundell's, Guildford furniture, **Chiddingstone 1637** |

Two structural facts worth more than the dates. **Six entries are ranges, not points** — St John's 1290–1520, Chichester 1300–1680, Cobham 1362–1598, Stirling 1450–1540, ironwork 1450–1700 — and a range is the authors explicitly saying the fabric is multi-period. Don't collapse them to a midpoint; that accretion case is precisely what a building generator should be able to produce. And **the ironwork is dated as a 250-year band**, which says they regarded domestic ironwork profiles as period-insensitive: one ironwork parts library serves the whole medieval range.

**The Scottish ladder is deliberately later and the authors say so** — Scotland has "comparatively few remaining anterior to the 15th century" and its phases "were later in Scotland in point of date than in England." A Scottish preset needs a **50–100 year date offset plus a different form vocabulary**: crow-stepped rather than straight gables, no four-centred arch, barrel vaults with applied ribs faking groining, turnpike stairs. The dormer sub-run (Stirling 1550 → Maybole 1620 → Newark 1620 → Hagg's 1685) is a 135-year evolution drawn on **one comparative sheet**.

**The promised analytical index does not exist.** Volume 1's preface promises "a carefully arranged chronological *and analytical* index"; volume 2's restates it narrowed to the chronological half; the word `analytic` occurs nowhere else in either volume. Half delivered, half never printed — recorded as a negative result.

### Two navigation findings that beat everything used so far

**Volume 1's plates carry a continuous volume plate number engraved in the outer top corner**, independent of the per-subject `Pl. n` — and it is **predictable in advance** by running the cumulative plate counts forward from leaf 13 (Chiddingstone 1–2, Oakham 3, … Lambeth 74–77, Stirling 78–81). Every checked prediction matched, and leaf 374 reading `81.` confirms the printed total independently of the list. Predict the number, crop the corner, confirm — better than any OCR method.

Which matters because **the OCR-based method this lane has leaned on fails in this book**. "Near-empty but non-zero OCR = plate leaf" is wrong here: the non-zero OCR is *letterpress bleeding forward* from the preceding text leaf. Use **400-px thumbnail file size** instead — vol 1 blanks 14–17 kB, plates 65–105 kB, text spreads 130–155 kB — then confirm with the corner number. Two further traps: the per-leaf OCR array is **offset by about +4 leaves inside the plate runs**, and the chronological list's two columns **OCR as separate blocks** (all names, then all dates), so any name-to-date pairing taken from OCR is scrambled and worthless.

### Dimensions: the scale ladder the factory was missing

Penshurst is the type-specimen and now fully measured. The great hall is **64′0″ × 38′8″** on the plan (the section and the text both print 38′9″ — a real discrepancy in the book, not a misreading), **48′0″ floor to ridge** from the text, with the roof in four bays over 64′ = a **16′ bay module** and a height-to-length ratio of exactly **3∶4**. The verify pass caught the important one: the plate's **32′0″ is ridge-to-wall-plate, not floor-to-ridge** — both arrowheads traced, and the section is broken so the floor is not even drawn. Also, Plate 1's plan is drawn with **north at the bottom**, so left is east.

Against the parish-church naves measured from Brandon (17–23 ft wide), the scale gap is now quantified: Linlithgow's whole pile is **175 × 166 ft**, its Parliament Hall **98 × 30 ft**, its chapel **50 × 26 ft**. A baronial hall is only ~30 ft wide but three to five times longer than a nave.

### The plan-generator state machine, stated by the authors

The introductory essay gives a **seven-step room-accretion ladder** — hall → solar → parlour and bedchamber → private dining room → withdrawing room → study and boudoir → tiring rooms — which is directly a plan-generator progression rather than a static plan. With it come the 14th-century baronial plan with its opening counts spelled out (screens passage, two openings one side, three the other, centre to kitchen), and the **solar-block rule**: the cross-wing sits at right angles with its ridge deliberately *below* the hall ridge, the hall gable rising above it pierced by a circular or spherical-triangular vent.

One England-versus-Scotland distinction is a hard modelling fork: **an Edinburgh timber front is non-structural casing over a stone wall, whereas an English frame *is* the wall.** Same appearance, opposite construction.

### Props: keys, ironwork, panelling

The **ten-keys plate** (vol 1 leaf 168, "Full size") is a key typology in one image, with the book supplying a bow-shape chronology — lozenge bows 13th–14th century, then circular, trefoil and quatrefoil — and per-key dating (most Elizabeth/James I, no. 10 no earlier than William III). Provenance: "most of these examples are from the neighbourhood of Uxbridge." **The verify pass established that no key is individually dimensioned** — "Full size" is the only scale note — so the reader's pixel-calibrated lengths are *not* printed dimensions and are excluded from the dimensions table.

Best technique find, and it rhymes with Gardner: the escutcheon is **built of three layers of pierced sheet iron with offset piercings, so flat sheet reads as mouldings**. Independent corroboration of the layer-stack construction from a different book and a different author.

A modelling correction from the panelling plates: **panel dimensions are given to the sunk field inside the frame**, so the framed unit is larger by two stile widths. Getting that backwards would make every panel too small.

### What the verify pass killed

Worth naming, because these are the failure modes: a plate was tagged `VERIFIED` off a **653 × 869 render** that cannot carry the claims made from it (Haddon Pl. 1 — downgraded to `PLATE`); the half-timber headline contrast was **backwards** (Prestbury's interlaced-circle field is confined to the upper storey and gables, the *same* rule as Mayfield, not a contrast to it); and a Lavenham oriel dimension of 9′10″ was a **height reported as a width** and carried into the modelling notes. Eight further figures presented as printed dimensions were misreadings.

## 21. Cycle 17 — Viollet-le-Duc: a second harvester, and the largest source on the shelf indexed

The *Dictionnaire raisonné* needed a different recipe from everything before it, so this cycle built one. New tool: [reference_manifests/vldsource.py](reference_manifests/vldsource.py), sitting alongside `iabook.py` as the lane's second harvester. New manifest: [viollet_article_index_v1.tsv](reference_manifests/viollet_article_index_v1.tsv) — **all 533 articles with figure counts**.

### Why this source is unlike the rest of the shelf

Every other book we have mined is a scan: page images to crop, OCR to fight, plates to locate by rhythm or file size. The *Dictionnaire* is not. fr.wikisource carries **the entire ten-volume work transcribed as clean text**, and every woodcut has been **extracted to Wikimedia Commons as an individually addressable PNG**, public domain, with descriptive filenames.

The scale is the headline: **533 articles, 3,287 figures, and roughly 7 million characters of text** (4.9 M measured across the 361 articles profiled so far). For comparison, the entire museum-object corpus this lane has built over twenty cycles is ~700 pieces, and the plate index across five mined books is 141 rows. **This one source is larger than everything else on the shelf combined.**

The richest articles by figure count:

| Figures | Article | | Figures | Article |
|---|---|---|---|---|
| 92 | Architecture militaire | | 53 | Chapiteau |
| 92 | Porte | | 52 | Construction — Voûtes |
| 82 | Tour | | 50 | Cathédrale |
| 81 | Clocher | | 49 | Maison |
| 80 | Sculpture | | 48 | Base |
| 60 | Architecture religieuse | | 48 | Charpente |
| 56 | Vitrail | | 46 | Voûte |
| 54 | Serrurerie | | 44 | Donjon |

There is also a dedicated **Construction** family — *Construction — Voûtes* (52), *— Développement* (42), *— Principes* (33) — which is the theoretical core, and `Escalier` at 34 figures and ~66,000 characters covers stairs, a subject **no book on the shelf touches at all**.

### The recipe, and its traps

```
1. list=allpages&apprefix=<PREFIX>              -> 533 articles
2. action=parse&page=<title>&prop=text          -> rendered HTML -> plain text
3. prop=images                                  -> per-article figure list
4. commons prop=imageinfo&iiprop=url|size|extmetadata -> file URL, pixel size, licence
```

Four traps, all recorded in the module docstring:

- **The article's own wikitext is ~250 bytes.** Content is transcluded from the `Page:` namespace by ProofreadPage, so `prop=revisions` size is meaningless and `prop=extracts` returns *nothing*. **Only `action=parse` expands the transclusion** — that one fact is the difference between this source being unusable and being the best on the shelf.
- **The title prefix uses a typographic apostrophe** (U+2019) in *l'architecture*. A straight quote silently matches zero pages.
- **Every article's image list includes ProofreadPage status icons** (`100%.svg` and friends), which inflate figure counts if not filtered.
- **Two page layouts exist, and only one cites the original book.** Type A opens with publisher metadata and a `( vol. N , p. A - B )` header (Abaque, Escalier); Type B opens with a bare navigation line and gives no volume or page at all (Charpente, Château, Voûte). Both carry the full text, so this only affects citation — but measured across 251 articles, **only about 6% are Type A**. A missing header is not a harvest failure, and for most articles the original pagination has to come from elsewhere.

One convenience worth exploiting: **the figure filenames encode the drawing type in French** — `Coupe.` = section, `Plan.`, `Elevation.`, `Construction.`, `Detail.` So which figures to open can be chosen from the filename rather than by downloading 3,287 images.

A practical note on my own tooling: long background index builds were **cut short twice** at ~250 and ~361 of 533 articles despite reporting exit 0. Figure counts survived because they are gathered in batched metadata calls; text lengths did not. The manifest is therefore complete on figure counts for all 533 and carries text lengths for 361 — stated in the file rather than papered over.

### What the five-article mine returned, and what verification killed

Four of five articles delivered (**Serrurerie's reader died on an API error and wrote nothing** — §F of the deep-dive, and re-queued). Nine adversarial verifications, **all nine refuted**; in every case the core survived and the specifics were corrected. Full record: [viollet_le_duc_five_articles_v0_1.md](viollet_le_duc_five_articles_v0_1.md). Manifests: 236 plate rows, a **359-term French building lexicon** ([viollet_vocabulary_v1.tsv](reference_manifests/viollet_vocabulary_v1.tsv)), and 142 text-stated dimensions ([viollet_dimensions_v1.tsv](reference_manifests/viollet_dimensions_v1.tsv)).

**Pitch — the gap Brandon left, filled and then corrected.** Viollet does give pitch rules *with a reason*: pitch rose to "60 et même 65 degrés" early in the 13th century **because the *bahut* (wall head over the vaults) had shrunk to about a metre and the rafter foot no longer fitted on it.** So pitch is derivable from wall-head width and covering, not a style constant. But the reader then sold a France-versus-England pitch band, and **verification killed it**: Viollet explicitly groups the two together — *«partout en France et en Angleterre»* — the real exception is the Midi, and Ely measures 63° against French plates at 39–50°. Replaced with **pitch = f(date, wall-head width, scantling, covering)**, which is the better rule anyway. Brandon's lone 40° datum turns out to sit in the *shallow* band.

**A roof system chosen by the building underneath it.** French practice is *charpentes à chevrons portant fermes* — every common rafter framed as its own truss, standing on *blochets* in a double *sablière*, classically **no purlins at all**. Viollet's stated reason is masonry, not timber: thin piers and light vaults cannot take the point loads that principals-and-purlins impose, so load must spread evenly along the whole wall head. What survives verification as the regional discriminators is **TIE_MODE (bearing versus suspended), scantling size, and purlin presence** — and his tie-beam doctrine is a testable invariant: the *entrait* resists spreading only, carries nothing, and hangs from the *poinçon*.

**Viollet contradicts Ungewitter on web coursing, and the contradiction holds.** Ungewitter divides the *diagonal* and lays beds radial to it. Viollet: *«L'arc formeret doit commander d'abord»* — divide the **wall arch** by the rubble width for *n* courses, divide the diagonal into the *same* number of larger parts, and every bed arris lies in a **vertical plane**, not radial. Reason given: the surplus arc development must be spread across all courses, so each course is a lens, thicker at mid-length. Figure 65 draws both webs on *identical* rib skeletons, which yields a schema rule: **rib layout and web coursing are independent fields**, not one derived from the other.

And the construction order is more radical than his reputation: **no centering in the cell at all** — *«on procède de suite à la construction des voûtes sans couchis»*, one mason and a boy, *«sans cintres et sans autres outils que sa hachette et sa cerce»*. Lowest third as plain walling, then a sliding grooved *cerce* off one template arc. Flagged hard, though: **his evidence is his own 19th-century restoration sites.**

**Two claimed disagreements dissolved into agreements** — and the agreements are worth more. Ungewitter *also* says the web setting-out drawing is useless on site and the mason works freehand; Gardner *also* denies rolled stock. Independent corroboration across three authors beats a manufactured conflict.

**The French town house is algorithmically different from the English manor.** Dollman's English ladder accretes differentiated rooms sideways; Viollet's French rule **repeats one room upward** — *«Toujours la salle à chaque étage»* — with occupancy assigned per storey: shop, then a *salle* that is also a bedroom, then chambers, then *galetas* for apprentices. No screens passage, no solar cross-wing, no ridge hierarchy; the service equivalent is the *allée*. Two plan types, not one: deep burgage plots (Cluny 6.5 × 18.4 m, Montpazier cell 8.3 × 20.2 m, corrected in verification) and a broad-shallow Burgundian type with the newel stair planted in the street wall and corbelled over the door — regionally exclusive, because Burgundy's hard stone can carry a thin cage on the tread ends. And **the jetty exists for land economics inside a fixed wall, not for weather** — stated outright.

**Stairs, previously a total gap**, now have a taxonomy, a 60-term lexicon, and an answer to the folklore question. Verification confirmed the taxonomy and the handedness finding while killing four of six numbers on one *vis* figure — so the geometry needs a second pass.

**Reims figure 14 is now the hardest calibration point on the shelf**: pixel-decoded at 58.8 px/m, reproducing every stated number (7.19 / 15.49 / 17.08 / 0.221 m).

Two translation traps worth carrying: ***arc ogive* means the diagonal rib, never "pointed arch"** — a false friend that would silently corrupt any vault work — and in this text **"0,50 c." means 0.50 metres**, proved internally by *huit pouces (0,22 c.)*.

**One systematic reader failure worth naming.** The conjecture flags **under-flagged in one consistent direction**: Viollet's own hedge markers — *affirmerons*, *hypothèse*, *restitution*, *C'est possible* — appear **zero times** in a 190 kB findings file, and three findings resting on those hedged passages were recorded as fact. He is famous for conjectural restoration, so this is the exact failure mode to guard against, and the verifier caught it.

## 23. Cycle 18 — Serrurerie: the lock-mechanism question, answered

The re-run of the article that killed its reader last cycle. It went wrong again — differently — and still delivered the cycle's headline. Manifests: 40 stale `LOCATED ONLY (READER NEVER DELIVERED)` rows **replaced** with 21 real rows; vocabulary manifest now 409 terms.

### The open question is closed (VERIFIED)

`Mecanisme.serrure.XIIe.siecle.png` is exactly what its name promises: **a drawn interior of a 12th-century lock with the cover removed** — the thing Gardner never figures, having said the medieval smith "concealed bolts and key-holes with great skill." Viollet's own text keys every letter on it:

> "La figure 24 présente le mécanisme très-simple de ces serrures. La boîte est circonscrite par les lettres *abcd*. Le pêle intérieur *p* tient au pêle extérieur *P′* par deux forts rivets qui glissent dans une coulisse percée à travers le pallâtre… un tour de clef fait descendre le cramponnet *c*… un tour de clef de *e* en *f* appuie sur le ressort *r*, et dégage le cramponnet."

So the mechanism is: a **case** (a–b–c–d) on a **pallâtre**; an **inner bolt** `p` pinned to an **outer bolt** `P′` by two strong rivets running in a slot cut through the plate; a **cramponnet** `c` which a turn of the key lowers to block the bolt or raises to free it; and a **spring** `r` which the key presses on its way from `e` to `f`. The dashed circle in the figure is the key's swept arc between those two letters. Viollet's own verdict: *"Rien n'est plus simple que ce mécanisme, encore employé aujourd'hui."*

**A correction to my own first reading, worth recording as method.** Before fetching the keyed text I read the figure by form alone and took the concentric circle for a ward assembly and `r` for a tumbler. Both wrong: `r` is the spring, and the circle is the key's rotation path. The lesson is the lane's own rule restated — a figure with a letter key is not read until the key is read.

### The finish question, settled in the original

Gardner's "laid over red cloth or paper" turns out to be a direct echo of his source, and Viollet is more specific:

> "Entre ces ornements de fer battu et le pallâtre, est apposé un **morceau de drap rouge** maintenu par les rivets qui retiennent les découpures."

and at the Saint-Sernin crypt lock, *"le morceau de drap interposé est donc **visible entre les découpures**"* — **the red shows through the piercing.** That is the two-material shader confirmed from the primary authority. At Ebreuil the substrate differs: the ironwork is bedded on **skins glued to the wood and painted bright red**.

But **on gilding Viollet is silent.** What he documents is red. Gardner's "profusely gilded" is his own addition, not corroborated by his source — so the finish rule should be *red ground under pierced ironwork*, with gilding held as a separate, weaker claim.

### Three corrections to what cycle 14 recorded from Gardner

| Cycle 14 said (from Gardner) | Viollet, the earlier authority |
|---|---|
| The triple strap's reason is **security** — limbs spring from behind the stonework, hard to wrench off | **Contradicts.** Viollet gives a purely **mechanical** reason: the branches multiply nail points, clamp the boards into a net of iron, and stop the band sagging under the boards' weight. He nowhere argues security. |
| **Date the hinge by its terminal** | **Silent** on that rule; he dates by **structure and process** — a welded C-arc off the collet = 11th c; collet passing through the leaf, *fausses pentures* and lens section = early 12th c; clasps over every weld later. A better axis, because it is about how the thing was made. |
| Pierced sheetwork is **laminated**, layer count = richness and date | **Refines and relocates it.** Viollet's lamination is **structural, in the band** — a *doublure*, or three bars laid together, **welded only at intervals and deliberately left free between**, for stiffness plus elasticity plus lightness. |

Assembly too: the dominant joint in pentures is **fire welding** (*soudure au marteau*) with scarfed ends, not tenon-and-mortice; rivets appear at the *crampons*. Viollet's own astonishment is that iron under welding heat behaved "like wax or lead."

### The constraint that changes how we build a door

> The *gonds* are bedded **into the jamb courses as the wall is built**, and by the time the ironwork arrives there is nothing left to notch.

So the ironwork is planned and positioned **from the start** — it is not applied to an already-finished door. Any door asset whose ironwork is a decal on a completed leaf has the construction order backwards.

### The earliest lock type, and two details worth having

The *serrure à bosse* is the oldest known — *"ne datent guère que du XIIe siècle"* — its box hammer-raised with bevelled edges, set on a pallâtre, boss on the outer face and bolt within, keyhole pierced above the bolt, generally only one. Two details a generator can use: the ribs riveted on the plate reinforce it **and** serve *"pour guider la clef, si l'on veut ouvrir la porte dans l'obscurité"* — to guide the key in the dark; and the box is **buried in a thick door but its back shows on the outside of a thin one**, a thickness-dependent variant. Ornament technique is cold work — *"battu et modelé au marteau, à froid"* — with *"pas de soudures, tout est rivé"*, and named joints: a *langue-de-carpe* taken in a *grain d'orge*.

### Process: the workflow failed, and the redesign still paid

Two failures, both mine to own. **The workflow crashed on my own bug** — `verdicts.filter(Boolean)` filters the wrapper object, not the `null` verdict inside it, so `x.v.refuted` threw once an agent errored. And **three of nine agents died on API errors**, including the priority locks reader — the second death on this same article region.

But the redesign was vindicated. **Checkpointing worked**: `hinges.json` survived *complete* — 18 verified figures, 40 findings, 26 dimensions and the nine-item Gardner check above — where the previous attempt lost every byte. And because 65 figures had already been cached to disk, I could open the priority figure and read the keyed text **myself**, which is how the headline finding exists at all. A partial file plus a warm cache beat a clean run that returns nothing.

The conjecture audit also worked this time: **4 of 40 findings flagged with the actual hedge word**, including Viollet naming his own source's limit — Mathurin Jousse's 1627 treatise used as a proxy for medieval practice, continuity asserted rather than demonstrated.

## 25. Cycle 19 — the debt-cleaning pass, which turned out not to be cheap

Three flagged items from cycles 17–18, done directly rather than by fan-out because each is a precise lookup. All three resolved, and the *Dictionnaire* being a dictionary is what made it easy: the disputed terms have **their own articles**.

### The *giron* gloss: corrected from Viollet's own entry

Cycle 17 recorded that a winder's *giron* is "measured at its wide end… near the outer wall, not on a modern walking line." The verifier refuted it; the `Giron` article settles it outright:

> "GIRON, s. m. Est la largeur d'une marche d'escalier. Le giron est dit *droit*, lorsque la marche est d'une égale largeur dans toute sa longueur; *triangulaire*, lorsque la marche est renfermée dans une cage circulaire. Alors **on mesure le giron de la marche au milieu de sa longueur**."

So the going of a winder is measured **at the middle of the step's length** — essentially the modern walking line, the *opposite* of what was recorded. That matters practically: measuring a newel-stair winder at mid-length versus at the outer end gives materially different numbers. The manifest row is replaced.

### Three more stair rules, free, from the sibling articles

- **`Noyau`** gives the newel as a 2×2 matrix plus two named bearing methods: newels are "**pleins ou évidés, tenant aux marches ou indépendants**" — solid or hollow, integral with the steps or independent — and an independent newel carries its steps by an *embrèvement* (housing) or a *repos* (ledge). Four variants and two joints, in one sentence.
- **`Limon`** gives a hard negative rule: a *limon* is a raking **timber**, and "**les limons de pierre n'étaient pas employés dans l'architecture du moyen âge**" — stone strings were *not* used in medieval work. Instead the step revolutions of square or oblong-plan stairs were carried **on arches**, "beaucoup plus solide que le système de limons appareillés." A generator that puts a stone string on a medieval stair is wrong, and the correct answer is an arch.
- **`Échiffre`** defines the *mur d'échiffre* as the wall the steps bear on **when that wall does not rise above the stepped levels** of the flight.

### The Escalier figures, now text-keyed instead of eyeballed

Cycle 17 had four of six numbers on the Mainz figure refuted. Viollet's text explains why they were unsafe: it describes fig. 22 as "**la moitié de son plan et une révolution entière**," with the construction being "des **marches portant noyau**, et en des **colonnettes, toutes d'égale hauteur**, soutenant chacune l'extrémité extérieure d'une marche" — but **it gives no count of colonnettes or winders at all.** So the counts were figure-readings presented as facts, and the honest record is the *relation* (one colonnette per step, all of equal height, newel integral with the steps) rather than a number.

Two figures nearby carry harder rules:

- **Fig. 21 — a newel-*less* vis.** From the 14th century, where space was tight, "on supprimait entièrement le noyau"; the steps are simply superposed in a spiral and **each carries a *boudin* at its inner end to serve as the handrail**, with a void where the newel would be. That is a distinct stair *type*, not a variant.
- **Fig. 23 — Reims.** A fabrication rule: "**trois marches sont prises dans une seule assise**" — three steps cut from a single course, because the Reims stone is enormous. And a load-path statement that inverts the obvious reading: the stone *chandelles* relieve the bearings, but "**par le fait, c'est le noyau D qui porte toute la charge**, et les pierres B ne sont qu'une suite d'étançons formant clôture à jour" — the newel carries everything and the outer props are an open screen. Recorded as a *variant* of Mainz's arrangement rather than a contradiction of it, since they are different buildings.

Also captured: the three soffit treatments for vis steps — *délardées*, simply *chanfreinées*, or left at *angles vifs* — which is a clean generator enum.

### The conjecture audit — and a correction to the correction

Cycle 17's verifier reported that "affirmerons", "hypothèse", "restitution" and "C'est possible" occur zero times in the findings file, implying unflagged hedges. **A census of the source shows those four strings occur zero times in *Construction — Voûtes* as well.** They are in the *separate* `Voûte` article (affirmerons ×1, hypothèse ×1, restitution ×2), which was only a secondary skim target. So that particular refutation **conflated two articles** and was unfounded as stated.

The real hedge count in the mined article is **six**: *nous supposons* ×3, *il est probable* ×1, *peut-être* ×2. And two of them qualify things already in our manifests, which matters far more than the bookkeeping:

- On the abutment rule recorded in cycle 17 as "span/4 for a semicircle": Viollet writes "**Il est probable** que les architectes gothiques primitifs s'étaient fait des règles très-simples pour les cas ordinaires; mais il est certain qu'ils s'en rapportaient à leur seul jugement" whenever anything was out of the ordinary. So **the span/4 rule is his reconstruction of what they might have used, not a documented medieval rule** — and the manifest should say so.
- On his whole rationalist reading: "**Les constructeurs gothiques n'avaient que l'instinct de cette théorie. Peut-être** possédaient-ils quelques-unes de ces formules mécaniques que l'on trouve encore indiquées dans les auteurs de la renaissance." **Viollet himself says the medieval builders had only the instinct of the theory** — which is the most honest sentence in the article and should temper how we present any of his thrust rules.

The lesson for the lane is symmetrical with the one it keeps learning about readers: **a verifier's refutation is itself a claim, and can be wrong.** This one was right about the numbers and wrong about the hedges.

## 27. Cycle 20 — the fortification group: a large haul, and it is unverified

The largest tranche attempted, and the first cycle where the infrastructure, not the design, decided the outcome. Deep-dive: [viollet_fortification_v0_1.md](viollet_fortification_v0_1.md). Manifests: 269 plate rows, a new [fortification_dimensions_v1.tsv](reference_manifests/fortification_dimensions_v1.tsv) (116 rows), 84 vocabulary terms.

### What happened, plainly

**Twelve of thirteen agents died on API 529 — server overload.** All six readers, all six verifiers. Only the synthesis agent survived. Finding no reader files, it **mined the four pre-saved source texts directly itself** and produced all four deliverables, then stated the gap in its own coverage section rather than papering over it.

So the pre-saving of texts to disk during recon — done to remove a fetch-time failure mode — turned out to be what let the cycle produce anything at all. That was luck as much as design, and worth naming as such.

**No verification pass ran.** In every previous cycle of this lane verification refuted or corrected something, twice killing a headline finding. This data is therefore a strong first draft, not checked fact. The flag travels on the rows themselves — every plate note is prefixed `UNVERIFIED`, every dimension row carries `verification = unverified-cycle20` — because a caveat that lives only in prose gets separated from the numbers. **Verifying it is now the top queue item.**

### The arms race, dated — the thing that turns a castle into a parameter set

Viollet organises all four articles around one narrative: attack and defence alternate the lead, and every form is dated by which was ahead.

| Date | Change | Cause given |
|---|---|---|
| to 5th c | Roman curtain, flanking towers on the Vitruvian rule; **no portcullis, no moving bridge** | — |
| c.1040 | Norman donjon at Arques; **door raised six metres**, ladder only | the defining early feature, and the type's fatal flaw |
| 12th c | gates shrink to "à peine 3 mètres d'ouverture" | only the ram, sap and mine could touch a wall, so every assault went at the doors |
| c.1200 | Coucy: the gate becomes an independent fort | **anti-treachery** — "beaucoup de places étaient prises par la trahison d'un poste" |
| 1250s | double enceinte and *lices*; outer towers **50–60 m apart** | distant fire outside, "courts flanquements très-multipliés" inside |
| 1285 | spurs (*becs*) added at the tangent | the round tower's dead point admitted outright |
| early 14th c | hoardings abandoned for **stone machicolation**, and **gates widen to 3.50 × 3.50 m** | crusade engines burned timber hoardings; mining improved, so the gate stopped being decisive |
| mid–late 14th c | powder arrives; answer is to **raise towers further** | flat-trajectory fire still weak |
| 1428 | Orléans: guns point-blank to 600 m | trench approaches plus massed guns |
| 16th c | bastion general; walls thicken to 7 m at Langres | **merlons became the danger** — "les merlons de pierre enlevés par les boulets se brisaient en éclats, véritable mitraille plus meurtrière encore que les boulets" |

At Coucy the two portcullis winches are **geared so only one can ever be raised at a time**, and the guard-room doors are turned so the passage cannot see how many men are inside — the period's detail is driven by fear of betrayal, not of armies.

### The best generator rule in the cycle

Curtain height is held **constant above outside ground, following the terrain** — "les remparts se conforment aux mouvements du sol, et les hourds suivent l'inclinaison du chemin de ronde." Outer enceinte ~10 m from ditch bottom to hoarding floor, inner ≥14 m, both as anti-escalade minima. **So a generator should drive wall-head level from terrain plus a constant, not from a fixed datum.** Viollet then states the general claim outright: "Il y avait donc alors des données, des règles, des formules pour l'architecture militaire."

Other parameters recovered with their governing reason: curtain 2–3 m thick pre-gunpowder; wall walk ≤ 2 m (which is *why* 15th-century walls need internal earth ramps); ashlar facing 0.30–0.50 m over rubble, "adequate against stone shot, fatal against iron"; machicolation corbels **0.70–1.20 m axis to axis** carrying a crenellated wall 0.33–0.40 m thick and 2 m high; tower interval set by shot range, via Vitruvius.

### Both headline mechanisms are shown in section

- **Portcullis: yes, three times over, plus a dedicated gear plate** — winch chamber in plan, bridge platform in plan, double-pulley arrangement in section. Counterweighted, two men on the winch, chocks and a barred pin to lock it down. One detail is drawn from the sockets, pitons and pulley seatings still in the wall.
- **Drawbridge: yes, in section** — the arms swinging back into their face grooves as the counterweights fall, with the trunnion detail and its iron-lined stone housing. A single-arm postern version carries a 0–3 m scale bar.
- And an unlooked-for one: the **tilting bridge at the Coucy donjon door has a hinged flap in its deck that drops a rope ladder into the paved ditch**, so the garrison can get out without lowering the bridge at all.

### The reliability verdict — the most useful paragraph in the cycle

This is the group where Viollet is least safe, because he was *rebuilding* three of the buildings he documents. Raw hedge counts across the four articles: *devaient* 43, *devait* 33, *paraît* 78, *supposé* 31, *peut-être* 25, *restaur\** 18. And the pattern matters more than the totals — **the hedged claims are not the marginal ones; they are frequently the upper defensive storey, which is exactly the part a generator most wants.**

Three named cases: the Pierrefonds hoarding plate is labelled a restoration in his own text; the Rouen donjon's entire upper storey is admitted invention — "nous n'avons, pour restaurer la partie supérieure, que des **données insuffisantes**. Toutefois **on doit admettre** que…"; and the Provins hoardings are inferred from a single ledge behind a confident double negative.

The practical rule the synthesis landed on is worth quoting whole:

> Take Viollet's dimensions of things that were standing when he measured them. Take his *reasons* seriously — a stated reason is exactly the invariant a generator must hold true when parameters move. **Do not take his skylines. Anything above the last surviving course — hoardings, crenellations, roofs, pinnacles, watch turrets — is his drawing, not the building's.**

Two of his assertions are flagged as *not* to be recorded as fact: that arrow loops were near-useless — his judgement, offered without test, and the basis of his claim that "la véritable défense était disposée au sommet des ouvrages" — and his garrison arithmetic of roughly one man per metre of wall. Build loops to his dimensions; do not build ballistics on his verdict.

### Two negative results and a cataloguing trap

**No crenel or merlon widths exist anywhere in these four articles** — the most basic castle parameter is simply not there, and must come from elsewhere. **257 of 269 figures were never opened**, so the plate rows are overwhelmingly located-not-viewed.

And the filename trap is now confirmed to be worse than "unreliable": the names **swap between plates**. `Coupe.hourds.courtine` is figure 33 and `Coupe.hourds.coursiere` is figure 32 — the exact opposite of what they imply. Four `Tour` files labelled Carcassonne are the porte Garonne of **Cadillac**. Cite by filename, never by the place or number a filename claims.

## 29. Cycle 21 — verifying the fortification haul, mechanically

Cycle 20's data had no adversarial pass because a 529 storm killed twelve of thirteen agents. My own queue note said to run the verification "small and staggered". I ran it **solo and mechanically instead**, which turned out to be the better instrument: the central question — are these French quotations real? — is decidable by string matching against the four local source texts, and a program does that more reliably than a reader.

### The quote audit: 112 of 116 verbatim, zero fabrications

Every dimension row carries a French quotation. Normalising both sides and testing for containment gives **112 exact hits**. The four remainders are all explained and none is invented:

- **Two** carry the synthesis agent's *own bracketed corruption notes* — `des [li]ces`, and `15 [m],50` annotated "source text prints '15 e ,50'". It flagged the damage in the quote field rather than silently emending. Honest.
- **One** elides a clause without marking the ellipsis: the source reads "surmontée de mâchicoulis **qui règnent tout le long de la courtine**, a son seuil posé à sept mètres". The number is correct and present.
- **One** is a whitespace artefact of my matcher.

So **all 116 numbers are real and sourced.** For data produced with no verification pass, that is a much better result than this lane's history would predict.

**Two bugs were mine, not the data's**, and both are worth recording because they would have produced false accusations:

1. Replacing punctuation with *spaces* rather than deleting it made `0 m , 50` and `0m,50` differ — that alone generated **69 spurious misses**, and had I stopped there I would have reported the haul as largely fabricated.
2. **`œ` does not NFD-decompose to `oe`**, so `.encode("ascii","ignore")` deletes it outright, and every quote containing *hors œuvre* failed. Six more spurious misses. **Ligature handling must be explicit** — a note now in the record, since French sources are full of œ.

The lesson generalises: **a verification tool that has not itself been verified produces confident false negatives.** My first pass "found" a 59% failure rate that was entirely my own normalisation.

### The evidence classes: five reclassifications, and one of them is the headline

The second target was whether any of Viollet's restorations are filed as measured fabric. Cross-referencing rows classed `measured-fabric` against his three restoration sites *and* against elements above the wall head found **seven candidates, of which five needed reclassifying**:

| Row | Why it moved to `documented-reconstruction` |
|---|---|
| **Carcassonne outer enceinte, 10 m** | datum is the **hoarding floor** |
| **Carcassonne inner enceinte, 14 m** | measured "du sol des lices **au sol des hourds**" |
| Pierrefonds machicolation holes, 0.42 m | Pierrefonds was **his own active building site** |
| Pierrefonds machicolation drop, 3 m | same — the walk is part of the restored crown |
| Carcassonne Visigothic tower, 1.28 m | datum is the **crenel sill**, and Carcassonne's crenellation is largely his |

**The first two are cycle 20's headline generator rule.** The 10 m / 14 m anti-escalade curtain heights are measured *to the hoarding floor* — and cycle 20 itself established that every timber superstructure in this group is Viollet's, drawn onto surviving sockets. The socket line is genuinely measurable; calling it "the hoarding floor" embeds a reconstruction. The rule survives as a design rule, but its datum is inferred, and the manifest now says so.

The Pierrefonds pair is the sharper worry: he quotes himself rebuilding the place — "déjà la partie de la tour carrée qui avait été jetée bas est remontée" — so a machicolation dimension measured there may be **his own new stonework**. I have reclassified rather than deleted, and flagged both for a source re-read, because I cannot tell from the text alone which it is.

One row was checked and **left alone**: Coucy's 55 m total height explicitly reads "non compris les pinacles" — Viollet excluding the reconstructed crown himself. Correctly classed.

Net: `measured-fabric` 92 → **87**, `documented-reconstruction` 10 → **15**, `viollet-conjecture` unchanged at 14. The blanket `UNVERIFIED` prefix on all 269 plate notes is replaced with the actual state — quote-audited, figures still unviewed.

### What this does not cover

The quote audit proves the numbers were **transcribed** faithfully. It does not prove they were **interpreted** faithfully — that a quoted figure measures the element the row claims. Spot-checks were consistent, but a full interpretive check needs the figures opened, and **257 of 269 remain unviewed**. The dated chronology and the mechanism-in-section claims are likewise unaudited, and stay in the queue.

## 30. Cycle 22 — the trace pass: geometry at last, and a number I refused to ship

The first cycle in this lane that outputs *geometry* rather than records. It produced working
tooling and three profile definitions, and it ended by marking all three **not dimensionally
fitted** — which is the honest result, not a failure.

### The call: reconstruct the construction, do not trace the pixels

There is no numpy, PIL, scipy or cv2 on this machine (checked, not assumed). But that is the
*second* reason for not tracing. The first is in Paley's own text: these profiles are
compass-and-square constructions — hollows are arcs of stated fractions of a circle "accurately
formed with the compasses", chamfers are 45° "generally (not invariably)", and every figure is
drawn in one fixed frame. **The authored form of the profile IS a chain of circular arcs and
straight runs.** Vectorising a hatched 1845 engraving would yield a noisy polyline that is
strictly *worse* than reconstructing the construction that made it.

So: a parametric profile format, with a generator, checked by eye against the plate.

### What was read

Plate I fig 6 was cropped and magnified (`sips -c 430 620 --cropOffset 600 990` on the w1100
scan). At that magnification the member chain is fully legible: a bold **bowtell**, a **hollow**,
a **smaller roll**, and a **45° chamfer**. Fig 7 shows *receding orders* with a quarter-round
hollow worked into each internal angle — a different compositional strategy from fig 6's single
run, and worth keeping as a distinct archetype.

### What was built

`reference_manifests/moulding_profile.py` — schema, generator, SVG writer. No dependencies.

* **Frame**, taken from Paley so every profile shares one coordinate system: `+x` runs soffit →
  wall face; `+y` runs wall-line → into the opening; origin at the arris; units inches, because
  that is what his dimension figures print. Chain read **soffit-first**, the order a mason works
  and the order Paley lists.
* **Primitives**, the observed vocabulary from §3: `fillet, chamfer, roll, hollow, ogee, quirk,
  step`.
* **`stock`** records the dashed uncut block Paley draws behind most figures — so a generator can
  model *stone removal* rather than extruding an outline, which is what a section actually
  represents. This matters for downstream asset work more than the outline does.

`reference_manifests/moulding_profiles_v1.json` — three profiles: `paley_pl1_fig6`,
`paley_pl1_fig7`, `brandon_pl9_tiebeam`. SVGs alongside for eyeball comparison.

### The defect, and why the profiles ship unfitted

`extent()` exists to check a built profile against its printed stock. It immediately failed:

| profile | built extent | printed stock |
|---|---|---|
| `paley_pl1_fig6` | 4.63 × 9.10 in | 11 × 6 in |
| `brandon_pl9_tiebeam` | 3.14 × 3.60 in | 20 × 14 in |

Fig 6 builds **taller than its own block**. The radii were eyeballed — which is exactly the
"estimate presented as a dimension" failure this lane keeps flagging in other people's work.

To fix it properly I converted the plate to PNG and wrote a pure-Python decoder (zlib inflate +
PNG unfiltering + greyscale reduction; `trace/png.py`, verified on the 1930 × 2951 scan, 10.7%
dark pixels) and started measuring the printed dimension lines to derive a real scale. That got:

* stock right edge ≈ x=1243, bottom edge ≈ y=771
* the dashed "11 in" line spans x≈939 → 1246 = **307 px**, i.e. **≈27.9 px/in**

and then **the cross-check failed**. If 11 in = 307 px then 6 in = 167 px and the stock top
should sit at y≈604; there is no horizontal line there, nor at the alternative y≈649. Worse, the
widest dark span in the fig-6 band is **359 px — wider than the 11-inch dimension itself**, which
means my sampling window is contaminated by the neighbouring figures. Isolating fig 6 from figs 5
and 7 on a foxed plate is a real sub-problem, not a detail.

**So the numbers are not shippable, and I did not ship them.** All three profiles are marked
`"fitted": false`, `"units": "relative"`, with a note stating they are good for member **shape and
order** and not for **size**. The `stock` block keeps its `status` field noting it *is* a printed
dimension, unlike the members.

### What this cycle establishes

1. The lane can produce geometry, and the format for it now exists and runs.
2. A profile is a **removal** from stock, not an extruded outline — recorded in the schema.
3. Pixel measurement off these plates is viable (the decoder works) but needs **per-figure
   isolation** first. That is now the concrete blocking step, with tooling already in place.
4. Negative-result discipline held under pressure to produce: a generator that emits confident
   wrong inches would have been worse than one that emits honest proportions.

## 31. Cycle 23 — the fit: a scale that finally cross-checks, and the '6' that was never a dimension

Cycle 22 ended blocked: the px/in scale for Paley Plate I fig 6 would not cross-check, because
the sampling window was contaminated by neighbouring figures. This cycle unblocked it, and in
doing so overturned two things I had recorded as fact.

### Per-figure isolation

`reference_manifests/seg.py` — block-OR downsample (which doubles as a dilation, bridging dashed
lines and hatch strokes so a figure resolves as ONE component) plus 8-connected BFS labelling.
On plate 1 it returns 48 components; k=4 over-merges, k=1 in a window separates cleanly.

`reference_manifests/pngw.py` — a greyscale PNG *writer* with crop, nearest-neighbour magnify and
a **burned-in coordinate grid**. Written because `sips --cropOffset` argument order is ambiguous
and every crop I made with it had to be re-derived. Now plate coordinates are ours: a feature
read by eye at 5× lands on a known plate pixel. This pair plus `png.py` is the lane's imaging
toolkit, no dependencies.

### The scale, and the cross-check that passes

Fig 6's printed **"11 in"** dimension line, arrowhead tip to arrowhead tip, spans plate
**x1003..1246 = 243 px** → **22.09 px/in**.

The check: fig 6's stock rectangle measures **x[1001,1242] = 241 px**, which at that scale is
**10.91 in** against **11 in printed — 0.8% error**. That is the independent confirmation cycle 22
could not get. Scale accepted; measurement tolerance ±0.1 in.

### Two corrections to the record

**1. Figures on one plate are drawn at DIFFERENT scales.** Fig 5 carries its own **"1 ft"** line,
spanning plate x725..1010 = 285 px → **23.75 px/in**. That is **7.5% off** fig 6's scale — far
outside measurement noise on 240+ px spans. This explains why Paley prints a dimension on
individual figures instead of one scale bar for the plate, and it kills any "measure one figure,
apply to the sheet" shortcut. **Every figure needs its own dimension line.** `paley_pl1_fig7` and
`brandon_pl9_tiebeam` are accordingly still unfitted — fig 6's scale may not be lent to them.

**2. The stock was never 11 × 6 in.** Cycle 22 recorded that. Measured, the box is
**11 × 9.28 in** (width printed, height derived). The "6" I recorded as a dimension is the
**figure number**, which Paley sets *inside* the stock rectangle — it sits at plate x≈1123,
y≈677, in clear space between the soffit and the bowtell. A caption read as a measurement.
Worth generalising: **on these plates the figure number lives inside the frame it labels.**

### The members, measured

At 22.09 px/in, ±0.1 in — soffit flat **4.66 in**; a hollow of **1.0 in** radius rising to the
arris; the principal **bowtell 4.07 in horizontal × 4.48 in vertical diameter** (mean r = 2.14),
near three-quarter round on a narrow neck; a hollow above it; a 45° chamfer to the wall face.

### A schema fix the source dictated

The chain built barely half the stock. The cause was real, not arithmetic: I accumulated heading
through every arc, so the chamfer inherited the exit tangent of a ~250° bowtell. But Paley says
chamfers are "generally (not invariably)" 45° — an angle **to the wall face**, to the drawing
frame, not to the previous member's tangent. Straight members now accept `absolute: true` and set
their heading outright. Chaining a flat off a three-quarter-round's exit tangent is meaningless;
chaining it off the wall face is what the engraver drew.

### The residual, left standing

With that fixed the chain closes to **8.14 × 9.48 in** against the measured **10.91 × 9.28**
stock. Depth agrees to **2.2%**; **width is 2.77 in short**, and I have not resolved why. Most
likely a soffit member outside the stock box — the connected component runs to plate x=667, well
left of the box edge at x=1001 — or the bowtell neck is still mismodelled.

I did not tune it away. A four-parameter grid search over sweeps and lengths *will* hit
10.91 × 9.28, and the result would be a number fitted to a target rather than read off a plate —
the exact failure this lane exists to catch. The residual is recorded as a residual.

### Also found

`iabook.py` gains a trap: **the `w` parameter in the IA download URL is a request, not a
guarantee.** `n88_w1000.jpg`, `n88_w1100.jpg` and `n88_w1400.jpg` are all 1930×2951 — three names,
one image. A larger `w` buys no detail, and the requested width must never be recorded as the
plate's resolution.

## 32. Cycle 24 — the residual was a bug, and the overlay caught what the numbers hid

Cycle 23 left fig 6 building **2.77 in short in width** and named two suspects: a missing soffit
member, or a mismodelled bowtell neck. **Both were wrong.** It was a bug in my own generator.

### The shared baseline — suspect one, eliminated

Fig 6's soffit does not continue past its stock box. Figs 5 and 6 **share one soffit baseline** at
plate y=772: the engraver drew a single continuous line across both figures. That is why
connected-component isolation returned x[667,1246] spanning both. The two dimension lines abut at
x≈1004–1010, which fixes the figure boundary exactly. No member was missing.

### The bug

Tracing the chain member by member: after the soffit flat reached x=4.66, the hollow moved x
**backwards** to 3.76, and the bowtell never advanced past 4.66 at all.

Every arc was being built on the **wrong side of the heading** — centre at `hd + sign·90°` where
it must be `hd − sign·90°`. A convex roll bulges *away* from its centre, so its centre lies to the
right of travel; a concave hollow cups *toward* its centre, so its centre lies to the left. With
the sign inverted, each arc curled the chain back on itself. The `ogee` path had the identical
bug. Fixing it took fig 6 from **8.14 → 10.70 in** width immediately.

**This is the second time in three cycles that a confident number came from unverified tooling.**
Cycle 21 recorded "a verification tool that has not itself been verified produces confident false
negatives." A *generator* that has not been verified produces confident false residuals — and I
spent cycle 23 hunting the plate for a member that was never missing.

### Compass form for arcs

Depth still would not close. The cause was structural: a near-full-round bowtell is attached at a
**neck**, so the outline enters and leaves at nearly the same place, and `sweep` alone cannot say
*where* it leaves — the next member then lands wrong. But Paley's arcs are struck with the
compasses: the engraver set a centre and swept between two angles. So arcs now accept
`centre` + `a_start` + `a_end` outright.

This is the same lesson as `absolute` on straight members in cycle 23, and worth stating once as a
rule: **where the plate states a position, take the position; do not derive it from an accumulated
heading.**

### Everything measured

| member | source | value |
|---|---|---|
| soffit flat | measured | 4.57 in (101 px, x1004–1105) |
| lower hollow | solved from its measured chord | r 1.63 in, sweep 94.6° |
| bowtell | measured, compass form | centre plate (1198,727), r 2.04 in, −174° → −253.6° |
| upper hollow | derived from two measured endpoints | r 0.82 in, sweep 180.1° |
| chamfer | measured | 4.01 in at **40.9°** |

That chamfer is worth its own line. Paley writes that chamfers are "generally (not invariably)"
45°. This one is **41°**. The hedge is real and load-bearing — a generator that hard-codes 45°
would be wrong on the very first figure on the very first plate.

### The overlay, and why the extent match was not enough

Built extent: **10.77 × 8.20 in** against measured **10.82 × 8.19** — 0.4% and 0.1%. On the
numbers, closed.

So I stamped the reconstruction over the engraving at 4× and looked
(`paley_pl1_fig6_overlay.png`). It tracks the drawn hatched/unhatched boundary along the soffit,
the lower hollow and the bead's left flank, and the chamfer lands on the stock's top-right corner.
But there is a visible **0.56 in (12 px) C0 break at the arris**, where the lower hollow's
chord-solved exit meets the bowtell's compass-form start. The two were measured independently and
do not coincide.

**The extent agreement was not evidence of correct geometry.** A chain can match a bounding box
while following a different path through it — and the bounding box here is set by the soffit, the
bead's right edge and the chamfer's end, none of which constrain the middle. Only the overlay
caught it. Numeric closure is now demoted: **no profile is fitted until it has been stamped over
its own plate and inspected.**

## 33. Cycle 25 — the break was a misidentified member, and a fitted circle that proves itself three ways

Cycle 24 closed fig 6's extents to 0.4% and then the overlay showed a **0.56 in break at the
arris**. The plan was to fix it with a tangency constraint. That was the wrong diagnosis too: the
member was not badly parameterised, it was **the wrong kind of member**.

### The rising limb is a chamfer, not a hollow

Traced from pixels at 12×, the limb from the soffit to the arris runs plate (1104,768) →
(1140,733): **straight to within 1.8 px over 36**, at **44.2°**. Cycles 23 and 24 both recorded it
as a hollow of radius 1.63 in swept 94.6°, solved from its chord — and a chord fit will happily
return a circle for a straight line. That misidentification is what broke the chain.

### The bowtell, least-squares fitted

Tracing the falling limb and fitting a circle algebraically: **centre plate (1190.5, 721.5),
r = 50.6 px = 2.29 in**, residual **mean 0.37 px, max 1.01 px** over 43 samples.

The fit then reproduces three drawn features it was never given:

| derived from the fit | drawn on the plate |
|---|---|
| bottom y = 772.1 | the soffit baseline, y = 772 |
| right x = 1241.1 | the stock's right edge, x = 1241–1243 |
| left x = 1139.9 | the arris cusp, x = 1140 |

Cycle 24's eyeballed centre (1198, 727) r = 45 px was 6 px and 0.25 in out — enough to push the
cusp 12 px off the circle and produce the break. A first attempt at fitting *did* fail (14 px mean
residual) because "rightmost dark pixel per row" traced the stock's dashed edge, not the bead;
tracing the crisp contour directly fixed it.

### Result

| | cycle 24 | cycle 25 |
|---|---|---|
| arris break | 0.56 in (12 px) | **0.056 in (1.2 px)** |
| extent vs measured 10.82 × 8.19 | 10.77 × 8.20 | 11.01 × 8.24 |

The break is now at the engraved line's own width. The extent is marginally *worse* (1.7% vs 0.4%)
and that is fine — cycle 24 established that extent agreement is not evidence of geometry, and
this profile is right where the last one was wrong.

Also measured: the soffit is **not level**. It rises 4 px over 100 (2.3°). That is at the noise
floor of an 1845 engraving on a 1930 px scan, but the measured value is more defensible than
assuming zero, and using it is what took the break from 0.204 in to 0.056 in.

### Paley's hedge, confirmed twice on one figure

Chamfers on fig 6 measure **44.2°** and **40.9°**. Neither is 45. His "generally (not invariably)"
is doing real work, and a generator that hard-codes 45° is wrong twice on the first figure of the
first plate.

### What the overlay raises next

The reconstruction now tracks the engraved boundary along the soffit, the chamfer, the cusp, the
whole bowtell and the upper members. But a **hatched diagonal band sits up-left of it**, running
from about plate (995,737) to (1185,642). It is either fig 5's mass encroaching past its own
dimension line — which stops at x=1010 — or a fig-6 member still unaccounted for. Two cycles of
confident wrong diagnoses say to measure it rather than guess, so it is queued.

## 34. Cycle 26 — the band answered: a section is a closed outline, and I had traced one side of it

Cycle 25's overlay left one question: a hatched diagonal band up-left of the reconstruction, from
about plate (995,737) to (1185,642). Fig 5 encroaching, or an unaccounted fig-6 member? Measured,
it is the second — and the answer carries a structural consequence for the format.

### Fig 5 does not encroach

Its mass ends at plate **x = 999–1003** across y615–675, matching its own stock edge at x=1007.
Only at y=600 does anything reach x=1030. The two figures respect their boundary.

### The band is fig 6's own upper face

Tracing the top of fig 6's mass column by column: **plate y ≈ 722**, near-horizontal (2.9° over
x1004–1100, deviation 4.5 px), then stepping up — y=712 at x=1110, ~700 at x=1130–1160, y=690 at
x=1170–1200. The slab between that face and the soffit is **46 px = 2.08 in** thick at x=1050.

That is a genuine fig-6 boundary, and my reconstruction never traced it.

### The consequence: a section is closed, my format is open

Fig 6's stone is bounded on **both** sides — soffit below at y=772, upper face above at y≈722. The
reconstruction traces the lower boundary only, which is why the overlay showed stone sitting above
a curve that appeared to be the profile. The curve *is* a real boundary; it is simply not the
whole outline.

`moulding_profile.py` models an **open chain** of members. A moulding section is a **closed
outline** — the block, minus what the mason removed. The `stock` field was the right instinct
(§22 recorded that a section represents *stone removal*, not an extruded outline) but the member
chain never followed it through. Either the members must close the loop, or the format needs an
explicit distinction between the **moulded face** and the **bounding planes of the block**.

This is the third schema correction the plate has dictated, and they rhyme: absolute angles for
straights (§31), compass form for arcs (§32), closed outlines here. Each time the format assumed
something the drawing does not.

### Method note: an 1845 plate scan has no white

The first binarization came back **entirely black**. The threshold was 188, chosen by eye. But
measured, this scan's clear paper has a median of **180** (tight, 175–183) and hatched stone a
median of **146–154**. There is no white on the page at all. Local-mean thresholding at a
*measured* 172 separates them cleanly; anything picked by eye does not.

Same shape as the traps already recorded: a tool whose parameter was guessed rather than measured
produces a confident, useless result. Measure the levels, then threshold.

## 35. Cycle 27 — the silhouette is not the profile, and fig 6 has two faces

The queue asked whether `moulding_profile.py` should close the loop. Tracing the full boundary
answered that, and turned up something more useful.

### The trace

`reference_manifests/contour.py` — local-mean binarization at the measured threshold, connected
labelling, Moore boundary trace. On fig 6 it returns a **1168-point closed boundary**.

### The bowtell is a real member

Worth confirming, because the binarized mass shows the bead embedded in stone rather than bulging
into void. Sampling 8 px inside against 8 px outside its fitted circle at twelve angles: **void
outside at 60°, 120°, 150°, 210° and 270°** — the upper-left, left and bottom — and abutting stone
elsewhere, where it meets neighbouring members. Interior reads a consistent 145–149; true void
reads 174–177. It bulges; it merely touches its neighbours.

### The silhouette cannot be used as the profile

The traced boundary merges three things that are not the moulding:

1. **Separate members, wherever they touch.** The bead's junctions vanish into the mass.
2. **The stock box's drawn top edge** at y=567 — a box line, not stone — which the component
   swallows whole.
3. **Fig 5**, through the soffit baseline the two figures share (§32).

So the mass is good for exactly one thing: deciding which side of an engraved line is void.
The profile itself has to come from line-tracing, as in §32–33. A silhouette is a different object
from a section, and on this plate it is a worse one.

### Fig 6 has two engraved faces, and the profile holds one

**Upper:** a continuous line from plate (1000,723) rising right through (1117,710), (1160,702),
(1185,637), (1213,597) to the stock's top-right corner (1242,591). This is the same line §34
measured as "the upper face at y≈722", and it runs directly into the 40.9° chamfer.

**Lower:** the chain already modelled — soffit, 44.2° chamfer, arris cusp, bowtell.

Both are real engraved boundaries with stone between them. **Which is the moulded face and which
the bed is not established, and I am not asserting it.** Three cycles of confident wrong diagnoses
on this one figure argue for leaving it open until something in the text or a companion plate
settles it.

### The format decision

**Members should not close the loop.** The moulded face is an open chain; the block's bounding
planes close it, and `stock` already holds the block — the instinct recorded in §22 was right. The
real gap is not topological, it is that fig 6 carries two faces and the profile records one.

## 36. Cycle 28 — Paley states the frame outright, and I had his axes swapped

The queue asked which of fig 6's two faces is the moulded one, and noted it was a reading problem
rather than a measurement one. Reading it settled the question — by dissolving it — and caught two
errors in my own tooling.

### The frame, in his words

There is no description page facing Plate I: leaf 89 carries only the imprint. The convention is
stated in the body text instead, and stated plainly. Lay the stone so that the wall-line, "or part
of the stone which lies in the plane of the outer wall, should be parallel with the end of the
paper nearest to you, and the soffit, or inner surface at right angles with it, parallel to one
side." And in copying: draw "the outer wall-line parallel with the bottom of the page, and the
soffit parallel to the side."

**The wall-line is horizontal. The soffit is vertical.**

So in fig 6, the horizontals — the line at y=772 and the upper line — lie in the **wall-plane**,
and the **vertical right edge at x=1241 is the soffit**. Cycles 22–27 called that bottom horizontal
"the soffit" throughout. It is the wall-line. `moulding_profile.py`'s frame note had the two axes
swapped, and is corrected.

### The chain order was backwards too

Paley: "a pencil were to be carried **along the wall-line first**, and afterwards in and out of
each cavity and round each projection." The docstring claimed soffit-first, and justified it as
"the order a mason works and the order Paley lists." He lists the opposite. That justification was
invented, and it survived six cycles because it sounded like craft knowledge.

### What the hatching means — and why the question dissolved

The figures are shaded "on the part which represents **the level surface of the flat side of the
stone**." The hatching is not decoration and not stone-in-general: it is the **sawn face** — the
cut section itself. He contrasts this deliberately with "the usual popular way of engraving Gothic
Mouldings", a perspective sketch showing the flat end and the moulded side at once, which he does
not use.

It follows that the boundary of the hatched region — minus the wall-line and soffit segments,
where the saw passed — **is** the moulded edge. So fig 6's two traced stretches were never rival
candidates for "the moulded face". They are two parts of **one continuous moulded edge** running
around the sawn section. The cycle-27 question was malformed, and no amount of further measurement
would have answered it.

### A scale that is advice, not data

Paley suggests reducing a traced outline to "a scale of an inch to a foot". It is tempting to read
that as the plates' scale. It is not — it is instruction to the reader copying mouldings in the
field, and the measurements refute it as a claim about the engravings: fig 5 is 23.75 px/in and
fig 6 is 22.09 px/in **on the same sheet** (§33).

### Confirmed from the source

His three planes — wall-plane, soffit-plane, and "the plane formed by chamfering an edge, which was
generally (not invariably) done at an angle of forty-five degrees" — are now quoted rather than
inferred. The measured chamfers of 44.2° and 40.9° (§33, §34) sit exactly where that hedge
predicts.

**Next confirmation available:** fig. 10, Plate 2 (leaf 92) labels the three planes directly —
a = chamfer-plane, n = soffit-plane, c = wall-plane. That is the figure to open to check the frame
by eye rather than by quotation.

## 37. Cycle 29 — the frame confirmed by eye, and a threshold that is not a constant

### Plate II fig 10, opened at full resolution

Paley's labelled diagram. Fig 10 is a plain chamfered section: a horizontal stretch, a diagonal,
and a vertical. The labels are **rotated capitals**, which is why the OCR gave "a … n … c" —
they are **A**, **B** and (by elimination) C.

* **A** sits below the **diagonal** → the **chamfer-plane**. Text: "a is the chamfer-plane." ✓
* **B** sits right of the **vertical** → the **soffit-plane**. Text: "n the soffit-plane." ✓

**The soffit is vertical**, confirmed by eye. §36's reading of the text was right, and the frame
correction stands on two independent legs now rather than one. I could not locate the third label
in either crop, so "the horizontal is the wall-plane" rests on elimination plus the text, not on a
label I have seen.

`paley_pl1_fig6` is re-expressed accordingly: the horizontal at plate y=772 is the **wall-line**,
the vertical at x=1241 is the **soffit** — and the bowtell's least-squares right extreme (x=1241.1,
§33) lands on it. Traversal is wall-line first. **No geometry changed**; every measurement from
§32–35 stands, only its labelling and order.

### The threshold is per-scan, and I reused it

Measuring fig 10's chamfer angle failed three times, each trace running straight to the edge of
whatever scan window I gave it. The cause:

| | clear paper | hatched stone | threshold |
|---|---|---|---|
| Plate I | 180 | 154 | 167 |
| Plate II | **167** | 146 | **156** |

**Plate II's paper is darker than Plate I's threshold.** Reusing 172 made the entire second sheet
read as solid stone, so "first dark pixel from the top" was always row one.

§34 recorded the lesson as "measure the levels, then threshold". That was too weak — I measured
once and then treated the number as a constant of the book. The rule is per **scan**, and
`contour.py` now carries it along with an instruction to assert `paper_median > threshold` before
trusting any trace. Same failure family as §32 (a generator never verified) and §33 (a chord fit
returning a circle for a straight line): the tool ran, produced numbers, and the numbers were
artefacts of the tool.

### What I did not measure

Fig 10's chamfer angle. After the threshold fix the trace still would not converge — my
scan-window bounds, not the plate, were setting the endpoints. Paley's chamfer-plane therefore
still rests on the two measurements from Plate I fig 6, **44.2°** and **40.9°**, against his
"generally (not invariably) forty-five". I would rather leave his own labelled example unmeasured
than report a third angle I cannot defend.

## 38. Cycle 30 — Paley's hedge, quantified: the textbook chamfer is 45, the recorded ones are not

§37 failed to measure fig 10's chamfer because my scan windows, not the plate, were setting the
endpoints. The fix was to stop choosing windows: isolate the figure as a connected component,
Moore-trace its boundary, and find the **longest maximal straight runs** on that boundary. No
window, no guess.

### Paley's three planes, recovered from pixels

On fig 10 the runs come out as:

| length | angle | what it is |
|---|---|---|
| 109 px | 178.4° | the **wall-line** — horizontal |
| 51 px | 87.8° | the **soffit** — vertical |
| 61, 51, 43, 35 px | 45.0°, 43.4°, 41.2°, 46.2° | the **chamfer-plane** |

And on Plate I fig 6, independently: 127 px at 179.5° (wall-line) and 80 px at 90.0° at x=1249
(soffit). **Paley's frame recovered from geometry alone on both plates**, with no reading involved
— a third confirmation after the text (§36) and the A/B labels (§37).

### The skew I had never measured

The horizontal reading 178.4° and the vertical 87.8° are not errors in the plate; they are page
skew. Using each figure's *own* known-horizontal and known-vertical runs as an internal angular
reference:

* **Plate I: +0.1° mean** — effectively unskewed, with a noise floor of about ±1°.
* **Plate II: −1.5° mean.**

So the 44.2° and 40.9° I reported in §33–34 stand **uncorrected** — Plate I needed no correction,
which I had not checked and got away with. Plate II's readings need +1.5°.

### The hedge, quantified

| | measured | skew-corrected |
|---|---|---|
| **fig 10** — Paley's own *labelled* chamfer example | 45.0, 43.4, 41.2, 46.2 | 46.5, 44.9, 42.7, 47.7 → **mean 45.5 ± 2** |
| **fig 6** — a moulding recorded from a building | 44.2, **40.9** | unchanged; noise floor ±1° |

His didactic figure is **a true 45°**. The mouldings he recorded from buildings depart from it —
40.9° is four degrees off and far outside Plate I's ±1° noise floor.

That is precisely what "generally (not invariably) done at an angle of forty-five degrees" means,
and it is now a number rather than a hedge. **A generator should neither hard-code 45° nor treat
the deviation as drafting noise: sample around 45° with a few degrees of real scatter.** This is
the first rule in the lane derived from measurement across two plates rather than from one figure.

### Method worth keeping

Longest-straight-run detection on a traced contour does three jobs at once: it finds the members,
it identifies the frame, and it calibrates the page skew from the frame it just found. It replaces
every hand-chosen scan window used in §32–37, all of which had to be guessed and two of which
silently produced artefacts.

## 39. Cycle 31 — fig 7 gives shape but not size, and it is built the other way round

The method from §38 run straight through on Plate I fig 7. It worked, and it produced a negative
result worth more than the fit would have been.

### The assertion earned its place immediately

§37 added `assert paper_median > threshold` before trusting any trace. On the first attempt at
fig 7 it fired: my "stone" sample region had a median of 178 — identical to paper. I had sampled
empty page. Under the old code that would have produced a binarization of noise and a confident
contour of nothing.

With a probe grid to find the actual mass: paper 178 > threshold 163 > stone 149. Component
**18407 px, plate bbox x[1230,1484] y[595,889]**, boundary **1166 points**.

### Fig 7 is built the opposite way from fig 6

| length | angle |
|---|---|
| 103 px | 89.4° |
| 87 px | 1.3° |
| 51 px | 3.4° |
| 43 px | 91.3° |
| 43 px | 88.7° |
| 37 px | 0.0° |
| 36 px | 88.4° |
| 34 px | 5.0° |

**Every long straight run is wall-plane or soffit-plane.** There is exactly one candidate anywhere
in the chamfer band (38.7°) and it is most likely an arc chord.

Fig 6's principal member sits on a **chamfer**. Fig 7 has **no chamfer-plane member at all** — its
mouldings are developed by sinking hollows in the wall- and soffit-planes only. That is Paley's own
sentence made visible: "by sinking hollows in any one of these surfaces, a group of mouldings would
be developed." Two adjacent figures on one sheet, two different generative strategies. For a
generator this is the more useful output than a fitted profile: the chamfer-first and
orthogonal-first families are distinct, and both are on Plate I.

### Fig 7 cannot be scaled

It carries **no dimension line**. The only dark-fraction spike anywhere near it is y=771, and at 5×
that is the figure's stock-box bottom edge — dashed, but with no arrowheads and no label. Since
§33 established that scale here is **per-figure** (fig 5 = 23.75 px/in, fig 6 = 22.09), fig 6's
scale may not be lent to it.

**Fig 7 yields shape but not size.** That is a real limit on the plate, not a gap in the method, and
it likely generalises: the plate carries ~25 figures and dimensions on very few.

### Two method corrections

**The "paper on both sides" test does not work.** I used it to confirm dimension lines — sample the
median 9 px above and below, require paper. Five candidates passed it and all five were hatching:
a 9 px offset lands *between* diagonal hatch strokes and reads as clean paper. The reliable
discriminator is a **sharp isolated spike in dark fraction** matching a known-good line's profile.
Fig 6's line reads 0.378 against a 0.04–0.17 background; fig 7's false candidates sat on a broad
0.16–0.30 plateau with no spike at all.

**Skew belongs to the plate, not the figure.** Fig 7's near-horizontals spread 0.0–5.0° and its
near-verticals 88.4–95.4° — about ±5°, against fig 6's ±1° on the *same sheet*. That spread is real
member variation, not page skew. So: measure skew on the figure with the **tightest** run
distribution and apply it plate-wide. Plate I = +0.1°, from fig 6.

### Attempted and not finished

A plate-wide census of dimension lines, to replace my cycle-22 note that only figs 6, 12 and 19
carry them. The scan returned 19 candidates, nearly all spanning 1400–1550 px — it scans **whole
rows**, so it is finding plate-wide baselines and the border rather than local dimension lines.
It needs local segment detection. Recorded as not done rather than reported.

## 40. Cycle 32 — the arrowhead is the only reliable signature, and five filters that were not

The queue asked for local dimension-line detection to replace §39's whole-row scan. Building it
took five failed designs, each of which failed for a reason worth recording, and produced a
detector that validates on both known lines but does not yet give a trustworthy census.

### What failed, and why

| design | failure |
|---|---|
| whole-row dash scan (§39) | finds plate-wide baselines and the sheet border, which run the full width |
| dash clusters + spike vs neighbouring rows | **154 false positives** — diagonal hatching is *periodic*, so any row-based test resonates with it |
| tall-band paper above **and** below | rejects fig 6's own line: a dimension line sits directly **under** the figure it measures |
| tall-band paper on the **best** side | still rejects fig 6 — on a packed plate there are figures above *and* below |
| cluster first, then check arrowheads | fails where two lines **abut**: figs 5 and 6 share an arrowhead, so a label-gap merge runs one line into the next and the segment starts 62 px before fig 6's own arrowhead |

The pattern across the middle three is the same: **no test based on a dimension line's
surroundings can work on a densely packed plate**, because the surroundings are other figures.

### What works: the arrowhead

Along fig 6's line the dashes have a median vertical thickness of **3 px**, and thick columns
(≥ 2× that) cluster at exactly **x1003–1008** and **x1240–1245** — its two arrowheads. Fig 5's
line gives **x727–732** and **x1003–1008**.

Those middle clusters are the same object: **fig 5's right arrowhead and fig 6's left arrowhead
coincide**, which independently reproduces §33's finding that the two figures abut, by a completely
different route.

So arrowheads drive the segmentation rather than validating it: find every thick cluster in a row,
take consecutive pairs, require dashes and a spike between them.

### And a second discovery: the label is thick too

Paley sets the label *inside* the line, and its glyphs are as thick as an arrowhead — so "11 in"
registers as a false arrowhead and splits fig 6 in two at x≈1108. Chaining segments that share an
endpoint recovers the true extent.

**Validation, both known lines:**

| | detected | known | error |
|---|---|---|---|
| fig 6 "11 in" | y799, x1007–1242 | x1003–1246 | ≤ 8 px |
| fig 5 "1 ft" | y922, x730–1005 | x725–1010 | ≤ 10 px |

### The census is not trustworthy yet

Run plate-wide it returns 63 candidates, and many have spans of **480–700 px** — the chaining
over-merges when features from several figures line up on one row. A real dimension line should
have at most one thick cluster (its label) strictly between its arrowheads; chains of three or more
segments are over-merges.

So the census is **not done**, and my §22 note that only figs 6, 12 and 19 carry dimensions remains
untested. `dimline.py` is in the repo with all five failures documented in its docstrings, because
the next person to try this will otherwise reach for the surroundings test first — it is the
obvious idea, and it cannot work here.

## 41. Cycle 33 — a third scale on one sheet, and an honest precision number

### The regularity filter

A ruled dashed line has evenly spaced dashes; a row that merely happens to cross several figures'
features does not. Measuring the coefficient of variation of the gaps — **with the single largest
gap dropped, because that gap is the label** — separates them. Fig 6 reads CV 0.85 with its label
gap included and **0.43** without; fig 5 reads 0.23. The worst false positives sit at 1.1–2.1.

At a 0.6 cut the census falls from **63 to 36**, and both known lines survive.

### The spot-check, which is the point

Five survivors inspected at full resolution:

| candidate | verdict |
|---|---|
| y=1272, span 698 | **false** — long parallel engraved lines, a figure's flat surfaces |
| y=416, span 331 | **false** — hatched block edges |
| y=1658, span 366 | **false** — figure outlines and hatching |
| y=2120, span 163 | **REAL** — dashed, arrowheads both ends, label **"10 ½ in"** |
| y=799, span 235 | **real** — fig 6's known "11 in" line (the validation case) |

So **precision is about 40%**. The filter chain is good enough to shortlist candidates for the eye,
and not good enough to enumerate. Recorded as a number rather than an impression, because the
temptation with a 36-row output is to treat it as a census.

The false positives are all *regular* — parallel rules and hatch boundaries are as evenly spaced as
a dimension line — so regularity cannot exclude them. The remaining discriminator is requiring an
actual **label** between the arrowheads.

### A third scale on Plate I

The new line's arrowhead clusters sit at **x764–777** and **x1091–1100**, with the label glyphs
"10 ½ in" between them at x934–969. Tip to tip: **336 px for 10½ in → 32.0 px/in**.

| figure | printed | px/in |
|---|---|---|
| fig 6 | "11 in" | 22.09 |
| fig 5 | "1 ft" | 23.75 |
| this one | "10 ½ in" | **32.0** |

**A 45% spread across one sheet.** §33 established that scale on this plate is per-figure from a
7.5% difference between two neighbours; a third figure at 32 px/in makes that finding quantitative
and much harder to dismiss as measurement error. Any attempt to read a size off an unlabelled
figure on this plate could be wrong by nearly half.

### A defect the find diagnosed

The detector returned this line as **two** fragments (x773–936 and x965–1095) rather than one.
Cause: "10 ½ in" is a physically wider label than "11 in", and its glyph clusters exceed the 12 px
gap the chaining allows. The fix is a wider chain tolerance — around 40 px — and it is a real defect
found by looking at the plate rather than by re-reading the code.

## 42. Cycle 34 — the census closed by fan-out: 3 real lines, 7 aliasing artefacts

§41 measured the detector at ~40% precision from a sample of five, and named the fix: require a
**label** between the arrowheads, since the surviving false positives are as *regular* as real
lines. Adding that test, plus widening the chain tolerance from 12 to 40 px (the "10 ½ in" label
is physically wider than "11 in" and was splitting its own line), cut the census from **63 → 36 →
10**, retaining all three known lines and re-joining the split one.

Then, instead of eyeballing a sample as in §41, every one of the ten was inspected independently
and adversarially — one reader per crop, then two more agents per positive, one briefed to refute
and one to re-read the label blind.

### Result: 3 confirmed, 7 rejected, zero refuting votes against the survivors

| plate row | span | label | verifier readings |
|---|---|---|---|
| y=799 | 235 px | **11 in** | "11 in", "11 in" |
| y=922 | 275 px | **1 ft.** | "1 ft.", "1 ft" |
| y=2120 | 322 px | **10 ½ in** | "10½ in", "10½ in" |

The independent readings match the detector's on all three, including the fraction — which is the
one glyph a reader flagged as less than certain, noting a "4" would show an angular apex where this
shows a rounded loop.

### All seven rejects are the same failure

Not seven different problems — one, seven times. Verbatim from the reports: *"an aliasing artifact
of the periodic 45-degree hatch strokes"*; *"the apparent dash pattern is an artifact of sampling
one horizontal scanline across the regular diagonal hatching"*; *"the hatch strokes reading as
evenly spaced dashes"*.

§40 recorded that hatching is periodic and any row-based test resonates with it. This closes that
loop: **every** false positive the pipeline ever produced is that one aliasing effect, plus
coincidental vertical strokes at the ends registering as arrowheads. There is no second failure
mode to hunt.

### Precision, honestly

**30% (3 of 10)** — worse than the 40% estimated in §41 from a sample of five, which is what
sampling five out of thirty-six buys you. The pipeline's real value is that it takes a 1930×2951
plate down to ten candidates a reader can settle in one pass, not that its output is a census.

### The cycle-22 note was wrong

I recorded early on that "dimensions in ft/in [are] on figs 6, 12, 19". Measured: the three lines
sit at y=799 (fig 6), y=922 (**fig 5**), and y=2120 (the fig 19/20 group). Fig 5 carries one and
was never listed. The claim was an impression from a first look, and it survived twelve cycles
because nothing tested it.

## 43. Cycle 35 — PIVOT: building dimensions for a fantasy setting

Ace redirected the lane mid-cycle: *"i need architecture measurements. buildings"*, then *"we are
building for a fantasy setting … cast a wide net, and then focus on the best results"*. The Paley
moulding thread stops at §42, closed.

### First, what fifteen cycles had actually produced

Consolidating every prior dimension manifest into one schema
(`reference_manifests/building_dimensions_v1.tsv`, 686 rows, 88% normalised to metres) exposed a
badly skewed corpus:

| well served | thin |
|---|---|
| roof members 166 | wall thickness 12 |
| room spans 84 | storey height 13 |
| bay spacing 68 | window openings 13 |
| vaults 26 | stairs 13 |
| | **doors 4, towers 6, merlons 0** |

A corpus that knew a tie-beam's scantling to the half-inch and barely knew how wide a door is —
i.e. thinnest on exactly what a game about entering buildings needs.

### The sweep

Fourteen scouts over building families, scored fantasy-value × dimension-richness, then the top
seven deep-mined. 22 agents.

| top | score | | bottom | score |
|---|---|---|---|---|
| tower, hall_manor, stair | 90 | | town_gate | 54 |
| curtain_rampart | 81 | | undercroft, defensive_misc | 45 |
| keep_donjon, town_house, timber_frame | 80 | | monastic | 36 |

**317 new rows** (`reference_manifests/fantasy_building_dimensions_v1.tsv`, 85% metric). Combined
with the consolidated set: **876 metric rows**. The gap closed where it mattered — doors 4→35,
towers 6→26, stairs 13→46, merlon/parapet 0→58, storey height 13→55.

### The standout find: a parametric rule, not a fact

Viollet's **`Meurtrière`** — never mined — carries a complete setting-out algorithm for placing
arrow loops on a round tower: strike the layout arc 2.20 m inside the circumference, divide it into
16, divide the outer circumference into 8, take points 0.30 m from the curtain face. With the loop
itself dimensioned (0.06 m slit, 35° field of fire) and datum'd on a 6.00 m tower with 1.20 m
walls. That is a generator input, not a data point — and it is datum'd on surviving fabric rather
than on Viollet's own restoration, which is why it survives the evidence-class objection below.

`Hourd` (hoarding: merlon heights, 0.30 m putlog sockets, Coucy's 48 corbels at 1.07 m projection)
and `Chemin de ronde` were likewise unmined.

### What the synthesis flagged, which matters more than the rows

- **Viollet contradicts himself across articles.** Coucy donjon: 30.50 m Ø / 55 m (*Donjon*) vs
  31 m / 64 m (*Château*), with a footnote in that same article saying 65 m.
- **One rule is geometrically impossible as stated** — the 13th-c formula gives ~1.00 m of merlon
  above the crenel sill, yet he also says men pass through the crenels "as through doors".
- **Evidence class, corpus-wide:** Carcassonne and Pierrefonds were **Viollet's own restoration
  sites**. Any crenel sill, machicolation walk or hoarding floor measured there is *his* work.
- **Do not average across systems:** jetty 0.28 m (plain) vs 1.65–2.00 m (on potences) are
  different constructions, not a range.
- **"Bay" is not one thing** — Eltham 17 ft (structural principals) vs Crosby Place ≈6.75 ft (roof
  panels). Never mix them in a generator.

### A unit trap that silently halved a harvest

`\d+\s*m\s*[,.]\s*\d+` is required for Viollet's metres — HTML stripping leaves a space before
the comma, so the obvious `\d+\s*m[,.]\d+` matches nothing. This under-reported *Manoir* from 8
hits to 0 on a first pass. And `0,45 c.` means 0.45 m, not 0.45 cm.

### Recommendation carried forward

Start on **round flanking tower**, **English great hall + screens + undercroft**, and **jettied
timber-frame town house** — the three families where the corpus supplies a closed set. Do not start
on forge, mill, cloister or church interior: the first two have no numbers at all.

## 44. Cycle 36 — the crenel formula, found at last; and documenting a trap is not fixing the tool

### The item that had been open since cycle 20 is closed

§27 recorded that crenel and merlon dimensions were "absent from all four fortification articles"
and queued *Créneau* and *Mâchicoulis* as the places to try. They were never tried. They have them,
and `Créneau` states the whole thing in one sentence:

> Les merlons ont 2 mètres de haut sur 1 m , 70 au moins, et 3 m , 30 au plus de largeur sur
> 0,45 c. d'épaisseur ; l'appui des créneaux est à 1 mètre du sol du chemin de ronde, et leur
> largeur est de 0,70 c.

**The complete 13th-century crenellation formula** — merlon 2.00 m high × 1.70–3.30 m wide ×
0.45 m thick, crenel sill 1.00 m above the walk, crenel 0.70 m wide. Plus a general rule for all
periods, archère openings of 0.07–0.08 m with a 0.40–0.45 m splay, and a stated design principle:
*"Les dimensions des crénelages étant données par la taille de l'homme"* — the wall-head is sized
by the human body, which is exactly the property a game needs.

`reference_manifests/fortification_geometry_v1.tsv` — **47 rows** from five articles, each with its
verbatim French quote and an evidence class.

### The trap, verified empirically

`\d+\s*m\s*[,.]\s*\d+` returns **9** hits on *Meurtrière*; the naive `\d+\s*m[,.]\d+`
returns **0**. The space before the comma is an artefact of HTML stripping and it silences the
obvious pattern completely.

### And then the tool failed anyway, for a reason worth recording

Five agents re-read the articles against my extraction. They found **3 errors and 20 omissions**.
Auditing why: Viollet writes numbers in **three** forms, and my extractor matched one.

| form | example | tokens | caught |
|---|---|---|---|
| F1 | `1 m ,60` | 16 | ✅ |
| F2 | `0,70 c.` | 18 | ❌ |
| F3 | `deux mètres`, `un mètre dix-huit centimètres` | 15 | ❌ |

**33% recall.** And F2 is the exact form §43 recorded as a trap — *"`0,45 c.` means 0.45 m, not
0.45 cm"* — which I wrote into the briefing and never added to the pattern. **Documenting a trap is
not the same as fixing the tool.** That is the lesson; it belongs with §32's unverified generator
and §37's threshold reused across scans.

A further wrinkle the verifiers caught: in the same corpus, `0,30 c.` means 0.30 m but the spelled
out *"trois centimètres"* means 0.03 m — real centimetres. The abbreviation and the word do not
mean the same thing.

### The error that mattered

I recorded the arrow-loop layout arc as struck **inside** the tower circumference. It is
**outside** — arc CD is the ground line the loops must cover (*"tous les points de l'arc de cercle
CD sont vus"*). On a 6.00 m tower that is radius 3.00 + 2.20 = **5.20 m**, not 0.80 m. Since this
rule is the best generator input in the whole corpus (§43), the sign error would have been
inherited by everything built from it.

Two smaller corrections: the Coucy gallery figure locates the gallery, not its vault springing;
and my "solive de bois de chêne" quote was a paraphrase — the source says *"pièce"*.

### The geometric contradiction, resolved rather than averaged

§43 flagged that the formula gives a 2.00 m merlon with a 1.00 m sill — a 1.00 m opening — while
Viollet also says men pass through the crenels *"debout … comme par autant de portes"*. Both
statements are in `Créneau`, paragraphs apart.

**Resolution: the 2.00 m is the merlon's height above the crenel sill, not above the wall-walk.**
A merlon is a block standing *on* the parapet, so its stated height is naturally from its own base.
That gives a 2.00 m opening — a man passes upright — and a total wall-head of 3.00 m. It also
reconciles Carcassonne's measured 1.60–1.80 m merlons, which under the other reading would give
0.60–0.80 m openings, impassable; and it explains why `Créneau` says *debout* while `Hourd` says
only *"assez élevés pour permettre à un homme de passer"*.

**For a generator:** walk → crenel sill 1.00 m → merlon top +2.00 m (3.00 m total); merlon
1.70–3.30 m wide, 0.45 m thick; crenel 0.70 m wide; hoarding floor at sill level, projecting
≤ 1.95 m on 0.30 m oak in 0.30 m square putlogs.

## 45. Cycle 37 — the re-harvest: 25% new, not the two-thirds I claimed

§44 measured my extractor at 33% recall and concluded that "roughly two thirds of the numbers in
those articles are still on the page". That inference was wrong, and measuring it is this cycle's
first result.

### The extractor

`reference_manifests/vldnum.py` — all four forms Viollet uses: `1 m ,60`, `0,70 c.`, spelled-out
(*deux mètres*, *un mètre dix-huit centimètres*), and digit-plus-word (*46 mètres*, *5 pieds*),
with overlap resolution. On the five cycle-36 articles it finds **53** tokens where the old
extractor found **20**.

### But the corpus was not two-thirds empty

Re-harvesting the nine articles mined before cycle 36 and comparing every extracted value against
what the manifests already hold:

| article | tokens | already known | **new** |
|---|---|---|---|
| Architecture militaire | 56 | 26 | **29** |
| Porte | 55 | 20 | **19** |
| Escalier | 40 | 39 | 14 |
| Serrurerie | 26 | 7 | 14 |
| Tour | 57 | 34 | 6 |
| Charpente | 47 | 48 | 3 |
| Construction — Voûtes | 22 | 21 | 2 |
| Donjon | 31 | 20 | 1 |
| Maison | 13 | 18 | 0 |

**88 of 347 tokens are new — 25%, not 67%.**

The reason is worth stating: those manifests were built by *agents reading the text*, not by my
regex. They read `0,70 c.` and *deux mètres* perfectly well. My regex was the weak link; corpus
coverage was not. **A tool's recall is not the corpus's recall**, and I conflated them.

(One caution on that table: the first pass showed *Construction — Voûtes* at 22 new because my
join matched article names by string and the manifest spells it "Voutes" without the circumflex.
Normalising accents dropped it to 2. A comparison is a tool too.)

### What the 88 actually are

Reading them, they fall into three kinds, and only the first is what I was after:

1. **Real building dimensions** — 17 rows, now in `viollet_reharvest_v1.tsv`. The best of them
   fill the corpus's thinnest category: *Porte* gives **pedestrian doorways at 1.00–1.50 m wide ×
   2.50–3.00 m high**, interior draught-lobbies at 1.95–2.27 m, nook-shafts 0.16–0.33 m, and the
   rationale — *"les personnes passant par ces portes n'avaient pas une taille qui atteignit six
   pieds"*. Doors had **four** rows in the whole corpus before the cycle-35 sweep.
2. **Drawing scales masquerading as dimensions** — *"à l'échelle de 0 m ,002 pour mètre"*,
   *"1 centimètre pour 15 mètres"*. Syntactically identical to a measurement. Noted in `vldnum.py`;
   filter on "à l'échelle de" and "pour mètre".
3. **Ballistics** — cannon lengths, bomb calibres, ranges of 120–175 m, a trebuchet arm of
   10.30 m. Not architecture, though a siege might want them.

### A new unit trap: the pied is not one unit

In *Architecture militaire* Viollet quotes Scala's bastion profile in feet and notes, in a single
parenthesis, *"(Scala parle ici de pieds romains 0,297896.)"* — **Roman** feet, not the pied de roi
of 0.3248 m. Converting that passage with the French foot is an **8.4% error** across six
dimensions, and nothing else in the text flags it. The same article quotes De Ville in *pas*, which
he defines as *"cinq pieds de roy"*.

Recorded in `vldnum.py`: when a passage quotes a foreign author, look for a stated foot before
converting.

## 46. Cycle 38 — Gwilt's stair rules, and two rules on one page that contradict each other

First mining of **Gwilt, *Encyclopaedia of Architecture*** (IA `encyclopaediaofa00gwil`).
**27 rows** → `reference_manifests/stair_rules_v1.tsv`. Stairs scored 90 in the §43 sweep and this
is the source that was named as richest; it delivers.

### Newland's pairing table, verified on the page image

Printed page 668 carries a two-column table that OCR mangles into `9 8^ 8 H 7 6 H 5 ■ih`.
Inspected at full resolution it is unambiguous:

| tread (in) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|
| riser (in) | 9 | 8½ | 8 | 7½ | 7 | 6½ | 6 | 5½ | 5 | 4½ |

Tread ascends by an inch as the riser descends by a half — a pitch range of **60.9° down to
17.8°**.

### The contradiction

The **same page**, four paragraphs earlier, gives a different rule: *"the convenient rise of a step
12 inches in width is 5½ inches … Thus 12 × 5½ = 66, which would be a constant numerator for the
proportion."* A constant tread × riser product of 66.

Newland's table does not obey it:

| tread | 5 | 8 | 11 | 12 | 14 |
|---|---|---|---|---|---|
| T × R | 45 | 60 | **66** | **66** | 63 |
| 66/T demands riser | 13.2 | 8.25 | 6.0 | 5.5 | 4.71 |
| error against the table | **47%** | 10% | 0% | 0% | 5% |

The product runs 45 → 66 → 63, hitting 66 only at treads 11–12 — precisely where Gwilt derived it.
At a 5 in tread the constant-product rule demands a **13.2 in riser and a 69° pitch** against the
table's 9 in. **Two incompatible rules, one page.** Use the table; the product rule is a local
approximation around its own datum, and Gwilt does not say so.

### Palladio, and a second unit trap of the kind found in §45

Paragraphs 2804–2805 give Palladio's stair rules — and they are in **Vicentine** units. Gwilt flags
it twice, once in a footnote (*"The Vicentine foot is about 13.6 inches English"*) and once in a
parenthesis (*"These are Vicentine inches"*), then quietly converts one figure himself: 4 Vicentine
feet = "4 feet 6 inches English".

Vicentine foot = **0.3454 m**; Vicentine inch = **28.8 mm**. So Palladio's riser range of 4–6
inches is **115–173 mm**, not the 102–152 mm it reads as in English inches — a **13% error**.

§45 found the same shape of trap with Scala's Roman foot in *Architecture militaire*. Two sources,
two foreign feet, each flagged in a single aside. **Where a source quotes a foreign author, assume
a foreign foot until the text says otherwise.**

Palladio also gives what the corpus needed: staircase **never narrower than 4 Vicentine ft (1.38 m)
so two people can pass**, plan **twice as long as broad**, tread 1–1½ Vicentine ft (0.345–0.518 m),
**11–13 steps to a flight before a half-pace**, an **odd** step count so the ascent ends on the
starting foot, and headway defined as clearing *"the tallest person … with his hat on"*. Gwilt's own
independent figure agrees closely: not less than **4 English feet** for two to pass.

### Two tooling traps, both found by cropping the wrong page

**The OCR leaf index is not the image index, and the offset drifts.** `encyclopaediaofa00gwil` has
1478 pageindex entries against an imagecount of 1474, and the byte slices straddle printed-page
boundaries, so a running head appears mid-slice. The slice containing "668 THEORY OF ARCHITECTURE"
is leaf **692**, but printed page 668 is image **n689** — offset 3; while the slice containing
"671" is leaf 696 against image n692 — offset **4**. It cannot be computed from
`len(pageindex) - imagecount`. Recipe now recorded in `iabook.py`: use `find()` for a
neighbourhood, then fetch two or three images and read the printed number off the page.

**And `CACHE` defaulted to the module's own directory**, so 5.8 MB of Gwilt scans downloaded
straight into `reference_manifests/`. Now honours `IABOOK_CACHE`.

## 47. Cycle 39 — the English halls: 351 rows, 23 triples, and 72 "errors" that were my own bug

Five books mined in parallel, each haul independently re-read.
**351 rows** → `reference_manifests/english_hall_dimensions_v1.tsv`, 76% normalised to metres,
28 flagged `ocr-uncertain`.

| book | rows |
|---|---|
| Turner/Parker vol 2 | 104 |
| Turner vol 1 | 74 |
| Garner & Stratton vol 1 | 63 |
| Turner/Parker vol 4 | 60 |
| Gotch | 50 |

**23 buildings with a full length × width × height triple** — the thing no other source in the
corpus supplies. Hampton Court great hall 32.3 × 12.2 × 12.5 m; New Hall Boreham 29.3 × 15.2 ×
12.2; Great Chalfield 12.2 × 6.1 × 6.1. Manor-house great halls cluster at **length 29–55 ft,
width 19–26 ft, aspect ratio 1.4:1 to 2.3:1**.

### The sill rule, which is worth more than its size suggests

I asked the miners to chase any rule about how high hall windows sit, because it decides whether a
thief can reach one. Garner & Stratton has it, and it is **two regimes in the same room**:

> the largest and most important occur at the dais or upper end of a great hall … whereas generally
> the other hall windows were high in the walls, the oriel or bay by means of its lowered sill threw
> light direct on to the buffet and the high table

Corroborated at Loseley: *"The floor of the oriel is raised, and the sill of the window brought
down; otherwise the windows in both side walls are high above the floor, as was usual."*

So: **ordinary hall side-window sills 7–8 ft above the floor** (Hoghton Tower 7 ft measured;
Parnham's former west windows *"only reached to about 8 feet from the floor"*) — unreachable without
furniture — while the **dais oriel/bay sill comes down near floor level** and is effectively a
walk-through. One hall, two very different affordances, and the difference is documented rather than
inferred.

### Wall thickness: one rule, and one clean absence

Gotch gives the Norman keep figure — *"seldom less than 8 ft, and sometimes as much as 16 or 20 ft"*
— with keep plans to match (White Tower 118 × 107 ft, Dover 90 ft square, Rochester 70 ft square,
Kenilworth 87 × 54, Peak 40 × 36).

And Garner & Stratton vol 1 contains **no numeric wall thickness at all**. The miner searched every
leaf for a number within 80 characters of "thick"/"thickness" and got brick sizes back. Walls are
called *"very thick"* repeatedly and stairs run *"in the thickness of the wall"*, but not one figure.
Recorded as a negative result so nobody mines it again for that.

### The 72 flagged errors were mine

The verify stage reported 72 errors — 27 provenance, 40 leaf/page references, 5 value-level. Almost
all said the same thing: *"these rows are not in this book at all."*

They were right, and the cause was my harness. To give each verifier its book I wrote
`BOOKS.find(x => x.slug === r.book)`, but `r.book` is the miner's free-text description
("Gotch, The Growth of the English House (IA growthofenglishh00gotc …)"), which never equals a slug.
So `.find()` returned undefined, `|| BOOKS[0]` fired, and **every verifier was handed Garner &
Stratton as its context** and correctly concluded the rows did not belong to it.

Checked directly: each miner's quotes are present in the book that miner named, all five. The
provenance was never wrong.

This is §19's lesson recurring — *a verifier's refutation is itself a claim* — with a new wrinkle:
here the verifier was not mistaken, it was **misinformed by the orchestration**. A fallback default
(`|| BOOKS[0]`) silently substituted wrong context rather than failing. It should have thrown.

Two genuine data faults did surface and are now marked `ocr-uncertain`: a mangled vulgar fraction
(`3^` for 3½ ft) and a spelled-out "twenty-four" whose unit has to be borrowed from the preceding
clause.

### And a caveat on my own triple extraction

The 23 triples come from regex-bucketing `element` text, and it mis-fires: Hoghton Tower shows a
2.1 m "great hall height" (that is the 7 ft **window sill**) and Belsay a 0.6 m width. The
underlying rows are sound — each carries its quote — but the L/W/H roll-up needs element names
normalised before it can be trusted. Flagged, not fixed.

### iabook alignment rule, now testable

§46 found the OCR leaf index diverging from the image index by a drifting amount. Pinned down:
**when `len(pageindex) == imagecount`, leaf N is image N and can be trusted** — verified on
`gri_33125010766489` (364 == 364; OCR leaf 125 holds printed p.108, which is what image n125 shows).
When they differ, the offset drifts and the page must be identified by eye. A separate harmless
quirk either way: a slice leads in with the tail of the previous page, so a mid-word start is **not**
evidence of misalignment — that was my first, wrong reading this cycle.

## 48. Cycle 40 — the controlled vocabulary, and two parser bugs a bad aspect ratio exposed

### The problem in a number

Six manifests, **1445 rows, 1039 distinct `element` strings, 51% of them occurring exactly once**.
Nothing can be queried across that, and §47 showed a naive roll-up actively lying: Hoghton Tower's
7 ft window sill read as a 2.1 m great-hall height.

### The vocabulary

`reference_manifests/element_vocab.py` — **41 canonical classes**, applied as a **mapping table**,
not a rewrite. The source wording stays: it is evidence, and several rows carry their qualification
inside it (*"wall thickness at the point a workman pierced it in 27 days"*). Rules are ordered, first
match wins, so `sill_height_above_floor` beats `height` and `wall_thickness` beats `thickness`.

Coverage improved across four passes as I read what was actually failing rather than guessing:
**82.6% → 84.6% → 89.3% → 91.1%**.

`building_dimensions_canonical_v1.tsv` — 1445 rows with the canonical column alongside the original.

Three classes emerged from the unmapped tail that I would not have thought to define:

* **`level_difference`** (drops, *relief of*, *command over*, level difference between two curtain
  walks) — a relationship between two parts, not a dimension of either. For a game about climbing and
  falling this is arguably the most directly useful class in the table.
* **`member_detail`** (42 rows) — Dollman and Brandon dimension every board, jamb, boss and baluster
  separately (*"barge board of Gable A, second figure"*). Wanted for detailing, not massing, and
  keeping it named stops it looking like a mapping gap.
* **`unresolved_in_source`** (7 rows) — where the source itself says the reading is unresolved. That
  is data about the source, not a failure of the table.

The residual **128 unmapped** are genuine one-offs — garrison capacities, artillery ranges, *"section
dimension at A.A"*. Going further would mean inventing a class per row.

### Then a 21.49:1 room found two parser bugs

The first strict roll-up gave Belsay Castle's hall an aspect ratio of **21.49:1** — a 13.1 m room
0.6 m wide. Tracing it:

1. **OCR-mangled fractions with a bracketed expansion.** The value read `21^ ft. (21 1/2 ft.)`. The
   `^` defeated every pattern, so the prose fallback fired and returned **0.61 m — exactly 2 ft**. A
   plausible-looking number, which is the worst kind of wrong. `56J feet (56 1/2 ft.)` did the same.
   Now the bracketed expansion is preferred: 6.553 m and 17.221 m.
2. **Spelled-out units.** `"47 feet 3 inches"` returned 14.326 m — **the inches were silently
   dropped**, because the feet-and-inches pattern only accepted `ft`/`in`, never `feet`/`inches`.
   Now 14.402 m.

Worth noting how the second one nearly escaped: my first patch to it *did not apply*, because the
string I was replacing had already been edited in §44 and no longer matched. The `.replace()` silently
did nothing and the test still failed — `grep` for the new text is what caught it. **A no-op edit
looks exactly like a wrong fix.**

### Triples: 27 loose became 12 strict

`derive_triples.py` now uses **`internal_*` only**. The generic `length`/`width`/`height` classes are
fallbacks for rows whose wording never said what they measured, and feeding them a triple produces
nonsense — Westminster Hall came out 17.0 × 21.0 m because `length` had matched *"length of
arbalétriers and chevrons, tenons included"*, a roof member.

| | before | after |
|---|---|---|
| triples | 27 | **12** |
| Hoghton Tower | 16.5 × 7.9 × **2.1** | no triple (correct — only L and W exist) |
| Belsay hall | 13.1 × **0.6** × 5.2 | 13.1 × **6.6** × 5.2, 2.00:1 |
| Westminster Hall | **17.0** × 21.0 × 11.5 | dropped (correct) |

Fewer and trustworthy beats more and wrong. The twelve: Padua 73.2 × 24.4 × 24.4; St Martin's
refectory 30.5 × 8.2 × 7.9; Hampton Court Great Watching Chamber 21.6 × 8.8 × 7.6; Guesten Hall
20.0 × 10.6 × 11.2; Penshurst 19.5 × 11.8 × 14.6; Horham 14.0 × 7.3 × 7.6; Belsay 13.1 × 6.6 × 5.2;
Haddon parlour 9.9 × 4.6 × 3.2; South Wraxall 9.8 × 6.1 × 6.1; Borwick 9.1 × 7.2 × 3.5; Wilderhope
8.8 × 6.1 × 4.2; Brenchley 5.8 × 5.0 × 2.8.

**Aspect ratio: median 1.90, range 1.15–3.70. Heights 2.8–24.4 m.**

And the §47 failure is now structurally impossible rather than merely fixed: `sill_height_above_floor`
is its own class with 18 rows, so a sill can no longer be mistaken for a ceiling.

## 49. Cycle 41 — the audit: older manifests clean, newer ones not, and a self-inflicted mess

**Paused mid-cycle at Ace's request.** Recorded at the point everything is consistent again.

### The audit came back negative, which was the point

Swept every manifest for the two §48 bug signatures — OCR-mangled fraction glyphs next to a numeral,
and spelled-out `feet`/`inches`:

| manifest | mangled | spelled |
|---|---|---|
| domestic, fortification, viollet, buttress, timber_roof | **0** | **0** |
| english_hall | 3 | 3 |
| fantasy_building | 1 | 0 |

**The older manifests are clean.** The §48 hypothesis — that they were "the same vintage and never
re-parsed" — was reasonable and wrong: they were built by agents who normalised values as they
read, so they never went through the fragile string path at all. Only my own later agent-mined
tables were affected.

### Then the second half found three more of the same species

Auditing *how* each metre value was obtained showed **43% of the fantasy table** resting on the
prose fallback — "grab the first number". Inspecting twenty of them found three distinct defects,
all returning plausible numbers:

1. **Pattern order beat position.** The fallback tried ft-and-inches first, then ft-only, so
   `43 ft x 21 ft 6 in x nearly 17 ft high` returned **21 ft 6 in** — the first ft+in *pair* — not
   the leading 43 ft. Four rows, including `about 101 ft x 36 ft 3 in` → 36.25 instead of 101.
   Fixed: collect every candidate with its position, take the **leftmost**, tie-break on **longest**.
   That tie-break was the subtle half — `40 ft 3 in` and `40 ft` both start at 0, and sorting on
   value silently preferred the shorter, dropping the inches.
2. **A leading bare number sharing the trailing unit.** `100 x 27 ft` means 100 by 27, but only
   "27 ft" carried a unit, so **twelve rows recorded their width as their length**. Fixed by
   distributing the unit leftwards.
3. **The "= X" rule firing on a unit definition.** `6 to 7 pieds = 1.95 to 2.28 (at Viollet's pied
   = 0.325)` returned **0.325** — the size of the foot. Fixed by requiring nothing numeric before
   the `=`.

Nineteen metre values changed, several by +100% to +179%.

### And I broke the file twice doing it

Both worth recording, because both are process failures rather than analysis ones.

**A `.replace()` that silently did nothing.** §48 already recorded "a no-op edit looks exactly like a
wrong fix" — and I hit it again this cycle, twice, because the string I was patching contained a
literal `[x\u00d7]` that my search string wrote as `[x×]`. The test kept failing and I kept
re-patching the same non-match.

**Then an excision that cut too far.** Replacing the block from `def _first_measure` onward removed
`_to_m_core`, `rows()` and `__main__` along with it — the file only defined four names afterwards
and every downstream call raised `NameError`. A second attempt at a surgical excision left an
unterminated docstring.

The fix in both cases was to stop patching and **rewrite the file whole**, which took one pass and
now carries a documented parsing history plus a 20-case regression suite: **19/20**. The survivor is
`6 to 7 pieds = …` returning 6.0, because "pieds" is not in the unit table — one row, documented in
the module rather than left to be rediscovered.

I also **reverted** one of my own fixes: a "leading bare number + unit column" rule meant to save
`20 (approx.), taken 5 m above ground` (a 20 m diameter measured *at* 5 m). Its greedy number kept
backtracking into partial ones — `43 ft` → `4` — and it cost more rows than it saved. Left as a
known one-row defect with the reasoning in the module. **Reverting a fix that is losing is cheaper
than defending it.**

### State on pause

All tables regenerated and idempotent: `building_dimensions_v1` 608/686 metric,
`english_hall_dimensions_v1` 268/351, `fantasy_building_dimensions_v1` 269/317, canonical table
1445 rows at 91.1% mapped, **12 strict internal L/W/H triples** unchanged.

## 50. Cycle 42 — the Viollet sweep: a parametric window rule, and bridges from zero

The queue's top open item: 524 of 533 Viollet articles had never met the four-form extractor
(`vldnum.py`, §45). Swept sixteen chosen by **game value rather than figure count** — a capital's
mouldings do not help a player climb anything; a kitchen, cellar, barn or bridge does.

**180 numeric tokens across sixteen articles never previously extracted.**
`reference_manifests/viollet_window_bridge_v1.tsv` — **35 rows** from the two richest.

### A clean negative, and it is the annoying one

`Grange`, `Moulin`, `Cave` and `Comble` — barn, mill, cellar, roof — carry **no dimensions at
all**. `Cave` is a 1,595-character stub and `Comble` is 207 characters. These are exactly the
vernacular building types a fantasy setting wants most, and Viollet is descriptive rather than
dimensional on every one of them. Recorded so nobody sweeps them again expecting numbers.
`Ferme` returned empty entirely — likely a title mismatch rather than an absent article.

### Windows: a parametric subdivision rule, not a table of sizes

The corpus held thirteen window rows before this. `Fenêtre` supplies something better than more
of them — **the rule that generates them**:

> il ne fallait pas laisser plus d'un mètre environ de vide entre les meneaux

Maximum **1.00 m of clear glass between mullions**, because beyond that the glazing needs iron
stanchions. And the resulting progression is stated explicitly as a recursion:

| window width | mullions |
|---|---|
| 2 m | 1 |
| 4 m | 1 principal + 2 secondary |
| 8 m | 1 principal + 2 secondary + 4 tertiary |

That is a **generator input of the same class as the arrow-loop rule in §44** — it produces
tracery at any width instead of describing one window. Reims corroborates it from the other end:
its lights measure **1.20 m or 2.30 m**, and where secondary mullions divide them the glazed space
comes back to **1.00 m**.

A second stated proportion, from the Carcassonne château window: total width between the embrasure
jambs **1.20 m**, embrasure depth **0.60 m** — *"moitié de la largeur"*, half the width, said
outright rather than measured off. (Carcassonne being Viollet's own restoration, it is classed
`documented-reconstruction`.)

And an operability limit worth having for interiors: a casement of **3–4 m** height is
*"assez incommode"* to open — which is why transoms exist.

### Bridges: from zero rows to twenty-one

`Pont` had never been touched. Three bridges arrive fully dimensioned:

* **Saint-Bénezet, Avignon** (1178–88) — 900 m long, deck **4.90 m** including parapets, 18 arches
  of **20–25 m** span, piers **30 m** end to end, voussoirs in four courses of **0.70 m**. Its
  chapel floor sits **4.50 m below the deck**, and past the chapel the deck **pinches to 2.00 m**.
* **Albi**, in brick, c.1335 — 250.50 m between abutments, deck **18 m above mean water**, seven
  arches of 22 m, six piers **8.55 m** thick, bricks **5 × 40 × 28 cm**.
* **Pont Saint-Esprit** (1265) — deck 5 m, 22 arches, ~1000 m.

Plus two timber recipes: piles at **12.00 m axis to axis** with heads no more than **2.00 m** above
water, and Villard de Honnecourt's note that **20-foot timbers yield a 50-foot rigid deck** — a
span-from-stock rule, not a measurement.

The single most game-useful line in the sweep is not a dimension at all. The Pont Notre-Dame of
1414 carried **sixty houses**, *"60 maisons esgales en structure"*, on a deck 18 *pas* wide. A
bridge that is also a street of identical houses is a level, and the 2.00 m pinch-point at
Avignon's chapel is a chokepoint with a source.

## 51. Cycle 43 — castles and bell towers, and the Coucy conflict traced to its source

Wrote up the articles §50 extracted but left in the scratchpad. `Château` and `Clocher` yielded
**54 rows** → `reference_manifests/viollet_castle_tower_v1.tsv`, 51 of them `measured`.

### The motte-and-bailey, dimensioned end to end

The single most reusable castle layout in the corpus, and now complete enough to generate:

| element | value |
|---|---|
| bailey parallelogram | **150 m × 90–110 m** |
| motte diameter | **27 m** |
| motte ditch width | 10–15 m |
| vallum | 2 m high × 10 m wide |
| covered way outside the defences | 2 m wide |
| gate threshold above the counterscarp | 2 m |

A later refinement worth having for wall geometry: the elliptical enceinte is *"une suite de
segments de cercle de trois mètres de corde environ, séparés par des portions de courtine d'un
mètre"* — **3 m arc segments alternating with 1 m straight curtain**. That is a repeat pitch, not
a description.

### The Coucy conflict, traced

§43 flagged Coucy's donjon as 55 m (*Donjon*) vs 64 m (*Château*) with a footnote saying 65 m, and
queued it as needing "a decision recorded, not an average". `Château` now supplies **both of its
own readings verbatim**:

> le donjon qui porte trente-un mètres de diamètre hors œuvre sur **soixante-quatre mètres** depuis
> le fond du fossé jusqu'au couronnement

> son diamètre étant de trente-un mètres et sa hauteur de **soixante-cinq** environ

The two differ by their **datum**, and the first says so: 64 m is measured *from the bottom of the
ditch*. The 65 m footnote states no datum. So these are not two measurements of one thing — they
are one measurement and one loose restatement. **Recommended reading: 31 m diameter × 64 m from
ditch bottom**, with the ditch depth as the variable that produces the other figures. The 55 m in
*Donjon* remains unreconciled and still needs its own datum checked.

**And the Louvre conflict resolves outright.** `Château` states *"le donjon du Louvre n'avait que
vingt mètres de diamètre environ sur trente mètres de haut"* — 20 m × **30 m**, against the ~40 m
in *Donjon*. Same shape of problem, and the sentence is a correction: Viollet is explicitly calling
Guillaume de Lorris exaggerated at that point.

### Castles, the rest

Rectangular XIIIc plan **47.50 × 39.00 m in-works** with a **25 × 30 m** courtyard; curtain
**2.70 m** thick; a gate-guarding donjon tower with **4.60 m** walls and *no ground-floor openings
at all*; a postern sill **8 m** above the wall base; lices **8 m below** the inner courtyard;
Château-Gaillard's rock-cut ditch **10 m wide × 7–8 m deep** with a **0.60 m ledge** on the
counterscarp — the ledge being exactly how Philip Augustus's soldiers got up it.

### Bell towers: a parametric setback

`Clocher` is the climbing-geometry article, and it gives the rule that generates a tower rather
than one tower's numbers:

> Chaque étage se retraite de **0,08 c.** à l'intérieur

**Each stage sets back 0.08 m internally** — so wall thickness, floor area and the ledge at every
level all fall out of stage count. Corroborated by the taper stated elsewhere in the same article:
a tall romanesque clocher runs **0.50 m at the base to 0.30 m at the summit**.

Heights now span the full range: Mollèges **2.06 m** square at base (the smallest recorded),
Carolingian bases **5–8 m** square, Bocherville **11.00 m** wide with a 4.00 m belfry stage and a
**27.00 m timber spire**, up to **80 m** overall and Chartres' clocher vieux at **103.50 m** to the
foot of the iron cross, its spire base **10.20 m** across in-works. Octagon walls can be as thin as
**0.80 m**, and a stone spire shell **0.25 m**.

For interiors: one clocher's ground vault sits **24.75 m above the church pavement**, and above it
the shaft runs *"d'une seule venue, sans voûtes ni planchers"* — a 100 m tower with a single
unfloored void inside it.

## 52. Cycle 44 — a printed vertical budget, and a cloister that leans on purpose

The four articles §50 extracted and §51 did not write up. **34 rows** →
`reference_manifests/viollet_interior_v1.tsv`. One of them is the best single find in the lane.

### `Manoir` prints a vertical stack-up, and it balances

Viollet sets out the storey heights of a manor as a **summing table**, in two columns:

| mezzanine side | | hall side | |
|---|--:|---|--:|
| entresolled storey | 2.30 | grande salle, floor to ceiling | 4.30 |
| floor construction | 0.30 | beams and corbels | 0.60 |
| entresol | 2.30 | joisting | 0.30 |
| floor construction | 0.30 | | |
| **total** | **5.20** | **total** | **5.20** |

Checked arithmetically: 2.30 + 0.30 + 2.30 + 0.30 = **5.20**, and 4.30 + 0.60 + 0.30 = **5.20**.

**A single-height great hall and a two-level entresolled block arrive at the same datum** — the
chemin de ronde above. That is the rule for stacking a mixed-use building, printed rather than
inferred, and the corpus has had nothing like it. Every earlier storey-height row was one number
for one room; this is a *budget* showing how two different internal subdivisions reconcile.

The ground floor is capped separately: *"n'a pas plus de 2 m,65"* sol to plafond.

### `Cloître` — the arcade leans on purpose

The cloister walk is fully dimensioned — dwarf wall (bahut) **0.45 m** above the pavement forming
a continuous bench, piers **0.50 m face × 1.50 m thick**, marble colonnettes only **0.11 m** in
diameter, and an upper wall pierced with roses at **0.35 m**, which Viollet says outright is
*"réellement qu'une cloison évidée"* — a screen, not a load wall.

But the detail worth keeping is a deliberate imperfection:

> la colonnette C sera posée verticale, tandis que la colonnette D sera posée inclinée de
> **0,02 c. ou 0,03 c.**

The **outer** colonnette of each pair is set leaning 20–30 mm, and the base centres sit
0.01–0.03 m wider apart than the capital astragals above them. A generator that stands both
colonnettes plumb is not simplifying — it is building something the masons deliberately did not.

### Fireplaces and floors

`Cheminée` gives the range: a primitive mantel is **0.20 m** thick; the Poitiers grand'salle
fireplace is **10.00 m wide × 2.30 m under the mantel** in a hall **16.30 m** wide in-works; a
sculpted hôtel chimney is **1.66 m under the mantel × 2.57 m** wide. And the fuel scales with it —
logs *"de deux ou trois mètres de long"*.

`Plafond` supplies the floor-framing threshold: below a **2–3 m** span, simple joisting on a wall
corbel suffices and nothing more is needed. Above that, a XVc example covers **15 × 6.50 m in five
bays with six beams**. The brick-vaulted variant uses voutains of **3 × 10 cm** brick on joists of
**0.32 m square set diagonally** — *"placées sur la diagonale, elles offrent une grande roideur"*,
a stiffness trick that changes the silhouette of every exposed ceiling.

## 53. Cycle 45 — the barn was there all along, and a numeral bug worth 44%

Swept sixteen more articles. **42 rows** → `reference_manifests/viollet_vernacular_v1.tsv`. The
cycle turned on two corrections, both to my own earlier work.

### §50's barn negative was wrong

§50 recorded that *"Grange, Moulin, Cave and Comble carry no dimensions at all"* and told future
cycles not to look again. `Grange` genuinely has none. But **`Ferme (Constructions rurales)` does**,
and I never fetched it — because `Ferme` is a **disambiguated title**, splitting into
`Ferme (Constructions rurales)` and `Ferme (Terme de charpenterie)`. §50 saw an empty return, wrote
"likely a title mismatch", and moved on without checking the article list. One line would have
caught it.

The contract it quotes is a complete barn specification:

| element | value |
|---|---|
| court (pourpris) | **40 × 30 toises** = 77.95 × 58.46 m |
| enclosure wall, excluding coping | **18 pieds** = 5.85 m |
| barn proper, minimum | **20 × 9 toises** = 38.98 × 17.54 m |
| eaves height | **12 pieds** = 3.90 m |
| lean-to dwelling beside the gate | 10–12 toises |

**And I checked the other five EMPTY returns properly this time**: `Poterne`, `Pont-levis`,
`Souterrain`, `Écurie` and `Cellier` **do not exist as articles at all** — those subjects live
inside `Porte`, `Château` and `Architecture militaire`. That is a different failure from a
disambiguated title, and §50 conflated the two.

Checking the list also surfaced what a bare `Hôtel` lookup had hidden: **`Hôtel-de-Ville`,
`Hôtellerie` and `Hôtel-Dieu`** — town hall, inn, hospital. `Hôtel` itself is a 1,034-character stub.

### A numeral bug that halved a wall

`dix-huit pieds` came out of the extractor as **3.25 m**. It should be 5.85. `_word()` split on the
hyphen, took the first token, and returned 10 instead of 18 — a **44% error, in a plausible range
for a wall height**, which is exactly why nothing flagged it. `quatre-vingt` was returning 4 rather
than 80.

Fixed for the additive teens (17/18/19), the vigesimal eighties and nineties, and *soixante-dix*.
Twelve-case check passes. The forms that already worked — `vingt-deux`, `cent quinze` — still do.

### What the sweep found

**`Puits` is the vertical-shaft article.** Wells run from **3 pieds (0.97 m)** to **2–3 toises**
in diameter; one is **2.57 m across and 30.20 m deep** with a water column reaching 6.30 m. The
margelle is a **1.00 m kerb, 0.22 m thick**. And at Château-Landon there is a **1.05 m well shaft
deliberately arranged to serve several storeys** — a vertical connection between floors, which is
a route.

**`Hôtel-Dieu` holds the largest interior in the corpus**: a ward **18.60 m wide in-works × 88.00 m
long**, roofed with **one-piece oak tie-beams of 21.40 m** and principal rafters of 19.00 m. Cells
partition it at **2 toises (3.95 m)** intervals, and the worked example leaves **6 m of circulation**
outside the cells in a 10 m ward.

**Refectories** give two clean volumes: Saint-Germain-des-Prés **40 × 10 m with vault keys at 16 m
and no central column line**; Poissy **47 × 12 m, keys at 20 m**.

Three smaller finds that are pure level geometry: a Saint-Antonin **shopfront 7 m wide** where
period ground storeys ran only 3–4 m; a **latrine shaft with no floors at all** from ground to roof;
and a belfry frame whose top **sways 0.05 m** when the great bell swings — *"à peine sensible"* at
the gallery below.

## 54. Cycle 46 — the audit that came back clean, and why

Took queue item **2** ahead of item 1 and should say why: §53's numeral bug was known to have
corrupted rows already written, and this lane's own precedent (§49) is that correctness of existing
data outranks new acquisition. A 44% error inside a plausible range does not announce itself.

### The blast radius

Re-extracted all **45 swept articles** and diffed every spelled-out numeral against the pre-fix
parser. **Four tokens change**, all of them `dix-huit`:

| article | token | buggy | correct | error |
|---|---|--:|--:|--:|
| Château | dix-huit mètres | 10.00 | **18.00** | 44% |
| Construction — Voûtes | dix-huit pouces | 0.27 | **0.49** | 44% |
| Créneau | dix-huit centimètres | 0.10 | **1.18** | in a compound |
| Ferme (Constructions rurales) | dix-huit pieds | 3.25 | **5.85** | 44% |

Not a trivial set. The first is **Coucy's flanking-tower diameter**; the third is the Béziers
crenellation height; the fourth is the barn contract's enclosure wall.

### Every one of them is correct in the manifests

Checked row by row rather than from memory:

| where | recorded | what the buggy extractor would have given |
|---|---|---|
| `viollet_castle_tower_v1` Coucy tower diameter | **18** | 10 |
| `fantasy_building_dimensions_v1` Béziers cornice | **1.18 m** | 0.10 |
| `viollet_vernacular_v1` barn enclosure wall | **18 pieds (5.85 m)** | 3.25 |
| `building_dimensions_v1` rib voussoir, large vaults | **0.50 m** | 0.27 |

**Zero corrupted rows.** And the reason is not luck — it is the **"no quote, no row"** rule the
lane adopted defensively in §36. Because every row carries its verbatim French, the number was
transcribed from *"dix-huit mètres"* each time, and the extractor's computed metres were never what
got written. The extractor was wrong four times and it never reached a manifest.

That is worth stating plainly, because it is the first time a discipline this lane adopted as
overhead has demonstrably prevented a real error. The quote is not provenance decoration. It is the
check.

### A regression suite, added late

`vldnum.py` now carries a **35-case French numeral suite** — un…seize, the additive teens, the
vigesimal eighties and nineties, *soixante-dix*, *soixante-quinze*, and the additive hundreds —
with a note to run it after any change to `_word()`. All 35 pass.

Two numeral bugs shipped before anything noticed, and neither crashed. Both returned a number in a
believable range, which is the only failure mode that matters here.

## 55. Cycle 47 — the sweep hits diminishing returns, measured

### The list check paid for itself immediately

§53's rule applied before fetching anything: of 26 candidate titles, **9 do not exist**
(`Léproserie`, `Grenier`, `Pressoir`, `Guérite`, `Coffre`, `Table`, `Bûcher`, `Vivier`, `Pilori`).
Nine fetches saved, and no false negative recorded.

### A third numeral bug, same failure mode

`quatre vingt seize toises` — 96 toises, 187.08 m — came out as **70.16 m**. The F3 regex allowed
only **two** number-words, so it matched "vingt seize" (36) and dropped the "quatre". A **62%
error**, and once again a number that looks like a plausible aqueduct length.

That is the third bug in `vldnum.py`'s numeral handling and the third that returned a believable
value rather than failing. Widened to three words; the seven-case check passes. I re-ran the §54
corpus audit immediately rather than waiting a cycle: **one instance corpus-wide**, the one caught
here, and it never reached a manifest.

### The yield curve, and a recommendation to stop

57 articles swept, **647 tokens from 2.73 million characters**. The decay across the last three
batches is not subtle:

| batch | tokens | articles | empty | per article |
|---|--:|--:|--:|--:|
| §50 — 16 chosen by game value | 180 | 15 | 4 | **12.0** |
| §53 — 16 more | 44 | 14 | 5 | **3.1** |
| §55 — 12, list-checked | 17 | 12 | 7 | **1.4** |

Corpus-wide: **the top 10 articles hold 64% of all tokens**, 30% of articles yield zero, and the
median article yields **4**. One token costs about 4,200 characters of reading.

**Recommendation: stop the broad Viollet sweep.** The productive range is exhausted — `Tour`,
`Architecture militaire`, `Porte`, `Château`, `Charpente`, `Escalier`, `Donjon`, `Pont` are all
mined, and they are where the dimensions live. The remaining ~476 articles will return roughly one
token each. Viollet is better used from here as a **targeted lookup** when a specific dimension is
wanted, not as a sweep.

The barn/mill/cellar gap §50 identified still stands and needs a different corpus — the English
vernacular volumes, not more of this one.

### What the last batch did yield

**`Fontaine` has walkable tunnels.** Town aqueduct passages **6 pieds high × 3 pieds wide
(1.95 × 0.97 m)** running **500+ toises unlit** — *"sans qu'il y aie aucune clarté sinon celle que
l'on y peut porter avec feu, le long desquels les personnes peuvent facilement cheminer"*. A
kilometre of lightless crawlspace under a town, with a stated section.

**`Colombier`** gives a small tower worth having: Nesle/Créteil pigeonnier **6.80 m internal
diameter with 1.00 m walls**; a smaller one **4.60 m across × 11.50 m** to the pinnacle tops; and
the lesser timber right at **16 pieds** holding 60–120 nest-holes.

**`Bahut`** — in its roofing sense — supplies the dwarf wall that carries a great roof: **1.25 m
high** at Notre-Dame de Paris, **0.40–0.60 m thick** generally.

**A false friend worth flagging:** Viollet's **`Lit` is the mortar bed**, not a bedstead. It gives
joint thicknesses (0.01–0.03 m in the XIIIc, 0.01 m maximum later) — useful for masonry, useless
for furniture, and it would be easy to mis-slot.

## 56. Cycle 48 — Addy closes the vernacular gap, and a scan that destroys every fraction

Picked up the lead §55/queue-0 parked: **Addy, *The Evolution of the English House* (1898)**, IA
`evolutionenglis00addygoog`. `len(pageindex) == imagecount == 263`, so this is the **trusted branch**
of the iabook.py offset rule — leaf N is image N, no drift hunt needed. Printed page = leaf − 34,
confirmed at five widely separated leaves (n59/25, n101/67, n137/103, n164/130, n192/158) and again
on an illustration (Padley plan, printed 136, found at n170).

New manifest: **`english_vernacular_dimensions_v1.tsv`, 149 rows across 37 buildings**, written by
a generator (`english_vernacular_dimensions_v1.py`) so every metre value is computed from the
printed feet/inches rather than hand-carried — the route that lost two numbers in §48. Plus
**`roof_pitch_derived_v1.tsv`** and **42 new rows** in `architecture_book_plates_v1.tsv`.

### The trap this cycle: the scan destroys every vulgar fraction

The OCR layer of this item contains **zero occurrences of ½, ¼ or ¾ across all 263 leaves** — yet
the book plainly prints them. Leaf n166 carries **three of them in two lines**, and I read them off
the page image at full resolution:

> the great barn of the manor house at Walton was 168 feet long, 53 feet wide, and **33½** feet
> high, viz., **21½** feet to the tie-beams, and 12 feet from them to the ridge-tree.

The OCR renders those as `33^` and `2\\`. Elsewhere: `6i`, `6 J`, `5^`, `9!`.

`engdim.DIM` requires the number to sit immediately against its unit, so **`33^ feet` matches
nothing at all**. Run on the Walton sentence, `engdim.dims()` returns three dimensions where the
page prints five — it drops both fractional ones. **The failure mode is silent omission, not silent
corruption**, which is the safer of the two and is why `engdim.FRAC` has never misfired. But the
consequences are real: the density scan **under-counts**, and a hall printed as `20 ft by 15½ ft`
comes back as one dimension rather than a pair, so the triple detector never sees it.

And the honest limit: **a fraction lost with no junk left behind is undetectable from the text
layer.** There is no way to bound the loss from OCR alone. Only the page image resolves it. So
every fractional value in the new manifest is either `verified_full_res=yes` or flagged
`ocr-uncertain` — 33 rows carry the former.

Three-way agreement on Walton, which is why it is recorded as verified: the full-res page (33½ /
21½ / 12), the arithmetic (21.5 + 12 = 33.5), and the Latin footnote — *"in altitudine sub trabe
xxi. ped' et dimid', et desursum trabe xii. ped'"*.

### The barn/mill/cellar gap (§50) is closed, and by a self-checking source

**Walton great barn, XIIc**, from the *Domesday of St Paul's* — and the Latin states it in
**perches, then checks itself**: *"x. perticas et dimid' in longitudine (et pertica est de xvi.
pedibus) et in latitudine iii. perticas et v. pedes"*. 10½ × 16 = **168 ft** ✓; 3 × 16 + 5 =
**53 ft** ✓. A unit conversion that validates both the number and the unit definition.

**Gunthwaite Hall barn, near Penistone — extant, measured, and effectively a complete parametric
barn**: 165 × 43 × 30 ft; 15 ft to the tie-beams and 15 above; **11 bays of 15 ft** (11 × 15 = 165
✓, self-checking again); two arcades of wooden pillars **14 in × 9 in** on stone pedestals;
tie-beams **23 ft** pillar to pillar; roof **in a single span across the whole breadth**; timber
frame filled with stonework to **8 ft 9 in**; six barn doors. Bay count, arcade section, aisle span,
plinth height and door count in one paragraph.

### The bay rule — the parametric result of the cycle

Addy's whole argument is that the English structural bay is not arbitrary: it is **the standing
room for two pairs of oxen**, which fixes it at the **linear perch of 16 ft**, while the breadth
varies freely. He gives the chain from Roman practice through to measured English fabric:

| source | rule | value |
|---|---|--:|
| Vitruvius | ox-stall breadth, min–max | 10–15 ft |
| Vitruvius | standing room per pair of oxen, min | 7 ft |
| Palladius | standing room per pair | 8 ft |
| Palladius | ox-house breadth | 15 ft |
| Columella | cow-house breadth | 9–10 ft |
| English | linear (building) perch = **bay length** | **16 ft** |
| English | land perch / rod (statute) | 16½ ft |
| Addy, measured | Roman bays, Bailgate, Lincoln, pillar centres | 14 ft 6 in |
| Addy, measured | parish church bays | ~15 ft |
| Bolsterstone | cow-house breadth, measured | 10 ft 8 in |

Then Excursus III (n245) tests it against documents: the Felsa cow-house of 1396-9 is **80 ft =
exactly 5 bays of 16**, and a berchary on the next page is **160 ft = 10 bays**. Both close.

**A conflict Addy prints without resolving**, and so do I: the Welsh Laws give the *long yoke* as
**16 ft** (i.187, i.539), **16½ ft** (ii.784, *"Sexdecim pedes et dimidium"*), and **15½ ft**
(ii.852, *"Pedes XV. et dimidium"*). All four citations are recorded as separate rows. **Do not
average them** — same discipline as Gwilt's two contradicting stair rules in §46.

### Roof pitch: queue item 4's gap closed six times over

The real prize is that Addy's sources state height **as a pair** — floor-to-tie-beam and
tie-beam-to-ridge — which makes the pitch derivable. Six buildings do this
(`roof_pitch_derived_v1.tsv`, all rows labelled DERIVED):

| building | breadth | rise | pitch | rise/run |
|---|--:|--:|--:|--:|
| Walton great barn | 53 ft | 12 ft | **24.4°** | 0.45 |
| Gunthwaite barn | 43 ft | 15 ft | **34.9°** | 0.70 |
| Kensworth hall | 30 ft | 11 ft | **36.3°** | 0.73 |
| Kensworth "house" (domus) | 17 ft | 7 ft | **39.5°** | 0.82 |
| Peveril keep, upper room | 19 ft | 10 ft | **46.5°** | 1.05 |
| Kensworth bower (thalamus) | 16 ft | 9 ft | **48.4°** | 1.13 |

Range 24.4°–48.4°, median 39.5°, and it sorts cleanly by span: **the wider the building the
shallower the roof.** Only Gunthwaite's span is author-guaranteed — Addy states the roof is a single
span across the whole breadth. **Walton's 24.4° is the row to distrust**: great barns are usually
arcaded, and if the roof is carried on the arcade rather than the outer walls the true span is less
than 53 ft and the pitch is steeper. Flagged in the file, not silently averaged in.

### The rest of the haul

**Kensworth manor house, XIIc** — the three-room plan (hall / *domus* / *thalamus*) with all three
dimensioned plus the roof split, and an ox-house, sheep-cote and lamb-cote. Addy prints the Latin,
so the Roman numerals check the English. The hall is *"nearly twice as big as the 'house' and bower
put together"*.

**A village forge with a footprint.** Bishop Hatfield's Survey, c. 1350, gives a newly built booth
at *"longitudinis xx pedum et latitudinis xviij pedum"* — 20 × 18 ft — and adds that another booth
**"as well as the village forge"** is the same size. That is the only smithy dimension the corpus
has from a contemporary document.

**A building code.** Fitz-Alwyne's Assize, A.D. 1189: London party-walls **of freestone, 3 ft thick
and 16 ft high**; and a deed of 1217/18 puts the upper-floor joists at **8 ft above the ground**.

**Peak (Peveril) Castle keep, Castleton** — 22 × 19 ft upper room, 17 ft to the "square" and 27 to
the ridge, first-floor entrance **4 ft 9 in wide, 8 ft 6 in above outside ground**, and a **sentry
aperture 6 ft 5 in deep × 4 ft 1 in broad** with an iron bar 4 ft 7 in above its floor. The keep's
roof is deliberately **concealed behind the parapet** so the rampart walk keeps a clear view and
beacon fires stay hidden — a design reason, not just a dimension.

**Small buildings with full openings schedules**: Gallarus Oratory (15 ft 3 in × 10 ft, doorway
with **inclining jambs** — 1 ft 11 in at the head, 2 ft 5 in at the sill, a 3 in square socket for
the door-frame 8 in under the lintel); Teampull Beannachadh on the Flannan Isles (walls 2 ft 5 in
to 2 ft 11 in thick around a chamber of only 7 × 5 × 5 ft 9 in, doorway 3 ft high, no window at
all); the Great Hatfield mud house (walls 1 ft 7 in of mud, eaves at 6 ft 2 in projecting 10 in, and
a **"speer"** — a hearth screen 4 ft long, *"so that it just covers the door"*).

**Padley Hall** and **Charney Bassett** give the first-floor-hall type twice over, with window
openings as small as **2 ft 6 in × 1 ft** — Addy's own comment is that *"the room must have been
badly lighted."*

### One row recorded as printed and flagged, not corrected

Addy quotes Parker's *Glossary* for a demolished Berkshire barn at Cholsey: *"303 feet long and 51
feet high."* The published figure for Cholsey is a **width** of about 54 ft, so this is probably a
slip for "wide" — by Parker or by Addy. It is recorded as printed with the flag in the quote field.
Correcting a source silently is how §51's Coucy conflict got confusing in the first place.

### Cycle 49 (partial — lane stood down mid-cycle) — the Bolsterstone plate refutes my own derived row

Inspected three plates at full resolution before the lane was stood down for the furniture v3 brief.
The item serves **2759 × 4109 regardless of the `w` requested** — confirmed a third time.

**`Section of Barn at Bolsterstone` (n109) — VERIFIED, and it refutes the assumption behind
`roof_pitch_derived_v1.tsv`.** The barn is **not** a simple gable. It is a **cruck truss with
outshuts**: two curved blades rise from low stone padstones set well inside the walls, cross at a
saddle block at the ridge, and carry a tie-beam that runs out *past* the blades to plates on both
sides; each outshut then takes its own, far shallower lean-to slope down to the outer wall.

So for an aisled or cruck barn:

- **breadth ≠ roof span.** The main roof spans between the blades, not between the outer walls.
- **there is no single pitch.** The main slope and the outshut slope are different by construction,
  so one number cannot describe the section at all.

**Consequence, applied:** Walton's derived **24.4°** is the shallowest row in the table and it is
the one built on the assumption this plate refutes. Great barns of that size are normally arcaded;
if Walton was, its true span is well under 53 ft and its main slope is steeper — while a genuine
outshut slope would be *shallower* still. The row is now marked `UNSAFE` in the file rather than
left to be read as a measurement. **Gunthwaite's 34.9° survives** — Addy explicitly states its roof
is *"in a single span extending across the whole breadth"*, and he says so precisely because it is
the unusual case.

The surrounding text (n109) also pays: Bolsterstone's ox-house holds **eight oxen and no more, four
to a bay, two to a stall**, in bays of **nearly 15 ft** — Addy's bay argument tested against a
standing building — with a loft over the aisle at **rather more than 6 ft**, and **four original
bays** west of which is work dated **1688**.

**`Plan of Kensworth Manor House` (n164) — VERIFIED, and "drawn to scale" is generous.** Measured
by connected-component labelling of the three enclosed white rooms (levels first, per contour.py:
this is a clean line block, dark median 0 / light median 255, so a 128 threshold is safe):

| room | stated | white interior | px/ft along length | px/ft across breadth |
|---|---|--:|--:|--:|
| HALLA | 35 × 30 ft | 949 × 739 px | 27.1 | 24.6 |
| THALAMUS | 22 × 16 ft | 608 × 346 px | 27.6 | 21.6 |
| DOMUS | 12 × 17 ft | 305 × 434 px | 25.4 | 25.5 |

The axis convention is consistent — horizontal is each room's stated *length*, vertical its
*breadth* — and DOMUS closes almost exactly (0.703 drawn vs 0.706 stated). But the lengths agree on
a common scale only to about **4%**, the breadths only to about **8%**, and **the horizontal and
vertical scales differ by roughly 12%**. Fitting a wall thickness to absorb it gives a *negative*
thickness on two of three pairings, so the anisotropy is not a wall-thickness artefact.

**The plan is therefore approximately, not strictly, to scale, and yields no number the text does
not already state.** It is worth having as a topology reference — hall / domus / thalamus in a row,
three conjectural doorways — and nothing more. Recorded as VERIFIED for what it shows, not promoted
to a dimension source.

### New evidence class

`documentary-survey` — a contemporary written survey of a building that no longer stands
(*Domesday of St Paul's*, the Melsa chronicle, Bishop Hatfield's Survey). Kept apart from `measured`
and from `documented-reconstruction` because its error mode is **scribal**, not metrological: the
risk is a miscopied Roman numeral, not a mis-set tape. 31 rows.

## 57. Queue

**Lane: BUILDING DIMENSIONS for a fantasy setting (§43).** Queue rebuilt this cycle — it had
accumulated duplicate numbering and four items already closed (crenel/merlon dimensions closed in
§44; vocabulary in §48; older-manifest audit in §49; Gwilt in §46; English halls in §47).

0. **[DONE in §56 — Addy extracted.]** 149 rows / 37 buildings in
   `english_vernacular_dimensions_v1.tsv`, 6 derived roof pitches, 42 plate rows. The barn / forge /
   cellar gap is closed and the bay rule is recovered with its Roman ancestry. Earlier partial
   result kept for the record: only TWO barns across the five previously cached English volumes,
   one dimensioned — Berwick St Leonard, XVc, **90 ft long × 25 wide, 50 across the transept**
   (Turner/Parker vol 4, leaf n228); a XIVc hall of 39 × 26 ft *"now used as a barn"* (leaf n41);
   a cellar at 31 ft 9 in × 16 ft 10 in × 7 ft 3 in (Garner & Stratton n125). **Those three are
   still not in any manifest** — they need a short cycle to write up.

0a. **[PARTLY DONE in §56, cycle 49 — LANE STOOD DOWN mid-cycle for the furniture v3 brief.]**
   Two of the four plates inspected at full resolution and marked VERIFIED: **Bolsterstone barn
   section (n109)**, which refuted the span assumption behind Walton's derived pitch (row now
   flagged `UNSAFE`), and the **Kensworth plan (n164)**, which turns out to be only approximately
   to scale and adds no number.
   **STILL UNREAD — resume here when the lane restarts:** **Plan of "Coit" at Upper Midhope
   (n105)** and **Section A. B., Castleton Castle (n194)** — both fetched and cached, neither
   inspected. n194 is the more valuable: it should show the keep roof concealed behind the parapet,
   and the keep's 17 ft / 27 ft pair means its pitch can be cross-checked against the drawing.
   The other 38 figures remain mapped but not read. Fetch at the source width — this item serves
   2759 × 4109 regardless of the `w` asked for.

0b. **Six more unmined vernacular titles** on the same shelf: Kent & Sussex, Surrey, Cotswold,
   Shropshire half-timber, *Das englische Haus*, Development of English Building Construction.

0c. **Fix `engdim.py` for fraction-destroying scans (§56).** `DIM` requires the number to abut its
   unit, so `33^ feet` matches nothing and the dimension vanishes. Allow a short run of OCR junk
   between number and unit, emit the value as `ocr-uncertain`, and **never guess the fraction** —
   the point is to make the omission visible, not to invent a number. Re-run the density scan on
   every book in the corpus afterwards to see how much has been silently missing.

1. **STOP the broad Viollet sweep — measured, not guessed (§55).** Yield decayed 12.0 → 3.1 → 1.4
   tokens per article across the last three batches; the top 10 articles hold 64% of everything and
   the median article yields 4. Use Viollet as a **targeted lookup** from here.
   **The next real work is the English vernacular shelf**, which is where the barn/mill/cellar gap
   (§50) has to be closed: Turner/Parker's vernacular volumes, and the agricultural surveys. Those
   are unmined and they cover exactly the building types Viollet does not dimension.
2. **[done in §54 — came back clean]** Re-run the corpus through the fixed numeral parser. Four
   tokens changed; **zero corrupted manifest rows**, because every row carries its verbatim quote and
   the number was always transcribed from the French rather than from the extractor.
2. **Finish the conflict resolution.** §51 traced two of the three: Coucy's 64 vs 65 m differ by
   DATUM (64 m is from the ditch bottom; the 65 m footnote states none), and the Louvre donjon is
   20 × 30 m per `Château`, which explicitly corrects the higher figure. Still open: the 55 m for
   Coucy in `Donjon` needs its datum checked, and the §44 merlon/crenel reading has not been applied
   back to the older rows.
3. **Re-class the Carcassonne and Pierrefonds rows** across every manifest. Both are Viollet's own
   restoration sites, so any crenel sill, machicolation walk, hoarding floor or window embrasure
   measured there is his work. Two rows written in §50 are already marked; the older manifests are
   not.
4. **Supply the three numbers the corpus does not have** and which the §43 build order needs:
   ~~hall roof pitch~~ **[DONE in §56 — six DERIVED pitches, 24.4°–48.4°, in
   `roof_pitch_derived_v1.tsv`]**; still open: tower storey height (derive as risers ×
   steps-per-turn, label DERIVED) and town-house frontage width.
5. **Vernacular gap [CLOSED in §56 for barn, forge and cellar-adjacent types].** Barn is now
   covered twice (Walton documentary, Gunthwaite measured-and-parametric), the village forge once
   (Bishop Hatfield's Survey, 20 × 18 ft), and cow-house/ox-house/sheep-cote/lamb-cote have both
   rules and instances. **Still absent everywhere: the MILL.** No mill dimension exists in Viollet
   or in Addy. It needs its own source.
6. Unmined shelf and the ~59 onward leads from Gardner and Dollman.
7. Paley residue, if ever wanted: fit the fig 19/20 group at its measured 32.0 px/in; Plates
   III–XVI are mapped but not read.
8. **Older open threads still live:** qualify the two cycle-19 rows (abutment `span/4` as
   Viollet's reconstruction; the "only the instinct of this theory" caveat on the thrust rules);
   second-pass the flagged Dollman plates (Haddon Pl. 1, the eight misread half-timber figures).

## 7. Caveats

Plate XII's and Plates XIV–XV's title bands were not legible in the head strip, so their subjects come from the text's own figure references rather than from the plate heads; both are consistent (capitals, then bases) but unconfirmed by eye. Only Plates I and X have been inspected at full resolution — the other fourteen are mapped, not read, and are tagged `PLATE` accordingly. Dimension readings are from screen inspection of a 1930 px scan and should be re-read at original JP2 resolution before any of them enter a stat table. The `measured` column in the TSV is marked `unknown` wherever the plate has not been eyeballed; absence of a `yes` is not evidence of absence.
