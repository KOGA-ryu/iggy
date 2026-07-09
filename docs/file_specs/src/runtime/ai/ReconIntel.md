# File Spec

Files: `src/runtime/ai/ReconIntel.hpp`, `src/runtime/ai/ReconIntel.cpp`

Verified at: `abe51abb`

## Owns

- Recon intel packet aggregation, summary fields, deterministic hash, and versioned text serialization.
- `ReconIntel`, `ReconIntelDecodeResult`, `captureReconIntel(...)`, `hashReconIntel(...)`, `serializeReconIntel(...)`, and `deserializeReconIntel(...)`.
- Order-sensitive scouted-guard packet semantics.

## Does Not Own

- Guard observation projection; `GuardRecon.*` owns `GuardReconObservation` creation.
- Notebook UI, save-slot persistence, co-op transfer policy, or live session scheduling.
- AI behavior state mutation or reasoning graph construction.

## Reads

- Caller-provided `GuardReconObservation` spans.
- Stable hash helpers, `Vec3`, behavior names from `AiBehaviorKind`, patrol mode, and reasoning node kind fields.
- Serialized `recon_intel.v1` text when decoding.

## Writes / Mutates

- No external state.
- Returns `ReconIntel`, `StableHashValue`, serialized text, or decode result.

## Calls Out To / Wires Out To

- Uses `StableHasher` with quantized float/vector hashing.
- Re-resolves decoded behavior text through `aiBehaviorKindName(...)`.
- Rebuilds derived summary through `captureReconIntel(...)` after decode.

## Called By / Entry Points

- Unit tests call capture, hash, serialize, and deserialize entry points directly.
- Current production callers are unknown from this slice.
- Grep proof: `rg -n "ReconIntel|captureReconIntel|serializeReconIntel|deserializeReconIntel|hashReconIntel" src tests`.

## Invariants

- `kReconAlertedLevel` defines the alert-count threshold.
- Capture preserves input order and copies guard observations.
- Hash is deterministic and order-sensitive.
- Serialization writes only guard rows; summary fields are recomputed on decode.
- Decode rejects the wrong header and reports `recon_intel_bad_header`.
- Decoded behavior strings must resolve to stable behavior-name storage, not temporary text.

## Tests / Proof Commands

- `rg -n "recon_intel_tests" cmake tests`.
- `rg -n "captureReconIntel|hashReconIntel|serializeReconIntel|deserializeReconIntel" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/GuardRecon.*` unless observation fields change.
- `src/runtime/ai/AiState.*` unless behavior names or patrol/reasoning fields change.
- Save/hash/golden files unless recon intel becomes durable state by explicit design.

## Update When

- Recon packet fields, summary derivation, hash ingredients, text format, decode reason codes, or behavior-name decoding changes.

## Do Not Update When

- Notebook or gameplay consumers change without changing the recon packet contract.
