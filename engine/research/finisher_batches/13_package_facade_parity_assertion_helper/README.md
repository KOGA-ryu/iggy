# 13 Package Facade Parity Assertion Helper

Status: pending.

Goal: extract package/file parity assertion helpers from package facade and CLI
tests where duplication is now obvious.

Scope:
- Test support only.

Guardrails:
- No production behavior changes.
- No result shape changes.
- No package behavior changes.
- No CLI output or exit-code changes.

Verification:
- Package facade tests.
- CLI tests touched by the helper.
