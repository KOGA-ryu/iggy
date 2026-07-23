# Persistence and Validation TODO

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| PER-001 | Creative document save and load | Integrated | Planned | Audit P1 found durable writes work, but app save drains only a copy, erases undo, and uses an inconsistent UI-only checkpoint | After Authoring publication identity lands, implement the honest save lifecycle and mixed-domain proof |
| PER-002 | Schema evolution and unknown data | Prototype | Planned | Envelope compatibility exists but Creative open bypasses it; Creative section restore accepts zero/future versions | Enforce compatibility and explicit section-version refusal |
| PER-003 | Deterministic hashes and receipts | Prototype | Planned | Runtime saves hash state; Creative saves hardcode a zero hash and never verify content | Add and verify a canonical Creative envelope-content hash |
| PER-004 | Generated-source persistence | Integrated | Accepted | World Layout source is versioned, migrated, bounded, synchronized, and transported atomically with the document | Replace revision-only source dirty tracking without changing codec bytes |
| PER-005 | Package and imported-asset validation | Missing | Planned | No package validator exists in the Creative-only checkout; map validation covers referenced static-mesh metadata only | Define importer/catalog validation under Assets and Object Composition |
| PER-006 | Product diagnostics | Integrated | Accepted | Typed bounded diagnostics, complete descriptor remediation, object focus, and truncation facts are live | Migrate validation cache freshness to the Authoring publication stamp |
| PER-007 | Acceptance evidence ledger | Missing | Planned | Department test queue exists but has no accepted repair commit or manual result | Record each accepted repair batch and PER-MAN-001 result |
| PER-008 | Honest editor save checkpoint | Missing | Planned | Four save paths erase history; live dirty flags are never acknowledged; source revision can alias on a branch | Preserve history, drain live dirty domains after durable success, and store clean state in history snapshots |
| PER-009 | Creative compatibility and integrity gate | Missing | Planned | Creative load does not call compatibility, section version has no refusal, and content hash is zero | Add compatibility, current/legacy/future pins, and valid-syntax tamper rejection |
| PER-010 | Exact round-trip evidence | Prototype | Planned | Capture compares a small object subset while claiming losslessness | Replace it with canonical durable document/source equivalence |
