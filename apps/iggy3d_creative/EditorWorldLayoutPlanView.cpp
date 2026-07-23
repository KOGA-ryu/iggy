#include "EditorWorldLayoutPlanView.hpp"

#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
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

CreativeEditorWorldLayoutPlanViewKey makeKey(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    cr::CreativeGridSettings grid,
    const CreativeEditorWorldLayoutInspection& inspection) noexcept {
  CreativeEditorWorldLayoutPlanViewKey key;
  key.sourceEpoch = state.sourceEpoch;
  key.sourceRevision = state.revision;
  key.activeLevelIndex = state.activeLevelIndex;
  key.topographyBuildCount = topography.buildCount;
  key.gridOrigin = grid.origin;
  key.gridSize = grid.size;
  key.gridCellSizeMeters = grid.cellSizeMeters;
  key.topographyVisible = topography.visible;
  key.lowerLevelContextVisible = state.planLowerLevelContextVisible;
  key.upperLevelContextVisible = state.planUpperLevelContextVisible;
  key.roofOverheadVisible = state.planRoofOverheadVisible;
  key.inspectionContentRevision = inspection.contentRevision;
  key.inspectionSourceKind = inspection.sourceKind;
  key.previewValidity = inspection.previewValidity;
  key.volatileSource = inspection.volatileSource;
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
    case cr::CreativeTerrainRecipeKind::Terrace:
      return DraftingRole::Plateau;
    case cr::CreativeTerrainRecipeKind::Cliff:
      return DraftingRole::Ridge;
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
    case Table::RoofAperture:
      return index < layout.roofApertures.size() &&
                     layout.roofApertures[index].levelIndex <
                         layout.levels.size()
                 ? layout.levels[layout.roofApertures[index].levelIndex]
                       .buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
    case Table::TopologyEdge:
      return index < layout.topologyEdges.size() &&
                     layout.topologyEdges[index].levelIndex <
                         layout.levels.size()
                 ? layout.levels[layout.topologyEdges[index].levelIndex]
                       .buildingIndex
                 : cr::kInvalidCreativeWorldLayoutIndex;
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
    case Table::RoofAperture:
    case Table::Object:
    case Table::TerrainProfile:
    case Table::TerrainPath:
    case Table::TopologyEdge:
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

Table selectionTable(
    CreativeEditorWorldLayoutSelectionKind kind) noexcept {
  switch (kind) {
    case CreativeEditorWorldLayoutSelectionKind::Building:
      return Table::Building;
    case CreativeEditorWorldLayoutSelectionKind::Level:
      return Table::Level;
    case CreativeEditorWorldLayoutSelectionKind::Room:
      return Table::Room;
    case CreativeEditorWorldLayoutSelectionKind::TopologyEdge:
      return Table::TopologyEdge;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      return Table::VerticalConnector;
    case CreativeEditorWorldLayoutSelectionKind::Box:
      return Table::Box;
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      return Table::Wall;
    case CreativeEditorWorldLayoutSelectionKind::Opening:
      return Table::Opening;
    case CreativeEditorWorldLayoutSelectionKind::RoofAperture:
      return Table::RoofAperture;
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
      return Table::TerrainProfile;
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
      return Table::TerrainPath;
    case CreativeEditorWorldLayoutSelectionKind::Object:
      return Table::Object;
    case CreativeEditorWorldLayoutSelectionKind::None:
      return Table::None;
  }
  return Table::None;
}

}  // namespace

bool refreshCreativeEditorWorldLayoutPlanView(
    CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    cr::CreativeGridSettings grid) {
  const CreativeEditorWorldLayoutInspection inspection =
      inspectCreativeEditorWorldLayout(state);
  const CreativeEditorWorldLayoutPlanViewKey key =
      makeKey(state, topography, grid, inspection);
  if (cache.valid && cache.key == key && !inspection.volatileSource) {
    return false;
  }

  std::span<const cr::CreativeTerrainContourSegment> contours;
  if (topography.visible && topography.cacheValid &&
      topography.plan.analysis.contours.accepted) {
    contours = topography.plan.analysis.contours.segments;
  }
  cr::CreativeWorldLayoutPlanProjectionRequest request;
  request.layout = inspection.source;
  request.grid = grid;
  request.activeLevelIndex = state.activeLevelIndex;
  request.contours = contours;
  request.includeLowerLevelContext = state.planLowerLevelContextVisible;
  request.includeUpperLevelContext = state.planUpperLevelContextVisible;
  request.includeRoofOverhead = state.planRoofOverheadVisible;
  cache.projection = cr::projectCreativeWorldLayoutPlan(request);
  cache.paintOrder.resize(cache.projection.primitives.size());
  std::iota(cache.paintOrder.begin(), cache.paintOrder.end(), 0U);
  std::stable_sort(
      cache.paintOrder.begin(), cache.paintOrder.end(),
      [&cache](std::size_t lhs, std::size_t rhs) {
        const PlanPrimitive& left = cache.projection.primitives[lhs];
        const PlanPrimitive& right = cache.projection.primitives[rhs];
        const auto drawOrder = [](const PlanPrimitive& primitive) {
          switch (primitive.layer) {
            case cr::CreativeWorldLayoutPlanLayer::LowerContext:
              return creativeEditorDraftingStyle(
                         DraftingRole::LowerLevelGhostOverlay)
                  .drawOrder;
            case cr::CreativeWorldLayoutPlanLayer::UpperContext:
              return creativeEditorDraftingStyle(
                         DraftingRole::UpperLevelGhostOverlay)
                  .drawOrder;
            case cr::CreativeWorldLayoutPlanLayer::Active:
            case cr::CreativeWorldLayoutPlanLayer::Overhead:
            case cr::CreativeWorldLayoutPlanLayer::Count:
              return creativeEditorDraftingStyle(
                         creativeEditorWorldLayoutPlanDraftingRole(primitive))
                  .drawOrder;
          }
          return std::uint8_t{0U};
        };
        const std::uint8_t leftOrder = drawOrder(left);
        const std::uint8_t rightOrder = drawOrder(right);
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
  const CreativeEditorWorldLayoutPlanHitStack stack =
      hitCreativeEditorWorldLayoutPlanStack(cache, state, layout, point,
                                           toleranceCells);
  if (stack.count > 0U) {
    return stack.items.front();
  }
  CreativeEditorWorldLayoutPlanHit hit;
  hit.testedPrimitiveCount = stack.testedPrimitiveCount;
  return hit;
}

CreativeEditorWorldLayoutPlanHitStack hitCreativeEditorWorldLayoutPlanStack(
    const CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutPlanPoint point, double toleranceCells) noexcept {
  CreativeEditorWorldLayoutPlanHitStack stack;
  if (!cache.valid || !cache.projection.accepted || !std::isfinite(point.x) ||
      !std::isfinite(point.z) || !std::isfinite(toleranceCells) ||
      toleranceCells < 0.0) {
    return stack;
  }

  for (auto ordered = cache.paintOrder.rbegin();
       ordered != cache.paintOrder.rend(); ++ordered) {
    const std::size_t primitiveIndex = *ordered;
    if (primitiveIndex >= cache.projection.primitives.size()) {
      continue;
    }
    const PlanPrimitive& primitive =
        cache.projection.primitives[primitiveIndex];
    if (creativeEditorWorldLayoutPlanPrimitiveSuppressed(state, layout,
                                                         primitive)) {
      continue;
    }
    const auto [table, sourceIndex] = selectableSource(state, primitive.source);
    const bool activeSource =
        primitive.layer == cr::CreativeWorldLayoutPlanLayer::Active;
    const bool roofApertureSource =
        primitive.layer == cr::CreativeWorldLayoutPlanLayer::Overhead &&
        table == Table::RoofAperture;
    if (table == Table::None || (!activeSource && !roofApertureSource)) {
      continue;
    }
    const auto [offsetX, offsetZ] =
        creativeEditorWorldLayoutPlanPrimitiveOffset(state, layout, primitive);
    const cr::CreativeWorldLayoutPlanHitTestResult geometry =
        cr::hitTestCreativeWorldLayoutPlanPrimitive(
            primitive, {point.x - offsetX, point.z - offsetZ}, toleranceCells);
    ++stack.testedPrimitiveCount;
    if (!geometry.hit) {
      continue;
    }
    ++stack.totalHitPrimitiveCount;
    const bool duplicate = std::any_of(
        stack.items.begin(), stack.items.begin() + stack.count,
        [&](const CreativeEditorWorldLayoutPlanHit& existing) {
          return existing.table == table &&
                 existing.sourceIndex == sourceIndex;
        });
    if (duplicate) {
      continue;
    }
    if (stack.count >= stack.items.size()) {
      stack.truncated = true;
      continue;
    }
    CreativeEditorWorldLayoutPlanHit& hit = stack.items[stack.count++];
    hit.hit = true;
    hit.primitiveIndex = primitiveIndex;
    hit.table = table;
    hit.sourceIndex = sourceIndex;
    hit.sourceLevelIndex = cr::creativeWorldLayoutSourceLevelAtDatum(
        layout, table, sourceIndex, cache.projection.activeFloorTopLayer);
    hit.role = primitive.role;
    hit.distanceCells = geometry.distanceCells;
    hit.testedPrimitiveCount = stack.testedPrimitiveCount;
  }
  return stack;
}

CreativeEditorWorldLayoutPlanHit cycleCreativeEditorWorldLayoutPlanHit(
    const CreativeEditorWorldLayoutPlanHitStack& stack,
    CreativeEditorWorldLayoutSelection currentSelection) noexcept {
  if (stack.count == 0U) {
    CreativeEditorWorldLayoutPlanHit miss;
    miss.testedPrimitiveCount = stack.testedPrimitiveCount;
    return miss;
  }
  const Table table = selectionTable(currentSelection.kind);
  for (std::size_t index = 0U; index < stack.count; ++index) {
    if (stack.items[index].table == table &&
        stack.items[index].sourceIndex == currentSelection.index) {
      return stack.items[(index + 1U) % stack.count];
    }
  }
  return stack.items.front();
}

CreativeEditorWorldLayoutPlanRegionSelection
selectCreativeEditorWorldLayoutPlanRegion(
    const CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutPlanPoint first,
    cr::CreativeWorldLayoutPlanPoint second,
    double toleranceCells,
    std::size_t sourceCapacity) {
  CreativeEditorWorldLayoutPlanRegionSelection selection;
  selection.requested = true;
  selection.mode = second.x >= first.x
                       ? cr::CreativeWorldLayoutPlanRegionMode::Window
                       : cr::CreativeWorldLayoutPlanRegionMode::Crossing;
  if (!cache.valid || !cache.projection.accepted || !std::isfinite(first.x) ||
      !std::isfinite(first.z) || !std::isfinite(second.x) ||
      !std::isfinite(second.z) || !std::isfinite(toleranceCells) ||
      toleranceCells < 0.0 || sourceCapacity == 0U ||
      sourceCapacity > cr::kCreativeSelectionTargetCapacity) {
    selection.reasonCode =
        "creative_editor_world_layout_plan_region_request_invalid";
    return selection;
  }

  selection.sources.reserve(
      std::min(sourceCapacity, cache.projection.primitives.size()));
  for (const std::size_t primitiveIndex : cache.paintOrder) {
    if (primitiveIndex >= cache.projection.primitives.size()) {
      continue;
    }
    const PlanPrimitive& primitive =
        cache.projection.primitives[primitiveIndex];
    if (creativeEditorWorldLayoutPlanPrimitiveSuppressed(state, layout,
                                                         primitive)) {
      continue;
    }
    const auto [table, sourceIndex] = selectableSource(state, primitive.source);
    const bool activeSource =
        primitive.layer == cr::CreativeWorldLayoutPlanLayer::Active;
    const bool roofApertureSource =
        primitive.layer == cr::CreativeWorldLayoutPlanLayer::Overhead &&
        table == Table::RoofAperture;
    if (table == Table::None || (!activeSource && !roofApertureSource)) {
      continue;
    }

    const auto [offsetX, offsetZ] =
        creativeEditorWorldLayoutPlanPrimitiveOffset(state, layout, primitive);
    const cr::CreativeWorldLayoutPlanHitTestResult geometry =
        cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
            primitive, {first.x - offsetX, first.z - offsetZ},
            {second.x - offsetX, second.z - offsetZ}, selection.mode,
            toleranceCells);
    ++selection.testedPrimitiveCount;
    if (geometry.status ==
            cr::CreativeWorldLayoutPlanHitTestStatus::InvalidRequest ||
        geometry.status ==
            cr::CreativeWorldLayoutPlanHitTestStatus::InvalidPrimitive) {
      selection.sources.clear();
      selection.reasonCode =
          "creative_editor_world_layout_plan_region_geometry_invalid";
      return selection;
    }
    if (!geometry.hit) {
      continue;
    }
    ++selection.matchedPrimitiveCount;
    const cr::CreativeWorldLayoutSourceRef source{table, sourceIndex};
    if (std::find(selection.sources.begin(), selection.sources.end(), source) !=
        selection.sources.end()) {
      continue;
    }
    if (selection.sources.size() >= sourceCapacity) {
      selection.sources.clear();
      selection.overflowed = true;
      selection.reasonCode =
          "creative_editor_world_layout_plan_region_source_capacity";
      return selection;
    }
    selection.sources.push_back(source);
  }

  selection.accepted = true;
  selection.reasonCode = selection.sources.empty()
                             ? "creative_editor_world_layout_plan_region_empty"
                             : "creative_editor_world_layout_plan_region_ready";
  return selection;
}

CreativeEditorObjectSelectionPlan planCreativeEditorWorldLayoutObjectSelection(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& currentSelection,
    std::span<const cr::CreativeWorldLayoutSourceRef> sources,
    CreativeEditorSelectionComposition composition,
    std::size_t objectCapacity) {
  CreativeEditorObjectSelectionPlan plan;
  plan.requested = true;
  plan.sourceCount = sources.size();
  if (!document.isValid() ||
      composition >= CreativeEditorSelectionComposition::Count ||
      objectCapacity == 0U ||
      objectCapacity > cr::kCreativeSelectionTargetCapacity ||
      state.generatedRevision != state.revision) {
    plan.reasonCode = "creative_editor_object_selection_request_invalid";
    return plan;
  }
  for (const cr::CreativeWorldLayoutSourceRef source : sources) {
    if (source.table == Table::None ||
        source.index == cr::kInvalidCreativeWorldLayoutIndex ||
        creativeEditorWorldLayoutSourceStableKey(state, source.table,
                                                 source.index)
            .empty()) {
      plan.reasonCode = "creative_editor_object_selection_source_invalid";
      return plan;
    }
  }

  std::vector<cr::CreativeObjectId> sourceObjectIds;
  sourceObjectIds.reserve(std::min(
      objectCapacity, static_cast<std::size_t>(document.objectCount())));
  for (const cr::CreativeObject& object : document.objects()) {
    const bool included = std::any_of(
        sources.begin(), sources.end(),
        [&](cr::CreativeWorldLayoutSourceRef source) {
          return cr::creativeWorldLayoutObjectBelongsToSource(
              state.source, object, source.table, source.index);
        });
    if (!included) {
      continue;
    }
    if (sourceObjectIds.size() >= objectCapacity) {
      plan.overflowed = true;
      plan.reasonCode = "creative_editor_object_selection_source_capacity";
      return plan;
    }
    sourceObjectIds.push_back(object.id);
  }

  if (composition != CreativeEditorSelectionComposition::Replace) {
    plan.objectIds.reserve(objectCapacity);
    for (const cr::TargetRef target :
         cr::selectedTargetList(currentSelection)) {
      if (target.value == cr::kInvalidId) {
        continue;
      }
      const cr::CreativeObjectId objectId =
          static_cast<cr::CreativeObjectId>(target.value);
      if (document.findObject(objectId) == nullptr ||
          std::find(plan.objectIds.begin(), plan.objectIds.end(), objectId) !=
              plan.objectIds.end()) {
        continue;
      }
      if (plan.objectIds.size() >= objectCapacity) {
        plan.overflowed = true;
        plan.objectIds.clear();
        plan.reasonCode = "creative_editor_object_selection_existing_capacity";
        return plan;
      }
      plan.objectIds.push_back(objectId);
    }
  }

  if (composition == CreativeEditorSelectionComposition::Replace) {
    plan.objectIds = std::move(sourceObjectIds);
  } else if (composition == CreativeEditorSelectionComposition::Add) {
    for (const cr::CreativeObjectId objectId : sourceObjectIds) {
      if (std::find(plan.objectIds.begin(), plan.objectIds.end(), objectId) !=
          plan.objectIds.end()) {
        continue;
      }
      if (plan.objectIds.size() >= objectCapacity) {
        plan.overflowed = true;
        plan.objectIds.clear();
        plan.reasonCode = "creative_editor_object_selection_add_capacity";
        return plan;
      }
      plan.objectIds.push_back(objectId);
    }
  } else {
    for (const cr::CreativeObjectId objectId : sourceObjectIds) {
      const auto found =
          std::find(plan.objectIds.begin(), plan.objectIds.end(), objectId);
      if (found != plan.objectIds.end()) {
        plan.objectIds.erase(found);
        continue;
      }
      if (plan.objectIds.size() >= objectCapacity) {
        plan.overflowed = true;
        plan.objectIds.clear();
        plan.reasonCode = "creative_editor_object_selection_toggle_capacity";
        return plan;
      }
      plan.objectIds.push_back(objectId);
    }
  }

  const cr::CreativeObjectId currentPrimary =
      currentSelection.selectedTarget.value == cr::kInvalidId
          ? cr::kInvalidObjectId
          : static_cast<cr::CreativeObjectId>(
                currentSelection.selectedTarget.value);
  if (composition != CreativeEditorSelectionComposition::Replace &&
      std::find(plan.objectIds.begin(), plan.objectIds.end(), currentPrimary) !=
          plan.objectIds.end()) {
    plan.primaryObjectId = currentPrimary;
  } else if (!plan.objectIds.empty()) {
    plan.primaryObjectId = composition == CreativeEditorSelectionComposition::Toggle
                               ? plan.objectIds.back()
                               : plan.objectIds.front();
  }
  plan.accepted = true;
  plan.reasonCode = plan.objectIds.empty()
                        ? "creative_editor_object_selection_empty"
                        : "creative_editor_object_selection_ready";
  return plan;
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
    case PlanRole::OpeningFacing:
      return DraftingRole::OpeningFacing;
    case PlanRole::Window:
      return DraftingRole::Window;
    case PlanRole::WindowShutter:
      return DraftingRole::WindowShutter;
    case PlanRole::Stair:
      return DraftingRole::Stair;
    case PlanRole::Ramp:
      return DraftingRole::Ramp;
    case PlanRole::RoofOutline:
      return DraftingRole::RoofOutline;
    case PlanRole::RoofRidge:
      return DraftingRole::RoofRidge;
    case PlanRole::RoofSkylight:
      return DraftingRole::RoofSkylight;
    case PlanRole::RoofClearance:
      return DraftingRole::RoofClearance;
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
  const Table table = selectionTable(state.selection.kind);
  if (table == Table::None || table == Table::Level ||
      table == Table::Building) {
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
