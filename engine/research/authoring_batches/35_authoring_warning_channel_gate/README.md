# Batch 35: Authoring Warning Channel Gate

## Goal
Decide whether authoring needs non-fatal warnings before adding any warning API.

## Current State
Diagnostics are mostly failure-oriented. Error-code stabilization is covered by Batch 28.

## Slices
1. Review possible warning cases: unused regions, unused profiles, unreachable targets, and cosmetic glyph omissions.
2. Decide whether warnings are needed or would create noise.
3. If needed, write the next implementation packet.
4. Otherwise document the no-go briefly.

## Verification
Read-only unless docs are updated.

## Hard Stops
No warning implementation in this gate, and no lint-policy expansion unless accepted.

## Expected Result
A clear decision on warning scope before any warning surface is added.
