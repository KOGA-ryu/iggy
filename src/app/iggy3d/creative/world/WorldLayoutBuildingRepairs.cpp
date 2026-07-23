#include "app/iggy3d/creative/world/WorldLayoutBuildingRepairs.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kOpeningSnapCells = 0.25;
constexpr double kGeometryEpsilon = 1.0e-9;

struct OpeningHostCandidate {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::Count;
};

void reject(CreativeWorldLayoutBuildingRepairResult& result,
            CreativeWorldLayoutBuildingRepairStatus status,
            std::string_view reasonCode) noexcept {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.reasonCode = reasonCode;
}

bool sameIssue(const CreativeWorldLayoutBuildingUsabilityIssue& lhs,
               const CreativeWorldLayoutBuildingUsabilityIssue& rhs) noexcept {
  return lhs.kind == rhs.kind && lhs.table == rhs.table &&
         lhs.index == rhs.index && lhs.buildingIndex == rhs.buildingIndex;
}

bool receiptContainsIssue(
    const CreativeWorldLayoutBuildingUsabilityReceipt& receipt,
    const CreativeWorldLayoutBuildingUsabilityIssue& issue) noexcept {
  return std::any_of(
      receipt.issues.begin(), receipt.issues.begin() + receipt.issueCount,
      [&issue](const CreativeWorldLayoutBuildingUsabilityIssue& candidate) {
        return sameIssue(candidate, issue);
      });
}

double snapUp(double value) noexcept {
  return std::ceil(value / kOpeningSnapCells - kGeometryEpsilon) *
         kOpeningSnapCells;
}

double snapNearest(double value) noexcept {
  return std::round(value / kOpeningSnapCells) * kOpeningSnapCells;
}

bool validRepairCandidate(
    const CreativeWorldLayout& candidate,
    const CreativeWorldLayoutBuildingRepairRequest& request) {
  const CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      validateCreativeWorldLayoutBuildingUsability(
          {&candidate, nullptr, request.config});
  return receipt.accepted && !receiptContainsIssue(receipt, request.issue);
}

bool configurePassableDoor(
    const CreativeWorldLayoutOpeningHostFrame& host,
    const CreativeWorldLayoutBuildingUsabilityConfig& config,
    CreativeWorldLayoutOpening& opening) noexcept {
  const double minimumWidthCells =
      (config.actorRadiusMeters * 2.0 + config.skinMeters * 2.0) /
      config.gridCellSizeMeters;
  const double minimumHeightCells =
      (config.actorHeightMeters + config.skinMeters) /
      config.gridCellSizeMeters;
  opening.kind = CreativeBuildingOpeningKind::Door;
  opening.widthCells = snapUp(std::max(1.0, minimumWidthCells));
  opening.cutoutBottomCells = 0.0;
  opening.cutoutHeightCells = snapUp(std::max(2.1, minimumHeightCells));
  opening.includeInsert = true;
  opening.insertBottomCells = 0.0;
  opening.insertHeightCells = opening.cutoutHeightCells;
  opening.insertWidthCells = opening.widthCells;
  opening.insertThicknessCells = std::min(0.15, host.wallThicknessCells);
  const double minimumCenter =
      opening.widthCells * 0.5 + kCreativeWorldLayoutOpeningEndClearanceCells;
  const double maximumCenter =
      host.lengthCells - opening.widthCells * 0.5 -
      kCreativeWorldLayoutOpeningEndClearanceCells;
  if (!std::isfinite(minimumCenter) || !std::isfinite(maximumCenter) ||
      minimumCenter > maximumCenter + kGeometryEpsilon ||
      opening.cutoutHeightCells > host.wallHeightCells + kGeometryEpsilon ||
      opening.insertThicknessCells <= 0.0) {
    return false;
  }
  opening.centerOffsetCells =
      std::clamp(snapNearest(host.lengthCells * 0.5), minimumCenter,
                 maximumCenter);
  return true;
}

bool tryDoorCandidate(
    const CreativeWorldLayoutBuildingRepairRequest& request,
    OpeningHostCandidate hostCandidate, bool exterior,
    std::string_view name,
    CreativeWorldLayoutBuildingRepairOperation operation,
    CreativeWorldLayoutBuildingRepairResult& result) {
  CreativeWorldLayout candidate = *request.layout;
  CreativeWorldLayoutOpening opening;
  opening.hostKind = CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = hostCandidate.roomIndex;
  opening.roomTopologyEdgeIndex = hostCandidate.topologyEdgeIndex;
  opening.roomEdge = hostCandidate.roomEdge;
  const CreativeWorldLayoutOpeningHostFrame initialHost =
      resolveCreativeWorldLayoutOpeningHost(candidate, opening);
  if (!initialHost.accepted || initialHost.exterior != exterior ||
      !configurePassableDoor(initialHost, request.config, opening)) {
    return false;
  }

  constexpr std::array<CreativeBuildingOpeningFacing, 2U> kFacings{
      CreativeBuildingOpeningFacing::PositiveNormal,
      CreativeBuildingOpeningFacing::NegativeNormal};
  constexpr std::array<CreativeDoorSwingSide, 2U> kSwingSides{
      CreativeDoorSwingSide::PositiveNormal,
      CreativeDoorSwingSide::NegativeNormal};
  constexpr std::array<CreativeDoorHingeSide, 2U> kHingeSides{
      CreativeDoorHingeSide::MinimumEdge,
      CreativeDoorHingeSide::MaximumEdge};
  for (const CreativeBuildingOpeningFacing facing : kFacings) {
    for (const CreativeDoorSwingSide swingSide : kSwingSides) {
      for (const CreativeDoorHingeSide hingeSide : kHingeSides) {
        CreativeWorldLayout attempt = candidate;
        std::uint64_t nextOrdinal = request.nextStableOrdinal;
        opening.facing = facing;
        opening.door.swingSide = swingSide;
        opening.door.hingeSide = hingeSide;
        opening.stableKey = mintCreativeWorldLayoutStableKey(
            attempt, nextOrdinal, "door");
        opening.name = name;
        const CreativeWorldLayoutOpeningValidationResult validation =
            validateCreativeWorldLayoutOpening({&attempt, &opening});
        if (!validation.accepted) {
          continue;
        }
        const std::size_t openingIndex = attempt.openings.size();
        attempt.openings.push_back(opening);
        if (!validRepairCandidate(attempt, request)) {
          continue;
        }
        result.accepted = true;
        result.changed = true;
        result.status = CreativeWorldLayoutBuildingRepairStatus::Ready;
        result.operation = operation;
        result.edited = std::move(attempt);
        result.nextStableOrdinal = nextOrdinal;
        result.resultTable = CreativeWorldLayoutTable::Opening;
        result.resultIndex = openingIndex;
        result.reasonCode =
            "creative_world_layout_building_repair_ready";
        return true;
      }
    }
  }
  return false;
}

template <typename TryCandidate>
bool forEachRoomHost(const CreativeWorldLayout& layout,
                     std::size_t roomIndex,
                     TryCandidate&& tryCandidate) {
  if (roomIndex >= layout.rooms.size()) {
    return false;
  }
  if (!layout.topologyEdges.empty() && !layout.roomBoundaries.empty()) {
    for (const CreativeWorldLayoutRoomBoundary& boundary :
         layout.roomBoundaries) {
      if (boundary.roomIndex == roomIndex &&
          tryCandidate(OpeningHostCandidate{
              roomIndex, boundary.topologyEdgeIndex,
              CreativeWorldLayoutRoomEdge::Count})) {
        return true;
      }
    }
    return false;
  }
  for (std::uint8_t value = 0U;
       value < static_cast<std::uint8_t>(CreativeWorldLayoutRoomEdge::Count);
       ++value) {
    if (tryCandidate(OpeningHostCandidate{
            roomIndex, kInvalidCreativeWorldLayoutIndex,
            static_cast<CreativeWorldLayoutRoomEdge>(value)})) {
      return true;
    }
  }
  return false;
}

bool repairMissingEntrance(
    const CreativeWorldLayoutBuildingRepairRequest& request,
    CreativeWorldLayoutBuildingRepairResult& result) {
  if (request.issue.table != CreativeWorldLayoutTable::Building ||
      request.issue.index >= request.layout->buildings.size()) {
    return false;
  }
  const std::size_t buildingIndex = request.issue.index;
  double lowestFloor = std::numeric_limits<double>::infinity();
  for (const CreativeWorldLayoutRoom& room : request.layout->rooms) {
    if (room.buildingIndex == buildingIndex &&
        room.levelIndex < request.layout->levels.size()) {
      lowestFloor = std::min(
          lowestFloor,
          request.layout->levels[room.levelIndex].floorTopLayer);
    }
  }
  for (std::size_t roomIndex = 0U; roomIndex < request.layout->rooms.size();
       ++roomIndex) {
    const CreativeWorldLayoutRoom& room = request.layout->rooms[roomIndex];
    if (room.buildingIndex != buildingIndex ||
        room.levelIndex >= request.layout->levels.size() ||
        std::fabs(request.layout->levels[room.levelIndex].floorTopLayer -
                  lowestFloor) > kGeometryEpsilon) {
      continue;
    }
    if (forEachRoomHost(
            *request.layout, roomIndex,
            [&](OpeningHostCandidate host) {
              return tryDoorCandidate(
                  request, host, true, "Entrance",
                  CreativeWorldLayoutBuildingRepairOperation::
                      AddExteriorEntrance,
                  result);
            })) {
      return true;
    }
  }
  return false;
}

bool repairDisconnectedRoom(
    const CreativeWorldLayoutBuildingRepairRequest& request,
    CreativeWorldLayoutBuildingRepairResult& result) {
  if (request.issue.table != CreativeWorldLayoutTable::Room ||
      request.issue.index >= request.layout->rooms.size()) {
    return false;
  }
  return forEachRoomHost(
      *request.layout, request.issue.index,
      [&](OpeningHostCandidate host) {
        return tryDoorCandidate(
            request, host, false, "Room Connection",
            CreativeWorldLayoutBuildingRepairOperation::ConnectRoom,
            result);
      });
}

bool repairOpeningClearance(
    const CreativeWorldLayoutBuildingRepairRequest& request,
    CreativeWorldLayoutBuildingRepairResult& result) {
  if (request.issue.table != CreativeWorldLayoutTable::Opening ||
      request.issue.index >= request.layout->openings.size()) {
    return false;
  }
  CreativeWorldLayout candidate = *request.layout;
  CreativeWorldLayoutOpening& opening =
      candidate.openings[request.issue.index];
  if (opening.kind != CreativeBuildingOpeningKind::Door) {
    return false;
  }
  const CreativeWorldLayoutOpeningHostFrame host =
      resolveCreativeWorldLayoutOpeningHost(candidate, opening);
  if (!host.accepted) {
    return false;
  }
  CreativeWorldLayoutOpening repaired = opening;
  const bool includeInsert = repaired.includeInsert;
  const double insertBottomCells = repaired.insertBottomCells;
  const double insertHeightCells = repaired.insertHeightCells;
  const double insertWidthCells = repaired.insertWidthCells;
  const double insertThicknessCells = repaired.insertThicknessCells;
  const std::string insertAssetId = repaired.insertAssetId;
  const CreativeBounds insertSourceBounds = repaired.insertAssetSourceBoundsMeters;
  const bool hasInsertSourceBounds = repaired.hasInsertAssetSourceBounds;
  if (!configurePassableDoor(host, request.config, repaired)) {
    return false;
  }
  repaired.stableKey = opening.stableKey;
  repaired.name = opening.name;
  repaired.includeInsert = includeInsert;
  if (!includeInsert) {
    repaired.insertBottomCells = insertBottomCells;
    repaired.insertHeightCells = insertHeightCells;
    repaired.insertWidthCells = insertWidthCells;
    repaired.insertThicknessCells = insertThicknessCells;
  }
  repaired.insertAssetId = insertAssetId;
  repaired.insertAssetSourceBoundsMeters = insertSourceBounds;
  repaired.hasInsertAssetSourceBounds = hasInsertSourceBounds;
  const CreativeWorldLayoutOpeningValidationResult validation =
      validateCreativeWorldLayoutOpening(
          {&candidate, &repaired, request.issue.index});
  if (!validation.accepted) {
    return false;
  }
  opening = std::move(repaired);
  if (!validRepairCandidate(candidate, request)) {
    return false;
  }
  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutBuildingRepairStatus::Ready;
  result.operation =
      CreativeWorldLayoutBuildingRepairOperation::ExpandOpeningClearance;
  result.edited = std::move(candidate);
  result.nextStableOrdinal = request.nextStableOrdinal;
  result.resultTable = CreativeWorldLayoutTable::Opening;
  result.resultIndex = request.issue.index;
  result.reasonCode = "creative_world_layout_building_repair_ready";
  return true;
}

}  // namespace

CreativeWorldLayoutBuildingRepairOperation
creativeWorldLayoutBuildingRepairOperation(
    CreativeWorldLayoutBuildingUsabilityIssueKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutBuildingUsabilityIssueKind::
        MissingExteriorEntrance:
      return CreativeWorldLayoutBuildingRepairOperation::AddExteriorEntrance;
    case CreativeWorldLayoutBuildingUsabilityIssueKind::DisconnectedRoom:
      return CreativeWorldLayoutBuildingRepairOperation::ConnectRoom;
    case CreativeWorldLayoutBuildingUsabilityIssueKind::
        OpeningClearanceTooSmall:
      return CreativeWorldLayoutBuildingRepairOperation::
          ExpandOpeningClearance;
    case CreativeWorldLayoutBuildingUsabilityIssueKind::BuildingWithoutRooms:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::InvalidConnector:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::
        ConnectorClearanceTooSmall:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::
        MissingVerticalConnection:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::MissingGeneratedFloor:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::
        MissingGeneratedOpening:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::
        MissingGeneratedConnector:
    case CreativeWorldLayoutBuildingUsabilityIssueKind::Count:
      return CreativeWorldLayoutBuildingRepairOperation::None;
  }
  return CreativeWorldLayoutBuildingRepairOperation::None;
}

CreativeWorldLayoutBuildingRepairResult
planCreativeWorldLayoutBuildingRepair(
    const CreativeWorldLayoutBuildingRepairRequest& request) {
  CreativeWorldLayoutBuildingRepairResult result;
  result.requested = true;
  result.nextStableOrdinal = request.nextStableOrdinal;
  if (request.layout == nullptr || request.nextStableOrdinal == 0U ||
      request.issue.kind >=
          CreativeWorldLayoutBuildingUsabilityIssueKind::Count) {
    reject(result, CreativeWorldLayoutBuildingRepairStatus::InvalidRequest,
           "creative_world_layout_building_repair_request_invalid");
    return result;
  }
  const CreativeWorldLayoutBuildingUsabilityReceipt baseline =
      validateCreativeWorldLayoutBuildingUsability(
          {request.layout, nullptr, request.config});
  if (!baseline.accepted || !receiptContainsIssue(baseline, request.issue)) {
    reject(result, CreativeWorldLayoutBuildingRepairStatus::InvalidRequest,
           "creative_world_layout_building_repair_issue_stale");
    return result;
  }

  result.operation =
      creativeWorldLayoutBuildingRepairOperation(request.issue.kind);
  bool repaired = false;
  switch (result.operation) {
    case CreativeWorldLayoutBuildingRepairOperation::AddExteriorEntrance:
      repaired = repairMissingEntrance(request, result);
      break;
    case CreativeWorldLayoutBuildingRepairOperation::ConnectRoom:
      repaired = repairDisconnectedRoom(request, result);
      break;
    case CreativeWorldLayoutBuildingRepairOperation::ExpandOpeningClearance:
      repaired = repairOpeningClearance(request, result);
      break;
    case CreativeWorldLayoutBuildingRepairOperation::None:
    case CreativeWorldLayoutBuildingRepairOperation::Count:
      reject(result, CreativeWorldLayoutBuildingRepairStatus::UnsupportedIssue,
             "creative_world_layout_building_repair_issue_unsupported");
      return result;
  }
  if (!repaired) {
    reject(result, CreativeWorldLayoutBuildingRepairStatus::NoValidCandidate,
           "creative_world_layout_building_repair_candidate_unavailable");
  }
  return result;
}

}  // namespace iggy3d::creative
