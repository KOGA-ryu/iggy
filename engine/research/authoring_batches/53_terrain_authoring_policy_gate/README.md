# Batch 53: Terrain Authoring Policy Gate

## Goal
Decide whether terrain glyphs should remain simple walkability only or grow typed terrain metadata.

## Current State
Converter config supports terrain glyph walkability promotion. Source-plan glyph kind includes terrain.

## Slices
1. Inspect level tile/material/collision/render needs around terrain glyphs.
2. Decide whether v1 terrain remains walkable/non-walkable only.
3. If more metadata is justified, propose a small follow-up packet.
4. Otherwise document no-go and keep fixtures simple.

## Verification
Read-only unless docs are updated.

## Hard Stops
No material catalog, tileset, biome, damage, or rendering semantics in this gate.

## Expected Result
Terrain authoring does not accidentally become a broad map authoring system.
