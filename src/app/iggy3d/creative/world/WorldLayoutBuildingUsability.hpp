#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "runtime/movement/MovementDefaults.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::size_t
    kCreativeWorldLayoutBuildingUsabilityIssueCapacity = 32U;

enum class CreativeWorldLayoutBuildingUsabilityStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  Ready,
  IssuesFound,
};

enum class CreativeWorldLayoutBuildingUsabilityIssueKind : std::uint8_t {
  BuildingWithoutRooms,
  MissingExteriorEntrance,
  OpeningClearanceTooSmall,
  InvalidConnector,
  ConnectorClearanceTooSmall,
  MissingVerticalConnection,
  DisconnectedRoom,
  MissingGeneratedFloor,
  MissingGeneratedOpening,
  MissingGeneratedConnector,
  Count,
};

// These defaults mirror the runtime player's standing movement envelope.
// Callers can supply a different envelope when validating another actor class.
struct CreativeWorldLayoutBuildingUsabilityConfig {
  double gridCellSizeMeters = 1.0;
  double actorRadiusMeters = iggy3d::kDefaultPlayerBodyRadiusMeters;
  double actorHeightMeters = iggy3d::kDefaultPlayerStandingHeightMeters;
  double maximumStepMeters = iggy3d::kDefaultPlayerStepHeightMeters;
  double skinMeters = iggy3d::kDefaultPlayerSkinMeters;
  double maximumRampSlopeDegrees =
      static_cast<double>(iggy3d::kDefaultMaxWalkableSlopeDegrees);
};

struct CreativeWorldLayoutBuildingUsabilityRequest {
  const CreativeWorldLayout* layout = nullptr;
  // Optional exact generated candidate. When present, semantic source rows
  // must resolve to their expected floor, opening, and connector objects.
  const CreativeDocument* generatedDocument = nullptr;
  CreativeWorldLayoutBuildingUsabilityConfig config;
};

struct CreativeWorldLayoutBuildingUsabilityIssue {
  CreativeWorldLayoutBuildingUsabilityIssueKind kind =
      CreativeWorldLayoutBuildingUsabilityIssueKind::BuildingWithoutRooms;
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t index = kInvalidCreativeWorldLayoutIndex;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
};

struct CreativeWorldLayoutBuildingUsabilityReceipt {
  bool requested = false;
  bool accepted = false;
  bool usable = false;
  bool capacityExceeded = false;
  CreativeWorldLayoutBuildingUsabilityStatus status =
      CreativeWorldLayoutBuildingUsabilityStatus::NotRequested;
  std::size_t buildingCount = 0U;
  std::size_t usableBuildingCount = 0U;
  std::size_t roomCount = 0U;
  std::size_t reachableRoomCount = 0U;
  std::array<CreativeWorldLayoutBuildingUsabilityIssue,
             kCreativeWorldLayoutBuildingUsabilityIssueCapacity>
      issues{};
  std::size_t issueCount = 0U;
  std::size_t droppedIssueCount = 0U;
};

[[nodiscard]] std::string_view
creativeWorldLayoutBuildingUsabilityReasonCode(
    CreativeWorldLayoutBuildingUsabilityIssueKind kind) noexcept;

// Control-path validation. For R rooms, O openings, C connectors, and S shared
// edge/boundary rows: discovery is O(R^2), legacy/direct opening matching is
// O(O*(S+B)), and fixed-point reachability is O(R*(O+C)). Output is
// deterministic and issue-bounded.
[[nodiscard]] CreativeWorldLayoutBuildingUsabilityReceipt
validateCreativeWorldLayoutBuildingUsability(
    const CreativeWorldLayoutBuildingUsabilityRequest& request);

}  // namespace iggy3d::creative
