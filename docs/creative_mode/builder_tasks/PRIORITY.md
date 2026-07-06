# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

## Claim Policy

1. If a task listed under **Pull Next** is present in `ready/`, claim the first
   such task.
2. If none of the Pull Next tasks are ready, claim the first ready task from the
   highest non-empty tier below.
3. If a task is already in `claimed/`, do not claim another until that task
   moves to `done/` or `blocked/`.
4. If this file falls behind the physical bucket, the physical bucket wins.

## Currently Claimed

None currently claimed.

## Pull Next

None currently ready.

## Tier 1: Correctness And Compatibility

None currently ready.

## Tier 2: Feature-Add Seams

None currently ready. Builder finished the previous feature-add seam queue
through E80.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

None currently ready.

## Parking Lot

Potential follow-ups still need more review before becoming ready cards:

- Future-storage mutation exposure in any future generic mutation UI.
- Follow-up receipt-builder implementation after E89.
- Follow-up active-room/collision service extraction after E91.
