# Asset Library audit — weapons factory, v0.1

2026-07-28 · Mac planning session. Audited against the stated goal: the Inlay Reference Library's museum photographs (Met + Cleveland, CC0) as the quality bar. Evidence: all 45 Asset Library figures (9 asset sets), all 10 Visual Proof plates and captions, all 31 reference photos and study notes, the eight distilled field-note rules. Every claim below is from direct inspection of the renders, not the captions.

## Verdict

The factory has built an excellent **instrument layer** and a weak **visual layer**, and the Asset Library currently presents the first as if it were the second. Mass solving, measured donors (PAS find 826275 as a guard source is genuinely great), distal-taper tables, deterministic topology with quoted debts, the bowtie lesson ("passed every gate and was still wrong… the render caught it") — this is real engineering discipline, ahead of most pipelines, and none of it is visible *as beauty* in a single plate. Held next to Met 32.75.225 or the Suleyman yatagan, every render in the library reads as a toy: not because the geometry is unmeasured, but because the plates show LOD-resolution geometry in a stand-in shader under judgment-free lighting, decorated by a thresholding script. The museum bar is a beauty bar. The library needs a finish layer with its own gates, and plates that let anyone see the difference.

## What is genuinely strong — keep all of it

- **One skeleton → finish-only tiers** as the rarity ladder, parameterized. Right architecture, historically defensible.
- **Provenance discipline**: accession-cited decoration choices, CC0-only sources, MANIFEST'd raw photos, STUDY_NOTES rules referenced by number. Better than most studios.
- **The eight field-note rules themselves** — they are correct and well-distilled; most findings below are cases where the renders violate the crew's own rules.
- **Engineering honesty**: quoted ngons, superseded builds documented, "still owed" lists, adaptation notes when a donor was rescaled.
- **The A/B method** (inlay color-only vs full stack) — correct experimental instinct; extend it, don't retire it.
- **Breadth**: six weapon families plus a donor system in days.

## Findings by asset set

### arming_sword_v1 / tiers (SEVERITY: high — this is the flagship)

1. **The stand-in shader invalidates every finish claim.** The proof page admits the dark blade is "a portable stand-in," yet the library judges tiers whose *entire axis is finish*: wear 0.38/0.30/0.22 produces **zero visible difference** between Soldier, Knight and Lord steel. The one thing the tier system varies is the one thing the plates cannot show. Either plate the shipped cel look, or a judgeable PBR pass — a stand-in can gate seams, never finish.
2. **All-black blades invert the crew's own ground logic.** Field note 1 says the ground is *prepared* for the decoration; rule 8 says restraint reads as wealth. Met 32.75.225 — the cited Soldier-mark precedent — is **bright steel**. Bluing was a cost. Making every tier's blade near-black erases tier legibility at silhouette distance (three black swords, colored grips) and spends the Lord's privilege on the Soldier. Correct ladder: Soldier bright munition steel + iron; Knight bright steel, brass, one modest blued panel if any; Lord full blued-and-gilt (the Cleveland 1916.1095 scheme). The blue-black ground should *be* the top of the ladder.
3. **Letterforms break the period read (Knight).** +INOMINE+ is set in a modern geometric monospace with even strokes and square terminals — it reads as CNC engraving or a UI label. The real +INNOMINEDOMINI+ group and the cited met_35367 use irregular hand-cut capitals: uneven strokes, wobbling baseline, wedge terminals. One authored period letterset (even 26 glyphs, deliberately imperfect) fixes every future inscription.
4. **Marks render proud; the evidence says flush.** The Soldier mark and Knight letters carry emboss-style highlights. The library's own flush-inlay evidence (Cleveland 1919.266: "dead-flush… pure material contrast + hairline seam") and latten practice say: flat fill, hairline seam shadow, contrast from material not relief. The seam-normal map exists in the four-map stack — it should read as a hairline, not a bevel.
5. **Soldier mark scale/placement.** It spans most of the blade width and straddles the fuller boundary. Real smith's marks are small (~⅓ blade width), sit *in* the fuller or clearly on one flat, near the forte. Ties to feature-line IDs: decoration fields must reference F-lines, and the fuller boundary is one.
6. **The fuller doesn't read as a groove.** In every full view it appears as a painted tan stripe (lighter than the flats — on a blued blade the fuller would be blued too). No concave shading, no edge shadows. Under the flat studio light the blade's *entire cross-section* fails to read — no grind planes, no ridge. This is precisely what the raking-light plate exists to catch.
7. **Hilt finish is below the blade's engineering.** At hero distance: guard is a flat slab with visible polygon kinks on the arm curves and écusson; a white sliver of background shows at the guard/grip seat (light leak — the numeric joint gate passes while the *picture* shows daylight; the visual seam plate must gate this); grip is texture-free vinyl with color bands (the proof claims waist, wrap band, riser swells — at plate resolution none of it reads; there is no wrap helix at all); pommel renders as a small flat-faceted octagon — 112 quads is a fine *donor* but it is being plated unsubdivided as a hero close-up. The Met reference's wheel pommel is massive, inscribed, and reads as the sword's counterweight; ours reads as a nut on a bolt.

### inlay_spike_v1 (SEVERITY: medium — right pipeline, wrong craft)

The four-map stack (mask/seam/rough/cavity), the strip UV, and the A/B are the right machinery. The INOMINE execution fails on letterforms (above), proud read (above), and the **context plate**: a giant featureless white cylinder looms beside the sword — a bounce card or prop left in frame. It reads as a broken render and destroys the plate's authority. Context plates need real context (mannequin hand, scabbard, table — or nothing).

### inlay_suleyman_v1 (SEVERITY: high — the extraction method is the finding)

Set the render beside Met 1993.14 and the method's ceiling is obvious. The reference's gold is *drawn*: individually modeled scroll strokes and creatures, constant line weight (field note 5), even interstitial ground (field note 4), a scalloped field whose ogee terminal flows onto clean steel (field note 3), placed at the **forte** of the blade. The extraction is *thresholded*: luminance islands fused into Rorschach clumps, stair-stepped mask aliasing at render scale, line weight nowhere constant, density gradient inherited from the photo's lighting rather than designed, in a rigid CAD hexagon with lollipop terminals, placed **mid-blade** on a straight European sword. Field notes 3, 4 and 5 are all violated by the crew's own showcase. Hue/luminance extraction can only ever harvest *where* gold was; it cannot recover *strokes*. The pipeline needs a vectorization/stroke-reconstruction stage (centerline trace → rebuild as constant-width strokes → re-rasterize at target texel density), or motifs authored from the reference rather than lifted. The logged sukashi segmentation failure is the same lesson from the other direction.

### falchion_v1 (SEVERITY: medium)

Bright steel here — which indicts the arming sword's black, since the two share an armory. Guard is a uniform-thickness flat slab with a paper-grain noise texture that reads as watercolor paper, not forged iron; arms end in stamped-looking curl discs. The brass collar is a flat saturated-yellow wedge that reads pasted on, with a light-leak sliver at its seat. Grip cloth texture exists but is print-flat. Chop-forward CoG (136 mm) is good and *should be shown* (balance-point marker plate).

### katana_v1 (SEVERITY: medium-high for family coherence)

A third material language: white-silver blade, cartoon-saturated gold habaki/kashira, washer-flat tsuba with no modeled features, flat blue tsuka with no tsukamaki diamonds (at any distance the wrap IS the katana's identity), sori barely perceptible. The hamon band exists but floats on an otherwise responseless blade. This set is furthest from its references and arguably shouldn't share a library page with the cited-and-measured arming sword until it has its own reference pack and study rules (tsukamaki is a weave — separate machinery, as already noted in the WPN-001 handoff).

### longsword_XVa_v1 (SEVERITY: low-medium)

The best silhouette in the library — the XVa taper and acute point are plausible and the guard proportions are better than the arming sword's. But a XVa is *defined* by its stiff mid-rib section, and the flat lighting erases it completely. This asset more than any other needs the raking plate to exist at all.

### sabre_v1 (SEVERITY: medium)

The knuckle bow — the set's announced feature — renders as a bent flat ribbon with a visible kink at every one of its 8 sweep rings. 86 quads is a runtime budget being exhibited as a hero plate. The cruciform barrel grip is the wrong family for a stirrup-hilt sabre (sabre grips: one-piece, backstrap, often wire-bound); the gold collar wedges repeat the falchion's pasted-on read. Yelman and clipped point are present in silhouette — good bones.

### donor_009_scentstopper (SEVERITY: low — but fix the exhibition)

The donor discipline is real (0 poles, deterministic, zoned rim, emission-through-bore proof of the tang tunnel — clever). Two exhibition problems: the 16-gon rim silhouette is visible in every shaded view (donors are all-quad precisely so subdivision works — plate them subdivided+creased), and the library page shows raw wireframes/aperture cards beside beauty claims without labeling which plates are instrument shots vs. appearance shots. Separate the two registers explicitly.

### Cross-cutting: numbers that disagree

The full-profile caption quotes 1.386 kg / CoG 87 then "re-solved 1.249 kg / CoG 87"; the instrument tape says 1.238 kg / CoG +98; the library caption says "1.39x kg, CoG 87". Three values for the flagship's mass on one page. Quote one current solve everywhere, tag superseded numbers as superseded — provenance discipline should extend to the factory's own measurements.

## Root causes (five, not fifty)

1. **Stand-in shader shown where finish is the claim.** No plate can currently judge steel, wear, bluing, or gold as materials.
2. **Runtime meshes exhibited as hero plates.** Donor budgets (86–134 quads) rendered close-up unsubdivided put polygon kinks in every silhouette.
3. **Decoration by threshold, not by stroke.** Extraction harvests placement, destroys draughtsmanship; plus modern letterforms and proud rendering against flush evidence.
4. **Ground strategy inverted.** Blued-black spent everywhere, erasing the tier ladder and contradicting the bright-steel precedents the tiers themselves cite.
5. **Plate hygiene has no standard.** Flat single light, grey void, stray prop geometry, no raking view, no clay view, no scale reference, instrument and beauty shots interleaved without labels.

## Gap-to-goal program (ranked by leverage)

1. **Judgeable material pass** — one steel family (bright / blued variants: micro-anisotropic grind response, patina/wear as *edge-and-touch-biased* masks so the wear parameter finally reads), one gold (flush fill + hairline seam + value range), brass ≠ saturated yellow, iron for guards. The WPN-001 `SINC_SH_ForgedSteel` spec already describes most of this. Every tier claim becomes visible the day this lands.
2. **Plate standard v2** (extends the brief already delivered): fixed camera set = full, forte crop, hilt, **raking-light**, **clay**, edge-on; dark backdrop for bright blades, light for dark; scale bar or mannequin hand in exactly one plate; balance-point marker on the profile plate; label every plate INSTRUMENT or APPEARANCE; no stray scene objects; subdivide+crease donors for appearance plates (runtime cage stays for instrument plates).
3. **Stroke-true decoration** — vectorize extractions to centerline strokes and rebuild at constant width; author one period letterset; flush preset as default (seam map = hairline, cavity = darkened recess only); decoration fields addressed to named feature lines (forte field, fuller band) per the feature-ID rule; kill mask aliasing by rasterizing at plate resolution.
4. **Per-tier ground ladder** — bright soldier steel → brass-touched knight → blued-and-gilt lord. Makes the rarity ladder legible from across a room, which is its game job.
5. **Hilt finish catch-up** — grip wrap geometry (the WPN-001 `GripWrap` helix group answers this directly), guard bars with real sections (lenticular/diamond, tapered, with knop volumes — the PAS donor's "forged upset" deserves to be visible), pommel exhibited at counterweight scale.
6. **Family coherence pass** — one armory, one light, one steel language across arming/falchion/longsword/sabre; katana quarantined until it has its own reference pack.

Items 1–3 move every current and future asset; 4–6 are per-family passes. Nothing above discards existing machinery — it adds the finish layer the instrument layer has earned.

## The one-line summary

The factory measures like a museum conservator and paints like a UI mockup; close that gap and the library will stand next to its own reference wall.
