# The Operating Loop v1.1 — how work flows from idea to landed

Distilled from the E100–E230 era: what the decomposition proved, what the receipt-table failure exposed, what
the perception review demanded. One loop, two modes, six stages, one scoreboard. Planner cuts cards, builder
(Codex) lands them, and — the part that was missing — **the loop closes on intent, not just gates.**

---

## 0. TWO MODES — right-size the process to the work

Every card declares its mode. The mode decides the gate set.

**MODE P — PRESERVATION** (refactor, debt, decomposition, dedup)
Behavior must not change. The golden + suite + compiler are the net. Ceremony is cheap insurance here:
preflight, byte-identical oracle, serial execution. This is what the existing machine is great at.

**MODE C — CREATION** (features: AI behavior, movement, abilities, tools, physics)
Behavior is *supposed* to change — **there is no golden to save you.** Judgment replaces the oracle:
a design ruling before build, pin-tests that assert the new behavior, and a planner read of the result diff.
Plus the observability law: **no invisible behavior** — every mode-C card names how the new behavior is
*observed* (debug overlay / receipt / test reading the same truth struct the sim produces).

**THE SPINE (never lightened, either mode):** kernel/job boundaries, save/replay/StateHash, cross-thread,
document-mutation ownership. Full GATE per `core_spine_work_rules.md`, always.

## 1. SELECT — the value gate comes before the correctness gate

The receipt-table failure passed every correctness gate and was still the wrong work. So selection is a gate:

- Pull from the live lane docs (`creative_debt_lane*`, `perception-3d-maximum`, `refactor_targets`,
  `decomposition_status`) — but rank by **value ÷ effort**, not card order.
- **Effort-inversion guard:** the biggest effort in flight must map to the biggest value. If an XL card is
  low-value, demote it — no matter how satisfying it looks.
- At equal effort, **the game (runtime lane) outranks the shell (app lane).**
- This is also where disagreement is cheapest: the planner stress-tests the premise *here*, before it hardens
  into a card. Argue at selection, not at autopsy.

## 2. RECON — verify to build-ready

- Read-only. Every claim carries `file:line` **at HEAD** — no "re-anchor later" hedges on a card being released.
- **Spot-check every scary claim** — recon agents hallucinate (the `fastWindow` incident). A claim that would
  change the plan's shape gets independently verified before it becomes an instruction.
- Classify the work: **deficit** (needs ownership/freshness design) vs **structural** (mechanical regroup) vs
  **feature** (needs a design ruling from the user first). Misclassification is how S becomes L.

## 3. CUT THE CARD — the contract

Every card carries:

1. **Mode** (P/C) and the goal in one line.
2. **THE METRIC PROMISE** — a measurable direction the change must move:
   *"receipt/\*Fields.cpp total LOC goes DOWN"*, *"edits-per-feature ≤ 2"*, *"new pin-test asserts X"*.
   Mechanical, checkable with `wc -l` / `git diff --stat`. The receipt-table failure dies at slice 1 here.
3. Owner files (HEAD-verified), method (**compiler-guided, never `replace_all`**), laws + *verified* hazards only.
4. **THE STOP CONDITION:** *"if reality contradicts this card's premise, STOP and report — do not satisfy the
   letter of the card."* The builder is required to refuse a broken premise, not complete it.
5. Exit criteria: the gates (§5a) + negative-greps + the metric check.

Stage in `blocked/`, release **one at a time on a clean tree**, serial on shared files.

## 4. BUILD — Codex

Compiler-guided; the compiler is the exhaustive reader-finder (grep undercounts; text replace corrupts).
Commit as `claude: planned. codex: <what happened>`. Invoking the STOP condition is a *success*, not a failure —
it gets the same commit dignity: `codex: stopped <card> — premise broken: <why>`.

## 5. VERIFY — the closed loop (the fix)

**(a) Gates — mechanical, every card:**
- Build green — **all targets, including `tools/`** (a tool rotted under a green gate once; never again).
- Full suite green. Mode P: golden **byte-identical**. Mode C: the new pin-tests green + any golden change is a
  single reviewed append.
- Ledger/TSV updates in the same commit.
- **THE METRIC CHECK:** did the promised number move the promised way? `wc -l` before/after. Automatic,
  no judgment needed — this alone catches "tripled the file it meant to shrink."

**(b) Intent check — judgment, planner:**
- After **slice 1** of any multi-slice effort, the planner reads the actual diff against the card's *purpose*
  and makes an explicit **kill-or-continue** call. Kill wrong paths at commit 2, not commit 16.
- Mode C always gets a result-diff read — there is no oracle standing behind it.

**(c) Standing audit — cadence, not suspicion:**
- Every ~10 landed cards, one read-only critique slice (the 5-axis rubric: correctness, change-cost, contract
  clarity, risk coverage, fitness-for-next) over the recent work — *even when nothing looks wrong*. Quality
  review is scheduled, not triggered by unease.

## 6. LEDGER — docs are the spine

- Flip the status line (`decomposition_status.md` pattern) the moment a card lands. A stale ledger is worse
  than none.
- Corrections version-bump the maximum they came from and get folded in — never live only in chat.
- Chat memory is fog with a nametag. If it isn't in the repo, it didn't happen.

## 7. THE SUBTRACTION RATCHET — runs across everything

- **Weight metrics only go down:** instrumentation LOC, golden-pinned field count, edits-per-feature.
  Tracked on the scoreboard; a card may not raise them without naming the contract that justifies it.
- **Boy-scout with teeth:** any card touching an over-built area must leave it lighter.
- **Rule of three:** abstract at the *third real use*, never the first. Tolerate small duplication early;
  merging three concrete things beats un-abstracting one wrong guess.
- **Blast-radius budget (mode C acceptance):** `new files = 1 · existing edits = 1 (as data) · test = 1 ·
  instrumentation edits = 0 · cross-lane = 0`. Every leak past budget **names a missing seam** and
  auto-appends to the debt backlog — the seam backlog generates itself from real feature demand.

## 8. ROLES & THE FOLDER-SPEC LAYER (v1.1 addition)

**The team:** PLANNER (Claude) — plans, rulings, cards, pin-tests, intent checks. **BUILDER** (Codex) —
lands slices, invokes STOP conditions. **REVIEWER** (Codex) — verifies gates, runs the standing ~10-card
audit, and **owns folder-spec freshness**. **RESEARCHER** (Codex) — works the queue in
`docs/research_requests_v1.md`; findings feed rulings as evidence.

**Folder specs (`AGENTS.md`, with `CLAUDE.md` symlinked beside it):** per major folder, auto-ingested by
both harnesses — the repo's skeleton so agents verify instead of reconstruct. Rules:
- **Shape only, never state:** purpose, data flow, laws, seams, hazards, pointers to the deep docs. NO line
  numbers, NO file lists, NO status, NO field counts (those rot in days). Anchor by symbol. ≤ ~60 lines.
- **Every spec carries `Verified at: <commit>`** — staleness must be visible, not discovered.
- **Reviewer refresh at each plan MILESTONE** (a plan phase satisfied): re-verify the specs of every folder
  the phase touched, update shape if wiring changed, re-stamp. Between milestones, builders trust specs for
  shape and verify state at card-cut (the existing re-anchor rule) — the two layers cover each other.
- A spec's checkable claims (include-direction, ownership) should graduate into tests where cheap (the TSV
  pattern); the rest stay stamped prose.
- Exemplar: `src/runtime/ai/AGENTS.md`. Roll-out to remaining lanes = reviewer's milestone work, one folder
  at a time, each verified at HEAD when written — never batch-generated from memory.

## THE SCOREBOARD — per card, cheap, mechanical

| metric | source | pass |
|---|---|---|
| LOC delta in owner area | `wc -l` before/after | moves as the metric promise said |
| existing files edited | `git diff --stat` | ≤ the card's budget |
| golden delta | `git diff` on `*.golden` | P: zero · C: one reviewed append |
| pinned-field count | oracle field count | never up without a named contract |
| suite | `ctest` | green; count goes UP when tests were promised |
| tools/ | full-target build | compiles |

## FAILURE MODES THIS LOOP EXISTS TO KILL (each earned its line)

| failure (the receipt) | the counter |
|---|---|
| Letter-of-the-card, purpose defeated (receipt tables, +5,674 LOC) | metric promise + STOP condition + slice-1 intent check |
| Recon hallucination hardened into instructions (`fastWindow`) | spot-check scary claims before carding |
| Green gates, wrong work | value gate at SELECT + standing audit cadence |
| Effort inversion (XL on low-value, nibble on high-value) | effort-inversion guard at SELECT |
| Tool rot under a green build | all-targets gate incl. `tools/` |
| Silent behavior in new features (guards that cheat, untested) | mode C: pin-tests + no-invisible-behavior law |
| Stale plans re-litigated (the corpse pile) | ledger flips at landing; version-bumped maximums |

---
**The loop in one line:** *select by value → recon to build-ready → card with a metric promise and a stop
condition → build compiler-guided → verify gates AND intent at slice 1 → flip the ledger → and subtract weight
on every pass.*
