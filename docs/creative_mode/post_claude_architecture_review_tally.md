# Post-Claude Creative Architecture Review Tally

This document is the normalized pressure-surface tally for the post-Claude
Creative editor review.

It is not a builder task list. It names the surfaces, ranks the architectural
pressure, and records the safest first cut for each surface. Builder cards
should come only after this tally is reviewed and a slice is chosen.

## Evidence Snapshot

- Repo scan roots: `src`, `apps`, `tests`, `docs/creative_mode`.
- Files scanned: 881.
- Nonblank LOC: 206,750.
- Branch hits: 12,930.
- Symbols: 8,791.
- Test targets: 262.
- Explicit `else if` count from scout: 329.
- Top non-parser branch pressure:
  - `apps/iggy3d_creative/main.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/view/OpeningMenuView.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/creative/mutation/Mutation.cpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- Top mirror/state pressure:
  - `ProductAppWindowState`: 652 LOC struct body, 28 prod files with
    `window.*` member hits, 132 Creative mirror assignments.
  - `ReceiptBuilder.cpp`: 2,670 LOC in the current dirty tree and still a
    broad receipt mirror.
- Top test pressure:
  - `tests/unit/product_creative_world_launch_tests.cpp`: 4,077 LOC, 35
    manually called tests, many scenario corridors over 100 LOC.
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`: 227 expected
    field tuples and 236 quoted `creative_*` receipt keys.

## Ranking Rules

- Rank is based on feature-add tax, ownership ambiguity, branch/receipt sprawl,
  and risk of green tests masking bad behavior.
- "First safe slice" means the smallest behavior-preserving cut or read-only
  audit that reduces uncertainty. It is not an implementation order.
- Parser/config ladders are separated from live UI/runtime ladders. They are
  noisy, but generally lower priority unless schema churn increases.

## State Ownership

### Rank 1

- Surface: `ProductAppWindowState` mirror sprawl.
- Pressure type: multi-writer state and mirror duplication.
- Current symptom: 652 LOC state struct, 28 prod writer/read surfaces, 132
  Creative mirror assignments, broad tests seeding flat window fields.
- Current owner: `ProductAppWindowState` as a window-state bucket.
- Correct owner: typed domain snapshots projected into window and receipt views.
- Feature-add touch count: 5-9 files for stateful Creative/product features.
- Likely reduction: very high.
- Risk: high.
- First safe slice: audit each field into authoritative, derived, transient,
  receipt-only, or deprecated mirror.
- Do not touch yet: behavior, field layout, launch/open/save routing.
- Proof: writer matrix plus owner classification with no code movement.

### Rank 2

- Surface: active Creative identity.
- Pressure type: duplicated domain truth.
- Current symptom: `CreativeAppState::identity` and flat
  `window.activeCreative*` mirror fields both influence routing, save flow,
  receipts, and tests.
- Current owner: split between `CreativeAppState`, `Operations.cpp`,
  `ReceiptBuilder`, `FrontendRouter`, and save flow.
- Correct owner: `CreativeActiveIdentity` as source truth, with window fields
  derived only for compatibility receipts.
- Feature-add touch count: 6-8 files for identity-sensitive launch/save/routing.
- Likely reduction: high.
- Risk: medium.
- First safe slice: enumerate every production read of `window.activeCreative*`
  and classify whether it can read `CreativeActiveIdentity` instead.
- Do not touch yet: save semantics or frontend routing behavior.
- Proof: routing/save identity reader table and one stale-mirror case pinned.

### Rank 3

- Surface: active room plus collision install.
- Pressure type: multi-writer derived runtime state.
- Current symptom: `window.activeRoom` and `window.activeRoomCollision` can be
  built or cleared through scattered paths; one can drift from the other.
- Current owner: `Operations`, room activation, room editor state, gameplay
  controller, tests.
- Correct owner: one active-room/collision install service returning one result.
- Feature-add touch count: 8-11 files for runtime-room integration changes.
- Likely reduction: high.
- Risk: high.
- First safe slice: audit all writers/readers and stale-state cases only.
- Do not touch yet: install behavior, collision policy, gameplay/session
  routing.
- Proof: writer graph showing every active-room/collision assignment.

### Rank 4

- Surface: undo, stale, dirty, and bake-fresh signals.
- Pressure type: layered change state.
- Current symptom: document revision/dirty, app undo stack, window stale flags,
  bake refresh diagnostics, and save dirty state are all related but live in
  separate places.
- Current owner: `CreativeDocument`, `CreativeAppState`, `InputFrame`,
  `Operations`, `ReceiptBuilder`, standalone undo.
- Correct owner: document change tracker plus derived bake/cache key based on
  `(documentId, revision)`.
- Feature-add touch count: 8-10 files for changes that affect edit/rebuild/save.
- Likely reduction: high.
- Risk: high.
- First safe slice: document the change-state lifecycle for create, delete,
  move, undo, save, refresh, and open.
- Do not touch yet: undo semantics, dirty-drain behavior, auto-refresh behavior.
- Proof: lifecycle matrix showing who owns each state before and after each
  operation.

## Receipts And Tests

### Rank 5

- Surface: receipt mirrors and command receipt boilerplate.
- Pressure type: repeated field copying, append chains, and expected-field tests.
- Current symptom: command receipt fields are copied from command frame receipts
  into window mirrors and then appended into render receipts; tests manually
  pin hundreds of field names.
- Current owner: `ReceiptBuilder.hpp`, `ReceiptBuilder.cpp`,
  `UiCommandFrame.hpp`, `UiCommandFrame.cpp`, command receipt tests.
- Correct owner: typed receipt schema/adapters that serialize from source
  receipts without a wide mirror layer.
- Feature-add touch count: 5-9 files per new diagnostic field or command
  receipt family.
- Likely reduction: very high.
- Risk: medium.
- First safe slice: extract repeated copy/append patterns into named helpers
  without changing public receipt shape.
- Do not touch yet: public receipt keys, sticky receipt semantics.
- Proof: diff removes repeated copy/append code while receipt tests stay green.

### Rank 6

- Surface: Creative command receipt tests.
- Pressure type: weak-green schema testing.
- Current symptom: default receipt tests prove printed fields exist, but not
  whether live input or command routing writes those fields.
- Current owner: receipt tests with direct `record*` calls and long expected
  tuple lists.
- Correct owner: field-list descriptor plus behavior-backed samples.
- Feature-add touch count: 4-7 test/code touches per command field.
- Likely reduction: very high.
- Risk: low-medium.
- First safe slice: split "schema exists" from "production path writes it" and
  introduce grouped receipt assertions.
- Do not touch yet: receipt field names or production recorder behavior.
- Proof: fewer manual expected tuples, same public receipt output, plus one
  behavior-backed sample per subgroup.

### Rank 7

- Surface: product Creative world launch mega-suite.
- Pressure type: broad scenario tests and fixture duplication.
- Current symptom: 4,077 LOC, 35 manually called tests, long scripts mix launch
  identity, active-room bake, command receipt, save/session, undo, and dirty
  state.
- Current owner: per-test boolean functions plus custom test `main()`.
- Correct owner: focused launch/input/bake fixtures plus a small number of
  true integration smokes.
- Feature-add touch count: 4-8 files or long test edits for a new visible
  command path.
- Likely reduction: high.
- Risk: medium.
- First safe slice: extract assertion helpers and scenario fixtures only.
- Do not touch yet: scenario meaning or coverage scope.
- Proof: same scenarios pass, but assertions are named behavior checks rather
  than repeated receipt corridors.

## Registries And Catalogs

### Rank 8

- Surface: Creative object kind registry.
- Pressure type: enum, switch, descriptor, catalog, and test duplication.
- Current symptom: adding a kind touches enum/string identity, descriptor rows,
  mutation eligibility, UI/create palette, bake/projection/save tests, and
  fixture setup.
- Current owner: scattered descriptor plus object/mutation/app-local policy.
- Correct owner: descriptor registry/catalog as noun truth table.
- Feature-add touch count: 5-7 code surfaces plus tests.
- Likely reduction: high.
- Risk: medium-high.
- First safe slice: add an inventory/tally test or static report showing every
  policy location per kind; no behavior change.
- Do not touch yet: migrate all policy into descriptors in one pass.
- Proof: generated touch map catches missing descriptor/identity/policy coverage.

### Rank 9

- Surface: mutation registry.
- Pressure type: branch ladder and metadata duplication.
- Current symptom: new mutations touch enum, metadata rows, payload validation,
  payload factory/variant, allowed-mutation switches, apply switches, dirty
  flags, and tests.
- Current owner: mutation switch code and apply helpers.
- Correct owner: mutation metadata table plus typed apply adapters.
- Feature-add touch count: 5-6 code surfaces for metadata-only mutations,
  9-11+ when storage is involved.
- Likely reduction: high.
- Risk: medium-high.
- First safe slice: classify every mutation by payload kind, allowed descriptor
  profile, dirty domain, and apply path.
- Do not touch yet: mutation execution semantics.
- Proof: mutation registry audit table, including future-storage no-op exposure.

### Rank 10

- Surface: Creative UI command path.
- Pressure type: command routing duplication.
- Current symptom: command identity, semantic id, row kind, panel placement,
  draw text, handler table, receipt category, diagnostics, and tests are spread
  across several files.
- Current owner: UI model, draw-list, command frame, command catalog, receipt
  builder, tests.
- Correct owner: command catalog/handler registry.
- Feature-add touch count: 7-9 code surfaces plus 2-4 tests.
- Likely reduction: high.
- Risk: medium.
- First safe slice: make the catalog mirror all existing command metadata and
  prove parity; do not dispatch from it yet unless already wired.
- Do not touch yet: dispatch ownership or receipt shape.
- Proof: catalog row set, semantic ids, command kinds, receipt names, and
  handler expectations match existing behavior.

### Rank 11

- Surface: brush palette eligibility.
- Pressure type: descriptor policy duplicated in app-local affordance code.
- Current symptom: palette admission depends on shape kind, projection profile,
  bounds/transform flags, editor-only filtering, and standalone-specific helper
  chains.
- Current owner: standalone helper chain plus descriptor facts.
- Correct owner: descriptor/catalog palette policy queried by standalone/product.
- Feature-add touch count: 4 files for object-kind palette behavior today.
- Likely reduction: medium.
- Risk: low.
- First safe slice: extract existing helpers first, then move policy behind a
  descriptor/catalog query.
- Do not touch yet: object semantics or capture representatives.
- Proof: descriptor tests cover palette facts; standalone capture and palette
  counts stay stable.

## Branch Ladders And Live Routing

### Rank 12

- Surface: standalone Creative app.
- Pressure type: god integration file.
- Current symptom: Vulkan/capture, grid rendering, brush policy, placement,
  picking, gizmo/move, path editing, undo wrappers, RoomBake preview, save/load
  proof, logging, capture script, and the loop all live in one app file.
- Current owner: `apps/iggy3d_creative/main.cpp`.
- Correct owner: narrow app-local modules, with `main.cpp` as orchestration.
- Feature-add touch count: high for standalone shape/tool/proof behavior.
- Likely reduction: high.
- Risk: low-medium for extraction-only slices.
- First safe slice: behavior-preserving extraction of existing helper groups.
- Do not touch yet: main-loop behavior, capture proof semantics, RoomBake policy.
- Proof: `main.cpp` loses helper definitions only; final capture hash/log counts
  remain stable.

### Rank 13

- Surface: product `InputFrame` and input priority ladders.
- Pressure type: imperative mode-priority corridor.
- Current symptom: product frame input owns frontend mouse, Creative UI command
  routing, manual rebuild, viewport pick, tool dispatch, old room/map-maker
  gates, Navigate handling, revision/undo, and auto-refresh.
- Current owner: `src/app/iggy3d/window/InputFrame.cpp`.
- Correct owner: ordered input phase dispatcher with named phase handlers.
- Feature-add touch count: medium-high for overlays/input owners.
- Likely reduction: medium-high.
- Risk: medium-high.
- First safe slice: split named phases without changing order.
- Do not touch yet: priority order, mouse capture semantics, old-surface gates.
- Proof: exact same focused input tests pass; function length and branch density
  drop by phase.

### Rank 14

- Surface: opening/menu hit routing.
- Pressure type: hit-test geometry drift.
- Current symptom: raw bands and hardcoded row math coexist with drawn UI/hit
  regions.
- Current owner: `OpeningMenuView.cpp` plus input routing.
- Correct owner: emitted hit regions consumed by input routing.
- Feature-add touch count: 3-5 files for new menu/overlay surfaces.
- Likely reduction: medium-high.
- Risk: high.
- First safe slice: inventory drawn surfaces versus click surfaces and pin any
  mismatches.
- Do not touch yet: actual hit routing.
- Proof: hit-region map shows no hidden old-surface blockers before migration.

### Rank 15

- Surface: pause/menu action handling.
- Pressure type: frontend action ladder.
- Current symptom: rows, router actions, confirm behavior, save/load behavior,
  and state transitions can drift.
- Current owner: `ActionHandlers.cpp` plus router/draw surfaces.
- Correct owner: `FrontendAction -> handler` registry or transition table.
- Feature-add touch count: 3-6 files per new action.
- Likely reduction: medium-high.
- Risk: medium.
- First safe slice: group existing actions into a read-only handler inventory.
- Do not touch yet: transition behavior.
- Proof: action map covers every drawn route and every confirm path.

## Persistence And Bake Duplication

### Rank 16

- Surface: persisted Creative object fields.
- Pressure type: manual schema copy chain.
- Current symptom: a new field touches `CreativeObject`, create/restore
  validation, save envelope, document section mapping, codec write/read, fixture
  setup, equality assertions, and mutation surfaces if editable.
- Current owner: document, save section, runtime codec, mutation tests.
- Correct owner: Creative object field schema with explicit default,
  validation, save mapping, and equality/test helpers.
- Feature-add touch count: 6 code surfaces plus 3 tests; 12+ if editable.
- Likely reduction: high.
- Risk: high.
- First safe slice: field inventory with ownership/default/save/test columns.
- Do not touch yet: schema migration or codec behavior.
- Proof: field matrix catches save/load gaps without changing serialization.

### Rank 17

- Surface: RoomBake refresh pipeline.
- Pressure type: bake/install/receipt duplicate pipeline.
- Current symptom: RoomBake builds data, product refresh installs active room and
  collision, stale/fresh state is updated, and receipt counts are mirrored in
  multiple layers.
- Current owner: `RoomBake`, `Operations`, `InputFrame`, `ReceiptBuilder`,
  standalone preview.
- Correct owner: one Creative bake refresh service/cache result.
- Feature-add touch count: 7-9 files for new bake output classes or policy.
- Likely reduction: high.
- Risk: medium-high.
- First safe slice: consolidate result ownership and summary fields without
  changing bake policy.
- Do not touch yet: RoomBake classification, active-room behavior, no-renderable
  policy.
- Proof: explicit/manual/auto/launch refresh paths report from one result shape.

### Rank 18

- Surface: RoomBake object classification.
- Pressure type: descriptor policy and runtime semantic classification.
- Current symptom: shape support, safe Line handling, anchor handling, Room
  metadata skip, and unsupported shape counts are concentrated in RoomBake with
  product-facing consequences.
- Current owner: `RoomBake.cpp`.
- Correct owner: explicit RoomBake classification result per object, backed by
  descriptor facts.
- Feature-add touch count: 4-6 files for a new bake output family.
- Likely reduction: medium-high.
- Risk: medium.
- First safe slice: expose classification rows and test them before changing
  emitted output.
- Do not touch yet: emitted geometry or collision surfaces.
- Proof: classification test explains every skip/include count.

## App-Shell Bloat

### Rank 19

- Surface: standalone capture/proof scripting.
- Pressure type: proof script embedded in app runtime.
- Current symptom: deterministic create/move/delete/undo/save/load proof logic
  shares space with interactive runtime.
- Current owner: standalone app source plus partial capture-script header.
- Correct owner: `StandaloneCaptureScript` and capture-proof helpers.
- Feature-add touch count: medium-high for new proof steps.
- Likely reduction: medium.
- Risk: low if extraction-only.
- First safe slice: move capture-only logging and step orchestration out of the
  runtime loop.
- Do not touch yet: interactive placement/move behavior.
- Proof: final capture hash and `ROUNDTRIP ... match=1` log stay stable.

### Rank 20

- Surface: standalone picking/proxy/tool affordance duplication.
- Pressure type: parallel implementation of Creative affordances.
- Current symptom: standalone owns visual proxies, hit proxies, brush placement,
  path movement, and capture representatives separate from product Creative UI.
- Current owner: standalone modules and app loop.
- Correct owner: descriptor-backed visual/picking/placement policies consumed by
  standalone.
- Feature-add touch count: 8-10 code surfaces for a new shape/tool affordance.
- Likely reduction: medium-high.
- Risk: medium.
- First safe slice: keep extractions behavior-preserving, then compare product
  and standalone affordance policy.
- Do not touch yet: kernel/product behavior.
- Proof: standalone capture stays stable and extracted modules share constants
  instead of duplicating proxy dimensions.

## Parser And Config Ladders

These are real branch hotspots, but lower priority than live UI/runtime and
Creative ownership seams.

### Rank 21

- Surface: scenario and room-asset text parsers.
- Pressure type: parser/config ladders.
- Current symptom: `FixtureScenarioLoader.cpp` and `RoomAsset.cpp` dominate raw
  `else if` counts.
- Current owner: content parser files.
- Correct owner: parser tables only if schema churn continues.
- Feature-add touch count: low-medium per schema field.
- Likely reduction: low-medium.
- Risk: low for table extraction, medium for compatibility mistakes.
- First safe slice: leave alone unless parser churn resumes.
- Do not touch yet: parser semantics.
- Proof: parser unit tests and backward compatibility cases if touched.

### Rank 22

- Surface: save codec compatibility reads.
- Pressure type: versioned serialization branchiness.
- Current symptom: guarded optional reads create branch density in
  `SaveCodec.cpp`.
- Current owner: save codec.
- Correct owner: versioned field reader/schema table if fields keep growing.
- Feature-add touch count: 1-2 codec surfaces per saved field, plus higher-level
  document mapping.
- Likely reduction: low until schema churn grows.
- Risk: medium because compatibility is critical.
- First safe slice: no immediate action; keep compatibility tests strong.
- Do not touch yet: existing save wire format.
- Proof: old-save and too-new-save tests remain pinned.

## Execution Guidance

Recommended execution order differs from pure payoff rank:

1. Finish the normalized tally and review it against current dirty work.
2. Use low-risk extraction first where behavior can be proven by stable capture
   or stable receipts.
3. Reduce receipt boilerplate next because it is measurable and safer than
   state ownership surgery.
4. Build read-only catalogs and inventories for object kinds, mutations, and
   commands before migrating behavior into them.
5. Touch `ProductAppWindowState`, active-room/collision, and input priority only
   after the writer/read maps are complete.

The repo is not mainly suffering from LOC. It is suffering from repeated
ownership decisions. The target state is not "fewer files"; it is fewer toll
booths per feature.
