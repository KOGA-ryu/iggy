# Forged-Iron Material Workstream

## Goal

- Material ID: `forged_iron_v1`
- Capability: intact hot-worked architectural iron for straps, plates,
  knuckles, pintles, nails, and rivets at ordinary and giant-house fabrication
  scales
- Workflow tier: `reusable-family`
- Actual acceptance geometry:
  `output/openwork_strap_hinge_geometry_v1.blend`
- Material target: the approved 3.112 m openwork moving leaf, 0.852 m fixed
  leaf, 0.41 m pintle/knuckle run, and their separate functional pieces
- Human-scale measurement fixture: Met object 55.61.58 catalogue envelope,
  0.406 by 0.041 m; its unreported thickness remains proof-only
- State target: production candidate. User review is still the acceptance
  authority.
- Unreal parity: false until the same coordinate reconstruction, maps, layer
  responses, tangent direction, distance behavior, and actual hinge are
  compared in Unreal.

## Capability boundary

This slice is clean intact forged iron. It includes:

- a continuous compact blue-black to warm-black dielectric forge skin;
- broad heat/scale value fields with at least sixteen related shades inside
  one component;
- finite planishing and cross-peen plane changes;
- restrained component-local worked-face brushing;
- a modern measured micro-roughness proxy used only as a bounded close-detail
  calibration;
- a separately addressable exposed-iron conductor response reserved for a
  later contact-polish mask;
- explicit one-times and ten-times fabrication scale;
- close, gameplay, grazing, repetition, and actual-hinge proof.

It excludes:

- active orange corrosion;
- uniform rust;
- pits, scratches, chips, cracks, dents, edge loss, soot, dirt, blood, and
  generalized grime;
- contact polish in the default candidate;
- wax, oil, paint, or another coating;
- silhouette displacement;
- inferred medieval brush-wire dimensions;
- AI-generated reference or runtime imagery.

The material must not disguise weak geometry. Openwork silhouette, knuckle
clearance, pintle, leaf thickness, arrises, and apertures remain geometry.

## Existing-state audit

The previous package is structurally ambitious but visibly fails.

1. `forged_iron_v1_base_color.png` contains bright rounded rectangular marks.
   The generator creates them by converting the strongest hammer activity
   directly into exposed conductive iron.
2. Hammering does not by itself prove that intact scale was removed down to
   bright metal. The rule creates a material-category change from a tool mask.
3. The saved specimen reads as smooth neutral gray plastic under the current
   proof light. Broad dark scale value, finite plane hierarchy, and a local
   worked direction do not survive as a coherent surface.
4. The current “door” proof mainly shows tiny fastener heads. It does not test
   the approved openwork hinge.
5. A 41.275 by 31.75 mm hammer envelope is sampled unchanged on a component
   authored at ten-times giant-house scale. This makes the macro fabrication
   vocabulary physically inconsistent with the target.
6. The present single atlas coordinate cannot keep giant fabrication marks
   scaled while retaining micro-topography and brush response in world metres.

## Live route

`profile -> finite pattern -> deterministic generator -> physical maps ->
versioned Blender node groups -> assigned openwork hinge material -> packed
saved blend -> reopened-asset test -> adversarial proofs`

The canonical owner remains this package. No parallel forged-iron material is
created.

## Stage ledger

| Stage | Status | Evidence |
| --- | --- | --- |
| 00 scope | complete | capability and exclusions above |
| 01 audit | complete | source, maps, node graph, saved blend, and current proofs inspected |
| 02 research | complete | `RESEARCH_INTENT.md` source and transfer ledger |
| 03 intent | complete | physical anatomy, frequency ladder, colour order, response causality, scale contract, and rejection rules frozen |
| 04 coded implementation blueprint | complete | `CODED_DEMANDS.md` specifies exact tests, generator changes, Blender nodes, geometry attributes, proofs, and gates |
| 05 pattern and map authoring | complete | twelve live physical lanes, compatibility maps, causal proof, hashes, and deterministic statistics generated |
| 06 shader integration | complete | nine versioned groups, twelve image nodes, three coordinate fields, four normal bands, named tangent, two physical responses, and metre displacement live |
| 07 damage | skipped-out-of-scope | all damage and narrative overlays are forbidden in this slice |
| 08 proof | repair requested | the new twelve-panel actual-hinge board exposes root-material defects that the older five proofs did not show |
| 09 repair | bounded next pass | conductor error and target framing are repaired; cloud colour, stamped worked response, flat roughness, and weak pivot response remain |
| 10 candidate audit | user not accepted | automated root-build gates are green, but visual acceptance is explicitly false and Unreal parity remains false |

## First failing observables

- The default metalness map contains non-zero conductive islands derived from
  hammer activity.
- The shader has no fabrication-scale attribute.
- Macro and micro maps share one coordinate.
- No worked-face direction map, anisotropic-rotation input, or worked normal
  exists.
- The saved material is not assigned to the actual openwork hinge.
- No actual-hinge render exists.

## Proving gate

The candidate is rejected unless all of the following are true:

- default metalness is exactly zero everywhere;
- planishing and cross-peen remain visible through normal/height/roughness
  without producing bright silver squares;
- broad base colour contains at least sixteen related shades and keeps quiet
  regions;
- `sinc_iron_fabrication_scale` is present on every proof and target mesh;
- the human strap uses scale 1 and the giant hinge uses scale 10;
- macro coordinates divide metre position by fabrication scale while
  micro/worked coordinates remain world-scale;
- worked response uses its own 2.436 by 0.492 m authored field rather than
  repeating in 0.164 m cross-stock rows;
- the worked normal and response maps are independent, packed, and live;
- tangent direction comes from `IGGY_IronUV` and worked response controls both
  anisotropy amount and rotation;
- the actual target objects all use the canonical material;
- the actual hinge has neutral front, material front, grazing, base-colour,
  and worked-response proofs;
- no Noise Texture, Voronoi, damage, rust, or default contact-polish path
  enters the public group;
- the saved file reopens in a separate Blender process and all focused tests
  pass.

## Candidate comparison bar

The visual hierarchy is judged against:

- BG3 modular iron: construction and broad plate values read before mottling;
- the user-supplied Adrien Roose metal method: roughness/gloss breakup,
  direction, masks, and manual selectivity are distinct layers;
- professional hot-worked iron examples: subtle plane variation with quiet
  areas, not procedural detail everywhere;
- the Canadian Conservation Institute stable-iron description: compact
  adherent silver-gray through blue-black/red-brown states, with active orange
  corrosion excluded.

The target is not photographic rust. It is a graphic, readable intact surface
whose broad dark values support the openwork geometry and whose close response
reveals believable making.

## Remaining risk

- The hammer dimensions are a later-period measured proxy, not medieval tool
  metrology.
- The 0.30-0.35 mm wire diameter evidence bounds a modern comparative finish,
  not the exact finish of the museum hinge.
- Visible 2.4-4.8 mm brush-bundle rhythm is an authored raster-resolvable
  translation, not bristle spacing.
- Ten-times fabrication scale is the approved giant-house adaptation, not a
  historical reconstruction.
- Unreal parity remains unverified.

## Repair record

1. The original hammer mask produced bright conductive rectangles. Conductive
   exposure is now zero; hammering survives only as plane, height, normal, and
   roughness response.
2. The first giant proof multiplied oxide, colour, micro, and worked scale
   together with tool scale and was over-lit. Macro fabrication and raw
   world-metre surface coordinates are now separate.
3. The first target camera framed only the moving leaf and concealed the
   pointed end and fixed leaf. The renderer now takes world-space bounds from
   all eight target objects and reserves a 15 percent complete-span margin.
4. The high-resolution worked-response proof exposed tidy repeated rows. The
   finish now owns a third coordinate and a twelve-pass field twice the base
   length and three times its width, while bundle spacing remains in raw
   metres.

The source-by-source comparison and unresolved delta are recorded in
`REFERENCE_DELTA.md`.

## Intact oxide layer calibration — 2026-07-31

The reduced-resolution actual-hinge board is
`output/intact_oxide_layer_sweep_v1/comparison_board.png`. Rows are A smooth
control, B quiet compact scale, C heavy forged scale, and D compressed
directional scale. Columns are front, grazing, gameplay, and the causal field
diagnostic where red is thermal macro, green is compression flow, and blue is
detail-priority-modulated grain.

Decision: **B quiet compact scale is the ranked repair basis, not an accepted
champion and not authorized for integration.** The current cumulative material
and canonical blend remain unchanged.

### Complete visual critique

| Rank | Candidate | Surviving strength | Rejection or repair reason |
| --- | --- | --- | --- |
| 1 | B quiet compact scale | Best bounded physical premise; blue-black value remains restrained; flat leaf and openwork silhouette survive; 67.7 percent of evaluated texels remain below the active-detail threshold | Final front, grazing, and gameplay views still sit too close to the smooth control; medium oxide identity is not legible enough; the diagnostic reveals diagonal harmonic interference instead of irregular compact scale |
| 2 | C heavy forged scale | Strongest roughness and morphology range without exposing conductor or breaking the flat plate read | The 20--90 micrometre envelope is only a giant-forge translation; extra range does not buy a commensurate visible improvement; the same harmonic signature remains and heavier colour variation is not yet justified by the consumer reference |
| 3 | A smooth control | Cleanest broad value grouping and no symbol, tile, or stripe signature | It is the known failure: one smooth coated-looking response with no medium intact-oxide structure and no meaningful distinction between active and quiet regions |
| 4 | D compressed directional scale | Directional response is correctly owned by tangent and roughness rather than painted scratches | Long coherent bands dominate the diagnostic and approach a candy-cane/brushed finish; it implies machine finishing more than compressed forge scale while still changing the final grazing view too little |

Strongest achievement: all rows preserve the actual hinge's flat planes,
functional apertures, component boundaries, complete dielectric oxide coverage,
and zero exposed conductor. The former cat-face/diamond pareidolia is absent.

Weakest area: the research-derived layers are not yet visually carrying their
weight. If the decisive structure is obvious only in the isolated diagnostic
and nearly absent from the final grazing response, the material has not gained
the intended intact-oxide identity.

Procedural signature: B, C, and especially D expose regular multi-sine
interference. It is not a closed symbolic motif, but it is still authored
mathematics reading before material evidence. Increasing its strength would
turn it into visible stripes; leaving it at the current strength leaves the
surface too smooth.

Distance result: every candidate preserves silhouette and broad value at
gameplay distance, but the new medium band collapses. Micro response is
properly subordinate, yet the missing irregular medium morphology prevents a
convincing close-to-gameplay handoff.

### Batched next repair

Repair B once, without another four-row sweep:

1. Replace the harmonic morphology bank with a nonperiodic, reflect-padded,
   band-limited material field at three separated metre scales.
2. Replace continuous compression sinusoids with two or three finite open
   worked-flow rails whose width, pressure, and fade change along their length.
3. Keep the thermal rail field and measured-like 8--35 micrometre envelope.
4. Keep grain out of colour and height; use it only as a bounded aggregate
   roughness perturbation below the medium band.
5. Preserve the existing detail-priority mask, but concentrate the readable
   medium response inside its active minority instead of globally increasing
   contrast.
6. Prove the repair with one B-only front, grazing, gameplay, independent
   roughness, independent medium morphology, and causal cumulative response.

The exact next gate is visual: the repaired B grazing view must show irregular
compact scale and compressed flow without a repeated wave, symbol, stain,
scratch, or loss of plate flatness. Only then may it replace the cumulative
oxide fields.

## Canonical-root actual-hinge acceptance board — 2026-08-02

The current proof is
`output/actual_hinge_acceptance_board_v1/forged_iron_v1_actual_hinge_acceptance_board.png`.
It reopens only `output/forged_iron_v1.blend`, uses the root
`IGGY_MAT_ReferenceForgedIron_v003`, renders twelve matched actual-consumer
panels, and verifies that the source `.blend` hash is unchanged. Archived
cumulative, oxide-sweep, quiet-repair, diamond, and cat-face outputs are not
inputs.

Decision: **repair requested**. This is not accepted and is not a production
candidate.

### Complete visual critique

Strongest surviving result: the full front, three-quarter, and gameplay panels
retain a quiet dark blue-grey family, several related shades inside one leaf,
complete oxide coverage, readable apertures, and no bright hammer-derived
conductive islands. The hinge remains legible before local texture, and the
material does not introduce rust or damage to create interest.

Weakest result: the material's medium manufacturing identity is not credible.
The isolated worked-response lane exposes large rounded rectangular passes with
regular internal stripes. They read as pasted bandages or decorative stamps,
not a restrained change of worked luster. Their low visibility in the combined
view is not a defense; an incorrect causal layer cannot be accepted because the
beauty render hides it.

The board records these defects:

1. **Broad colour morphology** — the unlit colour is a field of soft closed
   clouds with similar edge softness and scale. It reads as a generic mottling
   overlay rather than one connected compact oxide film organized by heat,
   compression, and stock direction.
2. **Worked response** — the tan/cyan diagnostic contains repeated oblong
   silhouettes, visible endpoints, and striped interiors. The finite-pass
   primitive remains capable of symbols and pareidolia even when its live
   amplitude is low.
3. **Roughness causality** — the full roughness lane is almost uniformly pale.
   Its surviving breakup is too close to the same broad cloudy organization,
   and the opposed close lights mainly reveal lamp falloff and aperture bevels,
   not a distinctly forged microfacet hierarchy.
4. **Normal hierarchy** — the combined normal is nearly flat at the actual
   consumer scale. The broad forging-plane and close micro bands do not form a
   readable close-to-gameplay handoff; only geometry-owned bevels and aperture
   lands respond decisively.
5. **Pivot response** — the pivot close-up reads as a smooth coated cylinder.
   The component tangent exists technically, but its manufacturing direction
   is not visually legible and does not distinguish barrel stock from broad
   leaf stock.
6. **Grazing range** — the grazing and moving-strip panels blow across broad
   portions of the leaf as one smooth field. They do not show a convincing
   sequence of broad plane, continuous oxide, localized worked luster, and
   subordinate microstructure.

Comparison boundary: the museum hinge photographs contain wood, shadow,
oxidation, damage, and historical condition that are excluded from this clean
core. Their usable evidence is still that the iron reads as a coherent dark
material with tight irregular surface response and construction-led
highlights. The current root preserves the dark group but replaces the tight
irregular response with soft clouds and closed worked stamps. Professional
game-material references similarly preserve broad plate hierarchy first, then
subordinate non-symbolic surface response; the root passes the first half and
fails the second.

Distance verdict:

- close: fails; the cloud and worked-pass primitives become identifiable;
- gameplay: macro value and construction survive, but medium identity collapses;
- distant: silhouette and dark material grouping survive;
- actual consumer: correctly assigned and stable, but not visually accepted.

Procedural signatures: rounded worked bars, repeated internal finish stripes,
soft cloud islands, and a roughness field whose visible organization is too
similar to colour. No cat-face is dominant in the final beauty render, but the
primitive family remains symbol-capable and is rejected on that basis.

### Batched repair blueprint

Preserve without alteration:

- the actual eight-object hinge and its semantic geometry;
- metre coordinates, seed, fabrication scale, and component tangent ownership;
- the sixteen-shade cool/warm dark palette envelope;
- binary material identity and default-zero conductor exposure;
- the two-complete-BSDF mixer;
- separate colour, roughness, normal, height, response, and semantic lanes;
- all locked absences, including rust, contact polish, and damage.

Replace only two visible causal owners:

1. **Compact oxide organization**
   - Strategy A, preferred: continuous full-coverage oxide driven by three open
     thermal rails with irregular longitudinal knots, plus one reflect-padded,
     band-limited connected morphology field at separated metre scales.
   - Strategy B: a hand-authored finite film atlas whose regions overlap into
     one connected coverage field and whose boundaries cannot close into
     isolated spots.
   - Both routes keep oxide surface height at zero. Thickness changes colour
     and roughness; broad forging planes and measured-proxy microstructure
     remain the only normal owners.
2. **Worked luster**
   - Strategy A, preferred control: disable worked normal and oxide anisotropy;
     retain the tangent only on the hidden conductor branch. This tests whether
     intact oxide needs any visible directional finish at all.
   - Strategy B: one continuous component-aligned luster field with open,
     off-frame direction changes and no finite stamp silhouette.
   - Strategy C: two or three open spline rails whose width and pressure vary
     along their length, with endpoints outside the visible host and no height,
     colour, scratch, or conductor claim.

Roughness must then be recomposed independently: intact baseline first,
thermal thickness within a narrow amplitude, connected medium morphology at a
different amplitude, and subpixel grain only as aggregate reflection. It may
reuse physical identities but may not copy colour or normal grayscale.

### Exact next decision gate

Render one reduced-resolution board on the same hinge and fixed lights:

- A — frozen root failure control;
- B — connected oxide plus no worked-luster contribution;
- C — B plus continuous component-aligned luster;
- D — B plus sparse open-rail luster.

Columns are full front, identical close neutral, identical opposed-light
difference, gameplay distance, connected-oxide diagnostic, independent
roughness, and luster diagnostic. Reject any route with a closed silhouette,
visible endpoint, repeated row, cloud island, copied lane, or detail that reads
before the broad dark metal group. Only one selected route may advance to a
new canonical material revision.

## Connected-oxide and luster repair selection — 2026-08-02

The reduced A--D matrix is
`output/connected_oxide_luster_sweep_v1/connected_oxide_luster_comparison_board.png`.
It uses the same actual hinge, cameras, light rigs, exposure, and source blend
for every row. The root and geometry SHA-256 values remain unchanged; no
candidate `.blend` was saved.

Decision: **advance B's response strategy only**. This does not accept a
material and does not replace `IGGY_MAT_ReferenceForgedIron_v003`.

### What the board proved

- A preserves the root's useful dark macro grouping but still exposes the
  rejected soft colour clouds and finite striped worked-response stamps.
- B keeps oxide coverage continuous, keeps oxide height and conductor exposure
  exactly zero, removes the worked-luster lane, and limits independent 8--40 mm
  organization to a narrow roughness perturbation. The plate becomes quiet and
  no decorative symbol reads in the physical panels.
- C and D are pixel-identical to B in all four physical columns: full front,
  close neutral, opposed-light difference, and gameplay. Under this reduced
  Eevee gate their added anisotropy produces no demonstrable surface response.
- D additionally fails its isolated lane: the uncapped curves remain three
  conspicuous horizontal rails. Open endpoints solved the cap problem but did
  not solve the decorative-stripe problem.

### Complete defect ledger

1. B is a structurally correct repair baseline, not finished forged iron. Its
   macro response is quieter than the root, but it still reads too evenly gray
   and the pivot remains smooth and coated-looking.
2. The opposed-light difference is dominated by aperture bevels, fastener
   edges, barrel segmentation, and lamp travel. The material contributes too
   little medium manufacturing identity to claim completion.
3. B's roughness is independent from colour and normal, but the amplitude is
   intentionally close to the visibility floor. Integration must verify it in
   Cycles before changing the amplitude.
4. C is rejected for this repair because it adds graph complexity without one
   changed proof pixel. That does not establish that dielectric anisotropy is
   universally useless; it establishes only that this Eevee consumer proof did
   not show it.
5. D is rejected because its response diagnostic is a repeated rail language
   and because the physical board supplies no countervailing benefit.
6. The root A remains frozen as the current user-not-accepted material until a
   B-derived integrated candidate survives an actual-asset board.

### Exact next gate

Build one noncanonical B-derived material revision. Reuse the frozen geometry,
component frames, two-complete-BSDF mixer, broad forging normal, and dark
palette envelope; replace the root cloud/stamp owners with B's full-coverage
oxide colour, independent roughness, and zero visible luster. Render the new
candidate beside frozen v003 in Cycles on the actual hinge under neutral,
opposed close, grazing, pivot, gameplay, and isolated-lane proofs. Preserve v003
and do not register a donor or claim user acceptance.

## B-derived v004 Cycles actual-hinge proof — 2026-08-02

The selected B response is now packaged separately as
`output/forged_iron_connected_oxide_candidate_v004/forged_iron_connected_oxide_candidate_v004.blend`.
It reopens with eight object-specific materials sharing
`IGGY_SH_ConnectedOxideForgedIron_v004`. Each material has exactly four live
packed fields; the shared group owns the two complete physical lobes, normal
decoder, and explicit zero-exposure mixer. The frozen v003 source hash did not
change.

The paired Cycles board is
`output/forged_iron_connected_oxide_candidate_v004/cycles_actual_hinge_proof_v1/forged_iron_v003_v004_cycles_comparison.png`.
Every proof pair uses the same geometry, camera, lights, AgX transform, and 32
Cycles samples.

Decision: **v004 is the preferred repair base, but repair is still requested**.
It is not accepted, current, or donor-ready.

### What improved

1. The soft closed v003 colour clouds are gone from the close, pivot, and full
   views.
2. The striped finite worked-response owner is absent rather than hidden.
3. Quiet plate regions survive from close through gameplay distance.
4. Oxide remains continuous dielectric material; exposure, oxide metalness,
   oxide height, and worked-luster amount remain exactly zero.
5. The candidate is structurally cleaner: one shared response group now owns
   physical mixing while object materials own only component fields and
   coordinates.

### Remaining defects

1. The isolated v004 base colour is nearly black. The lit result therefore
   depends too heavily on the lamps and drifts toward neutral warm gray instead
   of retaining the root's readable cool/warm dark-oxide hierarchy.
2. The independent roughness proof is almost uniform. Its 8--40 mm response is
   physically separated in code but does not form a useful visual handoff.
3. Both normal proofs remain nearly flat. The broad forging-plane amplitude is
   below the useful actual-consumer threshold.
4. The pivot is cleaner but remains a smooth coated cylinder. Removing the
   wrong pattern did not supply a correct component-scale forged response.
5. The opposed-light proof is dominated by apertures, bevel lands, fastener
   holes, and knuckle segmentation. Surface manufacturing identity remains
   subordinate to geometry alone.

### Exact next gate

Keep v004's connected-oxide/no-luster architecture frozen. Run one cheap
Cycles calibration board for three bounded optical/response variants plus the
current v004 control. Change only the connected oxide palette envelope, broad
forging-normal amplitude, and independent roughness amplitude. The variants
must retain full oxide coverage, zero oxide height, zero exposure, zero worked
luster, and identical aperiodic field coordinates. Required proofs are close
neutral, grazing, pivot, gameplay, base colour, roughness, and normal. Reject
any repair that restores clouds, spots, stripes, or a response that reads
before the dark forged-metal group.

## v004.1 audited optical-response calibration — 2026-08-02

The calibration board is
`output/forged_iron_v004_optical_calibration_v1/forged_iron_v004_optical_calibration_board.png`.
It compares the exact saved v004 control against three responses whose palette
endpoints are literal anchors from the audited sixteen-colour forge-skin set.
Only palette endpoints, independent roughness amplitudes, and a scalar on the
existing broad forging height were changed. No new pattern owner was created
and the saved v004 hash remained unchanged.

Decision: **select C's calibration recipe**. This is not a saved material,
acceptance, or donor promotion.

### Selection reasons

1. A is rejected because the isolated colour remains nearly black and leaves
   the lit result overly dependent on the lamps.
2. B correctly restores the audited cool dark-half palette and is the second
   strongest row. It remains quiet but supplies a weaker roughness/normal
   handoff than C.
3. C uses audited `#252a31` to `#353b46`, roughness baseline 0.680 with bounded
   connected-medium amplitude 0.055, and broad-normal multiplier 1.75. It
   retains a compressed cool forge-skin group while producing the strongest
   non-symbolic surface response of the valid rows.
4. D uses valid audited anchors, but its cool-to-warm range forms a localized
   warm passage around the hinge and moving leaf. At this clean-core stage it
   reads too much like an oxidation hotspot or the return of a broad cloud.
5. None of B--D creates spots, stamps, stripes, cat faces, diamonds, or visible
   luster rails. Physical differences remain deliberately subordinate at
   gameplay distance.
6. C still does not finish the pivot. Its selection calibrates intact-body
   response; component-scale barrel forging remains a later bounded owner.

### Harness correction

The first C row was rejected as invalid evidence before art review because its
tail and knuckles displayed mismatched proof lanes. The cause was Cycles
persistent render data retaining deleted temporary material/image bindings
between candidate rows. The renderer now disables persistent data and assigns
unique material names per row. The corrected board was rerendered from all
four rows, and no stale block remains. This is now a prohibited proof-harness
failure mode for disposable material sweeps.

### Exact next gate

Build one separate noncanonical v004.1 candidate using C's exact palette,
roughness, and broad-normal values. Preserve the shared v004 response group,
component fields, full oxide coverage, zero oxide height, zero conductor
exposure, and zero worked luster. Reopen it and render a paired Cycles board
against saved v004 under close neutral, grazing, pivot, gameplay, base colour,
roughness, and normal. Keep the pivot shortcoming explicit; do not add a new
barrel pattern during integration and do not promote a donor.

## v004.1 selected-C actual-asset integration — 2026-08-02

The exact selected-C recipe is now integrated into the separate reopenable
candidate
`output/forged_iron_connected_oxide_candidate_v004_1/forged_iron_connected_oxide_candidate_v004_1.blend`.
It uses literal audited `#252a31` and `#353b46` anchors, roughness baseline
0.680, thermal/medium/grain amplitudes 0.035/0.055/0.012, and the frozen 1.75
broad-normal multiplier. No new surface owner was added.

The candidate reopens with eight unique component materials and thirty-two
packed fields. Every material instantiates the unchanged
`IGGY_SH_ConnectedOxideForgedIron_v004` response group; its saved topology
signature is hash-locked at nine nodes, twenty links, and eleven interface
sockets. Frozen v003 and saved v004 remain byte-identical.

The paired board is
`output/forged_iron_connected_oxide_candidate_v004_1/cycles_v004_v004_1_proof_v1/forged_iron_v004_v004_1_cycles_comparison.png`.
It compares saved v004 and v004.1 with identical 720 by 240 Cycles tiles for
close neutral, grazing, pivot, gameplay, base colour, independent roughness,
and broad normal.

Decision: **v004.1 is the preferred repair base, but repair is still
requested.** It is not current, accepted, donor-ready, or Unreal-verified.

### What the integration proved

1. The isolated base-colour failure is repaired. v004.1 carries a visible
   compressed cool forge-skin field instead of v004's nearly black output.
2. The lit close, grazing, and gameplay views remain quiet. No worked stamp,
   rail, diamond, cat face, exposed-conductor island, rust, or damage returns.
3. The low-frequency optical modulation remains connected and subordinate; it
   does not close into the rejected v003 cloud islands.
4. Full oxide coverage, zero oxide metalness, zero oxide height, zero conductor
   exposure, and zero worked-luster amount survive every component field.
5. The saved-file and proof harnesses preserve both source hashes and disable
   disposable-render persistent data, preventing the previously observed
   stale field binding.

### Remaining actual-asset defects

1. The wider independent roughness field is still almost uniform in the actual
   hinge proof. It is physically separate but does not yet form a readable
   broad/medium/micro reflection hierarchy.
2. Multiplying the existing broad forging height by 1.75 does not materially
   change the real-asset normal proof. This proves the problem is not solved by
   another global strength increase.
3. The pintle and knuckles remain smooth coated cylinders. Their stock shape,
   manufacture direction, and useful feature sizes differ from the leaf stock,
   but the current generator gives them the same broad response recipe.
4. At gameplay distance the material preserves silhouette and value grouping,
   while manufacturing identity still comes mainly from geometry and lamps.

### Exact next gate

Freeze v004.1's accepted-for-repair palette and full-coverage oxide response.
Before authoring another visible field, audit the current broad-normal owner by
component: record height range in metres, slope/normal-angle distribution,
feature-size spectrum, and tangent/frame orientation separately for leaf,
pintle, and knuckle stock. Compare those numbers to written or image-translated
evidence for forged flat bar versus forged/drawn barrel stock.

Only after that audit, run one reduced normal-response decision board: v004.1
control, one leaf-stock construction response, and one barrel-stock
construction response, each shown isolated and cumulatively under identical
neutral and grazing light. A candidate must change the correct host class
without producing rings, stripes, stamps, closed motifs, or condition. Do not
alter colour, roughness, oxide coverage, shader topology, geometry, or damage
during that gate.

## v004.1 component-response ownership audit — 2026-08-02

The written and measured audit is `COMPONENT_RESPONSE_AUDIT.md`. Its generated
record is
`output/forged_iron_v004_1_component_response_audit_v1/manifest.json`, and the
four-panel actual-hinge board is
`output/forged_iron_v004_1_component_response_audit_v1/forged_iron_v004_1_component_response_audit_board.png`.
The source v004.1 blend remained byte-identical and no material candidate was
created.

Decision: **reject the shape-only global normal owner.** The prior exact-next
gate's phrase “barrel-stock construction response” is corrected: the
alternating knuckles are rolled/split-and-rolled tabs of their parent straps,
not independent cylindrical stock. Only the pintle is separate round stock.

### Measured diagnosis

1. The standardized broad-height correlation between every pair of the eight
   components is at least 0.9999968. One normalized three-rail, sixteen-knot
   gesture is being resized and restarted everywhere.
2. The moving and fixed leaves carry about 9.4 mm peak-to-peak integrated
   height, but their dominant longitudinal wavelengths are 1.558 m and 0.428 m
   respectively—almost exactly half of each host length.
3. Knuckles carry the same gesture at 2.609 mm peak-to-peak and 0.126–0.139 m
   dominant circumferential wavelength. All moving knuckles are identical; all
   fixed knuckles are identical.
4. Median selected-C normal angles already reach 0.96–1.47 degrees and 95th
   percentiles reach 1.84–2.84 degrees. The missing read is therefore not an
   amplitude shortage. The energy is organized into one or two smooth
   whole-host lobes rather than manufacturing frequencies.
5. The U/V proof visibly restarts at every object. Component-name seeds then
   break optical phase between each leaf and the knuckles formed from it.

### Canonical ownership correction

- `moving_leaf_stock` owns the moving leaf and moving knuckles as one rest-stock
  lineage;
- `fixed_leaf_stock` owns the fixed leaf and fixed knuckles as a second
  rest-stock lineage;
- `pintle_stock` owns the separate pin;
- circumferential eye coordinates and axial pintle coordinates remain
  secondary local frames;
- drift/swage compression, optional rear weld seam, and bearing faces remain
  later semantic overlays rather than base patterns.

### Exact next gate

Run one reduced, disposable normal-response comparison without changing saved
v004.1:

- A — unchanged v004.1 control;
- B — two parent-rest-stock flat-strap fields, each continuous from its leaf
  through its rolled knuckles;
- C — one separately owned axial pintle response, using progressive
  square/octagonal/round and swage evidence without periodic rings;
- D — B and C together.

Use the already recorded 41.275 by 31.75 mm human hammer-envelope proxy only as
a bounded feature-size reference, transformed by the approved ten-times giant
fabrication scale. Do not infer a literal surviving print from that envelope.
Use overlapping, partially obliterated broad influence and protect smooth rest
regions. Required columns are isolated height, isolated normal angle, neutral
close, grazing, pivot close, and gameplay. Reject visible hammer stamps,
repeated lobes, circumferential rings, UV seams, phase resets, or a response
that changes the wrong stock lineage. No integration, colour/roughness change,
condition, damage, shader-topology change, or donor promotion is authorized.

## v004.1 construction-response sweep — 2026-08-02

The reduced A--D board is
`output/forged_iron_v004_1_construction_response_sweep_v1/forged_iron_v004_1_construction_response_board.png`.
Its generated record is
`output/forged_iron_v004_1_construction_response_sweep_v1/manifest.json`, and
the complete visual review is `CONSTRUCTION_RESPONSE_SWEEP_REVIEW.md`. The
source v004.1 blend remained byte-identical. No candidate blend or production
map was created.

Decision: **reject every visible height/normal field and retain only the
construction-coordinate ownership.** No response row is selected for
integration.

### What passed

1. Moving leaf plus moving knuckles now has one parent-rest-stock coordinate
   contract; fixed leaf plus fixed knuckles has a second.
2. Every leaf/eye root comparison closes at exactly zero height error. The
   separately owned pintle also closes its circumference at zero height error.
3. B changes only the two strap lineages, C changes only the pin, and D is the
   exact union. Colour, roughness, zero luster, oxide coverage, geometry,
   source images, and shared shader topology remain frozen.
4. The corrected pin ring diagnostic averages around the circumferential axis,
   not the axial axis. Its circumference-mean height changes by only 0.00233 mm
   across the pin, confirming that no axial rings were authored.

### What failed

1. B/D turns the closed support of the planishing proxy into repeated large
   amoeba-like normal lobes. A literal ellipse was not drawn, but the proxy
   footprint still survives visibly and therefore fails the same no-stamp
   rule.
2. B's response energy is not constructionally balanced across eyes:
   MovingKnuckle_05 has only 0.047 mm peak-to-peak height while
   FixedKnuckle_02 reaches 2.373 mm. Phase continuity did not establish a
   believable finish.
3. C/D's one-millimetre pin height produces 3.625-degree median,
   6.735-degree P95, and 7.523-degree maximum normal angle. The documented
   square-to-octagonal-to-round operation became too literal an eight-fold
   residual.
4. That aggressive pin response still barely affects the lit actual-asset
   views: against A, C measures 70.10 dB close, 80.43 dB grazing, 63.76 dB at
   the pivot, and 82.21 dB at gameplay distance. This is the wrong lane, not an
   amplitude shortage.
5. A remains rejected because its shape-normalized three-rail field restarts
   on every component.

### Exact next gate

Do not author another hammer or facet normal field. Preserve the corrected
parent-stock and separate-pintle coordinates, then run one cheap response-owner
comparison on unchanged v004.1:

- A — frozen control;
- B — quiet broad roughness organization only, sampled in construction-owned
  coordinates with no compact support silhouette and no height;
- C — direction-dependent reflection only, separately oriented for flat
  strap, rolled eye, and axial pin, with no brushing lines or grooves;
- D — exact B plus C.

Precede that board with a neutral-clay moving-strip check of leaf planes,
rolled-eye geometry, and pin roundness so the material cannot conceal a
geometry-owned highlight defect. Keep any anisotropy or topology experiment
disposable until the board selects it. Damage, rust, scratches, dents, contact
polish, exposed conductor, oxide height, and donor promotion remain excluded.

## v004.1 clean-metal reflection routing — 2026-08-02

The geometry-first board is
`output/forged_iron_v004_1_reflection_routing_sweep_v1/forged_iron_v004_1_geometry_moving_strip_board.png`.
The four-row material board is
`output/forged_iron_v004_1_reflection_routing_sweep_v1/forged_iron_v004_1_reflection_routing_board.png`.
The manifest and complete review are respectively
`output/forged_iron_v004_1_reflection_routing_sweep_v1/manifest.json` and
`REFLECTION_ROUTING_SWEEP_REVIEW.md`.

Decision: **select C's uniform direction-dependent oxide response as a
calibration recipe.** This is not an integrated material or donor promotion.

### Geometry boundary

1. The clay leaves are broad planar plates. Reversing the moving strip reveals
   no hidden manufactured plane hierarchy; material response may reshape the
   highlight but may not claim to invent geometric facets.
2. Pintle and rolled-eye highlights remain controlled by smooth cylindrical
   geometry and hard bearing gaps. The selected response does not repair or
   conceal that pivot construction.
3. The tangent diagnostic verifies the live ownership needed by C: leaf U is
   longitudinal, eye U is circumferential, and pin U is axial.

### Material selection

1. B's open Hermite roughness field preserves every source mean and creates no
   stamps, clouds, rings, height, or new normal. It is nevertheless too weak to
   justify a new owner: B remains 57.61--64.62 dB from A across the five
   physical views.
2. C enables the already-present oxide anisotropy route only inside a
   disposable group copy and supplies one uniform response amount of `0.28`
   with rotation `0.50`. It introduces no visible texture pattern.
3. C changes both opposed-strip views in a balanced way: 39.28 dB from A under
   the left strip and 39.47 dB under the right. It remains quiet at close,
   pivot, and gameplay range and does not become brushed aluminium.
4. D contributes no persuasive roughness improvement beyond C; it remains
   57.74--64.69 dB from C and is rejected as unnecessary complexity.
5. The saved source group stays at a zero anisotropy multiplier, its topology
   hash stays `ce47658c611f97e6b4032596b223a7016fb05bf1daf30646b89c72b04227523b`,
   and saved v004.1 remains byte-identical.

### Exact next gate

Integrate only C into a separate v004.2 noncanonical candidate. Copy the v004
group under a new v004.2 name, preserve its nine nodes, twenty links, and
eleven sockets, and change only the existing anisotropy multiplier from zero
to one. Replace the per-component zero-luster images with packed uniform
`(0.28, 0.50, 1.0)` controls. Preserve v004.1 colour, roughness, broad normal,
oxide coverage, geometry, exposure, and source hash.

Save and reopen v004.2, then render a paired board against saved v004.1 using
close neutral, both opposed strip directions, pivot close, gameplay,
direction-amount, and source-normal panels. Reject tangent seams,
brushed-metal lines, frozen-image drift, any claim to fix clay-owned pivot
geometry, or a source mutation. Damage, condition, roughness expansion, new
normal authoring, oxide height, exposed conductor, Unreal parity, and donor
promotion remain out of scope.

## v004.2 directional-response integration — 2026-08-02

The saved candidate is
`output/forged_iron_connected_oxide_candidate_v004_2/forged_iron_connected_oxide_candidate_v004_2.blend`.
Its build manifest and separate-process reopen record are
`output/forged_iron_connected_oxide_candidate_v004_2/manifest.json` and
`output/forged_iron_connected_oxide_candidate_v004_2/reopen_contract.json`.
The paired proof is
`output/forged_iron_connected_oxide_candidate_v004_2/cycles_v004_1_v004_2_proof_v1/forged_iron_v004_1_v004_2_cycles_comparison.png`,
with the full review in `V004_2_INTEGRATION_REVIEW.md`.

Decision: **v004.2 replaces v004.1 as the preferred forged-iron repair base.**
It remains noncanonical, user-not-accepted, not a donor, and not
Unreal-verified.

### Exact integrated delta

1. Saved v004.1 stayed byte-identical.
2. `IGGY_SH_ConnectedOxideForgedIron_v004_2` is a copy of the v004 group with
   the same nine nodes, twenty links, eleven sockets, and topology hash
   `ce47658c611f97e6b4032596b223a7016fb05bf1daf30646b89c72b04227523b`.
3. The only group-default change is
   `Worked_Luster_Disabled.inputs[1]: 0.0 -> 1.0`.
4. Every component keeps its exact v004.1 base-colour, roughness, and normal
   float-pixel hashes. Each old zero-luster field is replaced by one packed
   four-by-four uniform control with RGB `(0.28, 0.50, 1.0)`.
5. Eight unique v004.2 materials and eight live v004.2 group consumers survive
   a separate Blender startup. Leaf-longitudinal, eye-circumferential, and
   pin-axial construction tangents remain unchanged.

### Visual result

1. The response changes broad highlight travel under both strip directions
   without introducing brushing lines, grooves, repeated symbols, cat-face or
   diamond pareidolia, tangent bands, or baked lighting.
2. v004.2 against v004.1 measures 44.030040 dB close, 39.282581 dB strip-left,
   39.474193 dB strip-right, 44.397690 dB pivot, and 49.521032 dB gameplay.
   These reproduce the disposable-C calibration rather than a new guess.
3. The isolated direction amount changes from black/zero to uniform `0.28`.
   All source lane hashes match, and the decoded source-normal proof pixels
   are identical. Independent PNG container hashes are retained but are not
   mistaken for pixel drift.
4. No tangent seam or plastic-narrow highlight appears. At gameplay distance
   the improvement is deliberately subtle and does not create surface noise.

### Remaining defects and exact next gate

The inherited shape-normalized broad normal remains rejected. The smooth pin
and bearing-gap response also remains geometry-owned. v004.2 improves only the
clean intact-oxide reflection route; it does not solve those defects.

The next bounded material decision is therefore a removal control, not a new
pattern: compare frozen v004.2 against the same saved material with the
rejected broad-normal input disconnected so geometry normals own the intact
surface. Use close neutral, both opposed strips, pivot, gameplay, and isolated
normal panels. If geometry normals alone improve plane continuity without
making the surface dead, remove the rejected owner in a separate v004.3
candidate. Do not author a replacement normal, add damage/condition, or claim
the pivot geometry is fixed in that workstream.

### Workflow-state limitation

`WORKFLOW_STATE.json` was not advanced for this bounded variation. The shared
workflow freezer rejects the package because historical DEM-PRODUCTION
entries 005--013 predate its mandatory `Test code` heading contract. Rewriting
those already executed demands merely to satisfy the current parser is outside
this workstream and could launder old evidence. DEM-PRODUCTION-014, its live
files, saved candidate, separate-process reopen, proof manifest, and focused
tests remain directly linked and hash-checked despite that pre-existing state
file limitation.

## v004.2 broad-normal removal control — 2026-08-02

The disposable paired proof is
`output/forged_iron_v004_2_geometry_normal_control_v1/forged_iron_v004_2_geometry_normal_control_board.png`.
Its machine record is the adjacent `manifest.json`, and the complete visual
review is `V004_2_GEOMETRY_NORMAL_CONTROL_REVIEW.md`.

Decision: **select geometry normals only as the normal-ownership recipe.** The
saved v004.2 candidate remains byte-identical and is still the preferred saved
repair base until a separate v004.3 integration passes. This experiment does
not create a blend, accept a material, register a donor, or establish Unreal
parity.

### Exact disposable delta

1. Row A independently opens saved v004.2 and keeps
   `Broad_Forging_Normal_Decode` Strength at `1.0`.
2. Row B independently opens the same blend, copies the shared group only in
   memory, and sets exactly that Strength to `0.0`.
3. The copied group retains the source node, link, interface-socket, and
   topology signatures. Its complete default diff is one socket.
4. All eight base-colour, roughness, source-normal, and direction-control
   float hashes match. Mesh hashes and UV inventories match. The saved
   direction multiplier remains one and every packed amount remains `0.28`.
5. Saved v004.2 retains SHA-256
   `d568dad25ab7400a5e51a3ec81eaf7db01665dab4bc35dff299eff09056e7864`.
6. The earlier moving-strip clay board is hash-registered as the geometry
   baseline and is not regenerated or presented as new evidence.

### Visual result

1. Geometry normals remove faint cloudy and dent-like response from the broad
   leaf planes without changing their large intact-oxide value hierarchy.
2. Left and right moving-strip views preserve the same macro highlight travel,
   so the selected cleanup is not a single-light or baked-direction trick.
3. The pivot keeps a continuous cylindrical highlight while losing weak
   vertical mottling. Bearing gaps and smooth-cylinder construction remain
   geometry-owned and unchanged.
4. Apertures, holes, joins, bevel lands, and the moving/fixed division remain
   readable. The repeated openwork silhouette remains literal consumer
   geometry rather than a material motif.
5. B against A measures 44.862448 dB close, 50.282430 dB strip-left,
   48.363634 dB strip-right, 44.721258 dB pivot, and 51.861939 dB gameplay.
   The effect appropriately recedes at gameplay distance.

### Exact next gate

Freeze DEM-PRODUCTION-016 for a saved v004.3 integration derived from v004.2.
Copy the shared group under a new version and change only the same Normal Map
Strength from one to zero. Retain the packed source normal for provenance but
leave it inactive. Save and reopen v004.3, then render a paired v004.2/v004.3
actual-hinge proof with the same seven views. Do not author a replacement
normal, introduce damage or condition, update the material catalog as accepted,
or treat the repeated openwork geometry as a shader defect.
