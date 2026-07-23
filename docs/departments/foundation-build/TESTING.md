# Foundation and Build Testing

## Automated Gate

```bash
python3 tools/repo_departments.py check
cmake -S . -B build
cmake --build build --target i3dc
```

The department checker is the focused proof for this documentation batch. A
broad CTest run is not required merely for ownership-document changes.

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| FND-MAN-001 | FND-001 | Open the department index and confirm current work, manual test demand, and file ownership are understandable without repository archaeology | Read documentation | Pending | Not yet |
