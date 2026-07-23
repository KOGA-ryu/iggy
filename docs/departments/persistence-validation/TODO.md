# Persistence and Validation TODO

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| PER-001 | Creative document save and load | Integrated | Manual Test Needed | Save codecs and save-load tests exist | Run a mixed-domain round trip and inspect editability |
| PER-002 | Schema evolution and unknown data | Prototype | Needs Audit | Versioned codecs exist | State forward, backward, unknown-field, and refusal laws explicitly |
| PER-003 | Deterministic hashes and receipts | Integrated | Needs Audit | Hash and receipt tests exist | Map all durable fields and intentional exclusions |
| PER-004 | Generated-source persistence | Integrated | Needs Audit | World Layout codec and source history exist | Prove detached, stale, repaired, and regenerated sources |
| PER-005 | Package and imported-asset validation | Integrated | Needs Audit | Package and static-mesh validators exist | Make missing files, materials, collision, and sockets actionable |
| PER-006 | Product diagnostics | Prototype | Needs Audit | Map and World Layout diagnostics exist | Add stable severity, object links, ownership, and repair guidance |
| PER-007 | Acceptance evidence ledger | Missing | Planned | Department test queue now exists | Record accepted commit and manual result without bloating save formats |
