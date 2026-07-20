#include "EditorWorldLayoutPlanView.hpp"

#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutPlanHitTest.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <span>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

using DraftingRole = CreativeEditorDraftingRole;
using PlanPrimitive = cr::CreativeWorldLayoutPlanPrimitive;
using PlanRole = cr::CreativeWorldLayoutPlanRole;
using SourceRef = cr::CreativeWorldLayoutPlanSourceRef;
using Table = cr::CreativeWorldLayoutTable;

bool transientCandidateActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.buildingTransform.active ||
         (state.buildingTemplatePlacement.active &&
          state.buildingTemplatePlacement.previewValid);
}

CreativeEditorWorldLayoutPlanViewKey makeKey(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    cr::CreativeGridSettings grid) noexcept {
  CreativeEditorWorldLayoutPlanViewKey key;
  key.sourceEpoch = state.sourceEpoch;
  key.sourceRevision = state.revision;
  key.activeLevelIndex = state.activeLevelIndex;
  key.topographyBuildCount = topography.buildCount;
  key.gridOrigin = grid.origin;
  key.gridSize = grid.size;
  key.gridCellSizeMeters = grid.cellSizeMeters;
  key.topographyVisible = topography.visible;
  key.transientCandidate = transientCandidateActive(state);
  return key;
}

DraftingRole terrainRole(cr::CreativeTerrainRecipeKind kind) noexcept {
  switch (kind) {
    case cr::CreativeTerrainRecipeKind::Hill:
      return DraftingRole::Hill;
    case cr::CreativeTerrainRecipeKind::Valley:
      return DraftingRole::Valley;
    case cr::CreativeTerrainRecipeKind::Crater:
      return DraftingRole::Crater;
    case cr::CreativeTerrainRecipeKind::Ridge:
      return DraftingRole::Ridge;
    case cr::CreativeTerrainRecipeKind::Road:
      return DraftingRole::Road;
    case cr::CreativeTerrainRecipeKind::River:
      return DraftingRole::River;
    case cr::CreativeTerrainRecipeKind::Ditch:
      return DraftingRole::Ditch;
    case cr::CreativeTerrainRecipeKind::RidgeLine:
      return DraftingRole::RidgeLine;
    case cr::CreativeTerrainRecipeKind::Plateau:
      return DraftingRole::Plateau;
    case cr::CreativeTerrainRecipeKind::Count:
      break;
  }
  return DraftingRole::RegionMask;
}

DraftingRole objectRole(const PlanPrimitive& primitive) noexcept {
  if (primitive.objectKind == cr::CreativeObjectKind::Unknown) {
    return DraftingRole::ObjectBounds;
  }
  switch (cr::categoryOf(primitive.objectKind)) {
    case cr::CreativeObjectCategory::Structural:
      return DraftingRole::ObjectArchitecture;
    case cr::CreativeObjectCategory::TerrainOrVolume:
      return DraftingRole::ObjectNature;
    case cr::CreativeObjectCategory::NavigationOrMovement:
      return primitive.objectKind == cr::CreativeObjectKind::CoverPoint
                 ? DraftingRole::ObjectCover
                 : primitive.kind ==
                           cr::CreativeWorldLayoutPlanPrimitiveKind::Point
                       ? DraftingRole::ObjectPoint
                       : DraftingRole::ObjectBounds;
    case cr::CreativeObjectCategory::VisualDressing:
      return primitive.objectKind == cr::CreativeObjectKind::Rock ||
                     primitive.objectKind ==
                         cr::CreativeObjectKind::FoliagePatch
                 ? DraftingRole::ObjectNature
                 : primitive.objectKind == cr::CreativeObjectKind::Crate ||
                           primitive.objectKind ==
                               cr::CreativeObjectKind::Barrel
                       ? DraftingRole::ObjectCover
                       : DraftingRole::ObjectProp;
    case cr::CreativeObjectCategory::Unknown:
    case cr::CreativeObjectCategory::Logic:
    case cr::CreativeObjectCategory::Testing:
    case cr::CreativeObjectCategory::LightSoundOrCamera:
    case cr::CreativeObjectCategory::AuthoringMeta:
    case cr::CreativeObjectCategory::Gameplay:
      return primitive.kind == cr::CreativeWorldLayoutPlanPrimitiveKind::Point
                 ? DraftingRole::ObjectPoint
                 : DraftingRole::ObjectProp;
  }
  return DraftingRole::ObjectBounds;
}

bool sourceMatches(const SourceRef& source, Table table,
                   std::size_t index) noexcept {
  return (source.primaryTable == table && source.primaryIndex == index) ||
         (source.secondaryTable == table && source.secondaryIndex == index);
}

std::size_t openingBuildingIndex(const cr::CreativeWorldLayout& layout,
                                 std::size_t openingIndex) noexcept {
  if (openingIndex >= layout.openings.size()) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  const cr::CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < layout.walls.size()) {
    return layout.walls[opening.wallIndex].buildingIndex;
  }
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      opening.roomIndex < layout.rooms.size()) {
    return layout.rooms[opening.roomIndex].buildingIndex;
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

std::size_t sourceBuildingIndex(const cr::CreativeWorldLayout& layout,
                                Table table, std::size_t index) noexcept {
  switch (table) {
    case Table::Building:
      return index < layout.buildings.size()
                 ? index
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::Level:
      return index < layout.levels.size()
                 ? layout.levels[index].buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::Room:
      return index < layout.rooms.size()
                 ? layout.rooms[index].buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::VerticalConnector:
      return index < layout.verticalConnectors.size()
                 ? layout.verticalConnectors[index].buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::Box:
      return index < layout.boxes.size()
                 ? layout.boxes[index].buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::Wall:
      return index < layout.walls.size()
                 ? layout.walls[index].buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::Opening:
      return openingBuildingIndex(layout, index);
    case Table::None:
    case Table::Object:
    case Table::TerrainProfile:
    case Table::TerrainPath:
    case Table::TerrainPathPoint:
      return cr::kInvalidCreativeWorldLayoutIndex;
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

bool openingUsesWall(const cr::CreativeWorldLayout& layout,
                     const PlanPrimitive& primitive,
                     std::size_t wallIndex) noexcept {
  if (primitive.source.primaryTable != Table::Opening ||
      primitive.source.primaryIndex >= layout.openings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening& opening =
      layout.openings[primitive.source.primaryIndex];
  return opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
         opening.wallIndex == wallIndex;
}

bool selectableTable(Table table) noexcept {
  switch (table) {
    case Table::Building:
    case Table::Level:
    case Table::Room:
    case Table::VerticalConnector:
    case Table::Box:
    case Table::Wall:
    case Table::Opening:
    case Table::Object:
    case Table::TerrainProfile:
    case Table::TerrainPath:
      return true;
    case Table::None:
    case Table::TerrainPathPoint:
      return false;
  }
  return false;
}

std::pair<Table, std::size_t> selectableSource(
    const CreativeEditorWorldLayoutState& state,
    const SourceRef& source) noexcept {
  const std::pair<Table, std::size_t> candidates[] = {
      {source.primaryTable, source.primaryIndex},
      {source.secondaryTable, source.secondaryIndex},
  };
  for (const auto [table, index] : candidates) {
    if (selectableTable(table) &&
        !creativeEditorWorldLayoutSourceStableKey(state, table, index).empty()) {
      return {table, index};
    }
  }
  return {Table::None, cr::kInvalidCreativeWorldLayoutIndex};
}

}  // namespace

bool refreshCreativeEditorWorldLayoutPlanView(
    CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    cr::CreativeGridSettings grid) {
  const CreativeEditorWorldLayoutPlanViewKey key =
      makeKey(state, topography, grid);
  if (cache.valid && cache.key == key && !transientCandidateActive(state)) {
    return false;
  }

  std::span<const cr::CreativeTerrainContourSegment> contours;
  if (topography.visible && topography.cacheValid &&
      topography.plan.contours.accepted) {
    contours = topography.plan.contours.segments;
  }
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  cache.projection = cr::projectCreativeWorldLayoutPlan(
      {&source, grid, state.activeLevelIndex, contours});
  cache.paintOrder.resize(cache.projection.primitives.size());
  std::iota(cache.paintOrder.begin(), cache.paintOrder.end(), 0U);
  std::stable_sort(
      cache.paintOrder.begin(), cache.paintOrder.end(),
      [&cache](std::size_t lhs, std::size_t rhs) {
        const PlanPrimitive& left = cache.projection.primitives[lhs];
        const PlanPrimitive& right = cache.projection.primitives[rhs];
        const std::uint8_t leftOrder =
            left.layer == cr::CreativeWorldLayoutPlanLayer::Context
                ? creativeEditorDraftingStyle(
                      DraftingRole::LowerLevelGhostOverlay)
                      .drawOrder
                : creativeEditorDraftingStyle(
                      creativeEditorWorldLayoutPlanDraftingRole(left))
                      .drawOrder;
        const std::uint8_t rightOrder =
            right.layer == cr::CreativeWorldLayoutPlanLayer::Context
                ? creativeEditorDraftingStyle(
                      DraftingRole::LowerLevelGhostOverlay)
                      .drawOrder
                : creativeEditorDraftingStyle(
                      creativeEditorWorldLayoutPlanDraftingRole(right))
                      .drawOrder;
        return leftOrder < rightOrder;
      });
  cache.key = key;
  cache.valid = true;
  ++cache.buildCount;
  return true;
}

void invalidateCreativeEditorWorldLayoutPlanView(
    CreativeEditorWorldLayoutPlanViewCache& cache) noexcept {
  cache.valid = false;
  cache.key = {};
  cache.projection = {};
  cache.paintOrder.clear();
}

CreativeEditorWorldLayoutPlanHit hitCreativeEditorWorldLayoutPlan(
    const CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutPlanPoint point, double toleranceCells) noexcept {
  CreativeEditorWorldLayoutPlanHit hit;
  if (!cache.valid || !cache.projection.accepted || !std::isfinite(point.x) ||
      !std::isfinite(point.z) || !std::isfinite(toleranceCells) ||
      toleranceCells < 0.0) {
    return hit;
  }

  for (auto ordered = cache.paintOrder.rbegin();
       ordered != cache.paintOrder.rend(); ++ordered) {
    const std::size_t primitiveIndex = *ordered;
    if (primitiveIndex >= cache.projection.primitives.size()) {
      continue;
    }
    const PlanPrimitive& primitive =
        cache.projection.primitives[primitiveIndex];
    if (primitive.layer != cr::CreativeWorldLayoutPlanLayer::Active ||
        creativeEditorWorldLayoutPlanPrimitiveSuppressed(state, layout,
                                                          primitive)) {
      continue;
    }
    const auto [table, sourceIndex] = selectableSource(state, primitive.source);
    if (table == Table::None) {
      continue;
    }
    const auto [offsetX, offsetZ] =
        creativeEditorWorldLayoutPlanPrimitiveOffset(state, layout, primitive);
    const cr::CreativeWorldLayoutPlanHitTestResult geometry =
        cr::hitTestCreativeWorldLayoutPlanPrimitive(
            primitive, {point.x - offsetX, point.z - offsetZ}, toleranceCells);
    ++hit.testedPrimitiveCount;
    if (!geometry.hit) {
      continue;
    }
    hit.hit = true;
    hit.primitiveIndex = primitiveIndex;
    hit.table = table;
    hit.sourceIndex = sourceIndex;
    hit.role = primitive.role;
    hit.distanceCells = geometry.distanceCells;
    return hit;
  }
  return hit;
}

CreativeEditorDraftingRole creativeEditorWorldLayoutPlanDraftingRole(
    const PlanPrimitive& primitive) noexcept {
  switch (primitive.role) {
    case PlanRole::RoomFloor:
      return DraftingRole::RoomFloor;
    case PlanRole::ExteriorWall:
      return DraftingRole::ExteriorWall;
    case PlanRole::InteriorPartition:
      return DraftingRole::InteriorPartition;
    case PlanRole::SharedBoundary:
      return DraftingRole::SharedBoundary;
    case PlanRole::Door:
      return DraftingRole::Door;
    case PlanRole::DoorSwing:
      return DraftingRole::DoorSwing;
    case PlanRole::Window:
      return DraftingRole::Window;
    case PlanRole::Stair:
      return DraftingRole::Stair;
    case PlanRole::Ramp:
      return DraftingRole::Ramp;
    case PlanRole::RoofOutline:
      return DraftingRole::RoofOutline;
    case PlanRole::RoofRidge:
      return DraftingRole::RoofRidge;
    case PlanRole::TerrainProfile:
    case PlanRole::TerrainPath:
      return terrainRole(primitive.terrainKind);
    case PlanRole::Contour:
      return primitive.contourMajor ? DraftingRole::ContourMajor
                                    : DraftingRole::ContourMinor;
    case PlanRole::Object:
      return objectRole(primitive);
    case PlanRole::Bridge:
      return DraftingRole::Bridge;
    case PlanRole::PlayerSpawn:
      return DraftingRole::PlayerSpawn;
    case PlanRole::NpcSpawn:
      return DraftingRole::NpcSpawn;
    case PlanRole::Count:
      break;
  }
  return DraftingRole::ObjectBounds;
}

bool creativeEditorWorldLayoutPlanPrimitiveSelected(
    const CreativeEditorWorldLayoutState& state,
    const PlanPrimitive& primitive) noexcept {
  Table table = Table::None;
  switch (state.selection.kind) {
    case CreativeEditorWorldLayoutSelectionKind::Room:
      table = Table::Room;
      break;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      table = Table::VerticalConnector;
      break;
    case CreativeEditorWorldLayoutSelectionKind::Box:
      table = Table::Box;
      break;
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      table = Table::Wall;
      break;
    case CreativeEditorWorldLayoutSelectionKind::Opening:
      table = Table::Opening;
      break;
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
      table = Table::TerrainProfile;
      break;
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
      table = Table::TerrainPath;
      break;
    case CreativeEditorWorldLayoutSelectionKind::Object:
      table = Table::Object;
      break;
    case CreativeEditorWorldLayoutSelectionKind::None:
    case CreativeEditorWorldLayoutSelectionKind::Level:
    case CreativeEditorWorldLayoutSelectionKind::Building:
      return false;
  }
  return sourceMatches(primitive.source, table, state.selection.index);
}

std::size_t creativeEditorWorldLayoutPlanPrimitiveBuildingIndex(
    const cr::CreativeWorldLayout& layout,
    const PlanPrimitive& primitive) noexcept {
  const std::size_t primary = sourceBuildingIndex(
      layout, primitive.source.primaryTable, primitive.source.primaryIndex);
  return primary != cr::kInvalidCreativeWorldLayoutIndex
             ? primary
             : sourceBuildingIndex(layout, primitive.source.secondaryTable,
                                   primitive.source.secondaryIndex);
}

std::pair<double, double> creativeEditorWorldLayoutPlanPrimitiveOffset(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& layout,
    const PlanPrimitive& primitive) noexcept {
  if (!state.buildingManipulation.active ||
      creativeEditorWorldLayoutPlanPrimitiveBuildingIndex(layout, primitive) !=
          state.buildingManipulation.buildingIndex) {
    return {0.0, 0.0};
  }
  return {static_cast<double>(state.buildingManipulation.previewDeltaXCells),
          static_cast<double>(state.buildingManipulation.previewDeltaZCells)};
}

bool creativeEditorWorldLayoutPlanPrimitiveSuppressed(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& layout,
    const PlanPrimitive& primitive) noexcept {
  if (state.roomManipulation.active &&
      primitive.role == PlanRole::RoomFloor &&
      sourceMatches(primitive.source, Table::Room,
                    state.roomManipulation.target.roomIndex)) {
    return true;
  }
  if (state.boxManipulation.active &&
      primitive.role == PlanRole::RoomFloor &&
      sourceMatches(primitive.source, Table::Box,
                    state.boxManipulation.target.boxIndex)) {
    return true;
  }
  if (state.verticalConnectorManipulation.active &&
      sourceMatches(primitive.source, Table::VerticalConnector,
                    state.verticalConnectorManipulation.target
                        .connectorIndex)) {
    return true;
  }
  if (state.wallManipulation.active &&
      (sourceMatches(primitive.source, Table::Wall,
                     state.wallManipulation.target.wallIndex) ||
       openingUsesWall(layout, primitive,
                       state.wallManipulation.target.wallIndex))) {
    return true;
  }
  if (state.openingManipulation.active &&
      sourceMatches(primitive.source, Table::Opening,
                    state.openingManipulation.target.openingIndex)) {
    return true;
  }
  return state.objectManipulation.active &&
         sourceMatches(primitive.source, Table::Object,
                       state.objectManipulation.objectIndex);
}

}  // namespace iggy3d_creative_app
