# Foundation and Build TODO

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| FND-001 | Department registry and dashboard | Support | Manual Test Needed | Checker covers 10 departments and 1481 files; i3dc build passes | Review the dashboard and file maps for usability |
| FND-002 | Department dependency policy | Prototype | Planned | Include graph is available to the generator | Define allowed department directions and fail new violations |
| FND-003 | Flat Creative app physical reorganization | Support | Planned | 255 files currently share one app directory | Move by reviewed department in mechanical behavior-neutral batches |
| FND-004 | Focused test ownership | Prototype | Needs Audit | Tests are assigned by department | Verify every capability has a proving test and no orphan suites |
| FND-005 | Build target clarity | Support | Needs Audit | i3dc, i3dp, libraries, and focused tests exist | Document target ownership and remove stale or duplicate registrations |
| FND-006 | Third-party and generated-content boundaries | Support | Needs Audit | third_party and asset scripts are separated physically | Pin licensing, generation, and build-time dependency rules |
| FND-007 | File-level purpose descriptions | Prototype | Planned | Generated file roles and include wiring exist | Manually enrich high-risk owners and knots without duplicating grep facts |
