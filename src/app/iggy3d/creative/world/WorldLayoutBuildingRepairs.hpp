#pragma once

#include "app/iggy3d/creative/world/WorldLayoutBuildingUsability.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutBuildingRepairOperation : std::uint8_t {
  None,
  AddExteriorEntrance,
  ConnectRoom,
  ExpandOpeningClearance,
  Count,
};

enum class CreativeWorldLayoutBuildingRepairStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  UnsupportedIssue,
  NoValidCandidate,
  Ready,
};

struct CreativeWorldLayoutBuildingRepairRequest {
  const CreativeWorldLayout* layout = nullptr;
  CreativeWorldLayoutBuildingUsabilityIssue issue;
  CreativeWorldLayoutBuildingUsabilityConfig config;
  std::uint64_t nextStableOrdinal = 1U;
};

struct CreativeWorldLayoutBuildingRepairResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutBuildingRepairStatus status =
      CreativeWorldLayoutBuildingRepairStatus::NotRequested;
  CreativeWorldLayoutBuildingRepairOperation operation =
      CreativeWorldLayoutBuildingRepairOperation::None;
  CreativeWorldLayout edited;
  std::uint64_t nextStableOrdinal = 1U;
  CreativeWorldLayoutTable resultTable = CreativeWorldLayoutTable::None;
  std::size_t resultIndex = kInvalidCreativeWorldLayoutIndex;
  std::string_view reasonCode =
      "creative_world_layout_building_repair_not_requested";
};

[[nodiscard]] CreativeWorldLayoutBuildingRepairOperation
creativeWorldLayoutBuildingRepairOperation(
    CreativeWorldLayoutBuildingUsabilityIssueKind kind) noexcept;

// Plans one conservative repair against semantic source truth. Only repairs
// with a deterministic valid candidate are accepted; ambiguous architecture is
// left unchanged for explicit creator editing.
[[nodiscard]] CreativeWorldLayoutBuildingRepairResult
planCreativeWorldLayoutBuildingRepair(
    const CreativeWorldLayoutBuildingRepairRequest& request);

}  // namespace iggy3d::creative
