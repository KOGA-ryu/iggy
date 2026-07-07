# E165 — Affordance anchor wire-strings (game seam §3 / fire-order step 3)

**STATUS: STAGED in `blocked/`.** Recon-verified vs HEAD (workflow `wdsgkuhia`). **Commit:** `claude: planned. codex: …`.
**Disjoint from the god-struct decomposition** (touches bake + AI, not `ProductAppWindowState`) — but still
**release only on a clean tree** (Codex is mid-decomposition).

---

## The seam, one line

The guard AI reader is **already live** — `nodeKindForAnchor` (`src/runtime/ai/ReasoningGraph.cpp:26-60`) turns
anchor-kind strings into reasoning nodes. Both bake emitters **drop** the designer's cover/patrol intel. This
card closes the buildable part: **`cover`, `patrol_post`, `monster`.**

## The contract (do not deviate — byte-exact)

Sole channel: `RoomAsset.anchors[].kind` (`src/content/assets/RoomAsset.hpp:28`, `std::string`) +
`.positionMeters` (`:30`). Read at `ReasoningGraph.cpp:154-158`. Strings the reader accepts:

| `.kind` string | → node | reader line |
|---|---|---|
| `cover` | coverCluster | `ReasoningGraph.cpp:51` |
| `patrol_post` | patrolPost | `:55` |
| `monster` | reference | `:35` |
| (`chokepoint`/`high_ground`/`hiding_spot` — reader live, see follow-up) | | `:39/:43/:47` |

**Any other string is silently dropped** (`nodeKindForAnchor` returns false at `:59`; node skipped at `:156`) —
no error, no fallback. A typo (`"Cover"`, `"patrolpost"`, `"navigation"`) = zero nodes. **A test with
hand-authored anchors is mandatory** because failures are silent.

## G1 — creative-doc bake: emit `cover` + `patrol_post`

Root cause: `CoverPoint`/`PatrolNode` descriptors carry `runtimeAnchorSemantic == None` (factory default,
`ObjectDescriptor.cpp:99-100`), so `descriptorSupportsRuntimeRoomAnchor` (`RoomBake.cpp:145-150`) returns false
and `classifyRoomBakeObject` (`:479-489`) hits `SkipUnsupportedAnchor` — the object never reaches `room.anchors`.

Fix (the existing emit path `RoomBake.cpp:286-289 → anchorForObject → BakeAnchor :704-710` then carries them):
1. Add `Cover` + `PatrolPost` to `CreativeRuntimeAnchorSemantic` (`ObjectDescriptor.hpp:133-143`, **append-only**).
2. Set the `CoverPoint` / `PatrolNode` descriptors' `runtimeAnchorSemantic` to the new values (in the descriptor factory).
3. Add `toString(CreativeRuntimeAnchorSemantic)` cases (`ObjectDescriptor.cpp:1854-1868`) returning **exactly**
   `"cover"` / `"patrol_post"`.

## G2 — ascii bake: `monster_spawn` → `"monster"` (not `"npc"`)

In `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp:344-345`, split the collapsed branch:
`if (tag == "npc_spawn" || tag == "monster_spawn") { return "npc"; }` →
`if (tag == "npc_spawn") return "npc"; if (tag == "monster_spawn") return "monster";`
**Downstream-safe** — `PackageSessionSeed.cpp:162` + `RoomBakeReachability.cpp:154` already alias `monster`==`npc`.

## The proving test (the handshake — the point of the whole card)

Author a room with a **cover point + a patrol post** → bake to `RoomAsset` → call `buildReasoningGraph` → assert
the graph contains a **`coverCluster`** node and a **`patrolPost`** node. Fails today, passes when G1 lands.
Extend the reasoning-graph or no-window-bake test harness (the vocab/fixture recon agent errored — **builder
picks the harness; confirm the CreativeObjectKind that == CoverPoint/PatrolNode while wiring the fixture**).

## Verified-safe (recon-confirmed)

- `SaveCodec.cpp` has **zero** `runtimeAnchorSemantic` refs — the enum ordinal is never serialized; `.kind`
  travels as a free string. **Appending enum variants is order-safe** — no receipt/key-order oracle shift.
- No compiled `.ascii` suite fixture authors an `M` glyph that would re-key on the monster change.

## Gates

Build green · full suite green (**+ the new proving test**) · receipt golden byte-identical (append-only enum,
no key shift) · then update the game plan seam §3 → CLOSED.

## Follow-up (separate card, NOT this one)

`chokepoint` / `high_ground` / `hiding_spot`: the reader is live, but **no ascii glyphs or creative object kinds
exist** for them (`AsciiRoomGrid` has P/N/M only; no descriptors carry those semantics). That's net-new
authoring surface — a scoped follow-up once cover/patrol prove the pipe.
