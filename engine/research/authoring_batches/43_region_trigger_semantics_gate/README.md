# Batch 43: Region Trigger Semantics Gate

## Goal
Decide whether source-plan regions can become trigger/check zones, or should stay AiMap/editor metadata only.

## Current State
Regions and role tags exist. Batch 11 covers AiMap region fixtures.

## Slices
1. Inspect current region-to-AiMap promoter and runtime interaction/player movement surfaces.
2. Decide whether trigger zones are needed now or should wait for editor/runtime event policy.
3. If approved, define one tiny trigger fact and a follow-up implementation packet.

## Verification
Read-only gate unless docs are updated.

## Hard Stops
No proximity scripting, no continuous runtime scanning hidden in actor logic, no UI/Edi.

## Expected Result
Region semantics remain controlled and compute-aware.
