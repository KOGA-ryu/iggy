# Receipt / app-state decomposition — handoff to Codex

**Owner of this work order:** whoever picks it up next (handed to Codex 2026-07-06).
**Lane:** app-layer infrastructure under `src/app/iggy3d/` (ReceiptBuilder + ProductAppWindowState).
**Goal of the whole effort:** kill the "18 files per feature" change-amplification by decomposing
the two app-layer god-artifacts so a feature edit recompiles a small TU, not a monolith.

---

## 1. What is already done (do NOT redo)

Three slices landed on `iggy3d-main`, all behavior-preserving, suite green at **255/255**:

| commit | slice | effect |
|---|---|---|
| `2b3e02d1` | 1 — extract the struct | `ProductAppWindowState` + diagnostics moved out of `ReceiptBuilder.hpp` into `ProductAppWindowState.hpp`. `ReceiptBuilder.hpp` 480 → 62 lines. |
| `150ff268` | 2 — re-point includers | 39 files that only needed the *struct* now include `ProductAppWindowState.hpp`; only the 19 real receipt-API users still include `ReceiptBuilder.hpp`. |
| `3f047d4d` | 3 — carve the builder | `buildProductAppReceipt` 1844-line warehouse → 58-line orchestrator + 10 domain appenders under `src/app/iggy3d/receipt/`. `ReceiptBuilder.cpp` 2673 → 592 lines. |

The current `src/app/iggy3d/receipt/` set: `ReceiptFields.hpp` (shared decls + `floatReceiptValue`),
`ReceiptFields.cpp`, and 10 `*Fields.cpp` domain appenders. `buildProductAppReceipt` is now:
derive setup-locals → call the 10 appenders in order → emit the `windowFailed` tail inline → return.

---

## 2. The verification gate (run this after EVERY slice — non-negotiable)

A slice is behavior-preserving only if all three pass:

```bash
cd /Users/kogaryu/iggy3d
cmake -S . -B build && cmake --build build -j8      # must be clean
ctest --test-dir build                              # must be 255/255 (or higher if tests added)
```

Plus the **receipt-order oracle** — the receipt is an ordered key/value list; reordering or dropping a
key silently breaks the goldens even if it still compiles. Diff the ordered literal keys before vs after:

```python
# multiline-aware — a plain grep MISSES appendReceiptField calls whose "key" is on the next line.
import re, subprocess
KEY = re.compile(r'appendReceiptField\(\s*receipt,\s*"([a-z0-9_]+)"')
before = KEY.findall(subprocess.check_output(["git","show","HEAD:src/app/iggy3d/ReceiptBuilder.cpp"],text=True))
# ...reconstruct `after` from the new files in call order, then assert before == after
```

For slice 3 this was **901 keys, identical order**. Keep that invariant.

---

## 3. The proven pattern (both remaining slices use it)

1. **Recon before cutting.** Map the exact line ranges, the read-dependencies, and the linkage of every
   helper. A block is safe to extract only if its range is contiguous and its boundary never splits a
   loop/if-body or falls between a local-var declaration and a later use of it.
2. **Deterministic extraction, not retyping.** Cut *verbatim line ranges* (a Python/`sed` slice), wrap in
   `namespace iggy3d { <signature> { <verbatim body> } }`. Never let an LLM paraphrase the body — that is
   how field values drift.
3. **Replicate the monolith's includes into each new TU.** The monolith compiles its whole body with its
   own include set, so *any contiguous sub-range compiles with that same set*. Copy the include block; over-
   inclusion is harmless and gives a clean first build.
4. **Cut bottom-up** (highest line ranges first) if editing in place, so earlier line numbers don't shift.
   Or do one atomic Python pass against the original line array (what slice 3 did).
5. Add new `.cpp` files to `CMakeLists.txt` right next to `ReceiptBuilder.cpp` (explicit list ~line 166).
6. Run the gate. Commit precisely (see §6).

---

## 4. Slice 5 — relocate the `record*` mutation-recording path (NEXT, mechanical, low-risk)

`ReceiptBuilder.cpp` (592 lines) is now almost entirely the *state-writing* path, which is a separate
concern from the *receipt-reading* orchestrator. Current structure:

- **1–275** — anon-namespace helpers for the record path: `setPhysicsMovementPlannerProof`,
  `copyProductCreativeUiCommand*Diagnostics` (create/delete/undo/roomShell/mutation), `reset*`,
  `productUiThemeReceiptName` / `productUiHitSurfaceReceiptName` / `uiHitKindReceiptName`,
  `creativeToolReceiptName`, `autoBakedRoomRefreshFields` / `uiCommandBakedRoomRefreshFields` accessors.
- **276–531** — the 11 `record*` API functions (declared in `ReceiptBuilder.hpp`):
  `recordProductPhysicsMovementPlannerTickProof` (276, physics), then
  `recordProductCreativeUi{Projection,InputFrame,DownstreamClick,CommandFrame,BakedRoomRefresh}`,
  `recordProductCreative{BakedRoomAutoRefresh,DocumentRevisionFrame,BakedRoomFresh,ViewportPickFrame,WireframeFrame}` (all creative).
- **532–592** — `buildProductAppReceipt` orchestrator (leave here, or move to a `ReceiptOrchestrator.cpp`).

**Target:** move the record path into `src/app/iggy3d/receipt/` — e.g. `CreativeReceiptRecording.cpp`
(the 10 creative `record*` + their `copy*`/`reset*`/`*ReceiptName` helpers) and
`PhysicsReceiptRecording.cpp` (`recordProductPhysicsMovementPlannerTickProof` +
`setPhysicsMovementPlannerProof`). The `record*` declarations stay in `ReceiptBuilder.hpp` (their public
API surface is fine); only the *definitions* move.

**The one hazard:** those anon-namespace helpers (`copy*`, `reset*`, `*ReceiptName`, the accessors) have
**internal linkage** and are shared among several `record*` functions. Keep each helper in the SAME new TU
as all its callers, OR promote it to external linkage in a shared header (like `floatReceiptValue` was).
Before moving, grep each helper's call sites (`grep -n '\bsymbol\b' ReceiptBuilder.cpp`) and confirm they
all land in the same target TU. If a helper is called from two target TUs, promote it, don't duplicate it
(ODR).

After this, `ReceiptBuilder.cpp` should be ~60 lines (just the orchestrator) or deleted in favour of
`ReceiptOrchestrator.cpp`.

---

## 5. Slice 4 — carve the `ProductAppWindowState` struct (follow-on, bigger, higher-leverage)

`ProductAppWindowState.hpp` is still one ~425-line header (~212 members) that EVERY includer recompiles
when any domain's state changes — the biggest remaining include-radius amplifier. Slice 1 already grouped
~54 members into typed sub-structs (e.g. `window.gameplayMovement.*`, `window.startup.*`).

**Strategy options (pick per the growth goal, this is a design call — recommend confirming with the user):**
- **(a) Sub-header split, one struct.** Keep `ProductAppWindowState` as one struct but move each typed
  sub-struct's *definition* into its own domain header (`gameplay/GameplayWindowState.hpp`, etc.) that
  `ProductAppWindowState.hpp` includes. Low-risk, but includers still recompile on any change (the outer
  struct is one TU-visible unit) — limited win.
- **(b) True split into independent structs** the app threads around separately. Highest include-radius win
  but a heavy, invasive change (every `window.foo.bar` access site and every function signature that takes
  `ProductAppWindowState&` is affected). This is the real refactor; scope it as its own multi-slice effort
  with the recon-workflow treatment.

**Hazard:** the string-mirror coupling (`ProductAppWindowState` is the successor to the 421/643-field
`ProductAppWindowState`/`ProductGameplayProjectionFrame` god-structs). Any split must keep the mirror
tests green. Do a call-graph murder-board (see `docs/saveload_murder_board.md` template) before cutting.

---

## 6. Boundaries & commit hygiene

- The Mac tree is **shared**. Commit only your own hunks; never stage another lane's in-progress
  `creative/*` / Blockout work. `git add` the exact files you changed, not `git add -A`.
- Codex commits as `codex: <slice>`; keep the "one slice = one green commit" discipline.
- Do **not** push (that is the user's call). Work on `iggy3d-main`.
- `Testing/Temporary/LastTest.log` is a build artifact — never stage it.

---

## 7. Hazards catalogue (learned this session — read before cutting)

- **Anon-namespace linkage.** File-local (`namespace {}`) helpers are invisible cross-TU. Any helper an
  extracted TU calls must be either moved with it or promoted to external linkage via a shared header.
  This is the #1 cause of link errors in this work.
- **Multiline `appendReceiptField`.** The key is often on the line after `receipt,`. A line-based `grep`
  undercounts; use a multiline-aware regex (Python `\s` crosses newlines) for the golden oracle.
- **Mid-body local decls.** A boundary must never fall between a `const bool x = ...;` and a later use.
  `windowFailed` (used by the last two fields) is why the orchestrator tail stayed inline in slice 3.
- **`.hpp`/`.cpp` partner pairing.** When re-pointing includes, a header re-point can strand its `.cpp`
  partner that uses the API; and tests that got a symbol *transitively* through a re-pointed header need a
  direct include added (slice 2 hit this twice).
- **Perl/sed rename over-match.** Dot-anchor renames (`s/\.prefix([A-Z])/.prefix.\l$1/`) and always include
  the `tools/` tree in scope; another struct sharing a prefix will be silently corrupted otherwise — verify
  by object name, and let the compiler + full suite flush stragglers.
- **Line-shift.** Cut bottom-up, or do one atomic pass against the original line array.
- **The suite is the real oracle.** 255 tests pin receipt contents heavily. Trust `ctest`, but pre-check
  with the static key-order diff to fail fast before an 87s test cycle.
