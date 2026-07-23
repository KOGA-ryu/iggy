#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

void setReplayFailure(CreativeTerrainOperationReplayReceipt& receipt,
                      CreativeTerrainOperationReplayStatus status,
                      std::size_t index,
                      CreativeTerrainOperationId operationId,
                      std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.failedOperationIndex = index;
  receipt.failedOperationId = operationId;
  receipt.reasonCode = reasonCode;
}

void setMutationStatus(CreativeTerrainOperationMutationReceipt& receipt,
                       CreativeTerrainOperationMutationStatus status,
                       std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool requestCarriesValidRecipe(
    const CreativeTerrainOperationMutationRequest& request) noexcept {
  switch (request.operationKind) {
    case CreativeTerrainOperationKind::GeneratedTerrain:
      return isValidCreativeTerrainGeneratorRecipe(request.generation) &&
             isValidCreativeTerrainCompositionRecipe(request.composition);
    case CreativeTerrainOperationKind::Region:
      return isValidCreativeTerrainRegionRecipe(request.region);
    case CreativeTerrainOperationKind::Grade:
      return isValidCreativeTerrainGradeRecipe(request.grade);
    case CreativeTerrainOperationKind::Profile:
      return isValidCreativeTerrainProfileRecipe(request.profile);
    case CreativeTerrainOperationKind::Path:
      return isValidCreativeTerrainPathSourceRecipe(request.path);
    case CreativeTerrainOperationKind::Stamp:
      return isValidCreativeTerrainStampRecipe(request.stamp);
    case CreativeTerrainOperationKind::Landform:
      return isValidCreativeTerrainLandformRecipe(request.landform);
    case CreativeTerrainOperationKind::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool operationCarriesValidRecipe(
    const CreativeTerrainOperation& operation) noexcept {
  switch (operation.kind) {
    case CreativeTerrainOperationKind::GeneratedTerrain:
      return isValidCreativeTerrainGeneratorRecipe(operation.generation) &&
             isValidCreativeTerrainCompositionRecipe(operation.composition);
    case CreativeTerrainOperationKind::Region:
      return isValidCreativeTerrainRegionRecipe(operation.region);
    case CreativeTerrainOperationKind::Grade:
      return isValidCreativeTerrainGradeRecipe(operation.grade);
    case CreativeTerrainOperationKind::Profile:
      return isValidCreativeTerrainProfileRecipe(operation.profile);
    case CreativeTerrainOperationKind::Path:
      return isValidCreativeTerrainPathSourceRecipe(operation.path);
    case CreativeTerrainOperationKind::Stamp:
      return isValidCreativeTerrainStampRecipe(operation.stamp);
    case CreativeTerrainOperationKind::Landform:
      return isValidCreativeTerrainLandformRecipe(operation.landform);
    case CreativeTerrainOperationKind::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool validOperationProvenance(
    CreativeTerrainOperationOwner owner,
    std::string_view sourceKey) noexcept {
  if (owner >= CreativeTerrainOperationOwner::Count ||
      sourceKey.size() > kCreativeTerrainOperationSourceKeyCapacity) {
    return false;
  }
  return owner == CreativeTerrainOperationOwner::Manual
             ? sourceKey.empty()
             : !sourceKey.empty();
}

[[nodiscard]] bool terrainCoordLess(CreativeTerrainCoord2 lhs,
                                    CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool hardEdgeLess(CreativeTerrainHardEdge lhs,
                                CreativeTerrainHardEdge rhs) noexcept {
  return lhs.first != rhs.first ? terrainCoordLess(lhs.first, rhs.first)
                                : terrainCoordLess(lhs.second, rhs.second);
}

[[nodiscard]] bool coordInsideBounds(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  const std::int64_t x =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t z =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  return x >= 0 && z >= 0 && x < bounds.widthCells && z < bounds.depthCells;
}

void eraseHardEdgesChangedBy(
    std::vector<CreativeTerrainHardEdge>& edges,
    const CreativeTerrainHeightField& before,
    const CreativeTerrainHeightField& after) {
  std::erase_if(edges, [&](CreativeTerrainHardEdge edge) {
    return before.heightAt(edge.first) != after.heightAt(edge.first) ||
           before.heightAt(edge.second) != after.heightAt(edge.second);
  });
}

void eraseHardEdgesTouching(
    std::vector<CreativeTerrainHardEdge>& edges,
    CreativeTerrainHeightFieldBounds bounds) {
  std::erase_if(edges, [&](CreativeTerrainHardEdge edge) {
    return coordInsideBounds(edge.first, bounds) ||
           coordInsideBounds(edge.second, bounds);
  });
}

[[nodiscard]] bool mergeHardEdges(
    std::vector<CreativeTerrainHardEdge>& edges,
    std::span<const CreativeTerrainHardEdge> additions) {
  if (additions.size() > kCreativeTerrainHardEdgeCapacity - edges.size()) {
    return false;
  }
  edges.insert(edges.end(), additions.begin(), additions.end());
  std::sort(edges.begin(), edges.end(), hardEdgeLess);
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  return validateCreativeTerrainHardEdges(edges);
}

}  // namespace

std::uint64_t hashCreativeTerrainHeightField(
    const CreativeTerrainHeightField& field) noexcept {
  const CreativeTerrainHeightFieldBounds bounds = field.bounds();
  StableHasher hasher;
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : field.heights()) {
    hasher.addU64(height);
  }
  return hasher.value();
}

std::uint64_t hashCreativeTerrainMaterialField(
    const CreativeTerrainMaterialField& field) noexcept {
  StableHasher hasher;
  for (const CreativeTerrainMaterialOverride& value : field.overrides()) {
    hasher.addI64(value.coord.x);
    hasher.addI64(value.coord.z);
    for (const std::uint8_t weight : value.weights) {
      hasher.addU64(weight);
    }
  }
  return hasher.value();
}

bool creativeTerrainHardEdgesEqual(
    std::span<const CreativeTerrainHardEdge> lhs,
    std::span<const CreativeTerrainHardEdge> rhs) noexcept {
  return validateCreativeTerrainHardEdges(lhs) &&
         validateCreativeTerrainHardEdges(rhs) && lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

std::uint64_t hashCreativeTerrainHardEdges(
    std::span<const CreativeTerrainHardEdge> edges) noexcept {
  StableHasher hasher;
  for (const CreativeTerrainHardEdge edge : edges) {
    hasher.addI64(edge.first.x);
    hasher.addI64(edge.first.z);
    hasher.addI64(edge.second.x);
    hasher.addI64(edge.second.z);
  }
  return hasher.value();
}

bool creativeTerrainHeightFieldsEqual(
    const CreativeTerrainHeightField& lhs,
    const CreativeTerrainHeightField& rhs) noexcept {
  return lhs.validateInvariants() && rhs.validateInvariants() &&
         lhs.bounds() == rhs.bounds() &&
         lhs.heights().size() == rhs.heights().size() &&
         std::equal(lhs.heights().begin(), lhs.heights().end(),
                    rhs.heights().begin(), rhs.heights().end());
}

bool creativeTerrainMaterialFieldsEqual(
    const CreativeTerrainMaterialField& lhs,
    const CreativeTerrainMaterialField& rhs) noexcept {
  return lhs.validateInvariants() && rhs.validateInvariants() &&
         lhs.overrides().size() == rhs.overrides().size() &&
         std::equal(lhs.overrides().begin(), lhs.overrides().end(),
                    rhs.overrides().begin());
}

bool validateCreativeTerrainOperationStack(
    const CreativeTerrainOperationStack& stack) noexcept {
  if (stack.version != kCreativeTerrainOperationStackVersion ||
      stack.nextOperationId == kInvalidCreativeTerrainOperationId ||
      !stack.baseHeightField.validateInvariants() ||
      !stack.baseMaterialField.validateInvariants() ||
      !validateCreativeTerrainHardEdges(stack.baseHardEdges) ||
      stack.operations.size() > kCreativeTerrainOperationCapacity) {
    return false;
  }
  CreativeTerrainOperationId maximumId = kInvalidCreativeTerrainOperationId;
  for (std::size_t index = 0U; index < stack.operations.size(); ++index) {
    const CreativeTerrainOperation& operation = stack.operations[index];
    if (operation.id == kInvalidCreativeTerrainOperationId ||
        !validOperationProvenance(operation.owner, operation.sourceKey) ||
        !operationCarriesValidRecipe(operation)) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (stack.operations[prior].id == operation.id) {
        return false;
      }
    }
    maximumId = std::max(maximumId, operation.id);
  }
  return stack.nextOperationId > maximumId;
}

const CreativeTerrainOperation* findCreativeTerrainOperation(
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationId operationId) noexcept {
  const std::size_t index =
      findCreativeTerrainOperationIndex(stack, operationId);
  return index < stack.operations.size() ? &stack.operations[index] : nullptr;
}

std::size_t findCreativeTerrainOperationIndex(
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationId operationId) noexcept {
  const auto found = std::find_if(
      stack.operations.begin(), stack.operations.end(),
      [operationId](const CreativeTerrainOperation& operation) {
        return operation.id == operationId;
      });
  return static_cast<std::size_t>(found - stack.operations.begin());
}

std::string_view toString(
    CreativeTerrainOperationReplayStatus status) noexcept {
  switch (status) {
    case CreativeTerrainOperationReplayStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainOperationReplayStatus::InvalidTerrain:
      return "InvalidTerrain";
    case CreativeTerrainOperationReplayStatus::InvalidStack:
      return "InvalidStack";
    case CreativeTerrainOperationReplayStatus::GenerationRejected:
      return "GenerationRejected";
    case CreativeTerrainOperationReplayStatus::CompositionRejected:
      return "CompositionRejected";
    case CreativeTerrainOperationReplayStatus::RegionRejected:
      return "RegionRejected";
    case CreativeTerrainOperationReplayStatus::GradeRejected:
      return "GradeRejected";
    case CreativeTerrainOperationReplayStatus::ProfileRejected:
      return "ProfileRejected";
    case CreativeTerrainOperationReplayStatus::PathRejected:
      return "PathRejected";
    case CreativeTerrainOperationReplayStatus::StampRejected:
      return "StampRejected";
    case CreativeTerrainOperationReplayStatus::LandformRejected:
      return "LandformRejected";
    case CreativeTerrainOperationReplayStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainOperationKind kind) noexcept {
  switch (kind) {
    case CreativeTerrainOperationKind::GeneratedTerrain:
      return "GeneratedTerrain";
    case CreativeTerrainOperationKind::Region:
      return "Region";
    case CreativeTerrainOperationKind::Grade:
      return "Grade";
    case CreativeTerrainOperationKind::Profile:
      return "Profile";
    case CreativeTerrainOperationKind::Path:
      return "Path";
    case CreativeTerrainOperationKind::Stamp:
      return "Stamp";
    case CreativeTerrainOperationKind::Landform:
      return "Landform";
    case CreativeTerrainOperationKind::Count:
      return "Invalid";
  }
  return "Invalid";
}

bool parseCreativeTerrainOperationKind(
    std::string_view text,
    CreativeTerrainOperationKind& output) noexcept {
  if (text == "GeneratedTerrain") {
    output = CreativeTerrainOperationKind::GeneratedTerrain;
    return true;
  }
  if (text == "Region") {
    output = CreativeTerrainOperationKind::Region;
    return true;
  }
  if (text == "Grade") {
    output = CreativeTerrainOperationKind::Grade;
    return true;
  }
  if (text == "Profile") {
    output = CreativeTerrainOperationKind::Profile;
    return true;
  }
  if (text == "Path") {
    output = CreativeTerrainOperationKind::Path;
    return true;
  }
  if (text == "Stamp") {
    output = CreativeTerrainOperationKind::Stamp;
    return true;
  }
  if (text == "Landform") {
    output = CreativeTerrainOperationKind::Landform;
    return true;
  }
  return false;
}

std::string_view toString(CreativeTerrainOperationOwner owner) noexcept {
  switch (owner) {
    case CreativeTerrainOperationOwner::Manual:
      return "Manual";
    case CreativeTerrainOperationOwner::WorldLayout:
      return "WorldLayout";
    case CreativeTerrainOperationOwner::Count:
      return "Invalid";
  }
  return "Invalid";
}

bool parseCreativeTerrainOperationOwner(
    std::string_view text,
    CreativeTerrainOperationOwner& output) noexcept {
  if (text == "Manual") {
    output = CreativeTerrainOperationOwner::Manual;
    return true;
  }
  if (text == "WorldLayout") {
    output = CreativeTerrainOperationOwner::WorldLayout;
    return true;
  }
  return false;
}

std::string_view toString(
    CreativeTerrainOperationMutationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainOperationMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainOperationMutationStatus::InvalidTerrain:
      return "InvalidTerrain";
    case CreativeTerrainOperationMutationStatus::InvalidStack:
      return "InvalidStack";
    case CreativeTerrainOperationMutationStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainOperationMutationStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainOperationMutationStatus::IdExhausted:
      return "IdExhausted";
    case CreativeTerrainOperationMutationStatus::NotFound:
      return "NotFound";
    case CreativeTerrainOperationMutationStatus::ReplayRejected:
      return "ReplayRejected";
    case CreativeTerrainOperationMutationStatus::NoChange:
      return "NoChange";
    case CreativeTerrainOperationMutationStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

CreativeTerrainOperationReplayResult replayCreativeTerrainOperations(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationReplayCache* cache) {
  CreativeTerrainOperationReplayResult result;
  CreativeTerrainOperationReplayReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.operationCount = stack.operations.size();
  if (!legacyTerrain.validateInvariants()) {
    setReplayFailure(receipt,
                     CreativeTerrainOperationReplayStatus::InvalidTerrain, 0U,
                     kInvalidCreativeTerrainOperationId,
                     "creative_terrain_operation_legacy_terrain_invalid");
    return result;
  }
  if (!validateCreativeTerrainOperationStack(stack)) {
    setReplayFailure(receipt,
                     CreativeTerrainOperationReplayStatus::InvalidStack, 0U,
                     kInvalidCreativeTerrainOperationId,
                     "creative_terrain_operation_stack_invalid");
    return result;
  }

  const CreativeTerrainSurfacePlan baseSurface =
      buildCreativeComposedTerrainSurfacePlan(
          legacyTerrain, stack.baseHeightField, stack.baseHardEdges);
  if (!baseSurface.accepted) {
    setReplayFailure(receipt,
                     CreativeTerrainOperationReplayStatus::InvalidStack, 0U,
                     kInvalidCreativeTerrainOperationId,
                     "creative_terrain_operation_base_topology_invalid");
    return result;
  }

  CreativeTerrainHeightField current = stack.baseHeightField;
  CreativeTerrainMaterialField currentMaterial = stack.baseMaterialField;
  std::vector<CreativeTerrainHardEdge> currentHardEdges = stack.baseHardEdges;
  const auto adoptHeightField = [&](CreativeTerrainHeightField next) {
    eraseHardEdgesChangedBy(currentHardEdges, current, next);
    current = std::move(next);
  };
  CreativeTerrainPathSourceCache stagedPathCache;
  CreativeTerrainPathSourceCache* targetedPathCache = nullptr;
  if (cache != nullptr && cache->pathSource != nullptr) {
    stagedPathCache = *cache->pathSource;
    targetedPathCache = &stagedPathCache;
  }
  for (std::size_t index = 0U; index < stack.operations.size(); ++index) {
    const CreativeTerrainOperation& operation = stack.operations[index];
    if (!operation.enabled) {
      ++receipt.disabledOperationCount;
      continue;
    }
    ++receipt.enabledOperationCount;
    const CreativeTerrainSurfacePlan canonicalSource =
        buildCreativeComposedTerrainSurfacePlan(legacyTerrain, current);
    if (operation.kind == CreativeTerrainOperationKind::Region) {
      CreativeTerrainRegionRecipeResult region =
          buildCreativeTerrainRegionRecipe(current, canonicalSource,
                                           operation.region);
      if (!region.receipt.accepted) {
        setReplayFailure(receipt,
                         CreativeTerrainOperationReplayStatus::RegionRejected,
                         index, operation.id,
                         "creative_terrain_operation_region_rejected");
        return result;
      }
      ++receipt.regionOperationCount;
      receipt.evaluatedCellCount += region.receipt.evaluatedCellCount;
      receipt.modifiedCellCount += region.receipt.modifiedCellCount;
      receipt.featheredCellCount += region.receipt.featheredCellCount;
      adoptHeightField(std::move(region.heightField));
      continue;
    }
    if (operation.kind == CreativeTerrainOperationKind::Grade) {
      CreativeTerrainGradeRecipeResult grade = buildCreativeTerrainGradeRecipe(
          current, canonicalSource, operation.grade);
      if (!grade.receipt.accepted) {
        setReplayFailure(receipt,
                         CreativeTerrainOperationReplayStatus::GradeRejected,
                         index, operation.id,
                         "creative_terrain_operation_grade_rejected");
        return result;
      }
      ++receipt.gradeOperationCount;
      receipt.evaluatedCellCount += grade.receipt.evaluatedCellCount;
      receipt.modifiedCellCount += grade.receipt.modifiedCellCount;
      receipt.featheredCellCount += grade.receipt.falloffCellCount;
      receipt.lastGradeReadout = grade.receipt.readout;
      adoptHeightField(std::move(grade.heightField));
      continue;
    }
    if (operation.kind == CreativeTerrainOperationKind::Profile) {
      CreativeTerrainProfileRecipeResult profile =
          buildCreativeTerrainProfileRecipe(current, canonicalSource,
                                            operation.profile);
      if (!profile.receipt.accepted) {
        setReplayFailure(receipt,
                         CreativeTerrainOperationReplayStatus::ProfileRejected,
                         index, operation.id,
                         "creative_terrain_operation_profile_rejected");
        return result;
      }
      ++receipt.profileOperationCount;
      receipt.evaluatedCellCount += profile.receipt.evaluatedCellCount;
      receipt.modifiedCellCount += profile.receipt.modifiedCellCount;
      adoptHeightField(std::move(profile.heightField));
      continue;
    }
    if (operation.kind == CreativeTerrainOperationKind::Path) {
      CreativeTerrainPathSourceCache* pathCache =
          cache != nullptr && cache->pathOperationId == operation.id
              ? targetedPathCache
              : nullptr;
      CreativeTerrainPathSourceResult path =
          buildCreativeTerrainPathSourceRecipe(current, canonicalSource,
                                               currentMaterial,
                                               operation.path, pathCache);
      if (!path.receipt.accepted) {
        setReplayFailure(receipt,
                         CreativeTerrainOperationReplayStatus::PathRejected,
                         index, operation.id,
                         "creative_terrain_operation_path_rejected");
        return result;
      }
      CreativeTerrainMaterialField stagedMaterial = currentMaterial;
      if (!path.materialEdits.empty() &&
          !stagedMaterial.apply(path.materialEdits).accepted) {
        setReplayFailure(receipt,
                         CreativeTerrainOperationReplayStatus::PathRejected,
                         index, operation.id,
                         "creative_terrain_operation_path_material_rejected");
        return result;
      }
      ++receipt.pathOperationCount;
      receipt.pathSegmentCount += path.receipt.segmentCount;
      receipt.pathCenterlineCellCount += path.receipt.centerlineCellCount;
      receipt.pathGeneratedControlCount +=
          path.receipt.generatedControlCount;
      receipt.pathRebuiltSegmentCount += path.receipt.rebuiltSegmentCount;
      receipt.pathReusedSegmentCount += path.receipt.reusedSegmentCount;
      receipt.evaluatedCellCount += path.receipt.evaluatedCellCount;
      receipt.modifiedCellCount += path.receipt.modifiedCellCount;
      receipt.materialEditCount += path.receipt.materialEditCount;
      adoptHeightField(std::move(path.heightField));
      currentMaterial = std::move(stagedMaterial);
      continue;
    }
    if (operation.kind == CreativeTerrainOperationKind::Stamp) {
      CreativeTerrainStampPlan stamp = buildCreativeTerrainStampPlan(
          current, currentMaterial, canonicalSource, operation.stamp);
      if (!stamp.accepted) {
        setReplayFailure(receipt,
                         CreativeTerrainOperationReplayStatus::StampRejected,
                         index, operation.id,
                         "creative_terrain_operation_stamp_rejected");
        return result;
      }
      ++receipt.stampOperationCount;
      receipt.evaluatedCellCount += stamp.affectedCellCount;
      receipt.modifiedCellCount += stamp.changedHeightCellCount;
      receipt.materialModifiedCellCount +=
          stamp.changedMaterialCellCount;
      adoptHeightField(std::move(stamp.heightField));
      currentMaterial = std::move(stamp.materialField);
      continue;
    }
    if (operation.kind == CreativeTerrainOperationKind::Landform) {
      CreativeTerrainLandformResult landform = buildCreativeTerrainLandform(
          current, canonicalSource, currentMaterial, operation.landform);
      if (!landform.receipt.accepted) {
        setReplayFailure(
            receipt, CreativeTerrainOperationReplayStatus::LandformRejected,
            index, operation.id,
            "creative_terrain_operation_landform_rejected");
        return result;
      }
      CreativeTerrainMaterialField stagedMaterial = currentMaterial;
      if (!landform.materialEdits.empty() &&
          !stagedMaterial.apply(landform.materialEdits).accepted) {
        setReplayFailure(
            receipt, CreativeTerrainOperationReplayStatus::LandformRejected,
            index, operation.id,
            "creative_terrain_operation_landform_material_rejected");
        return result;
      }
      ++receipt.landformOperationCount;
      receipt.evaluatedCellCount += landform.receipt.evaluatedCellCount;
      receipt.modifiedCellCount += landform.receipt.modifiedCellCount;
      receipt.featheredCellCount += landform.receipt.transitionCellCount;
      receipt.materialEditCount += landform.receipt.materialEditCount;
      eraseHardEdgesTouching(currentHardEdges, operation.landform.bounds);
      adoptHeightField(std::move(landform.heightField));
      if (!mergeHardEdges(currentHardEdges, landform.hardEdges)) {
        setReplayFailure(
            receipt, CreativeTerrainOperationReplayStatus::LandformRejected,
            index, operation.id,
            "creative_terrain_operation_landform_hard_edges_rejected");
        return result;
      }
      currentMaterial = std::move(stagedMaterial);
      continue;
    }
    const CreativeTerrainGenerationResult generation =
        buildCreativeTerrainGenerationPlan(operation.generation);
    if (!generation.receipt.accepted) {
      setReplayFailure(
          receipt, CreativeTerrainOperationReplayStatus::GenerationRejected,
          index, operation.id,
          "creative_terrain_operation_generation_rejected");
      return result;
    }
    const CreativeTerrainCompositionResult composition =
        composeCreativeTerrainGeneration(current, currentMaterial,
                                          canonicalSource, generation,
                                          operation.composition);
    if (!composition.receipt.accepted) {
      setReplayFailure(
          receipt, CreativeTerrainOperationReplayStatus::CompositionRejected,
          index, operation.id,
          "creative_terrain_operation_composition_rejected");
      return result;
    }
    receipt.evaluatedCellCount += generation.receipt.generatedCellCount;
    receipt.evaluatedOctaveCount +=
        generation.receipt.evaluatedOctaveCount;
    receipt.modifiedCellCount += composition.receipt.modifiedCellCount;
    receipt.materialModifiedCellCount +=
        composition.receipt.materialModifiedCellCount;
    receipt.protectedCellCount += composition.receipt.protectedCellCount;
    receipt.featheredCellCount += composition.receipt.featheredCellCount;
    adoptHeightField(composition.heightField);
    currentMaterial = composition.materialField;
  }

  const CreativeTerrainSurfacePlan finalSurface =
      buildCreativeComposedTerrainSurfacePlan(legacyTerrain, current,
                                               currentHardEdges);
  if (!finalSurface.accepted) {
    setReplayFailure(receipt,
                     CreativeTerrainOperationReplayStatus::InvalidStack, 0U,
                     kInvalidCreativeTerrainOperationId,
                     "creative_terrain_operation_output_topology_invalid");
    return result;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainOperationReplayStatus::Ready;
  receipt.outputCellCount = current.cellCount();
  receipt.outputMaterialOverrideCount = currentMaterial.overrideCount();
  receipt.outputHardEdgeCount = currentHardEdges.size();
  receipt.heightHash = hashCreativeTerrainHeightField(current);
  receipt.materialHash = hashCreativeTerrainMaterialField(currentMaterial);
  receipt.hardEdgeHash = hashCreativeTerrainHardEdges(currentHardEdges);
  receipt.reasonCode = "creative_terrain_operation_replay_ready";
  result.heightField = std::move(current);
  result.materialField = std::move(currentMaterial);
  result.hardEdges = std::move(currentHardEdges);
  if (targetedPathCache != nullptr) {
    *cache->pathSource = std::move(stagedPathCache);
  }
  return result;
}

CreativeTerrainOperationMutationPlan planCreativeTerrainOperationMutation(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainHeightField& currentDerived,
    const CreativeTerrainMaterialField& currentDerivedMaterial,
    const CreativeTerrainOperationStack& currentStack,
    const CreativeTerrainOperationMutationRequest& request,
    CreativeTerrainPathSourceCache* pathCache,
    const std::vector<CreativeTerrainHardEdge>* currentDerivedHardEdges) {
  CreativeTerrainOperationMutationPlan plan;
  CreativeTerrainOperationMutationReceipt& receipt = plan.receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.operationId = request.operationId;
  receipt.operationCountBefore = currentStack.operations.size();
  receipt.operationCountAfter = receipt.operationCountBefore;
  if (!legacyTerrain.validateInvariants() ||
      !currentDerived.validateInvariants() ||
      !currentDerivedMaterial.validateInvariants() ||
      (currentDerivedHardEdges != nullptr &&
       !validateCreativeTerrainHardEdges(*currentDerivedHardEdges))) {
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::InvalidTerrain,
                      "creative_terrain_operation_mutation_terrain_invalid");
    return plan;
  }
  if (!validateCreativeTerrainOperationStack(currentStack) ||
      (currentStack.operations.empty() &&
       (currentStack.baseHeightField.cellCount() != 0U ||
        currentStack.baseMaterialField.overrideCount() != 0U ||
        !currentStack.baseHardEdges.empty()))) {
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::InvalidStack,
                      "creative_terrain_operation_mutation_stack_invalid");
    return plan;
  }
  std::vector<CreativeTerrainHardEdge> resolvedCurrentHardEdges =
      currentDerivedHardEdges == nullptr
          ? std::vector<CreativeTerrainHardEdge>{}
          : *currentDerivedHardEdges;
  if (!currentStack.operations.empty()) {
    const CreativeTerrainOperationReplayResult currentReplay =
        replayCreativeTerrainOperations(legacyTerrain, currentStack);
    if (!currentReplay.receipt.accepted ||
        !creativeTerrainHeightFieldsEqual(currentReplay.heightField,
                                          currentDerived) ||
        !creativeTerrainMaterialFieldsEqual(currentReplay.materialField,
                                            currentDerivedMaterial) ||
        (currentDerivedHardEdges != nullptr &&
         !creativeTerrainHardEdgesEqual(currentReplay.hardEdges,
                                        *currentDerivedHardEdges))) {
      setMutationStatus(
          receipt, CreativeTerrainOperationMutationStatus::InvalidStack,
          "creative_terrain_operation_derived_field_mismatch");
      return plan;
    }
    if (currentDerivedHardEdges == nullptr) {
      resolvedCurrentHardEdges = currentReplay.hardEdges;
    }
  }
  if (request.kind >= CreativeTerrainOperationMutationKind::Count) {
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::InvalidRequest,
                      "creative_terrain_operation_mutation_kind_invalid");
    return plan;
  }

  CreativeTerrainOperationStack staged = currentStack;
  bool changed = false;
  switch (request.kind) {
    case CreativeTerrainOperationMutationKind::Add: {
      if (!requestCarriesValidRecipe(request) ||
          !validOperationProvenance(request.owner, request.sourceKey)) {
        setMutationStatus(
            receipt, CreativeTerrainOperationMutationStatus::InvalidRequest,
            "creative_terrain_operation_add_recipe_invalid");
        return plan;
      }
      if (staged.operations.size() >= kCreativeTerrainOperationCapacity) {
        setMutationStatus(
            receipt,
            CreativeTerrainOperationMutationStatus::CapacityExceeded,
            "creative_terrain_operation_capacity_exceeded");
        return plan;
      }
      if (staged.nextOperationId ==
          std::numeric_limits<CreativeTerrainOperationId>::max()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::IdExhausted,
                          "creative_terrain_operation_id_exhausted");
        return plan;
      }
      if (staged.operations.empty()) {
        staged.baseHeightField = currentDerived;
        staged.baseMaterialField = currentDerivedMaterial;
        staged.baseHardEdges = resolvedCurrentHardEdges;
      }
      receipt.operationId = staged.nextOperationId++;
      receipt.operationIndexBefore = staged.operations.size();
      receipt.operationIndexAfter = staged.operations.size();
      CreativeTerrainOperation operation;
      operation.id = receipt.operationId;
      operation.enabled = request.enabled;
      operation.owner = request.owner;
      operation.sourceKey = request.sourceKey;
      operation.kind = request.operationKind;
      operation.generation = request.generation;
      operation.composition = request.composition;
      operation.region = request.region;
      operation.grade = request.grade;
      operation.profile = request.profile;
      operation.path = request.path;
      operation.stamp = request.stamp;
      operation.landform = request.landform;
      staged.operations.push_back(std::move(operation));
      changed = true;
      break;
    }
    case CreativeTerrainOperationMutationKind::Update: {
      if (!requestCarriesValidRecipe(request) ||
          !validOperationProvenance(request.owner, request.sourceKey)) {
        setMutationStatus(
            receipt, CreativeTerrainOperationMutationStatus::InvalidRequest,
            "creative_terrain_operation_update_recipe_invalid");
        return plan;
      }
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = index;
      CreativeTerrainOperation replacement;
      replacement.id = request.operationId;
      replacement.enabled = request.enabled;
      replacement.owner = request.owner;
      replacement.sourceKey = request.sourceKey;
      replacement.kind = request.operationKind;
      replacement.generation = request.generation;
      replacement.composition = request.composition;
      replacement.region = request.region;
      replacement.grade = request.grade;
      replacement.profile = request.profile;
      replacement.path = request.path;
      replacement.stamp = request.stamp;
      replacement.landform = request.landform;
      changed = staged.operations[index] != replacement;
      staged.operations[index] = replacement;
      break;
    }
    case CreativeTerrainOperationMutationKind::SetEnabled: {
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = index;
      changed = staged.operations[index].enabled != request.enabled;
      staged.operations[index].enabled = request.enabled;
      break;
    }
    case CreativeTerrainOperationMutationKind::Move: {
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      if (request.targetIndex >= staged.operations.size()) {
        setMutationStatus(
            receipt, CreativeTerrainOperationMutationStatus::InvalidRequest,
            "creative_terrain_operation_target_index_invalid");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = request.targetIndex;
      changed = index != request.targetIndex;
      if (changed) {
        CreativeTerrainOperation moved = staged.operations[index];
        using Difference =
            std::vector<CreativeTerrainOperation>::difference_type;
        staged.operations.erase(
            staged.operations.begin() + static_cast<Difference>(index));
        staged.operations.insert(staged.operations.begin() +
                                     static_cast<Difference>(
                                         request.targetIndex),
                                 std::move(moved));
      }
      break;
    }
    case CreativeTerrainOperationMutationKind::Remove: {
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = index;
      using Difference =
          std::vector<CreativeTerrainOperation>::difference_type;
      staged.operations.erase(
          staged.operations.begin() + static_cast<Difference>(index));
      changed = true;
      break;
    }
    case CreativeTerrainOperationMutationKind::BakeAll:
      if (staged.operations.empty()) {
        break;
      }
      staged.baseHeightField = currentDerived;
      staged.baseMaterialField = currentDerivedMaterial;
      staged.baseHardEdges = resolvedCurrentHardEdges;
      staged.operations.clear();
      receipt.operationIndexBefore = 0U;
      receipt.operationIndexAfter = 0U;
      changed = true;
      break;
    case CreativeTerrainOperationMutationKind::Count:
      break;
  }

  if (!changed) {
    plan.stack = currentStack;
    plan.heightField = currentDerived;
    plan.materialField = currentDerivedMaterial;
    plan.hardEdges = resolvedCurrentHardEdges;
    receipt.accepted = true;
    receipt.operationCountAfter = currentStack.operations.size();
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::NoChange,
                      "creative_terrain_operation_mutation_no_change");
    return plan;
  }

  if (staged.operations.empty()) {
    plan.heightField = staged.baseHeightField;
    plan.materialField = staged.baseMaterialField;
    plan.hardEdges = staged.baseHardEdges;
    staged = {};
    receipt.replay.requested = true;
    receipt.replay.accepted = true;
    receipt.replay.status = CreativeTerrainOperationReplayStatus::Ready;
    receipt.replay.outputCellCount = plan.heightField.cellCount();
    receipt.replay.outputMaterialOverrideCount =
        plan.materialField.overrideCount();
    receipt.replay.outputHardEdgeCount = plan.hardEdges.size();
    receipt.replay.heightHash =
        hashCreativeTerrainHeightField(plan.heightField);
    receipt.replay.materialHash =
        hashCreativeTerrainMaterialField(plan.materialField);
    receipt.replay.hardEdgeHash =
        hashCreativeTerrainHardEdges(plan.hardEdges);
    receipt.replay.reasonCode = "creative_terrain_operation_replay_base_ready";
  } else {
    CreativeTerrainOperationReplayCache replayCache{
        receipt.operationId, pathCache};
    CreativeTerrainOperationReplayResult replay =
        replayCreativeTerrainOperations(
            legacyTerrain, staged,
            pathCache == nullptr ? nullptr : &replayCache);
    receipt.replay = replay.receipt;
    if (!replay.receipt.accepted) {
      setMutationStatus(
          receipt, CreativeTerrainOperationMutationStatus::ReplayRejected,
          "creative_terrain_operation_mutation_replay_rejected");
      return plan;
    }
    plan.heightField = std::move(replay.heightField);
    plan.materialField = std::move(replay.materialField);
    plan.hardEdges = std::move(replay.hardEdges);
  }

  plan.stack = std::move(staged);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.operationCountAfter = plan.stack.operations.size();
  setMutationStatus(receipt, CreativeTerrainOperationMutationStatus::Applied,
                    "creative_terrain_operation_mutation_applied");
  return plan;
}

}  // namespace iggy3d::creative
