#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/hash/StableHash.hpp"
#include "runtime/ai/GuardRecon.hpp"

namespace iggy3d {

// A guard whose normalized alert level is at or above this is reported as "on alert" in the intel
// summary (suspicious/searching/combat vs calm patrol). Named, not magic.
inline constexpr float kReconAlertedLevel = 0.5F;

// Intel-packet kernel (game_master_plan L3 -- the transferable co-op payload). captureReconIntel
// aggregates the guards a thief scouted (GuardReconObservations) into ONE ReconIntel value the
// notebook renders and the knight+priestess duo will later receive. PURE, deterministic, and
// ORDER-SENSITIVE: the scouted order IS intel; swapping two guards changes the hash -- no
// set-semantics. Floor-plan / objective / hazard facts join later from RoomFacts; the guard half
// ships now and the packet is valid with an empty garrison.
struct ReconIntel {
  std::vector<GuardReconObservation> guards;  // scouted garrison, in scouted order
  std::uint32_t guardCount = 0;
  std::uint32_t alertedGuardCount = 0;        // guards with alertLevel >= kReconAlertedLevel
  bool anyGuardHasLastKnownTarget = false;    // "the garrison knows an intruder is here"
};

// Fold N observations into the packet + its derived summary. Copy-in, order-preserving, pure.
ReconIntel captureReconIntel(std::span<const GuardReconObservation> observations);

// Deterministic digest of the whole packet (float-quantized for platform stability, like
// StateHash). Identical packets hash identically; reordering guards changes the hash.
StableHashValue hashReconIntel(const ReconIntel& intel);

struct ReconIntelDecodeResult {
  bool ok = false;
  std::string reasonCode;
  ReconIntel intel;
};

// Serialize the packet to deterministic, versioned text -- lossless floats via std::to_chars, the
// same discipline SaveCodec uses (to_chars out, strtof in). Round-trips: deserialize(serialize(x))
// reproduces x. Only the scouted guards are written; the derived summary is recomputed on decode so
// there is one source of truth.
std::string serializeReconIntel(const ReconIntel& intel);
ReconIntelDecodeResult deserializeReconIntel(std::string_view text);

}  // namespace iggy3d
