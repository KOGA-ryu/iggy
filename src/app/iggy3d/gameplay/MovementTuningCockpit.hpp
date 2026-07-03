#pragma once

#include <filesystem>
#include <string>

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "config/MovementDimensionProfile.hpp"

namespace iggy3d {

// M-LAB s1 -- the tuning cockpit's swap + export actions (app-side; the Controller is untouched, this
// changes VALUES only). Free functions so tests drive the REAL action path with explicit inputs.

// Serialize a row to the `[movement_dimension_profile]` TOML schema (lossless shortest-form floats,
// quoted strings). The exact inverse of config's parseMovementDimensionProfileRow (round-trip proven).
std::string serializeMovementDimensionProfileRow(const MovementDimensionProfile& row);

struct MovementTuningSwapResult {
  bool applied = false;
  std::string appliedProfileId;
  bool admissionMismatch = false;  // the row's limit != the live (hashed) session admission limit
};

// Hot-swap: cycle to the next dimension row, apply it to the live window tuning, remember it as the
// last-applied id, and set receipts. The session's HASHED admission limit is NOT touched -- when the
// new row's limit differs, the receipt honestly flags `admission_limit_unswapped`.
MovementTuningSwapResult applyMovementTuningProfileSwap(ProductAppWindowState& window,
                                                        float sessionAdmissionLimitMeters);

struct MovementTuningExportResult {
  bool ok = false;
  std::string reasonCode;
  std::string exportId;
  std::filesystem::path path;
};

// Export: assemble the live tuning + last-applied row into a MovementDimensionProfile, validate
// coherence (fail closed on an incoherent row -- a dash that outruns its admission limit), then write
// a numbered `cockpit_export_<n>.movementprofile.toml` under `<saveRoot>/movement_profiles/`. Sets
// receipts. Deterministic (directory-scan numbering, no wall-clock).
MovementTuningExportResult exportMovementTuningProfile(ProductAppWindowState& window,
                                                       const std::filesystem::path& saveRoot);

}  // namespace iggy3d
