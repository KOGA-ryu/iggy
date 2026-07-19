#include "EditorWorldLayoutOpeningInternal.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>

namespace iggy3d_creative_app::opening_detail {

using detail::noteWorldLayoutSourceChange;

std::pair<cr::CreativeTerrainCoord2, cr::CreativeTerrainCoord2> roomEdgeSegment(
    const cr::CreativeWorldLayoutRoom& room,
    cr::CreativeWorldLayoutRoomEdge edge) {
  switch (edge) {
    case cr::CreativeWorldLayoutRoomEdge::North:
      return {{room.footprint.minimum.x, room.footprint.minimum.z},
              {room.footprint.maximum.x, room.footprint.minimum.z}};
    case cr::CreativeWorldLayoutRoomEdge::East:
      return {{room.footprint.maximum.x, room.footprint.minimum.z},
              {room.footprint.maximum.x, room.footprint.maximum.z}};
    case cr::CreativeWorldLayoutRoomEdge::South:
      return {{room.footprint.minimum.x, room.footprint.maximum.z},
              {room.footprint.maximum.x, room.footprint.maximum.z}};
    case cr::CreativeWorldLayoutRoomEdge::West:
      return {{room.footprint.minimum.x, room.footprint.minimum.z},
              {room.footprint.minimum.x, room.footprint.maximum.z}};
    case cr::CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return {};
}

CreativeEditorWorldLayoutOpeningHost openingHost(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  CreativeEditorWorldLayoutOpeningHost host;
  cr::CreativeTerrainCoord2 start{};
  cr::CreativeTerrainCoord2 end{};
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
    if (opening.wallIndex >= layout.walls.size()) {
      return host;
    }
    const cr::CreativeWorldLayoutWall& wall = layout.walls[opening.wallIndex];
    start = wall.start;
    end = wall.end;
    host.baseLayer = wall.baseLayer;
    host.wallHeightCells = wall.heightCells;
  } else if (opening.hostKind ==
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    if (opening.roomIndex >= layout.rooms.size() ||
        opening.roomEdge >= cr::CreativeWorldLayoutRoomEdge::Count) {
      return host;
    }
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[opening.roomIndex];
    std::tie(start, end) = roomEdgeSegment(room, opening.roomEdge);
    const cr::CreativeWorldLayoutLevel* level =
        cr::creativeWorldLayoutLevelForRoom(layout, opening.roomIndex);
    if (level == nullptr) {
      return host;
    }
    host.baseLayer = level->floorTopLayer;
    host.wallHeightCells = level->wallHeightCells;
  } else {
    return host;
  }
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  host.lengthCells = std::hypot(dx, dz);
  if (!std::isfinite(host.lengthCells) || host.lengthCells <= 0.0 ||
      !std::isfinite(host.baseLayer) ||
      !std::isfinite(host.wallHeightCells) || host.wallHeightCells <= 0.0 ||
      !std::isfinite(host.baseLayer + host.wallHeightCells)) {
    return {};
  }
  host.valid = true;
  host.start = {static_cast<double>(start.x), static_cast<double>(start.z)};
  host.end = {static_cast<double>(end.x), static_cast<double>(end.z)};
  return host;
}

double openingHostOffset(CreativeEditorWorldLayoutOpeningHost host,
                         CreativeEditorWorldLayoutPoint point) noexcept {
  if (!host.valid || !detail::finiteWorldLayoutPoint(point)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double dx = host.end.x - host.start.x;
  const double dz = host.end.z - host.start.z;
  return ((point.x - host.start.x) * dx + (point.z - host.start.z) * dz) /
         host.lengthCells;
}

CreativeEditorWorldLayoutPoint openingHostPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) noexcept {
  if (!host.valid || !std::isfinite(offsetCells)) {
    return {};
  }
  const double inverseLength = 1.0 / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * inverseLength *
                             offsetCells,
          host.start.z + (host.end.z - host.start.z) * inverseLength *
                             offsetCells};
}

bool openingIntervalsOverlap(
    const cr::CreativeWorldLayoutOpening& candidate,
    CreativeEditorWorldLayoutOpeningHost candidateHost,
    const cr::CreativeWorldLayoutOpening& existing,
    CreativeEditorWorldLayoutOpeningHost existingHost) noexcept {
  if (!candidateHost.valid || !existingHost.valid ||
      !std::isfinite(candidate.widthCells) || candidate.widthCells <= 0.0 ||
      !std::isfinite(existing.widthCells) || existing.widthCells <= 0.0) {
    return false;
  }
  const double candidateTop =
      candidateHost.baseLayer + candidateHost.wallHeightCells;
  const double existingTop =
      existingHost.baseLayer + existingHost.wallHeightCells;
  if (candidateTop <= existingHost.baseLayer + kOpeningGeometryEpsilon ||
      existingTop <= candidateHost.baseLayer + kOpeningGeometryEpsilon) {
    return false;
  }
  const double candidateDx = candidateHost.end.x - candidateHost.start.x;
  const double candidateDz = candidateHost.end.z - candidateHost.start.z;
  const double existingDx = existingHost.end.x - existingHost.start.x;
  const double existingDz = existingHost.end.z - existingHost.start.z;
  const double parallelCross =
      candidateDx * existingDz - candidateDz * existingDx;
  const double lineCross =
      candidateDx * (existingHost.start.z - candidateHost.start.z) -
      candidateDz * (existingHost.start.x - candidateHost.start.x);
  if (std::fabs(parallelCross) > kOpeningGeometryEpsilon ||
      std::fabs(lineCross) > kOpeningGeometryEpsilon) {
    return false;
  }
  const CreativeEditorWorldLayoutPoint existingCenter =
      openingHostPoint(existingHost, existing.centerOffsetCells);
  const double inverseCandidateLength = 1.0 / candidateHost.lengthCells;
  const double existingOffsetOnCandidate =
      (existingCenter.x - candidateHost.start.x) * candidateDx *
          inverseCandidateLength +
      (existingCenter.z - candidateHost.start.z) * candidateDz *
          inverseCandidateLength;
  return std::isfinite(existingOffsetOnCandidate) &&
         std::fabs(existingOffsetOnCandidate - candidate.centerOffsetCells) <=
             (existing.widthCells + candidate.widthCells) * 0.5 +
                 kOpeningGeometryEpsilon;
}

double pointDistance(CreativeEditorWorldLayoutPoint lhs,
                     CreativeEditorWorldLayoutPoint rhs) noexcept {
  return std::hypot(lhs.x - rhs.x, lhs.z - rhs.z);
}

void considerSegment(OpeningHostProjection& best,
                     CreativeEditorWorldLayoutPoint point,
                     cr::CreativeTerrainCoord2 start,
                     cr::CreativeTerrainCoord2 end, double tolerance,
                     cr::CreativeWorldLayoutOpeningHostKind hostKind,
                     std::size_t hostIndex,
                     cr::CreativeWorldLayoutRoomEdge roomEdge) {
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (lengthSquared <= 0.0) {
    return;
  }
  const double t = std::clamp(
      ((point.x - start.x) * dx + (point.z - start.z) * dz) /
          lengthSquared,
      0.0, 1.0);
  const double projectedX = start.x + t * dx;
  const double projectedZ = start.z + t * dz;
  const double distance =
      std::hypot(point.x - projectedX, point.z - projectedZ);
  if (distance >= best.distanceCells) {
    return;
  }
  const double length = std::sqrt(lengthSquared);
  best.hit = distance <= tolerance;
  best.hostKind = hostKind;
  best.wallIndex = hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall
                       ? hostIndex
                       : cr::kInvalidCreativeWorldLayoutIndex;
  best.roomIndex = hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge
                       ? hostIndex
                       : cr::kInvalidCreativeWorldLayoutIndex;
  best.roomEdge = roomEdge;
  best.centerOffsetCells = t * length;
  best.lengthCells = length;
  best.distanceCells = distance;
}

OpeningHostProjection nearestOpeningHost(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point, double tolerance,
    std::size_t activeLevelIndex) {
  OpeningHostProjection best;
  const cr::CreativeWorldLayoutLevel* activeLevel =
      activeLevelIndex < layout.levels.size()
          ? &layout.levels[activeLevelIndex]
          : nullptr;
  for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
    const cr::CreativeWorldLayoutWall& wall = layout.walls[index];
    if (activeLevel != nullptr &&
        (wall.buildingIndex != activeLevel->buildingIndex ||
         std::fabs(wall.baseLayer - activeLevel->floorTopLayer) >
             kOpeningGeometryEpsilon)) {
      continue;
    }
    considerSegment(best, point, wall.start, wall.end, tolerance,
                    cr::CreativeWorldLayoutOpeningHostKind::Wall, index,
                    cr::CreativeWorldLayoutRoomEdge::North);
  }
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    if (activeLevelIndex < layout.levels.size() &&
        layout.rooms[roomIndex].levelIndex != activeLevelIndex) {
      continue;
    }
    for (std::uint8_t edgeValue = 0U;
         edgeValue < static_cast<std::uint8_t>(
                         cr::CreativeWorldLayoutRoomEdge::Count);
         ++edgeValue) {
      const auto edge =
          static_cast<cr::CreativeWorldLayoutRoomEdge>(edgeValue);
      const auto [start, end] = roomEdgeSegment(layout.rooms[roomIndex], edge);
      considerSegment(best, point, start, end, tolerance,
                      cr::CreativeWorldLayoutOpeningHostKind::RoomEdge,
                      roomIndex, edge);
    }
  }
  return best;
}

bool sameHost(const cr::CreativeWorldLayoutOpening& opening,
              const OpeningHostProjection& projection) noexcept {
  if (opening.hostKind != projection.hostKind) {
    return false;
  }
  return opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall
             ? opening.wallIndex == projection.wallIndex
             : opening.roomIndex == projection.roomIndex &&
                   opening.roomEdge == projection.roomEdge;
}


bool nearlyEqual(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= kOpeningGeometryEpsilon;
}

bool validOpeningKind(cr::CreativeBuildingOpeningKind kind) noexcept {
  return kind == cr::CreativeBuildingOpeningKind::Door ||
         kind == cr::CreativeBuildingOpeningKind::Window;
}

bool validOpeningPose(cr::CreativeBuildingOpeningPose pose) noexcept {
  return pose >= cr::CreativeBuildingOpeningPose::Closed &&
         pose <=
             cr::CreativeBuildingOpeningPose::OpenFromEndPositiveNormal;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) noexcept {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

CreativeEditorWorldLayoutOpeningSettings openingSettings(
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  return {opening.centerOffsetCells,
          opening.widthCells,
          opening.cutoutBottomCells,
          opening.cutoutHeightCells,
          opening.pose,
          opening.includeInsert};
}

cr::CreativeWorldLayoutOpening openingWithSettings(
    const cr::CreativeWorldLayoutOpening& existing,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  cr::CreativeWorldLayoutOpening candidate = existing;
  const bool insertTracksBottom =
      nearlyEqual(existing.insertBottomCells, existing.cutoutBottomCells);
  const bool insertTracksHeight =
      existing.insertHeightCells > 0.0 &&
      nearlyEqual(existing.insertHeightCells, existing.cutoutHeightCells);
  const bool insertTracksWidth =
      existing.insertWidthCells > 0.0 &&
      nearlyEqual(existing.insertWidthCells, existing.widthCells);
  candidate.centerOffsetCells = settings.centerOffsetCells;
  candidate.widthCells = settings.widthCells;
  candidate.cutoutBottomCells = settings.sillHeightCells;
  candidate.cutoutHeightCells = settings.heightCells;
  candidate.pose = settings.pose;
  candidate.includeInsert = settings.includeInsert;
  if (insertTracksBottom) {
    candidate.insertBottomCells = settings.sillHeightCells;
  }
  if (insertTracksHeight) {
    candidate.insertHeightCells = settings.heightCells;
  }
  if (insertTracksWidth) {
    candidate.insertWidthCells = settings.widthCells;
  }
  return candidate;
}

bool sameEditableOpening(const cr::CreativeWorldLayoutOpening& lhs,
                         const cr::CreativeWorldLayoutOpening& rhs) noexcept {
  return lhs.centerOffsetCells == rhs.centerOffsetCells &&
         lhs.widthCells == rhs.widthCells &&
         lhs.cutoutBottomCells == rhs.cutoutBottomCells &&
         lhs.cutoutHeightCells == rhs.cutoutHeightCells &&
         lhs.pose == rhs.pose && lhs.includeInsert == rhs.includeInsert &&
         lhs.insertBottomCells == rhs.insertBottomCells &&
         lhs.insertHeightCells == rhs.insertHeightCells &&
         lhs.insertWidthCells == rhs.insertWidthCells &&
         lhs.insertThicknessCells == rhs.insertThicknessCells &&
         lhs.insertAssetId == rhs.insertAssetId &&
         lhs.hasInsertAssetSourceBounds ==
             rhs.hasInsertAssetSourceBounds &&
         sameBounds(lhs.insertAssetSourceBoundsMeters,
                    rhs.insertAssetSourceBoundsMeters);
}

OpeningValidation validateOpeningCandidate(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate) {
  if (openingIndex >= state.source.openings.size() ||
      !validOpeningKind(candidate.kind) || !validOpeningPose(candidate.pose) ||
      !std::isfinite(candidate.centerOffsetCells) ||
      !std::isfinite(candidate.widthCells) ||
      !std::isfinite(candidate.cutoutBottomCells) ||
      !std::isfinite(candidate.cutoutHeightCells) ||
      candidate.widthCells < kOpeningMinimumWidthCells ||
      candidate.cutoutBottomCells < 0.0 ||
      candidate.cutoutHeightCells <= 0.0) {
    return {};
  }
  const cr::CreativeBoundsMetrics assetSource =
      cr::measureCreativeBounds(candidate.insertAssetSourceBoundsMeters);
  const bool validAsset =
      candidate.hasInsertAssetSourceBounds
          ? !candidate.insertAssetId.empty() && assetSource.valid &&
                cr::isPositiveCreativeVec3(assetSource.size)
          : candidate.insertAssetId.empty();
  if (!validAsset) {
    return {false,
            "creative_editor_world_layout_opening_insert_asset_invalid",
            "opening insert asset metadata is invalid"};
  }
  if (candidate.kind == cr::CreativeBuildingOpeningKind::Door &&
      !nearlyEqual(candidate.cutoutBottomCells, 0.0)) {
    return {false, "creative_editor_world_layout_door_sill_invalid",
            "door openings must begin at floor height"};
  }
  if (candidate.kind == cr::CreativeBuildingOpeningKind::Window &&
      candidate.pose != cr::CreativeBuildingOpeningPose::Closed) {
    return {false, "creative_editor_world_layout_window_pose_invalid",
            "windows do not support door swing poses"};
  }

  const CreativeEditorWorldLayoutOpeningHost host =
      openingHost(state.source, candidate);
  if (!host.valid) {
    return {false, "creative_editor_world_layout_opening_host_invalid",
            "opening host is unavailable"};
  }
  const double halfWidth = candidate.widthCells * 0.5;
  if (candidate.centerOffsetCells - halfWidth <
          kOpeningEndClearanceCells - kOpeningGeometryEpsilon ||
      candidate.centerOffsetCells + halfWidth >
          host.lengthCells - kOpeningEndClearanceCells +
              kOpeningGeometryEpsilon) {
    return {false, "creative_editor_world_layout_opening_end_clearance_invalid",
            "opening needs a quarter-cell wall pier at each end"};
  }
  if (candidate.cutoutBottomCells + candidate.cutoutHeightCells >
      host.wallHeightCells + kOpeningGeometryEpsilon) {
    return {false, "creative_editor_world_layout_opening_height_invalid",
            "opening exceeds the host wall height"};
  }

  if (!std::isfinite(candidate.insertBottomCells) ||
      !std::isfinite(candidate.insertHeightCells) ||
      !std::isfinite(candidate.insertWidthCells) ||
      !std::isfinite(candidate.insertThicknessCells) ||
      candidate.insertBottomCells < 0.0 ||
      candidate.insertHeightCells < 0.0 ||
      candidate.insertWidthCells < 0.0 ||
      candidate.insertThicknessCells < 0.0) {
    return {false, "creative_editor_world_layout_opening_insert_invalid",
            "opening insert dimensions are invalid"};
  }
  if (candidate.includeInsert) {
    const double insertBottom =
        candidate.kind == cr::CreativeBuildingOpeningKind::Window &&
                candidate.insertBottomCells == 0.0
            ? candidate.cutoutBottomCells
            : candidate.insertBottomCells;
    const double insertHeight = candidate.insertHeightCells > 0.0
                                    ? candidate.insertHeightCells
                                    : candidate.cutoutHeightCells;
    const double insertWidth = candidate.insertWidthCells > 0.0
                                   ? candidate.insertWidthCells
                                   : candidate.widthCells;
    if (insertWidth > candidate.widthCells + kOpeningGeometryEpsilon ||
        insertBottom + kOpeningGeometryEpsilon <
            candidate.cutoutBottomCells ||
        insertBottom + insertHeight >
            candidate.cutoutBottomCells + candidate.cutoutHeightCells +
                kOpeningGeometryEpsilon) {
      return {false, "creative_editor_world_layout_opening_insert_fit_invalid",
              "opening insert no longer fits its cutout"};
    }
  }

  cr::CreativeWorldLayout staged = state.source;
  staged.openings[openingIndex] = candidate;
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(staged)) {
    return {false, "creative_editor_world_layout_window_requires_exterior",
            "windows must remain on exterior room edges"};
  }
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(staged);
  if (!expanded.accepted ||
      openingIndex >= expanded.expanded.openings.size()) {
    return {false, expanded.reasonCode, expanded.reasonCode};
  }
  const cr::CreativeWorldLayoutOpening& resolved =
      expanded.expanded.openings[openingIndex];
  for (std::size_t index = 0U; index < expanded.expanded.openings.size();
       ++index) {
    if (index == openingIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutOpening& existing =
        expanded.expanded.openings[index];
    if (existing.wallIndex == resolved.wallIndex &&
        std::fabs(existing.centerOffsetCells - resolved.centerOffsetCells) <=
            (existing.widthCells + resolved.widthCells) * 0.5 +
                kOpeningGeometryEpsilon) {
      return {false, "creative_editor_world_layout_opening_overlap",
              "opening overlaps another opening on this wall"};
    }
  }
  return {true, "creative_editor_world_layout_opening_settings_ready",
          "opening settings ready"};
}

CreativeEditorWorldLayoutEditReceipt commitOpeningCandidate(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate,
    std::string statusMessage) {
  const OpeningValidation validation =
      validateOpeningCandidate(state, openingIndex, candidate);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  if (sameEditableOpening(state.source.openings[openingIndex], candidate)) {
    return {true, false,
            "creative_editor_world_layout_opening_settings_no_change"};
  }
  state.source.openings[openingIndex] = candidate;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     openingIndex};
  noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true,
          "creative_editor_world_layout_opening_settings_updated"};
}


bool snappedOpeningDelta(double current, double start,
                         double& output) noexcept {
  const double delta = current - start;
  if (!std::isfinite(delta)) {
    return false;
  }
  output = std::round(delta / kOpeningSnapCells) * kOpeningSnapCells;
  return std::isfinite(output);
}

CreativeEditorWorldLayoutOpeningTarget openingTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool includeResizeHandles) noexcept {
  if (!detail::worldLayoutOpeningOnActiveLevel(state, state.source,
                                               openingIndex) ||
      !detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  const CreativeEditorWorldLayoutOpeningHost host =
      openingHost(state.source, opening);
  if (!host.valid) {
    return {};
  }
  const double halfWidth = opening.widthCells * 0.5;
  const CreativeEditorWorldLayoutPoint center =
      openingHostPoint(host, opening.centerOffsetCells);
  const CreativeEditorWorldLayoutPoint start =
      openingHostPoint(host, opening.centerOffsetCells - halfWidth);
  const CreativeEditorWorldLayoutPoint end =
      openingHostPoint(host, opening.centerOffsetCells + halfWidth);
  if (includeResizeHandles) {
    const double centerDistance = pointDistance(point, center);
    const double startDistance = pointDistance(point, start);
    const double endDistance = pointDistance(point, end);
    double nearestDistance = centerDistance;
    CreativeEditorWorldLayoutOpeningHandle nearestHandle =
        CreativeEditorWorldLayoutOpeningHandle::Move;
    if (startDistance < nearestDistance) {
      nearestDistance = startDistance;
      nearestHandle = CreativeEditorWorldLayoutOpeningHandle::Start;
    }
    if (endDistance < nearestDistance) {
      nearestDistance = endDistance;
      nearestHandle = CreativeEditorWorldLayoutOpeningHandle::End;
    }
    if (nearestDistance <= toleranceCells) {
      return {openingIndex, nearestHandle};
    }
  }
  const double pointOffset = openingHostOffset(host, point);
  if (!std::isfinite(pointOffset)) {
    return {};
  }
  const double boundedOffset =
      std::clamp(pointOffset, opening.centerOffsetCells - halfWidth,
                 opening.centerOffsetCells + halfWidth);
  if (pointDistance(point, openingHostPoint(host, boundedOffset)) <=
      toleranceCells) {
    return {openingIndex, CreativeEditorWorldLayoutOpeningHandle::Move};
  }
  return {};
}

}  // namespace iggy3d_creative_app::opening_detail
