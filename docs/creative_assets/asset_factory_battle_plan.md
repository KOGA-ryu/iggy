# Asset Factory Battle Plan — the pipelined production doctrine

Companion to `detailing_program.md` (which holds the fidelity rubric, detail
levers, protocols P1–P8, and the exhaustive per-kit detail list). THIS doc is
the **operating model**: how to run asset production as a pipeline so the box —
the only serial resource — is never idle and never the thing we wait on.

---

## 1. The Factory Model (what we converged on)

Three lanes run **concurrently**, each consuming a different resource:

| Lane | Who | Resource | Work |
|---|---|---|---|
| **A — Author** | agent Workflows (4–10 parallel) | tokens | write/detail generator scripts, adversarial preflight (find→skeptic-verify), draft READMEs/fixtures |
| **B — Fabricate** | the box | box CPU | Blender generation (2–3 kits can run concurrently), emit/measure, C++ build, ctest suite |
| **C — Integrate** | me | attention | assemble + my-eyes review (P6), splice, pins, visual gallery review, commit, memory |

**Pipeline law:** while the box fabricates kit N, agents author kits N+1..N+k,
and I integrate/review kit N−1. Three kits are always in flight at different
stages. The failure mode to avoid is everyone waiting on one stage — which is
what happens without a queue.

**Queue discipline:**
- Lane A keeps ≥2 preflighted generator bodies in the ready queue at all times.
- Lane B batches: generate 2–3 kits per Blender window, then ONE build+suite
  for the whole batch (compiles are the serial cost — amortize them).
- Lane C commits per kit or per coherent batch; never mixes lanes in a commit.

**Box scheduling:** Blender jobs are I/O-light and can run 2–3 wide on 28
cores; `cmake --build -j28` and ctest want the machine to themselves. So the
rhythm is: [gen, gen, gen] → [one build+suite] → [renders], repeat.

## 2. Standing roles

- **Ace** — go/no-go per phase; delivers reference textures/concepts
  (`reference_requests.md`); playtests; signals when Codex's marathon lands
  (which unblocks reconciliation — until then everything stacks on asset-bld-4).
- **Me** — foreman: plans batches, runs Lane C, drives the box over SSH, owns
  every commit, adjudicates when preflight verdicts disagree (lesson: check the
  geometry myself), maintains the lessons list.
- **Agent teams** — authors and skeptics only. They never touch the box repo
  directly; they write bodies to scratchpad, I assemble/integrate. (This is what
  keeps shared contract files collision-free.)
- **The box** — fabricator. Nothing thinks on the box; it generates, measures,
  compiles, tests, renders.

## 3. What fills "waiting on the box" (Lane A backlog, always available)

Whenever the box is busy, spend tokens on, in priority order:
1. Detailing the next kits' generators (the §5 list in `detailing_program.md`).
2. Building/refining factory tooling (see Phase 0 below).
3. Preflighting queued bodies (adversarial lens sets).
4. Authoring the *next* batch of new-kit bodies (backlog: traversal variants,
   kitchen/feast, garden/courtyard, ruins/catacombs, docks/harbor, library/
   study, chapel, alchemist, jail-wagon/gallows, siege).
5. Drafting fixture compositions + README prose for kits in the queue.
6. Reviewing galleries from the last batch (visual QA never blocks the box).

## 4. Phases

### Phase 0 — Force multipliers (build BEFORE the next big batch)
The one-time investments that raise all later throughput:
- **N-gon shared block** (`SEG=16` default; same helper names/signatures so
  every existing + polished body upgrades for free on reassembly). Biggest
  visual win in the whole program.
- **`reintegrate_kit.py`** — one command per kit: regenerate → emit → replace
  spec map → lint --write → (batch) build+suite hook.
- **Static pre-checkers** run before any box generation: socket-name
  `[a-z0-9_]`, grounding, degenerate boxes, coplanar faces — the classes that
  bit us (P6/P8).
- **Socket-name rule folded into lint_contracts.py** (closes the linter gap
  that let awning_rail through).
- **`before_after.py`** — old-vs-new gallery montage for every re-integration.
- Risk check to close: fixtures embed measured bounds; confirm bound drift from
  re-detailing doesn't break any fixture assertion (roster tests don't check
  bounds; verify the hand-authored socket fixtures don't either), else refresh
  fixtures as part of reintegrate.

### Phase 1 — Re-integrate the 10 polished kits (queue fuel already in hand)
The polish pass (sub-part detail, Lever B) is authored + preflighted for:
heist, armory, lighting, mannequin, interior, tavern, dungeon, guard, grove,
marsh. Reassemble each against the NEW N-gon block (Levers A+B together),
then pipeline in batches of 3–4: [gen ×3] → [build+suite] → [before/after
review] → commit. Order: heroes first (heist, armory, lighting, mannequin),
then room sets, then grove/marsh.

### Phase 2 — Detail the remaining 13 kits
Author detail passes (Lane A) for: workshop, woodland, crag, farm, riverbank,
rigging, portal, animal, market, then structural/legacy at T1 (architecture,
infrastructure, settlement, simulation, homestead, cave, rock_cliff,
stealth_blockout). Same pipeline rhythm. The §5 list is the spec.

### Phase 3 — New content batches (only after detail parity)
Resume new kits (backlog in §3.4) — now authored directly against the N-gon
block and the pre-checkers, so they arrive at T2 by default.

### Phase 4 — Texture intake (event-driven, whenever Ace delivers)
Real seamless tiles from `reference_requests.md` land → swap procedural tiles
(the retexture map pattern from the gold/steel pass), re-measure pins, re-render
galleries. Unlocks T3 material richness (Lever F) for the hero kits.

## 5. Battle rhythm (per batch, the loop I run)

1. PLAN: pick the next 3–4 kits; confirm bodies are preflighted + my-eyes
   reviewed (P6: export prefixes, heads, socket names, dropped constants).
2. FABRICATE: assemble → pre-checkers → generate (2–3 wide) on the box.
3. MEASURE: emit_kit each; replace spec maps; pins per P4 (measure, never guess).
4. GATE: one build + full suite for the batch (playtest_process_owner excluded).
5. REVIEW: galleries + before/after; any render-only defect loops that kit back
   to Lane A (its batch slot is not held open).
6. COMMIT: per kit or per batch, my-hunks-only, lane-isolation check, push.
7. RECORD: memory lessons; queue state; launch the next Lane-A workflow BEFORE
   starting the next fabricate window (keep the queue ≥2 deep).

## 6. Current state (2026-07-23)

- 23 kits / 699 assets shipped, linter green @ f5124d0f; recon still BLOCKED on
  Codex's marathon (Ace signals).
- Lane A output IN HAND: 10 polished generator bodies (preflight-verified) in
  scratchpad — Phase 1 fuel.
- Tooling in hand: assemble/emit/splice/roster/render. Phase 0 items are the gap.
- Awaiting from Ace: phase go/no-go; eventually Tier-1 textures (Phase 4).
