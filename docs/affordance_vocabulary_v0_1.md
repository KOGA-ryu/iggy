# Affordance Vocabulary v0.1 — the cross-lane contract (A7)

> The ONE vocabulary connecting authored maps to the AI's reasoning graph and the future
> encounter deck. Two agents build against this table: **Codex** (map generation / ASCII
> authoring lane) authors the affordances; **Claude's fleet** (AI lane) consumes them.
> Planner-brokered per `docs/ai-lane-maximum.md` (map v1.5, stream A7).
>
> (This contract supersedes map v1.5's A7 header/ownership language — ratified in map v1.6.)
>
> **Wire format law:** the contract travels as **anchor-kind STRINGS** on `RoomAnchorAsset.kind`
> (already a string). No shared header is required across the lanes — the authoring side emits
> strings; the AI side maps strings → `ReasoningNodeKind` (runtime/ai). **Unknown anchor kinds
> must never ERROR anywhere** — ignore-and-continue is the law (verified: already true end to
> end). HONEST SCOPE of "harmless": no reasoning node and no error, BUT anchor-synthesized
> sessions seed a generic marker ENTITY per anchor (`entityFromAnchor` fallthrough) — so maps
> that AUTHOR new kinds will shift their entity counts/receipts/hash; maps that don't are
> byte-identical. A consumable kind nobody authors is fully dormant. **Ordering: the five NEW
> kinds land in any order. The `monster` row is the ONE ordering-sensitive change** — it
> RE-KEYS live output (see its note).

## The table

| Concept | Anchor kind (wire string) | ReasoningNodeKind (AI side) | Deck semantics (A8b) | Status |
|---|---|---|---|---|
| Player start | `spawn` (existing, `P`) | reference | player_start region | LIVE both sides |
| NPC spawn | `npc` (existing, `N`) | reference | enemy spawn slot | LIVE both sides |
| **Monster spawn** | **`monster` (NEW — `M` de-collapses from npc)** | reference | enemy spawn slot (monster flavour) | A7 both halves |
| Exit | `exit` (existing, `E`) | exit | escape route | LIVE both sides |
| Objective item | `treasure`/`key` (existing); `pickup` consume-only (no authoring emitter yet) | objective | objective socket | LIVE both sides |
| **Chokepoint** | **`chokepoint` (NEW)** | chokepoint | narrow route; Trapper `requires` | A7 both halves |
| **High ground** | **`high_ground` (NEW)** | highGround | archer/roof slot | A7 both halves |
| **Hiding spot** | **`hiding_spot` (NEW)** | hidingSpot | search target; stealth affordance | A7 both halves |
| **Cover** | **`cover` (NEW)** | coverCluster | cover cluster | A7 both halves |
| **Patrol post** | **`patrol_post` (NEW)** | patrolPost | patrol anchoring | A7 both halves (see note 2) |
| Trap slot | `trap` (existing, `T` — currently inert) | — (no node in v0.1) | trap_slot (Trapper placement) | authoring LIVE; deck consumes in A8b |
| Hazard socket | `reset_zone` (existing, `R` — **has LIVE reset-to-spawn gameplay**) | — (no node in v0.1) | hazard socket (future) | authoring+runtime LIVE; deck later |

Notes:
1. **Stream-4 resolution (corrected):** `T` (trap) is genuinely inert — documented as a
   placement affordance, visible to the deck layer, runtime behavior a future gameplay slice.
   `R` (reset_zone) is **NOT inert**: it already drives live reset-to-spawn gameplay
   (`Controller.cpp` `findResetZoneAt` → `resetProductPlayerToSpawn`, test-pinned) — only its
   deck/hazard-socket semantics are future. The authoring reference carries STALE
   "no reset behavior" lines that the Codex order corrects. `?`/`marker` stays a plain
   annotation. `M` stops collapsing into `npc` (its own wire string so encounter generation
   distinguishes monster from guard; entity seeding treats both identically for now).
   **ORDERING (monster row only):** `M`→`monster` re-keys live output — `warden_vault`
   authors 5 M glyphs that would silently demote to inert markers if authoring lands alone.
   The AI half (a7s1) lands FIRST or in the same window; zero suite consumers of M today, so
   the exposure is product-behavioral, not gate-visible — which is exactly why it's ruled.
2. `patrol_post` v0.1 compiles to graph patrolPost nodes (search/scoring targets) ONLY.
   Deriving actual patrol ROUTES from authored posts is a future slice (routes today come
   from scenario TOML waypoints).
3. Glyph choices for the NEW kinds belong to the authoring lane (Codex) — glyph space is his;
   marker-with-tag syntax is equally acceptable. The wire strings above are FIXED.
4. New kinds append to this table by planner-brokered v-bump only; wire strings are
   append-only, never renamed.

## The two halves (who builds what)

- **AI lane (Claude fleet — slice a7s1):** `markerToReasoningNode` string→kind mapping in
  `runtime/ai` (buildReasoningGraph consumes the five NEW kinds + monster); `PackageSessionSeed`
  treats `monster` anchors exactly like `npc` for entity seeding (world/ shared ground per map
  v1.5); tests with hand-authored anchors; graph readout shows authored nodes.
- **Authoring lane (Codex):** ASCII glyph table / marker syntax additions emitting the five NEW
  wire strings; `M` → `monster` (ordering note above); compile through `AsciiRoomToRoomAsset`;
  `docs/ascii_dungeon_authoring_reference.md` **§1 glyph table + §4 marker semantics** updated,
  and the stale T/R inertness notes in §4/§8 corrected. Seed order:
  `docs/creative_mode/codex_order_affordance_authoring.md`.
- **Deferred (stated so it can't silently drop):** creative-lane GameplayMarker emission of
  these wire strings is a LATER Codex order, not part of v0.1.
