# Custom Engine Tools — Industry Survey v1.0 (2026-07-16)

What proprietary/in-house engine tooling converged on across the industry,
cut against iggy3d's engine layer. Companion to
`docs/creative_desktop_ui_field_survey.md` (which owns editor-UI anatomy —
none of that is repeated here). **Future engine/pipeline batches cite
coordinates from this doc** (section.item); version-bump on corrections.

Method: 13-agent survey — Naughty Dog, Guerrilla/Decima (+ Kojima adoption),
DICE/Frostbite (+ the BioWare cautionary tale), Capcom RE Engine, Ubisoft
(Anvil/Dunia/Snowdrop), Bungie (Halo→Destiny), Sucker Punch + Media Molecule
(Dreams), small-team engines (Our Machinery, Factorio, The Witness, No Man's
Sky, Animal Well, Croteam), a cross-cutting pipelines agent, plus three
critic-directed gap-fills: live C++ iteration, the DCC→engine boundary, and
nav/cover/traversal derivation. Primary sources throughout (GDC, CEDEC,
studio blogs, postmortems).

Judging rule: STEAL/SKIP for one developer maintaining a custom C++/Vulkan
engine, every investment cut against the playable vertical slice.

---

## 0. The convergent laws (every studio, independently)

1. **Iteration latency is the metric that kills projects.** Destiny 1:
   overnight map load + 20-min open + 20-min compile to move one node — the
   campaign's "Franken-story" was stitched from old pieces because nobody
   could afford fresh iteration. Capcom bet an entire engine rewrite on
   15 min → 10 s. ND designers quote "<30 s data builds" as what made tuning
   possible. Frostbite: 24 h → 3 s. Anthem's 24-hour lighting bakes
   "discourage people from fixing bugs." Snowdrop was FOUNDED on this.
2. **One data model, one mutation choke point.** The Truth, LunaEdit, Dreams'
   edit-list, our dispatcher — undo, diff, dirty-tracking, and collaboration
   are derived features of the model, never per-tool code.
3. **Three schemas; only storage is permanent** (Frostbite/Chabant): tool
   schema churns freely, runtime schema churns freely, the SAVE FORMAT is
   forever — cheap to get wrong on day one, ruinous to change once content
   exists.
4. **Derived data is never authored and never versioned.** Deterministic
   one-way derivation, content-hash keyed, regenerated at will; authored
   data is sacred and tiny. (Recast, TLOU posts, Decima, Sucker Punch,
   Capcom include-vs-reference.)
5. **At small scale the editor lives inside the game process.** Factorio
   FFF-252 is the definitive negative result: their standalone editor rotted
   because every action needed implementing twice; merging editor into game
   logic made every feature editor-capable for free. The Witness, Animal
   Well, Snowdrop agree. Out-of-process splits exist for console devkits and
   crash-isolating hundred-person teams.
6. **Determinism is the test oracle.** Factorio's heavy mode (save/load +
   CRC every tick), NMS's seed contract, Sucker Punch's ordered-append
   religion, FC5's same-input→same-output pipeline contract.
7. **Game-shaped engines ship; general engines don't.** Billy Basso lost 3
   years to a general 3D engine before Animal Well; Our Machinery — world-
   class engineers — shut down; BioWare's Anthem died on an engine grown for
   someone else's game. ND's Drake's Fortune postmortem: "we'd tried to be
   too clever… choose your battles."

**Verdict on iggy3d: already on the right side of every law.** Document-is-
truth + typed commands + one dispatcher + hash/save symmetry oracles +
headless capture + in-process editor is the architecture the survivors
converged on. What follows is deepening, not pivoting.

## 1. Measured invariants — cheapest, do first

- **1.1 Latency budgets as tests.** Pin `edit → bake → playable` wall time
  and suite wall time in CI; a regression is a broken build (Bungie's whole
  tragedy is this number compounding unwatched; Capcom tracks it fleet-wide
  at ~2 person-days/month upkeep).
- **1.2 Bake determinism oracle.** Same CreativeDocument → byte-identical
  baked room, as a pinned test. Extends the capture-hash invariant one layer
  down; it is what makes bake caching and CI regeneration trustworthy.
- **1.3 Heavy-mode lane (Factorio FFF-315, nightly on the box).** Replay a
  typed-command script; after EVERY command, round-trip the save codec and
  compare doc-hash before vs after. Composes the existing symmetry oracle
  into the strongest serialization invariant documented anywhere.
- **1.4 Quality dial rule (Bungie radiosity knob).** Any derivation that
  grows past seconds gets a draft mode with degraded output — never a
  faster-hardware plan. Corollary (Sapien "play from the edit form"): keep a
  runtime path that consumes the document directly so trying a change never
  waits on the bake.

## 2. Derived-data spine — the bake grows products

The nav/cover gap-fill's central finding: for a stealth game, spatial AI
data IS the core derived data, and industry practice is unanimous — derive
from geometry at bake, override sparsely in the document, revalidate cheaply
at runtime. **SKIP adopting Recast/Detour** (its voxelize front half exists
to regularize triangle soup; our occupancy index already IS the regular
form — integrating it buys a dependency plus float-nondeterminism risk
against the hash invariant). Steal its shapes instead:

- **2.1 Typed affordance schema on bake products, NOW, before content
  accumulates.** First-class enums/bitfields: walkable-area types, cover
  posts (facing + low/high class), traversal links with capability flags
  (thief-only climb vs guard-usable stairs) — Detour polyArea/polyFlags and
  TLOU typed-posts precedent. **This directly resolves the game-master-plan's
  one live seam break** (affordance wire-strings emitted by no bake path):
  schema fields on bake artifacts, validated receipt-oracle style, replacing
  stringly wire-tags. ~1–2 days.
- **2.2 Reachability derivation, natively.** Walkable-cell extraction +
  flood-fill regions over the occupancy index at bake; agent profile
  (step height, crouch clearance, jump envelope — thief vs guard) as a typed
  bake parameter producing per-profile layers. CI oracle: every patrol node,
  spawn, and objective sits on the spawn's connected component — fail the
  bake, not the play session. ~1–2 days on a grid.
- **2.3 Cover-post derivation (Killzone/TLOU pattern).** Walkable cells
  adjacent to solid columns → posts with facing + height class; runtime
  revalidates a candidate with a few existing raycast-LOS checks before an
  agent commits (baked data proposes, runtime disposes). Editor pin/delete
  overrides live in the document and survive rebakes. ~2–3 days.
- **2.4 Exposure map (TLOU) — highest gameplay leverage per byte.** Per-room
  visibility bitmap from occupancy + existing raycast LOS. Feeds AI path
  cost, the thief's intel-notebook rendering (what the scout can PROVE is
  unseen), and the CI assertion "a stealth path exists below exposure
  threshold." Heed the TLOU oscillation postmortem: route from low-frequency
  inputs, never per-frame recomputation. ~2–4 days.
- **2.5 Auto traversal links, derive-then-override.** Scan walkable-boundary
  columns for height deltas within the jump/drop/climb envelope; emit typed
  edges; editor authors exceptions as document override records. Industry
  honesty: auto-detection is the hard 20% (Recast never shipped it) — but a
  constrained grid makes it tractable where meshes weren't. Dying Light's
  postmortem is the anti-pattern ceiling: 50,000 hand-placed ledges, then
  scrapped. Never hand-annotate what geometry already implies.
- **2.6 Metrics sheet (ND discipline).** ONE checked-in constants table
  (step/drop/jump spans, cover heights, crouch clearance) shared by the
  motor, the bake detectors, AND the Blender kit's component BOMs — every
  kit piece annotation-compatible by construction; a metric change is one
  diff with test fallout, not a content audit.
- **2.7 Miniature derived-data cache.** Manifest of
  hash(inputs + baker-version + params) per artifact; skip on match
  (~100 lines — Dunia's DevPatcher pattern minus the farm). Bounded
  dependency edges: assert which types may reference which, so a local edit
  can never transitively dirty the world (Bungie's megatask fix, one assert
  for us).

## 3. Live iteration — data-side, deliberately

The gap-fill's platform finding is decisive: **the entire commercial C++
live-patching market (Live++, Recode, VS Hot Reload) is Windows-only; Mac-
primary is structurally outside it**, and the DLL-swap alternative carries a
standing contract (no statics, stable layouts, repointing discipline) that
"silently rots when violated" — a tax that pays off across a team, pure
overhead at n=1.

- **3.1 cvar/tuning layer first** (~1–2 days): auto-registering typed cvars,
  auto panel, persist to disk. Rule: any scalar you'll touch twice lives in
  a cvar or document property, never a recompile. This is 90% of what people
  buy Live++ for.
- **3.2 Watched constants** (afternoon): file-scraped tweakable literals for
  mid-experiment fudge factors; deleted when the value stabilizes.
- **3.3 Shader hot reload as data reload** (~1 day): file-watch GLSL →
  SPIR-V → pipeline recreate; MoltenVK tolerates it fine.
- **3.4 Fast-restart harness, measured.** Cold-start-to-restored-session
  (load doc + replay command log + reopen panels) under ~3–5 s IS hot
  reload without the lying-state failure mode (Blow's Witness model). Only
  if restart is irreducibly slow, carve ONE leaf module behind a C ABI —
  and macOS specifics are documented traps (dyld same-path handle caching:
  copy-with-rename per reload, the Godot #90108 bug; dlclose is a polite
  fiction; ad-hoc signing on Apple Silicon).
- **3.5 Command-log replay as the refactor oracle** (near-zero cost, ours
  uniquely): record (doc snapshot, command stream); replay headlessly before
  and after ANY code change; capture-hash equality = behavior-preserving.
  Handmade Hero's Day-23 loop upgraded — commands instead of raw input,
  immune to timing nondeterminism.
- **3.6 In-game debug menu culture (ND).** One persistent menu tree over
  every system's toggles/tuning, compiled in, gated like the shell.
  M-LAB already embodies this; extend rather than external-tool it.
- **3.7 Compile-time hygiene** (quarterly ritual): clang -ftime-trace +
  ClangBuildAnalyzer on the top-3 offenders; ccache Mac-side, ccache/sccache
  on the box. SKIP unity builds in the dev loop (kills incrementality and
  the cache); SKIP C++20 modules for now (two-toolchain bring-up).

## 4. Replay and bot oracles — the test culture's next rung

- **4.1 Solver bot for baked rooms (Croteam pattern — the highest-leverage
  new idea in the survey).** Croteam's bot physically plays every puzzle;
  headless time-lapse runs the whole game in ~20 min (~15,000 human hours
  equivalent). Ours: drive a thief-agent through each baked room asserting
  "stealth route exists / patrol can't softlock / objective reachable" —
  the playability regression oracle for a stealth game, and it exercises
  the ReconIntel seam for free. Gate bakes on it. (Frostbite AutoPlayers and
  Capcom's nightly replay fleet are the AAA forms; Capcom's two hard-won
  rules: segment scenarios so one failure doesn't void the night, and
  budget a small permanent maintenance tax.)
- **4.2 Top-down PNG dumps of derived data** (Decima lesson: they found
  SHIPPED bugs the moment baked data was visualized in 2D): occupancy,
  reachability regions, cover posts, exposure, patrol routes — through the
  existing headless render path, so every debug view doubles as a golden
  artifact. Debug-viz convention from the nav gap-fill: categorical colors
  PLUS printed numbers (Crytek reverted to numbers; score deltas are too
  small to read by hue), and a failure-reason string per rejected candidate.
- **4.3 Golden-image tests with a one-command bless workflow** (Sea of
  Thieves lesson: if adding a test isn't trivial, future-you won't).
- **4.4 Command-script runner** (Capcom Command Interface / Factorio Lua
  snippets, minus the embedded language): a text/JSON list of typed command
  payloads runnable headless over a document — batch surgery, macro
  authoring, and heavy-mode fixtures from one mechanism. SKIP embedding a
  scripting language (Capcom's IronPython is frozen at Python 2.7 forever —
  an embedded language marries you to its maintenance trajectory).
- **4.5 AutoBot sweep at 1% scale (Decima).** Scripted teleport to N fixture
  points → capture hash + PNG + loaded-asset list per point, diffed across
  commits; a JSON-lines file in git is the single-dev warehouse.

## 5. The DCC boundary — for when the import seam unblocks

The gap-fill's verdict is one coherent recipe (~7–10 days total, sequenced):

- **5.1 Stock glTF (.glb) as the only boundary format**, parsed with cgltf.
  SKIP FBX (proprietary SDK, reverse-engineered writer, macOS hostility),
  SKIP USD (composition-engine complexity, zero gameplay-metadata
  convention), SKIP a custom Blender exporter (The Witness needed a senior
  engineer owning that marriage for years — wrong 2026 call).
- **5.2 The ONLY Blender-side code: a ~100-line batch-export script** to a
  watched directory with pinned settings. Keeps Blender-API-churn exposure
  to one file. Pin the Blender version in-repo; exporters regress
  (T100954 silently dropped custom properties).
- **5.3 Import as pure hashed function**: key = hash(.glb bytes) +
  importer version + settings; derived cache outside git (Insomniac
  signature caches); oracle test "same glb in → byte-identical conditioned
  artifact out." Never hash raw export bytes as identity — no DCC
  guarantees byte-stable output across versions.
- **5.4 Metadata rides the asset, schema at the border**: Godot's suffix
  vocabulary subset (-col, -colonly, -noimp, SOCKET_ empties) + Blender
  custom properties → glTF extras, validated engine-side with hard failures
  on unknown keys. No side manifest files — nobody surveyed uses one.
  One-way pipeline: .blend = source, glTF = transport, never re-imported.
- **5.5 Conditioning via meshoptimizer** (weld/degenerate/cache-optimize)
  inside the hashed import step. Atlas rule as an assertion: exactly one
  material, name matches `palette_*`, else error. NO tangents (flat-shaded
  palette art) — if normal maps ever appear, reference mikktspace verbatim.
- **5.6 Auto-reimport engine-side**: watch → content-hash gate → re-import →
  hot swap via asset-handle indirection, expressed as a typed ReimportAsset
  command (headless-testable, gated under --capture). Capcom's deeper form
  as the north star: reload as a property of the HANDLE SYSTEM, so every
  asset type hot-reloads for free. Headless `blender --background` export
  validation runs on the box in CI, not in the interactive loop.

## 6. Document/command spine — deepening what exists

- **6.1 Persist the command history (Dreams — the big one).** Dreams'
  document IS an edit-list (100k edits: undo, tiny saves, scrubbing, remix,
  rollback all fall out). Ours: checkpoint + command suffix, every
  derivation a pure function of it, hash the list — the capture-hash then
  covers the entire authoring pipeline. Version-history-with-rollback
  becomes a feature, not a backup.
- **6.2 Schema as single source (ToolsDDL/DC at 1% scale).** One declarative
  field table per object type (X-macro or tiny codegen — no DDL compiler)
  generating save-codec entries, inspector rows, validation, payload types.
  The industry answer to exactly the god-struct/string-mirror drift this
  repo has fought; it's how 2001-era Blam tools survived 25 years.
- **6.3 Prototypes with per-instance overrides** (Our Machinery / Tiled
  classes): guard archetype carries defaults; a placed guard stores only
  deltas. Directly serves the asset doctrine's derivation chains.
- **6.4 Witness serialization details** (serve the hash invariant directly):
  deterministic sort by stable ID; no length-prefixed arrays in text
  formats; dual decimal+hex floats if text ever round-trips floats; and the
  sun-entity law — **tag editor-ephemeral/derived state so it is excluded
  from serialization, change detection, and the hash** (phantom diffs poison
  everything). Per-writer ID ranges from a checked-in file (Witness
  id_ranges) make cross-lane object-ID collisions structurally impossible —
  relevant with Codex as a second writer.
- **6.5 Budget thermometer (Dreams).** A live gauge in creative mode
  (occupancy / draw cost / doc size vs room budget) that warns at authoring
  time — the occupancy index makes this nearly free. Display budget as
  policy, don't hard-wire one cap (LBP3 lesson).
- **6.6 Deterministic decoration passes (NMS combinator economics + FC5).**
  Seeded, re-runnable variation passes (palette swaps, prop scatter, wear)
  over authored parts, reading document attributes, writing a derived layer
  that never touches hand-authored regions. Store seeds, never output; the
  hash oracle already verifies determinism. Multiplies one dev's low-poly
  kit geometrically.

## 7. Skip ledger — with the postmortems that earned each entry

- Out-of-process editor / RPC splits — team- and console-scale (Capcom
  needed devkits + WPF; Guerrilla's in-process pain never justified the
  ceremony at our scale; Frostbite's split cost a permanent two-language
  schema-conversion tax).
- C++ hot-reload products — Windows-only market, full stop. DLL-swap only
  if a measured restart bottleneck appears, one leaf module max.
- Embedded scripting language — Capcom's Python-2.7-forever; ND's own DC
  adoption pain WITH dedicated compiler engineers.
- Visual scripting / node-everything — exists to unblock hundreds of
  non-programmers; a graph editor+debugger+versioner is a permanent tax
  with an audience of one (Snowdrop, Frostbite Schematics, Decima
  NodeGraph unanimity).
- Build farms, distributed caches, Perforce topology, semantic content
  merge, multi-user editing — coordination machinery for headcount we
  don't have. The two-agent tree share is solved by commit discipline.
- Recast/Detour as a dependency; dynamic/tiled navmesh (SC Conviction
  needed a dedicated AI tech lead); runtime ledge scanning (Dying Light
  burns ~200 rays/frame for open-world player freedom a grid doesn't need).
- Houdini/DCC-embedded tooling — single-operator pipeline bottleneck
  (GRW postmortem); fat Blender add-ons break with every release.
- Packing/archiving during development — Killzone-era artists lost their
  iteration loop to it; loose files measured FASTER from cache.
- Hybrid iteration/ship asset system — Butcher's own regret; keep two
  honest paths (document-direct for iteration, bake for ship).
- UGC-platform ambitions — Dreams, with Sony money, couldn't sustain it.

## 8. Doctrine sentences (adopt verbatim)

- "Shims are cancer" (Butcher): never a temporary API between editor and
  runtime; land the dependency first or build an honest throwaway.
- "Code beats documents" (Butcher): a workflow rule that matters is a test,
  assert, or API shape — never prose.
- "Do not validate game data by human hands" (AC Origins/Routhier).
- "Doesn't exist > Asleep > Optimized > Naive" (Sucker Punch): cull work
  classes before optimizing them.
- "Document sacred, backends disposable" (Dreams axed four renderers, never
  the edit format).
- "Reduce time in-between applications" (Guerrilla): the change→see-it loop
  outranks any feature.
- "You are not your users" (FC5/Capcom UX): even at n=1 — prototype the
  workflow, don't build the first idea; note friction the moment you feel
  it.
- Never rebuild the toolchain mid-production (Insomniac web tools;
  Guerrilla called their own mid-Horizon rebuild a gamble that happened to
  pay off).

## 9. Sequencing for one developer (leverage order; Ace cuts orders)

1. **§1 measured invariants** — days, and they insure everything else.
2. **§2.1 typed affordance schema** — closes the known seam break; then
   §2.2 reachability + CI oracle, §2.6 metrics sheet.
3. **§4.2 derived-data PNG dumps + §4.3 goldens** — highest tooling ROI/hour.
4. **§3.1–3.3 cvars, watched constants, shader reload** — the live-feel
   package, ~3 days total.
5. **§2.3–2.4 cover posts + exposure map** — the stealth game's spine.
6. **§4.1 solver bot** — the playability oracle; grows with every room.
7. **§6.1 persisted command history + §6.2 schema-as-source** — the deep
   structural investments, when a batch slot opens.
8. **§5 DCC boundary** — the day the import seam unblocks, the recipe is
   written.
