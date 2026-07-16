#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr double kOpeningHitToleranceCells = 0.75;
constexpr double kSelectionHitToleranceCells = 0.35;
constexpr double kOpeningSnapCells = 0.25;
constexpr double kOpeningEndClearanceCells = 0.25;
constexpr double kOpeningMinimumWidthCells = 0.25;
constexpr double kOpeningGeometryEpsilon = 1.0e-9;

using detail::clearWorldLayoutInteraction;
using detail::invalidateWorldLayoutPreview;
using detail::mintWorldLayoutStableKey;
using detail::noteWorldLayoutSourceChange;
using detail::worldLayoutManipulatedRect;
using detail::worldLayoutRectHandleAt;

bool finitePoint(CreativeEditorWorldLayoutPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.z);
}

bool toGridCoord(CreativeEditorWorldLayoutPoint point,
                 cr::CreativeTerrainCoord2& output) noexcept {
  if (!finitePoint(point)) {
    return false;
  }
  const double roundedX = std::round(point.x);
  const double roundedZ = std::round(point.z);
  if (roundedX < std::numeric_limits<std::int32_t>::min() ||
      roundedX > std::numeric_limits<std::int32_t>::max() ||
      roundedZ < std::numeric_limits<std::int32_t>::min() ||
      roundedZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output.x = static_cast<std::int32_t>(roundedX);
  output.z = static_cast<std::int32_t>(roundedZ);
  return true;
}

std::size_t ensurePrimaryBuilding(CreativeEditorWorldLayoutState& state) {
  if (!state.source.buildings.empty()) {
    return 0U;
  }
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = mintWorldLayoutStableKey(state, "building");
  building.name = "Building 1";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  state.source.buildings.push_back(std::move(building));
  return 0U;
}

cr::CreativeWorldLayoutRect normalizedRect(cr::CreativeTerrainCoord2 first,
                                           cr::CreativeTerrainCoord2 second) {
  return {{std::min(first.x, second.x), std::min(first.z, second.z)},
          {std::max(first.x, second.x), std::max(first.z, second.z)}};
}

cr::CreativeTerrainCoord2 cardinalEnd(cr::CreativeTerrainCoord2 start,
                                      cr::CreativeTerrainCoord2 requested) {
  const std::int64_t deltaX = static_cast<std::int64_t>(requested.x) - start.x;
  const std::int64_t deltaZ = static_cast<std::int64_t>(requested.z) - start.z;
  if (std::llabs(deltaX) >= std::llabs(deltaZ)) {
    requested.z = start.z;
  } else {
    requested.x = start.x;
  }
  return requested;
}

struct OpeningHostProjection {
  bool hit = false;
  cr::CreativeWorldLayoutOpeningHostKind hostKind =
      cr::CreativeWorldLayoutOpeningHostKind::Wall;
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutRoomEdge roomEdge =
      cr::CreativeWorldLayoutRoomEdge::North;
  double centerOffsetCells = 0.0;
  double lengthCells = 0.0;
  double distanceCells = std::numeric_limits<double>::infinity();
};

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
    host.wallHeightCells = wall.heightCells;
  } else if (opening.hostKind ==
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    if (opening.roomIndex >= layout.rooms.size() ||
        opening.roomEdge >= cr::CreativeWorldLayoutRoomEdge::Count) {
      return host;
    }
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[opening.roomIndex];
    std::tie(start, end) = roomEdgeSegment(room, opening.roomEdge);
    host.wallHeightCells = room.wallHeightCells;
  } else {
    return host;
  }
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  host.lengthCells = std::hypot(dx, dz);
  if (!std::isfinite(host.lengthCells) || host.lengthCells <= 0.0 ||
      !std::isfinite(host.wallHeightCells) || host.wallHeightCells <= 0.0) {
    return {};
  }
  host.valid = true;
  host.start = {static_cast<double>(start.x), static_cast<double>(start.z)};
  host.end = {static_cast<double>(end.x), static_cast<double>(end.z)};
  return host;
}

double openingHostOffset(CreativeEditorWorldLayoutOpeningHost host,
                         CreativeEditorWorldLayoutPoint point) noexcept {
  if (!host.valid || !finitePoint(point)) {
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
    CreativeEditorWorldLayoutPoint point, double tolerance) {
  OpeningHostProjection best;
  for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
    const cr::CreativeWorldLayoutWall& wall = layout.walls[index];
    considerSegment(best, point, wall.start, wall.end, tolerance,
                    cr::CreativeWorldLayoutOpeningHostKind::Wall, index,
                    cr::CreativeWorldLayoutRoomEdge::North);
  }
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
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

CreativeEditorWorldLayoutSelection hitTest(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point) {
  const OpeningHostProjection host =
      nearestOpeningHost(layout, point, kSelectionHitToleranceCells);
  if (host.hit) {
    for (std::size_t index = layout.openings.size(); index > 0U; --index) {
      const cr::CreativeWorldLayoutOpening& opening =
          layout.openings[index - 1U];
      if (!sameHost(opening, host)) {
        continue;
      }
      const double halfWidth = std::max(0.25, opening.widthCells * 0.5);
      if (std::fabs(opening.centerOffsetCells -
                    host.centerOffsetCells) <= halfWidth) {
        return {CreativeEditorWorldLayoutSelectionKind::Opening, index - 1U};
      }
    }
    if (host.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
      return {CreativeEditorWorldLayoutSelectionKind::Wall,
              host.wallIndex};
    }
    return {CreativeEditorWorldLayoutSelectionKind::Room, host.roomIndex};
  }
  for (std::size_t index = layout.rooms.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutRect& rect =
        layout.rooms[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Room, index - 1U};
    }
  }
  for (std::size_t index = layout.boxes.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutRect& rect =
        layout.boxes[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Box, index - 1U};
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt selectAt(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  const CreativeEditorWorldLayoutSelection selected =
      hitTest(state.source, point);
  const bool changed = selected.kind != state.selection.kind ||
                       selected.index != state.selection.index;
  state.selection = selected;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  state.statusMessage =
      selected.kind == CreativeEditorWorldLayoutSelectionKind::None
          ? "selection cleared"
          : "layout symbol selected";
  return {true, changed, "creative_editor_world_layout_selected"};
}

bool roomFootprintOverlaps(const cr::CreativeWorldLayout& layout,
                           cr::CreativeWorldLayoutRect footprint,
                           std::size_t buildingIndex,
                           std::int32_t baseLayer,
                           std::size_t ignoredRoom =
                               cr::kInvalidCreativeWorldLayoutIndex) {
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    if (index == ignoredRoom) {
      continue;
    }
    if (layout.rooms[index].buildingIndex != buildingIndex ||
        layout.rooms[index].baseLayer != baseLayer) {
      continue;
    }
    const cr::CreativeWorldLayoutRect existing = layout.rooms[index].footprint;
    if (std::max(footprint.minimum.x, existing.minimum.x) <
            std::min(footprint.maximum.x, existing.maximum.x) &&
        std::max(footprint.minimum.z, existing.minimum.z) <
            std::min(footprint.maximum.z, existing.maximum.z)) {
      return true;
    }
  }
  return false;
}

struct RoomSettingsValidation {
  bool accepted = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_settings_invalid";
  std::string message = "room shell settings are invalid";
};

struct OpeningValidation {
  bool accepted = false;
  std::string reasonCode =
      "creative_editor_world_layout_opening_settings_invalid";
  std::string message = "opening settings are invalid";
};

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
         lhs.insertThicknessCells == rhs.insertThicknessCells;
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

CreativeEditorWorldLayoutRoomSettings roomSettings(
    const cr::CreativeWorldLayoutRoom& room,
    cr::CreativeWorldLayoutRect footprint) noexcept {
  return {footprint, room.baseLayer, room.wallHeightCells,
          room.wallThicknessCells, room.floorThicknessCells};
}

RoomSettingsValidation validateRoomSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    const CreativeEditorWorldLayoutRoomSettings& settings) {
  const double width = static_cast<double>(settings.footprint.maximum.x) -
                       settings.footprint.minimum.x;
  const double depth = static_cast<double>(settings.footprint.maximum.z) -
                       settings.footprint.minimum.z;
  if (roomIndex >= state.source.rooms.size() || width <= 0.0 || depth <= 0.0 ||
      settings.wallHeightCells == 0U || settings.floorThicknessCells == 0U ||
      !std::isfinite(settings.wallThicknessCells) ||
      settings.wallThicknessCells <= 0.0 ||
      width <= settings.wallThicknessCells * 2.0 ||
      depth <= settings.wallThicknessCells * 2.0) {
    return {};
  }

  const cr::CreativeWorldLayoutRoom& existingRoom =
      state.source.rooms[roomIndex];
  if (roomFootprintOverlaps(state.source, settings.footprint,
                            existingRoom.buildingIndex, settings.baseLayer,
                            roomIndex)) {
    return {false, "creative_editor_world_layout_room_overlap",
            "rooms may touch but cannot overlap"};
  }

  for (const cr::CreativeWorldLayoutOpening& opening : state.source.openings) {
    if (opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex != roomIndex) {
      continue;
    }
    const bool horizontal =
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::North ||
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::South;
    const double edgeLength = horizontal ? width : depth;
    const double halfWidth = opening.widthCells * 0.5;
    if (opening.centerOffsetCells - halfWidth <
            kOpeningEndClearanceCells - kOpeningGeometryEpsilon ||
        opening.centerOffsetCells + halfWidth >
            edgeLength - kOpeningEndClearanceCells +
                kOpeningGeometryEpsilon) {
      return {
          false,
          "creative_editor_world_layout_room_resize_opening_invalid",
          "resize would move an opening outside its wall",
      };
    }
  }

  cr::CreativeWorldLayout candidate = state.source;
  cr::CreativeWorldLayoutRoom& candidateRoom = candidate.rooms[roomIndex];
  candidateRoom.footprint = settings.footprint;
  candidateRoom.baseLayer = settings.baseLayer;
  candidateRoom.wallHeightCells = settings.wallHeightCells;
  candidateRoom.wallThicknessCells = settings.wallThicknessCells;
  candidateRoom.floorThicknessCells = settings.floorThicknessCells;
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    return {false, expanded.reasonCode, expanded.reasonCode};
  }
  for (std::size_t index = 0U; index < expanded.expanded.openings.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        expanded.expanded.openings[index];
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const cr::CreativeWorldLayoutOpening& existing =
          expanded.expanded.openings[prior];
      if (opening.wallIndex == existing.wallIndex &&
          std::fabs(opening.centerOffsetCells -
                    existing.centerOffsetCells) <=
              (opening.widthCells + existing.widthCells) * 0.5 + 1.0e-9) {
        return {false, "creative_editor_world_layout_opening_overlap",
                "resize would overlap openings on a shared wall"};
      }
    }
  }
  return {true, "creative_editor_world_layout_room_settings_ready",
          "room shell settings ready"};
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
  if (openingIndex >= state.source.openings.size() || !finitePoint(point) ||
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

CreativeEditorWorldLayoutEditReceipt addRoomPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "drag room to its opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  if (rect.minimum.x == rect.maximum.x || rect.minimum.z == rect.maximum.z) {
    state.statusMessage = "room needs width and depth";
    return {false, false, "creative_editor_world_layout_room_degenerate"};
  }
  if (roomFootprintOverlaps(state.source, rect, 0U, 0)) {
    state.statusMessage = "rooms may touch but cannot overlap";
    return {false, false, "creative_editor_world_layout_room_overlap"};
  }
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = ensurePrimaryBuilding(state);
  room.stableKey = mintWorldLayoutStableKey(state, "room");
  room.name = "Room " + std::to_string(state.source.rooms.size() + 1U);
  room.footprint = rect;
  state.source.rooms.push_back(std::move(room));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                     state.source.rooms.size() - 1U};
  noteWorldLayoutSourceChange(state, "room added");
  return {true, true, "creative_editor_world_layout_room_added"};
}

CreativeEditorWorldLayoutEditReceipt addFloorPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "floor start set; choose opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  if (rect.minimum.x == rect.maximum.x || rect.minimum.z == rect.maximum.z) {
    state.statusMessage = "floor needs width and depth";
    return {false, false, "creative_editor_world_layout_floor_degenerate"};
  }
  cr::CreativeWorldLayoutBox box;
  box.buildingIndex = ensurePrimaryBuilding(state);
  box.kind = cr::CreativeObjectKind::Floor;
  box.stableKey = mintWorldLayoutStableKey(state, "floor");
  box.name = "Floor " + std::to_string(state.source.boxes.size() + 1U);
  box.footprint = rect;
  box.baseLayer = 0;
  box.heightCells = 1U;
  state.source.boxes.push_back(std::move(box));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                     state.source.boxes.size() - 1U};
  noteWorldLayoutSourceChange(state, "floor added");
  return {true, true, "creative_editor_world_layout_floor_added"};
}

CreativeEditorWorldLayoutEditReceipt addWallPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "wall start set; choose end";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  point = cardinalEnd(state.anchor, point);
  state.anchorActive = false;
  if (point == state.anchor) {
    state.statusMessage = "wall needs length";
    return {false, false, "creative_editor_world_layout_wall_degenerate"};
  }
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = ensurePrimaryBuilding(state);
  wall.stableKey = mintWorldLayoutStableKey(state, "wall");
  wall.name = "Wall " + std::to_string(state.source.walls.size() + 1U);
  wall.start = state.anchor;
  wall.end = point;
  wall.baseLayer = 0;
  state.source.walls.push_back(std::move(wall));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                     state.source.walls.size() - 1U};
  noteWorldLayoutSourceChange(state, "wall added");
  return {true, true, "creative_editor_world_layout_wall_added"};
}

CreativeEditorWorldLayoutEditReceipt addOpening(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutPoint point,
    cr::CreativeBuildingOpeningKind kind) {
  const OpeningHostProjection projection =
      nearestOpeningHost(state.source, point, kOpeningHitToleranceCells);
  if (!projection.hit) {
    state.statusMessage = "place the opening on a room edge or partition";
    return {false, false, "creative_editor_world_layout_wall_not_found"};
  }
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = projection.hostKind;
  opening.wallIndex = projection.wallIndex;
  opening.roomIndex = projection.roomIndex;
  opening.roomEdge = projection.roomEdge;
  opening.kind = kind;
  opening.pose = cr::CreativeBuildingOpeningPose::Closed;
  opening.includeInsert = true;
  if (kind == cr::CreativeBuildingOpeningKind::Door) {
    opening.widthCells = 1.0;
    opening.cutoutBottomCells = 0.0;
    opening.cutoutHeightCells = 2.1;
    opening.insertBottomCells = 0.0;
    opening.insertHeightCells = 2.1;
    opening.insertWidthCells = 1.0;
    opening.insertThicknessCells = 0.15;
  } else {
    opening.widthCells = 1.5;
    opening.cutoutBottomCells = 1.0;
    opening.cutoutHeightCells = 1.2;
    opening.insertBottomCells = 1.0;
    opening.insertHeightCells = 1.2;
    opening.insertWidthCells = 1.5;
    opening.insertThicknessCells = 0.10;
  }
  const double wallLength = projection.lengthCells;
  const double halfWidth = opening.widthCells * 0.5;
  const double minimumCenter = halfWidth + kOpeningEndClearanceCells;
  const double maximumCenter =
      wallLength - halfWidth - kOpeningEndClearanceCells;
  if (minimumCenter > maximumCenter + kOpeningGeometryEpsilon) {
    state.statusMessage = "wall is too short for this opening";
    return {false, false, "creative_editor_world_layout_wall_too_short"};
  }
  opening.centerOffsetCells =
      std::clamp(std::round(projection.centerOffsetCells * 4.0) / 4.0,
                 minimumCenter, maximumCenter);
  cr::CreativeWorldLayout candidate = state.source;
  candidate.openings.push_back(opening);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    state.statusMessage = expanded.reasonCode;
    return {false, false, expanded.reasonCode};
  }
  const cr::CreativeWorldLayoutOpening& resolved =
      expanded.expanded.openings.back();
  for (std::size_t index = 0U; index + 1U < expanded.expanded.openings.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& existing =
        expanded.expanded.openings[index];
    if (existing.wallIndex == resolved.wallIndex &&
        std::fabs(existing.centerOffsetCells - resolved.centerOffsetCells) <=
            (existing.widthCells + resolved.widthCells) * 0.5 +
                kOpeningGeometryEpsilon) {
      state.statusMessage = "opening overlaps an existing opening";
      return {false, false, "creative_editor_world_layout_opening_overlap"};
    }
  }
  opening.stableKey = mintWorldLayoutStableKey(
      state, kind == cr::CreativeBuildingOpeningKind::Door ? "door" : "window");
  opening.name =
      (kind == cr::CreativeBuildingOpeningKind::Door ? "Door " : "Window ") +
      std::to_string(state.source.openings.size() + 1U);
  state.source.openings.push_back(std::move(opening));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     state.source.openings.size() - 1U};
  noteWorldLayoutSourceChange(
      state, kind == cr::CreativeBuildingOpeningKind::Door ? "door added"
                                                           : "window added");
  return {true, true, "creative_editor_world_layout_opening_added"};
}

}  // namespace

const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept {
  switch (tool) {
    case CreativeEditorWorldLayoutTool::Select:
      return "Select";
    case CreativeEditorWorldLayoutTool::Room:
      return "Room";
    case CreativeEditorWorldLayoutTool::Floor:
      return "Floor";
    case CreativeEditorWorldLayoutTool::Wall:
      return "Partition";
    case CreativeEditorWorldLayoutTool::Door:
      return "Door";
    case CreativeEditorWorldLayoutTool::Window:
      return "Window";
    case CreativeEditorWorldLayoutTool::Count:
      break;
  }
  return "Unknown";
}

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey) {
  state = {};
  state.source.stableKey =
      layoutKey.empty() ? "world_layout" : std::move(layoutKey);
  state.statusMessage = "blank layout";
}

void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout) {
  state = {};
  state.source = std::move(layout);
  state.nextStableOrdinal =
      1U + state.source.buildings.size() + state.source.rooms.size() +
      state.source.boxes.size() + state.source.walls.size() +
      state.source.openings.size() + state.source.terrainProfiles.size() +
      state.source.terrainPaths.size();
  state.statusMessage = "layout loaded";
}

void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept {
  state.savedRevision = state.revision;
}

bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.revision != state.savedRevision;
}

bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.previewVisible && state.preview.accepted &&
         state.preview.document.isValid() &&
         state.previewLayoutRevision == state.revision;
}

const cr::CreativeDocument& creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept {
  return creativeEditorWorldLayoutPreviewActive(state) ? state.preview.document
                                                       : liveDocument;
}

const cr::CreativeWorldLayout& creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.buildingTransform.active &&
                 state.buildingTransform.sourceRevision == state.revision &&
                 state.buildingTransform.buildingIndex <
                     state.buildingTransform.candidate.buildings.size()
             ? state.buildingTransform.candidate
             : state.source;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutTool tool) {
  if (tool >= CreativeEditorWorldLayoutTool::Count) {
    return {false, false, "creative_editor_world_layout_tool_invalid"};
  }
  const bool changed = state.tool != tool || state.anchorActive ||
                       state.roomManipulation.active ||
                       state.boxManipulation.active ||
                       state.wallManipulation.active ||
                       state.buildingManipulation.active ||
                       state.buildingTransform.active ||
                       state.openingManipulation.active ||
                       state.boxSettingsDraft.active ||
                       state.wallSettingsDraft.active ||
                       state.openingSettingsDraft.active;
  state.tool = tool;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  state.statusMessage =
      std::string(creativeEditorWorldLayoutToolLabel(tool)) + " tool";
  return {true, changed, "creative_editor_world_layout_tool_set"};
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  if (!finitePoint(point)) {
    return {false, false, "creative_editor_world_layout_point_non_finite"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Select) {
    return selectAt(state, point);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Door) {
    return addOpening(state, point, cr::CreativeBuildingOpeningKind::Door);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Window) {
    return addOpening(state, point, cr::CreativeBuildingOpeningKind::Window);
  }
  cr::CreativeTerrainCoord2 gridPoint;
  if (!toGridCoord(point, gridPoint)) {
    return {false, false, "creative_editor_world_layout_point_out_of_range"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Room) {
    return addRoomPoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Floor) {
    return addFloorPoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
    return addWallPoint(state, gridPoint);
  }
  return {false, false, "creative_editor_world_layout_tool_invalid"};
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point) {
  if (phase >= CreativeEditorWorldLayoutGesturePhase::Count) {
    return {false, false, "creative_editor_world_layout_gesture_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Cancel) {
    state.anchorActive = false;
    state.statusMessage = "layout gesture cancelled";
    return {true, false, "creative_editor_world_layout_gesture_cancelled"};
  }
  const bool dragTool = state.tool == CreativeEditorWorldLayoutTool::Room ||
                        state.tool == CreativeEditorWorldLayoutTool::Floor ||
                        state.tool == CreativeEditorWorldLayoutTool::Wall;
  if (!dragTool) {
    return {false, false, "creative_editor_world_layout_gesture_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Begin) {
    cr::CreativeTerrainCoord2 gridPoint;
    if (!toGridCoord(point, gridPoint)) {
      return {false, false, "creative_editor_world_layout_point_out_of_range"};
    }
    state.anchorActive = true;
    state.anchor = gridPoint;
    clearWorldLayoutInteraction(state);
    state.statusMessage =
        state.tool == CreativeEditorWorldLayoutTool::Room
            ? "drag room to its opposite corner"
            : state.tool == CreativeEditorWorldLayoutTool::Floor
                  ? "drag floor to its opposite corner"
                  : "drag partition to its end";
    return {true, false, "creative_editor_world_layout_gesture_started"};
  }
  if (!state.anchorActive) {
    return {false, false,
            "creative_editor_world_layout_gesture_not_active"};
  }
  return applyCreativeEditorWorldLayoutPoint(state, point);
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings) {
  const RoomSettingsValidation validation =
      validateRoomSettings(state, roomIndex, settings);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  if (room.footprint.minimum == settings.footprint.minimum &&
      room.footprint.maximum == settings.footprint.maximum &&
      room.baseLayer == settings.baseLayer &&
      room.wallHeightCells == settings.wallHeightCells &&
      room.wallThicknessCells == settings.wallThicknessCells &&
      room.floorThicknessCells == settings.floorThicknessCells) {
    return {true, false,
            "creative_editor_world_layout_room_settings_no_change"};
  }
  room.footprint = settings.footprint;
  room.baseLayer = settings.baseLayer;
  room.wallHeightCells = settings.wallHeightCells;
  room.wallThicknessCells = settings.wallThicknessCells;
  room.floorThicknessCells = settings.floorThicknessCells;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  noteWorldLayoutSourceChange(state, "room shell settings updated");
  return {true, true, "creative_editor_world_layout_room_settings_updated"};
}

CreativeEditorWorldLayoutRoomTarget findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!finitePoint(point) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const CreativeEditorWorldLayoutSelection hit = hitTest(state.source, point);
  if (hit.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      hit.index < state.source.rooms.size()) {
    const CreativeEditorWorldLayoutRoomHandle handle = worldLayoutRectHandleAt(
        state.source.rooms[hit.index].footprint, point, toleranceCells);
    if (handle != CreativeEditorWorldLayoutRoomHandle::None) {
      return {hit.index, handle};
    }
  }
  if (hit.kind != CreativeEditorWorldLayoutSelectionKind::None) {
    return {};
  }
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index < state.source.rooms.size()) {
    const CreativeEditorWorldLayoutRoomHandle handle = worldLayoutRectHandleAt(
        state.source.rooms[state.selection.index].footprint, point,
        toleranceCells);
    if (handle != CreativeEditorWorldLayoutRoomHandle::None) {
      return {state.selection.index, handle};
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutRoomManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Cancel) {
    const bool changed = state.roomManipulation.active;
    state.roomManipulation = {};
    state.statusMessage = "room manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_room_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutRoomTarget target =
        findCreativeEditorWorldLayoutRoomTarget(state, point, toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutRoomHandle::None ||
        target.roomIndex >= state.source.rooms.size()) {
      return selectAt(state, point);
    }
    const cr::CreativeWorldLayoutRect footprint =
        state.source.rooms[target.roomIndex].footprint;
    clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                       target.roomIndex};
    state.anchorActive = false;
    state.roomManipulation = {
        true,
        state.revision,
        target,
        point,
        footprint,
        footprint,
        true,
        "creative_editor_world_layout_room_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutRoomHandle::Move
            ? "drag to move room"
            : "drag to resize room";
    return {true, true,
            "creative_editor_world_layout_room_manipulation_started"};
  }
  if (!state.roomManipulation.active ||
      state.roomManipulation.target.roomIndex >= state.source.rooms.size()) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_not_active"};
  }
  const std::size_t roomIndex = state.roomManipulation.target.roomIndex;
  const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  if (state.revision != state.roomManipulation.sourceRevision ||
      !(room.footprint.minimum ==
            state.roomManipulation.originalFootprint.minimum &&
        room.footprint.maximum ==
            state.roomManipulation.originalFootprint.maximum)) {
    state.roomManipulation = {};
    state.statusMessage = "room changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_room_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Update) {
    cr::CreativeWorldLayoutRect footprint;
    const bool coordinateValid = worldLayoutManipulatedRect(
        state.roomManipulation, point, footprint);
    if (coordinateValid &&
        footprint.minimum == state.roomManipulation.previewFootprint.minimum &&
        footprint.maximum == state.roomManipulation.previewFootprint.maximum) {
      return {true, false, state.roomManipulation.reasonCode};
    }
    RoomSettingsValidation validation;
    if (coordinateValid) {
      validation = validateRoomSettings(
          state, roomIndex, roomSettings(room, footprint));
    } else {
      validation.reasonCode =
          "creative_editor_world_layout_room_manipulation_out_of_range";
      validation.message = "room drag exceeds the layout coordinate range";
    }
    state.roomManipulation.previewFootprint = footprint;
    state.roomManipulation.previewValid = validation.accepted;
    state.roomManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "room drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutRoomManipulation(
          state, CreativeEditorWorldLayoutRoomManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.roomManipulation.previewValid) {
    const std::string reasonCode = state.roomManipulation.reasonCode;
    state.roomManipulation = {};
    return {false, false, reasonCode};
  }
  const cr::CreativeWorldLayoutRect footprint =
      state.roomManipulation.previewFootprint;
  state.roomManipulation = {};
  return setCreativeEditorWorldLayoutRoomSettings(
      state, roomIndex, roomSettings(room, footprint));
}

bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept {
  if (openingIndex >= state.source.openings.size()) {
    return false;
  }
  output = openingSettings(state.source.openings[openingIndex]);
  return true;
}

CreativeEditorWorldLayoutOpeningHost
resolveCreativeEditorWorldLayoutOpeningHost(
    const CreativeEditorWorldLayoutState& state,
    std::size_t openingIndex) noexcept {
  const cr::CreativeWorldLayout &source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (openingIndex >= source.openings.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutOpening& opening = source.openings[openingIndex];
  CreativeEditorWorldLayoutOpeningHost host = openingHost(source, opening);
  if (state.wallManipulation.active &&
      opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex == state.wallManipulation.target.wallIndex) {
    host.start = {
        static_cast<double>(state.wallManipulation.previewStart.x),
        static_cast<double>(state.wallManipulation.previewStart.z),
    };
    host.end = {
        static_cast<double>(state.wallManipulation.previewEnd.x),
        static_cast<double>(state.wallManipulation.previewEnd.z),
    };
  }
  std::size_t hostBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < source.walls.size()) {
    hostBuildingIndex = source.walls[opening.wallIndex].buildingIndex;
  } else if (opening.hostKind ==
                 cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
             opening.roomIndex < source.rooms.size()) {
    hostBuildingIndex = source.rooms[opening.roomIndex].buildingIndex;
  }
  if (state.buildingManipulation.active &&
      hostBuildingIndex == state.buildingManipulation.buildingIndex) {
    const double deltaX = static_cast<double>(
        state.buildingManipulation.previewDeltaXCells);
    const double deltaZ = static_cast<double>(
        state.buildingManipulation.previewDeltaZCells);
    host.start.x += deltaX;
    host.start.z += deltaZ;
    host.end.x += deltaX;
    host.end.z += deltaZ;
  }
  host.lengthCells =
      std::hypot(host.end.x - host.start.x, host.end.z - host.start.z);
  host.valid = std::isfinite(host.lengthCells) && host.lengthCells > 0.0;
  return host;
}

CreativeEditorWorldLayoutOpeningTarget
findCreativeEditorWorldLayoutOpeningTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!finitePoint(point) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::Opening &&
      state.selection.index < state.source.openings.size()) {
    const CreativeEditorWorldLayoutOpeningTarget selectedTarget =
        openingTargetAt(state, state.selection.index, point, toleranceCells,
                        true);
    if (selectedTarget.handle !=
        CreativeEditorWorldLayoutOpeningHandle::None) {
      return selectedTarget;
    }
  }
  for (std::size_t index = state.source.openings.size(); index > 0U; --index) {
    const CreativeEditorWorldLayoutOpeningTarget target = openingTargetAt(
        state, index - 1U, point, toleranceCells, false);
    if (target.handle != CreativeEditorWorldLayoutOpeningHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  if (openingIndex >= state.source.openings.size()) {
    state.statusMessage = "opening selection is stale";
    return {false, false,
            "creative_editor_world_layout_opening_index_invalid"};
  }
  const cr::CreativeWorldLayoutOpening candidate = openingWithSettings(
      state.source.openings[openingIndex], settings);
  return commitOpeningCandidate(state, openingIndex, candidate,
                                "opening settings updated");
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutOpeningManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel) {
    const bool changed = state.openingManipulation.active;
    state.openingManipulation = {};
    state.statusMessage = "opening manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_opening_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutOpeningTarget target =
        findCreativeEditorWorldLayoutOpeningTarget(state, point,
                                                   toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutOpeningHandle::None ||
        target.openingIndex >= state.source.openings.size()) {
      return selectAt(state, point);
    }
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[target.openingIndex];
    const CreativeEditorWorldLayoutOpeningHost host =
        openingHost(state.source, opening);
    const double pointerOffset = openingHostOffset(host, point);
    if (!host.valid || !std::isfinite(pointerOffset)) {
      return {false, false,
              "creative_editor_world_layout_opening_host_invalid"};
    }
    clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                       target.openingIndex};
    state.anchorActive = false;
    state.openingManipulation = {
        true,
        state.revision,
        target,
        pointerOffset,
        opening.centerOffsetCells,
        opening.widthCells,
        opening.centerOffsetCells,
        opening.widthCells,
        true,
        "creative_editor_world_layout_opening_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutOpeningHandle::Move
            ? "drag opening along its wall"
            : "drag to resize opening";
    return {true, true,
            "creative_editor_world_layout_opening_manipulation_started"};
  }

  if (!state.openingManipulation.active ||
      state.openingManipulation.target.openingIndex >=
          state.source.openings.size()) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_not_active"};
  }
  const std::size_t openingIndex =
      state.openingManipulation.target.openingIndex;
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  if (state.revision != state.openingManipulation.sourceRevision ||
      opening.centerOffsetCells !=
          state.openingManipulation.originalCenterOffsetCells ||
      opening.widthCells != state.openingManipulation.originalWidthCells) {
    state.openingManipulation = {};
    state.statusMessage = "opening changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_opening_manipulation_stale"};
  }

  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Update) {
    const CreativeEditorWorldLayoutOpeningHost host =
        openingHost(state.source, opening);
    const double pointerOffset = openingHostOffset(host, point);
    double delta = 0.0;
    if (!host.valid || !std::isfinite(pointerOffset) ||
        !snappedOpeningDelta(pointerOffset,
                             state.openingManipulation.startPointerOffsetCells,
                             delta)) {
      return {false, false,
              "creative_editor_world_layout_opening_pointer_invalid"};
    }

    double center = state.openingManipulation.originalCenterOffsetCells;
    double width = state.openingManipulation.originalWidthCells;
    const double originalMinimum = center - width * 0.5;
    const double originalMaximum = center + width * 0.5;
    switch (state.openingManipulation.target.handle) {
      case CreativeEditorWorldLayoutOpeningHandle::Move:
        center += delta;
        break;
      case CreativeEditorWorldLayoutOpeningHandle::Start: {
        const double minimum = originalMinimum + delta;
        center = (minimum + originalMaximum) * 0.5;
        width = originalMaximum - minimum;
        break;
      }
      case CreativeEditorWorldLayoutOpeningHandle::End: {
        const double maximum = originalMaximum + delta;
        center = (originalMinimum + maximum) * 0.5;
        width = maximum - originalMinimum;
        break;
      }
      case CreativeEditorWorldLayoutOpeningHandle::None:
      case CreativeEditorWorldLayoutOpeningHandle::Count:
        return {
            false, false,
            "creative_editor_world_layout_opening_manipulation_handle_invalid"};
    }
    if (center == state.openingManipulation.previewCenterOffsetCells &&
        width == state.openingManipulation.previewWidthCells) {
      return {true, false, state.openingManipulation.reasonCode};
    }

    CreativeEditorWorldLayoutOpeningSettings settings =
        openingSettings(opening);
    settings.centerOffsetCells = center;
    settings.widthCells = width;
    const cr::CreativeWorldLayoutOpening candidate =
        openingWithSettings(opening, settings);
    const OpeningValidation validation =
        validateOpeningCandidate(state, openingIndex, candidate);
    state.openingManipulation.previewCenterOffsetCells = center;
    state.openingManipulation.previewWidthCells = width;
    state.openingManipulation.previewValid = validation.accepted;
    state.openingManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "opening drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutOpeningManipulation(
          state, CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          point, toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.openingManipulation.previewValid) {
    const std::string reasonCode = state.openingManipulation.reasonCode;
    state.openingManipulation = {};
    return {false, false, reasonCode};
  }
  CreativeEditorWorldLayoutOpeningSettings settings = openingSettings(opening);
  settings.centerOffsetCells =
      state.openingManipulation.previewCenterOffsetCells;
  settings.widthCells = state.openingManipulation.previewWidthCells;
  const cr::CreativeWorldLayoutOpening candidate =
      openingWithSettings(opening, settings);
  state.openingManipulation = {};
  return commitOpeningCandidate(state, openingIndex, candidate,
                                "opening updated");
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const CreativeEditorWorldLayoutSelection selected = state.selection;
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    return deleteCreativeEditorWorldLayoutBuilding(state, selected.index);
  }
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      selected.index < state.source.rooms.size()) {
    const std::size_t removedRoom = selected.index;
    state.source.rooms.erase(state.source.rooms.begin() +
                             static_cast<std::ptrdiff_t>(removedRoom));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.hostKind !=
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
                      return false;
                    }
                    if (opening.roomIndex == removedRoom) {
                      return true;
                    }
                    if (opening.roomIndex > removedRoom) {
                      --opening.roomIndex;
                    }
                    return false;
                  });
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Box &&
      selected.index < state.source.boxes.size()) {
    state.source.boxes.erase(state.source.boxes.begin() +
                             static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Opening &&
             selected.index < state.source.openings.size()) {
    state.source.openings.erase(state.source.openings.begin() +
                                static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Wall &&
             selected.index < state.source.walls.size()) {
    const std::size_t removedWall = selected.index;
    state.source.walls.erase(state.source.walls.begin() +
                             static_cast<std::ptrdiff_t>(removedWall));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.hostKind !=
                        cr::CreativeWorldLayoutOpeningHostKind::Wall) {
                      return false;
                    }
                    if (opening.wallIndex == removedWall) {
                      return true;
                    }
                    if (opening.wallIndex > removedWall) {
                      --opening.wallIndex;
                    }
                    return false;
                  });
  } else {
    return {false, false, "creative_editor_world_layout_selection_missing"};
  }
  state.selection = {};
  noteWorldLayoutSourceChange(state, "layout symbol deleted");
  return {true, true, "creative_editor_world_layout_selection_deleted"};
}

CreativeEditorWorldLayoutEditReceipt cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept {
  const bool changed = state.previewVisible;
  invalidateWorldLayoutPreview(state);
  state.statusMessage = "3D preview closed";
  return {true, changed, "creative_editor_world_layout_preview_cancelled"};
}

CreativeEditorWorldLayoutPreviewReceipt previewCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document) {
  CreativeEditorWorldLayoutPreviewReceipt receipt;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  receipt.status = compiled.receipt.status;
  if (!compiled.receipt.accepted) {
    state.statusMessage = compiled.receipt.reasonCode;
    receipt.reasonCode = compiled.receipt.reasonCode;
    invalidateWorldLayoutPreview(state);
    return receipt;
  }
  cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  receipt.status = preview.status;
  receipt.accepted = preview.accepted;
  receipt.changed = preview.accepted;
  receipt.reasonCode = preview.reasonCode;
  if (!preview.accepted) {
    state.statusMessage = preview.reasonCode;
    invalidateWorldLayoutPreview(state);
    return receipt;
  }
  state.preview = std::move(preview);
  state.previewVisible = true;
  state.previewLayoutRevision = state.revision;
  state.statusMessage = "exact 3D preview ready";
  return receipt;
}

CreativeEditorWorldLayoutApplyReceipt confirmCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState) {
  CreativeEditorWorldLayoutApplyReceipt result;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       state.source);
  if (!compiled.receipt.accepted) {
    result.reasonCode = compiled.receipt.reasonCode;
    state.statusMessage = result.reasonCode;
    return result;
  }
  result.apply = cr::applyCreativeWorldLayoutPlanWithHistory(
      appState, compiled.plan, "desktop_world_layout_confirm");
  result.accepted = result.apply.accepted;
  result.changed = result.apply.changed;
  result.reasonCode = result.apply.reasonCode;
  if (result.accepted) {
    state.generatedRevision = state.revision;
    invalidateWorldLayoutPreview(state);
    state.statusMessage =
        result.changed ? "layout generated in 3D" : "3D output already current";
  } else {
    state.statusMessage = result.reasonCode;
  }
  return result;
}

}  // namespace iggy3d_creative_app
