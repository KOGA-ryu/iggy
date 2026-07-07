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

## Completion Brief

- Site inventory (file -> role -> lines):
  - `src/content/authoring/EditableRoomDocument.cpp`
    - Validator: `validSemanticTag()` accepts traversal tags at lines 45-50.
    - Authoring-to-runtime surface tag merge: `traversalTagsWithStructuralTag(...)` preserves non-structural tags while stripping `walkable`/`blocker`/`projectile_blocker` at lines 222-232.
    - Emits runtime `walkable`, `blocker`, and `projectile_blocker` surface tags at lines 275-323.
    - Consumes `clamber` to add top walkable surfaces/runtime ids at lines 351-353 and 581-582.
  - `src/content/assets/RoomAsset.cpp`
    - Parses spatial-surface roles from `walkable`, `blocker`, `projectile_blocker`, `opening` at lines 223-244.
    - Validator: `validTraversalTag()` accepts all listed tags except `clamber_candidate` at lines 273-276.
    - Room asset validation requires role-compatible tags at lines 547-579.
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
    - Emits Creative bake `walkable`, `blocker`, `projectile_blocker` traversal tags at lines 364-418.
  - `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`
    - Serializes surface shape/role names including `opening`, `walkable`, `blocker`, `projectile_blocker` at lines 99-122.
    - Parser-compatible traversal filter accepts the RoomAsset set, excluding `clamber_candidate`, at lines 125-129.
    - Adds role-derived traversal tags on export at lines 140-159.
  - `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
    - Emits/propagates `blocker` from authored room semantics at lines 70-78.
    - Consumes `clamber` to choose `ledge` mesh role at line 119.
    - Emits wall/object/door surface tags: `projectile_blocker` lines 210-217 and 289-298; `blocker` lines 239-255 and 325-332; `walkable`+`clamber` lines 263-282.
    - Consumes `clamber` to add object walkable tops at lines 477-482.
  - `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
    - Emits authored traversal tags: floor `walkable` at line 48; wall `clamber_candidate` at line 75; object `object`/`prop`/`ledge`/`clamber` at lines 92-94; object `object`/`prop`/`crate` at lines 95-97.
    - Also emits terrain/gameplay-like strings into traversal tags (`blocked_slope`, `ramp`, `elevated_floor`, `terrain_*`) at lines 50-65. These are outside the prompted vocabulary and outside both closed validators.
  - `src/runtime/movement/MovementTraversalSlots.cpp`
    - Tag->slot parse table maps `vault` -> `Vault`, `clamber` -> `Clamber`, `wire_walk` -> `WireWalk` at lines 27-40.
    - Consumes `clamber` for clamber top/blocker surface discovery at lines 277-305.
    - Scans room surface traversal tags into movement affordances at lines 345-352.
    - Legacy name fallback for rail/id `vault` and `wire` at lines 356-373.
    - Slot kind -> string table returns `vault`, `clamber`, `wire_walk` at lines 531-540.
  - `src/runtime/movement/MovementTraversal.cpp`
    - Traversal mechanic -> string table returns `vault`, `clamber`, `wire_walk` at lines 952-961.
  - `src/runtime/collision/CollisionQuery.cpp`
    - Collision surface role/shape names use `walkable`, `blocker`, `projectile_blocker`, `opening` at lines 199-222.
  - `src/runtime/player/PlayerMotor.cpp`
    - Player motor phase string table returns `wire_walk` at lines 353-362.
  - `src/runtime/ai/ReasoningGraph.cpp`
    - Reasoning edge string table returns `walkable` for `ReasoningEdgeKind::walkable` at lines 123-126. This is semantically adjacent, not a RoomAsset traversal tag validator/consumer.
  - `src/runtime/save/SaveCodec.cpp`
    - Save keys include `walkable` bool fields at lines 885 and 1547. These are authored-room semantics fields, not traversal tag values.
  - `src/projection/debug/DebugProjection.cpp`
    - Debug strings include motor phase `wire_walk` at lines 142-149 and HUD words `walkable`/`blocked` at lines 224-228. These are display/debug strings, not traversal tag contract owners.
  - `src/render/vulkan/BufferImageResources.cpp`
    - Render room role color handles role `opening` at lines 168-175. This is a RoomStaticMesh role, not a traversal tag.
  - Current grep false positives from the suggested regex:
    - `src/app/iggy3d/receipt/WorldAuthoringFields.cpp` and `src/app/iggy3d/receipt/ActiveRoomFields.cpp` contain receipt keys with `projectile_blocker` substrings, not traversal tag values.
- Validator sets + exact drift diff:
  - `EditableRoomDocument.cpp::validSemanticTag()` accepts:
    - `walkable`, `blocker`, `projectile_blocker`, `opening`, `clamber`, `clamber_candidate`, `vault`, `wire_walk`, `no_player`, `debug_only`.
  - `RoomAsset.cpp::validTraversalTag()` accepts:
    - `walkable`, `blocker`, `projectile_blocker`, `opening`, `clamber`, `vault`, `wire_walk`, `no_player`, `debug_only`.
  - `AsciiRoomAssetText.cpp::parserCompatibleTraversalTag()` accepts the same set as `RoomAsset.cpp`, also excluding `clamber_candidate`.
  - Exact confirmed drift:
    - `clamber_candidate` is accepted by authored editable-room validation and emitted by ASCII authored wall semantics, but is rejected/dropped by RoomAsset parser/export compatibility.
  - Additional unprompted drift to resolve:
    - ASCII authored-room code puts non-closed semantic/gameplay strings in `traversalTags`: `object`, `prop`, `ledge`, `crate`, `floor`, `blocked_slope`, `ramp`, `elevated_floor`, and `terrain_*`.
    - Some of these may belong in `gameplayTags` only, or a separate material/terrain catalog, not the traversal tag catalog.
- Parse-table mappings:
  - RoomAsset spatial surface role parse:
    - `walkable` -> `RoomSpatialSurfaceRole::Walkable`
    - `blocker` -> `RoomSpatialSurfaceRole::Blocker`
    - `projectile_blocker` -> `RoomSpatialSurfaceRole::ProjectileBlocker`
    - `opening` -> `RoomSpatialSurfaceRole::Opening`
  - Movement traversal slot parse:
    - `vault` -> `MovementTraversalSlotKind::Vault`
    - `clamber` -> `MovementTraversalSlotKind::Clamber`
    - `wire_walk` -> `MovementTraversalSlotKind::WireWalk`
  - Movement/player/debug names:
    - `TraversalMechanic::{Vault,Clamber,WireWalk}` -> `vault`/`clamber`/`wire_walk`.
    - `MovementTraversalSlotKind::{Vault,Clamber,WireWalk}` -> same.
    - `PlayerMotorPhase::WireWalk` -> `wire_walk`.
  - Collision role names mirror RoomAsset roles: `walkable`, `blocker`, `projectile_blocker`, `opening`.
- Proposed catalog shape + home:
  - Home: `src/content/assets/TraversalTag.hpp` plus `.cpp` if local style wants an out-of-line table.
    - Rationale: traversal tags are part of the `RoomAsset` content contract consumed by Creative bake, product ASCII authoring, runtime movement/collision, and debug/projection code. `content/assets` is already reachable from those lanes through `RoomAsset.hpp` without importing product-app or creative internals into runtime.
    - Ownership: shared/broker contract per `docs/ownership_map.md`; Codex owns Creative/RoomBake emitters, runtime/simulation owns movement consumers, and content asset validation is the shared seam.
  - API shape:
    - `enum class TraversalTag { Walkable, Blocker, ProjectileBlocker, Opening, Clamber, ClamberCandidate, Vault, WireWalk, NoPlayer, DebugOnly };` only after owner review resolves `ClamberCandidate`.
    - `std::string_view traversalTagId(TraversalTag) noexcept;`
    - `bool parseTraversalTag(std::string_view, TraversalTag&) noexcept;`
    - `bool validTraversalTag(std::string_view) noexcept;`
    - `std::span<const TraversalTag> allTraversalTags() noexcept;`
    - Convenience constants if preferred locally, e.g. `kTraversalTagWalkable = "walkable"`.
    - Optional category helpers:
      - `bool traversalTagIsStructuralRole(TraversalTag)` for `walkable`/`blocker`/`projectile_blocker`/`opening`.
      - `bool traversalTagIsMovementAffordance(TraversalTag)` for `clamber`/`vault`/`wire_walk` and possibly `clamber_candidate` if retained.
  - Test shape:
    - New `traversal_tag_catalog_tests` pins totality: every enum value stringifies, parses, and is included in `allTraversalTags`.
    - Validator parity tests prove EditableRoomDocument, RoomAsset, and AsciiRoomAssetText consume the same catalog.
- Flagged semantic questions (owner decisions needed):
  - Is `clamber_candidate` a durable RoomAsset traversal tag?
    - If yes: add it to RoomAsset/parser-compatible validation and document that it is an authoring hint accepted in asset text even if runtime movement does not create slots from it.
    - If no: remove it from EditableRoomDocument/ASCII authored wall traversal tags or move it to `gameplayTags`/authoring metadata so it cannot reach RoomAsset traversal validation.
  - Are `object`, `prop`, `ledge`, `crate`, `floor`, `blocked_slope`, `ramp`, `elevated_floor`, and `terrain_*` valid traversal tags, or should they be gameplay/material/terrain tags only?
    - Current evidence suggests they are not in the closed traversal vocabulary but are currently emitted in traversal tag vectors by ASCII authoring code.
  - Should `no_player` and `debug_only` remain in the RoomAsset traversal tag catalog if no current runtime consumer was found in this pass?
  - Should collision/display role names use the same catalog constants where names overlap, or stay separate because they stringify enums rather than tag vectors?
- Drafted implementation card(s):
  - Draft E126: `TraversalTag Catalog Contract + Validator Parity`
    - Scope:
      - Add `src/content/assets/TraversalTag.hpp` and `.cpp` if needed.
      - Add `tests/unit/traversal_tag_catalog_tests.cpp`; register in `cmake/iggy3d_tests.cmake`.
      - Migrate the three closed validators only:
        - `EditableRoomDocument.cpp::validSemanticTag`
        - `RoomAsset.cpp::validTraversalTag`
        - `AsciiRoomAssetText.cpp::parserCompatibleTraversalTag`
      - Do not migrate emitters/consumers yet.
    - Required owner decision before implementation:
      - Pin whether `clamber_candidate` is retained in the catalog or removed/moved out of traversal tags.
    - Acceptance:
      - Validator parity test proves all three validators accept exactly the same catalog set.
      - Existing `editable_room_document_tests`, `room_asset_loader_tests`, `creative_document_room_bake_tests`, and ASCII room tests that touch asset text remain green.
      - If `clamber_candidate` is retained, add a RoomAsset parse/validation test that an authored surface tag with `clamber_candidate` no longer fails.
      - If removed, add an ASCII authored wall test proving wall semantics no longer emits `clamber_candidate` as a traversal tag.
  - Draft E127: `TraversalTag Catalog Migration For Emitters And Consumers`
    - Scope:
      - Replace hand-written string literals with catalog constants/helpers in:
        - `RoomBake.cpp`
        - `EditableRoomDocument.cpp` surface emitters/role checks
        - `AsciiRoomAssetText.cpp`
        - `AsciiRoomToRoomAsset.cpp`
        - `AsciiRoomToAuthoredRoom.cpp`
        - `MovementTraversalSlots.cpp`
        - `MovementTraversal.cpp`
        - `CollisionQuery.cpp`
        - optionally display-only/debug files if owners agree.
      - Keep policy local: RoomBake still decides which surfaces are walkable/blockers; movement still decides which tags become traversal slots.
    - Acceptance:
      - No receipt/render/save semantics changes except intentional validator resolution.
      - Movement slot tests prove `vault`, `clamber`, `wire_walk` behavior unchanged.
      - RoomBake and RoomAsset tests prove emitted tags validate against the catalog.
      - ASCII authored room tests prove no non-catalog traversal tags leak into RoomAsset unless owner-approved.
- Concerns/deferred:
  - The prompted regex returns 80 current non-test hits, but includes substring false positives in receipt field names and render/static role strings. The implementation card should use catalog migration by semantic site, not blind grep replacement.
  - `RoomAsset` and `EditableRoomDocument` drift is live and small; the broader ASCII traversal/gameplay tag mixing is a separate policy issue that should be resolved before a mechanical all-site migration.
  - No source/header/CMake edits were made in this read-only card.
  - Suggested verification was run:
    - `grep -rnE '"(walkable|blocker|projectile_blocker|opening|clamber|clamber_candidate|vault|wire_walk|no_player|debug_only)"' --include='*.cpp' --include='*.hpp' /Users/kogaryu/iggy3d/src | grep -vi test | wc -l` -> `80`
    - `grep -rn 'validSemanticTag\|validTraversalTag' --include='*.cpp' /Users/kogaryu/iggy3d/src`
