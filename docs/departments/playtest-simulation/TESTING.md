# Playtest and Simulation Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_play_preparation_tests \
  creative_playtest_launch_tests \
  playtest_lifecycle_tests \
  playtest_process_owner_tests \
  i3dp_headless_smoke
ctest --test-dir build \
  -R '^(creative_play_preparation_tests|creative_playtest_launch_tests|playtest_lifecycle_tests|playtest_process_owner_tests|i3dp_headless_smoke)$' \
  --output-on-failure
```

## Manual Acceptance

The current playtest command and canonical save contract need one dedicated
product brief before a manual case can be considered reliable.

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| PLY-MAN-001 | PLY-001 | Launch the canonical authored map in the separate playtest executable and return without changing the editor document | Command contract pending | Blocked | Not yet |
