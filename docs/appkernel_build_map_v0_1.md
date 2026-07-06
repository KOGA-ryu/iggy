# AppKernel Build Map v0.1 — kill the 18-files-per-feature amplification

**Author:** Claude (planner). **Date:** 2026-07-06. **Status:** map only — gated by
[core_spine_work_rules.md](core_spine_work_rules.md). This is a multi-slice campaign, not a refactor.
Each slice is independently valuable and independently gated. **Do not land it as one big bang** —
that would *be* the 700k-LOC refactor we're avoiding.

---

## 1. The measured pain (Gate 0, field 1)

One feature crosses 14–22 files. Real specimens from git:
- `18e7de94 "add standalone oriented box picking"` → 14 files (`StandalonePicking` ×2, `PreviewProxies`
  ×2, `main.cpp`, cmake, tests, docs).
- `8791e365 "land creative builder queue seams"` → 22 files (`Document`, `DocumentWireframe`,
  `ObjectDescriptor`, `MutationApply`, `SpatialProjection`, `RoomShell`, `RoomBake`, `Operations` + 8
  test files).

**Two distinct amplifiers, precisely located:**

| # | Amplifier | Evidence | Fix family |
|---|---|---|---|
| **A** | **The god-struct.** `ProductAppWindowState` = **~615 fields** (`ReceiptBuilder.hpp:160-811`), touched by **66 files**. A feature adds fields here + threads them through receipts/input-frame/operations/UI/bridge/automation. | 615 fields / 66 files | decompose into owned app-Systems |
| **B** | **Reader fan-out.** A change to the authoring model fans to every subsystem that reads `CreativeDocument` (Document, Wireframe, Descriptor, Mutation, SpatialProjection, RoomShell, Bake) because each reader hand-codes the shape. | specimen B | a capability/descriptor seam + one projection point |
| **C** | **Wiring coupling.** `main.cpp` (1651 lines) is edited for every feature because systems are wired by hand there. | specimen A | a System registration seam |

**Crucial scoping fact:** `src/runtime` is **already decomposed** into 24 systems/stores with boundaries
(`MovementSystem`, `PhysicsBodyStore`, `InteractionSystem`, `clock`, …). **The amplification is
entirely in the app/window/product layer.** This is an app-layer coordinator build, NOT a runtime rewrite.

## 2. The target (what "the kernel" is here)

An **`AppKernel`**: the app-layer runtime coordinator that OWNS app lifecycle and a registry of owned
**app-Systems**, each owning its slice of what is today `ProductAppWindowState`. It owns *boundaries,
not domains* — it does not implement snapping/baking/selection; it creates systems in order, routes a
frame through them, and shuts them down in reverse. The 615-field struct becomes a shrinking shim, then
dies, as each System claims its state.

`AppKernel` is the **one** sanctioned kernel name (per doctrine). Everything it owns is a `System`/
`Store`. No `SaveKernel`/`SelectionKernel`.

## 3. Build on what exists — do NOT reinvent

- The runtime's own `System`/`Store` pattern (copy its shape upward to the app layer).
- The in-flight `CreativeAppState` extraction (creative-decoupling) — the first proving-ground System.
- The **receipt/mirror discipline** (`ReceiptBuilder`) — reused as the *additive migration shim*:
  a field stays mirrored in the god-struct (tests pin it) while its owning System is extracted, so no
  behavior changes and nothing lands big-bang.

## 4. Components in dependency order (the build)

```txt
L0  AppKernel skeleton
      Owns: startup (absorbs AppShell), the main loop (owns window/Loop), shutdown ORDER,
            and a registry<AppSystem>. Additive: wraps the existing entry; behavior identical.

L1  AppSystem boundary + registration seam
      One contract: init(ctx) / tick(frame) / shutdown(); each System owns its own state.
      Systems self-register -> main.cpp stops being edited per feature (kills amplifier C).

L2  Extract app-Systems from ProductAppWindowState, ONE domain per gated slice
      Candidates (each a slice): CreativeAppState (in flight), SaveBrowserSystem, AutomationSystem
      (exists), GameplayViewSystem, InputRoutingSystem, DiagnosticsHudSystem, ...
      Each extraction: state moves into the System; god-struct keeps a shrinking mirror (shim) until
      that domain's readers move over; then the mirror is deleted. Kills amplifier A, one domain at a time.

L3  CreativeDocument capability/descriptor seam
      A new object capability is declared in ONE descriptor row + consumed via a stable interface, so
      Document/Wireframe/Descriptor/Mutation/SpatialProjection/RoomShell/Bake read a contract, not the
      raw shape. Extends the existing ObjectDescriptor pattern to capabilities. Kills amplifier B.

Core systems the AppKernel owns (gated SEPARATELY, not part of the decomposition):
      JobSystem (bake freeze -- already preflighted, jobsystem_bake_preflight_v0_2.md).
      Clock already exists (runtime/clock); wire it through the kernel if/when real-time is chosen.
```

## 5. Boundary — Owns / Does NOT own

```txt
AppKernel OWNS:                          AppKernel DOES NOT OWN:
  app startup + shutdown ORDER             any domain logic (snap/bake/select/move)
  the main loop + frame routing            CreativeDocument authoring truth (a System owns it)
  the app-System registry + lifetimes      runtime simulation (Session/SessionTick already own it)
  who-ticks-before-whom ordering           Vulkan resource semantics (render System/backend own it)
  the frame-level completion drain point    gameplay rules
```

## 6. Migration strategy — why this is NOT the refactor you fear

- **Additive + incremental.** The god-struct and its mirrors STAY while Systems are extracted behind
  them; the existing 240+ tests pin behavior at every step (same pattern already used for the
  creative-decoupling cuts). Nothing is rewritten wholesale.
- **One System per slice, each independently shippable.** Each extraction reduces amplification for
  *that* domain immediately — value on every slice, no "big reveal at the end."
- **The god-struct dies by subtraction,** not by a rename commit. When its last field has an owner, it's
  deleted — and that deletion is the final proof, not the plan.

## 7. Acceptance metric (per slice) — the amplification must visibly drop

Every slice names the domain's **next real feature** and shows it now lands in **≤3 files** instead of
N. No metric drop, no merge. (This is the whole point; a slice that doesn't shrink the touch-count is
architecture theater and gets reverted per §10.)

## 8. Review gates (per slice, from the doctrine)

```txt
G1 slice preflight accepted (which System, which fields move, which mirror stays)
G2 System type added, state still mirrored in god-struct (behavior identical, tests green)
G3 readers of that domain moved to the System; mirror marked deprecated
G4 god-struct mirror for that domain deleted; tests still green
G5 amplification-drop demonstrated on the domain's next feature (<=3 files)
G6 naming + ownership audit
```

## 9. Tests

- **Regression (the spine's safety net):** the full existing suite (240+) must stay green at every
  gate — it is the oracle that the shim-then-extract preserved behavior.
- **Per-System unit:** the extracted System owns/updates its state correctly in isolation.
- **Integration:** AppKernel starts systems in order, routes a frame, shuts down in reverse; no
  use-after-free across the shim boundary.
- **Amplification metric:** the §7 touch-count demonstration, recorded per slice.

## 10. Non-goals / deletion path

- ❌ No big-bang reorg; no renaming the runtime (it's already decomposed); no moving `Session`/
  `SessionTick`/simulation into the app layer.
- ❌ No new `Kernel`-named types besides `AppKernel`.
- ❌ No speculative Systems with no state to own — a System exists only to claim real god-struct fields.
- ❌ Not blocked on the JobSystem or a Clock — those are separate, independently-gated core systems.
- **Deletion path:** each slice is one System behind a mirror; if it doesn't drop the amplification
  metric (§7), revert the extraction and restore the god-struct fields — the mirror made rollback one
  commit. The AppKernel skeleton (L0) is behavior-identical, so it too reverts cleanly.

## 11. First slice (the beachhead)

**L0 `AppKernel` skeleton + L2 first extraction = `CreativeAppState`** (already in flight —
creative-decoupling). Reasons: the boundary is already user-mandated and partly built; it's the
highest-churn app domain; and finishing it *as the template `AppSystem` under the new `AppKernel`*
proves the whole pattern (shim → extract → delete → metric) on a real domain before it's applied to
the other ~10. Ship that, show the next creative feature landing in ≤3 files, then repeat domain by
domain until the 615-field struct is gone.

## 12. Verdict

Justified: yes — the pain is measured (615 fields / 66 files / 14–22-file features), the fix family is
precise (app-layer System decomposition, not a runtime rewrite), a proving-ground domain exists
(`CreativeAppState`), and the migration is incremental-behind-mirrors so it never becomes the refactor
it prevents. The honest work is a **sequenced campaign of gated System extractions**, each proven by a
dropping touch-count — starting now, before the god-struct grows past 615.
