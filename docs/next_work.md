# iggy3d — Next Work / Onboarding

> Start-here map for the next working session. Written 2026-06-30 at a good stopping point: suite green, coupling decoupling underway, ASCII authoring surface fully scouted. Each stream below is self-contained — pick one and go. Companion docs: [`ascii_dungeon_authoring_reference.md`](ascii_dungeon_authoring_reference.md), [`multiroom_connectivity_design.md`](multiroom_connectivity_design.md).

## Current state (what's true right now)

- **Test suite: 171/171 green** on branch `iggy3d-main` (pushed). Red means something again — keep it that way. A red baseline is the root enabler of everything going wrong.
- **The vision**: the ASCII room grid is the **AI dungeon-authoring surface** — AI drafts 80% (generate ASCII → compile → verify via receipts), human does the vital 20% (taste/direction). Proven end-to-end today: a 24×24 dungeon (`fixtures/rooms/ascii/warden_vault.iggyroom.txt`) authored, compiled clean, booted live, 26 markers → 26 entities + 11 objectives, all deterministic.
- **Discipline that must hold**: everything is deterministic + receipt-proven. Build + full suite gate every change. Prefer output-preserving refactors verified by byte-identical receipts. The engine's determinism + honest receipts are the crown jewels — they're what let an AI author *and self-verify* worlds.

## Quick reference (commands)

```bash
# build lib + app + tests
make -C build -j8                       # everything
make -C build -j8 iggy3d iggy3d_app     # just lib + product app

# tests (run from repo root; fixtures resolve relative to it)
ctest --test-dir build                  # full suite (the gate)
ctest --test-dir build -R <name>        # one test
ctest --test-dir build -L <label>       # by label (save|frontend|menu|product|movement|smoke|unit...)

# author + verify an ASCII room (the closed loop, one command)
tools/ascii_room.py <grid-file>                    # preview compile + receipt
tools/ascii_room.py <grid-file> --activate         # boot a live session + entity receipt
```

---

## Stream 1 — Coupling: decouple the `ProductAppWindowState` string-mirror

**Why**: `ProductAppWindowState` (in `src/app/iggy3d/ReceiptBuilder.hpp`) is a ~421-field god-struct (~208 `std::string`) whose flat fields are a hand-copied mirror of typed result structs, read back by `ReceiptBuilder.cpp` (~1568 lines) to emit the `key=value` receipt. It's the repo's central coupling: a change to one subsystem ripples through it. Keeping the codebase AI-legible/modifiable is existential for the whole AI-authoring bet — this is the 15-year insurance, not cosmetics.

**Pattern (proven, output-preserving)**: find a cluster of flat `window.<field>` that mirrors an existing typed result struct → store the struct on the window (`ProductXResult x;`) → producer does `window.x = result;` (keep any normalization, e.g. empty→"none") → `ReceiptBuilder` reads `window.x.<member>` → delete the flat fields. **The observable contract is the receipt TEXT**, which tests assert — keep every emitted key byte-identical; struct defaults must match the old flat-field defaults. **The full suite is the gate** (a green run proves structure changed, behavior didn't).

**Done so far** (commits on `iggy3d-main`): `savedMarkerBind` (19→1, the pristine case), `productSaveLoad` (12→1, partial cluster).

**Per-cluster HAZARDS (learned the hard way — check every one before deleting fields):**
1. **Multiple writers** — a field is often written by the main `record*Result` helper AND by error/edge paths elsewhere. `grep -rn "window\.<field>"` for ALL writers, **no `| head`**. The build gate catches misses.
2. **Circular include** — the cluster's result-struct header may `#include "ReceiptBuilder.hpp"` (its function takes `ProductAppWindowState&`), e.g. `ascii_room/Activation.hpp`. You can't add its struct as a window member without breaking the cycle first (forward-declare `ProductAppWindowState` in that header → ~6-file ripple, or extract the result type to its own header).
3. **Type mismatch** — struct counts may be `std::size_t` while flat fields were `std::uint64_t`; `appendReceiptField` has `uint64_t`+`bool` overloads but NO `size_t` → reading `size_t` directly is ambiguous. Use `static_cast<std::uint64_t>(...)` at the read.
4. **Direct test readers** — some tests read `window.<flatField>` directly (e.g. `product_ascii_room_activation_tests`) → update them to the struct member.
5. **Partial / mis-grouped clusters** — not every prefix-sharing field belongs to the result (e.g. `productSaveLoadSource/SelectedId` are selection state, not load result). Keep the odd ones flat.

**Next candidates** (from the analysis workflow): `roomEditorPreview` (already has `ProductRoomEditorPlacementPreviewResult`), `asciiRoomActivation` (has hazards 2+3+4 — the meatier cut). Re-run the mapping if needed: the 46-cluster analysis lived in a workflow; regenerate by scouting `ProductAppWindowState` field clusters.

**Done-criteria**: string-mirror clusters replaced by typed members, receipt byte-identical, suite green. Each cut = one small commit.

---

## Stream 2 — The closed AI-authoring loop (Horizon 1)

**Why**: this is the atom the whole vision is made of — AI generates a room, plays it headless, reads receipts, self-corrects. Single-room authoring already works (`tools/ascii_room.py`). The missing half is **AI playtests its own room**.

**Where / how to start**:
- Authoring surface + rules: [`ascii_dungeon_authoring_reference.md`](ascii_dungeon_authoring_reference.md) (glyphs, coords, elevation, input format, gotchas). Read it before authoring.
- Tool: `tools/ascii_room.py <grid> [--activate]` — validate + compile/activate + receipt.
- **Next build**: a headless *playthrough* harness. After `--activate`, drive `game.player_position=<x,y,z>` + `game.move_x/move_y` + `game.interact` (in the same automation tape) to walk the intended loop, and read gameplay receipts (movement blocked/moved, `active_room_collision_*`, objective completion, `SessionOutcome`) to assert **reachability + winnability + balance**. Then the AI can generate → playtest → refine in a loop.
- **Gotcha to respect**: automation `game.move_x=1` is a ~0.055m velocity frame, NOT a tile jump — seed position with `game.player_position` then move (see reference §10). This is the same issue behind the ramp-slope smoke saga.

**Done-criteria**: a script/agent that, given a grid, boots it, walks the loop, and returns pass/fail on reachability + objective completion. That closes Horizon 1.

---

## Stream 3 — Multi-room connectivity (the big vision gap)

**Why**: today it's ROOMS, not a dungeon — no room-to-room links exist anywhere, and only `package.rooms.front()` runs. "AI generates a dungeon" currently = one room. This is the #1 gap for the dungeon vision. **Design-heavy** — see the full design stub: [`multiroom_connectivity_design.md`](multiroom_connectivity_design.md).

**Done-criteria**: a dungeon = a graph of rooms + portals; the runtime loads and transitions between rooms; `E`/`+` (or a new glyph) can target another room. This is a real feature build, not a refactor.

---

## Stream 4 — Wire (or document) the inert markers

**Why**: `T` (trap), `R` (reset zone), `?` (inspect) have tags but NO wired runtime behavior; `M` (monster) is identical to `N` (npc). An AI told to "add traps and monsters" produces decorative glyphs. Product decision needed: wire real behavior (trap damage, reset/respawn, distinct monster) or document them as decorative so authoring doesn't over-promise.

**Where**: marker→gameplay binding is in `src/app/iggy3d/world/PackageSessionSeed.cpp` (anchor kind → entity seed); glyph→tag in `src/app/iggy3d/ascii_room/AsciiRoomGrid.cpp`. Coverage gap noted in the reference §8.

---

## Stream 5 — Fix the ramp-tile-center slope seam

**Why**: a real latent physics bug. At (near) the exact center-X of a `RampEast` `>` tile, a ground sample reads the flat/contour band instead of the slope (plane height at ramp center == mid elevation → `|dy|<=0.001` → `Contour`/airborne). Currently only worked around by seeding position deep into the tile.

**Where**: `src/runtime/collision/CollisionQuery.cpp` `sampleSurfaceHeightAtOrBelow` (~259) + slope sampling in `src/runtime/movement/MovementPolicy.cpp`. Fix likely biases the sample toward the slope surface, or fixes the travel-direction epsilon. **Guardrail**: `ctest -L movement` + the player-physics tests + the ascii smokes (see reference §8 for the proven set). This touches the character-controller — verify carefully.

---

## Notes for the next session

- Untracked `docs/plan_bucket/physics_*.md` are NOT ours — do not commit them.
- Commit prefix `claude:`; end commit messages with `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`. Commit/push only when asked.
- Memory (`~/.claude/.../memory/`) has the durable context: `ascii-dungeon-authoring`, `save-delete-consolidation`, `iggy3d-build-test`, `iggy3d-ascii-editor-smokes-broken`.
