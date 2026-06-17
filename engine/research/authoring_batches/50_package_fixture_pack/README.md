# Batch 50: Package Fixture Pack

## Goal
If package execution lands, add a small checked-in package fixture pack mirroring canonical single-file scenarios.

## Current State
Batch 14/17/18 define and gate package layout/execution. Batch 44 covers metadata later.

## Slices
1. Create two or three package fixtures only after Batch 18 approval/implementation.
2. Include one basic movement package, one interaction/pickup package, and one negative package if policy supports it.
3. Add package runner tests using the same expected outcomes as single-file fixtures.
4. Document package fixtures separately from one-file fixtures.

## Verification
Focused package runner tests. Full verification at batch end.

## Hard Stops
Do not proceed before package runner approval. No recursive discovery, dependency resolution, asset catalogs, or UI/Edi.

## Expected Result
Package mode has executable examples without replacing one-file TOML fixtures.
