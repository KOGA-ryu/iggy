#include "EditorWorldLayout.hpp"

#include "EditorToolDescriptor.hpp"
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

[[nodiscard]] bool rectContains(
    const cr::CreativeWorldLayoutRect& rect,
    cr::CreativeTerrainCoord2 point) noexcept {
  return point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
         point.z >= rect.minimum.z && point.z <= rect.maximum.z;
}

[[nodiscard]] std::size_t findBridgeAttachments(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutRect& footprint,
    std::string& pathKey,
    cr::CreativeTerrainWatercourseCrossingId& crossingId) {
  std::size_t count = 0U;
  for (const cr::CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    if (path.recipe.kind != cr::CreativeTerrainPathKind::River &&
        path.recipe.kind != cr::CreativeTerrainPathKind::Trench) {
      continue;
    }
    for (const cr::CreativeTerrainWatercourseCrossing& crossing :
         path.recipe.watercourse.crossings) {
      const auto point = std::find_if(
          path.recipe.points.begin(), path.recipe.points.end(),
          [&](const cr::CreativeTerrainPathSourcePoint& candidate) {
            return candidate.id == crossing.pointId;
          });
      if (point == path.recipe.points.end() ||
          !rectContains(footprint, point->coord)) {
        continue;
      }
      ++count;
      pathKey = path.stableKey;
      crossingId = crossing.id;
    }
  }
  return count;
}

[[nodiscard]] bool bridgeAttachmentClaimed(
    const cr::CreativeWorldLayout& layout,
    std::string_view pathKey,
    cr::CreativeTerrainWatercourseCrossingId crossingId) {
  return std::any_of(
      layout.objects.begin(), layout.objects.end(),
      [&](const cr::CreativeWorldLayoutObject& object) {
        return object.usesBridgeRecipe &&
               object.bridge.watercoursePathKey == pathKey &&
               object.bridge.crossingId == crossingId;
      });
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



CreativeEditorWorldLayoutEditReceipt beginWorldLayoutDrag(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeTerrainCoord2 point) {
  const CreativeEditorToolDescriptor& descriptor =
      describeCreativeEditorWorldLayoutTool(state.tool);
  if (descriptor.worldLayoutInputProfile !=
      CreativeEditorWorldLayoutInputProfile::Drag) {
    return {false, false, "creative_editor_world_layout_gesture_tool_invalid"};
  }
  state.anchorActive = true;
  state.anchor = point;
  state.statusMessage = descriptor.worldLayoutInteractionPrompt;
  return {true, false, "creative_editor_world_layout_anchor_set"};
}

CreativeEditorWorldLayoutEditReceipt addRoomPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    return beginWorldLayoutDrag(state, point);
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  CreativeEditorWorldLayoutRoomSettings settings;
  settings.footprint = rect;
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
    return beginWorldLayoutDrag(state, point);
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  CreativeEditorWorldLayoutRoomSettings settings;
  settings.footprint = rect;
  return createCreativeEditorWorldLayoutBuildingShell(state, settings);
}

CreativeEditorWorldLayoutEditReceipt addFloorPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    return beginWorldLayoutDrag(state, point);
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
    return beginWorldLayoutDrag(state, point);
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
                          const VerticalConnectorToolSpec& spec,
                          cr::CreativeGridSettings grid) {
  if (!state.anchorActive) {
    return beginWorldLayoutDrag(state, point);
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
      cr::planCreativeWorldLayoutVerticalConnector(grid, state.source,
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
  profile.usesLandformRecipe = true;
  profile.landform.kind = cr::CreativeTerrainLandformKind::Plateau;
  profile.landform.bounds.minimum = {point.x - 4, point.z - 4};
  profile.landform.bounds.widthCells = 8U;
  profile.landform.bounds.depthCells = 8U;
  profile.landform.baseHeightCells = 1U;
  profile.landform.targetHeightCells = 4U;
  profile.landform.terraceCount = 4U;
  profile.landform.edge = cr::CreativeTerrainLandformEdge::Slope;
  profile.landform.edgeWidthCells = 2U;
  profile.landform.material = cr::CreativeTerrainMaterial::Grass;
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
  const CreativeEditorWorldLayoutTool draftTool =
      kind == cr::CreativeTerrainRecipeKind::Road
          ? CreativeEditorWorldLayoutTool::Road
          : CreativeEditorWorldLayoutTool::Ditch;
  CreativeEditorWorldLayoutTerrainPathDraft& draft = state.terrainPathDraft;
  if (!draft.active || draft.tool != draftTool) {
    draft = {};
    draft.active = true;
    draft.tool = draftTool;
    draft.path.recipe.kind =
        kind == cr::CreativeTerrainRecipeKind::Road
            ? cr::CreativeTerrainPathKind::Road
            : cr::CreativeTerrainPathKind::Trench;
    draft.path.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
    draft.path.recipe.crossSection =
        kind == cr::CreativeTerrainRecipeKind::Road
            ? cr::CreativeTerrainPathCrossSection::Flat
            : cr::CreativeTerrainPathCrossSection::Cut;
    draft.path.recipe.paintSurface = true;
    draft.path.recipe.material =
        kind == cr::CreativeTerrainRecipeKind::Road
            ? cr::CreativeTerrainMaterial::Dirt
            : cr::CreativeTerrainMaterial::Sand;
    if (kind != cr::CreativeTerrainRecipeKind::Road) {
      draft.path.recipe.watercourse.bankSlopeCells = 1U;
      draft.path.recipe.watercourse.drainageDirection =
          cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd;
    }
    draft.path.recipe.nextPointId = 1U;
  }
  cr::CreativeTerrainPathSourceRecipe& recipe = draft.path.recipe;
  if (!recipe.points.empty() && recipe.points.back().coord == point) {
    state.statusMessage = "path point already added";
    return {true, false,
            "creative_editor_world_layout_path_point_current"};
  }
  if (recipe.points.size() >= cr::kCreativeTerrainPathPointCapacity ||
      recipe.nextPointId ==
          std::numeric_limits<cr::CreativeTerrainPathSourcePointId>::max()) {
    state.statusMessage = "terrain path point limit reached";
    return {false, false,
            "creative_editor_world_layout_path_point_capacity"};
  }
  const bool road = kind == cr::CreativeTerrainRecipeKind::Road;
  recipe.points.push_back({recipe.nextPointId++, point, 4U, 1U,
                           static_cast<std::uint16_t>(road ? 0U : 1U), 0});
  state.anchorActive = true;
  state.anchor = recipe.points.front().coord;
  state.statusMessage =
      std::string(road ? "road" : "ditch") + " draft: " +
      std::to_string(recipe.points.size()) +
      " points; click to add, Enter or double-click to finish";
  return {true, false,
          "creative_editor_world_layout_path_point_added"};
}

CreativeEditorWorldLayoutEditReceipt finishTerrainPathDraft(
    CreativeEditorWorldLayoutState& state) {
  CreativeEditorWorldLayoutTerrainPathDraft& draft = state.terrainPathDraft;
  if (!draft.active || draft.path.recipe.points.size() < 2U ||
      !cr::isValidCreativeTerrainPathSourceRecipe(draft.path.recipe)) {
    state.statusMessage = "terrain path needs at least two valid points";
    return {false, false,
            "creative_editor_world_layout_path_draft_incomplete"};
  }
  const bool road = draft.tool == CreativeEditorWorldLayoutTool::Road;
  draft.path.stableKey =
      mintWorldLayoutStableKey(state, road ? "road" : "ditch");
  state.source.terrainPaths.push_back(std::move(draft.path));
  state.anchorActive = false;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                     state.source.terrainPaths.size() - 1U};
  noteWorldLayoutSourceChange(state, road ? "road added" : "ditch added");
  return {true, true, "creative_editor_world_layout_path_added"};
}

CreativeEditorWorldLayoutEditReceipt cancelTerrainPathDraft(
    CreativeEditorWorldLayoutState& state) {
  const bool changed = state.terrainPathDraft.active;
  state.terrainPathDraft = {};
  state.anchorActive = false;
  state.gesturePreviewGridPointValid = false;
  state.gesturePreviewGridPoint = {};
  state.statusMessage = "terrain path draft cancelled";
  return {true, changed,
          "creative_editor_world_layout_path_draft_cancelled"};
}

CreativeEditorWorldLayoutEditReceipt addBridgePoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    return beginWorldLayoutDrag(state, point);
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
  std::string pathKey;
  cr::CreativeTerrainWatercourseCrossingId crossingId =
      cr::kInvalidCreativeTerrainWatercourseCrossingId;
  const std::size_t attachmentCount =
      findBridgeAttachments(state.source, footprint, pathKey, crossingId);
  if (attachmentCount > 1U) {
    state.statusMessage = "bridge footprint contains multiple crossings";
    return {false, false,
            "creative_editor_world_layout_bridge_attachment_ambiguous"};
  }
  if (attachmentCount == 1U &&
      bridgeAttachmentClaimed(state.source, pathKey, crossingId)) {
    state.statusMessage = "crossing already has a bridge";
    return {false, false,
            "creative_editor_world_layout_bridge_attachment_occupied"};
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
  if (attachmentCount == 1U) {
    object.usesBridgeRecipe = true;
    object.bridge.watercoursePathKey = std::move(pathKey);
    object.bridge.crossingId = crossingId;
  }
  state.source.objects.push_back(std::move(object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  const bool attached = attachmentCount == 1U;
  noteWorldLayoutSourceChange(
      state, attached ? "bridge attached to crossing" : "bridge added");
  return {true, true,
          attached
              ? "creative_editor_world_layout_bridge_attached"
              : "creative_editor_world_layout_bridge_added"};
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
  const CreativeEditorToolDescriptor& descriptor =
      describeCreativeEditorWorldLayoutTool(tool);
  return descriptor.id == CreativeEditorToolId::Count ? "Unknown"
                                                       : descriptor.name.data();
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

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutTool tool) {
  if (tool >= CreativeEditorWorldLayoutTool::Count) {
    return {false, false, "creative_editor_world_layout_tool_invalid"};
  }
  const bool changed = state.tool != tool || state.anchorActive ||
                       state.terrainPathDraft.active ||
                       state.roomManipulation.active ||
                       state.roomCornerManipulation.active ||
                       state.roomBoundaryManipulation.active ||
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

CreativeEditorWorldLayoutEditReceipt detail::applyWorldLayoutToolPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
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
    return addVerticalConnectorPoint(state, gridPoint, *connector, grid);
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

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  return detail::applyWorldLayoutToolPoint(state, point, {});
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
  if (phase >= CreativeEditorWorldLayoutGesturePhase::Count) {
    return {false, false, "creative_editor_world_layout_gesture_invalid"};
  }
  if (state.terrainPathDraft.active) {
    if (phase == CreativeEditorWorldLayoutGesturePhase::Cancel) {
      return cancelTerrainPathDraft(state);
    }
    if (phase == CreativeEditorWorldLayoutGesturePhase::Commit) {
      return finishTerrainPathDraft(state);
    }
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Cancel) {
    state.anchorActive = false;
    state.gesturePreviewGridPointValid = false;
    state.gesturePreviewGridPoint = {};
    state.statusMessage = "layout gesture cancelled";
    return {true, false, "creative_editor_world_layout_gesture_cancelled"};
  }
  const CreativeEditorToolDescriptor& descriptor =
      describeCreativeEditorWorldLayoutTool(state.tool);
  if (descriptor.worldLayoutInputProfile !=
      CreativeEditorWorldLayoutInputProfile::Drag) {
    return {false, false, "creative_editor_world_layout_gesture_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Begin) {
    cr::CreativeTerrainCoord2 gridPoint;
    if (!toGridCoord(point, gridPoint)) {
      return {false, false, "creative_editor_world_layout_point_out_of_range"};
    }
    clearWorldLayoutInteraction(state);
    CreativeEditorWorldLayoutEditReceipt receipt =
        beginWorldLayoutDrag(state, gridPoint);
    receipt.reasonCode = "creative_editor_world_layout_gesture_started";
    return receipt;
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
      detail::applyWorldLayoutToolPoint(state, point, grid);
  state.gesturePreviewGridPointValid = false;
  state.gesturePreviewGridPoint = {};
  return receipt;
}



}  // namespace iggy3d_creative_app
