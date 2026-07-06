# E53: Algorithm Map Current-State Triage

## Objective

Audit `creative_editor_algorithms_map_v0_1.md` against the current codebase and
turn only verified, bounded items into either corrected notes or builder cards.

## Problem

The algorithm map is useful, but it mixes current-state evidence, roadmap
recommendations, and claims that may already be stale after E23-E35 work. For
example, it describes undo as a kernel snapshot ring even though current product
undo lives in `CreativeAppState`, and standalone undo currently has its own
header.

If builder treats the map as authoritative without triage, it can generate
duplicated or mis-scoped repair work.

## Required Reads

- `docs/creative_mode/creative_editor_algorithms_map_v0_1.md`
- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/ready/*.md`
- `src/app/iggy3d/creative/CreativeAppState.hpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `apps/iggy3d_creative/main.cpp`
- Current projection/picking/snap/mutation files as needed for cited claims.

## Scope

- Mark which map claims are current, stale, speculative, or already covered by a
  builder card.
- Correct stale current-state claims in the map or create a short companion
  review note if editing the map directly would be too noisy.
- Add new builder cards only for bounded, evidence-backed work not already
  covered by E36-E52.

## Acceptance

- The algorithm map can no longer be mistaken for fully current code truth.
- Existing cards are cross-referenced where they already cover map items.
- Any new cards have exact file evidence and small acceptance gates.

## Suggested Checks

- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused whitespace scan over edited docs.

## Do Not

- Do not implement algorithm work in this slice.
- Do not create broad “build the roadmap” cards.
- Do not overwrite current builder cards without preserving their scoped
  acceptance criteria.

## Completion Brief

Status: done.

Files modified:
- `docs/creative_mode/creative_editor_algorithms_map_v0_1.md`

Actions taken:
- Added a 2026-07-06 triage layer to the algorithm map so it is explicitly a
  planning map, not fully current code truth.
- Corrected stale undo current-state language:
  - product live owns `CreativeDocumentUndoStack` on `CreativeAppState`;
  - standalone aliases that same stack type through `StandaloneUndo.hpp`;
  - there is no kernel `CreativeDocument` history service and no redo stack.
- Corrected stale standalone picking current-state language:
  - E42 replaced the standalone projected center-depth picker with a
    world-space ray/AABB helper;
  - product-live picking remains a separate seam to audit before reusing that
    fix.
- Cross-referenced already-covered work:
  - E42, E46, E49, E50, E52, E54/E55, E62, E71, and E74.
- Marked `CreativeObjectAABBIndex`, redo, `withUndo`, snap expansion,
  marquee/multi-select, token generation, navmesh/parkour/PVS, and other large
  roadmap ideas as speculative until separately carded.
- Marked rotate/scale and ascii pipeline claims as prior recon not re-verified
  by E53.
- No new builder cards were added in this slice. The current ready queue already
  has bounded evidence-backed work, and E53 did not gather new code evidence
  that justified another small acceptance gate.

Verification:
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing-whitespace scan over:
  - `docs/creative_mode/creative_editor_algorithms_map_v0_1.md`
  - `docs/creative_mode/builder_tasks/claimed/E53-algorithm-map-current-state-triage.md`

All verification passed.
