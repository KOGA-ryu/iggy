# Gardner, *Ironwork* Part I — chapters IV and V: deep-dive record

2026-07-29 · Deep-dive companion to [architecture_books_findings_v0_1.md](architecture_books_findings_v0_1.md) §20.
Source: J. Starkie Gardner, *Ironwork, from the earliest times to the end of the mediaeval period* (1893),
IA `cu31924004684902`, Cornell scan, 172 leaves, public domain. **leaf = printed page + 13.**

Produced by a fan-out of five parallel readers (locks · door-linings · small fittings · chapter V text ·
chapter IV text) followed by an adversarial verification pass over the fourteen strongest claims.
**Nothing in the verified set was refuted**; four claims were tightened and those tightenings are what is
recorded here. Figures: **all 21 in chapters IV–V (figs 37–57) were opened and read at full resolution.**

> **One correction to the consolidation step.** The synthesis agent received truncated copies of three of the
> five reader payloads and therefore reported figs 43, 46, 47, 52 and 57 as "located only, never opened", and
> described its own coverage gap honestly. Recovering the full payloads from the run journal shows all five
> *were* read at resolution with `sips` crops. Their statuses are corrected to VERIFIED in the plate index, and
> §R below carries the material the synthesis never saw. Fig 57's ambiguous printed page ("14^^" in the List of
> Illustrations OCR) resolves to **p.144, leaf 157**.

---

### This ironwork is not black — the finish spec is the highest-value finding

The single most useful line in these two chapters is a material spec, not a form description. Gardner, p.137:

> "the iron was brightly tinned and laid over red cloth or paper."

That is bright white metal openwork over a saturated red ground — a two-material shader, and it applies to "locks, hinges, and handles" of the German pierced-thistle family, which includes figs 50 and 51. Reinforced three pages later for the door-lining family, p.140: the linings were "illuminated in black and white, red and blue, and profusely gilded". And at Bruck (p.142): "The ground of the lozenges was painted alternately red and blue, so that the general effect was like gold lace on a scarlet-and-blue chequer."

Gilding is the default across both chapters (pp.116, 119, 125, 128, 129, 134, 140) — the Windsor group "was originally gilt" (p.128, verified by eye), the French coffers were "painted and gilt" (p.119). A keyword scan of every leaf in chapter V turned up **no mention of etching and no mention of cast iron at all**; the only finishing processes named are gilding, tinning (p.137 only) and painting. If the factory ships this asset class as dark bare metal, it is wrong on the book's own evidence.

### Construction: the layer stack, and where it stops applying

Chapter V opens with the era rule (p.115, verified by eye):

> "Heat was now applied only in the preliminary stages, and the greater part of the work was accomplished by the file and saw, or by embossing the iron"

and closes the thought with "The direct productions of the forge and hammer were seldom admitted into the design." Note the hedges — "the greater part", "seldom" — and note that the book does not use the word "cold"; that was an inference in the reader pass and has been withdrawn. Carving from the solid is explicitly the top-tier case only, "when the highest realms of art were reached".

Build order for the mid tier, p.116 (verified): hinges made "of several thicknesses of sheet iron, pierced to represent tracery, and riveted together in strong frames", escalating from a single thickness (the Chartres south choir aisle guichet) to "three or four sheets of piercing superimposed". **Layer count is simultaneously a richness and a date parameter.** This recipe is scoped to France and the fifteenth century, not pan-European.

The important limit is one page later, p.117 (verified):

> "the crockets, pinnacles, and leading lines of the tracery are chiselled and filed from the solid iron in full relief, and the pierced sheet-work plays but a very subordinate part."

So the top tier is *not* laminated sheet. A correction was applied here: an earlier reading called the pierced sheet a "backing" — the book never says that, and the word has been dropped, because a generator could wrongly model a flat rear plate from it. The scope is also comparative and tied to the named Rouen and Evreux fittings, not to medieval ironwork generally.

The best assembly spec is p.128 (verified), on the Windsor group:

> "the caps, bases, mouldings, crockets, and cusps are chased out of the solid, and tenoned, morticed, and riveted together as in joinery. Depth and richness are given by using one thickness upon another, over a background of saw-pierced sheet iron"

Joinery in iron — tenons and mortices — is the assembly model, and it explains fig 45 exactly: pierced tracery panels held on a plain plate by small rivet clips.

### Locks: parts, vocabulary, and why no figure shows a mechanism

Rim-lock anatomy, p.119 (verified): **front plate** (decorated with scroll-work), **back plate** (fixes to the door, and is "extended beyond the lock, and cut into leaves, animals' heads, fleurs-de-lis, and other forms"), **bolts**, **latches**, with **master-keys** carrying "tracery handles and innumerable wards". The extended-and-cut back plate is precisely the shaped silhouette seen in figs 49 and 51.

Door-furniture vocabulary, p.129 (verified): a **"vizzying," or guichet**, a square **escutcheon**, and a **handle-plate in form of a rose window**. Add **banded iron** and nails "shaped into rosettes" (p.140), **diploma work** (p.132), **corona** and **pricket** (pp.124, 134), and **buttress-shaped hasp** (p.127) — the only hasp description in either chapter.

Not one of the ten figures examined shows a bolt, ward, tumbler, spring, hasp or shackle. The book explains why, p.118 (verified): "their mechanism is careful, concealing bolts and key-holes with great skill." **Anything the factory needs about internal warding must come from another source.** The nearest thing to warding evidence is a silhouette: fig 51's keyhole is cruciform — round eye, narrow stem, two side notches forming a cross-bar, and a small round hole at the foot — i.e. a cross-warded key.

### Sheathed doors: four measured lattice systems from one generator

All four were read at resolution. The measurements below are **scan-pixel ratios from the 1377 px page images, not real dimensions** — usable as proportions, not as millimetres.

| Fig | Leaf | Door | Strap angle | Band width / crossing pitch | Nails at crossings | Cell filler |
|---|---|---|---|---|---|---|
| 53 | 152 | Cracow Rathhaus | 45° exactly | 0.25 (of perpendicular spacing) | **bare** | pierced plate, 4-fold cruciform, 2 variants checkerboard |
| 54 | 153 | Bruck on the Mur | 50.9° | 0.17 (thinnest) | rosette | pierced plate, essentially all different |
| 55 | 154 | Karlstein | 46° (treat as 45°) | 0.16 | whirl | **painted on the wood**, no iron |
| 56 | 156 | Krems | 54.3° | 0.38 (broadest) | large fluted boss | embossed iron figure plate |

The system is two parts, in the book's own words (p.139): "lining entire doors with pierced and embossed plates and straps of iron" — lattice member and cell filler are separate fabricated parts, so emit them as separate meshes with separate materials.

Four rules worth more than the numbers:

- **Nail placement is diagnostic.** Cracow puts a ring nail at every plate centre and a quatrefoil at every strap-segment midpoint and leaves the crossings bare; Bruck and Karlstein put a rosette or whirl at every crossing. Karlstein's border band runs a strict period-3 sequence — whirl, cross, cross — with a whirl landing at every corner and shoulder.
- **Clipping differs by material.** Cracow's iron diaper is simply cut off by the arch curve and the illustration edge. Karlstein's iron straps are also cut off, but the *painted* charges are re-fitted: the bottom half-lozenges hold compressed but complete eagles and lions.
- **Krems is a double checkerboard with a mid-height swap** — boss versus figure between crossings and interspaces, then griffin versus arms within the interspaces above, "and the lower half is diapered with imperial eagles and lions" (p.142). Four figure plates and one boss cover an entire door.
- **Bruck is the expensive one.** "few of the designs being repeated" (p.140), so it needs a grammar — a vertical spine with top and bottom finials, mirrored scroll branches, and one central emblem slot (shield, pentagram, rosette, tracery arcade, dragon pair, medallion) — not a library. Every plate observed is bilaterally symmetric about the lozenge's vertical axis, unlike Cracow's four-fold motifs.

The tabernacle-door variant (fig 57, located only) flips the interstice shape: "the interstices, which in the richer examples were rectangular, are filled with carved iron tracery, with filigree, or with pierced and embossed subjects" (p.142), the Krems subjects being "taken, in part at least, from the New Testament".

### Architectural miniaturisation, and the one hard dimension

Fig 48 is the whole thesis in one object. Its size is stated twice, in caption and text (p.132, verified): "the lock-plate, eighteen inches high, taken from the Church of Maria-Saal, in Carinthia, now preserved in the Klagenfurt Museum. From its unusual size and elaborate character, it is regarded as having been a diploma work". On a 457 mm plate, the roughly six cusped lancets per bay scale to something like 6–8 mm wide — **jeweller-scale filing**, which is the real lesson. The plate carries a complete miniature building: dentil cap, an arcaded gallery of about seven bays with crocketed gablets, a pendant leaf frieze on moulded string courses, four pinnacled buttress strips dividing three bays, crocketed ogee gables, lancet ranks, a transom band of quatrefoils-in-square, and a base cut into four concave scallops with pendant drops.

The book's own framing, p.126 (OCR only): "the effects of wood and stone are produced in iron in miniature", attributed to the combined smith, clockmaker and architect bringing "the more precise, cultivated, and elaborated tools of the mechanician" to bear. And p.118 (verified): "The basis in the design of all is the flamboyant architecture of the period." So the lock generator should be driven by a *tracery* generator, not by an ornament library.

Every real dimension in the two chapters, for completeness:

| Object | Dimension | Page |
|---|---|---|
| Lock-plate, Maria-Saal / Klagenfurt (fig 48) | 18 in high | 132 + caption |
| Traceried lock on the Hal door | more than a foot in length | 122 |
| Windsor gates | about 7 ft high | 128 |
| Osnabrück herse-light | over 7 ft high | 124 |
| Deux-Acren candelabra | over 6 ft high | 124 |
| Ghent gun "Dulle Griete" | 16,803 kg; 19 ft long × 11 ft circumference | 121 |
| Delhi pillar capital | 3 ft 6 in high | 95 |

No plate gauges, no sheet thicknesses, no keyhole dimensions anywhere.

### Dating rules that survived verification

| Rule | Statement | Page |
|---|---|---|
| Lock design peak | "The best in design belong to the close of the fifteenth century, but they increase in richness during the sixteenth." | 118 (verified) |
| German motif clock | thistle enters "from the beginning of the sixteenth century" and ousts the vine; "for a century no ironwork of any pretension was forged in Germany into the composition of which the thistle did not enter" | 135 (OCR) |
| Late thistle | leaves take "a definite cruciform shape, which henceforth characterises them until the final disuse of the plant"; in extreme degeneration only "cross-hatching, the last trace of the calyx" survives | 137, 139 |
| Trellis threading | oldest German work (Cologne, Aix-la-Chapelle) threads all bars of one direction through the other; Magdeburg, 1495 threads them alternately | 143 (OCR) |
| Bar section | 15th-c. trellis made "with square instead of round iron" | 143→145 (OCR) |
| Door-linings | none Austrian older than the fifteenth century; Cracow "the latter half of the fifteenth century"; the custom outlasted Renaissance architecture | 139 (OCR) |
| Passion-flower | fully developed type "belongs, however, to the Renaissance" | 145 (OCR) |

Two of these interact usefully. Figs 49, 50 and 51 are all thistle-derived and so read sixteenth century by the p.135 rule, while fig 48 — pure tracery, no thistle — reads fifteenth. The cross-hatched, stippled bulbous calyx visible on both thistle heads of fig 49 is a direct instance of the late marker.

Regional presets over one generator: French = both refinement and detail high; Flemish/Brabançon = "sturdier and plainer than the French" (p.129, verified); German = "while the tracery is delicate, the buttresses and pinnacles are intolerably coarse" (p.132) — detail count up, moulding refinement down. Stamping belongs to the earlier French school (pp.100, 112, 114), file/saw/chisel to the locksmith tier; do not mix them on one asset.

### The Victorian "Oriental influence" thesis — assessed, not repeated

Chapter IV is titled "The Transition, due to Oriental Influence in the Fourteenth Century", so the causal claim is structural to the book and cannot be quietly cited around. It should not be repeated as fact.

What Gardner claims: that the Western smith learned his defining tools from the East — "From the East the smith learnt to use the file and saw, and the sumptuous arts of graving, inlaying with gold and silver, damascening and embossing" (p.94) — and that his forms followed, the earliest iron grilles being "copies (for the sake of strength) in iron of the pierced marble so extensively used in Saracenic archit[e]cture; and, emanating from our instructors in geometry, they are naturally geometric in design" (p.95).

What he offers as evidence:

- **Resemblance judged by eye**, doing nearly all the work: "a rich and essentially Saracenic diaper" (p.105); "It is unquestionably an Eastern design" (p.105, of a grille contracted to Roger Johnson of London in 1428); handles "which might have been taken straight from the mosque" (p.109).
- **A trade-route narrative** with no object, workshop, treatise or named craftsman in transmission (p.95).
- **One genuinely evidential item**: a plate at Rendcombe, Gloucestershire "has Arabic numerals and figures engraved upon it, presenting their supposed earliest use in any work connected with building" (p.111). Note "supposed". Numerals travel with notation and commerce; they establish contact, not that ornament was copied.
- **Hearsay**: the 1212 Pamplona grille "is said to have been made from Moorish chains" (p.69), reported as report.

The strongest internal reason to distrust the thesis is Gardner himself. At p.89, the one place in this stretch where he actually reasons about a mechanism, he explains an "entirely novel and rather Oriental effect" with a purely local, mundane cause: "It is just the sort of rendering we might get from a smith, set to work from a drawing without sections, and unacquainted with the process of stamping." A misread drawing, not a trade route. He does not notice that this undercuts the chapter title.

The thesis also does double duty as a value judgement — the East gets credit for the tools and blame for the decline in the same sentence, since the same fashion "required the smith to produce in iron the wood lattice and the pierced marble window, forms proper to wood and stone, by which his art declined" (p.94) — and "our instructors in geometry" is rhetoric carrying an argument. This is standard 1893 diffusionism, and it is a finding about Gardner, not about medieval ironwork.

**Usable residue.** Keep the forms as design-family labels and drop the causation: the geometric interlace grille (St Anastasia's Verona, St Mark's Venice, "probably as old as the thirteenth" century, p.95); the halved-and-riveted diagonal notched-strap diaper at the Canterbury choir side entrance, whose design also occurs in wood at Luxeuil (p.105); and the flattened-ellipse handle ring "shaped like a crescent with the horns beaten round to join the spindle" (p.109). Do not name assets "Saracenic" or "Oriental" on this book's authority, do not put the causal claim into asset metadata, and treat "Saracenic outline" applied to German tracery handles (p.132) as Gardner's shorthand for a look.

### Verification status, corrections, and gaps

**Verified by eye at full resolution** (page images opened, and in several cases cropped with `sips` into figure bands and detail tiles): figs 44, 45, 48, 49, 50, 51, 53, 54, 55, 56. Every quoted passage from pp.115, 116, 117, 118, 119, 127, 128, 129, 132 and 137 was re-read from the page image, not from OCR.

**Located only (marked PLATE, never opened):** figs 43, 46, 47, 52, 57. Quotes from pp.94, 109, 111, 120–126, 135, 138–145 are OCR-derived from the djvu text and have not been proof-read against the images — treat their wording as very likely but not proven. Known OCR corruptions in this item include "gravmg" for "graving", "archit€|cture", "previausly", "aflSxed", "conceaHng" and folios rendered as "ii6"/"ii8".

**Nothing in the adversarially verified set was refuted.** Six claims were confirmed verbatim on the correct pages; four were tightened, and the tightenings are recorded here rather than the original wording:

| Original wording | Correction |
|---|---|
| pierced sheet as a subordinate "backing" (p.117) | "backing" dropped — the book says only that it "plays but a very subordinate part"; it makes no statement about a rear plate |
| "the grandest work" (p.117) | the book's phrase is comparative, "all these grander works", scoped to the named Rouen and Evreux fittings |
| vandyked edges "copied from" plate armour (p.116) | the book asserts identical pattern and contemporaneity, not direction of influence; "copied from" was inference. It also never defines "vandyked" |
| locks "quality-graded like goldsmiths' work" (p.118) | no grading system is described; the page gives a price datum and p.117 a comparison to gold and silver work |

Two page-mechanics notes for the record: p.115 prints **no folio** (it is a chapter-opening page), so its page number rests on the Contents entry and the "116" running head on the next leaf; and the p.118 quotation is not contiguous typography — it breaks around the inline fig 44 and resumes below the caption on the same page. Leaf 149 carries figs 49 and 50 together, each with its own caption; the LEAF = PAGE + 13 offset holds throughout.

**Flagged uncertainties that must not be silently resolved.** Fig 48: the curled tongue-shaped element with a dark central slot on the centre axis cannot be identified even at 4× upscale — swivelling keyhole cover, keyhole in a leaf mount, or decorative pendant — and consequently it is unknown whether the trefoil-pierced roundel above it is the keyhole or a boss; the prose gives no help. Fig 50: the fine regular vertical striation across the main field could be a file-cut or combed iron ground, the weave of the red cloth or paper backing, or a halftone screen artefact — do not commit a shader without a modern photograph of the Augsburg piece. Fig 45: the small corner monograms read approximately "P.H.D." and "O.J", low confidence.

**A text/figure count discrepancy left open:** p.137 says "Four typical examples are illustrated (Figs. 49-51)" but only three figures are numbered in that range. Either fig 52 was counted, or one cut counted twice, or the sentence is loose.

**Coverage gap in this synthesis.** Only two of the five reader payloads (locks, door-linings) survived intact into this consolidation step; the other three were truncated before reaching it, along with the tail of the door-lining text findings and the tail of the verdict list. Any findings those readers made — in particular on figs 43, 46, 47, 52 and 57, and on chapter IV's plates — are **not represented here** and should be re-requested rather than assumed absent.

### Onward sources credited in these chapters

Viollet-le-Duc (cited as "Le Duc", pp.116, 139) is the highest-value lead: he figures the interlaced-band door with sheet ornament, the St Bertin (St Omer) overlapping vandyked plates, and pierced and embossed leaf hinges from the Abbey of Poissy and a house at Gallardon near Chartres — i.e. the actual hinge and door-band drawings. Also: Shaw, "Decorative Arts" (p.116, a flamboyant door then in private hands); Du Sommerard, "Arts du Moyen Age" (p.116, two gilt pierced panels from the tabernacle of St Loup, Troyes — relevant to the gilding question); Gailhabaud (p.134, the Chapelle ardente of Nonnburg near Salzburg); Raschdorf (p.135, a Cologne private-collection thistle piece); Van Ysendyck, "Belgian Architecture" (p.126, an older pierced-plate shutter grille than the Ghent one); "Mr. King" (p.132, on the Lüneburg hinges as *finely coloured* tracery design — relevant to the painted-ironwork question); and the footnote at p.113 to *Transactions of the RIBA*, vol. vii, N.S., pp.160–162, where the German lozenge-leaf vine hinge series (Erfurt, Thann, Oppenheim, Caub, Zülpich, Magdeburg, Oberwesel, Schloss Lahneck) is said to be figured — directly harvestable. Gough the antiquary (pp.128–129) described the Windsor gates as "gilded copper", which is the evidence for their gilding.

Collections holding the verified objects, for modern photographic reference: Klagenfurt Museum (fig 48, ex Maria-Saal, Carinthia); Augsburg Museum (fig 50); Amerling Collection, Vienna (fig 51); St George's Chapel, Windsor (fig 45); Neuberg, Styria (fig 49); Rathhaus, Cracow (fig 53); Priory of Bruck on the Mur (fig 54); Castle of Karlstein near Prague (fig 55); the suppressed monastery at Krems (figs 56, 57); and the South Kensington Museum, now the V&A, for fig 46 and fig 47."
  },
  "workflowProgress": [
    {
      "type": "workflow_phase",
      "index": 1,
      "title": "Read"
    },
    {
      "type": "workflow_phase",
      "index": 2,
      "title": "Verify"
    },
    {
      "type": "workflow_phase",
      "index": 3,
      "title": "Synthesize"
    },
    {
      "type": "workflow_agent",
      "index": 1,
      "label": "read:locks",
      "phaseIndex": 1,
      "phaseTitle": "Read",
      "agentId": "a0a567c4d9f26f613",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785372788754,
      "queuedAt": 1785372784803,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785373482610,
      "tokens": 99390,
      "toolCalls": 40,
      "durationMs": 693856,
      "resultPreview": "{"figures":[{"fig":"45","printed_page":129,"leaf":142,"title":"Fig. 45.—Lock in St. George's Chapel, Windsor.","content_type":"wood-engraved line block (inline, above text); small monograms at lower corners, left reads approx. \"P.H.D.\" and right \"O.J\" — both uncertain","status":"VERIFIED","observations":"Landscape rectangular lock plate, aspect roughly 2.4:1, shown flat-on with no perspective.…"
    },
    {
      "type": "workflow_agent",
      "index": 2,
      "label": "read:linings",
      "phaseIndex": 1,
      "phaseTitle": "Read",
      "agentId": "a628ac29d7262a45a",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785372788339,
      "queuedAt": 1785372784803,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374188618,
      "tokens": 174515,
      "toolCalls": 45,
      "durationMs": 1400279,
      "resultPreview": "{"figures":[{"fig":"53","printed_page":139,"leaf":152,"title":"Part of door-lining from the Rathhaus, Cracow. Fifteenth century.","content_type":"line woodcut / wood-engraving, pure outline (no tone), inline in text page","status":"VERIFIED","observations":"Upper part of an iron-lined door with a very flat segmental head on large rounded shoulders (rounded-rectangle outline). THREE CONCENTRIC ZONE…"
    },
    {
      "type": "workflow_agent",
      "index": 3,
      "label": "read:furniture",
      "phaseIndex": 1,
      "phaseTitle": "Read",
      "agentId": "acc601853a545f629",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785372788364,
      "queuedAt": 1785372784803,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785373486263,
      "tokens": 99539,
      "toolCalls": 38,
      "durationMs": 697899,
      "resultPreview": "{"figures":[{"fig":"46","printed_page":130,"leaf":143,"title":"FIG. 46.—An extremely rich Flemish \"vizzying,\" or guichet, with iron wattlework. In the South Kensington Museum.","content_type":"full-page inline wood-engraving (by J. D. Cooper, per List of Illustrations), tonal/shaded rendering of the object in situ in a wooden door","status":"VERIFIED","observations":"Read at 1378x2157 plus four …"
    },
    {
      "type": "workflow_agent",
      "index": 4,
      "label": "read:chap5_text",
      "phaseIndex": 1,
      "phaseTitle": "Read",
      "agentId": "aef5a3b839c474a0a",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785372788841,
      "queuedAt": 1785372784803,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785373602527,
      "tokens": 119691,
      "toolCalls": 37,
      "durationMs": 813686,
      "resultPreview": "{"figures":[{"fig":"Fig. 43","printed_page":117,"leaf":130,"title":"Handle of sacristy, Rouen Cathedral.","content_type":"line engraving (wood-engraved outline, signed \"R.C.H. del.\")","status":"VERIFIED","observations":"A large oval closing-ring seen face-on. The ring is of flattened-elliptical cross-section and is worked with many fine longitudinal grooves/mouldings running the whole way round,…"
    },
    {
      "type": "workflow_agent",
      "index": 5,
      "label": "read:chap4_text",
      "phaseIndex": 1,
      "phaseTitle": "Read",
      "agentId": "afe469ebbd547a8e0",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785372788568,
      "queuedAt": 1785372784803,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785373484027,
      "tokens": 88015,
      "toolCalls": 25,
      "durationMs": 695458,
      "resultPreview": "{"figures":[{"fig":"Fig. 37","printed_page":98,"leaf":111,"title":"Frieze of the grille in the Palazzo Publico, Siena.","content_type":"tonal wood/process engraving after a photograph (fine parallel-line hatching on the frame, solid blacks in the openwork)","status":"VERIFIED","observations":"One bay of a large church/civic grille, seen straight on. THREE STACKED ZONES visible. (1) Top: a heavy ho…"
    },
    {
      "type": "workflow_agent",
      "index": 6,
      "label": "verify:0",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "ab313d392c2d613c4",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195031,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution. Fetched gardner_ironwork1/n12…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374303955,
      "tokens": 43202,
      "toolCalls": 11,
      "durationMs": 108924,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at full resolution. Fetched gardner_ironwork1/n128_w1700.jpg (1377x2157) and read the whole page with the Read tool.\
\
PAGE IDENTITY CONFIRMED (leaf 128 = printed p.115), by three independent checks: (a) leaf 128 is the opening page of \"V. THE AGE OF THE LOCKSMITH, FIFTEENTH AND SIXTEENTH CENTURIES.\"; (b) the Contents list (leaf 10) reads \"V. The Age of the…"
    },
    {
      "type": "workflow_agent",
      "index": 7,
      "label": "verify:1",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a7a08d640c356a084",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195019,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution (Read of gardner_ironwork1/n12…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374265680,
      "tokens": 38867,
      "toolCalls": 5,
      "durationMs": 70661,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at full resolution (Read of gardner_ironwork1/n129_w1700.jpg; printed folio \"116\" and running head \"IRON.\" both visible in the page header, so LEAF=PAGE+13 holds). Both quoted fragments appear verbatim, in the order quoted, and are not OCR-corrupted:\
\
(1) end of first paragraph, verbatim: \"A great many hinges were made, at this time and in the fifteenth …"
    },
    {
      "type": "workflow_agent",
      "index": 8,
      "label": "verify:2",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a52f782628389e815",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195333,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED on the correct page. I fetched leaf 129 fresh (ga…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374308549,
      "tokens": 40846,
      "toolCalls": 10,
      "durationMs": 113216,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED on the correct page. I fetched leaf 129 fresh (gardner_ironwork1/n129_w1700.jpg, 1378x2157) and READ the full page image: running head is \"116  IRON.\", so printed p.116 = leaf 129 as recorded. The quoted sentence appears verbatim in the first paragraph, reading in the image (not just OCR): \"...and another from the Abbey of St. Bertin, at St. Omer, in which t…"
    },
    {
      "type": "workflow_agent",
      "index": 9,
      "label": "verify:3",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "aab7e402740331900",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195985,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "Confirmed at full resolution on the page image (gardner_iro…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374273113,
      "tokens": 36404,
      "toolCalls": 8,
      "durationMs": 77128,
      "resultPreview": "{"refuted":false,"reason":"Confirmed at full resolution on the page image (gardner_ironwork1/n130_w1700.jpg), not merely from OCR. Running head verified as \"THE AGE OF THE LOCKSMITH.\" with folio 117, so leaf 130 = printed page 117 as recorded. The quoted sentence appears verbatim in the body text immediately below the Fig. 43 caption: \"In all these grander works the crockets, pinnacles, and lea…"
    },
    {
      "type": "workflow_agent",
      "index": 10,
      "label": "verify:4",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a7e70c67775d88781",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195614,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "VERIFIED by reading the page image (gardner_ironwork1/n131_…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374262828,
      "tokens": 37490,
      "toolCalls": 7,
      "durationMs": 67214,
      "resultPreview": "{"refuted":false,"reason":"VERIFIED by reading the page image (gardner_ironwork1/n131_w1700.jpg, cropped bands) at full resolution. Printed page number confirmed: header reads \"118    IRON.\" on leaf 131, consistent with the LEAF = PRINTED PAGE + 13 rule. Both quoted fragments appear verbatim on that page. Top band (pre-figure): \"sitely wrought are the finest of the locks, that over £1000 has be…"
    },
    {
      "type": "workflow_agent",
      "index": 11,
      "label": "verify:5",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a351a5ba0edccae88",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195498,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED on the actual page image (leaf 131 fetched at wid…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374266497,
      "tokens": 41330,
      "toolCalls": 8,
      "durationMs": 70999,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED on the actual page image (leaf 131 fetched at width 1700 -> 1378x2157, read at full resolution; running head reads \"118   IRON.\"). The quote appears verbatim in the first paragraph, lines 3-5: \"The best in design belong to the close of the fifteenth century, but they increase in richness during the sixteenth.\" OCR and image agree exactly; no OCR corruption …"
    },
    {
      "type": "workflow_agent",
      "index": 12,
      "label": "verify:6",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a652a6f61bc7b1cad",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195627,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution. Fetched gardner_ironwork1/n13…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374278370,
      "tokens": 41370,
      "toolCalls": 9,
      "durationMs": 82743,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at full resolution. Fetched gardner_ironwork1/n132_w1700.jpg and read the cropped head band (/private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books/p119_a.jpg). Running head reads \"THE AGE OF THE LOCKSMITH.   119\", so leaf 132 = printed p.119 (consistent with LEAF = PAGE + 13). The page opens: \"wrought master-keys…"
    },
    {
      "type": "workflow_agent",
      "index": 13,
      "label": "verify:7",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "ad03768f7662b316c",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374195008,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED — checked both the OCR (pages.json[132]) and the …",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374262830,
      "tokens": 36731,
      "toolCalls": 7,
      "durationMs": 67822,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED — checked both the OCR (pages.json[132]) and the page JPEG at full resolution via two sips crops of /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books/gardner_ironwork1/n132_w1700.jpg.\
\
PAGE IDENTITY: correct. Running head visually reads \"THE AGE OF THE LOCKSMITH.\" with folio \"119\" at the right — leaf 132 =…"
    },
    {
      "type": "workflow_agent",
      "index": 14,
      "label": "verify:8",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "ab4bb642b79cdf41d",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374267058,
      "queuedAt": 1785374192735,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at printed p.119 = leaf 132 (running head read fr…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374347918,
      "tokens": 40309,
      "toolCalls": 7,
      "durationMs": 80860,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at printed p.119 = leaf 132 (running head read from the image: \"THE AGE OF THE LOCKSMITH.  119\"). Read gardner_ironwork1/n132_w1700.jpg at full resolution; the passage is the second-to-last paragraph and reads verbatim: \"The coffers of sheet iron, strengthened with straps carried from back to front, enriched with tracery, and with lids curved like a trunk, a…"
    },
    {
      "type": "workflow_agent",
      "index": 15,
      "label": "verify:9",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "af8d96fd0f8480a6b",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374269135,
      "queuedAt": 1785374192736,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "VERIFIED against the page image at full resolution (/privat…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374354388,
      "tokens": 43081,
      "toolCalls": 8,
      "durationMs": 85253,
      "resultPreview": "{"refuted":false,"reason":"VERIFIED against the page image at full resolution (/private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books/gardner_ironwork1/n140_w1700.jpg), not just OCR. Leaf 140 carries printed page number 127 with running head \"THE AGE OF THE LOCKSMITH.\" — page reference is correct.\
\
The quote appears verbatim in the first paragraph (…"
    },
    {
      "type": "workflow_agent",
      "index": 16,
      "label": "verify:10",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a4913d31a3e651225",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374269670,
      "queuedAt": 1785374192736,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution. Fetched leaf 141 (gardner_iro…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374379756,
      "tokens": 42663,
      "toolCalls": 11,
      "durationMs": 110086,
      "resultPreview": "{"reason":"CONFIRMED at full resolution. Fetched leaf 141 (gardner_ironwork1/n141_w1100.jpg, 1378x2157) and Read it: running head reads \"128   IRON.\", so LEAF 141 = printed p.128 as recorded. The quoted sentences appear verbatim in the final third of the page: \"Many thousand pieces of carefully filed iron have been required in the construction of this monumental work; and the caps, bases, mould…"
    },
    {
      "type": "workflow_agent",
      "index": 17,
      "label": "verify:11",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a01834b7c5929861d",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374270827,
      "queuedAt": 1785374192736,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution (VERIFIED, not OCR-only). Fetc…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374372669,
      "tokens": 42709,
      "toolCalls": 9,
      "durationMs": 101841,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at full resolution (VERIFIED, not OCR-only). Fetched leaf 142 as gardner_ironwork1/n142_w1700.jpg and read three crops: header, top text band, bottom text band. Running head reads \"THE AGE OF THE LOCKSMITH.   129\", so printed p.129 = leaf 142 (leaf = page + 13 holds). The quoted sentence appears verbatim in lines 1-5 of the page: \"On the / south door of the …"
    },
    {
      "type": "workflow_agent",
      "index": 18,
      "label": "verify:12",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "ac8663b7676c5accc",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374280758,
      "queuedAt": 1785374192736,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution (Read tool on cropped page ima…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374357530,
      "tokens": 37953,
      "toolCalls": 6,
      "durationMs": 76771,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at full resolution (Read tool on cropped page images, not OCR alone).\
\
Quote 1 — printed p.129 = leaf 142, VERIFIED verbatim in the image: \"Flemish ironwork, like the Flemish chests, was probably in fashion during the reign of Edward IV., and was certainly imitated in England during the succeeding reigns. It is not easy to distinguish, except by its greater …"
    },
    {
      "type": "workflow_agent",
      "index": 19,
      "label": "verify:13",
      "phaseIndex": 2,
      "phaseTitle": "Verify",
      "agentId": "a2d7bdbc8c8888b30",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374282282,
      "queuedAt": 1785374192736,
      "attempt": 1,
      "lastToolName": "StructuredOutput",
      "lastToolSummary": "CONFIRMED at full resolution. Read the page image gardner_i…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374370158,
      "tokens": 41145,
      "toolCalls": 9,
      "durationMs": 87876,
      "resultPreview": "{"refuted":false,"reason":"CONFIRMED at full resolution. Read the page image gardner_ironwork1/n145_w1700.jpg (leaf 145 = printed p.132), cropped to the lower text band, and the quote appears verbatim: \"No grander specimen has been produced than the lock-plate, eighteen inches high, taken from the Church of Maria-Saal, in Carinthia, now preserved in the Klagenfurt Museum. From its unusual size an…"
    },
    {
      "type": "workflow_agent",
      "index": 20,
      "label": "synthesize",
      "phaseIndex": 3,
      "phaseTitle": "Synthesize",
      "agentId": "a4c8ae32b7221f6ec",
      "model": "claude-opus-5",
      "state": "done",
      "startedAt": 1785374385147,
      "queuedAt": 1785374381559,
      "attempt": 1,
      "lastToolName": "Bash",
      "lastToolSummary": "cd /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6…",
      "promptPreview": "CONTEXT — you are mining one book for a medieval-game 3D asset factory's reference library.

BOOK: J. Starkie Gardner, "Ironwork, from the earliest times to the end of the mediaeval period" (1893),
Internet Archive id `cu31924004684902`, 172 leaves, Cornell scan. Public domain.

WORKING DIR: /private/tmp/claude-501/-Users-kogaryu-iggy3d/61d58121-6ce0-4ca5-99b5-8469a117d7aa/scratchpad/books   (cd h…",
      "lastProgressAt": 1785374890947,
      "tokens": 80237,
      "toolCalls": 5,
      "durationMs": 505799,
      "resultPreview": "===TSV_PLATES===
gardner_ironwork1_1893	cu31924004684902	fig 43	130	Handle of the sacristy door, Rouen Cathedral (caption wording unconfirmed)	door handle / lock furniture	inline wood-engraved line block	no	PLATE	https://archive.org/download/cu31924004684902/page/n130_w1700.jpg	Located only: the caption was seen on the p.117 page image during verification of the adjacent text, but the figure itsel…"
    }
  ],
  "totalTokens": 1225487,
  "totalToolCalls": 305
}


---

## R. Material recovered from the truncated reader payloads
This section is not in the synthesis above. It comes from the three reader returns that were cut short before consolidation.

### R2. Small fittings — guichet, tabernacles, handle (chapter V)

**46 — FIG. 46.—An extremely rich Flemish "vizzying," or guichet, with iron wattlework. In the South Kensington Museum.** · leaf 143 (printed p.130) · **VERIFIED**

Read at 1378x2157 plus four sips crops (cornice, mid, wattle band, right stile). Structure top-to-bottom, all one iron frame let into a plank door (wood rendered as fine vertical striations; two large domed nail-heads in the right stile).

(1) CORNICE / HEAD, 5 stacked mouldings: (a) battlemented cresting of small rectangular merlons, ~22-24 across, even pitch; (b) a bead row (pearl / bead-and-reel), ~55-60 beads; (c) a broad band of running scroll — repeating interlocked hooked C/crescent forms, ~13-14 repeats, reads as a stylised running wave or vine; (d) a cabled (spiral-cut) roll moulding — clear twisted-rope torus; (e) plain fillet + beaded lower edge. The cornice oversails the frame on all visible sides.

(2) UPPER TRACERY REGISTER: two-light head. Field is pierced sheet tracery of pointed vesica / leaf-shaped lights, each vesica containing a quatrefoil piercing; the vesicas interlock in a reticulated net. Junctions carry small curled leaf-crockets and there are foliate cusp-terminals. A vertical central mullion runs through, headed by a small rectangular block.

(3) MIDDLE REGISTER: twin sub-arches (segmental/pointed, with a moulded soffit) each filled with the same quatrefoil-in-reticulation tracery. Centred on the mullion is a heater-shaped SHIELD charged with a plain couped cross; the shield field is matted/granulated (dense punched cross-hatch), the cross itself smooth and raised — so raised device on a sunk-and-punched ground.

(4) BALUSTER TIER: five upright shafts standing free in front of the tracery, each a turned/knopped baluster of stacked collars and knops with a pointed spear/finial head — miniature stone shafts. They rise to about half the aperture height.

(5) WATTLE BAND (the feature Gardner singles out): a literal basket weave in iron across the full width. ~5 heavy vertical stakes (the continuing bars) with ~12 slender round horizontal weaver rods woven alternately in front of and behind each stake (single randing). Rods are irregular, slightly tapered, with visible hammered ends; some read as paired. This is osier hurdle-work reproduced in iron.

(6) LOWER REGISTER: 5-6 heavy square-section vertical bars, each with a moulded cap and a moulded splayed base (miniature pier base / buttress plinth), standing in front of a flat pierced plate of ogee/vesica reticulated tracery (no quatrefoils here — simpler, pointed-oval net).

(7) SILL: heavy projecting moulded sill with a cabled/spiral-cut lower roll, oversailing like the cornice.

FIXING: short square lugs/tangs project horizontally from the iron frame into the wooden stiles on both left and right (two visible each side).

NOT PRESENT: no hinge, pivot, latch, lock, keyhole or handle is visible anywhere on this figure. As illustrated the guichet is a FIXED grated aperture (consistent with "vizzying" = a viewing hole), not an opening wicket. Flagging this because the assignment anticipated hinges/lock.

ARCHITECTURAL MINIATURISATION — stone elements present: battlemented parapet/cresting; bead and cable mouldings; reticulated bar tracery with quatrefoils and cusping; vesica lights; leaf crockets; moulded caps and bases to shafts; heraldic shield; projecting moulded cornice and sill (string-courses).

*Generator decomposition.* Generator decomposition. Params: aperture_w, aperture_h, n_registers(=4 here), n_balusters(5), n_lower_bars(6), n_weavers(12), n_stakes(5).
PARTS: [cornice_stack] = ordered list of profile bands, each band = (profile_type, repeat_unit, repeat_count): merlon(22-24), bead(~58), wave_scroll(13-14), cable_twist(continuous, pitch ~ 1.5x rod dia), fillet. Same stack mirrors as [sill_stack] minus merlons.
[tracery_plate] = 2D reticulation generator: lattice of interlocking pointed vesicas; per-cell fill = quatrefoil | empty; cusp count per arc = 2-3; node decoration = curled leaf. Two variants used at different scales (quatrefoil-rich upper, plain-net lower) -> one generator, one boolean param.
[baluster] = lathe profile: [base_splay, shaft, collar, knop, collar, shaft, spearhead]; instance along x with even spacing.
[wattle_band] = weave solver: n_stakes vertical cylinders + n_weavers horizontal rods, each weaver sinusoid in Z with period = 2*stake_spacing, phase alternating per course; rod radius ~ 0.35 * stake radius; add per-rod noise in length/taper.
[shield] = heater outline + charge(cross) raised, ground = punch-noise displacement map.
[fixing_lug] = square prism instanced at frame rail ends.
DEPTH ORDER (matters, and matches p.128 text): balusters/bars in FRONT, tracery plate BEHIND, void behind that. Build as 3 parallel Z-layers.
SURFACE: sunk ground = fine punch stipple; mouldings = clean chased edges; wattle = forged round with irregular hammer facets.

**47 — FIG. 47.—Tabernacle grille from Ottoburg, Tyrol. In the South Kensington Museum. German fifteenth-century work.** · leaf 144 (printed p.131) · **VERIFIED**

Read at 1377x2157 plus six crops (top, door, left flank, base, right-jamb fitting, door foot). This is the strongest architectural-miniaturisation specimen in the cluster: a free-standing iron cupboard/shrine built exactly like a stone tabernacle-house.

PLAN: canted, three-faced front (flat centre face + two oblique flanks returning to the wall) — a half-hexagon/chevet plan. Four angle uprights result.

(1) SUPERSTRUCTURE — 3 GABLES (wimpergs), one over each of the three faces, centre gable largest. Each gable is a steep two-sided pediment of moulded bar; the raking edges carry CROCKETS (curled, deeply undercut leaf forms, 3-4 per rake); each apex carries a tall FINIAL. Each gable tympanum is pierced with cusped/trefoiled openings (a trefoil under the apex flanked by lancet-shaped voids).

(2) CRESTING: behind and between the gables, running along the cornice, a band of small fleur-de-lis / trefoil-headed uprights, closely and evenly spaced (~10-12 per face) — an open crest, not battlemented.

(3) PINNACLES: 4 very tall slender angle pinnacles rising well above the gables, plus the 3 gable finials = 7 vertical spikes in silhouette. Each pinnacle is a square-section rod carrying a regular ladder of small tabs/knops (~10-14 up the shaft, even pitch) and a lower spearhead-shaped boss, terminating in a spike. This is a stone crocketed pinnacle abstracted into a rod-plus-tabs — exactly the feature Gardner calls "intolerably coarse" (p.132).

(4) CORNICE: a plain moulded box cornice with a row of round rivet heads along it, oversailing the body on all three faces.

(5) FRONT DOOR (the centre face): ogee/two-centred pointed arched head. Archivolt is a CABLED (rope-twist) roll moulding. Spandrels above the arch are filled with flat pierced foliate scrollwork (rinceaux, C-scroll leaves); above the arch apex a small quatrefoil-in-circle. Door field = a TRELLIS of square-section bars crossing at 45 degrees on a regular pitch (lozenge mesh, roughly 8x11 intersections), with a four-lobed cross/quatrefoil rosette applied at EVERY intersection. A central vertical mullion runs up to the arch apex, dividing the trellis into two leaves/lights. Behind the trellis is a pierced ground. Bottom rail carries a cable bead.

(6) FLANK PANELS: tall openwork grille panels. Field = 3-4 stacked registers of blind lancet arcading in pierced tracery (each register a row of small cusped lancets with tracery heads) — a vertical stack of miniature windows. A diagonal strap brace crosses the left flank. Partway up each angle upright there is a small sloping pent-roof shelf = a BUTTRESS SET-OFF (weathering) — a purely stone-architectural device.

(7) ANGLE UPRIGHTS / BUTTRESSES: each is faced with a continuous double row of curled leaf-crockets running its whole height on both edges, i.e. a crocketed buttress shaft. Coarse, repetitive, mechanically spaced (~14-16 leaves per side).

(8) FITTINGS: on the right (closing) jamb of the door, a small applied rectangular plate with two slot-like marks — most consistent with a lock keeper / keyhole escutcheon, but the engraving is small and I cannot resolve it with confidence (flagged uncertain). Two round bosses (rivets) on the left jamb. A small rectangular staple at the door foot, possibly a bolt keep. No hinge is visible from the front.

(9) BASE: moulded plinth/sill rail with an evenly spaced row of round rivet heads (~8 across the front). FEET: the two front angles stand on feet masked by applied heater-shaped BLANK SHIELDS (escutcheons); the two outer/rear angles stand on plain C-scroll volute feet. Object stands clear of the ground, casting a shadow.

STONE ELEMENTS INVENTORY (answer to the brief): gable/wimperg, crocket, finial, pinnacle, buttress with set-off/weathering, open fleur-de-lis cresting, cornice/string-course, ogee arch, cabled roll moulding, spandrel rinceaux, quatrefoil, cusped lancet arcading in stacked registers, moulded plinth, heraldic shield. Every one of these is a stone-mason's element used at ~1:6 or smaller scale in iron. One ornament library serves both.

*Generator decomposition.* Generator decomposition. TOP-LEVEL: object = canted_prism(n_faces=3 front-facing, cant_angle ~ 45deg) x [base_plinth, body, cornice, gable_array, pinnacle_array].
PARAMS: face_w_centre, face_w_flank, body_h, gable_pitch (steep, ~65-70deg), crocket_pitch, pinnacle_h/body_h ~ 0.55, n_crest_units_per_face, trellis_pitch, trellis_angle=45.
[gable] = 2 raking bars + tympanum tracery(trefoil + 2 lancets) + crockets instanced along rake at even arclength + apex finial. Reusable with the SAME code as a stone wimperg.
[pinnacle] = square rod + tab ladder (n_tabs, pitch, tab_size) + spearhead boss at z_frac ~0.35 + apex spike. Rod-and-tab is the iron-specific simplification of a stone crocketed spire: expose a param 'abstraction' switching between leaf-crockets (stone) and tabs (iron).
[cresting] = 1D instancer of fleur-de-lis unit along a rail; unit = the p.145 iris spec (2 inner elevated petals, 2 outer recurved petals, 2 stamens).
[buttress_shaft] = extruded moulded profile + crocket_row instanced on both edges + one or more set-off wedges at z_fracs.
[door] = arch(ogee, cable_moulding_archivolt) + spandrel_rinceaux(pierced sheet) + trellis(bar_section=square, angle=45, pitch, rosette_at_every_intersection) + mullion + rails.
[trellis] IS THE KEY REUSABLE PIECE: same generator serves fig 47 door (lozenge, rosettes) and the p.142 door-linings; toggle interstice = lozenge|rectangular, and node_ornament = none|rosette|other.
[foot] = variant: shield_masked | scroll_volute. Instance per angle with a variant selector (front=shield, rear=scroll).
RIVET ROWS: 1D instancer of dome heads along plinth and cornice rails, even pitch.
CONSTRUCTION per p.128 text: parts tenoned/morticed/riveted, layered sheet over saw-pierced sheet -> model as discrete Z-layers with visible rivets, NOT as one solid.

**52 — FIG. 52.—Handle from church door of St. Marein, in Styria.** · leaf 151 (printed p.138) · **VERIFIED**

Read at 1378x2157 plus two upscaled crops (staple/grip, corner). Drawn flat-on against a wooden door (grain rendered as long wavy vertical lines); the handle casts a soft shadow down-right, so the drop stands clear of the plate. Pure outline style, so piercings read as white and embossing as chased hatching.

TWO COMPONENTS:

A) SQUARE BACKPLATE of pierced sheet iron. Edge treatment: a regular row of small semicircular notches (scalloped/cusped edge) all round — ~13-14 notches per side, even pitch, so ~52-56 total. Just inside the edge, a border row of small piercings ALTERNATING round holes and short oval slits. Field: a symmetrical branching THISTLE arabesque, bilaterally symmetric about the vertical axis and roughly about the horizontal too. Structure is a stem graph: stems spring from nodes, branch two or three ways, each terminal ending in a pointed lobed thistle leaf whose interior is slit/pierced and filled with fine parallel chased grooves. At almost every branch node sits a domed BOSS with a spiral/snail-shell chased top (I count on the order of 40-50 of these bosses over the plate). Along the stem axes are scattered small round punched holes and short slits. Two fan-like sets of concentric arcs flank the central staple, symmetric left and right — deliberate ornament (Gardner, p.138, says the thistle "simulated the oak, the fan, the Eastern spathe"), not wear marks.

B) DROP HANDLE, heart-shaped / shield-shaped: broad rounded shoulders, sides curving in, terminating in a point at the bottom. It has a plain moulded outer rim; the top edge is a wide flat band filled with fine vertical striations (reeding/chased grooves) and its inner edge is scalloped into three lobes with three round piercings between them. Inside the rim, the same pierced thistle arabesque as the plate, at slightly larger scale, with the same spiral bosses and hatched leaves.

SUSPENSION: a plain rectangular staple (squared U-loop) whose two legs pass through the backplate at the centre; the handle hangs on it. At the top of the staple is a block/collar (nut- or knop-like) through which the pin passes.

NO tracery, NO gables, NO pinnacles: this object is NOT architectural miniaturisation. It is the OTHER German idiom — flat pierced-and-embossed thistle sheetwork. Useful as the contrast case. Per p.136 such work "was brightly tinned and laid over red cloth or paper," so the correct material read is bright white metal over a red ground showing through the piercings.

No lock, escutcheon or keyhole on this figure.

*Generator decomposition.* Generator decomposition — this is a 2.5D SHEET generator, not an architectural one.
PARAMS: plate_size (square), notch_count_per_side (13-14), border_hole_pattern (alt hole|slit), sheet_thickness (thin), boss_height (low, 'slightly embossed' per p.137), drop_outline (heart/shield), n_scallops_on_grip (3).
PIPELINE: (1) square base plate; (2) boolean the scalloped edge = instance a half-circle cutter along each side at even pitch; (3) border ring instancer alternating circle and capsule cutters; (4) generate a SYMMETRIC BRANCHING STEM GRAPH: seed nodes on the vertical axis, branch angle ~30-50deg, 2-3 children, depth 3-4, mirror across the vertical axis (and near-mirror across the horizontal); (5) at each terminal, place a thistle-leaf cutter: pointed lobed outline, interior slit, plus 3-6 parallel groove lines; (6) at each internal node, place a domed boss with a spiral groove on its crown; (7) scatter small round hole cutters along stem arcs; (8) add two fan motifs = concentric arc grooves radiating from the centre staple, mirrored.
The drop handle reuses steps 4-7 inside a heart/shield outline, plus a reeded top band (n parallel grooves) with 3 scallops and 3 round piercings.
STAPLE = rectangular U loop + collar block; handle pivots on it (rig as a hinge joint with limited swing).
MATERIAL: two-layer shader — bright tinned iron sheet over a red backing visible through every piercing.
REUSE: this same generator, per p.136-137, produces LOCKS, HINGES and HANDLES; only the outline changes. Build outline as a swappable input.

**57 — FIG. 57.—Tabernacle door in the sacristy of the Hospital Church in Krems. Late fifteenth century.** · leaf 157 (printed p.144) · **VERIFIED**

LEAF CONFIRMED: leaf 157, printed page 144 (the List of Illustrations OCR "14^^" resolves to 144). It is the last figure in the book. Read at 1378x2157 plus four upscaled crops (gable head, hinge, lock, central register).

OVERALL SHAPE: tall door, rectangular below, with the two top corners CANTED at ~45deg to a short flat apex — a cropped/flattened gable head (five-sided, 'coffin-headed' outline). Drawn in very slight perspective so the right edge and its lock are seen partly in profile.

ARMATURE: a massive orthogonal bar grid. 2 vertical mullion bars divide the width into 3 columns; horizontal transom bars divide the height into 6 full registers plus the gable register above — about 19-20 rectangular panels total. Prose (p.142) confirms the richer form has RECTANGULAR interstices, and that is what this is: rectangular panels, not lozenges. Crucially the panels are OPENWORK — you can see through them; the massive horizontal bars pass straight across the figure subjects (a bar crosses the Crucifixion at hip level), and the sheet-metal figures are applied over/behind the bars.

BORDER: outer frame band = a running undulating wavy ribbon/stem with leaves, on a hatched ground, following the canted head; inner frame band (the vertical stiles and the mullions) = a two-strand plait/interlace with small cusped leaf sprigs.

REGISTER BANDS: between every register runs a horizontal band of undulating wave-stem ornament — a single serpentine stem with alternating half-palmette leaves above and below, on a fine hatched (matted) ground. Same unit on every band.

PANEL FILLINGS (cut from sheet, embossed and chased; drapery rendered by close parallel grooves): gable centre = a nimbed Christ figure with cross-staff over an arch, flanked by two flying angels (Resurrection); gable left = a group of standing figures with a tall staff/candle. Register 2: left = a crowned/seated figure; centre = foliage with a central roundel; right = a mounted or striding figure spearing a DRAGON (St George). Register 3: three panels of foliage with roundels and grille-like uprights. Register 4 (centre of the door): the CRUCIFIXION — Christ on a titulus-headed cross, flanked figures (Mary, John), small kneeling figures, and angel/bust forms at the cross-arms; right panel = the Agony in the Garden (a tree, a chalice above, a kneeling bearded figure, sleeping companions). Register 5: foliage/rosette panel left; centre a stag or hound among branches; right a figure among branches. Register 6 (bottom): huntsmen with spears and dogs, a quadruped, and a group of figures. Bottom rail = a plait/interlace band. Subject mix agrees with p.142: "taken, in part at least, from the New Testament."

FITTINGS: on the LEFT (hanging) edge, TWO plain rectangular hinge leaves/lugs project — upper (about 1/5 down) and lower (about 4/5 down). They are entirely undecorated, in deliberate contrast to the face; three round rivet/bolt heads sit in the stile band beside the upper one.
On the RIGHT (closing) edge at mid-height, a LOCK seen partly in profile: a rectangular box case straddling the stile, shaded with fine vertical hatching; outboard of it a vertical POINTED-OVAL (vesica-shaped) plate; and projecting from the centre of that plate a short stubby cylinder with a shaped end — either the shot bolt or a turn-knob. I cannot resolve which from this engraving; flagged uncertain.
No handle or ring is shown.

ARCHITECTURAL MINIATURISATION: much weaker here than in fig 47. The only architectural gesture is the canted gable head; there is no tracery, no crockets, no pinnacles, no buttresses. This door belongs to the pierced-and-embossed FIGURE-SUBJECT idiom on a bar armature, not to the tracery idiom. Two distinct generator families are therefore needed for German/Austrian tabernacle doors.

*Generator decomposition.* Generator decomposition — ARMATURE + APPLIQUE, three Z-layers.
PARAMS: door_w, door_h, cant_height (top corners cut at 45deg), n_cols=3, n_rows=6, bar_section (square, massive: bar_w ~ 0.04*door_w), band_h (register band), border_w.
LAYER 0 (structure): outline = rectangle with two 45deg corner cuts + short flat apex. Frame stiles/rails + n_cols-1 vertical mullions + n_rows-1 horizontal transoms = orthogonal bar lattice with RECTANGULAR cells. Cells are VOID by default.
LAYER 1 (ornament bands): 1D instancers along every rail/mullion. Two repeat units only: (a) undulating wave-stem with alternating half-palmettes (used on horizontal bands and the outer border, follows the canted rake); (b) two-strand plait with leaf sprigs (used on vertical stiles/mullions and the bottom rail). Ground under both = hatched/matted punch texture.
LAYER 2 (subject appliques): per-cell payload from a library, cut as flat silhouettes from sheet, then embossed (low relief) and chased with parallel drapery grooves. Payload types: figure_scene | foliage_with_roundel | animal_scene. Note the applique OVERLAPS the bars — model as a separate sheet plane in front of/behind the armature, not as a cell fill. This overlap is the diagnostic look.
CELL LAYOUT PATTERN observed: centre column carries the narrative climax (Resurrection at apex, Crucifixion at mid-height); flanking columns carry saints/hunt/foliage; foliage registers alternate with figure registers. Encode as a row-type sequence.
FITTINGS: hinge = plain rectangular leaf, instanced at 2 z_fracs (~0.2, ~0.8) on the hanging edge, deliberately UNDECORATED. Lock = box case + vesica outer plate + short cylindrical bolt/knob, at z_frac ~0.5 on the closing edge.
FINISH per p.142 (Znaim doors): 'still preserve their gold and coloured decoration' -> support a polychrome+gilt variant.

### R1. Chapter IV figures — grilles, knocker, handles

**Fig. 37 — Frieze of the grille in the Palazzo Publico, Siena.** · leaf 111 (printed p.98) · **VERIFIED**

One bay of a large church/civic grille, seen straight on. THREE STACKED ZONES visible. (1) Top: a heavy horizontal entablature/architrave rail, deep (roughly 1/4 of the bay height), rendered as several stacked flat fascias with a chamfer - this is Gardner's framing 'really built up of plates of iron', and the seams between the plates are visible as horizontal lines. Round rivet/nail heads run along it at intervals, and three small applied triangular/arrowhead ornaments sit on its face. (2) The frieze panel proper: a rectangular opening, roughly 2:1 landscape, bordered on all four sides by a narrow dentil/nick band (a strip punched or chiselled into a row of small square teeth - a 'key-border'). Inside, a bilaterally symmetrical field of beaten sheet-iron scrollwork: two large C-scroll vine stems spring from the bottom centre and coil outward and up, throwing off secondary tendrils that terminate in split trefoil/palmette leaves and small volute buds. Leaf tips are broad, cleft, and slightly dished. At the exact centre stands a quadruped in profile-ish view - the beast Gardner calls 'the wolf of Rome' - rendered with a stippled/roughened (punched, granular) skin against the smooth polished scrolls; it is the only naturalistic element and it visually anchors the symmetry axis. Small forms under its belly could be the suckling twins but the block is too coarse to confirm. (3) Below the frieze, a second heavy rail (again built-up fascias with rivet heads and a dentil band), and beneath that the top edge of the quatrefoil grille field: a row of five pointed-arch heads (the upper bows of the quatrefoils) meeting at cusps, each junction masked by a small beaten leaf/boss, with short stub spikes at the meeting points. Flanking the bay, two vertical framing members of the same built-up moulded construction, each carrying an applied vertical strip with a repeating notched/chevron pattern; beyond them fragments of the neighbouring panels' foliage are visible, proving the design repeats bay by bay.

*Generator decomposition.* Fully parametric. FRAME = extruded box profile built from N stacked plate fascias (N=3-4), plus corner overlap; decorate with (a) rivet array along each rail, spacing s, (b) dentil strip inset from the opening edge, tooth pitch p, (c) applied notched vertical strip on the stiles. PANEL = mirror-symmetric scroll generator: root at bottom-centre, two primary logarithmic-spiral stems, recursion depth 2-3, each terminal instanced from a small library of leaf types {split trefoil, palmette, volute bud}; leaves are flat sheet with a shallow dish and a raised centre rib. CENTRE MOTIF = swappable heraldic slot (beast | shield | badge) with a distinct roughened material to contrast the polished scrolls. GRILLE FIELD BELOW = quatrefoil lattice tile, with a leaf boss instanced at every cusp junction. The bay is the tiling unit: repeat along X. Textures wanted: two iron materials in one asset - smooth hammered sheet for the scrolls, granular/punched for the beast, plus rivet-head decals.

**Fig. 38 — Grille belonging to M. Le Secq des Tournelles.** · leaf 115 (printed p.102) · **VERIFIED**

A window grille shown in perspective, still (or reconstructed as) set into a plank window frame at the right. FIVE horizontal tiers, roughly EIGHT units wide. Every bar - vertical and horizontal - is a square bar given a spiral twist along its whole length (the engraver renders it as continuous oblique nicks). The vertical bars are the generating element: each one FORKS, the two limbs bowing outward and then curling back inward and over at the top, so that adjacent forks close against each other and read as a row of hearts, point downward, cleft upward. In the cleft of every heart hangs a small pendant terminal beaten into a spearhead/leaf shape (flat, with a raised midrib and a stubby tip), attached to the bar that continues up through the cleft. Six horizontal twisted rails cross the field (top edge, bottom edge, and one between each tier). At EVERY crossing of a horizontal rail with a vertical bar there is an applied stamped rosette - a small 5-6 petal flower with a bossed centre - masking the joint. The horizontal rails run out beyond the grille at both ends and are drawn flattened and tapered into leaf-shaped or spade-shaped tails where they pass through/behind the frame into the masonry; two or three similar flat leafy tails also project from the top rail and the bottom rail. A few small hooks project below the bottom tier. The bottom rail carries a denser run of rosettes than the intermediate rails.

*Generator decomposition.* Clean parametric prop. PARAMS: tiers T, bays B, bar section w, twist pitch, heart width/height ratio, pendant style. GENERATOR: (1) build a twisted-square-bar profile once, reuse for all members; (2) per bay, sweep the heart outline as two mirrored cubic curves from a common apex, tangent-continuous into a straight riser above and below; (3) instance the pendant spearhead at each heart cleft; (4) lay horizontal twisted rails at tier boundaries; (5) instance a stamped rosette at every rail x bar intersection (this is the cheap detail that sells the piece); (6) instance flattened leaf tails on all rail ends that penetrate the frame. Gardner's own rule for this family (p.101) is exactly this: bar ends 'beaten either into tufts of spiny leaves, fleurs-de-lis, bunches of lilies, or tridents, and the intersections of the vertical and horizontal bars often concealed by flowers or rosettes' - so terminal style and intersection-mask style are the two variation knobs for a whole family of window grilles.

**Fig. 39 — Knocker from Stockbury.** · leaf 122 (printed p.109) · **VERIFIED**

Mounted on a vertical-boarded door (rendered as vertical grain hatching). PARTS, top to bottom: (1) BACK PLATE - a circular disc of sheet iron, slightly dished/domed, whose whole margin is cut into a continuous saw-tooth of about 26-28 small triangular teeth (Gardner's 'notched and lobed round the margin' / 'vandyked'). The disc face is pierced with about seven or eight CROSS-shaped slots - short-armed crosses with slightly expanded arm-ends - arranged in a loose ring around the centre; also about six small round holes, some clearly nail holes, some decorative punchings. No engraved surface ornament: all the decoration is cut through or cut into the edge. (2) Two small STAPLES (eyes) riveted through the plate near the top, set apart by roughly a third of the disc diameter. (3) The BOW - a bar bent into a wide U or horseshoe, hung by its two upturned ends in the two staples; the bow is of flattened section with a slight central ridge and is thickest at the bottom of the curve. This is the part the hand lifts. (4) From the bottom of the bow a long tapered PENDANT/striker descends past the lower edge of the plate; it is flat and blade-like where it leaves the bow, then passes through a moulded collar/knop and continues as a straight tapering shank. (5) At the bottom, an ANVIL/striking stud fixed to the door independently of the plate: a short projecting forged block, roughly cuboid, whose outer face carries an incised LOZENGE within a square; the pendant's foot rests on top of it, so lifting and dropping the bow drives the pendant onto the stud. The whole thing is one of the plainest members of its family - Gardner explicitly calls it 'a simple form of this kind of work'.

*Generator decomposition.* Ideal small prop, 5 sub-meshes. PLATE: disc, radius R, thickness t, dish depth d; margin teeth = radial saw pattern, count N, tooth depth h; pierce with an array of cross slots (arm length, arm width, count, ring radius) plus a scatter of round holes. This is a boolean-array generator - vary N, cross count, and slot shape {cross | trefoil | keyhole} and you get Gardner's whole stated range (p.109: 'fancifully pierced with crosses, trefoils, key-holes, etc.'). STAPLES: two instanced eyes. BOW: swept flattened bar along a U arc, thickness tapering toward the ends. PENDANT: tapered sweep + one turned collar. ANVIL STUD: small chamfered box + incised lozenge (a groove, not a boss). Rig: bow + pendant as one rigid child pivoting in the staples - a single-axis swing, which is all the animation a knocker needs.

**Fig. 40 — Handle, with pierced tracery, from Stogumber Church, Somerset.** · leaf 123 (printed p.110) · **VERIFIED**

A ring-handle on a circular pierced plate, the richest of the four small props here. PARTS: (1) BACK PLATE - circular sheet-iron disc, visibly dished so that it rises toward the centre. Its edge is finished with a stout applied CIRCULAR BAND laid over the plate margin (Gardner p.110: 'the plate being reinforced by stout circular bands'); the band's surface is worked into a continuous run of close oblique nicks so it reads as a cable or rope moulding. (2) The plate field inside the band is PIERCED with radiating tracery: about sixteen slots radiate from the centre hub, alternating longer lancet/dagger-shaped openings with shorter ones, and the outer end of each slot finishes in a cusped, trefoil-like head, so the whole face reads as a sun or rose-window rosette. Between the slots stand narrow solid ribs of the same width, and a plain annulus is left between the slot heads and the rope band. A second, finer scalloped/beaded band appears on the plate near the bottom. (3) CENTRE / SPINDLE - a rectangular block standing proud at the hub, transversely ribbed with about eight parallel grooves (a reeded or fluted collar). The ring's two ends are gathered into this block; it functions as the staple through which the ring hangs, and it is Gardner's 'spindle'. (4) The RING - a stout closed ring hanging forward and down over the lower half of the plate, roughly 0.6 of the plate diameter. Its outer surface is nicked/beaded like the rim band, and at the bottom centre - where the hand pulls - there is a cluster of small beads or knops. The ring's own thickness swells slightly at the bottom. The ring overlaps and partly hides the lower tracery, which is drawn faintly through/behind it.

*Generator decomposition.* Three parts, high visual return. PLATE: dished disc; radial slot array is a single boolean pattern with params {slot count, alternation ratio, cusp head style, hub radius, outer annulus width}; the rope rim is a torus with a helical nick pattern - reuse the same nick shader/geometry on the ring. SPINDLE: small reeded box (groove count g), doubles as the pivot. RING: torus with a bottom-centre bead cluster. Rig: ring pivots about the spindle axis and can also swing out from the door. FAMILY KNOB per Gardner p.110: the same plate accepts a ring, a 'stirrup-shaped' handle, or 'interlacing knots' - so build one plate and three swappable grips. Rim treatments named in the text and directly usable as variants: 'key-borders, crenelations, or vandyked edgings', optionally with cross-bands connecting the circular bands.

**Fig. 41 — Handle in Westcott Barton Church.** · leaf 124 (printed p.111) · **VERIFIED**

NOTE A DISCREPANCY: the text on p.110 cites '(Fig. 41)' for 'the small bar-handles', but Fig. 41 is not a bar-handle - it is a large oval ring on a narrow vertical strap plate. Fig. 42 is the bar-handle. Treat the p.110 citation as a mis-reference. WHAT IS ACTUALLY SHOWN: (1) a narrow VERTICAL STRAP PLATE running down a plank door, its outline cusped into a symmetrical sequence - from the top: a pointed LOZENGE finial (with an incised lozenge on its face), then a fleur-de-lis / trefoil expansion with two outward-curling side lobes, then a waisted narrowing, then a central lozenge boss, then a mirrored fleur-de-lis expansion, then a bottom lozenge finial. So the plate is one cusped strap generated by mirroring a half-profile about its midpoint. (2) A staple near the upper third of the plate carries (3) a large VERTICAL OVAL RING hanging to the right and downward, taller than wide (roughly 1.5:1), of stout section that appears flat-oval with a rounded outer face; two or three narrow COLLARS or wrapped ties are visible clasped around the ring on its right limb. (4) Fixings: lozenge-shaped nail heads (diamonds) - one at each plate finial and boss, plus four more scattered on the door boards clear of the plate, which are presumably the nails of the door's own construction or of a vanished plate. The ring hangs in front of and partly across the plate's lower half.

*Generator decomposition.* Two parts. STRAP PLATE: generate from a half-profile spline mirrored top/bottom; the cusp vocabulary is {lozenge finial, fleur-de-lis lobe, waist} sequenced along the length - the same grammar Gardner gives for hinge straps ('the termination of every scroll is a graceful fleur-de-lis ... simple straps terminate in a triple or even in a single fleur-de-lis', p.111), so ONE profile generator serves plates, escutcheons and hinge straps. RING: oval torus (two radii), plus 1-3 instanced collar bands at parameterised positions along it - the collars are a cheap, period-correct detail and mark this as a forged rather than cast ring. NAIL HEADS: lozenge pyramid, instanced; scatter set on the door as a separate decal layer. Rig: ring swings in the staple.

**Fig. 42 — Fourteenth-century door-handle.** · leaf 124 (printed p.111) · **VERIFIED**

A bar-handle (bail handle) - the type Gardner describes as 'small bar-handles ... attached at each end to the doors by traceried plates or rosettes' with 'cleverly forged knops in the centre'. PARTS: (1) TWO ROSETTE PLATES, one at top and one at bottom, identical, each a lobed/cusped sheet-iron rosette of roughly quatrefoil skeleton with each lobe sub-cusped, giving about 10-12 small lobes around a circular centre; each is fixed by a single visible central fastener (a small squarish rivet/nail head) - the top plate is drawn with the fastener at its centre, the bottom plate likewise. (2) The HANDLE itself is one bar bent into a squared U (a staple/bail): from each rosette a short arm projects perpendicular out from the door, then turns through a right angle so the long member runs vertically, parallel to and standing clear of the door by roughly one bar-length. The bar is of square or rectangular section. (3) Small COLLARS/mouldings mark each of the two right-angle bends. (4) At the mid-point of the long vertical member is a forged KNOP: a spool or baluster swelling formed of two flat collars flanking a barrel-shaped bulge - it looks turned but is hammered. The handle is drawn casting a shadow on the boards, emphasising the standoff. Nothing is pierced; all the enrichment is in the two rosette plates and the single knop.

*Generator decomposition.* The cheapest of the four to build and the most reusable. PARAMS: bar section w, standoff s, length L, knop style, rosette lobe count. GENERATOR: sweep a square profile along a path of 6 points (rosette -> out -> bend -> down -> bend -> in -> rosette) with filleted corners; instance a collar at each bend; instance a knop at t=0.5 (or at t=0.33/0.67 for a two-knop variant); instance the rosette plate at both ends. ROSETTE PLATE: quatrefoil base outline with recursive sub-cusping (depth 1) - same generator as the pierced-tracery cusps in Fig. 40, so share it. Gardner's stated variant to build as well: 'sometimes the plates of attachment extend the length of the handle, and are richly pierced' - i.e. swap the two rosettes for one long pierced backplate.

### R3. Chapter IV text — stated findings

| Page | Finding |
|---|---|
| 93 | The chapter's thesis in one sentence: in the fourteenth century the smith stops relying on heat and starts working iron COLD, with the tools of the joiner and the locksmith. This is the single most useful dating/technique rule in the chapter. |
| 93 | Sheet iron enters the repertoire at the transition, cut into tracery and into leaves and flowers - and this is what splits the blacksmith's trade into locksmith and armourer. |
| 93 | The claimed route of Oriental influence: Crusades -> Saracenic art of Western Asia -> along the trade routes -> Northern Italy (Venice) -> France -> England. Gardner opens with the Papal crusading motto as his emblem for it. |
| 94 | What Gardner says the East actually contributed: TOOLS AND PROCESSES (file, saw, graving, inlaying with precious metal, damascening, embossing) plus TWO BORROWED FORMS (the wood lattice and the pierced marble window). He blames the second half for the decline of the art. |
| 94 | The iron pillar of Delhi - the chapter's only hard dimensions for a forged object, and its only claimed absolute date for Eastern work. |
| 96 | GRILLE EVOLUTION SEQUENCE (Italy), stage 1: the earliest Italian grilles are literally iron copies of pierced marble, built as a GROOVED frame holding pierced sheet panels - no rivets. Gardner puts them at Verona and Venice, probably 13th century, and says the type was quickly abandoned as too laborious. |
| 96 | Stage 2-3: riveted strap lattices in geometric patterns, then figures and badges cut from sheet and riveted between the lattice, then armorial badges and cyphers. Dated example: the Perugia palace grille of broad flat riveted straps forming rectangular cells filled with rampant griffins and coroneted A's in circles. |
| 96 | Stage 4: circles (possibly copied from roundel glazing at St Mark's) give way to the QUATREFOIL, which becomes the dominant Italian grille unit; circles and quatrefoils survive side by side at San Miniato, Florence. |
| 97 | QUATREFOIL GRILLE CONSTRUCTION, plainest form: four bent bows tied together with collars, and a sharp spike WELDED IN at each junction of the segments. Richer form: the spikes and the loose collar ends are beaten out into leaves; a badge is set in the centre of each quatrefoil; leaf borders and a leafy defensive cresting with 'arching tridents' are added. |
| 97 | The Italian cathedral grille type (Siena, Orvieto and others) is a FRAMED, PANELLED composition: a massive-looking richly moulded frame that is actually built up of iron plates, dividing the field into rectangular panels of quatrefoil filling, with a sheet-iron frieze of foliage and heraldry above, and a foliated defensive cresting on top. |
| 97 | Dated anchor and a counting rule: Orvieto, 1337, is the earliest dated example - 4 quatrefoils per panel; the Florence (Santa Trinita) adaptation has 30 per panel and no cresting; the Siena Palazzo Publico version has 9 per panel plus subsidiary pointed quatrefoils in the interspaces. |
| 98 | Interspace filling as a distinct enrichment step: the gaps left between quatrefoils get filled either with SUBSIDIARY POINTED QUATREFOILS (Siena) or by setting the quatrefoils WITHIN CIRCLES (Santa Croce, Florence). |
| 98 | CRESTING vocabulary, Italian: straight spikes with occasional agave/yucca-like flower spikes, and a lotus-like finial over each vertical framing bar; at Orvieto, slender fleurs-de-lis studded with spikes, lofty finials, and two tiers of cusped foliage over the frame's vertical divisions. |
| 99 | The Santa Croce grille of 1371 is the best-documented specimen: rectangular panels, six quatrefoils within circles per panel, interspace ornament, every segment junction beaten into a leaf, a plain cornice with a black-letter dedication instead of a frieze, and NO cresting. Its gate is a full iron copy of a traceried Gothic window. |
| 99 | TECHNIQUE, the Santa Croce gate: iron was PUNCHED into architectural mouldings with tools, as well as chiselled and filed, and the twisted pillars were composed of several separate moulded pieces. Gardner notes the tracery was out of scale and the fashion did not spread. |
| 99 | Gardner's own verdict on Italian ironwork: it was joinery and carving in iron from the start, not smithing, and stayed that way until the seventeenth century. |
| 100 | FRANCE, the pivotal construction change: the Rouen Cathedral choir-aisle gates carry perhaps the earliest flat iron tracery on a grille, and the arrival of tracery plus sheet iron let the smith abandon welds and collars for RIVETS - which Gardner calls a revolution in the craft. |
| 100 | The St Denis grille (figured by Viollet-le-Duc) as the type-specimen of the new construction: sharply bent scrolls with ends beaten thin and cut into quatrefoil leaves, riveted to upright bars which are themselves FACED with punched sheet-iron strips. |
| 100 | Le Puy-en-Velay cloister grille gives a within-period dating rule: caps and bases made by HAMMER ALONE without the file, and sheet ironwork still WELDED rather than riveted, are processes 'soon afterwards abandoned'. |
| 101 | French fourteenth-century norm: grilles of small bars THREADED through each other vertically or diagonally, sometimes enriched with pierced plates and borders. Gardner separately distinguishes THREADED (French/German) from PINNED (St Alban's) from HALVED (Chichester, Christchurch) as three different intersection joints. |
| 101 | French window-grille terminal and intersection vocabulary, stated as a family: bar ends beaten into tufts of spiny leaves, fleurs-de-lis, bunches of lilies, or tridents; intersections concealed by flowers or rosettes. Defensive versions get scrolls, hooks and 'spinous leaves'. |
| 103 | Fig. 38 identified: a window grille said to come from the house of Jacques Coeur at Bourges, with the vertical bars opened out to form the outline of a HEART. (The plate caption instead names the owner, M. Le Secq des Tournelles.) |
| 103 | ENGLAND: the new fashion killed decorated door hingework, which had been the chief branch of English smithing; surviving fourteenth-century examples are rare and provincial. |
| 104 | St Alban's shrine grille (time of Edward I) - the only English trellis grille, with hard dimensions: rectangular panels filled with half-round bars only HALF AN INCH in diameter, crossing diagonally in some panels and at right angles in others, PINNED at every intersection, with a sheet-iron border pierced in quatrefoils as a cornice. |
| 104 | Chichester Cathedral gates (late 14th / early 15th c): bars neatly HALVED at intersections forming small square panels each framing a plain quatrefoil, and - novel in England - assembled with pins or rivets, with no collars and no welding. |
| 104 | Salisbury choir grilles give a second bar gauge and a cusp-orientation variable: flat iron straps about THREE QUARTERS OF AN INCH wide, rectangular framing, rough pointed quatrefoils with cusps PERPENDICULAR (vertical) rather than diagonal - and where the cusps are vertical the interspaces shrink, so at Wells the quatrefoils were welded at the cusps instead of merely bent over. |
| 104 | Christchurch, Hants: bars halved where they cross AND the quatrefoil cusps LET INTO A MORTICE HOLE in the bars - carpentry joints executed in iron, and contemporaries noticed. |
| 105 | BEST GENERATOR SPEC IN THE CHAPTER - the Henry V chantry grille, Westminster Abbey, by Roger Johnson of London, 1428 (agreement still extant): two tiers of long narrow round-headed arches filled with a quatrefoil diaper; the arched pieces are APPLIED ON THE FACE TO HIDE THE CONSTRUCTION; heavy vertical bars behind; the diaper is short, nearly uniform forged pieces, halved at intersections and merely WEDGED into notches cut in the concealed upright framing; richness added by DUPLICATING each diaper piece in sheet iron cut a little broader and riveted to the back. |
| 105 | Canterbury choir side entrance: the simplest Saracenic-diaper recipe - long NOTCHED straps set vertically, similar straps crossing diagonally, halved and riveted. |
| 105 | Gardner's blunt conclusion about the transition in England: the smith did not design this work and his role in making it was mechanical, so English decorative smithing effectively died for about two centuries, surviving only in armour. |
| 107 | ENGLISH TOMB RAILINGS - a distinct, purely functional English type and its evolution: plain massive vertical bars with NO horizontal bars or filling to give a foothold; introduced no earlier than the end of the fourteenth century; bars set with the ANGLE TO THE FRONT; heavy battlemented cornice; tall turret-like buttressed standards (six on the Black Prince's tomb) possibly for tapers; cornice enriched with stamped lions' heads or crests. |
| 107 | TOMB-RAIL DATING LADDER, with dates: bars soon carried up and sharpened to points 'like the stakes of a stockade', or barbed like arrow-heads (Langham, possibly c.1376); standards enriched with crocketed finials (Fitzalan, Arundel, 1415); traceried sheet-metal border in several thicknesses, or an inscription between twisted fillets, replacing the battlements (Ashton, St John's Cambridge - earliest crests of the founder on the standards); horizontal bars hidden by richly decorated straps with foliated bar-ends (Hungerford, d.1411; Beckington, Wells, d.1464, with massive richly wrought turret standards). |
| 109 | KNOCKER / HANDLE FORM - the most Saracenic type is a flattened ellipse, a crescent whose horns are beaten round to join the SPINDLE. Surface finishing is enumerated as six named operations, and two motif traditions coexist: old smith's zoomorphs (dragons or dogs' heads biting each other or the spindle) and new armorial devices. |
| 110 | BACK PLATE anatomy and variation: either a simple sheet-iron disc notched and lobed round the margin and bossed out in the centre, or fancifully pierced with crosses, trefoils, key-holes etc. Richer examples combine sheet with forging - the plate reinforced by stout circular bands, sometimes connected by cross-bands, punched and filed into key-borders, crenelations or vandyked edgings. |
| 110 | GRIP variants stated for these plates: rings (plain or decorated), STIRRUP-SHAPED handles, or rarely INTERLACING KNOTS. Separately, small BAR-HANDLES fixed at each end by traceried plates or rosettes 'seem mostly to belong to the fifteenth century', sometimes with forged knops in the centre, sometimes with the attachment plates extended the whole length of the handle and richly pierced. |
| 111 | ESCUTCHEON (keyhole plate) anatomy: either plain rectangular or polygonal plates with fleurs-de-lis at the angles (Winchester, Chichester), or - most often - founded on the SHIELD, though cut into arabesques so exuberant that the shield is disguised. One at Rendcombe, Gloucestershire, carries engraved Arabic numerals; the large Hereford plate carries Bishop Audley's initials, device and a butterfly in RAISED IRON RIVETED to the plate. |
| 111 | FOURTEENTH/FIFTEENTH-CENTURY HINGE RULE: hingework survives mainly where security mattered - sacristy and treasury doors, muniment chests - and the old forms persist but with 'far more refined drawing, as if the smith were no longer the sole designer'. Terminal rule: scroll ends become fleurs-de-lis; the most elegant are simple straps ending in a triple or a single fleur-de-lis. |
| 112 | French hinge variants worth their own asset slots, with a late-13th-century technique note from Coutances: deeply MOULDED strap-hinges, SPLIT at the ends, bearing two STAMPED FLOWERS; and moulded straps forming a diagonal trellis over a whole door, fixed to the wood by the same stamped rosettes used as nail-heads. |
| 113 | GERMAN VINE-LEAF DATING RULE: after passing through many conventional forms, the German vine leaf settles in the fifteenth century into a flat lozenge-shaped leaf so deeply cleft as to form a distinct quatrefoil; the type is peculiarly Rhenish and is characterised by a multitude of leaves branching from straight or slightly curved slender stems. |
| 114 | A density benchmark for espalier-style hinge coverage: Schloss Lahneck on the Rhine has hinges almost covering the doors, 'trained in the singularly stiff manner of espaliers', bearing some TWO HUNDRED AND FIFTY leaves in all. Erfurt Cathedral (c. mid-15th c) has six scroll hinges on the outside and the inner face completely covered by a diaper of rosettes, leaves and armorial bearings. |
| 114 | Gardner's regional derivation for Germany: German architectural ironwork of this period is based entirely on the rich STAMPED work of France - general ideas, the vine, its conventional forms, the small fleurs-de-lis among the leaves, and sparing tracery - until about 1450, when Flanders and Brabant redirect it. |
| 110 | Terminology harvest from this chapter (period/technical terms actually used by Gardner): collar, tie, spindle, back plate, escutcheon plate, cresting, frieze, cornice, standard, pricket, crocket, finial, cusp, bow (of a quatrefoil), diaper, trellis, key-border, crenelation, vandyked edging, pounced, pinked, lined, serrated, notched, halved, threaded, pinned, wedged, morticed, welded, riveted, faced, shingled bar, stamped. |
| 101 | A charming and specific one-off worth recording as a novelty variant: a window grille in M. le Secq des Tournelles' collection in which the bows of the quatrefoils are united by passing through JESTERS' BELLS; and the Langeac screen, where quatrefoils are set DIAGONALLY so that no considerable interspaces remain, with twisted bars, rosettes, and a cresting of spikes growing through tulip-shaped flowers. |
| 108 | A useful negative/contrast rule about English practice: whenever a doorway or opening in masonry could be entirely filled, the English did NOT use upright bars - they used arcaded or panelled screens. Vertical-bar railings were reserved for free-standing tomb enclosures, for protection. |
| 108 | France used a wattle-like binding for upright-bar grilles: the Sainte Chapelle choir grilles appear to have been of upright bars bound together by IRON WATTLEWORK, a form still seen in Belgium, possibly copied from fashionable trellised garden hedges. |
| 113 | Gardner's comparative verdict for the period: English and French smithing are evenly balanced, both in retrogression, but English work has more vitality and probably led where designs are similar. |
| 103 | Constructive (non-decorative) ironwork was increasing in this period and is documented in fabric accounts - Gardner points to the accounts of Notre Dame and the Sainte Chapelle in Paris, and to iron window-guards following the lead lines of glazing (Canterbury, attributed to William of Sens, killed by a fall from the scaffolding in 1179; also Chartres, Le Mans, and the rose window of Notre Dame de Dijon). |

### R4. Chapter V text — stated findings

| Page | Finding |
|---|---|
| 115 | THESIS. The third mediaeval period is defined by a shift of the *dominant tool* away from the forge: heat is demoted to a preparatory stage and the finished form is produced by cutting, filing and embossing cold metal. This is the mechanism by which the locksmith displaces the blacksmith. |
| 115 | THESIS, corollary. The required competence becomes cross-trade: the ironworker must be an architect/planner AND locksmith AND armourer AND jeweller. |
| 115 | THESIS, national causes. England drops out because of (a) change of fashion and (b) the civil wars; France keeps going because invasion fear lifted, and absorbs locksmith/armourer method into smithing. |
| 116 | MATERIAL SHIFT. Sheet iron becomes the general stock. Doors are strengthened with thin interlaced hoop-iron bands fixed to the wood by rosettes or decorated nails - the origin of the lattice/diaper door. |
| 116 | TECHNIQUE/VOCAB. 'Vandyked' edges - zig-zag/dagged sheet edges - are copied directly from contemporary plate armour; also used as thin edging on long hinge straps (Auxerre). |
| 116 | CONSTRUCTION. Traceried hinges were built up from SEVERAL THICKNESSES of pierced sheet riveted together in strong frames - not cut from one plate. |
| 116 | GENERATOR DIAL. Richness of pierced grillework is achieved by stacking pierced sheets: earliest examples one thickness, then three or four superimposed. |
| 117 | TECHNIQUE HIERARCHY in grand work: the structural/leading lines (crockets, pinnacles, tracery ribs) are CHISELLED AND FILED FROM THE SOLID in full relief; pierced sheet is only the subordinate infill/background. |
| 118 | QUALITY/VALUE. Locks reached goldsmiths'-work refinement; over 1000 pounds had been paid for a single lock by 1893. |
| 118 | DATING RULE for locks: best DESIGN at the close of the 15thC; richness keeps increasing through the 16thC. So 'richer' does not mean 'better' or 'later-good' - richness rises while design quality falls. |
| 118 | LOCK GENERATOR. Basis = flamboyant architecture of the period; focal content = figures of saints and kings, masks, escutcheons of arms, chiselled from solid iron and chased as if in silver. Named subject programmes: the twelve apostles under canopies, the Garden of Eden. |
| 118 | MOUNTING CONTEXT + mechanism. These locks were fixed to richly carved presses, trunks and doors, and the mechanism deliberately conceals bolts and key-holes. Their master-keys have tracery handles and innumerable wards. |
| 119 | RIM-LOCK GENERATOR + long-run dating. Richly ornamented rim-locks appear from the 13thC: front plate decorated with scrollwork ending in 13thC leaves and animals' heads; BACK plate (the fixing plate) extended beyond the lock body and cut into leaves, animals' heads, fleurs-de-lis. Cock's head and eagle occur. Surface: incised lines, and twisted and notched mouldings. The style persists through the 14thC with only slight detail changes. |
| 119 | KNOCKER GENERATOR. Knockers and closing rings 'nearly always' take an animal form, forged, frequently over a pierced traceried back plate. |
| 119 | FRENCH COFFER GENERATOR (diagnostic of France). Sheet-iron body; straps carried from back to front; lid curved like a trunk; front and side panels of tracery formed of THIN PIERCED PLATES LAID OVER EACH OTHER; projecting pieces shaped like buttresses of two or three stages; the angle buttresses lengthened downwards into legs. Close of the 14thC. AND: all were painted and gilt. |
| 119 | Nails are themselves designed objects at this date, of highly varied design. |
| 120 | GEOGRAPHY / TRADE. Flanders and Brabant become the home of the iron industry in the 15thC as England's supremacy slips. Brussels' 14thC iron and steel workers reputed unsurpassed in Europe. Antwerp's port scale is given as 2,500 ships at one time, 500 entering in a single day - and Gardner infers that whatever such commercial towns made was exported and influenced even distant countries. |
| 121 | STYLE-TRAVEL LAW (explicit). A mature style transplanted to a fresh region does not arrive mature - it 'pushes back' into the older, more robust stages and re-runs them quickly. France/England were at the third and least vigorous stage; Flanders restarted at the massive-forging stage and then caught up. |
| 121 | REAL WEIGHT AND DIMENSIONS. The Ghent bombard 'Dulle Griete' / Mad Meg: 16,803 kilos, nineteen feet long, eleven feet in circumference, formed of WELDED COILS, first half of the 15thC, with the arms of Philippe le Bon STAMPED on it. Mons Meg (Edinburgh Castle) reputed made at Mons in 1476; further examples at Basle and Mont St Michel. |
| 121 | DATED, NAMED WORK. Massive market-place rails at Mechlin by Jean de Cuyper, 1531. |
| 122 | STAMPED-HINGE GENERATOR (Hal, Notre Dame, early 15thC) and its diagnostic vine mutation: leaves symmetrically cleft and spined LIKE THISTLE LEAVES; grape bunches OVAL and about the size of a prickly pear; a FACE STAMPED at about the centre of each leaf; straps ending in massive fleurs-de-lis. Massiveness and relief exceed anything made when the style was at its zenith - i.e. a late tour de force, not an early work. |
| 122 | DIMENSION. The traceried lock on the left-hand Hal door is more than a foot in length and must be contemporary with the stamped hinges; its design resembles the grille to the Chantry of Henry V. |
| 122 | TRADE ORGANISATION - the key statement. Josse Matsys of Louvain held the municipal posts of ARCHITECT and CLOCKMAKER as well as BLACKSMITH. The trade is dynastic: his second son Quentin (b.1466) 'followed his father's calling'. |
| 122 | WELL-COVER GENERATOR (Antwerp, dated 1470; Gardner attributes it to Josse Matsys, not Quentin). NO STAMPS used. Canopy carried by four clustered columns supporting four cusped arches converging to a centre; the canopy 'roof' is a tangle of interlacing branches and leaves (probably vine) plus a conventional flower that both DROOPS to form pendants and SOARS upward into pinnacles; crowned by a figure of Salvius Brabo in Roman costume holding a spear and the hand of the giant Antigonus; the arch springings masked by four smaller figures dressed in skins. |
| 123 | The figures-in-the-round on the Antwerp well-cover are called out as exceptional for a blacksmith, and as having permanently changed the art. |
| 123 | OBJECT TYPE + VOCAB. 'Jacquemart' = a large open iron belfry ending in a fleche with two costumed iron AUTOMATA to strike the bells; the Dijon/Notre Dame example known to have been made at Courtrai in the 15thC. |
| 123 | OBJECT TYPE. 'Font-cranes' - ponderous forged cranes for raising the font cover - are conspicuous in Belgian baptisteries, ranging 15th to 17thC (Hal, Breda, Bois-le-Duc, Zutphen, Ypres, Dixmunde), and are otherwise wholly peculiar to Belgium (one Cologne exception). |
| 123 | CANDELABRUM GENERATOR (Belgian churches, usual form). Tripod foot + simple angular stem broken by a knop or moulding + supporting circles OR rows of lights stepped one above another; the circles/bars carry spikes and sockets for candles and are attached to the stem either by brackets or hung from it by straps and scrolls. Variants: Ypres (stem ornamented by trefoil leaves and fleurs-de-lis); Lierre (candles in rows, central spike taking a detachable seven-branched candlestick); Tournai (three tiers pierced with quatrefoils, crowned with leaves). |
| 124 | REAL DIMENSIONS for lighting furniture: candelabra at Deux-Acren and Chapelle-a-Wattines, Hainault, OVER SIX FEET HIGH; the Osnabruck herse-light OVER SEVEN FEET HIGH. |
| 124 | HERSE-LIGHT GENERATOR + painted parts. Massive tripod foot; moulded stem; two spandrel-shaped brackets filled with tracery; on these a VERTICAL TRIANGLE of moulded iron bars filled with rose-pattern tracery and PAINTED IRON SHIELDS; on two sides of the triangle a step-like arrangement of scrolls and spikes for fifteen candles; rings on the foot so it can be moved. |
| 124 | MATERIAL-CHOICE RULE. Permanent and votive coronae, candelabra and sanctuary lamps were normally in MORE PRECIOUS METALS; wrought iron was used for them only during a brief period when iron was intrinsically valued. Most surviving iron examples were for INTERMITTENT use - built to carry very large quantities of tapers for great ceremonials. |
| 124 | FOLDING LECTERN GENERATOR: made on the principle of folding deck-chairs, with leather or pigskin tops; iron legs with worked mouldings, sometimes flower-work, finials shaped into HEADS or FRUITS; book-rails often richly pierced. Same workmanship as the candelabra. |
| 125 | FINISH RULE, general and explicit. Seats, stands, alms-boxes, pulpits, catafalques and hearses of this workmanship were ALL originally decorated in glowing colours, if not partly gilt. |
| 125 | SURVIVAL BIAS. Mediaeval iron church grilles were largely displaced during the Renaissance by carved marble, wood, bronze and brass screens; Belgian churches, less systematically looted than English and French, retain most of the best ironwork. |
| 125 | FLEMISH GRILLE GENERATOR (14th-15thC, typical). Massive upright bars CHISELLED to indicate, slightly but effectively, the carved caps and bases of stonework; the bars form long linear panels with traceried arches. Louvain Hotel de Ville treasury window grilles, 1463, add a band of IMITATION WATTLES in iron over the top of the arch; the same feature twice at Breda. |
| 125 | DATED IMPORT + VOCAB 'slam-bar'. The gates of Bishop West's Chapel, Ely, 1515-1533, are Flemish work in England: upper tier of linear panels of TWISTED bars with FORGED caps and bases and richly traceried arches; lower tier of narrower panels with a base of pierced tracery, a band of Flemish arabesque, traceried arches with fleurs-de-lis and shields; heavy branching interlaced scrolls filling the head, blossoming into TUDOR ROSES instead of leaves; and 'a massive turned and moulded slam-bar' giving a touch of Flemish Renaissance feeling. |
| 126 | TECHNIQUE TERM 'drifted'. Purely protective grilles are made of strong bars drifted through each other, forming lozenge or rectangular interspaces; they are made decorative by traceried designs spanning several interspaces, by roses at every intersection, or by battlemented and spiked cornices/crestings. |
| 126 | RARE GRILLE TYPE. Strong sheet-iron plates pierced into DIFFERENT arabesque designs and let into a rectangular framing - shutter grilles of the tabernacle of the chapel of the counts of Flanders, Ghent, mid-16thC (part in South Kensington); an older one figured by Van Ysendyck. |
| 126 | WORKSHOP PRACTICE - the mechanism behind Brabancon precision. When the roles of smith, clockmaker and architect are combined in one individual, the more precise, cultivated and elaborated TOOLS OF THE MECHANICIAN are brought to bear; the result always leans to architectural forms, producing the effects of wood and stone in iron IN MINIATURE. |
| 132 | QUALITY RANKING (Gardner's, stated twice). Flemish/Brabancon work never quite attained the refinement and delicacy of the French; German productions were as inferior to the Flemish as the Flemish were to the French. Louvain (the Matsys home) is the principal seat of the Brabancon school; there is nothing to indicate production in Antwerp or Flanders proper - and Gardner infers this from DISTRIBUTION: had it been made in the great commercial towns it would now be scattered over Europe. |
| 126 | GUICHET GENERATOR. At St Pierre, Louvain, every armoire in the chancel-aisle chapels is fitted with a small CIRCULAR guichet filled with flamboyant tracery, 'of a great variety of design'. |
| 127 | STANDARDISATION - the explicit statement. The familiar flat Brabancon boxes have small intricate GEOMETRIC tracery REPEATED over cover and sides, often in longitudinal bands separated by plain ridges and binding, and locks with a UNIFORMLY rude, ill-designed buttress-shaped hasp; Gardner infers from that very uniformity that they were produced in ONE CENTRE and abundantly exported. |
| 128 | REPETITION AS STYLE MARKER. The Windsor gates' great amount of repetition is named as a defect COMMON IN PERPENDICULAR WORK. |
| 128 | REAL DIMENSIONS + full parts list, Windsor gates: two gates ABOUT SEVEN FEET HIGH and two much higher hexagonal piers; each gate of three bays separated by buttresses with crocketed niches and finials; each bay = a two-storey traceried window under a three-sided two-storey canopy with feathered arches and crocketed pinnacles, finishing in a parapet of open tracery; the canopy's upper storey recedes and is tied to the lower by FLYING BUTTRESSES; piers planned as four sides of a hexagon (hence double buttresses at each angle), with an extra upper storey of double-light traceried windows and FIVE richly wrought open cressets or lanterns on the continued angle buttresses. |
| 128 | CONSTRUCTION SPEC - the key build recipe. MANY THOUSAND pieces of carefully FILED iron; caps, bases, mouldings, crockets and cusps CHASED OUT OF THE SOLID and TENONED, MORTICED AND RIVETED together AS IN JOINERY; depth and richness given by using ONE THICKNESS UPON ANOTHER over a background of SAW-PIERCED sheet iron. |
| 128 | FINISH. The Windsor gates were ORIGINALLY GILT and still gilt in Gough's day - so completely that the antiquary Gough described them as a work of GILDED COPPER. |
| 129 | VOCABULARY - 'vizzying'. Gardner records 'vizzying' as a period/local term for a guichet (a small viewing wicket), used alongside 'square escutcheon' and 'handle-plate'; the Windsor ambulatory example is a handle-plate in the form of a ROSE WINDOW surrounded by the Garter. Another rich Flemish vizzying, with the wattle border, is in the Museum (Fig.46), and one is preserved at Compton Wynyates from a house of the last years of Henry VII. |
| 129 | REGIONAL DISCRIMINATION RULE. Flemish ironwork was in fashion in England under Edward IV and was certainly imitated in the following reigns; it is distinguished from French only by greater elaboration and different architectural detail, and its locks, handles and door-knockers are STURDIER AND PLAINER than the French, which they rarely rival in taste and refinement. Belgian work of this date is abundant between the Meuse and the Rhine (old Duchy of Cleves) and as far south as Cologne. |
| 132 | DATED GERMAN TRACERY ANCHORS: St Ulric, Augsburg, grille supposed c.1470 - a VESICA-SHAPED DIAPER filled with tracery in which the fleur-de-lis is oft-repeated; tracery screens at Heidingsfeld near Wurzburg, 1510; tracery used more frequently for tabernacle doors. |
| 132 | REGIONAL DIAGNOSTIC (German). Intricate OPEN TRACERY handles of SARACENIC OUTLINE are peculiarly German, as are tracery back-plates and INTERTWINING, LEAFLESS, BRANCHING handles. At Luneburg, strap-hinges crossing a door and richly worked handles form 'a museum of delicate and... finely coloured tracery design' (colour noted by Mr. King). |
| 132 | GUILD PRACTICE - the masterpiece. The 18-inch lock-plate from Maria-Saal, Carinthia (Klagenfurt Museum) is, from its unusual size and elaborate character, regarded as having been a DIPLOMA WORK. |
| 133 | GUILD PRACTICE - the other end. The Cologne font-crane's simple triangular form, feeble vesica tracery and 'unnecessary and defective mechanism' proclaim it the effort of a 'PRENTICE HAND. |
| 133 | COLOUR SPEC (verified across the page break) + guild heraldry. The Cologne 'rastellum' - a light traceried railing with fleur-de-lis cresting, five prickets for candles and five shields blazoned with TAILORS' SHEARS - stands on a rafter exquisitely painted with figure subjects after the Cologne school, 'while the ironwork is rose and blue and gold'. |
| 134 | CHAPELLE ARDENTE GENERATOR (Nonnburg near Salzburg, engraved by Gailhabaud): a roof/catafalque with SIX GABLES supported on TWISTED COLUMNS, filled with tracery and cusps, holding innumerable prickets along its RIDGES AND EAVES, with finial-like candelabra at its angles; beneath it a dwarf railing filled with tracery, also supporting candelabra. |
| 134 | GUILD COMMISSION, dated and signed. The corona made by GERT BULSINCK OF VREDEN in 1489 was PRESENTED TO THE CHURCH BY THE CORPORATION OF LOCKSMITHS. It consists of two most richly pierced SHEET-IRON bands carrying canopied niches with figures of saints, a candle in front of each; at the centre, beneath a wrought canopy, a figure of the Virgin IN GILDED WOOD, with two kneeling figures above. |
| 135 | DATING RULE - the strongest in the chapter. The THISTLE enters German ironwork at the beginning of the 16thC, immediately ousts the vine, and for A CENTURY no ironwork of any pretension was forged in Germany without it. Cologne 16thC work is specifically distinguished from all earlier German work by 'the constant use of the thistle'. In England the thistle is used only rarely (e.g. the choir gate-hinges at Wells). |
| 138 | THISTLE MORPHOLOGY PROGRESSION. In the Kempen chandelier the thistle leaves take 'a definite CRUCIFORM shape, which henceforth characterises them until the final disuse of the plant in ironwork'; earlier (Cologne 1549 wall-brackets, St Columba's candlesticks) the leaves are BOLDLY MODELLED and rounded. Late on, the thistle 'became protean, and simulated the oak, the fan, the Eastern spathe, the fleur-de-lis, the cross, or mere tracery' - and when all sense of the original is lost the derivation is betrayed only by SOME CROSS-HATCHING, the last trace of the calyx. |
| 137 | SURFACE + COLOUR SPEC, the single most important finish statement in the chapter. The thistle formed the basis of all the pierced and SLIGHTLY EMBOSSED sheet-iron, SEA-WEED-LOOKING ornament applied to locks, hinges and handles, 'in which the iron was BRIGHTLY TINNED and LAID OVER RED CLOTH OR PAPER'. The SPLAYED LOCKS peculiar to Germany were often treated thus (Rathhaus of Cologne, of Bingen, and elsewhere on the Rhine). Thousands of examples survive, from the utmost simplicity to extreme richness. |
| 138 | NAMED MAKER + workshop attribution by hand. The richest examples of the tinned pierced thistle work 'seem due to A. F. Butsch', seen in the locks of the Soyter Collection and the chapel at Blutenbourg near Munich. Particularly rich hinges in the Nuremberg Museum are 'embossed to an unusual height, and assuming, as frequently happens, an almost GEOMETRIC arrangement'. A dated example: the thistle terminating the vertical bars of a grille in Freiburg im Breisgau Cathedral, 1538. |
| 139 | GEOGRAPHY + LAG. The pierced/embossed door-LINING fashion (already occasional in France, magnificent at Erfurt) was VERY PREVALENT in Austria, Bohemia and Poland; none of the Austrian examples appear older than the 15thC, and the custom was RETAINED LONG AFTER the adoption of Renaissance architecture. In England scarcely any attempt to make defensive door-linings decorative has been made since the 13thC. |
| 139 | COLOUR SPEC (verified across pp.139-140). The Austrian/Bohemian/Polish door-linings are all characterised by great richness of detail, and 'when illuminated in BLACK AND WHITE, RED AND BLUE, and PROFUSELY GILDED, their effect must have been very splendid'. |
| 140 | DOOR-LINING GENERATOR, richest example (Priory of Bruck, on the Mur). The door is DIAPERED with banded iron, STUDDED WITH NAILS SHAPED INTO ROSETTES, and the interspaces filled with the most elaborately pierced and embossed ornaments of fine German Late Gothic character; thistle and fleur-de-lis twined into arabesques or mingled with tracery of extraordinary diversity, 'FEW OF THE DESIGNS BEING REPEATED'. Cracow (Rathhaus, University) examples are thistle-based and of the latter half of the 15thC. |
| 142 | COLOUR SPEC, exact. On the Bruck door 'the ground of the lozenges was painted ALTERNATELY RED AND BLUE, so that the general effect was like GOLD LACE ON A SCARLET-AND-BLUE CHEQUER.' |
| 142 | COLOUR SPEC + heraldic tinctures. At Karlstein near Prague the armorial bearings are merely PAINTED ON THE WOODWORK between the iron straps: the BLACK EAGLE OF AUSTRIA ON GOLD alternating with the SILVER LION OF BOHEMIA ON RED; the iron straps are fixed by WELL-MODELLED NAILS and decorated with GOLD-AND-BLACK ROSETTES. At Krems, by contrast, the arms are SPLENDIDLY EMBOSSED IN IRON - upper half griffins alternating with a coat-of-arms, lower half diapered with imperial eagles and lions, nails finely worked; same workmanship in Carinthia and Steier, all believed close of the 15thC. |
| 142 | TABERNACLE-DOOR GENERATOR. Main bars ALWAYS MASSIVE though often almost wholly concealed by pierced foliage and arabesques; interstices, RECTANGULAR in the richer examples, filled with carved iron tracery, with FILIGREE, or with pierced and embossed subjects. At Krems the interspace designs are 'cut out of sheet metal, embossed and chased', subjects in part from the New Testament; three similar doors at Znaim 'still preserve their GOLD AND COLOURED DECORATION'. Gardner derives the eastern diapered doors FROM these Belgian/German trellised tabernacle grilles, which 'the richer taste of further east decorated with rosettes and other ornaments at the intersections of the bars'. |
| 143 | DATING RULE - trellis grille bar geometry. In the OLDEST German specimens (Cologne, Aix-la-Chapelle) ALL the diagonal bars running one way are threaded through those running the other way. In the somewhat later, more ponderous Magdeburg example of 1495 they PASS THROUGH EACH OTHER ALTERNATELY, as in the familiar work of a century later - from which the 1495 work differs, however, in being made with SQUARE INSTEAD OF ROUND IRON. Gardner reads this as German smiths 'courting difficulties in order to display their skill'. Later trellises are complicated by rings and other ornaments interlaced in the bars, with traceried cornices, twisted and moulded vertical bars, armorial bearings. |
| 145 | MOTIF DERIVATION + DATING, fleur-de-lis to Passion-flower. A richer terminal than the thistle was needed for standard-bar tops and crestings; this was got by elaborating the ordinary TWELFTH-CENTURY IRIS or fleur-de-lis, which consisted of TWO INNER ELEVATED AND TWO OUTER RECURVED PETALS, AND TWO STAMENS. In German hands it complicated until it resembled the Passion-flower. The FULLY DEVELOPED type belongs to the RENAISSANCE (not the medieval period) and consists of 'a SPINDLE-SHAPED COIL OF WIRE for the pistil, with ELONGATED HAMMER-SHAPED STAMENS, and SLENDER RECURVED PETALS.' |

### R5. All onward sources credited across both chapters

- Viollet-le-Duc — cited in OCR as "Le Duc" (p.116, p.139): figures the interlaced-band door with sheet ornament, the St Bertin (St Omer) overlapping vandyked plates, and pierced/embossed leaf hinges from the Abbey of Poissy (Ile de France) and a house at Gallardon near Chartres. Almost certainly the Dictionnaire raisonné du mobilier français / de l'architecture. HIGH VALUE: source of the actual hinge and door-band drawings.
- Shaw, "Decorative Arts" (p.116) — figures a "splendid flamboyant door" then in private hands. (Henry Shaw, Specimens of the Decorative Arts / Details of Elizabethan Architecture — needs confirming.)
- Du Sommerard, "Arts du Moyen Age" (p.116) — figures two sumptuous GILT pierced panels from the tabernacle of the Abbey of St Loup, Troyes. Relevant to the gilding question.
- Gailhabaud (p.134) — engraved the Chapelle ardente of Nonnburg near Salzburg (roof/catafalque with six gables on twisted columns, prickets along ridges and eaves). Likely Gailhabaud, "L'Architecture du Ve au XVIIe siècle" / "Monuments anciens et modernes".
- Raschdorf (p.135) — figures a Cologne private-collection thistle ironwork example finer than the St Columba funeral candlesticks. Likely Julius Raschdorff, Cologne architectural publications.
- Van Ysendyck, "Belgian Architecture" (p.126) — figures an older pierced-plate shutter grille than the Ghent counts' chapel tabernacle grille. (J. J. van Ysendyck, Documents classés de l'art dans les Pays-Bas.)
- "Mr. King" (p.132) — described the Lüneburg strap-hinges and handles as finely COLOURED tracery design; probably Thomas Harper King. Relevant to the painted/coloured-ironwork question.
- Transactions of the Royal Institute of British Architects, vol. vii, New Series, pp. 160-162 (footnote, p.113) — "Many of these are figured": the German lozenge-leaf vine hinge series (Erfurt, Thann, Oppenheim, Caub, Zülpich, Magdeburg, Oberwesel, Schloss Lahneck). Directly harvestable hinge drawings.
- Gough the antiquary (p.128-129) — his description of the Windsor gates as a work of "gilded copper" is the evidence that they were gilt; a primary antiquarian description worth locating.
- Named makers/attributions to chase: A. F. Butsch (richest pierced-thistle locks; Soyter Collection; chapel at Blutenbourg near Munich, p.137); Gert Bulsinck of Vreden, corona of 1489 presented by the Corporation of Locksmiths (p.134); Josse Matsys of Louvain, municipal architect+clockmaker+blacksmith, and his son Quentin (b.1466) — Antwerp well-cover dated 1470, Louvain font-crane (p.122); Jean de Cuyper, Mechlin market rails, 1531 (p.121).
- Collections holding the assigned locks, for photographic reference: Klagenfurt Museum (fig 48, ex Maria-Saal, Carinthia); Augsburg Museum (fig 50); Amerling Collection, Vienna (fig 51); St George's Chapel, Windsor (fig 45); Neuberg, Styria (fig 49). Also South Kensington Museum (now V&A) for fig 46 vizzying and fig 47 Ottoburg tabernacle grille.
- Viollet-le-Duc (Gardner writes him as 'Le Duc') - cited THREE times in this chapter as the source of published illustrations: a small grille formerly in St Denis (p.100), a grille from Rouen (p.101), and a sketch of the wooden Saracenic diaper at Luxeuil (p.105). Almost certainly the 'Dictionnaire raisonne du mobilier francais' (vol. on serrurerie) and/or the 'Dictionnaire raisonne de l'architecture'. HIGH PRIORITY - a French primary picture source with measured drawings, and it covers both the iron and the wooden versions of the same diaper.
- Chabot - cited on p.101 as having figured two splendid late-fourteenth-century window grilles from Troyes Cathedral, plus another double-window grille from Troyes and a fifteenth-century trellis grille from Nancy. Author's first name and title not given by Gardner; needs a catalogue search (likely a French ferronnerie plate-book).
- Matthew Digby Wyatt - cited on p.103 as having figured the (now vanished) hingework at Hunstanton, Norfolk. Valuable because the originals are lost, so the plates are the only record. Probably 'Metal-Work and its Artistic Application' (1852).
- Transactions of the Royal Institute of British Architects, vol. vii, New Series, pp. 160-162 - given in Gardner's own footnote on p.113 as where 'many of these' German fifteenth-century Rhenish vine-leaf hinges are figured (Marburg, Oberwesel, Neukirchen, Koln, Thann, Oppenheim, Caub, Zulpich, Magdeburg, Erfurt, Schloss Lahneck). A precise page citation - the single most efficient new lead in the chapter for German hingework.
- James Fergusson - cited on p.94 for dating the Delhi iron pillar to about A.D. 400. Presumably 'History of Indian and Eastern Architecture'. Only relevant if we ever want monumental forged columns.
- The South Kensington / art 'handbooks on the Arts of Persia, India, etc.' - Gardner explicitly defers to these for Eastern armour on p.94 ('for an account of this Eastern armour the handbooks on the Arts of Persia, India, etc., must be consulted'). Probably the South Kensington Museum art handbook series. Relevant to the weapons/armour lane rather than architecture.
- M. le Secq des Tournelles - a private collector, twice cited (p.101, and as the owner named in the caption of Fig. 38). His collection became the Musee Le Secq des Tournelles at Rouen, now the largest ironwork museum in the world - a live reference-photography target for the reference-scout lane, not a book.
- Mr. Parker, of Oxford - holds an unpublished sketch from Verona of a grille with quaint animals and quatrefoil ornaments cut from sheet metal and riveted between geometric latticework (p.96). Manuscript/collection lead, probably unrecoverable, noted for completeness.
- The agreement for the Henry V chantry grille, Westminster Abbey, with Roger Johnson of London, 6 Henry VI (1428) - Gardner says it 'is still extant' (p.105). A documentary primary source: a real fifteenth-century ironwork contract, which would give period part-names, quantities and possibly prices. Worth chasing in Westminster Abbey muniments.
- The will of the Black Prince, cited on p.106 as containing 'minute directions' for his monument at Canterbury but NO mention of any grille or rail - Gardner uses the silence to date the railings later. A documentary source for tomb furniture specification.
- F. Liger, "La Ferronnerie" (to 1875, unfinished; Gardner borrowed many illustrations from it — source of this book's figs 1-16, Roman/Gallo-Roman fittings)
- Raymond Bordeaux, "La Serrurerie du Moyen Age" (published by Parker, Oxford, 1858) — a series of plates of medieval hinges in England and France with descriptive text; source of Gardner's figs 25, 26, 33, 39-45 (hinges, knockers, handles). HIGH PRIORITY for a door-fittings library.
- Publications of the Government Printing Office (k.k. Hof- und Staatsdruckerei), Vienna — source of Gardner's figs 48-57, i.e. the Klagenfurt/Styria/Krems/Cracow/Bruck/Prague material including fig 52 and fig 57. The upstream plates will be larger and cleaner.
- Viollet-le-Duc ("Le Duc") — cited for a lozenge-banded door with sheet ornament, hinges from the Abbey of Poissy (Ile-de-France) and from a house at Gallardon near Chartres, plus grilles and window grilles (index: Viollet le Duc 61-63, 69, 79, 100, 105, 116)
- Du Sommerard, "Les Arts du Moyen Age" — figures two sumptuous gilt panels from the tabernacle of the Abbey of St. Loup, Troyes (directly relevant to the tabernacle-door cluster)
- Shaw's "Decorative Arts" — figures a splendid flamboyant iron door then in private hands
- Van Ysendyck, "Belgian Architecture" (Documents classés de l'art dans les Pays-Bas) — figures an older tabernacle shutter-grille from the chapel of the counts of Flanders, Ghent
- Raschdorf — figures a thistle wall-bracket from a private collection in Cologne
- Gailhabaud — engraved the Chapelle ardente of Nonnburg near Salzburg (six-gabled catafalque, twisted columns, tracery, prickets); index: Gailhabaud 69, 73, 134, 164
- Dr. Ludwig Beck (1884) — abridged edition of Liger with an addendum on medieval ironwork
- Jules Garnier, "Le Fer", Bibliotheque des Merveilles, 1878
- Prof. Meyer of Carlsruhe (Karlsruhe) — handbook on "Schmiedekunst", c.1888
- "Blacksmithing" text-book in Weale's rudimentary series
- Du Chaillu, "The Viking Age" (Murray) — source of Gardner's figs 23-24, Norse hinges (Vanga, Faabergs)
- Mr. King — cited for a description of the finely coloured tracery hinges and handles at Luneburg (author/work not named by Gardner; unresolved)
- Le Secq des Tournelles (index: 69, 101) — the Rouen ironwork collection, now the Musee Le Secq des Tournelles
- Mathurin Jousse (index, under Smiths) — 17th-c. author on locksmithing/ironwork
- Parker of Oxford — lent Gardner several illustrations; publisher of Bordeaux above
- Gough the antiquary — described the Windsor gates as gilded copper (early description of the object)
- Viollet-le-Duc (cited throughout as "Le Duc", indexed as "Viollet le Duc", pp.61-63, 69, 79, 100, 105, 116) - source of the figured interlaced hoop-iron door, the St Bertin (St Omer) overlapping vandyked plates, and the late-14thC pierced/embossed sheet hinges from the Abbey of Poissy (Ile de France) and a house at Gallardon near Chartres. Almost certainly the Dictionnaire raisonne de l'architecture francaise. HIGH PRIORITY - it is the upstream source for most of Gardner's French hinge and door material.
- Shaw's "Decorative Arts" (Henry Shaw) - figures a splendid flamboyant pierced door, then in private hands (p.116).
- Du Sommerard, "Arts du Moyen Age" - figures two sumptuous GILT pierced panels from the tabernacle of the abbey church of St Loup, Troyes (p.116). Relevant to the gilding evidence.
- Van Ysendyck, "Belgian Architecture" (p.126) - figures an older pierced sheet-iron arabesque shutter grille than the mid-16thC Ghent tabernacle example. Likely Van Ysendyck, Documents classes de l'art dans les Pays-Bas.
- Gailhabaud (indexed pp.69, 73, 134, 164) - engraved the Chapelle ardente of Nonnburg near Salzburg (p.134). Likely Jules Gailhabaud, Monuments anciens et modernes / L'architecture du Ve au XVIIe siecle.
- Raschdorf (p.135) - figures a rich thistle-ornamented example from a private collection in Cologne. Likely Julius Raschdorff.
- Gough, the antiquary (p.129) - described the St George's Chapel, Windsor gates while still gilt, mistaking them for gilded copper. Richard Gough, Sepulchral Monuments in Great Britain. Useful as an independent 18thC witness to the gilding.
- "Mr. King" (p.132) - described the Luneburg strap-hinges and handles as 'finely coloured tracery design'. Full name not given in the chapter; probably Thomas Harper King (writer on Belgian/medieval architecture). UNCERTAIN attribution - flag before chasing.
- Transactions of the Royal Institute of British Architects, vol. vii, New Series, pp.160-162 (footnote on p.113, immediately before chapter V) - figures many of the German lozenge-leaf Rhenish hinges (Marburg, Oberwesel, Neukirchen, Kolin, Thann, Oppenheim, Caub, Zulpich, Magdeburg, Erfurt).
- COLLECTIONS named as holding chapter-V material (for photographic reference-harvesting): South Kensington Museum (now V&A) - casts of the Hal doors and the Hal font-crane, the Flemish vizzying Fig.46, the Ottoburg tabernacle grille Fig.47, part of the Ghent counts-of-Flanders shutter grille, two trellised tabernacle doors, French sheet-iron coffers; British Museum - French coffers; Sauvageot Collection, Louvre - portable screen; Klagenfurt Museum - the 18 in Maria-Saal diploma lock (Fig.48); Augsburg Museum - chest lock (Fig.50); Nuremberg Museum - high-embossed thistle hinges; Amerling Collection, Vienna - splayed lock (Fig.51); Soyter Collection - A. F. Butsch locks.
- PRIMARY LEAD FOR THIS CLUSTER: 'publications of the Government Printing Office, Vienna' (k.k. Staatsdruckerei) — the stated source of figs 48-57, i.e. all four door-linings (Cracow, Bruck, Karlstein, Krems) plus the Krems tabernacle door and the Styria/Augsburg/Amerling/Klagenfurt locks. Original Austrian state-press plates would be far more detailed than these 1893 re-engravings.
- Transactions of the Royal Institute of British Architects, vol. vii, New Series, pp. 160-162 — cited in a footnote on printed p.113 as figuring many of the Rhenish/Erfurt-type hinge and diaper doors.
- M. Liger, 'La Ferronnerie' — source of Gardner's figs 1-16; also cited for Roman trellis-plus-scale-pattern door at Wiesbaden and Pompeian chests.
- Raymond Bordeaux, 'La Serrurerie du Moyen Age' (Parker, Oxford) — source of figs 25, 26, 33, 39-45 (handles, knockers, hinges).
- Du Chaillu, 'The Viking Age' (Murray) — source of figs 23-24.
- Worsaae, handbook on Danish Art — cited for the golden horns whose ornament parallels the Stillingfleet/Staplehurst door figures.
- Raschdorf(f) — cited on printed p.135 as figuring a fine thistle ironwork example from a private collection in Cologne.
- 'Le Duc' (i.e. Viollet-le-Duc) — cited as having sketched the wooden counterpart of the Canterbury Saracenic strap diaper at Luxeuil.
- SITES TO CHASE FOR BETTER PHOTOGRAPHY OF THIS DOOR FAMILY: Erfurt Cathedral (interior diaper, mid-15th c.); Coutances Cathedral central door and Mont St Michel (French late-13th c. moulded-strap diagonal trellis with stamped rosette nail-heads); Rathhaus and University of Cracow; Priory of Bruck-an-der-Mur; Castle of Karlstein (Karlstejn); suppressed monastery at Krems and the Hospital Church at Krems; Znaim (Znojmo) — three tabernacle doors retaining gold and colour; Magdeburg 1495; Cologne and Aix-la-Chapelle (earliest threaded trellises); Carinthia and Steyr; and for the English circle-intersection family: Skipwith, Hormead near Buntingford, Durham north doors.

### R6. Caveats recorded by the readers

- All five assigned figures were opened as JPEGs at native resolution (1377 px wide, the maximum this scan offers) and additionally cropped with sips into figure bands and detail tiles, so all five are marked VERIFIED. Every quoted passage from pp.115, 128, 132 was additionally re-read from the page image, not just OCR; the p.137 and p.129 quotes were read directly off the full-page images. Quotes from pp.116-127 and 135-140 are OCR-derived and I have not image-verified those specific lines — treat their wording as ~99% but not proof-read.
- FIG 48 UNCERTAINTY: the element on the centre axis below the round cusped boss — a raised, curled tongue/leaf shape with a deep dark central slot — is at the limit of the woodcut's resolution even at 4x upscale. I can see it is applied, stands proud, and overlaps the tracery. I cannot determine whether it is (a) a swivelling keyhole cover, (b) the keyhole itself set in a curled leaf mount, or (c) a purely decorative pendant. Correspondingly I cannot say for certain whether the trefoil-pierced roundel above it is the keyhole or a decorative boss. The book's prose gives no help — it describes fig 48 only by size, provenance and 'diploma work' status.
- FIG 50 GROUND UNCERTAINTY: the main field shows a fine, very regular vertical striation. Because fig 50 is a screened halftone from a photograph (not a line cut), I cannot tell whether this is a file-cut/combed striated iron ground on the object, the weave of the red cloth/paper backing the text describes, or a printing screen artefact. Do not commit a shader to it without a modern photograph of the Augsburg piece.
- NO MECHANISM IS SHOWN ANYWHERE. All five figures are exterior plate/case views. There is no ward arrangement, no bolt, no tumbler, no spring, no hasp and no shackle visible in fig 45, 48, 49, 50 or 51. The book itself explains why (p.118: mechanism 'concealing bolts and key-holes with great skill') but gives no sections or exploded views in this chapter. Anything the factory needs about internal warding must come from another source.
- TEXT/FIGURE COUNT DISCREPANCY: p.137 says "Four typical examples are illustrated (Figs. 49-51)" — but only three figures are numbered in that range (49 and 50 on p.136, 51 on p.137). Either the author counted fig 52 (handle, St Marein, p.138), or counted the two halves of one cut, or the sentence is simply loose. Flagging rather than resolving.
- NEGATIVE FINDINGS, checked by keyword scan across leaves 126-155 (printed pp.113-142): there is NO mention of etching and NO mention of cast iron anywhere in this chapter. The finishing processes named are gilding (pp.116, 119, 125, 128, 129, 134, 140), tinning (p.137 only), and painting/illuminating in colour (pp.132, 139, 140). The word 'cast' appears only for plaster casts held by the South Kensington Museum, never for cast iron. 'Ward' occurs as a lock term only at p.119 ('innumerable wards'); other hits are 'toward/afterwards'. 'Hasp' occurs only at p.127. Chisel/file/saw are named at pp.115, 117, 126, 128.
- REAL DIMENSIONS in this chapter are scarce: lock-plate 18 inches high (fig 48, stated in both caption and text); traceried lock on the Hal door 'more than a foot in length' (p.122); Windsor gates 'about seven feet high' (p.128); Osnabrück herse-light 'over seven feet high' (p.124); Deux-Acren candelabra 'over six feet high' (p.124); Ghent gun Dulle Griete 16,803 kilos, 19 ft long by 11 ft circumference (p.121). No thicknesses, no keyhole dimensions, no plate gauges anywhere.
- ATTRIBUTION CAUTION: fig 45's provenance in the prose is soft. The page says St George's Chapel 'boasts two other fine flamboyant locks of perhaps Brabançon workmanship (Fig. 45)' — so 'perhaps Brabançon' is the author's hedge, and the caption asserts only the location. The wider Windsor attribution to Edward IV and Quentin Matsys is explicitly labelled tradition, not fact ('tradition does no more than connect them').
- MONOGRAM READING on fig 45 is uncertain: small marks at the lower corners of the block read approximately 'P.H.D.' (left) and 'O.J' (right). Low confidence — could be 'P.H.D.' / 'O.T.' or similar draughtsman/engraver signatures.
- The 'splayed lock' type name (p.137) is the book's own term and is the label that ties figs 48, 50 and 51 into one family: a plate that flares outward toward its base with concave sides. I am inferring — not quoting — that fig 48 belongs to that family, since the text applies 'splayed' explicitly only in the p.137 paragraph illustrated by figs 49-51. Fig 48's silhouette is unambiguously of the same flaring type.
- Figure-number error IN THE BOOK: on printed p.110 Gardner cites '(Fig. 41)' for 'the small bar-handles ... attached at each end to the doors by traceried plates or rosettes', but Fig. 41 (leaf 124) is captioned 'Handle in Westcott Barton Church' and shows a large vertical OVAL RING on a cusped strap plate, not a bar-handle. The bar-handle with a central knop is Fig. 42. Do not trust figure citations in that passage.
- Caption/text mismatch for Fig. 38: the plate caption (leaf 115) reads 'Grille belonging to M. Le Secq des Tournelles', while the text on p.103 refers to it as the grille 'said to be from the house of Jacques Coeur, at Bourges'. Both are compatible (Bourges provenance, Le Secq des Tournelles ownership) but the provenance is hedged by Gardner himself ('said to be'). Separately, p.101 describes a DIFFERENT Le Secq des Tournelles grille (quatrefoil bows through jesters' bells) - do not conflate the two.
- The heart motif in Fig. 38 is almost certainly a rebus on the owner's surname (Jacques COEUR). Gardner does not make this point; the reading is MY INFERENCE, not the book's.
- The Oriental-influence thesis is weakly evidenced. Gardner offers no Eastern ironwork as a model; his comparanda for the two 'unquestionably Eastern' English designs are French/Norman JOINERY (St Pierre at Caen, Luxeuil). He concedes iron was barely used architecturally in the East and elsewhere in the same chapter claims decorative architectural ironwork ORIGINATED IN ENGLAND. The Delhi pillar (India, c. A.D. 400) is adduced but is irrelevant to Saracenic Europe. Treat 'Saracenic'/'Oriental' in this book as a style label for geometric interlace and diaper, NOT as sourced provenance. Do not propagate 'Saracenic origin' as fact in asset metadata.
- Gardner has a visible English-priority bias that runs against his own thesis (p.95 'originated in England'; p.113 'England probably led the way'). Discount his direction-of-transmission claims generally.
- OCR problems flagged, unverified against images: the Della Scala date renders as '137 S' (read as 1375) and the maker in the footnote as 'Bovinio di Campilione, 1380' - very likely Bonino da Campione, but I could NOT confirm the spelling from a page image. Also garbled in OCR: 'pierced shield of arras' (p.97, almost certainly 'arms'), 'gravmg' (graving), 'Erliirt' (Erfurt), 'Ch§,lons-sur-Marne' (Chalons), 'Trocaddro' (Trocadero), 'Delia Scala' (Della Scala), 'St Anastasia's' Verona, 'Zulpich'.
- Verification status: I read leaves 111, 115, 122, 123, 124 at full resolution (plus crops), and additionally verified the printed text of p.104 and p.105 against page images (leaves 117, 118) because those two pages carry the chapter's hardest dimensions and its best construction spec. All quotes attributed to pp.98, 102, 104, 105, 109, 110, 111 are image-verified verbatim. Quotes from pp.93-97, 99-101, 103, 106-108, 112-114 are from the djvu OCR only and are marked 'stated' rather than image-verified - they read cleanly but I did not look at those page images.
- No hOCR derivatives exist for this item; all text came from _djvu.xml. Leaf = printed page + 13 held throughout the chapter (confirmed at p.98->111, p.102->115, p.109->122, p.110->123, p.111->124, p.104->117, p.105->118).
- Figs 43 and 44 (Rouen p.117, Evreux p.118) named in my assignment brief fall OUTSIDE Chapter IV - Chapter IV ends on printed p.114 and Chapter V ('The Age of the Locksmith') begins on p.115 (leaf 128). I did not mine them.
- The engraver signatures I read on the figures are 'D & H' (Figs 40, 41, 42) and an illegible scratched name on Fig. 39 that looks like 'J. LEWIS' plus a second word. I cannot confirm either attribution.
- No dimensions are given anywhere in the chapter for knockers, handles, escutcheons or hinges - only for grille bar stock (1/2 in half-round at St Alban's; 3/4 in flat strap at Salisbury) and for the Delhi pillar. Any prop sizes we choose for the small door furniture will be our own invention, not sourced from this book.
- LEAF MAPPING CONFIRMED for this cluster: leaf = printed page + 13. Verified independently on leaf 143 (p.130), 144 (p.131), 151 (p.138), 152 (p.139), 155 (p.142), 156 (p.143), 157 (p.144). No ghost leaves encountered.
- FIG 57 LEAF RESOLVED: the List of Illustrations OCR "14^^" is printed page 144, and fig 57 is on LEAF 157. Verified by reading the page: the header reads "144 IRON." and the caption reads "FIG. 57.—Tabernacle door in the sacristy of the Hospital Church in Krems. Late fifteenth century." It is the last figure in the book.
- One text_finding is tagged printed_page 144 for the List-of-Illustrations credit note (figs 48-57 from the Vienna Government Printing Office). That note is actually on LEAF 13, printed page x (roman-numbered front matter), NOT on printed page 144. I used 144 only because the schema requires an integer page. Flagging so the page is not cited wrongly.
- All four assigned figures are marked VERIFIED: each was read as a full page at 1378x2157 with the Read tool, and each was additionally read as 2-6 sips crops (several upscaled 2-3x) to resolve mouldings, weave, rivets, hinges and lock. Nothing in the figure descriptions is inferred from OCR or caption alone.
- FIG 46 — NO LOCK OR HINGE. The assignment asked for hinges, lock and finials on this object. I looked specifically and there are none: no pivot, hinge, latch, keyhole or handle appears anywhere in the engraving. The iron frame is fixed into the wooden door by short square lugs on both stiles. As illustrated it is a fixed grated aperture, consistent with "vizzying" meaning a viewing hole. Do not model it as an opening wicket on the authority of this figure.
- FIG 46 CORNICE band (c): I describe the broad running band as interlocked hooked C/crescent forms reading as a stylised wave or vine. I could not determine at this scan resolution whether it is a true two-strand interlace, a Vitruvian scroll, or a running vine. The battlemented cresting, the bead row and the cabled roll below it ARE unambiguous.
- FIG 47 RIGHT-JAMB FITTING: a small applied rectangular plate with two slot-like marks. It is at the right position and height for a lock keeper or keyhole escutcheon, but the engraved area is only about 40x90 px at the largest available width and I cannot confirm it is a lock. Reported as uncertain. There is also a small rectangular staple at the door foot that may be a bolt keep.
- FIG 47 PLAN: I read the plan as a canted three-faced front (half-hexagon), inferred from three gables, four angle uprights, and the stepped/angled plinth. This is an inference from a single three-quarter view, not a stated fact. The count of 7 vertical spikes (4 angle pinnacles + 3 gable finials) is a direct observation.
- FIG 47 PROVENANCE: caption says "from Ottoburg, Tyrol" but the prose on p.132 hedges — "said to have come from the chateau of Ottoburg". Record as traditional attribution. Modern spelling may be Ottenburg/Ottoburg; I did not resolve it. OCR renders chateau as "chiteau".
- FIG 52 CONCENTRIC ARCS: two fan-like sets of concentric arcs flank the central staple. My first reading was wear-arcs from the swinging handle; I rejected that because they are bilaterally symmetric, and because Gardner (p.138) lists "the fan" among the forms the thistle simulated. I now read them as deliberate ornament, but this is an interpretation, not a certainty.
- FIG 52 DROP OUTLINE: I describe it as heart-shaped/shield-shaped (rounded shoulders, incurved sides, pointed base). Gardner gives no shape term for it. "Heart" is my word, not the book's.
- FIG 57 LOCK: seen partly in profile because the door is drawn in slight perspective. I can resolve a rectangular box case on the stile, a pointed-oval (vesica) plate outboard of it, and a short projecting cylinder with a shaped end. I cannot tell whether that cylinder is the shot bolt or a turn-knob/key spindle. Flagged uncertain.
- FIG 57 SUBJECT IDENTIFICATIONS are my readings of a small line-block: Resurrection (gable centre), St George and dragon (upper right), Crucifixion (centre), Agony in the Garden (right of centre), huntsmen with dogs and a stag (lower registers). Gardner only says the subjects are "taken, in part at least, from the New Testament" and names none. Treat the individual identifications as inferred.
- FIG 57 REGISTER COUNT: I counted 3 columns x 6 registers plus a gable register, about 19-20 panels. The gable register is partly cut by the canted corners so the exact panel count there is arguable.
- OCR GARBLING encountered and how I handled it: "carried but" -> read as "carried out" (p.116, emendation flagged in the finding); "Brabangon" -> Brabancon; "chiteau" -> chateau; "coronse" -> coronae; "14^^" -> 144; "rastellura" -> rastellum; "Bresgau" is printed thus (for Breisgau). The fig 52 caption spelling "St. Marein, in Styria" was read directly off the image at full resolution, so it is confirmed, not OCR.
- DIMENSIONS: NO measurement is given for any of the four assigned figures. The only absolute dimensions anywhere in leaves 141-152 are the Maria-Saal lock-plate at eighteen inches high (p.132) and the Windsor gates at about seven feet high (p.128). Any scale used for figs 46, 47, 52, 57 must come from elsewhere (the V&A/S.K.M. object records for figs 46 and 47 would be the place to look).
- ARCHITECTURAL-MINIATURISATION ANSWER, split by object, because it is not uniform: fig 47 = YES, heavily (gable, crocket, finial, pinnacle, buttress with set-off, cresting, ogee arch, cabled moulding, quatrefoil, cusped lancet arcading, moulded plinth, shield). Fig 46 = YES, moderately (battlemented cresting, reticulated tracery with quatrefoils, crockets, cusping, moulded caps/bases, bead and cable mouldings, shield) but with one non-architectural intruder, the woven wattle band. Fig 57 = LARGELY NO (only the canted gable head; no tracery, crockets, pinnacles or buttresses at all). Fig 52 = NO (flat pierced thistle sheetwork, zero architectural elements). So the shared stone/iron ornament library covers the tracery idiom (figs 46, 47) but a SECOND, independent generator family is needed for the pierced-sheet foliage idiom (fig 52) and the bar-armature-plus-figure-applique idiom (fig 57).
- SCOPE: chapter V runs printed pp.115-145 (leaves 128-158). Leaf 159 is blank; leaf 160 begins the index. The chapter ends mid-topic - Gardner explicitly defers the developed Passion-flower to 'our second volume', so the Renaissance continuation is NOT in this book.
- ASSIGNMENT: I read the PROSE. The `figures` array is a by-product - I marked VERIFIED only the seven figures I actually looked at at full resolution (Figs 43, 44, 48, 51, 53, 54, 56), and two of those (Figs 53 and 56) I inspected only as a cropped band, which I have noted in their observations. Figs 45, 46, 47, 49, 50, 52, 55, 57 are PLATE - caption and surrounding text only, not inspected. Defer to the figure readers.
- EVIDENCE STANDARD: every quote in text_findings that carries a technique, colour, dimension or dating claim was checked against the page image at width 1700 - specifically pp.115, 116, 117, 118, 119, 121, 122, 124, 125, 126, 127, 128, 132, 133, 137, 139, 140, 142, 143, 145. Quotes from pp.120, 123, 130, 131, 134, 135, 136, 138, 141, 144 rest on the djvu OCR only; that OCR proved accurate everywhere I checked it, but treat those as one notch weaker.
- OCR ERRORS I CORRECTED AGAINST THE IMAGES: 'Dulle Gribte' -> 'Dulle Griete'; 'Matsya family' -> 'Matsys family'; 'dated r47o' -> 'dated 1470'; 'over ;Eiooo' -> 'over [pound sign]1000'; 'Brabangon' -> 'Brabancon'; 'sted' -> 'steel'; 'Lifege' -> 'Liege'; 'Erliirt' -> 'Erfurt'. I have NOT silently normalised 'emblasoned' (p.133, shields blazoned with tailors' shears) - that spelling IS what is printed.
- GARDNER'S RANKINGS ARE OPINION, NOT MEASUREMENT. 'German productions were as inferior to the Flemish as these were in turn to the French' (p.132), 'intolerably coarse' pinnacles, 'feeble' tracery, and the repetition-as-defect judgement on Perpendicular work are 1893 aesthetic verdicts. They are useful as a craftsmanship-tier ladder for asset variants but should not be reported as historical fact.
- SOME ATTRIBUTIONS ARE FLAGGED AS UNCERTAIN BY GARDNER HIMSELF: the Antwerp well-cover is popularly given to Quentin Matsys but Gardner argues it must be Josse's (Quentin was 10-12 when it was finished, and there was a second Quentin, Josse's son, b.1466, who followed the trade); the Windsor gates' connection to Edward IV and to Quentin Matsys is 'tradition' only; Mons Meg is 'commonly reputed' made at Mons in 1476; the Osnabruck herse-light is 'probably' of Belgian origin; the Ely gates' Flemish origin Gardner asserts as beyond doubt but the Matsys attribution is tradition.
- THE 'THREE PERIODS' FRAME IS GARDNER'S OWN PERIODISATION (smith / transition / locksmith-and-armourer). His 'third and least vigorous stage' language is evaluative; the underlying technical claim (cold cutting displaces hot forging as the primary shaping operation) is the part that is factually load-bearing for us.
- DIMENSIONS ARE SPARSE AND MIXED-UNIT. Everything real that appears in the chapter: Dulle Griete 16,803 kilos / 19 ft long / 11 ft circumference (p.121); Hal lock 'more than a foot in length' (p.122); Deux-Acren and Chapelle-a-Wattines candelabra 'over six feet high' (p.124); Osnabruck herse-light 'over seven feet high' with 15 candles (p.124); Windsor gates 'about seven feet high' (p.128); Maria-Saal lock-plate 18 inches high (p.132); Hal stamped hinges c.250 leaves is from chapter IV p.114 (Schloss Lahneck), not chapter V. There are NO thicknesses, NO bar sections in inches, and NO weights for anything but the gun.
- 'VIZZYING' (p.129) is a genuinely unusual term and Gardner gives it in quotation marks both times, glossing it as 'guichet'. I could not corroborate it elsewhere in this book beyond the index entry (Guichet, pp.116, 126, 128, 129). Treat it as a real but rare/local usage that Gardner is recording, not as standard vocabulary.
- The claim that architectural ironwork was painted and parcel-gilt rather than bare is INDEPENDENTLY CORROBORATED five times inside this one chapter (p.119 coffers 'all were painted and gilt'; p.124 'painted iron shields'; p.125 'decorated in glowing colours, if not partly gilt'; p.128 Windsor 'the whole was originally gilt' and mistaken for gilded copper; p.133 'the ironwork is rose and blue and gold'; pp.139-140 'black and white, red and blue, and profusely gilded'; p.142 'gold lace on a scarlet-and-blue chequer'; p.142 Znaim 'still preserve their gold and coloured decoration'). This is the chapter's strongest and most repeated finish signal.
- SEPARATE AND DISTINCT FINISH for German small hardware: brightly TINNED pierced sheet over RED CLOTH OR PAPER (p.137). This is not paint and not gilding - it is a white/bright metal fretwork over a coloured textile ground. Do not conflate it with the polychrome-and-gilt architectural finish.
- MOST REPEATABLE OF THE FOUR, ranked for generator value: (1) FIG 55 KARLSTEIN — best by a wide margin. Exact 45deg lattice, one lozenge size, exactly two nail types, a strictly periodic border (whirl + 2 crosses, period 3, whirl locked to corners), and only two painted charges alternating. The iron is trivial geometry and all variety is 2-colour paint, so one lattice + 2 nail meshes + 2 charge decals reproduce an entire door. (2) FIG 56 KREMS — also highly repeatable: only 1 boss + 4 figure plates for a whole door, but the band angle is ~54deg (not 45) and the crossing/interspace cells are different sizes, so the tiling needs a rhombic lattice rather than a square one. (3) FIG 53 CRACOW — one plate motif in 2 alternating variants, plus a running-scroll border that must be swept along a curved outline; the bare-crossing / midpoint-nail rule is unusual and must not be defaulted to the more common ornament-at-crossings. (4) FIG 54 BRUCK — LEAST repeatable by design: every interspace plate differs, so it needs a generative grammar, not instancing. Its tiling is the simplest of the four but its content cost is the highest.
- ALL dimensions I report are pixels on the 1377px-wide Internet Archive page scans, not real-world units. The book gives NO physical dimensions for any of these four doors. The only absolute iron sizes anywhere nearby are for grilles on printed p.104 (half-inch half-round bars; three-quarter-inch flat straps).
- These are 1893 wood-engravings copied from earlier Vienna plates, i.e. two generations of redrawing. Angles and pitches I measured (Cracow 45deg, Bruck 51deg, Karlstein 46deg, Krems 54deg) are properties of the ENGRAVING and may not be metrically faithful to the ironwork. The 45deg readings for Cracow and Karlstein are safe; the 51deg and 54deg readings for Bruck and Krems are consistent across two independent measurements each but should be treated as approximate.
- Figs 53 and 54 are explicitly 'part of' their doors, so overall door proportions, sill treatment, hinge positions and any cresting are UNKNOWN for those two. Fig 55 is the only complete door. Fig 56 shows no edge at all.
- MY MOST INFERENTIAL CLAIM: for fig 56 (Krems) I read the plain chevron-hatched lozenges as the CROSSINGS of very broad bands, and the figured lozenges as the interspaces. This is an inference from measurement — the boss lattice (x-period 499-510, y-period 347, offset rows) only closes if the bosses sit at crossings, and I independently confirmed that the plain cells (190x283px) are measurably SMALLER than the figured cells (309x411px), which a single uniform lozenge diaper could not produce. But an alternative reading — that the surface is a diaper of two different plain and figured plate sizes with no bands at all — cannot be fully excluded from a wood-engraving.
- Fig 54's strap width (w~43px) and hence its derived lozenge dimensions are the weakest numbers in this report; the cable-edged strap margins make the edge position ambiguous at engraving resolution. The lattice vector (163,201) is solid; the width is +/-15%.
- AMBIGUITY IN THE BOOK'S OWN WORDS for Karlstein: 'The iron straps are fixed by well-modelled nails, and decorated with gold-and-black rosettes.' I cannot determine from the text which of the two stud types I see is the 'nail' and which the 'rosette'. My reading is that the cross-in-roundel discs are the structural nails and the whirl/pinwheel bosses are the gold-and-black rosettes, because the whirls are larger, more plastically modelled and fall at the crossings and corners. This is INFERRED, not stated.
- The small groups of short engraved strokes below each Krems boss are probably an engraver's relief/shadow convention, but I cannot rule out that they represent small punched ornament on the band. Flagged as uncertain.
- Cracow's two alternating cruciform plate variants are clearly distinguishable in the engraving and their checkerboard phase is consistent across the sample I measured, but with only ~6 whole plates fully visible I cannot prove the alternation holds over the whole door. Treat 'plate_variant_count = 2, checkerboard' as observed-in-figure rather than certain.
- OCR is garbled in places: 'Erfurt' appears variously as 'Erfürt' and 'Erliirt'; the figure-list entry for fig 55 renders as 'S5-— " Prague'; 'Fig. S3' for 'Fig. 53'. I verified all four printed page numbers and captions directly against the page images, so the leaf mapping (printed page + 13) is confirmed for 139->152, 140->153, 141->154, 143->156.
- The book calls the Karlstein bird 'the black eagle of Austria'. The engraving unambiguously shows a DOUBLE-HEADED eagle, which is the Imperial rather than the Austrian eagle. I am reporting what the image shows alongside what the book says; do not silently follow Gardner's label.

