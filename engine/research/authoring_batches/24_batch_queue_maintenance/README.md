# Batch 24: Batch Queue Maintenance

## Goal
Keep `engine/research/authoring_batches` accurate as packets complete and priorities shift.

## Current State
The bucket is the durable builder queue.

## Slices
1. Mark completed packets consistently.
2. Remove or rewrite stale future packets that no longer match repo direction.
3. Add replacement packets if the queue has fewer than ten incomplete actionable batches.
4. Keep the root README queue accurate.

## Verification
Docs diff check. No code tests unless files outside research are touched.

## Hard Stops
No production code in this maintenance batch.

## Expected Result
Builder always has a current, ordered bucket to scoop from.
