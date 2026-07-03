# Codex work order — affordance authoring (A7, authoring-lane half)

> From the Mac planner, 2026-07-02. Contract: `docs/affordance_vocabulary_v0_1.md` (read it
> first — the wire strings are FIXED there; everything else below is yours to design).
> Scope: the ASCII map-generation surface. Your lane: `src/app/iggy3d/ascii_room/**` + the
> authoring reference doc. Do NOT touch `src/runtime/**` (the AI side consumes your strings
> independently — the halves are decoupled by design; land in any order).

## The job
Make the five NEW affordance kinds authorable from the ASCII surface, and de-collapse `M`:

1. **`M` emits anchor kind `monster`** (today `anchorKindForMarkerTag` collapses
   monster_spawn → "npc" in `AsciiRoomToRoomAsset.cpp` — give it its own wire string).
   **ORDERING NOTE (the one sequencing-sensitive item):** this re-keys live output —
   `warden_vault.iggyroom.txt` authors 5 `M` glyphs whose entities silently demote to inert
   markers until the AI-lane consumption half lands. The planner is sequencing that half
   (a7s1) immediately; land this item after confirming with the planner that a7s1 is done,
   or in the same window. Items 2–3 have no ordering constraint.
2. **New authorable kinds** emitting these exact wire strings:
   `chokepoint`, `high_ground`, `hiding_spot`, `cover`, `patrol_post`.
   Glyph choices (or marker-with-tag syntax — your call; glyph space is yours) — pick what
   fits the 26-glyph grammar and the AsciiRoomCanvas direction.
3. **Reference doc updated — correct sections, and fix the stale claims:** the glyph table is
   **§1** and marker→gameplay semantics is **§4** (not §8, which is the proven/unproven list).
   Document the new affordances there, and AMEND the stale inertness notes (§4 and the §8
   items around lines ~264/289): per the Stream-4 resolution, `T` is an inert placement
   affordance (behavior later); `R` is **NOT inert** — it already has live reset-to-spawn
   gameplay (`Controller.cpp`, test-pinned); the doc's current "no reset behavior" lines are
   WRONG today and must be corrected, not restated.

## Constraints
- Wire strings EXACTLY as the contract table — append-only, never renamed.
- Unknown-kind tolerance stays intact end to end (no consumer may error on kinds it doesn't
  know — verify the compile path doesn't reject unrecognized tags).
- Deterministic compile (same grid → same anchors, stable order) — the existing discipline.
- Gate: full ctest suite green; the ascii smokes + authoring tests updated deliberately where
  they pin glyph tables.

## Done
Every kind in the contract's "A7 both halves" rows is authorable from a grid file, compiles to
a `RoomAnchorAsset` with the exact wire string, and the reference doc (§1 table + §4
semantics, stale notes corrected) tells an AI author how to use them. Report which
glyphs/syntax you chose so the planner can log them in the contract (v0.2 bump).
