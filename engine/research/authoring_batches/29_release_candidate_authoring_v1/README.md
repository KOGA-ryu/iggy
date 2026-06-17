# Batch 29: Release Candidate Authoring v1

Status: complete.

## Goal
Freeze a minimal Authoring v1 acceptance set.

## Current State
Requires the prior hardening/debugging packets to be complete or consciously skipped.

## Slices
1. List supported TOML tables, CLI modes, canonical fixtures, and known non-goals.
2. Run the full canonical fixture pack through CLI normal, trace, lint/check modes if present.
3. Add or update one concise release-candidate doc.
4. Do not add new semantics.

## Verification
Full configure/build/CTest and fixture CLI sweep.

## Hard Stops
No feature additions in this packet.

## Expected Result
Authoring v1 has a clear supported surface and acceptance set.

## Completed Coverage
- Added `engine/research/authoring_v1_release_candidate.md`.
- Release-candidate acceptance now names the supported single-file TOML and
  package inputs, run/lint/check/trace modes, expectations, diagnostics,
  preview model, canonical fixtures, negative fixtures, package fixtures, and
  package parity coverage.
- No new scenario semantics were added.
