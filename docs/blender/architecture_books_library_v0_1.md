# Public-domain architecture & ornament book library — v0.1 (reference scout lane)

2026-07-29 · Ace asked for license-free "ancient architecture books — filigree, cathedrals, houses, anything like that." Three parallel verification passes over Internet Archive / Gutenberg / Gallica / Heidelberg / NYPL / HathiTrust; **~80 works verified with working scan URLs and captured rights statements**. Manifest: [reference_manifests/architecture_books_v1.tsv](reference_manifests/architecture_books_v1.tsv) (families: cathedral_construction, gothic_detail, cathedral_monographs, general_history, medieval_houses, tudor_halftimber, cottages_vernacular, construction_trades, ornament_grammar, metalwork_filigree, goldsmith_treatises, classical_treatises). Sibling of the museum-object manifests — those are photos of *objects*; this shelf is **measured drawings, construction geometry, and technique procedure**, which objects can't give us.

## 1. Why this shelf matters to the factory

The 19th century did our homework: working architects published *dimensioned* survey drawings of exactly the buildings we model, because that's how the profession learned before photography got cheap. That means: molding cross-sections ready to become sweep profiles, scaled plans/sections for kit proportions, vault setting-out geometry for the cathedral kits, framing diagrams for half-timber, and workshop treatises describing how period metalwork was actually made (which is what makes generated detail read as *constructed* rather than decorated — the MEASURED_CONSTRUCTION_DOSSIER argument, in book form).

## 2. Top shelf — start here, by factory need

| Need | Book | Why it wins |
|---|---|---|
| Everything medieval-construction | **Viollet-le-Duc, Dictionnaire raisonné de l'architecture** (10 vols) | Thousands of cutaway construction woodcuts. The whole work is **transcribed on French Wikisource with every image extracted** — browsable/searchable by article (Porte, Charpente, Escalier…), no PDF spelunking |
| Gothic construction, in English | **Ungewitter, Manual of Gothic Construction** (Ricker tr., 1920) | THE German construction textbook, translated — masonry coursing, vault setting-out, tracery, roof carpentry, dimensioned |
| Molding profiles → sweep curves | **Paley, Manual of Gothic Moldings**; Parker's Glossary plates vols | Sheets of dated cross-section profiles; nearest thing to a downloadable profile library |
| Scaled church plans en masse | **Dehio & Bezold atlas** (601 plates, Heidelberg) | Largest corpus of comparative measured church plans/sections ever printed |
| Timber roofs | **Brandon, Open Timber Roofs**; **Tredgold, Carpentry** + atlas | Measured roof sections with joints — direct structural-oak kit references |
| Medieval houses | **Turner & Parker** (4 vols); **Dollman, Analysis of Ancient Domestic Architecture** | The core typology + true measured town houses/inns |
| Half-timber vocabulary | **Parkinson & Ould** (Shropshire/Hereford/Cheshire); **Jackson, The Half-Timber House** | 100 collotype close-ups of framing; Jackson teaches sill/post/girt/brace anatomy explicitly |
| Tudor measured folios | **Garner & Stratton v1** (+ 1923 plate selection) | Folio measured plans/elevations/full-size mouldings (v2 not digitized free) |
| Ironwork (hinges→gates) | **Shaw, Examples of Ornamental Metal Work**; **Starkie Gardner, Ironwork I–III** | Shaw = literal modeling sheets; Gardner I = the medieval survey |
| Ornament grammar | **Owen Jones**; **Meyer, Handbook of Ornament**; **Speltz** | Jones = color pattern atlas; Meyer = *constructional* line drawings (geometry, not vibes); Speltz = fastest silhouette lookup |
| Filigree & goldsmithing | see §3 | |
| Classical/parametric rules | **Vitruvius** (Gutenberg, searchable); **Palladio** (Ware 1738, dimensioned plates) | Proportional systems that transfer straight into kit parameters |
| Cathedral-by-cathedral | **Britton, Cathedral Antiquities**; **Bell's Cathedral Series** (Gutenberg) | Bell's numbers: Salisbury 23668, York 19420, Durham 20191, Lincoln 43477, Exeter 19424, Ely 21003, Wells 32280, Gloucester 25682, Hereford 19487, Winchester 20346, Rochester 25084, St Albans 19494, Ripon 25800, Carlisle 19881, Lichfield 37049, St Paul's 25266, Canterbury 43517 |
| Trades & tools | **Moxon, Mechanick Exercises** (1703); **Diderot plates** (11 vols) | Period tool geometry + workshop scenes for every building trade |
| Sleeper find | **d'Espouy, Fragments d'architecture du Moyen âge et de la Renaissance** (1897) | 106 Beaux-Arts measured rendered plates, PD-marked — medieval/Renaissance detail at ornament scale, exactly our register |

## 3. The filigree answer

No standalone pre-1931 filigree monograph exists digitized (verified by title sweep — hits are novels and surgery). The technique knowledge lives *inside*:

- **Cellini, Treatises on Goldsmithing** (Ashbee tr. 1898) — a working master's chapters on filigree, niello, enamel, with tool woodcuts. The single best source.
- **Theophilus** (Hendrie tr. 1847) — 12th-century workshop recipes: wire-drawing, soldering, gilding. Medieval procedure from a medieval hand.
- **Rathbone, Simple Jewellery** (1911) — working drawings of how wirework shapes are actually bent; the "how would a smith build this" sanity check for generated filigree.
- **Castellani** (1862 lecture + 1871 Gems) — the Etruscan filigree/granulation revival firm explaining the technique.
- **Rosenberg, Geschichte der Goldschmiedekunst** — THE technical history (Granulation volume, 1918); only a thin 1908 part is on IA — flagged in §6 as a manual-retrieval target.
- Dated visual exemplars: **Evans, English Jewellery** (Anglo-Saxon filigree onward), **Clifford Smith, Jewellery**, **Luthmer** (Renaissance color plates), **Cheapside Hoard** catalogue (a real excavated jeweller's stock), plus Pollen's V&A handbook.

Pair these with the existing Met/Cleveland object manifests: books give the *procedure and cross-sections*, museum photos give the surface truth.

## 4. Rights discipline (how to read the manifest's license column)

- **The date rule:** US public domain = published before 1931. Every entry qualifies by date; the license column records what the *host* asserts on top of that.
- **Cleanest tiers first:** Getty scans on IA (`NOT_IN_COPYRIGHT`), items with explicit Public Domain Mark / CC0, Project Gutenberg ("public domain in the USA"), BHL/Smithsonian ("public domain" stated). Plain IA library scans often carry no rights field — PD by date, note kept per item.
- **Google-scan re-uploads** (`…goog` ids): PD content, but Google's front matter *requests* personal/non-commercial use. Where a non-Google scan of the same title exists we listed it; the handful of `…goog`-only entries (Addy, Taylor, Davenport) are flagged in-column.
- **Gallica:** marked "domaine public," but BnF's standard terms put *commercial reuse* under BnF licensing/fees. For anything commercial-product-facing, prefer the IA/NYPL/Heidelberg copy of the same work; Gallica stays fine for research and measurement. (Same per-item caution as the Internet-Archive rule in [ai_blender_workflow_extract_v0_1.md](ai_blender_workflow_extract_v0_1.md) §2.)
- **HathiTrust `pdus`:** full view + page downloads from US IPs; whole-PDF needs partner login (affects only Innocent 1916 here).
- US law adds a backstop: faithful reproductions of PD 2D works gain no new copyright (Bridgeman v. Corel) — but our provenance doctrine stands regardless: record host + statement per item, which the manifest does.

## 5. Harvest notes

- IA: `archive.org/details/<id>` → prefer the JP2/full-res downloads over the derived PDF for plate extraction; Getty `gri_` scans are typically 400 ppi.
- Viollet-le-Duc: harvest per-article from fr.wikisource (images already extracted to Commons at full resolution) rather than cropping the PDFs.
- Heidelberg diglit serves IIIF — plate-level max-resolution URLs, right register for A/B plates.
- Multi-volume gotchas recorded in the manifest notes: Britton's IA volume labels are shuffled; Brandon vol 1 copy lacks 2 plates; sibling-id lists are in the notes column.
- Downstream conventions (folder layout, plate crops, STUDY_NOTES citations) follow the existing scout-lane recipe in the reference_manifests — books are cited as `author_year, plate/page` the way museum items are cited by accession.

## 6. Gaps & unverified

- **Not digitized free anywhere:** Sharpe's *Architectural Parallels* (1848); **Garner & Stratton vol 2** (1911) and the 1929 2nd ed. — the 1923 Boston 158-plate selection is the working substitute.
- **Manual-retrieval targets:** Rosenberg's *Granulation* volume (1918) — HathiTrust record 103052820 and Google Books qQ1aAAAAYAAJ existed but bot-blocks/geo-redirects prevented confirmation; Wickes' *Spires and Towers* (the measured-spires book) has a LoC record (item 2008570565) whose page 403'd — both are worth a human-browser check.
- **On-page rights lines unverified** for Gallica and Heidelberg (both 403 automated fetchers); statements above are the hosts' documented standards — spot-verify at harvest time.
- Castellani 1862 attribution (IA catalogues it "Anon"), d'Espouy 1925 vol-2 identity, and Diderot per-volume trade mapping beyond charpente=t.2: recorded as likely-but-unconfirmed; ARTFL's plate browser resolves the Diderot mapping on use.
