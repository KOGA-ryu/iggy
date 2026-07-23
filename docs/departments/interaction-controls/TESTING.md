# Interaction and Controls Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_editor_controls_tests \
  creative_editor_action_hints_tests \
  creative_select_tests \
  creative_editor_measurement_tests \
  creative_viewport_layout_tests
ctest --test-dir build \
  -R '^(creative_editor_controls_tests|creative_editor_action_hints_tests|creative_select_tests|creative_editor_measurement_tests|creative_viewport_layout_tests)$' \
  --output-on-failure
```

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| INT-MAN-001 | INT-001 | Navigate and edit with MacBook touchpad, keyboard, and PS5 controller without capture loss or conflicting actions | `./build/i3dc` | Pending | Not yet |

### INT-MAN-001 Steps

1. Enter and leave fly-look repeatedly with the touchpad.
2. Navigate, select, confirm, cancel, rotate, and adjust distance.
3. Repeat the workflow with the PS5 controller.
4. Open the radial tool wheel and return to editing without losing camera axes.
5. Verify Cross confirms, Circle cancels, and flight controls do not trigger
   domain actions.
6. Record every overlap, inversion, dead zone, focus, or capture defect.
