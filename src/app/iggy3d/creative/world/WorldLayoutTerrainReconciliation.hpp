#pragma once

#include "app/iggy3d/creative/world/WorldLayoutTerrainImpact.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutTerrainConflictResolution : std::uint8_t {
  Block,
  Regenerate,
  Count,
};

enum class CreativeWorldLayoutTerrainReconciliationStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  ImpactRejected,
  DecisionInvalid,
  Conflict,
  Ready,
};

struct CreativeWorldLayoutTerrainConflictDecision {
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t generatedIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeWorldLayoutTerrainConflictResolution resolution =
      CreativeWorldLayoutTerrainConflictResolution::Block;
};

struct CreativeWorldLayoutTerrainConflict {
  CreativeWorldLayoutTable generatedTable = CreativeWorldLayoutTable::None;
  std::size_t generatedIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeWorldLayoutTerrainImpactStatus impactStatus =
      CreativeWorldLayoutTerrainImpactStatus::NoEffect;
  bool desiredSourcePresent = false;
  CreativeWorldLayoutTable desiredTable = CreativeWorldLayoutTable::None;
  std::size_t desiredIndex = kInvalidCreativeWorldLayoutIndex;
  bool canDetachAndKeep3D = false;
};

struct CreativeWorldLayoutTerrainReconciliationRequest {
  const CreativeDocument* document = nullptr;
  const CreativeWorldLayout* generatedLayout = nullptr;
  const CreativeWorldLayout* desiredLayout = nullptr;
  std::span<const CreativeWorldLayoutTerrainConflictDecision> decisions;
};

struct CreativeWorldLayoutTerrainReconciliationResult {
  bool requested = false;
  bool accepted = false;
  bool blocked = false;
  CreativeWorldLayoutTerrainReconciliationStatus status =
      CreativeWorldLayoutTerrainReconciliationStatus::NotRequested;
  std::uint64_t detachedSourceCount = 0U;
  std::vector<CreativeWorldLayoutTerrainConflict> conflicts;
  std::string reasonCode =
      "creative_world_layout_terrain_reconciliation_not_requested";
  std::string impactReasonCode =
      "creative_world_layout_terrain_impact_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutTerrainConflictResolution resolution) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutTerrainReconciliationStatus status) noexcept;

// O((g + d + c) log(g + d + c)), where g is generated terrain sources, d is
// desired terrain sources, and c is decisions. Conflicts retain generated
// source order. The kernel is read-only and rejects stale or unused decisions.
[[nodiscard]] CreativeWorldLayoutTerrainReconciliationResult
reconcileCreativeWorldLayoutTerrain(
    const CreativeWorldLayoutTerrainReconciliationRequest& request);

}  // namespace iggy3d::creative
