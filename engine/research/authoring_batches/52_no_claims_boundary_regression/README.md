# Batch 52: No-Claims Boundary Regression Pack

Status: complete.

## Goal
Harden the source-plan no-claims/promotion safety flags with explicit TOML fixtures and diagnostics.

## Current State
`RuntimeGameplayAsciiSourcePlanNoClaims` and `RuntimeGameplayAsciiSourcePlanPromotionPolicy` exist; unsafe flags are validation failures.

## Slices
1. Add targeted negative TOML fixtures for unsafe no-claims and unsafe promotion flags.
2. Ensure diagnostics include table/key/line through TOML reader source-location mapping.
3. Add focused validator and CLI failure tests.
4. Update fixture README regression-only list.

## Verification
Focused source-plan/TOML/CLI diagnostics tests.

## Hard Stops
Do not loosen safety flags. No runtime execution claims or file parsing claims beyond current reader boundary.

## Expected Result
Boundary safety claims cannot regress silently.

## Completed Coverage
- Added regression fixtures for unsafe `[no_claims]` and `[promotion]` boundary flags.
- TOML mirrored source-plan diagnostics now identify the unsafe table, key, and source line.
- Validator issues identify which unsafe flag tripped through the existing issue index field.
- Focused file-reader and CLI failure matrices cover both unsafe boundary fixtures.
