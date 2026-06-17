# Batch 38: No Hidden Defaults Audit

Status: complete.

## Goal
Ensure canonical authored scenarios do not depend on invisible C++ converter config while keeping lower-level override tests intact.

## Current State
The canonical fixture contract forbids hidden converter config. Regression fixtures may depend on C++ config intentionally.

## Slices
1. Grep TOML/source-plan/converter tests for supplied default frame, profile catalog, terrain, default control, and AiMap policies.
2. Classify each as canonical-unsafe vs targeted override coverage.
3. Add assertions/docs preventing canonical fixtures from needing hidden config.
4. Keep explicit override tests where they prove behavior.

## Verification
Focused CLI and converter tests.

## Hard Stops
Do not delete config override coverage just because canonical fixtures are self-contained.

## Expected Result
The self-contained fixture promise remains enforceable.

## Completed Coverage
- Added a canonical-fixture audit that reads every canonical fixture and adapts it through the default authoring adapter path.
- The audit asserts no explicit adapter config, C++ default frame, C++ profile catalog, terrain defaults, default control, or region AI map policy is supplied.
- Existing explicit converter config tests remain in place as targeted override/regression coverage.
- Canonical fixture CLI sweep still covers run/trace/check behavior separately from lower-level config override tests.
