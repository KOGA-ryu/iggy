# Batch 51: File/Package Parity

## Goal
Prove a package scenario and its equivalent one-file TOML scenario produce the same run/check/trace results.

## Current State
Requires package runner and package fixture pack.

## Slices
1. Pick one simple package fixture with a one-file canonical equivalent.
2. Run both through the facade/CLI path.
3. Compare final rows, summary counts, expectation result, and trace rows where supported.
4. Add diagnostics parity only for one representative negative package.

## Verification
Focused package parity tests.

## Hard Stops
No new package semantics, directory discovery, or output format changes.

## Expected Result
Package mode is a wrapper around existing authoring semantics, not a separate behavior path.
