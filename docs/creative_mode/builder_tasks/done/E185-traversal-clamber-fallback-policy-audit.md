# E185 — Traversal Clamber Fallback Policy Audit

## Status

Ready. Read-only audit prompted by E184.

Commit prefix after review: `claude: planned. codex: ...`.

## Goal

Map the current clamber authored-tag policy before any implementation card tries
to tighten or simplify it.

E184 intentionally preserved behavior after discovering that the current runtime
accepts blocker-authored clamber with an untagged walkable top fallback. My E184
proof wording incorrectly implied both top and blocker surfaces must carry the
`clamber` tag. This audit should make the actual policy explicit and decide
whether the next card is a behavior-preserving documentation/test cleanup or a
deliberate behavior change.

## Scope

Audit only. Do not edit source, tests, CMake, receipt golden, or production
docs beyond this task card.

Inspect at minimum:

- `src/runtime/movement/MovementTraversalSlots.cpp`
- `src/runtime/movement/MovementTraversal.cpp`
- `tests/unit/movement_traversal_slots_tests.cpp`
- `tests/unit/movement_traversal_tests.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
- `src/app/iggy3d/world/MovementTestLab.cpp`

## Questions To Answer

Document the current behavior for each case:

- top surface tagged `clamber`, blocker surface tagged `clamber`
- top tagged `clamber`, blocker untagged
- top untagged, blocker tagged `clamber`
- neither top nor blocker tagged `clamber`
- `clamber_candidate` present but no durable `clamber` tag
- legacy mesh-id fallback paths involving `clamber`

For each case, report whether the current code:

- creates an authored affordance
- creates a clamber slot
- uses top fallback or blocker fallback
- records the affordance source surface as top, blocker, or legacy fallback
- is already pinned by a focused test

## Policy Decision Needed

Recommend one of these paths, with evidence:

1. **Preserve current behavior.** Treat a blocker-authored `clamber` tag as
   enough to authorize clamber, and allow walkable top fallback.
2. **Tighten behavior intentionally.** Require both the walkable top and actor
   blocker surfaces to carry durable `clamber`, with tests and migration notes.
3. **Split semantics.** Define one tag as the authored affordance source and
   another as the measured geometry helper, if current data implies that.

Do not implement the decision in this audit card. Draft the follow-up card in
the completion brief only.

## Explicit Non-Scope

Do not migrate or change:

- traversal tag catalog values
- movement slot construction
- legacy fallback behavior
- RoomAsset/ASCII serialization
- Creative emitters
- movement mechanic display names
- PlayerMotor phase names
- debug/projection display strings
- collision role strings
- `ProductAppWindowState`
- renderer/Vulkan/window code

## Required Commands

Run and report:

```sh
rg -n "clamber|clamber_candidate|legacy_name_fallback|kindForTraversalTag|findSurfaceForMesh|findActorBlockerForMesh" /Users/kogaryu/iggy3d/src/runtime/movement /Users/kogaryu/iggy3d/tests/unit/movement_traversal*_tests.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room /Users/kogaryu/iggy3d/src/app/iggy3d/world/MovementTestLab.cpp
git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required unless you add a focused guard test, which this
card does not ask for.

## Required Report

Completion brief must include:

- exact files inspected
- the case matrix from **Questions To Answer**
- tests that currently pin each case, and gaps
- recommendation among the three policy paths
- draft follow-up card title/scope
- confirmation that no source/test/CMake/receipt golden files were edited

## Completion Brief

- Files inspected:
  - `src/runtime/movement/MovementTraversalSlots.cpp`
  - `src/runtime/movement/MovementTraversal.cpp`
  - `tests/unit/movement_traversal_slots_tests.cpp`
  - `tests/unit/movement_traversal_tests.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
  - `src/app/iggy3d/world/MovementTestLab.cpp`

- Current policy evidence:
  - `kindForTraversalTag(...)` accepts only durable movement tags `vault`, `clamber`, and `wire_walk`; `clamber_candidate` is parsed as a catalog tag but intentionally returns no movement slot kind (`MovementTraversalSlots.cpp:30-55`).
  - authored affordance source is whichever tagged surface is encountered first for a mesh/kind while scanning `room.spatialSurfaces`, and the stored source id is that surface id (`MovementTraversalSlots.cpp:79-93`, `MovementTraversalSlots.cpp:360-367`).
  - clamber slot geometry prefers a `clamber`-tagged walkable top, but falls back to the first same-mesh walkable surface if none is tagged (`MovementTraversalSlots.cpp:198-214`, `MovementTraversalSlots.cpp:296-304`).
  - clamber front geometry prefers a `clamber`-tagged actor blocker, but falls back to the first same-mesh actor blocker if none is tagged (`MovementTraversalSlots.cpp:216-230`, `MovementTraversalSlots.cpp:312-318`).
  - no legacy clamber mesh-id fallback exists; legacy fallback is only for rail meshes whose ids contain `vault` or `wire` (`MovementTraversalSlots.cpp:371-388`).

- Case matrix:

| Case | Authored affordance? | Clamber slot? | Fallback behavior | Affordance source | Existing test coverage |
| --- | --- | --- | --- | --- | --- |
| top tagged `clamber`, blocker tagged `clamber` | Yes, from the first tagged surface in document surface order | Yes | No fallback when both preferred surfaces exist | usually top in current test ordering; source is not intrinsically top-only | Pinned by `registryBuildsMeasuredClamberSlot()` with top before blocker and source `tagged_top` (`movement_traversal_slots_tests.cpp:19-67`, `movement_traversal_slots_tests.cpp:135-156`). |
| top tagged `clamber`, blocker untagged | Yes | Yes if a same-mesh actor blocker exists | top is preferred; blocker uses actor-blocker fallback | top | Not directly pinned. This is a focused gap. |
| top untagged, blocker tagged `clamber` | Yes | Yes if a same-mesh walkable top exists | top uses walkable fallback; blocker is preferred | blocker | Pinned by runtime traversal tests: `clamberTopSurface()` is only `walkable`, `clamberBlockerSurface()` has `blocker, clamber`, and clamber applies/previews via `clamber_block:clamber_top` (`movement_traversal_tests.cpp:53-88`, `movement_traversal_tests.cpp:276-303`, `movement_traversal_tests.cpp:496-527`). |
| neither top nor blocker tagged `clamber` | No | No | no authored affordance, so no clamber slot build | none | Pinned by `clamberRequiresAuthoredSurfaceTags()` using `legacy_clamber_wall` with only `walkable`/`blocker` tags (`movement_traversal_slots_tests.cpp:71-80`, `movement_traversal_slots_tests.cpp:159-164`). |
| `clamber_candidate` present but no durable `clamber` tag | No | No | `clamber_candidate` is authoring hint only; no movement affordance | none | Pinned by `nonMovementCatalogTagsDoNotBuildAffordances()` from E184 (`movement_traversal_slots_tests.cpp:262-273`) and catalog expectations that `clamber_candidate` is not a movement slot tag. ASCII walls emit `clamber_candidate` by default (`AsciiRoomToAuthoredRoom.cpp:78-91`). |
| legacy mesh-id fallback paths involving `clamber` | No clamber fallback exists | No, from name alone | `legacy_name_fallback` only applies to rail `vault`/`wire` ids | none for clamber | Pinned by `legacy_clamber_wall` no-affordance/no-slot test (`movement_traversal_slots_tests.cpp:71-80`, `movement_traversal_slots_tests.cpp:159-164`) and by source code limiting legacy fallback to `vault` and `wire` (`MovementTraversalSlots.cpp:371-388`). |

- Authoring/emitter findings:
  - ASCII ordinary walls default to `clamber_candidate`, not durable `clamber` (`AsciiRoomToAuthoredRoom.cpp:78-91`).
  - ASCII `movement_clamber_ledge_proxy` objects author durable `clamber` in object semantics (`AsciiRoomToAuthoredRoom.cpp:94-109`).
  - RoomAsset conversion turns durable clamber objects into role `ledge`, emits a generated walkable top with `walkable` + `clamber`, and carries object tags including `clamber` onto the actor blocker (`AsciiRoomToRoomAsset.cpp:119-131`, `AsciiRoomToRoomAsset.cpp:248-297`, `AsciiRoomToRoomAsset.cpp:491-503`).
  - Movement test lab ledge objects include durable `clamber` as an object traversal tag, which then follows the object conversion path (`MovementTestLab.cpp:40-48`, `MovementTestLab.cpp:126-155`).

- Recommendation:
  - Choose path 1: preserve current behavior.
  - Rationale: current runtime and product-facing tests already depend on blocker-authored clamber with an untagged top fallback. The existing policy is coherent if `clamber` means "this mesh has a clamber affordance" and same-mesh top/blocker surfaces are measured helper geometry. Tightening both-surface requirements would be a behavior change and would require migration of existing authored/test data that currently uses one durable tag plus fallback geometry.
  - Do not choose path 2 in a cleanup slice. It already failed the focused `movement_traversal_tests` behavior gate during E184.
  - Do not choose path 3 yet. There is no second durable runtime tag today that cleanly distinguishes affordance source from measured helper geometry; `clamber_candidate` is an authoring hint, not a runtime movement slot tag.

- Draft follow-up card:
  - Title: `E186 — Traversal Clamber Fallback Policy Guard Tests`
  - Scope:
    - Behavior-preserving test/comment cleanup only.
    - Add explicit focused tests in `movement_traversal_slots_tests.cpp` for top-only durable `clamber` with blocker fallback and blocker-only durable `clamber` with top fallback.
    - Keep existing no-tag and `clamber_candidate` no-affordance tests.
    - Add a short implementation comment near `appendClamberSlot(...)` explaining that `clamber` authorizes the affordance on any same-mesh surface while top/blocker geometry can fall back to same-mesh helper surfaces.
    - Do not change `TraversalTag`, slot construction, legacy fallbacks, RoomAsset/ASCII serialization, movement mechanic names, or product/window state.
  - Acceptance:
    - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d movement_traversal_slots_tests movement_traversal_tests traversal_tag_catalog_tests -j10`
    - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(movement_traversal_slots_tests|movement_traversal_tests|traversal_tag_catalog_tests)$' --output-on-failure`
    - `git -C /Users/kogaryu/iggy3d diff --check`

- Commands run:
  - `rg -n "clamber|clamber_candidate|legacy_name_fallback|kindForTraversalTag|findSurfaceForMesh|findActorBlockerForMesh" /Users/kogaryu/iggy3d/src/runtime/movement /Users/kogaryu/iggy3d/tests/unit/movement_traversal*_tests.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room /Users/kogaryu/iggy3d/src/app/iggy3d/world/MovementTestLab.cpp`
  - `git -C /Users/kogaryu/iggy3d diff --check`

- Scope confirmation:
  - No source, test, CMake, receipt golden, production docs, traversal tag catalog values, movement slot construction, legacy fallback behavior, RoomAsset/ASCII serialization, Creative emitters, movement mechanic display names, `PlayerMotor` phase names, debug/projection display strings, collision role strings, `ProductAppWindowState`, renderer/Vulkan, staging, commit, push, or window launch changes were made.
