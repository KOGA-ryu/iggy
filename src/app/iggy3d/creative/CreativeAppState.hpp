#pragma once

#include "app/iggy3d/creative/Facade.hpp"

#include <cstdint>
#include <string>

namespace iggy3d::creative {

// Identity + save-status of the creative world the product app currently has
// live. SLICE 2 moves this off ProductAppWindowState (the god-struct) into
// creative's own container; the god-struct mirror fields are kept in lockstep
// (additive) so the receipt + identity tests stay green while production
// readers migrate onto this.
struct CreativeActiveIdentity {
  std::string saveId = "none";
  std::string savePath = "none";
  std::string worldId = "none";
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  CreativeObjectId nextObjectId = kInvalidObjectId;

  std::string saveStatus = "creative_world_save_not_requested";
  std::string saveReasonCode = "creative_world_save_not_requested";
  std::uint64_t saveDirtyFlagsBefore = 0;
  std::uint64_t saveDirtyFlagsDrained = 0;
  std::uint64_t saveDirtyFlagsAfter = 0;
  std::string saveSavedAtUtc = "none";

  // Reset to the "no live creative world" baseline. The single owner of the
  // clear body: the product app calls this instead of triplicating the field
  // writes across Operations / save::Flow / the pause flow.
  void clear() noexcept {
    saveId = "none";
    savePath = "none";
    worldId = "none";
    documentId = kInvalidDocumentId;
    objectCount = 0;
    nextObjectId = kInvalidObjectId;
    saveStatus = "creative_world_save_not_requested";
    saveReasonCode = "creative_world_save_not_requested";
    saveDirtyFlagsBefore = 0;
    saveDirtyFlagsDrained = 0;
    saveDirtyFlagsAfter = 0;
    saveSavedAtUtc = "none";
  }

  // True when a creative world is live (a save id, a world id, or a document).
  [[nodiscard]] bool worldActive() const noexcept {
    return (!saveId.empty() && saveId != "none") ||
           (!worldId.empty() && worldId != "none") ||
           documentId != kInvalidDocumentId;
  }
};

// Self-contained home for creative's app-scoped state. SLICE 1 wrapped only the
// logical Facade; SLICE 2 adds the active-world identity so the product app can
// read creative's own state for routing/save instead of the god-struct.
struct CreativeAppState {
  Facade facade;
  CreativeActiveIdentity identity;
};

}  // namespace iggy3d::creative
