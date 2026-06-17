# 13 Package Facade Parity Assertion Helper

Status: complete.

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

Result:
- Added `engine/tests/support/AuthoringParityTestSupport.hpp` for source/package
  TOML facade run summary, expectation, and trace parity assertions.
- Updated the package facade test to use the shared parity assertions while
  preserving assertion messages and result comparisons.
- Left CLI output-contract helpers unchanged because those tests compare
  rendered text projections rather than package/source facade parity.
