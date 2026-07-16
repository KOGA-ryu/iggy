#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr double kOpeningHitToleranceCells = 0.75;
constexpr double kSelectionHitToleranceCells = 0.35;

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

void invalidatePreview(CreativeEditorWorldLayoutState& state) {
  state.previewVisible = false;
  state.previewLayoutRevision = 0U;
  state.preview = {};
}

void noteSourceChange(CreativeEditorWorldLayoutState& state,
                      std::string reason) {
  if (state.revision != std::numeric_limits<std::uint64_t>::max()) {
    ++state.revision;
  }
  invalidatePreview(state);
  state.statusMessage = std::move(reason);
}

bool keyExists(const cr::CreativeWorldLayout& layout, std::string_view key) {
  const auto matches = [&](const auto& value) {
    return value.stableKey == key;
  };
  return std::any_of(layout.buildings.begin(), layout.buildings.end(),
                     matches) ||
         std::any_of(layout.rooms.begin(), layout.rooms.end(), matches) ||
         std::any_of(layout.boxes.begin(), layout.boxes.end(), matches) ||
         std::any_of(layout.walls.begin(), layout.walls.end(), matches) ||
         std::any_of(layout.openings.begin(), layout.openings.end(), matches) ||
         std::any_of(layout.terrainProfiles.begin(),
                     layout.terrainProfiles.end(), matches) ||
         std::any_of(layout.terrainPaths.begin(), layout.terrainPaths.end(),
                     matches);
}

std::string mintKey(CreativeEditorWorldLayoutState& state,
                    std::string_view prefix) {
  for (;;) {
    const std::string candidate =
        std::string(prefix) + "_" + std::to_string(state.nextStableOrdinal++);
    if (!keyExists(state.source, candidate)) {
      return candidate;
    }
  }
}

std::size_t ensurePrimaryBuilding(CreativeEditorWorldLayoutState& state) {
  if (!state.source.buildings.empty()) {
    return 0U;
  }
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = mintKey(state, "building");
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
  room.stableKey = mintKey(state, "room");
  room.name = "Room " + std::to_string(state.source.rooms.size() + 1U);
  room.footprint = rect;
  state.source.rooms.push_back(std::move(room));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                     state.source.rooms.size() - 1U};
  noteSourceChange(state, "room added");
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
  box.stableKey = mintKey(state, "floor");
  box.name = "Floor " + std::to_string(state.source.boxes.size() + 1U);
  box.footprint = rect;
  box.baseLayer = 0;
  box.heightCells = 1U;
  state.source.boxes.push_back(std::move(box));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                     state.source.boxes.size() - 1U};
  noteSourceChange(state, "floor added");
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
  wall.stableKey = mintKey(state, "wall");
  wall.name = "Wall " + std::to_string(state.source.walls.size() + 1U);
  wall.start = state.anchor;
  wall.end = point;
  wall.baseLayer = 0;
  state.source.walls.push_back(std::move(wall));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                     state.source.walls.size() - 1U};
  noteSourceChange(state, "wall added");
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
  if (wallLength < opening.widthCells) {
    state.statusMessage = "wall is too short for this opening";
    return {false, false, "creative_editor_world_layout_wall_too_short"};
  }
  const double halfWidth = opening.widthCells * 0.5;
  opening.centerOffsetCells =
      std::clamp(std::round(projection.centerOffsetCells * 4.0) / 4.0,
                 halfWidth, wallLength - halfWidth);
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
        std::fabs(existing.centerOffsetCells - resolved.centerOffsetCells) <
            (existing.widthCells + resolved.widthCells) * 0.5) {
      state.statusMessage = "opening overlaps an existing opening";
      return {false, false, "creative_editor_world_layout_opening_overlap"};
    }
  }
  opening.stableKey = mintKey(
      state, kind == cr::CreativeBuildingOpeningKind::Door ? "door" : "window");
  opening.name =
      (kind == cr::CreativeBuildingOpeningKind::Door ? "Door " : "Window ") +
      std::to_string(state.source.openings.size() + 1U);
  state.source.openings.push_back(std::move(opening));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     state.source.openings.size() - 1U};
  noteSourceChange(state, kind == cr::CreativeBuildingOpeningKind::Door
                              ? "door added"
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

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutTool tool) {
  if (tool >= CreativeEditorWorldLayoutTool::Count) {
    return {false, false, "creative_editor_world_layout_tool_invalid"};
  }
  const bool changed = state.tool != tool || state.anchorActive;
  state.tool = tool;
  state.anchorActive = false;
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
    state.statusMessage = "room shell settings are invalid";
    return {false, false,
            "creative_editor_world_layout_room_settings_invalid"};
  }
  const cr::CreativeWorldLayoutRoom& existingRoom =
      state.source.rooms[roomIndex];
  if (roomFootprintOverlaps(state.source, settings.footprint,
                            existingRoom.buildingIndex,
                            settings.baseLayer, roomIndex)) {
    state.statusMessage = "rooms may touch but cannot overlap";
    return {false, false, "creative_editor_world_layout_room_overlap"};
  }
  for (const cr::CreativeWorldLayoutOpening& opening : state.source.openings) {
    if (opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex != roomIndex) {
      continue;
    }
    const bool horizontal =
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::North ||
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::South;
    const double edgeLength = horizontal
                                  ? width
                                  : depth;
    const double halfWidth = opening.widthCells * 0.5;
    if (opening.centerOffsetCells - halfWidth < 0.0 ||
        opening.centerOffsetCells + halfWidth > edgeLength) {
      state.statusMessage = "resize would move an opening outside its wall";
      return {false, false,
              "creative_editor_world_layout_room_resize_opening_invalid"};
    }
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
  noteSourceChange(state, "room shell settings updated");
  return {true, true, "creative_editor_world_layout_room_settings_updated"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const CreativeEditorWorldLayoutSelection selected = state.selection;
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
  noteSourceChange(state, "layout symbol deleted");
  return {true, true, "creative_editor_world_layout_selection_deleted"};
}

CreativeEditorWorldLayoutEditReceipt cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept {
  const bool changed = state.previewVisible;
  invalidatePreview(state);
  state.statusMessage = "3D preview closed";
  return {true, changed, "creative_editor_world_layout_preview_cancelled"};
}

CreativeEditorWorldLayoutPreviewReceipt previewCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document) {
  CreativeEditorWorldLayoutPreviewReceipt receipt;
  state.anchorActive = false;
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  receipt.status = compiled.receipt.status;
  if (!compiled.receipt.accepted) {
    state.statusMessage = compiled.receipt.reasonCode;
    receipt.reasonCode = compiled.receipt.reasonCode;
    invalidatePreview(state);
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
    invalidatePreview(state);
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
    invalidatePreview(state);
    state.statusMessage =
        result.changed ? "layout generated in 3D" : "3D output already current";
  } else {
    state.statusMessage = result.reasonCode;
  }
  return result;
}

}  // namespace iggy3d_creative_app
