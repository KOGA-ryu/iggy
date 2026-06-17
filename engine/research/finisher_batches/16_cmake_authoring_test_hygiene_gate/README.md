# 16 CMake Authoring Test Hygiene Gate

Status: pending.

Goal: assess whether authoring test registration and fixture compile
definitions can be simplified without renaming tests or changing labels.

Scope:
- Gate/design unless a very small CMake helper is obvious.

Guardrails:
- Do not rename tests.
- Do not change labels.
- Do not alter fixture paths or CLI output contracts.

Verification:
- CMake configure and focused tests if CMake changes.
- Docs diff review if design-only.
