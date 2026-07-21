#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <tuple>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr std::array<CreativeEditorWorldLayoutPaletteEntry, 16U>
    kWorldLayoutPaletteEntries = {{
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Select",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Select, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Estate House",
         CreativeEditorWorldLayoutPaletteActivation::BuildingTemplate,
         CreativeEditorWorldLayoutTool::Select,
         cr::kBuilderEstateHouseTemplateId},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Building Shell",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::BuildingShell, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Add Room",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Room, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Floor",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Floor, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Partition",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Wall, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Door",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Door, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Window",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Window, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Stair",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Stair, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Ramp",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Ramp, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Terrain, "Plateau",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Plateau, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Terrain, "Road",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Road, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Terrain, "Ditch",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Ditch, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Object, "Bridge",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Bridge, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Gameplay, "Player Spawn",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::PlayerSpawn, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Gameplay, "NPC Spawn",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::NpcSpawn, {}},
    }};

struct VerticalConnectorToolSpec {
  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Count;
  cr::CreativeWorldLayoutVerticalConnectorKind kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Count;
  const char* label = "Unknown";
  const char* keyPrefix = "vertical_connector";
};

constexpr std::array<VerticalConnectorToolSpec, 2U> kVerticalConnectorToolSpecs{
    {
        {CreativeEditorWorldLayoutTool::Stair,
         cr::CreativeWorldLayoutVerticalConnectorKind::Stair, "Stair", "stair"},
        {CreativeEditorWorldLayoutTool::Ramp,
         cr::CreativeWorldLayoutVerticalConnectorKind::Ramp, "Ramp", "ramp"},
    }};

const VerticalConnectorToolSpec*
verticalConnectorToolSpec(CreativeEditorWorldLayoutTool tool) noexcept {
  const auto found = std::find_if(
      kVerticalConnectorToolSpecs.begin(), kVerticalConnectorToolSpecs.end(),
      [tool](const VerticalConnectorToolSpec& candidate) {
        return candidate.tool == tool;
      });
  return found == kVerticalConnectorToolSpecs.end() ? nullptr : &*found;
}

std::string verticalConnectorReasonCode(const VerticalConnectorToolSpec& spec,
                                        std::string_view suffix) {
  return "creative_editor_world_layout_" + std::string(spec.keyPrefix) +
         std::string(suffix);
}

std::string verticalConnectorMessage(const VerticalConnectorToolSpec& spec,
                                     std::string_view suffix) {
  return std::string(spec.keyPrefix) + std::string(suffix);
}

using detail::clearWorldLayoutInteraction;
using detail::invalidateWorldLayoutPreview;
using detail::mintWorldLayoutStableKey;
using detail::noteWorldLayoutSourceChange;
using detail::worldLayoutManipulatedRect;
using detail::worldLayoutRectHandleAt;

bool toGridCoord(CreativeEditorWorldLayoutPoint point,
                 cr::CreativeTerrainCoord2& output) noexcept {
  if (!detail::finiteWorldLayoutPoint(point)) {
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
  const CreativeEditorWorldLayoutRoomSettings settings{
      rect, 0.0, cr::kDefaultCreativeWorldLayoutWallHeightCells,
      cr::kDefaultCreativeWorldLayoutWallThicknessCells, 1U};
  if (state.source.buildings.empty()) {
    return createCreativeEditorWorldLayoutBuildingShell(state, settings);
  }
  std::size_t buildingIndex = creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      state.source.buildings.size() == 1U) {
    buildingIndex = 0U;
  }
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (state.activeLevelIndex < state.source.levels.size() &&
      state.source.levels[state.activeLevelIndex].buildingIndex ==
          buildingIndex) {
    levelIndex = state.activeLevelIndex;
  } else {
    for (std::size_t index = 0U; index < state.source.levels.size(); ++index) {
      if (state.source.levels[index].buildingIndex != buildingIndex) {
        continue;
      }
      if (levelIndex != cr::kInvalidCreativeWorldLayoutIndex) {
        levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
        break;
      }
      levelIndex = index;
    }
  }
  return createCreativeEditorWorldLayoutRoom(state, levelIndex, settings);
}

CreativeEditorWorldLayoutEditReceipt addBuildingShellPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "drag building shell to its opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  return createCreativeEditorWorldLayoutBuildingShell(
      state,
      {rect, 0.0, cr::kDefaultCreativeWorldLayoutWallHeightCells,
       cr::kDefaultCreativeWorldLayoutWallThicknessCells, 1U});
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
  box.anchorLayer = 0.0;
  box.layerCount = 1U;
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
  CreativeEditorWorldLayoutOpeningPlacementRequest request;
  request.point = point;
  request.kind = kind;
  return applyCreativeEditorWorldLayoutOpeningPlacement(state, request);
}

struct ContainingRoomResult {
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  bool ambiguous = false;
};

ContainingRoomResult findContainingRoom(
    const cr::CreativeWorldLayout& layout, std::size_t levelIndex,
    cr::CreativeWorldLayoutRect footprint,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex) {
  ContainingRoomResult result;
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[index];
    if (room.levelIndex != levelIndex ||
        (buildingIndex < layout.buildings.size() &&
         room.buildingIndex != buildingIndex) ||
        footprint.minimum.x < room.footprint.minimum.x ||
        footprint.maximum.x > room.footprint.maximum.x ||
        footprint.minimum.z < room.footprint.minimum.z ||
        footprint.maximum.z > room.footprint.maximum.z) {
      continue;
    }
    if (result.index != cr::kInvalidCreativeWorldLayoutIndex) {
      result.ambiguous = true;
      return result;
    }
    result.index = index;
  }
  return result;
}

std::size_t nextHigherLevel(const cr::CreativeWorldLayout& layout,
                            std::size_t lowerLevelIndex,
                            std::size_t buildingIndex) noexcept {
  if (lowerLevelIndex >= layout.levels.size()) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  const double lowerElevation = layout.levels[lowerLevelIndex].floorTopLayer;
  std::size_t result = cr::kInvalidCreativeWorldLayoutIndex;
  double resultElevation = std::numeric_limits<double>::infinity();
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& level = layout.levels[index];
    if (level.buildingIndex != buildingIndex ||
        !std::isfinite(level.floorTopLayer) ||
        level.floorTopLayer <= lowerElevation ||
        level.floorTopLayer >= resultElevation) {
      continue;
    }
    result = index;
    resultElevation = level.floorTopLayer;
  }
  return result;
}

cr::CreativeWorldLayoutVerticalDirection
verticalDirection(cr::CreativeTerrainCoord2 start,
                  cr::CreativeTerrainCoord2 end) noexcept {
  const std::int64_t deltaX = static_cast<std::int64_t>(end.x) - start.x;
  const std::int64_t deltaZ = static_cast<std::int64_t>(end.z) - start.z;
  if (std::llabs(deltaX) >= std::llabs(deltaZ)) {
    return deltaX >= 0 ? cr::CreativeWorldLayoutVerticalDirection::PositiveX
                       : cr::CreativeWorldLayoutVerticalDirection::NegativeX;
  }
  return deltaZ >= 0 ? cr::CreativeWorldLayoutVerticalDirection::PositiveZ
                     : cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
}

CreativeEditorWorldLayoutEditReceipt
addVerticalConnectorPoint(CreativeEditorWorldLayoutState& state,
                          cr::CreativeTerrainCoord2 point,
                          const VerticalConnectorToolSpec& spec) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage =
        verticalConnectorMessage(spec, " low end set; choose high end");
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }

  const cr::CreativeTerrainCoord2 start = state.anchor;
  const cr::CreativeWorldLayoutRect footprint = normalizedRect(start, point);
  state.anchorActive = false;
  if (footprint.minimum.x >= footprint.maximum.x ||
      footprint.minimum.z >= footprint.maximum.z) {
    state.statusMessage =
        verticalConnectorMessage(spec, " needs both run and width");
    return {false, false, verticalConnectorReasonCode(spec, "_degenerate")};
  }
  if (state.activeLevelIndex >= state.source.levels.size()) {
    state.statusMessage = verticalConnectorMessage(
        spec, " needs a selected lower building level");
    return {false, false,
            verticalConnectorReasonCode(spec, "_lower_level_missing")};
  }

  const ContainingRoomResult lower =
      findContainingRoom(state.source, state.activeLevelIndex, footprint);
  if (lower.ambiguous || lower.index == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage =
        lower.ambiguous
            ? verticalConnectorMessage(spec,
                                       " footprint crosses room ownership")
            : verticalConnectorMessage(spec, " must fit inside one lower room");
    return {false, false,
            verticalConnectorReasonCode(spec, "_lower_room_invalid")};
  }
  const std::size_t buildingIndex =
      state.source.rooms[lower.index].buildingIndex;
  const std::size_t upperLevelIndex =
      nextHigherLevel(state.source, state.activeLevelIndex, buildingIndex);
  if (upperLevelIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = verticalConnectorMessage(
        spec, " needs an adjacent upper building level");
    return {false, false,
            verticalConnectorReasonCode(spec, "_upper_level_missing")};
  }
  const ContainingRoomResult upper = findContainingRoom(
      state.source, upperLevelIndex, footprint, buildingIndex);
  if (upper.ambiguous || upper.index == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage =
        upper.ambiguous
            ? verticalConnectorMessage(
                  spec, " footprint crosses upper room ownership")
            : verticalConnectorMessage(spec, " must fit inside one upper room");
    return {false, false,
            verticalConnectorReasonCode(spec, "_upper_room_invalid")};
  }

  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = buildingIndex;
  connector.lowerRoomIndex = lower.index;
  connector.upperRoomIndex = upper.index;
  connector.kind = spec.kind;
  connector.direction = verticalDirection(start, point);
  connector.stableKey = "pending_" + std::string(spec.keyPrefix);
  connector.name = std::string(spec.label) + " " +
                   std::to_string(state.source.verticalConnectors.size() + 1U);
  connector.footprint = footprint;
  state.source.verticalConnectors.push_back(std::move(connector));
  const std::size_t connectorIndex =
      state.source.verticalConnectors.size() - 1U;
  const cr::CreativeWorldLayoutVerticalConnectorPlan plan =
      cr::planCreativeWorldLayoutVerticalConnector({}, state.source,
                                                   connectorIndex);
  if (!plan.accepted) {
    state.source.verticalConnectors.pop_back();
    state.statusMessage = std::string(plan.reasonCode);
    return {false, false, std::string(plan.reasonCode)};
  }
  state.source.verticalConnectors.back().stableKey =
      mintWorldLayoutStableKey(state, spec.keyPrefix);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
                     connectorIndex};
  noteWorldLayoutSourceChange(state, verticalConnectorMessage(spec, " added"));
  return {true, true, verticalConnectorReasonCode(spec, "_added")};
}

CreativeEditorWorldLayoutEditReceipt addPlateau(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = mintWorldLayoutStableKey(state, "plateau");
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = point;
  profile.baseHeightCells = 4U;
  profile.radiusCells = 8U;
  profile.amplitudeCells = 1U;
  profile.spacingCells = 4U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  state.source.terrainProfiles.push_back(std::move(profile));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainProfile,
                     state.source.terrainProfiles.size() - 1U};
  noteWorldLayoutSourceChange(state, "plateau added");
  return {true, true, "creative_editor_world_layout_plateau_added"};
}

CreativeEditorWorldLayoutEditReceipt addTerrainPathPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point,
    cr::CreativeTerrainRecipeKind kind) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = kind == cr::CreativeTerrainRecipeKind::Road
                              ? "road start set; choose end"
                              : "ditch start set; choose end";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeTerrainCoord2 start = state.anchor;
  state.anchorActive = false;
  if (start == point) {
    state.statusMessage = "terrain path needs length";
    return {false, false, "creative_editor_world_layout_path_degenerate"};
  }
  const std::uint16_t height =
      kind == cr::CreativeTerrainRecipeKind::Ditch ? 2U : 1U;
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = mintWorldLayoutStableKey(
      state, kind == cr::CreativeTerrainRecipeKind::Road ? "road" : "ditch");
  path.kind = kind;
  path.firstPointIndex = state.source.terrainPathPoints.size();
  path.pointCount = 2U;
  path.elevation = cr::CreativeTerrainPathElevation::Level;
  path.halfWidthCells = 1U;
  path.amplitudeCells = 1U;
  path.paintSurface = true;
  path.material = cr::CreativeTerrainMaterial::Count;
  state.source.terrainPathPoints.push_back({start, height});
  state.source.terrainPathPoints.push_back({point, height});
  state.source.terrainPaths.push_back(std::move(path));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                     state.source.terrainPaths.size() - 1U};
  noteWorldLayoutSourceChange(
      state, kind == cr::CreativeTerrainRecipeKind::Road ? "road added"
                                                         : "ditch added");
  return {true, true, "creative_editor_world_layout_path_added"};
}

CreativeEditorWorldLayoutEditReceipt addBridgePoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "bridge start set; choose opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect footprint =
      normalizedRect(state.anchor, point);
  state.anchorActive = false;
  if (footprint.minimum == footprint.maximum ||
      footprint.minimum.x == footprint.maximum.x ||
      footprint.minimum.z == footprint.maximum.z) {
    state.statusMessage = "bridge needs width and length";
    return {false, false, "creative_editor_world_layout_bridge_degenerate"};
  }
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Bridge;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  object.stableKey = mintWorldLayoutStableKey(state, "bridge");
  object.name = "Bridge " + std::to_string(state.source.objects.size() + 1U);
  object.boundsCells = {
      {static_cast<double>(footprint.minimum.x), 0.0,
       static_cast<double>(footprint.minimum.z)},
      {static_cast<double>(footprint.maximum.x), 0.35,
       static_cast<double>(footprint.maximum.z)},
  };
  object.tags = {"world_layout:object"};
  state.source.objects.push_back(std::move(object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  noteWorldLayoutSourceChange(state, "bridge added");
  return {true, true, "creative_editor_world_layout_bridge_added"};
}

CreativeEditorWorldLayoutEditReceipt addSpawnPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point,
    CreativeEditorWorldLayoutTool tool) {
  cr::CreativeWorldLayoutObject object;
  object.stableKey = mintWorldLayoutStableKey(
      state, tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                 ? "player_spawn"
                 : "npc_spawn");
  object.tags = {"world_layout:object"};
  object.kind = tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                    ? cr::CreativeObjectKind::SpawnPoint
                    : cr::CreativeObjectKind::NpcSpawn;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.name = tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                    ? "Player Spawn"
                    : "NPC Spawn";
  object.pointCells = {static_cast<double>(point.x), 0.0,
                       static_cast<double>(point.z)};
  state.source.objects.push_back(std::move(object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  noteWorldLayoutSourceChange(
      state, tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                 ? "player spawn added"
                 : "NPC spawn added");
  return {true, true, "creative_editor_world_layout_object_added"};
}

}  // namespace

const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept {
  switch (tool) {
    case CreativeEditorWorldLayoutTool::Select:
      return "Select";
    case CreativeEditorWorldLayoutTool::BuildingShell:
      return "Building Shell";
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
    case CreativeEditorWorldLayoutTool::Stair:
      return "Stair";
    case CreativeEditorWorldLayoutTool::Ramp:
      return "Ramp";
    case CreativeEditorWorldLayoutTool::Plateau:
      return "Plateau";
    case CreativeEditorWorldLayoutTool::Road:
      return "Road";
    case CreativeEditorWorldLayoutTool::Ditch:
      return "Ditch";
    case CreativeEditorWorldLayoutTool::Bridge:
      return "Bridge";
    case CreativeEditorWorldLayoutTool::CatalogAsset:
      return "Catalog Asset";
    case CreativeEditorWorldLayoutTool::PlayerSpawn:
      return "Player Spawn";
    case CreativeEditorWorldLayoutTool::NpcSpawn:
      return "NPC Spawn";
    case CreativeEditorWorldLayoutTool::Count:
      break;
  }
  return "Unknown";
}

bool creativeEditorWorldLayoutToolIsVerticalConnector(
    CreativeEditorWorldLayoutTool tool) noexcept {
  return verticalConnectorToolSpec(tool) != nullptr;
}

const char* creativeEditorWorldLayoutPaletteCategoryLabel(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept {
  switch (category) {
    case CreativeEditorWorldLayoutPaletteCategory::Structure:
      return "Structures";
    case CreativeEditorWorldLayoutPaletteCategory::Terrain:
      return "Terrain";
    case CreativeEditorWorldLayoutPaletteCategory::Object:
      return "Objects";
    case CreativeEditorWorldLayoutPaletteCategory::Gameplay:
      return "Gameplay";
    case CreativeEditorWorldLayoutPaletteCategory::Count:
      break;
  }
  return "Unknown";
}

std::span<const CreativeEditorWorldLayoutPaletteEntry>
creativeEditorWorldLayoutPaletteEntries() noexcept {
  return kWorldLayoutPaletteEntries;
}


CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutTool tool) {
  if (tool >= CreativeEditorWorldLayoutTool::Count) {
    return {false, false, "creative_editor_world_layout_tool_invalid"};
  }
  const bool changed = state.tool != tool || state.anchorActive ||
                       state.roomManipulation.active ||
                       state.verticalConnectorManipulation.active ||
                       state.boxManipulation.active ||
                       state.wallManipulation.active ||
                       state.buildingManipulation.active ||
                       state.buildingTransform.active ||
                       state.generatedBuildingDraft.active ||
                       state.buildingTemplatePlacement.active ||
                       state.openingManipulation.active ||
                       state.objectManipulation.active ||
                       state.roomSettingsDraft.active ||
                       state.verticalConnectorSettingsDraft.active ||
                       state.boxSettingsDraft.active ||
                       state.wallSettingsDraft.active ||
                       state.openingSettingsDraft.active ||
                       state.generatedLevelSettingsDraft.active;
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
  if (!detail::finiteWorldLayoutPoint(point)) {
    return {false, false, "creative_editor_world_layout_point_non_finite"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Select) {
    return detail::selectWorldLayoutAtPoint(state, point);
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
  if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell) {
    return addBuildingShellPoint(state, gridPoint);
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
  if (const VerticalConnectorToolSpec* connector =
          verticalConnectorToolSpec(state.tool)) {
    return addVerticalConnectorPoint(state, gridPoint, *connector);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Plateau) {
    return addPlateau(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Road) {
    return addTerrainPathPoint(state, gridPoint,
                               cr::CreativeTerrainRecipeKind::Road);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Ditch) {
    return addTerrainPathPoint(state, gridPoint,
                               cr::CreativeTerrainRecipeKind::Ditch);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Bridge) {
    return addBridgePoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::PlayerSpawn ||
      state.tool == CreativeEditorWorldLayoutTool::NpcSpawn) {
    return addSpawnPoint(state, gridPoint, state.tool);
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
    state.gesturePreviewGridPointValid = false;
    state.gesturePreviewGridPoint = {};
    state.statusMessage = "layout gesture cancelled";
    return {true, false, "creative_editor_world_layout_gesture_cancelled"};
  }
  const bool dragTool =
      state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
      state.tool == CreativeEditorWorldLayoutTool::Room ||
      state.tool == CreativeEditorWorldLayoutTool::Floor ||
      state.tool == CreativeEditorWorldLayoutTool::Wall ||
      creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
      state.tool == CreativeEditorWorldLayoutTool::Road ||
      state.tool == CreativeEditorWorldLayoutTool::Ditch ||
      state.tool == CreativeEditorWorldLayoutTool::Bridge;
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
    if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell) {
      state.statusMessage = "drag building shell to its opposite corner";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Room) {
      state.statusMessage = "drag room to its opposite corner";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Floor) {
      state.statusMessage = "drag floor to its opposite corner";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
      state.statusMessage = "drag partition to its end";
    } else if (const VerticalConnectorToolSpec* connector =
                   verticalConnectorToolSpec(state.tool)) {
      state.statusMessage = verticalConnectorMessage(
          *connector,
          " drag starts at the low end and finishes at the high end");
    } else if (state.tool == CreativeEditorWorldLayoutTool::Road) {
      state.statusMessage = "drag road to its end";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Ditch) {
      state.statusMessage = "drag ditch to its end";
    } else {
      state.statusMessage = "drag bridge to its opposite corner";
    }
    return {true, false, "creative_editor_world_layout_gesture_started"};
  }
  if (!state.anchorActive) {
    return {false, false,
            "creative_editor_world_layout_gesture_not_active"};
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Update) {
    cr::CreativeTerrainCoord2 gridPoint;
    if (!toGridCoord(point, gridPoint)) {
      return {false, false,
              "creative_editor_world_layout_point_out_of_range"};
    }
    const bool changed = !state.gesturePreviewGridPointValid ||
                         state.gesturePreviewGridPoint != gridPoint;
    state.gesturePreviewGridPointValid = true;
    state.gesturePreviewGridPoint = gridPoint;
    return {true, changed,
            changed ? "creative_editor_world_layout_gesture_preview_updated"
                    : "creative_editor_world_layout_gesture_preview_current"};
  }
  CreativeEditorWorldLayoutEditReceipt receipt =
      applyCreativeEditorWorldLayoutPoint(state, point);
  state.gesturePreviewGridPointValid = false;
  state.gesturePreviewGridPoint = {};
  return receipt;
}



}  // namespace iggy3d_creative_app
