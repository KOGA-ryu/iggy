# Batch 44: Package Metadata Manifest

## Goal
If package execution is approved, add minimal package metadata that tools can display without changing scenario execution.

## Current State
Batch 14 proposes package layout. Batch 17/18 gate and implement package runner if approved.

## Slices
1. Add package metadata only after package runner exists: title, description, authoring version, canonical scenario file name.
2. Parse/read metadata without executing scenario semantics.
3. Add tests for missing/invalid metadata according to accepted package policy.
4. Update package docs/examples.

## Verification
Focused package tests; full verification at batch end.

## Hard Stops
Do not proceed before Batch 17/18 approval. No directory discovery, dependency resolution, or asset catalogs.

## Expected Result
Packages can expose stable metadata to future tools without becoming a content management system.
