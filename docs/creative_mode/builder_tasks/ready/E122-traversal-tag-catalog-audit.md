# E122: TraversalTag Catalog Audit

## Objective

**Read-only.** Map the complete traversal-tag vocabulary surface across the repo
— every literal, every validator, every tag→slot parser, every baker that emits
tags, every consumer — and pin the exact drift. Then produce the exact
implementation card(s) for a single authoritative `TraversalTag` catalog.

This card writes NO source changes. It exists so the risky cross-lane
implementation (Creative bake ↔ runtime movement ↔ product) is scoped from an
exact map instead of a guess.

## Why This Exists

From `docs/complexity_audit_v0_1.md` (finding #3 / bucket 2, the one HIGH). The
traversal-tag vocabulary (`walkable / blocker / projectile_blocker / opening /
clamber / clamber_candidate / vault / wire_walk / no_player / debug_only`) has
**no owner**: ~100 literal occurrences across 13 non-test files, two independent
bakers, three tag→slot parse tables, and **two validators that have already
drifted** —

- `content/authoring/EditableRoomDocument.cpp:48` `validSemanticTag()` **accepts**
  `clamber_candidate`.
- `content/assets/RoomAsset.cpp` `validTraversalTag()` (≈:273) **does not list**
  `clamber_candidate` — it rejects it.

So a wall tagged `clamber_candidate` passes authoring validation and fails bake
validation. That drift is the confirmed live bug this catalog will retire.

## Required Work

1. **Inventory every tag literal** (non-test). Confirmed starting set of 13
   files — for each, record HOW tags are used (emit / validate / parse-to-slot /
   consume) and the line ranges:
   - `content/authoring/EditableRoomDocument.cpp`, `content/assets/RoomAsset.cpp`
   - `app/iggy3d/creative/adapters/RoomBake.cpp`
   - `app/iggy3d/ascii_room/{AsciiRoomAssetText,AsciiRoomToRoomAsset,AsciiRoomToAuthoredRoom}.cpp`
   - `runtime/movement/{MovementTraversal,MovementTraversalSlots}.cpp`
   - `runtime/ai/ReasoningGraph.cpp`, `runtime/collision/CollisionQuery.cpp`,
     `runtime/player/PlayerMotor.cpp`, `runtime/save/SaveCodec.cpp`
   - `projection/debug/DebugProjection.cpp`
   Reproduce with:
   `grep -rnE '"(walkable|blocker|projectile_blocker|opening|clamber|clamber_candidate|vault|wire_walk|no_player|debug_only)"' --include='*.cpp' --include='*.hpp' src | grep -vi test`
2. **Enumerate every validator** (the closed valid-set functions). Record the
   EXACT accepted set of each and the diff between them. At minimum:
   `EditableRoomDocument.cpp validSemanticTag`, `RoomAsset.cpp validTraversalTag`.
3. **Enumerate every tag→slot / tag→enum parse table** (the movement-side ones,
   plus any in collision/ai). Record the mapping each encodes.
4. **Determine the canonical set.** Which tags are current, which are legacy, and
   the open semantic questions — especially: is `clamber_candidate` a real tag
   that `RoomAsset` should accept, or a stale one `EditableRoomDocument` should
   drop? **Do not answer this unilaterally — flag it for the Creative/runtime
   owners with the evidence.**
5. **Specify the catalog shape.** Propose: where it lives (a shared header at the
   bake↔movement seam — evaluate `content/` vs a new `core`-adjacent home; it is
   a shared contract, so it must be reachable from creative, runtime, and
   product without a lane-crossing include hack), the closed enum + tag↔string
   table + one `validSemanticTag`. Each of the 13 sites should reference catalog
   constants; bakers keep their own occupancy→tag POLICY but stop hand-writing
   string literals. Model it on the credited `CreativeObjectDescriptor` /
   `ProductCreativeUiCommandKind` table patterns.
6. **Produce the implementation card(s).** Draft `E126` (and split into a second
   card by lane if the migration cannot land in one safe slice) as ready-to-run
   cards: exact per-file edits, the drift resolution, and a per-site
   acceptance. Place the drafts in this card's output; they enter `ready/` only
   after review. Tag cross-lane ownership per `docs/ownership_map.md` (this is a
   `shared`/broker contract).

## Acceptance Notes

Deliverable is a written map (a new doc, e.g.
`docs/creative_mode/traversal_tag_catalog_audit.md`, or a long section appended
to this card) containing: the full site inventory, the exact per-validator
accepted sets + the drift diff, the parse-table mappings, the proposed catalog
shape + home, the flagged semantic questions, and the drafted implementation
card(s). **No source file changes.**

## Do Not

- Do not change any source, header, or CMake file.
- Do not decide the fate of ambiguous tags (`clamber_candidate` etc.) yourself —
  flag them for owner review.
- Do not create the catalog in this card — only specify and draft it.
- Do not stage, commit, or push.

## Suggested Verification

```sh
# read-only; the "verification" is that the inventory is reproducible:
grep -rnE '"(walkable|blocker|projectile_blocker|opening|clamber|clamber_candidate|vault|wire_walk|no_player|debug_only)"' --include='*.cpp' --include='*.hpp' /Users/kogaryu/iggy3d/src | grep -vi test | wc -l
grep -rn 'validSemanticTag\|validTraversalTag' --include='*.cpp' /Users/kogaryu/iggy3d/src
```

## Completion Brief

Append:

- Site inventory (file → role → lines):
- Validator sets + exact drift diff:
- Parse-table mappings:
- Proposed catalog shape + home:
- Flagged semantic questions (owner decisions needed):
- Drafted implementation card(s):
- Concerns/deferred:
