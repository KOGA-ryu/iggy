#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"

#include "app/iggy3d/creative/recipes/BridgeRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainGradeAdapters.hpp"
#include "app/iggy3d/creative/recipes/WatercourseRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative::world_layout_compile {

void setStatus(CreativeWorldLayoutReceipt& receipt,
               CreativeWorldLayoutStatus status,
               std::string_view reasonCode,
               bool accepted) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

[[nodiscard]] bool validDocument(const CreativeDocument& document) noexcept {
  return document.isValid() && document.id() != kInvalidDocumentId &&
         document.nextObjectId() != kInvalidObjectId &&
         document.terrainField().validateInvariants() &&
         document.terrainHeightField().validateInvariants() &&
         document.terrainMaterialField().validateInvariants();
}

[[nodiscard]] bool validStableKey(std::string_view key) noexcept {
  if (key.empty() || key.size() > 128U) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    const unsigned char character = static_cast<unsigned char>(value);
    return std::isalnum(character) != 0 || value == '_' || value == '-' ||
           value == '.';
  });
}

[[nodiscard]] bool validTerrainOwnership(
    CreativeWorldLayoutTerrainOwnership ownership) noexcept {
  return ownership == CreativeWorldLayoutTerrainOwnership::PreserveExisting ||
         ownership == CreativeWorldLayoutTerrainOwnership::ReplaceAll;
}

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] bool worldCoordinate(double origin,
                                   double cellSize,
                                   long double coordinate,
                                   double& output) noexcept {
  const long double value = static_cast<long double>(origin) +
                            static_cast<long double>(cellSize) * coordinate;
  if (!std::isfinite(value) ||
      value < -std::numeric_limits<double>::max() ||
      value > std::numeric_limits<double>::max()) {
    return false;
  }
  output = static_cast<double>(value);
  return std::isfinite(output);
}

[[nodiscard]] bool layoutBounds(const CreativeGridSettings& grid,
                                CreativeWorldLayoutRect rect,
                                double baseLayer,
                                std::uint16_t heightCells,
                                CreativeBounds& output) noexcept {
  if (!validRect(rect) || heightCells == 0U ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return false;
  }
  return worldCoordinate(grid.origin.x, grid.cellSizeMeters, rect.minimum.x,
                         output.min.x) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, rect.minimum.z,
                         output.min.z) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters, baseLayer,
                         output.min.y) &&
         worldCoordinate(grid.origin.x, grid.cellSizeMeters, rect.maximum.x,
                         output.max.x) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, rect.maximum.z,
                         output.max.z) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                         static_cast<long double>(baseLayer) + heightCells,
                         output.max.y);
}

[[nodiscard]] bool layoutPoint(const CreativeGridSettings& grid,
                               CreativeTerrainCoord2 coord,
                               double layer,
                               CreativeVec3& output) noexcept {
  return worldCoordinate(grid.origin.x, grid.cellSizeMeters, coord.x,
                         output.x) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters, layer,
                         output.y) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, coord.z,
                         output.z);
}

[[nodiscard]] bool layoutPoint(const CreativeGridSettings& grid,
                               CreativeVec3 cells,
                               CreativeVec3& output) noexcept {
  return worldCoordinate(grid.origin.x, grid.cellSizeMeters, cells.x,
                         output.x) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters, cells.y,
                         output.y) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, cells.z,
                         output.z);
}

[[nodiscard]] bool layoutBounds(const CreativeGridSettings& grid,
                                CreativeBounds cells,
                                CreativeBounds& output) noexcept {
  return layoutPoint(grid, cells.min, output.min) &&
         layoutPoint(grid, cells.max, output.max);
}

[[nodiscard]] std::string childKey(std::string_view buildingKey,
                                   std::string_view localKey) {
  return std::string(buildingKey) + "." + std::string(localKey);
}

[[nodiscard]] bool registerKey(std::unordered_set<std::string>& keys,
                               std::string key,
                               CreativeWorldLayoutTable table,
                               std::size_t index,
                               CreativeWorldLayoutReceipt& receipt) {
  if (!validStableKey(key)) {
    receipt.failedTable = table;
    receipt.failedIndex = index;
    setStatus(receipt, CreativeWorldLayoutStatus::InvalidSymbol,
              "creative_world_layout_stable_key_invalid");
    return false;
  }
  if (!keys.insert(std::move(key)).second) {
    receipt.failedTable = table;
    receipt.failedIndex = index;
    setStatus(receipt, CreativeWorldLayoutStatus::DuplicateStableKey,
              "creative_world_layout_stable_key_duplicate");
    return false;
  }
  return true;
}

[[nodiscard]] bool hasTag(std::span<const std::string> tags,
                          std::string_view tag) noexcept {
  return std::any_of(tags.begin(), tags.end(),
                     [tag](const std::string& value) { return value == tag; });
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!tag.empty() && !hasTag(tags, tag)) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] bool collectObjectRemovalOrder(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> removeIds,
    std::vector<CreativeObjectId>& output) {
  std::unordered_set<CreativeObjectId> remaining(removeIds.begin(),
                                                  removeIds.end());
  if (remaining.size() != removeIds.size()) {
    return false;
  }
  std::vector<CreativeObjectId> orderedIds(removeIds.begin(), removeIds.end());
  if (std::any_of(orderedIds.begin(), orderedIds.end(),
                  [&](CreativeObjectId id) {
                    return document.findObject(id) == nullptr;
                  })) {
    return false;
  }
  output.reserve(remaining.size());
  while (!remaining.empty()) {
    const auto leaf = std::find_if(
        orderedIds.begin(), orderedIds.end(), [&](CreativeObjectId candidate) {
          if (!remaining.contains(candidate)) {
            return false;
          }
          return std::none_of(
              document.objects().begin(), document.objects().end(),
              [&](const CreativeObject& object) {
                return remaining.contains(object.id) &&
                       object.parentId == candidate;
              });
        });
    if (leaf == orderedIds.end()) {
      output.clear();
      return false;
    }
    output.push_back(*leaf);
    remaining.erase(*leaf);
  }
  return true;
}

[[nodiscard]] bool clearTerrain(CreativeDocument& document) {
  std::vector<CreativeTerrainControlEdit> terrain;
  terrain.reserve(document.terrainField().controls().size());
  for (const CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    terrain.push_back({CreativeTerrainEditKind::Remove, control});
  }
  if (!terrain.empty() &&
      !document.applyTerrainControlEdits(terrain).accepted) {
    return false;
  }

  std::vector<CreativeTerrainMaterialEdit> materials;
  materials.reserve(document.terrainMaterialField().overrides().size());
  for (const CreativeTerrainMaterialOverride& value :
       document.terrainMaterialField().overrides()) {
    materials.push_back(
        {CreativeTerrainMaterialEditKind::Clear, value.coord, value.material});
  }
  return materials.empty() ||
         document.applyTerrainMaterialEdits(materials).accepted;
}

[[nodiscard]] std::vector<CreativeTerrainControlEdit> terrainDiff(
    std::span<const CreativeTerrainControlPoint> before,
    std::span<const CreativeTerrainControlPoint> after) {
  std::vector<CreativeTerrainControlEdit> edits;
  std::size_t beforeIndex = 0U;
  std::size_t afterIndex = 0U;
  while (beforeIndex < before.size() || afterIndex < after.size()) {
    if (afterIndex >= after.size() ||
        (beforeIndex < before.size() &&
         coordLess(before[beforeIndex].coord, after[afterIndex].coord))) {
      edits.push_back(
          {CreativeTerrainEditKind::Remove, before[beforeIndex++]});
      continue;
    }
    if (beforeIndex >= before.size() ||
        coordLess(after[afterIndex].coord, before[beforeIndex].coord)) {
      edits.push_back(
          {CreativeTerrainEditKind::Upsert, after[afterIndex++]});
      continue;
    }
    if (!(before[beforeIndex] == after[afterIndex])) {
      edits.push_back(
          {CreativeTerrainEditKind::Upsert, after[afterIndex]});
    }
    ++beforeIndex;
    ++afterIndex;
  }
  return edits;
}

[[nodiscard]] std::vector<CreativeTerrainMaterialEdit> materialDiff(
    std::span<const CreativeTerrainMaterialOverride> before,
    std::span<const CreativeTerrainMaterialOverride> after) {
  std::vector<CreativeTerrainMaterialEdit> edits;
  std::size_t beforeIndex = 0U;
  std::size_t afterIndex = 0U;
  while (beforeIndex < before.size() || afterIndex < after.size()) {
    if (afterIndex >= after.size() ||
        (beforeIndex < before.size() &&
         coordLess(before[beforeIndex].coord, after[afterIndex].coord))) {
      edits.push_back({CreativeTerrainMaterialEditKind::Clear,
                       before[beforeIndex].coord,
                       before[beforeIndex].material});
      ++beforeIndex;
      continue;
    }
    if (beforeIndex >= before.size() ||
        coordLess(after[afterIndex].coord, before[beforeIndex].coord)) {
      edits.push_back(makeCreativeTerrainMaterialWeightEdit(
          after[afterIndex].coord, after[afterIndex].weights));
      ++afterIndex;
      continue;
    }
    if (!(before[beforeIndex] == after[afterIndex])) {
      edits.push_back(makeCreativeTerrainMaterialWeightEdit(
          after[afterIndex].coord, after[afterIndex].weights));
    }
    ++beforeIndex;
    ++afterIndex;
  }
  return edits;
}

[[nodiscard]] bool applyTerrainOperationMutation(
    CreativeDocument& staged,
    CreativeTerrainOperationMutationRequest request,
    CreativeWorldLayoutCompileResult& result) {
  const CreativeTerrainOperationMutationReceipt receipt =
      staged.applyTerrainOperationMutation(request);
  if (!receipt.accepted) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
              receipt.reasonCode);
    return false;
  }
  if (receipt.changed) {
    result.plan.terrainOperationMutations.push_back(std::move(request));
  }
  return true;
}

[[nodiscard]] bool reconcileWorldLayoutTerrainOperations(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutCompileResult& result,
    CreativeDocument& staged) {
  const std::string pathPrefix =
      layout.stableKey + std::string{"/terrain_path/"};
  const std::string landformPrefix =
      layout.stableKey + std::string{"/terrain_landform/"};
  const std::string bridgeApproachPrefix =
      layout.stableKey + std::string{"/bridge_approach/"};
  const auto managed = [&](const CreativeTerrainOperation& operation) {
    return operation.owner == CreativeTerrainOperationOwner::WorldLayout &&
           (operation.sourceKey.starts_with(pathPrefix) ||
            operation.sourceKey.starts_with(landformPrefix) ||
            operation.sourceKey.starts_with(bridgeApproachPrefix));
  };
  std::map<std::string, CreativeTerrainOperationId, std::less<>> existing;
  for (const CreativeTerrainOperation& operation :
       staged.terrainOperationStack().operations) {
    if (!managed(operation)) {
      continue;
    }
    if (!existing.emplace(operation.sourceKey, operation.id).second) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
                "creative_world_layout_terrain_operation_source_duplicate");
      return false;
    }
  }

  std::unordered_set<std::string> desiredKeys;
  desiredKeys.reserve(layout.terrainProfiles.size() +
                      layout.terrainPaths.size() +
                      layout.objects.size() * 2U);
  for (const CreativeWorldLayoutTerrainProfile& profile :
       layout.terrainProfiles) {
    if (profile.usesLandformRecipe) {
      desiredKeys.insert(creativeWorldLayoutTerrainLandformSourceKey(
          layout.stableKey, profile.stableKey));
    }
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    desiredKeys.insert(creativeWorldLayoutTerrainPathSourceKey(
        layout.stableKey, path.stableKey));
  }
  for (const CreativeWorldLayoutObject& object : layout.objects) {
    if (!object.usesBridgeRecipe) {
      continue;
    }
    const std::string base = bridgeApproachPrefix + object.stableKey;
    desiredKeys.insert(base + "/left");
    desiredKeys.insert(base + "/right");
  }

  const bool replaceAll =
      layout.terrainOwnership == CreativeWorldLayoutTerrainOwnership::ReplaceAll;
  std::vector<CreativeTerrainOperationId> removeIds;
  removeIds.reserve(staged.terrainOperationStack().operations.size());
  for (const CreativeTerrainOperation& operation :
       staged.terrainOperationStack().operations) {
    if ((replaceAll && (!managed(operation) ||
                        !desiredKeys.contains(operation.sourceKey))) ||
        (!replaceAll && managed(operation) &&
         !desiredKeys.contains(operation.sourceKey))) {
      removeIds.push_back(operation.id);
    }
  }
  for (const CreativeTerrainOperationId id : removeIds) {
    CreativeTerrainOperationMutationRequest remove;
    remove.kind = CreativeTerrainOperationMutationKind::Remove;
    remove.operationId = id;
    if (!applyTerrainOperationMutation(staged, std::move(remove), result)) {
      return false;
    }
  }

  std::vector<CreativeTerrainOperationId> desiredIds;
  desiredIds.reserve(layout.terrainProfiles.size() +
                     layout.terrainPaths.size() + layout.objects.size() * 2U);
  const auto applyDesired = [&](CreativeTerrainOperationMutationRequest request,
                                CreativeWorldLayoutTable table,
                                std::size_t index) {
    const std::string sourceKey = request.sourceKey;
    const auto found = std::find_if(
        staged.terrainOperationStack().operations.begin(),
        staged.terrainOperationStack().operations.end(),
        [&](const CreativeTerrainOperation& operation) {
          return operation.owner ==
                     CreativeTerrainOperationOwner::WorldLayout &&
                 operation.sourceKey == sourceKey;
        });
    request.kind = found == staged.terrainOperationStack().operations.end()
                       ? CreativeTerrainOperationMutationKind::Add
                       : CreativeTerrainOperationMutationKind::Update;
    request.operationId =
        found == staged.terrainOperationStack().operations.end()
            ? kInvalidCreativeTerrainOperationId
            : found->id;
    if (!applyTerrainOperationMutation(staged, std::move(request), result)) {
      result.receipt.failedTable = table;
      result.receipt.failedIndex = index;
      return false;
    }
    const auto resolved = std::find_if(
        staged.terrainOperationStack().operations.begin(),
        staged.terrainOperationStack().operations.end(),
        [&](const CreativeTerrainOperation& operation) {
          return operation.owner ==
                     CreativeTerrainOperationOwner::WorldLayout &&
                 operation.sourceKey == sourceKey;
        });
    if (resolved == staged.terrainOperationStack().operations.end()) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
                "creative_world_layout_terrain_operation_missing");
      return false;
    }
    desiredIds.push_back(resolved->id);
    return true;
  };

  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& profile =
        layout.terrainProfiles[index];
    if (!profile.usesLandformRecipe) {
      continue;
    }
    CreativeTerrainOperationMutationRequest request;
    request.owner = CreativeTerrainOperationOwner::WorldLayout;
    request.sourceKey = creativeWorldLayoutTerrainLandformSourceKey(
        layout.stableKey, profile.stableKey);
    request.operationKind = CreativeTerrainOperationKind::Landform;
    request.landform = profile.landform;
    request.enabled = true;
    if (!applyDesired(std::move(request),
                      CreativeWorldLayoutTable::TerrainProfile, index)) {
      return false;
    }
  }

  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& path = layout.terrainPaths[index];
    CreativeTerrainOperationMutationRequest request;
    request.owner = CreativeTerrainOperationOwner::WorldLayout;
    request.sourceKey = creativeWorldLayoutTerrainPathSourceKey(
        layout.stableKey, path.stableKey);
    request.operationKind = CreativeTerrainOperationKind::Path;
    request.path = path.recipe;
    request.enabled = true;
    if (!applyDesired(std::move(request),
                      CreativeWorldLayoutTable::TerrainPath, index)) {
      return false;
    }
  }

  result.plan.watercoursePlans.clear();
  result.plan.bridgePlans.clear();
  result.receipt.watercourseCount = 0U;
  result.receipt.watercourseCrossingCount = 0U;
  result.receipt.bridgeRecipeCount = 0U;
  result.receipt.bridgeGeneratedObjectCount = 0U;
  result.receipt.bridgeApproachGradeCount = 0U;
  const CreativeGridSettings& grid = staged.gridSettings();
  CreativeDocument crossingSource = staged;
  std::vector<CreativeTerrainOperationId> crossingGradeIds;
  for (const CreativeTerrainOperation& operation :
       crossingSource.terrainOperationStack().operations) {
    if (operation.owner == CreativeTerrainOperationOwner::WorldLayout &&
        operation.sourceKey.starts_with(bridgeApproachPrefix)) {
      crossingGradeIds.push_back(operation.id);
    }
  }
  for (const CreativeTerrainOperationId operationId : crossingGradeIds) {
    CreativeTerrainOperationMutationRequest remove;
    remove.kind = CreativeTerrainOperationMutationKind::Remove;
    remove.operationId = operationId;
    if (!crossingSource.applyTerrainOperationMutation(remove).accepted) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
                "creative_world_layout_bridge_source_terrain_invalid");
      return false;
    }
  }
  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    if (symbol.recipe.kind != CreativeTerrainPathKind::River &&
        symbol.recipe.kind != CreativeTerrainPathKind::Trench) {
      continue;
    }
    CreativeWatercourseRecipeRequest watercourse;
    watercourse.instanceKey = symbol.stableKey;
    watercourse.name = symbol.stableKey;
    watercourse.grid = grid;
    watercourse.source = symbol.recipe;
    CreativeWatercoursePlanResult planned = planCreativeWatercourse(
        crossingSource.terrainHeightField(), watercourse);
    if (!planned.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = planned.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_watercourse_rejected");
      return false;
    }
    ++result.receipt.watercourseCount;
    result.receipt.watercourseCrossingCount += planned.receipt.crossingCount;
    result.plan.watercoursePlans.push_back(std::move(planned.plan));
  }

  for (std::size_t index = 0U; index < layout.objects.size(); ++index) {
    const CreativeWorldLayoutObject& symbol = layout.objects[index];
    if (!symbol.usesBridgeRecipe) {
      continue;
    }
    const auto watercourse = std::find_if(
        result.plan.watercoursePlans.begin(),
        result.plan.watercoursePlans.end(),
        [&](const CreativeWatercoursePlan& plan) {
          return plan.instanceKey == symbol.bridge.watercoursePathKey;
        });
    if (watercourse == result.plan.watercoursePlans.end()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_bridge_watercourse_missing");
      return false;
    }
    const auto crossing = std::find_if(
        watercourse->crossings.begin(), watercourse->crossings.end(),
        [&](const CreativeWatercourseCrossingFrame& frame) {
          return frame.id == symbol.bridge.crossingId;
        });
    if (crossing == watercourse->crossings.end()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_bridge_crossing_missing");
      return false;
    }
    CreativeBridgeRecipeRequest bridge;
    bridge.instanceKey = symbol.stableKey;
    bridge.name = symbol.name;
    bridge.gridCellSizeMeters = grid.cellSizeMeters;
    bridge.source = symbol.bridge;
    bridge.crossing = *crossing;
    bridge.tags = symbol.tags;
    bridge.tags.push_back(creativeWorldLayoutTag(layout.stableKey));
    bridge.tags.push_back(creativeWorldLayoutProvenanceTag(
        layout, CreativeWorldLayoutTable::Object, index));
    CreativeBridgeRecipeResult planned = planCreativeBridge(bridge);
    if (!planned.receipt.accepted || planned.approachGradeCount != 2U) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = planned.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_bridge_recipe_rejected");
      return false;
    }
    const std::string base = bridgeApproachPrefix + symbol.stableKey;
    for (std::size_t side = 0U; side < planned.approachGradeCount; ++side) {
      CreativeTerrainOperationMutationRequest grade;
      grade.owner = CreativeTerrainOperationOwner::WorldLayout;
      grade.sourceKey = base + (side == 0U ? "/left" : "/right");
      grade.operationKind = CreativeTerrainOperationKind::Grade;
      grade.grade = planned.approachGrades[side];
      grade.enabled = true;
      if (!applyDesired(std::move(grade), CreativeWorldLayoutTable::Object,
                        index)) {
        return false;
      }
    }
    ++result.receipt.bridgeRecipeCount;
    result.receipt.bridgeGeneratedObjectCount +=
        planned.receipt.generatedObjectCount;
    result.receipt.bridgeApproachGradeCount +=
        planned.receipt.approachGradeCount;
    result.plan.bridgePlans.push_back(std::move(planned));
  }

  std::vector<CreativeTerrainOperationId> finalOrder;
  finalOrder.reserve(staged.terrainOperationStack().operations.size());
  for (const CreativeTerrainOperation& operation :
       staged.terrainOperationStack().operations) {
    if (!managed(operation)) {
      finalOrder.push_back(operation.id);
    }
  }
  finalOrder.insert(finalOrder.end(), desiredIds.begin(), desiredIds.end());
  for (std::size_t targetIndex = 0U; targetIndex < finalOrder.size();
       ++targetIndex) {
    const std::size_t currentIndex = findCreativeTerrainOperationIndex(
        staged.terrainOperationStack(), finalOrder[targetIndex]);
    if (currentIndex == targetIndex) {
      continue;
    }
    CreativeTerrainOperationMutationRequest move;
    move.kind = CreativeTerrainOperationMutationKind::Move;
    move.operationId = finalOrder[targetIndex];
    move.targetIndex = targetIndex;
    if (!applyTerrainOperationMutation(staged, std::move(move), result)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] CreativeTerrainPathEndpointJoin endpointJoinAt(
    const CreativeTerrainPathSourceRecipe& recipe,
    std::size_t pointIndex) noexcept {
  if (pointIndex == 0U) {
    return recipe.startJoin;
  }
  return pointIndex + 1U == recipe.points.size()
             ? recipe.endJoin
             : CreativeTerrainPathEndpointJoin::Open;
}

struct TerrainPathEndpointRef {
  std::size_t pathIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t pointIndex = kInvalidCreativeWorldLayoutIndex;
};

[[nodiscard]] CreativeTerrainGradeRecipe reverseGrade(
    CreativeTerrainGradeRecipe recipe) noexcept {
  std::swap(recipe.start, recipe.end);
  std::swap(recipe.startHeightCells, recipe.endHeightCells);
  recipe.crossSlopePermille = -recipe.crossSlopePermille;
  return recipe;
}

[[nodiscard]] bool exactGradeEndpointSegment(
    const CreativeTerrainPathSourceRecipe& path,
    std::size_t pointIndex,
    CreativeTerrainGradeRecipe& output) noexcept {
  if (path.kind != CreativeTerrainPathKind::Road ||
      path.elevation != CreativeTerrainPathElevation::Grade ||
      path.curve != CreativeTerrainPathCurvePolicy::Linear ||
      path.crossSection != CreativeTerrainPathCrossSection::Flat ||
      path.points.size() < 2U ||
      (pointIndex != 0U && pointIndex + 1U != path.points.size())) {
    return false;
  }
  const std::size_t segmentIndex =
      pointIndex == 0U ? 0U : path.points.size() - 2U;
  const CreativeTerrainPathSourcePoint& from = path.points[segmentIndex];
  const CreativeTerrainPathSourcePoint& to = path.points[segmentIndex + 1U];
  if (from.halfWidthCells != to.halfWidthCells ||
      from.bankPermille != to.bankPermille || from.amplitudeCells != 0U ||
      to.amplitudeCells != 0U) {
    return false;
  }
  CreativeTerrainGradePathSegmentRequest request;
  request.start = from.coord;
  request.end = to.coord;
  request.startHeightCells = from.heightCells;
  request.endHeightCells = to.heightCells;
  request.halfWidthCells = from.halfWidthCells;
  request.crossSlopePermille = from.bankPermille;
  request.falloffCells = path.falloffCells;
  const CreativeTerrainGradeAdapterPlan plan =
      planCreativeTerrainGradePathSegment(request);
  if (!plan.accepted || plan.recipeCount != 1U) {
    return false;
  }
  output = pointIndex == 0U ? reverseGrade(plan.recipes[0])
                            : plan.recipes[0];
  return true;
}

[[nodiscard]] bool pointOnPerimeter(CreativeTerrainGradeRect rect,
                                    CreativeTerrainCoord2 point) noexcept {
  return point.x >= rect.minimum.x && point.x < rect.maximum.x &&
         point.z >= rect.minimum.z && point.z < rect.maximum.z &&
         (point.x == rect.minimum.x || point.x == rect.maximum.x - 1 ||
          point.z == rect.minimum.z || point.z == rect.maximum.z - 1);
}

[[nodiscard]] bool exactGridCoordinate(double value,
                                       std::int32_t& output) noexcept {
  if (!std::isfinite(value)) {
    return false;
  }
  const double rounded = std::round(value);
  if (std::fabs(value - rounded) > 1.0e-9 ||
      rounded < std::numeric_limits<std::int32_t>::min() ||
      rounded > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(rounded);
  return true;
}

[[nodiscard]] bool bridgeGradeRect(const CreativeWorldLayoutObject& object,
                                   CreativeTerrainGradeRect& output) noexcept {
  return object.kind == CreativeObjectKind::Bridge &&
         object.mode == CreativeObjectLibraryPlacementMode::Bounds &&
         exactGridCoordinate(object.boundsCells.min.x, output.minimum.x) &&
         exactGridCoordinate(object.boundsCells.min.z, output.minimum.z) &&
         exactGridCoordinate(object.boundsCells.max.x, output.maximum.x) &&
         exactGridCoordinate(object.boundsCells.max.z, output.maximum.z) &&
         output.minimum.x < output.maximum.x &&
         output.minimum.z < output.maximum.z;
}

void rejectPathNetwork(CreativeWorldLayoutCompileResult& result,
                       std::size_t pathIndex,
                       std::string_view reasonCode) {
  result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
  result.receipt.failedIndex = pathIndex;
  setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
            reasonCode);
}

[[nodiscard]] bool validateBuildingPadEndpoint(
    const CreativeWorldLayout& layout,
    TerrainPathEndpointRef endpoint,
    CreativeWorldLayoutCompileResult& result) {
  const CreativeTerrainPathSourceRecipe& path =
      layout.terrainPaths[endpoint.pathIndex].recipe;
  CreativeTerrainGradeRecipe pathGrade;
  if (!exactGradeEndpointSegment(path, endpoint.pointIndex, pathGrade)) {
    rejectPathNetwork(result, endpoint.pathIndex,
                      "creative_world_layout_path_building_pad_grade_invalid");
    return false;
  }

  std::size_t matchingBuildingCount = 0U;
  for (const CreativeWorldLayoutBuilding& building : layout.buildings) {
    CreativeTerrainGradeBuildingPadApproachRequest request;
    request.padFootprint = {building.rootFootprint.minimum,
                            building.rootFootprint.maximum};
    request.terrainEndpoint = pathGrade.start;
    request.padEndpoint = pathGrade.end;
    request.terrainHeightCells = pathGrade.startHeightCells;
    request.padHeightCells = pathGrade.endHeightCells;
    request.halfWidthCells = pathGrade.halfWidthCells;
    request.crossSlopePermille = pathGrade.crossSlopePermille;
    request.falloffCells = pathGrade.falloffCells;
    const CreativeTerrainGradeAdapterPlan plan =
        planCreativeTerrainGradeBuildingPadApproach(request);
    if (plan.accepted && plan.recipeCount == 1U &&
        plan.recipes[0] == pathGrade) {
      ++matchingBuildingCount;
    }
  }
  if (matchingBuildingCount != 1U) {
    rejectPathNetwork(result, endpoint.pathIndex,
                      "creative_world_layout_path_building_pad_unmatched");
    return false;
  }
  return true;
}

[[nodiscard]] bool validateBridgeApproachPair(
    const CreativeWorldLayout& layout,
    std::size_t objectIndex,
    std::span<const TerrainPathEndpointRef> endpoints,
    CreativeWorldLayoutCompileResult& result) {
  if (endpoints.size() != 2U) {
    const std::size_t failedPath = endpoints.empty()
                                       ? kInvalidCreativeWorldLayoutIndex
                                       : endpoints.front().pathIndex;
    rejectPathNetwork(result, failedPath,
                      "creative_world_layout_bridge_approach_pair_required");
    return false;
  }
  CreativeTerrainGradeRect footprint;
  if (!bridgeGradeRect(layout.objects[objectIndex], footprint)) {
    rejectPathNetwork(result, endpoints.front().pathIndex,
                      "creative_world_layout_bridge_footprint_invalid");
    return false;
  }

  std::array<CreativeTerrainGradeRecipe, 2U> authored;
  for (std::size_t index = 0U; index < authored.size(); ++index) {
    const TerrainPathEndpointRef endpoint = endpoints[index];
    if (!exactGradeEndpointSegment(
            layout.terrainPaths[endpoint.pathIndex].recipe,
            endpoint.pointIndex, authored[index])) {
      rejectPathNetwork(result, endpoint.pathIndex,
                        "creative_world_layout_bridge_approach_grade_invalid");
      return false;
    }
  }

  const std::int64_t width =
      static_cast<std::int64_t>(footprint.maximum.x) - footprint.minimum.x;
  const std::int64_t depth =
      static_cast<std::int64_t>(footprint.maximum.z) - footprint.minimum.z;
  std::int64_t firstLength = 0;
  std::int64_t secondLength = 0;
  const auto negativeSide = [&](const CreativeTerrainGradeRecipe& recipe) {
    return width >= depth ? recipe.end.x == footprint.minimum.x
                          : recipe.end.z == footprint.minimum.z;
  };
  if (!negativeSide(authored[0]) && negativeSide(authored[1])) {
    std::swap(authored[0], authored[1]);
  }
  if (width >= depth) {
    firstLength = static_cast<std::int64_t>(authored[0].end.x) -
                  authored[0].start.x;
    secondLength = static_cast<std::int64_t>(authored[1].start.x) -
                   authored[1].end.x;
  } else {
    firstLength = static_cast<std::int64_t>(authored[0].end.z) -
                  authored[0].start.z;
    secondLength = static_cast<std::int64_t>(authored[1].start.z) -
                   authored[1].end.z;
  }
  if (firstLength <= 0 || firstLength != secondLength ||
      firstLength > std::numeric_limits<std::uint16_t>::max() ||
      authored[0].endHeightCells != authored[1].endHeightCells ||
      authored[0].falloffCells != authored[1].falloffCells ||
      authored[0].crossSlopePermille !=
          -authored[1].crossSlopePermille) {
    rejectPathNetwork(result, endpoints.front().pathIndex,
                      "creative_world_layout_bridge_approach_pair_mismatch");
    return false;
  }

  CreativeTerrainGradeBridgeApproachRequest request;
  request.bridgeFootprint = footprint;
  request.firstTerrainHeightCells = authored[0].startHeightCells;
  request.deckHeightCells = authored[0].endHeightCells;
  request.secondTerrainHeightCells = authored[1].startHeightCells;
  request.approachLengthCells = static_cast<std::uint16_t>(firstLength);
  request.crossSlopePermille = authored[0].crossSlopePermille;
  request.falloffCells = authored[0].falloffCells;
  const CreativeTerrainGradeAdapterPlan plan =
      planCreativeTerrainGradeBridgeApproaches(request);
  if (!plan.accepted || plan.recipeCount != 2U ||
      authored[0] != plan.recipes[0] ||
      authored[1] != reverseGrade(plan.recipes[1])) {
    rejectPathNetwork(result, endpoints.front().pathIndex,
                      "creative_world_layout_bridge_approach_adapter_mismatch");
    return false;
  }
  return true;
}

[[nodiscard]] bool validateWorldLayoutTerrainPathNetwork(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutCompileResult& result) {
  std::vector<std::vector<TerrainPathEndpointRef>> bridgeEndpoints(
      layout.objects.size());
  for (std::size_t pathIndex = 0U;
       pathIndex < layout.terrainPaths.size(); ++pathIndex) {
    const CreativeTerrainPathSourceRecipe& recipe =
        layout.terrainPaths[pathIndex].recipe;
    const std::array<std::size_t, 2U> endpointIndices{
        0U, recipe.points.size() - 1U};
    for (const std::size_t pointIndex : endpointIndices) {
      const CreativeTerrainPathEndpointJoin join =
          endpointJoinAt(recipe, pointIndex);
      if (join == CreativeTerrainPathEndpointJoin::Open ||
          join == CreativeTerrainPathEndpointJoin::Blend) {
        continue;
      }
      const CreativeTerrainPathSourcePoint& endpoint =
          recipe.points[pointIndex];
      if (join == CreativeTerrainPathEndpointJoin::Bridge) {
        std::size_t matchCount = 0U;
        for (std::size_t objectIndex = 0U;
             objectIndex < layout.objects.size(); ++objectIndex) {
          CreativeTerrainGradeRect footprint;
          if (bridgeGradeRect(layout.objects[objectIndex], footprint) &&
              pointOnPerimeter(footprint, endpoint.coord)) {
            bridgeEndpoints[objectIndex].push_back({pathIndex, pointIndex});
            ++matchCount;
          }
        }
        if (matchCount != 1U) {
          rejectPathNetwork(
              result, pathIndex,
              "creative_world_layout_path_bridge_endpoint_unmatched");
          return false;
        }
        continue;
      }
      if (join == CreativeTerrainPathEndpointJoin::BuildingPad) {
        if (!validateBuildingPadEndpoint(layout, {pathIndex, pointIndex},
                                         result)) {
          return false;
        }
        continue;
      }

      bool matched = false;
      for (std::size_t otherPathIndex = 0U;
           otherPathIndex < layout.terrainPaths.size(); ++otherPathIndex) {
        const CreativeTerrainPathSourceRecipe& other =
            layout.terrainPaths[otherPathIndex].recipe;
        for (std::size_t otherPointIndex = 0U;
             otherPointIndex < other.points.size(); ++otherPointIndex) {
          if (pathIndex == otherPathIndex && pointIndex == otherPointIndex) {
            continue;
          }
          const CreativeTerrainPathSourcePoint& candidate =
              other.points[otherPointIndex];
          if (candidate.coord != endpoint.coord) {
            continue;
          }
          const bool candidateEndpoint =
              otherPointIndex == 0U ||
              otherPointIndex + 1U == other.points.size();
          if (candidateEndpoint &&
              endpointJoinAt(other, otherPointIndex) !=
                  CreativeTerrainPathEndpointJoin::Intersection) {
            continue;
          }
          if (candidate.heightCells != endpoint.heightCells) {
            result.receipt.failedTable =
                CreativeWorldLayoutTable::TerrainPath;
            result.receipt.failedIndex = pathIndex;
            setStatus(
                result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_path_intersection_height_mismatch");
            return false;
          }
          matched = true;
        }
      }
      if (!matched) {
        rejectPathNetwork(
            result, pathIndex,
            "creative_world_layout_path_intersection_unmatched");
        return false;
      }
    }
  }
  for (std::size_t objectIndex = 0U; objectIndex < bridgeEndpoints.size();
       ++objectIndex) {
    if (!bridgeEndpoints[objectIndex].empty() &&
        !validateBridgeApproachPair(layout, objectIndex,
                                    bridgeEndpoints[objectIndex], result)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool stageWorldLayoutTerrain(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    std::unordered_set<std::string>& stableKeys,
    CreativeWorldLayoutCompileResult& result,
    CreativeDocument& staged) {
  if (layout.terrainOwnership ==
          CreativeWorldLayoutTerrainOwnership::ReplaceAll &&
      !clearTerrain(staged)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
              "creative_world_layout_terrain_clear_rejected");
    return false;
  }

  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& symbol =
        layout.terrainProfiles[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::TerrainProfile, index,
                     result.receipt)) {
      return false;
    }
    CreativeTerrainLandformKind expectedLandformKind =
        CreativeTerrainLandformKind::Count;
    const bool validLandform =
        symbol.usesLandformRecipe &&
        creativeTerrainRecipeLandformKind(symbol.kind,
                                          expectedLandformKind) &&
        expectedLandformKind == symbol.landform.kind &&
        isValidCreativeTerrainLandformRecipe(symbol.landform);
    if (symbol.usesLandformRecipe && !validLandform) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_landform_invalid");
      return false;
    }
    const bool validRetainingData =
        symbol.retainingEdge.version ==
            kCreativeRetainingEdgeRecipeVersion &&
        isValidCreativeRetainingEdgeSettings(symbol.retainingEdge.settings);
    if (!validRetainingData ||
        (symbol.usesRetainingEdgeRecipe &&
         (!validLandform ||
          symbol.landform.edge != CreativeTerrainLandformEdge::Retaining ||
          !isValidCreativeRetainingEdgeSourceRecipe(symbol.retainingEdge) ||
          symbol.retainingEdge.terrainProfileKey != symbol.stableKey))) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_retaining_edge_source_invalid");
      return false;
    }
    if (symbol.usesLandformRecipe) {
      continue;
    }
    if (symbol.kind == CreativeTerrainRecipeKind::Terrace ||
        symbol.kind == CreativeTerrainRecipeKind::Cliff ||
        symbol.blend != CreativeTerrainProfileBlend::Set ||
        symbol.rodPolicy != CreativeTerrainProfileRodPolicy::Fill) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_profile_not_absolute");
      return false;
    }
    CreativeTerrainProfileRecipeRequest request;
    request.document = &staged;
    request.kind = symbol.kind;
    request.center = symbol.center;
    request.baseHeightCells = symbol.baseHeightCells;
    request.radiusCells = symbol.radiusCells;
    request.amplitudeCells = symbol.amplitudeCells;
    request.spacingCells = symbol.spacingCells;
    request.blend = symbol.blend;
    request.rodPolicy = symbol.rodPolicy;
    request.direction = symbol.direction;
    request.frequency = symbol.frequency;
    const CreativeTerrainRecipeResult recipe =
        buildCreativeTerrainProfileRecipe(request);
    if (!recipe.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = recipe.receipt.kernelReasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                recipe.receipt.reasonCode);
      return false;
    }
    if (!recipe.plan.controlEdits.empty() &&
        !staged.applyTerrainControlEdits(recipe.plan.controlEdits).accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
                "creative_world_layout_profile_stage_rejected");
      return false;
    }
  }

  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::TerrainPath, index,
                     result.receipt)) {
      return false;
    }
    if (!isValidCreativeTerrainPathSourceRecipe(symbol.recipe)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_path_invalid");
      return false;
    }
  }
  if (!validateWorldLayoutTerrainPathNetwork(layout, result)) {
    return false;
  }

  result.plan.terrainEdits = terrainDiff(
      document.terrainField().controls(), staged.terrainField().controls());
  result.plan.materialEdits = materialDiff(
      document.terrainMaterialField().overrides(),
      staged.terrainMaterialField().overrides());
  if (result.plan.terrainEdits.size() > kCreativeTerrainControlCapacity ||
      result.plan.materialEdits.size() >
          kCreativeTerrainMaterialOverrideCapacity) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::CapacityExceeded,
              "creative_world_layout_diff_capacity_exceeded");
    result.plan = {};
    return false;
  }
  return reconcileWorldLayoutTerrainOperations(layout, result, staged);
}

[[nodiscard]] bool shiftBuildingVertically(
    CreativeBuildingRecipeRequest& building,
    double offsetMeters) noexcept {
  if (!std::isfinite(offsetMeters)) {
    return false;
  }
  const auto shift = [offsetMeters](double& value) {
    value += offsetMeters;
    return std::isfinite(value);
  };
  if (building.rootMode == CreativeBuildingRootMode::CreateRoom &&
      (!shift(building.rootBounds.min.y) ||
       !shift(building.rootBounds.max.y))) {
    return false;
  }
  for (CreativeBuildingBoxSpec& box : building.boxes) {
    if (!shift(box.bounds.min.y) || !shift(box.bounds.max.y)) {
      return false;
    }
  }
  for (CreativeBuildingWallSpec& wall : building.walls) {
    if (!shift(wall.start.y) || !shift(wall.end.y)) {
      return false;
    }
  }
  return true;
}

}  // namespace iggy3d::creative::world_layout_compile
