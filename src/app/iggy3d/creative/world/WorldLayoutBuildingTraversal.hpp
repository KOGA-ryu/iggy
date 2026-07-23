#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "runtime/movement/MovementDefaults.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::size_t
    kCreativeWorldLayoutBuildingTraversalIssueCapacity = 32U;

enum class CreativeWorldLayoutBuildingTraversalStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  RoomBakeFailed,
  SurfaceBakeFailed,
  Ready,
  IssuesFound,
};

enum class CreativeWorldLayoutBuildingTraversalIssueKind : std::uint8_t {
  MissingRoomFloorContact,
  RoomStandingClearanceBlocked,
  OpeningPassageBlocked,
  ConnectorTraversalBlocked,
  TraversalCapacityExceeded,
  Count,
};

struct CreativeWorldLayoutBuildingTraversalConfig {
  double actorRadiusMeters = iggy3d::kDefaultPlayerBodyRadiusMeters;
  double actorHeightMeters = iggy3d::kDefaultPlayerStandingHeightMeters;
  double maximumStepMeters = iggy3d::kDefaultPlayerStepHeightMeters;
  double groundSnapMeters = iggy3d::kDefaultPlayerGroundSnapMeters;
  double skinMeters = iggy3d::kDefaultPlayerSkinMeters;
  double floorHeightToleranceMeters = 0.05;
  double openingApproachMarginMeters = 0.05;
  double pathEndpointToleranceMeters = 0.03;
  double rampMoveStepMeters = 0.10;
  std::size_t maximumMotorStepsPerPath = 1024U;
};

struct CreativeWorldLayoutBuildingTraversalRequest {
  const CreativeWorldLayout* layout = nullptr;
  const CreativeDocument* generatedDocument = nullptr;
  CreativeWorldLayoutBuildingTraversalConfig config;
};

struct CreativeWorldLayoutBuildingTraversalIssue {
  CreativeWorldLayoutBuildingTraversalIssueKind kind =
      CreativeWorldLayoutBuildingTraversalIssueKind::MissingRoomFloorContact;
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t index = kInvalidCreativeWorldLayoutIndex;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
};

struct CreativeWorldLayoutBuildingTraversalReceipt {
  bool requested = false;
  bool accepted = false;
  bool traversable = false;
  bool capacityExceeded = false;
  CreativeWorldLayoutBuildingTraversalStatus status =
      CreativeWorldLayoutBuildingTraversalStatus::NotRequested;
  std::string_view reasonCode =
      "creative_world_layout_building_traversal_not_requested";
  std::size_t roomCount = 0U;
  std::size_t roomFloorContactCount = 0U;
  std::size_t roomStandingClearanceCount = 0U;
  std::size_t passageCount = 0U;
  std::size_t traversablePassageCount = 0U;
  std::size_t connectorCount = 0U;
  std::size_t traversableConnectorCount = 0U;
  PhysicsSpatialSurfaceColliderBakeStatus surfaceBakeStatus =
      PhysicsSpatialSurfaceColliderBakeStatus::MissingSurfaceSet;
  std::array<CreativeWorldLayoutBuildingTraversalIssue,
             kCreativeWorldLayoutBuildingTraversalIssueCapacity>
      issues{};
  std::size_t issueCount = 0U;
  std::size_t droppedIssueCount = 0U;
};

[[nodiscard]] std::string_view
creativeWorldLayoutBuildingTraversalReasonCode(
    CreativeWorldLayoutBuildingTraversalIssueKind kind) noexcept;

// Heavy control-path validation over an exact generated candidate. The room
// bake is reduced to source-owned building structure, with door leaves removed
// to model an openable passage. The runtime player body and movement planner
// then prove room occupancy, door crossing, and bidirectional connector travel.
// Callers should cache the receipt by source/document revision.
[[nodiscard]] CreativeWorldLayoutBuildingTraversalReceipt
validateCreativeWorldLayoutBuildingTraversal(
    const CreativeWorldLayoutBuildingTraversalRequest& request);

}  // namespace iggy3d::creative
