# Persistence and Validation

## Purpose

Own durable truth and explicit failure reporting across saves, loads, packages,
replay, generated sources, and product acceptance.

## Owns

- Creative and runtime save formats, codecs, envelopes, and schema evolution.
- Deterministic hashes, receipts, replay records, and ordering contracts.
- Package and asset validation.
- Map, building, terrain, and generated-source diagnostics as durable facts.
- Migration and unknown-data policy.

## Does Not Own

- Domain geometry or editor widgets.
- Runtime simulation behavior.
- Transient renderer or input state unless a persistence contract explicitly
  adopts it.

## Dependency Direction

Reads stable domain records and serializes them without inventing domain policy.
Domain departments expose explicit codecs or records. UI consumes typed
diagnostics and may not parse persistence internals.

## Primary Owners

- `src/app/iggy3d/save/`
- `src/runtime/save/`, `src/runtime/replay/`, and runtime diagnostics
- Creative validation owners and save-related tests
- `apps/iggy3d_creative/EditorPersistence.*`

See [FILES.md](FILES.md) for the complete generated assignment.
