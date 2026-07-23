#include "app/iggy3d/creative/world/DocumentSectionInternal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <set>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::document_section_internal {

[[nodiscard]] std::vector<SaveCreativeDocumentVoxelChunkRecord>
toSaveVoxelChunks(const creative::CreativeVoxelField& field) {
  std::vector<SaveCreativeDocumentVoxelChunkRecord> records;
  records.reserve(static_cast<std::size_t>(field.chunkCount()));
  for (const creative::CreativeVoxelChunk& chunk : field.chunks()) {
    SaveCreativeDocumentVoxelChunkRecord record;
    record.x = chunk.coord.x;
    record.y = chunk.coord.y;
    record.z = chunk.coord.z;
    record.cells.reserve(chunk.occupiedCellCount);
    for (std::size_t localIndex = 0;
         localIndex < chunk.materials.size(); ++localIndex) {
      const creative::CreativeObjectKind material =
          chunk.materials[localIndex];
      if (material == creative::CreativeObjectKind::Unknown) {
        continue;
      }
      record.cells.push_back(
          {static_cast<std::uint16_t>(localIndex),
           std::string{creative::serializedObjectKindId(material)}});
    }
    records.push_back(std::move(record));
  }
  return records;
}

namespace {

[[nodiscard]] bool checkedVoxelGlobalCoord(std::int32_t chunkCoord,
                                           std::int32_t localCoord,
                                           std::int32_t& out) noexcept {
  const std::int64_t value =
      static_cast<std::int64_t>(chunkCoord) *
          creative::kCreativeVoxelChunkEdge +
      localCoord;
  if (value < std::numeric_limits<std::int32_t>::min() ||
      value > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

}  // namespace

[[nodiscard]] bool toCreativeVoxelField(
    std::span<const SaveCreativeDocumentVoxelChunkRecord> records,
    creative::CreativeVoxelField& out) {
  constexpr std::uint64_t kMaxPersistedVoxelCellCount = 16'777'216U;
  std::vector<creative::CreativeVoxelEdit> edits;
  std::uint64_t totalCellCount = 0;
  for (const SaveCreativeDocumentVoxelChunkRecord& chunk : records) {
    if (chunk.cells.empty() ||
        chunk.cells.size() > creative::kCreativeVoxelChunkCellCount ||
        totalCellCount > kMaxPersistedVoxelCellCount - chunk.cells.size()) {
      return false;
    }
    totalCellCount += chunk.cells.size();
  }
  edits.reserve(static_cast<std::size_t>(totalCellCount));

  std::set<std::tuple<std::int32_t, std::int32_t, std::int32_t>> chunkCoords;
  for (const SaveCreativeDocumentVoxelChunkRecord& chunk : records) {
    if (!chunkCoords.emplace(chunk.x, chunk.y, chunk.z).second) {
      return false;
    }
    std::unordered_set<std::uint16_t> localIndices;
    localIndices.reserve(chunk.cells.size());
    for (const SaveCreativeDocumentVoxelCellRecord& cell : chunk.cells) {
      if (cell.localIndex >= creative::kCreativeVoxelChunkCellCount ||
          !localIndices.insert(cell.localIndex).second) {
        return false;
      }
      creative::CreativeObjectKind material =
          creative::CreativeObjectKind::Unknown;
      if (!creative::parseSerializedObjectKindId(cell.material, material) ||
          material == creative::CreativeObjectKind::Unknown ||
          material == creative::CreativeObjectKind::Count) {
        return false;
      }

      const std::int32_t localX =
          cell.localIndex % creative::kCreativeVoxelChunkEdge;
      const std::int32_t localY =
          (cell.localIndex / creative::kCreativeVoxelChunkEdge) %
          creative::kCreativeVoxelChunkEdge;
      const std::int32_t localZ =
          cell.localIndex /
          (creative::kCreativeVoxelChunkEdge *
           creative::kCreativeVoxelChunkEdge);
      creative::CreativeGridCoord3 global{};
      if (!checkedVoxelGlobalCoord(chunk.x, localX, global.x) ||
          !checkedVoxelGlobalCoord(chunk.y, localY, global.y) ||
          !checkedVoxelGlobalCoord(chunk.z, localZ, global.z)) {
        return false;
      }
      edits.push_back({global, material});
    }
  }

  creative::CreativeVoxelField restored;
  const creative::CreativeVoxelMutationReceipt receipt = restored.apply(edits);
  if (!receipt.accepted || (edits.empty() ? receipt.changed
                                         : !receipt.changed)) {
    return false;
  }
  out = std::move(restored);
  return true;
}

[[nodiscard]] std::vector<SaveCreativeDocumentTerrainControlRecord>
toSaveTerrainControls(const creative::CreativeTerrainField& field) {
  std::vector<SaveCreativeDocumentTerrainControlRecord> records;
  records.reserve(static_cast<std::size_t>(field.controlCount()));
  for (const creative::CreativeTerrainControlPoint& control : field.controls()) {
    records.push_back({control.coord.x, control.coord.z, control.heightCells,
                       control.radiusCells});
  }
  return records;
}

[[nodiscard]] bool toCreativeTerrainField(
    std::span<const SaveCreativeDocumentTerrainControlRecord> records,
    creative::CreativeTerrainField& out) {
  if (records.size() > creative::kCreativeTerrainControlCapacity) {
    return false;
  }
  std::vector<creative::CreativeTerrainControlEdit> edits;
  edits.reserve(records.size());
  for (const SaveCreativeDocumentTerrainControlRecord& record : records) {
    creative::CreativeTerrainControlPoint control;
    control.coord = {record.x, record.z};
    control.heightCells = record.heightCells;
    control.radiusCells = record.radiusCells;
    if (!creative::isValidCreativeTerrainControlPoint(control)) {
      return false;
    }
    edits.push_back(
        {creative::CreativeTerrainEditKind::Upsert, control});
  }
  creative::CreativeTerrainField restored;
  const creative::CreativeTerrainMutationReceipt receipt = restored.apply(edits);
  if (!receipt.accepted || (edits.empty() ? receipt.changed : !receipt.changed)) {
    return false;
  }
  out = std::move(restored);
  return true;
}

[[nodiscard]] SaveCreativeDocumentTerrainHeightFieldRecord
toSaveTerrainHeightField(
    const creative::CreativeTerrainHeightField& field) {
  SaveCreativeDocumentTerrainHeightFieldRecord record;
  if (field.cellCount() == 0U) {
    return record;
  }
  const creative::CreativeTerrainHeightFieldBounds bounds = field.bounds();
  record.present = true;
  record.minimumX = bounds.minimum.x;
  record.minimumZ = bounds.minimum.z;
  record.widthCells = bounds.widthCells;
  record.depthCells = bounds.depthCells;
  record.heights.assign(field.heights().begin(), field.heights().end());
  return record;
}

[[nodiscard]] bool toCreativeTerrainHeightField(
    const SaveCreativeDocumentTerrainHeightFieldRecord& record,
    creative::CreativeTerrainHeightField& output) {
  if (!record.present) {
    if (record.minimumX != 0 || record.minimumZ != 0 ||
        record.widthCells != 0U || record.depthCells != 0U ||
        !record.heights.empty()) {
      return false;
    }
    output.clear();
    return true;
  }
  creative::CreativeTerrainHeightField restored;
  const creative::CreativeTerrainHeightFieldReplaceReceipt receipt =
      restored.replace(
          {{record.minimumX, record.minimumZ}, record.widthCells,
           record.depthCells},
          record.heights);
  if (!receipt.accepted || !receipt.changed) {
    return false;
  }
  output = std::move(restored);
  return true;
}

[[nodiscard]] std::vector<SaveCreativeDocumentTerrainHardEdgeRecord>
toSaveTerrainHardEdges(
    std::span<const creative::CreativeTerrainHardEdge> edges) {
  std::vector<SaveCreativeDocumentTerrainHardEdgeRecord> records;
  records.reserve(edges.size());
  for (const creative::CreativeTerrainHardEdge edge : edges) {
    records.push_back(
        {edge.first.x, edge.first.z, edge.second.x, edge.second.z});
  }
  return records;
}

[[nodiscard]] bool toCreativeTerrainHardEdges(
    std::span<const SaveCreativeDocumentTerrainHardEdgeRecord> records,
    std::vector<creative::CreativeTerrainHardEdge>& output) {
  if (records.size() > creative::kCreativeTerrainHardEdgeCapacity) {
    return false;
  }
  std::vector<creative::CreativeTerrainHardEdge> restored;
  restored.reserve(records.size());
  for (const SaveCreativeDocumentTerrainHardEdgeRecord& record : records) {
    restored.push_back({{record.firstX, record.firstZ},
                        {record.secondX, record.secondZ}});
  }
  if (!creative::validateCreativeTerrainHardEdges(restored)) {
    return false;
  }
  output = std::move(restored);
  return true;
}

[[nodiscard]] std::vector<SaveCreativeDocumentTerrainOperationRecord>
toSaveTerrainOperations(
    const creative::CreativeTerrainOperationStack& stack) {
  std::vector<SaveCreativeDocumentTerrainOperationRecord> records;
  records.reserve(stack.operations.size());
  for (const creative::CreativeTerrainOperation& operation :
       stack.operations) {
    SaveCreativeDocumentTerrainOperationRecord record;
    record.id = operation.id;
    record.enabled = operation.enabled;
    record.owner = std::string(creative::toString(operation.owner));
    record.sourceKey = operation.sourceKey;
    record.operationKind = std::string(creative::toString(operation.kind));
    record.generationVersion = operation.generation.version;
    record.generatorKind =
        std::string(creative::toString(operation.generation.kind));
    record.seed = operation.generation.seed;
    record.minimumX = operation.generation.bounds.minimum.x;
    record.minimumZ = operation.generation.bounds.minimum.z;
    record.widthCells = operation.generation.bounds.widthCells;
    record.depthCells = operation.generation.bounds.depthCells;
    record.baseHeightCells = operation.generation.baseHeightCells;
    record.reliefCells = operation.generation.reliefCells;
    record.horizontalScaleCells = operation.generation.horizontalScaleCells;
    record.octaveCount = operation.generation.octaveCount;
    record.persistence = operation.generation.persistence;
    record.lacunarity = operation.generation.lacunarity;
    record.slopeDamping = operation.generation.slopeDamping;
    record.paintMaterials = operation.generation.paintMaterials;
    record.biomeIntent =
        std::string(creative::toString(operation.generation.biomeIntent));
    record.lowlandMaterial =
        std::string(creative::toString(operation.generation.lowlandMaterial));
    record.highlandMaterial =
        std::string(creative::toString(operation.generation.highlandMaterial));
    record.materialTransitionHeightCells =
        operation.generation.materialTransitionHeightCells;
    record.compositionVersion = operation.composition.version;
    record.mask = std::string(creative::toString(operation.composition.mask));
    record.mode = std::string(creative::toString(operation.composition.mode));
    record.featherCells = operation.composition.featherCells;
    record.protectedRegions.reserve(
        operation.composition.protectedRegionCount);
    for (std::size_t regionIndex = 0U;
         regionIndex < operation.composition.protectedRegionCount;
         ++regionIndex) {
      const creative::CreativeTerrainCompositionProtectedRegion& region =
          operation.composition.protectedRegions[regionIndex];
      record.protectedRegions.push_back(
          {region.bounds.minimum.x, region.bounds.minimum.z,
           region.bounds.widthCells, region.bounds.depthCells,
           std::string(creative::toString(region.mask))});
    }
    record.regionVersion = operation.region.version;
    record.regionMinimumX = operation.region.bounds.minimum.x;
    record.regionMinimumZ = operation.region.bounds.minimum.z;
    record.regionWidthCells = operation.region.bounds.widthCells;
    record.regionDepthCells = operation.region.bounds.depthCells;
    record.regionMask = std::string(creative::toString(operation.region.mask));
    record.regionMode = std::string(creative::toString(operation.region.mode));
    record.regionAmountCells = operation.region.amountCells;
    record.regionTargetHeightCells = operation.region.targetHeightCells;
    record.regionNoiseReliefCells = operation.region.noiseReliefCells;
    record.regionNoiseScaleCells = operation.region.noiseScaleCells;
    record.regionFeatherCells = operation.region.featherCells;
    record.regionSeed = operation.region.seed;
    record.gradeVersion = operation.grade.version;
    record.gradeStartX = operation.grade.start.x;
    record.gradeStartZ = operation.grade.start.z;
    record.gradeEndX = operation.grade.end.x;
    record.gradeEndZ = operation.grade.end.z;
    record.gradeStartHeightCells = operation.grade.startHeightCells;
    record.gradeEndHeightCells = operation.grade.endHeightCells;
    record.gradeHalfWidthCells = operation.grade.halfWidthCells;
    record.gradeCrossSlopePermille = operation.grade.crossSlopePermille;
    record.gradeFalloffCells = operation.grade.falloffCells;
    record.profileVersion = operation.profile.version;
    record.profileKind =
        std::string(creative::toString(operation.profile.profile));
    record.profileBlend =
        std::string(creative::toString(operation.profile.blend));
    record.profileRodPolicy =
        std::string(creative::toString(operation.profile.rodPolicy));
    record.profileCenterX = operation.profile.center.x;
    record.profileCenterZ = operation.profile.center.z;
    record.profileBaseHeightCells = operation.profile.baseHeightCells;
    record.profileRadiusCells = operation.profile.radiusCells;
    record.profileAmplitudeCells = operation.profile.amplitudeCells;
    record.profileSpacingCells = operation.profile.spacingCells;
    record.profileDirection =
        std::string(creative::toString(operation.profile.direction));
    record.profileFrequency = operation.profile.frequency;
    record.profileSeed = operation.profile.seed;
    record.landformVersion = operation.landform.version;
    record.landformKind =
        std::string(creative::toString(operation.landform.kind));
    record.landformMinimumX = operation.landform.bounds.minimum.x;
    record.landformMinimumZ = operation.landform.bounds.minimum.z;
    record.landformWidthCells = operation.landform.bounds.widthCells;
    record.landformDepthCells = operation.landform.bounds.depthCells;
    record.landformBaseHeightCells = operation.landform.baseHeightCells;
    record.landformTargetHeightCells = operation.landform.targetHeightCells;
    record.landformTerraceCount = operation.landform.terraceCount;
    record.landformDirection =
        std::string(creative::toString(operation.landform.direction));
    record.landformEdge =
        std::string(creative::toString(operation.landform.edge));
    record.landformEdgeWidthCells = operation.landform.edgeWidthCells;
    record.landformFeatherCells = operation.landform.featherCells;
    record.landformPaintSurface = operation.landform.paintSurface;
    record.landformMaterial =
        std::string(creative::toString(operation.landform.material));
    record.landformErosion =
        std::string(creative::toString(operation.landform.erosion));
    record.landformErosionReliefCells =
        operation.landform.erosionReliefCells;
    record.landformSeed = operation.landform.seed;
    record.pathVersion = operation.path.version;
    record.pathKind = std::string(creative::toString(operation.path.kind));
    record.pathElevation =
        std::string(creative::toString(operation.path.elevation));
    record.pathCurve = std::string(creative::toString(operation.path.curve));
    record.pathCrossSection =
        std::string(creative::toString(operation.path.crossSection));
    record.pathStartJoin =
        std::string(creative::toString(operation.path.startJoin));
    record.pathEndJoin =
        std::string(creative::toString(operation.path.endJoin));
    record.pathFalloffCells = operation.path.falloffCells;
    record.pathPaintSurface = operation.path.paintSurface;
    record.pathMaterial =
        std::string(creative::toString(operation.path.material));
    record.pathRoadShoulderWidthCells =
        operation.path.road.shoulderWidthCells;
    record.pathRoadMaximumGradePermille =
        operation.path.road.maximumGradePermille;
    record.pathRoadEdgeTreatment = static_cast<std::uint8_t>(
        operation.path.road.edgeTreatment);
    record.pathRoadEdgeWidthMeters = operation.path.road.edgeWidthMeters;
    record.pathRoadEdgeHeightMeters = operation.path.road.edgeHeightMeters;
    record.pathRoadEdgeMaterial =
        static_cast<std::uint8_t>(operation.path.road.edgeMaterial);
    record.pathWatercourseBankSlopeCells =
        operation.path.watercourse.bankSlopeCells;
    record.pathWatercourseDrainageDirection = static_cast<std::uint8_t>(
        operation.path.watercourse.drainageDirection);
    record.pathWatercourseSurfacePolicy = static_cast<std::uint8_t>(
        operation.path.watercourse.surfacePolicy);
    record.pathWatercourseSurfaceInsetCells =
        operation.path.watercourse.surfaceInsetCells;
    record.pathWatercourseNextCrossingId =
        operation.path.watercourse.nextCrossingId;
    record.pathWatercourseCrossings.reserve(
        operation.path.watercourse.crossings.size());
    for (const creative::CreativeTerrainWatercourseCrossing& crossing :
         operation.path.watercourse.crossings) {
      record.pathWatercourseCrossings.push_back(
          {crossing.id, crossing.pointId, crossing.bankClearanceCells,
           crossing.deckClearanceCells, crossing.approachLengthCells});
    }
    record.pathNextPointId = operation.path.nextPointId;
    record.pathPoints.reserve(operation.path.points.size());
    for (const creative::CreativeTerrainPathSourcePoint& point :
         operation.path.points) {
      record.pathPoints.push_back({point.id,
                                   point.coord.x,
                                   point.coord.z,
                                   point.heightCells,
                                   point.halfWidthCells,
                                   point.amplitudeCells,
                                   point.bankPermille});
    }
    if (operation.kind == creative::CreativeTerrainOperationKind::Stamp) {
      record.stampRecipeVersion = operation.stamp.version;
      record.stampVersion = operation.stamp.stamp.version;
      record.stampAssetId = operation.stamp.stamp.assetId;
      record.stampLabel = operation.stamp.stamp.label;
      record.stampAssetVersion = operation.stamp.stamp.assetVersion;
      record.stampSourceDocumentId =
          operation.stamp.stamp.sourceDocumentId;
      record.stampSourceRevision = operation.stamp.stamp.sourceRevision;
      record.stampContentSignature =
          operation.stamp.stamp.contentSignature;
      record.stampSourceMinimumX =
          operation.stamp.stamp.sourceMinimum.x;
      record.stampSourceMinimumZ =
          operation.stamp.stamp.sourceMinimum.z;
      record.stampMinimumHeightCells =
          operation.stamp.stamp.minimumHeightCells;
      record.stampHeightField.present = true;
      record.stampHeightField.widthCells =
          operation.stamp.stamp.widthCells;
      record.stampHeightField.depthCells =
          operation.stamp.stamp.depthCells;
      record.stampHeightField.heights = operation.stamp.stamp.heights;
      for (std::uint16_t localZ = 0U;
           localZ < operation.stamp.stamp.depthCells; ++localZ) {
        for (std::uint16_t localX = 0U;
             localX < operation.stamp.stamp.widthCells; ++localX) {
          const std::size_t stampIndex =
              static_cast<std::size_t>(localZ) *
                  operation.stamp.stamp.widthCells +
              localX;
          const creative::CreativeTerrainMaterialWeights& weights =
              operation.stamp.stamp.materials[stampIndex];
          if (weights == creative::creativeTerrainMaterialSolidWeights(
                             creative::CreativeTerrainMaterial::Grass)) {
            continue;
          }
          SaveCreativeDocumentTerrainMaterialRecord material;
          material.x = localX;
          material.z = localZ;
          material.material = std::string(creative::toString(
              creative::dominantCreativeTerrainMaterial(weights)));
          material.hasWeights = true;
          material.grassWeight = weights[static_cast<std::size_t>(
              creative::CreativeTerrainMaterial::Grass)];
          material.dirtWeight = weights[static_cast<std::size_t>(
              creative::CreativeTerrainMaterial::Dirt)];
          material.stoneWeight = weights[static_cast<std::size_t>(
              creative::CreativeTerrainMaterial::Stone)];
          material.sandWeight = weights[static_cast<std::size_t>(
              creative::CreativeTerrainMaterial::Sand)];
          record.stampMaterials.push_back(std::move(material));
        }
      }
      record.stampTargetMinimumX = operation.stamp.targetMinimum.x;
      record.stampTargetMinimumZ = operation.stamp.targetMinimum.z;
      record.stampQuarterTurns = operation.stamp.quarterTurns;
      record.stampMirrorX = operation.stamp.mirrorX;
      record.stampMirrorZ = operation.stamp.mirrorZ;
      record.stampMode =
          std::string(creative::toString(operation.stamp.mode));
      record.stampElevation =
          std::string(creative::toString(operation.stamp.elevationMode));
      record.stampManualHeightOffsetCells =
          operation.stamp.manualHeightOffsetCells;
    }
    records.push_back(std::move(record));
  }
  return records;
}

[[nodiscard]] bool toCreativeTerrainOperationStack(
    std::span<const SaveCreativeDocumentTerrainOperationRecord> records,
    std::uint32_t sectionVersion,
    std::uint32_t stackVersion,
    creative::CreativeTerrainOperationId nextOperationId,
    const SaveCreativeDocumentTerrainHeightFieldRecord& baseHeightField,
    std::span<const SaveCreativeDocumentTerrainHardEdgeRecord> baseHardEdges,
    std::span<const SaveCreativeDocumentTerrainMaterialRecord> baseMaterials,
    const creative::CreativeTerrainMaterialField& legacyFinalMaterial,
    creative::CreativeTerrainOperationStack& output) {
  if (records.size() > creative::kCreativeTerrainOperationCapacity) {
    return false;
  }
  creative::CreativeTerrainOperationStack restored;
  restored.version = stackVersion <= 9U
                         ? creative::kCreativeTerrainOperationStackVersion
                         : stackVersion;
  restored.nextOperationId = nextOperationId;
  if (!toCreativeTerrainHeightField(baseHeightField,
                                    restored.baseHeightField)) {
    return false;
  }
  if (!toCreativeTerrainHardEdges(baseHardEdges,
                                  restored.baseHardEdges)) {
    return false;
  }
  if (sectionVersion >= kSaveCreativeDocumentTerrainPathVersion) {
    if (!toCreativeTerrainMaterialField(baseMaterials,
                                        restored.baseMaterialField)) {
      return false;
    }
  } else if (!records.empty()) {
    restored.baseMaterialField = legacyFinalMaterial;
  }
  restored.operations.reserve(records.size());
  for (const SaveCreativeDocumentTerrainOperationRecord& record : records) {
    creative::CreativeTerrainOperation operation;
    operation.id = record.id;
    operation.enabled = record.enabled;
    if (sectionVersion >=
        kSaveCreativeDocumentTerrainOperationProvenanceVersion) {
      if (!creative::parseCreativeTerrainOperationOwner(record.owner,
                                                        operation.owner) ||
          record.sourceKey.size() >
              creative::kCreativeTerrainOperationSourceKeyCapacity) {
        return false;
      }
      operation.sourceKey = record.sourceKey;
    }
    if (!creative::parseCreativeTerrainOperationKind(record.operationKind,
                                                     operation.kind)) {
      return false;
    }
    operation.generation.version =
        sectionVersion >= kSaveCreativeDocumentTerrainGenerationIntentVersion
            ? record.generationVersion
            : creative::kCreativeTerrainGeneratorRecipeVersion;
    if (!creative::parseCreativeTerrainGeneratorKind(
            record.generatorKind, operation.generation.kind)) {
      return false;
    }
    operation.generation.seed = record.seed;
    operation.generation.bounds = {
        {record.minimumX, record.minimumZ}, record.widthCells,
        record.depthCells};
    operation.generation.baseHeightCells = record.baseHeightCells;
    operation.generation.reliefCells = record.reliefCells;
    operation.generation.horizontalScaleCells = record.horizontalScaleCells;
    operation.generation.octaveCount = record.octaveCount;
    operation.generation.persistence = record.persistence;
    operation.generation.lacunarity = record.lacunarity;
    operation.generation.slopeDamping = record.slopeDamping;
    if (sectionVersion >=
        kSaveCreativeDocumentTerrainGenerationIntentVersion) {
      operation.generation.paintMaterials = record.paintMaterials;
      if (!creative::parseCreativeTerrainBiomeIntent(
              record.biomeIntent, operation.generation.biomeIntent) ||
          !creative::parseCreativeTerrainMaterial(
              record.lowlandMaterial,
              operation.generation.lowlandMaterial) ||
          !creative::parseCreativeTerrainMaterial(
              record.highlandMaterial,
              operation.generation.highlandMaterial)) {
        return false;
      }
      operation.generation.materialTransitionHeightCells =
          record.materialTransitionHeightCells;
    } else {
      operation.generation.paintMaterials = false;
      operation.generation.biomeIntent =
          creative::CreativeTerrainBiomeIntent::Custom;
      operation.generation.lowlandMaterial =
          creative::CreativeTerrainMaterial::Grass;
      operation.generation.highlandMaterial =
          creative::CreativeTerrainMaterial::Grass;
      operation.generation.materialTransitionHeightCells =
          creative::kCreativeTerrainMinimumHeightCells;
    }
    operation.composition.version =
        sectionVersion >= kSaveCreativeDocumentTerrainGenerationIntentVersion
            ? record.compositionVersion
            : creative::kCreativeTerrainCompositionRecipeVersion;
    if (!creative::parseCreativeTerrainCompositionMask(
            record.mask, operation.composition.mask) ||
        !creative::parseCreativeTerrainCompositionMode(
            record.mode, operation.composition.mode)) {
      return false;
    }
    operation.composition.featherCells = record.featherCells;
    if (sectionVersion >=
        kSaveCreativeDocumentTerrainGenerationIntentVersion) {
      if (record.protectedRegions.size() >
          creative::kCreativeTerrainCompositionProtectedRegionCapacity) {
        return false;
      }
      for (const SaveCreativeDocumentTerrainProtectedRegionRecord& savedRegion :
           record.protectedRegions) {
        creative::CreativeTerrainCompositionMask savedMask =
            creative::CreativeTerrainCompositionMask::Count;
        if (!creative::parseCreativeTerrainCompositionMask(savedRegion.mask,
                                                            savedMask)) {
          return false;
        }
        const creative::CreativeTerrainProtectedRegionMutationReceipt added =
            creative::addCreativeTerrainCompositionProtectedRegion(
                operation.composition,
                {{{savedRegion.minimumX, savedRegion.minimumZ},
                  savedRegion.widthCells, savedRegion.depthCells},
                 savedMask});
        if (!added.accepted || !added.changed) {
          return false;
        }
      }
    }
    if (sectionVersion >= kSaveCreativeDocumentTerrainRegionVersion) {
      operation.region.version = record.regionVersion;
      operation.region.bounds = {
          {record.regionMinimumX, record.regionMinimumZ},
          record.regionWidthCells, record.regionDepthCells};
      if (!creative::parseCreativeTerrainCompositionMask(
              record.regionMask, operation.region.mask) ||
          !creative::parseCreativeTerrainRegionMode(
              record.regionMode, operation.region.mode)) {
        return false;
      }
      operation.region.amountCells = record.regionAmountCells;
      operation.region.targetHeightCells = record.regionTargetHeightCells;
      operation.region.noiseReliefCells = record.regionNoiseReliefCells;
      operation.region.noiseScaleCells = record.regionNoiseScaleCells;
      operation.region.featherCells = record.regionFeatherCells;
      operation.region.seed = record.regionSeed;
    }
    operation.grade.version = record.gradeVersion;
    operation.grade.start = {record.gradeStartX, record.gradeStartZ};
    operation.grade.end = {record.gradeEndX, record.gradeEndZ};
    operation.grade.startHeightCells = record.gradeStartHeightCells;
    operation.grade.endHeightCells = record.gradeEndHeightCells;
    operation.grade.halfWidthCells = record.gradeHalfWidthCells;
    operation.grade.crossSlopePermille = record.gradeCrossSlopePermille;
    operation.grade.falloffCells = record.gradeFalloffCells;
    if (sectionVersion >= kSaveCreativeDocumentTerrainProfileVersion) {
      operation.profile.version = record.profileVersion;
      operation.profile.center = {record.profileCenterX,
                                  record.profileCenterZ};
      operation.profile.baseHeightCells = record.profileBaseHeightCells;
      operation.profile.radiusCells = record.profileRadiusCells;
      operation.profile.amplitudeCells = record.profileAmplitudeCells;
      operation.profile.spacingCells = record.profileSpacingCells;
      operation.profile.frequency = record.profileFrequency;
      operation.profile.seed = record.profileSeed;
      if (!creative::parseCreativeTerrainProfileKind(
              record.profileKind, operation.profile.profile) ||
          !creative::parseCreativeTerrainProfileBlend(
              record.profileBlend, operation.profile.blend) ||
          !creative::parseCreativeTerrainProfileRodPolicy(
              record.profileRodPolicy, operation.profile.rodPolicy) ||
          !creative::parseCreativeTerrainProfileDirection(
              record.profileDirection, operation.profile.direction)) {
        return false;
      }
    } else if (operation.kind ==
               creative::CreativeTerrainOperationKind::Profile) {
      return false;
    }
    if (sectionVersion >= kSaveCreativeDocumentTerrainLandformVersion) {
      operation.landform.version = record.landformVersion;
      operation.landform.bounds = {
          {record.landformMinimumX, record.landformMinimumZ},
          record.landformWidthCells, record.landformDepthCells};
      operation.landform.baseHeightCells = record.landformBaseHeightCells;
      operation.landform.targetHeightCells = record.landformTargetHeightCells;
      operation.landform.terraceCount = record.landformTerraceCount;
      operation.landform.edgeWidthCells = record.landformEdgeWidthCells;
      operation.landform.featherCells = record.landformFeatherCells;
      operation.landform.paintSurface = record.landformPaintSurface;
      operation.landform.erosionReliefCells =
          record.landformErosionReliefCells;
      operation.landform.seed = record.landformSeed;
      if (!creative::parseCreativeTerrainLandformKind(
              record.landformKind, operation.landform.kind) ||
          !creative::parseCreativeTerrainLandformDirection(
              record.landformDirection, operation.landform.direction) ||
          !creative::parseCreativeTerrainLandformEdge(
              record.landformEdge, operation.landform.edge) ||
          !creative::parseCreativeTerrainMaterial(
              record.landformMaterial, operation.landform.material) ||
          !creative::parseCreativeTerrainLandformErosion(
              record.landformErosion, operation.landform.erosion)) {
        return false;
      }
    } else if (operation.kind ==
               creative::CreativeTerrainOperationKind::Landform) {
      return false;
    }
    if (operation.kind == creative::CreativeTerrainOperationKind::Path) {
      operation.path.version =
          (sectionVersion < kSaveCreativeDocumentTerrainRoadVersion &&
           record.pathVersion == 1U) ||
                  (sectionVersion <
                       kSaveCreativeDocumentTerrainWatercourseVersion &&
                   sectionVersion >= kSaveCreativeDocumentTerrainRoadVersion &&
                   record.pathVersion == 2U)
              ? creative::kCreativeTerrainPathSourceVersion
              : record.pathVersion;
      if (!creative::parseCreativeTerrainPathKind(record.pathKind,
                                                  operation.path.kind) ||
          !creative::parseCreativeTerrainPathElevation(
              record.pathElevation, operation.path.elevation) ||
          !creative::parseCreativeTerrainPathCurvePolicy(
              record.pathCurve, operation.path.curve) ||
          !creative::parseCreativeTerrainPathCrossSection(
              record.pathCrossSection, operation.path.crossSection) ||
          !creative::parseCreativeTerrainPathEndpointJoin(
              record.pathStartJoin, operation.path.startJoin) ||
          !creative::parseCreativeTerrainPathEndpointJoin(
              record.pathEndJoin, operation.path.endJoin) ||
          !creative::parseCreativeTerrainMaterial(record.pathMaterial,
                                                  operation.path.material) ||
          record.pathPoints.size() >
              creative::kCreativeTerrainPathPointCapacity) {
        return false;
      }
      operation.path.falloffCells = record.pathFalloffCells;
      operation.path.paintSurface = record.pathPaintSurface;
      if (sectionVersion >= kSaveCreativeDocumentTerrainRoadVersion) {
        if (record.pathRoadEdgeTreatment >= static_cast<std::uint8_t>(
                creative::CreativeTerrainRoadEdgeTreatment::Count) ||
            record.pathRoadEdgeMaterial >= static_cast<std::uint8_t>(
                creative::CreativeStructuralMaterial::Count)) {
          return false;
        }
        operation.path.road.shoulderWidthCells =
            record.pathRoadShoulderWidthCells;
        operation.path.road.maximumGradePermille =
            record.pathRoadMaximumGradePermille;
        operation.path.road.edgeTreatment =
            static_cast<creative::CreativeTerrainRoadEdgeTreatment>(
                record.pathRoadEdgeTreatment);
        operation.path.road.edgeWidthMeters =
            record.pathRoadEdgeWidthMeters;
        operation.path.road.edgeHeightMeters =
            record.pathRoadEdgeHeightMeters;
        operation.path.road.edgeMaterial =
            static_cast<creative::CreativeStructuralMaterial>(
                record.pathRoadEdgeMaterial);
      }
      if (sectionVersion >=
          kSaveCreativeDocumentTerrainWatercourseVersion) {
        if (record.pathWatercourseDrainageDirection >=
                static_cast<std::uint8_t>(
                    creative::CreativeTerrainWatercourseDrainageDirection::
                        Count) ||
            record.pathWatercourseSurfacePolicy >=
                static_cast<std::uint8_t>(
                    creative::CreativeTerrainWaterSurfacePolicy::Count) ||
            record.pathWatercourseCrossings.size() >
                creative::kCreativeTerrainWatercourseCrossingCapacity) {
          return false;
        }
        operation.path.watercourse.bankSlopeCells =
            record.pathWatercourseBankSlopeCells;
        operation.path.watercourse.drainageDirection = static_cast<
            creative::CreativeTerrainWatercourseDrainageDirection>(
            record.pathWatercourseDrainageDirection);
        operation.path.watercourse.surfacePolicy =
            static_cast<creative::CreativeTerrainWaterSurfacePolicy>(
                record.pathWatercourseSurfacePolicy);
        operation.path.watercourse.surfaceInsetCells =
            record.pathWatercourseSurfaceInsetCells;
        operation.path.watercourse.nextCrossingId =
            record.pathWatercourseNextCrossingId;
        operation.path.watercourse.crossings.reserve(
            record.pathWatercourseCrossings.size());
        for (const SaveCreativeDocumentTerrainWatercourseCrossingRecord&
                 crossing : record.pathWatercourseCrossings) {
          operation.path.watercourse.crossings.push_back(
              {crossing.id, crossing.pointId, crossing.bankClearanceCells,
               crossing.deckClearanceCells, crossing.approachLengthCells});
        }
      }
      operation.path.nextPointId = record.pathNextPointId;
      operation.path.points.reserve(record.pathPoints.size());
      for (const SaveCreativeDocumentTerrainPathPointRecord& point :
           record.pathPoints) {
        operation.path.points.push_back({point.id,
                                         {point.x, point.z},
                                         point.heightCells,
                                         point.halfWidthCells,
                                         point.amplitudeCells,
                                         point.bankPermille});
      }
    }
    if (operation.kind == creative::CreativeTerrainOperationKind::Stamp) {
      if (sectionVersion < kSaveCreativeDocumentTerrainStampVersion) {
        return false;
      }
      creative::CreativeTerrainHeightField stampHeight;
      creative::CreativeTerrainMaterialField stampMaterial;
      if (!toCreativeTerrainHeightField(record.stampHeightField,
                                        stampHeight) ||
          !toCreativeTerrainMaterialField(record.stampMaterials,
                                          stampMaterial)) {
        return false;
      }
      const creative::CreativeTerrainHeightFieldBounds stampBounds =
          stampHeight.bounds();
      if (stampBounds.minimum != creative::CreativeTerrainCoord2{} ||
          stampHeight.cellCount() == 0U) {
        return false;
      }
      operation.stamp.version = record.stampRecipeVersion;
      operation.stamp.stamp.version = record.stampVersion;
      operation.stamp.stamp.assetId = record.stampAssetId;
      operation.stamp.stamp.label = record.stampLabel;
      operation.stamp.stamp.assetVersion = record.stampAssetVersion;
      operation.stamp.stamp.sourceDocumentId =
          record.stampSourceDocumentId;
      operation.stamp.stamp.sourceRevision = record.stampSourceRevision;
      operation.stamp.stamp.contentSignature =
          record.stampContentSignature;
      operation.stamp.stamp.sourceMinimum = {
          record.stampSourceMinimumX, record.stampSourceMinimumZ};
      operation.stamp.stamp.widthCells = stampBounds.widthCells;
      operation.stamp.stamp.depthCells = stampBounds.depthCells;
      operation.stamp.stamp.minimumHeightCells =
          record.stampMinimumHeightCells;
      operation.stamp.stamp.heights.assign(stampHeight.heights().begin(),
                                           stampHeight.heights().end());
      operation.stamp.stamp.materials.assign(
          operation.stamp.stamp.heights.size(),
          creative::creativeTerrainMaterialSolidWeights(
              creative::CreativeTerrainMaterial::Grass));
      for (const creative::CreativeTerrainMaterialOverride& material :
           stampMaterial.overrides()) {
        if (material.coord.x < 0 || material.coord.z < 0 ||
            material.coord.x >= stampBounds.widthCells ||
            material.coord.z >= stampBounds.depthCells) {
          return false;
        }
        const std::size_t materialIndex =
            static_cast<std::size_t>(material.coord.z) *
                stampBounds.widthCells +
            static_cast<std::size_t>(material.coord.x);
        operation.stamp.stamp.materials[materialIndex] = material.weights;
      }
      operation.stamp.targetMinimum = {record.stampTargetMinimumX,
                                       record.stampTargetMinimumZ};
      operation.stamp.quarterTurns = record.stampQuarterTurns;
      operation.stamp.mirrorX = record.stampMirrorX;
      operation.stamp.mirrorZ = record.stampMirrorZ;
      if (!creative::parseCreativeTerrainStampMode(record.stampMode,
                                                   operation.stamp.mode) ||
          !creative::parseCreativeTerrainStampElevationMode(
              record.stampElevation, operation.stamp.elevationMode)) {
        return false;
      }
      operation.stamp.manualHeightOffsetCells =
          record.stampManualHeightOffsetCells;
    }
    restored.operations.push_back(std::move(operation));
  }
  if (!creative::validateCreativeTerrainOperationStack(restored) ||
      (restored.operations.empty() &&
       (restored.baseHeightField.cellCount() != 0U ||
        restored.baseMaterialField.overrideCount() != 0U ||
        !restored.baseHardEdges.empty()))) {
    return false;
  }
  output = std::move(restored);
  return true;
}

[[nodiscard]] std::vector<SaveCreativeDocumentPatternRecipeRecord>
toSavePatternRecipes(const creative::CreativePatternRecipeStore& store) {
  std::vector<SaveCreativeDocumentPatternRecipeRecord> records;
  records.reserve(store.recipes.size());
  for (const creative::CreativePatternRecipe& recipe : store.recipes) {
    SaveCreativeDocumentPatternRecipeRecord record;
    record.id = recipe.id;
    record.kind = std::string{creative::toString(recipe.kind)};
    record.sourceObjectIds.assign(recipe.sourceObjectIds.begin(),
                                  recipe.sourceObjectIds.end());
    record.generatedObjectIds.assign(recipe.generatedObjectIds.begin(),
                                     recipe.generatedObjectIds.end());
    record.linearDirection =
        std::string{creative::toString(recipe.linear.direction)};
    record.linearCopyCount =
        std::string{creative::toString(recipe.linear.copyCount)};
    record.linearSpacing =
        std::string{creative::toString(recipe.linear.spacing)};
    record.linearCellSize = recipe.linear.cellSize;
    record.linearMaxGeneratedObjects = recipe.linear.maxGeneratedObjects;
    record.radialPivot = {recipe.radial.pivot.x, recipe.radial.pivot.y,
                          recipe.radial.pivot.z};
    record.radialAxis = std::string{creative::toString(recipe.radial.axis)};
    record.radialInstanceCount =
        std::string{creative::toString(recipe.radial.instanceCount)};
    record.radialSweep =
        std::string{creative::toString(recipe.radial.sweep)};
    record.radialMaxGeneratedObjects = recipe.radial.maxGeneratedObjects;
    record.scatterObjectKind =
        std::string{creative::serializedObjectKindId(recipe.scatter.objectKind)};
    record.scatterAssetId = recipe.scatter.assetId;
    record.scatterAssetContentHash = recipe.scatter.assetContentHash;
    record.scatterAssetMaterialVariant = recipe.scatter.assetMaterialVariant;
    record.scatterAssetSourceBounds =
        toSaveBounds(recipe.scatter.assetSourceBounds);
    record.scatterPaintCenters.reserve(recipe.scatter.paintCenters.size());
    for (creative::CreativeVec3 center : recipe.scatter.paintCenters) {
      record.scatterPaintCenters.push_back({center.x, center.y, center.z});
    }
    record.scatterExclusions.reserve(recipe.scatter.exclusions.size());
    for (const creative::CreativeAssetScatterExclusion& exclusion :
         recipe.scatter.exclusions) {
      record.scatterExclusions.push_back(
          {{exclusion.center.x, exclusion.center.y, exclusion.center.z},
           exclusion.radiusMeters});
    }
    record.scatterMask = std::string{creative::toString(recipe.scatter.mask)};
    record.scatterYaw = std::string{creative::toString(recipe.scatter.yaw)};
    record.scatterBaseYawRadians = recipe.scatter.baseYawRadians;
    record.scatterRadiusMeters = recipe.scatter.radiusMeters;
    record.scatterSpacingMeters = recipe.scatter.spacingMeters;
    record.scatterDensityFraction = recipe.scatter.densityFraction;
    record.scatterScaleVariation = recipe.scatter.scaleVariation;
    record.scatterMaximumSlopeRadians = recipe.scatter.maximumSlopeRadians;
    record.scatterProjectToTerrainSurface =
        recipe.scatter.projectToTerrainSurface;
    record.scatterAvoidCollisions = recipe.scatter.avoidCollisions;
    record.scatterSeed = recipe.scatter.seed;
    record.scatterMaxGeneratedObjects = recipe.scatter.maxGeneratedObjects;
    records.push_back(std::move(record));
  }
  return records;
}

namespace {

[[nodiscard]] bool parsePatternAxis(
    std::string_view value, creative::CreativeAxis3& out) noexcept {
  if (value == "X") {
    out = creative::CreativeAxis3::X;
    return true;
  }
  if (value == "Y") {
    out = creative::CreativeAxis3::Y;
    return true;
  }
  if (value == "Z") {
    out = creative::CreativeAxis3::Z;
    return true;
  }
  return false;
}

}  // namespace

[[nodiscard]] bool toCreativePatternRecipeStore(
    std::span<const SaveCreativeDocumentPatternRecipeRecord> records,
    std::uint32_t storeVersion,
    creative::CreativePatternRecipeId nextRecipeId,
    creative::CreativePatternRecipeStore& output) {
  if (records.size() > creative::kCreativePatternRecipeCapacity) {
    return false;
  }
  if (storeVersion == 0U ||
      storeVersion > creative::kCreativePatternRecipeStoreVersion) {
    return false;
  }
  creative::CreativePatternRecipeStore restored;
  restored.version = creative::kCreativePatternRecipeStoreVersion;
  restored.nextRecipeId = nextRecipeId;
  restored.recipes.reserve(records.size());
  for (const SaveCreativeDocumentPatternRecipeRecord& record : records) {
    creative::CreativePatternRecipe recipe;
    recipe.id = record.id;
    if (!creative::parseCreativePatternRecipeKind(record.kind, recipe.kind) ||
        record.sourceObjectIds.size() >
            creative::kCreativePatternRecipeSourceObjectCapacity ||
        record.generatedObjectIds.size() >
            creative::kCreativeLinearArrayGeneratedObjectCapacity) {
      return false;
    }
    recipe.sourceObjectIds.assign(record.sourceObjectIds.begin(),
                                  record.sourceObjectIds.end());
    recipe.generatedObjectIds.assign(record.generatedObjectIds.begin(),
                                     record.generatedObjectIds.end());
    if (!creative::parseCreativeLinearArrayDirection(
            record.linearDirection, recipe.linear.direction) ||
        !creative::parseCreativeLinearArrayCopyCount(
            record.linearCopyCount, recipe.linear.copyCount) ||
        !creative::parseCreativeLinearArraySpacing(
            record.linearSpacing, recipe.linear.spacing) ||
        !parsePatternAxis(record.radialAxis, recipe.radial.axis) ||
        !creative::parseCreativeRadialArrayInstanceCount(
            record.radialInstanceCount, recipe.radial.instanceCount) ||
        !creative::parseCreativeRadialArraySweep(
            record.radialSweep, recipe.radial.sweep)) {
      return false;
    }
    recipe.linear.cellSize = record.linearCellSize;
    recipe.linear.maxGeneratedObjects = record.linearMaxGeneratedObjects;
    recipe.radial.pivot = {record.radialPivot.x, record.radialPivot.y,
                           record.radialPivot.z};
    recipe.radial.maxGeneratedObjects = record.radialMaxGeneratedObjects;
    if (recipe.kind == creative::CreativePatternRecipeKind::AssetScatter) {
      if (storeVersion < 2U ||
          !creative::parseSerializedObjectKindId(record.scatterObjectKind,
                                                 recipe.scatter.objectKind) ||
          !creative::parseCreativeAssetScatterRecipeMask(
              record.scatterMask, recipe.scatter.mask) ||
          !creative::parseCreativeAssetScatterRecipeYaw(
              record.scatterYaw, recipe.scatter.yaw) ||
          record.scatterPaintCenters.size() >
              creative::kCreativeAssetScatterPaintCenterCapacity ||
          record.scatterExclusions.size() >
              creative::kCreativeAssetScatterExclusionCapacity) {
        return false;
      }
      recipe.scatter.assetId = record.scatterAssetId;
      recipe.scatter.assetContentHash = record.scatterAssetContentHash;
      recipe.scatter.assetMaterialVariant =
          record.scatterAssetMaterialVariant;
      recipe.scatter.assetSourceBounds =
          toCreativeBounds(record.scatterAssetSourceBounds);
      recipe.scatter.paintCenters.reserve(record.scatterPaintCenters.size());
      for (const SaveCreativeDocumentVec3Record& center :
           record.scatterPaintCenters) {
        recipe.scatter.paintCenters.push_back({center.x, center.y, center.z});
      }
      recipe.scatter.exclusions.reserve(record.scatterExclusions.size());
      for (const SaveCreativeDocumentPatternRecipeRecord::Exclusion& exclusion :
           record.scatterExclusions) {
        recipe.scatter.exclusions.push_back(
            {{exclusion.center.x, exclusion.center.y, exclusion.center.z},
             exclusion.radiusMeters});
      }
      recipe.scatter.radiusMeters = record.scatterRadiusMeters;
      recipe.scatter.baseYawRadians = record.scatterBaseYawRadians;
      recipe.scatter.spacingMeters = record.scatterSpacingMeters;
      recipe.scatter.densityFraction = record.scatterDensityFraction;
      recipe.scatter.scaleVariation = record.scatterScaleVariation;
      recipe.scatter.maximumSlopeRadians =
          record.scatterMaximumSlopeRadians;
      recipe.scatter.projectToTerrainSurface =
          record.scatterProjectToTerrainSurface;
      recipe.scatter.avoidCollisions = record.scatterAvoidCollisions;
      recipe.scatter.seed = record.scatterSeed;
      recipe.scatter.maxGeneratedObjects =
          record.scatterMaxGeneratedObjects;
    }
    restored.recipes.push_back(std::move(recipe));
  }
  if (!creative::validateCreativePatternRecipeStore(restored)) {
    return false;
  }
  output = std::move(restored);
  return true;
}

namespace {

[[nodiscard]] std::string_view saveMeasurementMode(
    creative::CreativeMeasurementMode mode) noexcept {
  switch (mode) {
    case creative::CreativeMeasurementMode::Distance: return "Distance";
    case creative::CreativeMeasurementMode::AxisProjected:
      return "AxisProjected";
    case creative::CreativeMeasurementMode::Vertical: return "Vertical";
    case creative::CreativeMeasurementMode::Slope: return "Slope";
    case creative::CreativeMeasurementMode::Perimeter: return "Perimeter";
    case creative::CreativeMeasurementMode::Area: return "Area";
    case creative::CreativeMeasurementMode::Count: break;
  }
  return {};
}

[[nodiscard]] std::string_view saveMeasurementAxis(
    creative::CreativeMeasurementAxis axis) noexcept {
  switch (axis) {
    case creative::CreativeMeasurementAxis::X: return "X";
    case creative::CreativeMeasurementAxis::Y: return "Y";
    case creative::CreativeMeasurementAxis::Z: return "Z";
    case creative::CreativeMeasurementAxis::Count: break;
  }
  return {};
}

[[nodiscard]] std::string_view saveMeasurementSnapKind(
    creative::CreativeMeasurementSnapKind kind) noexcept {
  switch (kind) {
    case creative::CreativeMeasurementSnapKind::None: return "None";
    case creative::CreativeMeasurementSnapKind::Grid: return "Grid";
    case creative::CreativeMeasurementSnapKind::Surface: return "Surface";
    case creative::CreativeMeasurementSnapKind::Vertex: return "Vertex";
    case creative::CreativeMeasurementSnapKind::Opening: return "Opening";
    case creative::CreativeMeasurementSnapKind::Level: return "Level";
    case creative::CreativeMeasurementSnapKind::Count: break;
  }
  return {};
}

template <typename Enum, std::size_t Size>
[[nodiscard]] bool parseMeasurementEnum(
    std::string_view value,
    const std::array<std::pair<std::string_view, Enum>, Size>& rows,
    Enum& output) noexcept {
  const auto found = std::find_if(
      rows.begin(), rows.end(),
      [value](const auto& row) { return row.first == value; });
  if (found == rows.end()) {
    return false;
  }
  output = found->second;
  return true;
}

[[nodiscard]] bool parseMeasurementMode(
    std::string_view value,
    creative::CreativeMeasurementMode& output) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"Distance"},
                creative::CreativeMeasurementMode::Distance},
      std::pair{std::string_view{"AxisProjected"},
                creative::CreativeMeasurementMode::AxisProjected},
      std::pair{std::string_view{"Vertical"},
                creative::CreativeMeasurementMode::Vertical},
      std::pair{std::string_view{"Slope"},
                creative::CreativeMeasurementMode::Slope},
      std::pair{std::string_view{"Perimeter"},
                creative::CreativeMeasurementMode::Perimeter},
      std::pair{std::string_view{"Area"},
                creative::CreativeMeasurementMode::Area},
  };
  return parseMeasurementEnum(value, rows, output);
}

[[nodiscard]] bool parseMeasurementAxis(
    std::string_view value,
    creative::CreativeMeasurementAxis& output) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"X"}, creative::CreativeMeasurementAxis::X},
      std::pair{std::string_view{"Y"}, creative::CreativeMeasurementAxis::Y},
      std::pair{std::string_view{"Z"}, creative::CreativeMeasurementAxis::Z},
  };
  return parseMeasurementEnum(value, rows, output);
}

[[nodiscard]] bool parseMeasurementSnapKind(
    std::string_view value,
    creative::CreativeMeasurementSnapKind& output) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"None"},
                creative::CreativeMeasurementSnapKind::None},
      std::pair{std::string_view{"Grid"},
                creative::CreativeMeasurementSnapKind::Grid},
      std::pair{std::string_view{"Surface"},
                creative::CreativeMeasurementSnapKind::Surface},
      std::pair{std::string_view{"Vertex"},
                creative::CreativeMeasurementSnapKind::Vertex},
      std::pair{std::string_view{"Opening"},
                creative::CreativeMeasurementSnapKind::Opening},
      std::pair{std::string_view{"Level"},
                creative::CreativeMeasurementSnapKind::Level},
  };
  return parseMeasurementEnum(value, rows, output);
}

}  // namespace

std::vector<SaveCreativeDocumentMeasurementAnnotationRecord>
toSaveMeasurementAnnotations(
    const creative::CreativeMeasurementAnnotationStore& store) {
  std::vector<SaveCreativeDocumentMeasurementAnnotationRecord> records;
  records.reserve(store.annotations.size());
  for (const creative::CreativeMeasurementAnnotation& annotation :
       store.annotations) {
    SaveCreativeDocumentMeasurementAnnotationRecord record;
    record.id = annotation.id;
    record.name = annotation.name;
    record.mode = std::string{saveMeasurementMode(annotation.mode)};
    record.axis = std::string{saveMeasurementAxis(annotation.axis)};
    record.closePath = annotation.closePath;
    record.points.reserve(annotation.pointCount);
    for (std::size_t index = 0U; index < annotation.pointCount; ++index) {
      const creative::CreativeMeasurementAnnotationPoint& point =
          annotation.points[index];
      record.points.push_back(
          {point.x, point.y, point.z,
           std::string{saveMeasurementSnapKind(point.snapKind)}});
    }
    records.push_back(std::move(record));
  }
  return records;
}

bool toCreativeMeasurementAnnotationStore(
    std::span<const SaveCreativeDocumentMeasurementAnnotationRecord> records,
    std::uint32_t storeVersion,
    creative::CreativeMeasurementAnnotationId nextAnnotationId,
    creative::CreativeMeasurementAnnotationStore& output) {
  if (records.size() > creative::kCreativeMeasurementAnnotationCapacity) {
    return false;
  }
  creative::CreativeMeasurementAnnotationStore restored;
  restored.version = storeVersion;
  restored.nextAnnotationId = nextAnnotationId;
  restored.annotations.reserve(records.size());
  for (const SaveCreativeDocumentMeasurementAnnotationRecord& record : records) {
    if (record.points.size() >
        creative::kCreativeMeasurementAnnotationPointCapacity) {
      return false;
    }
    creative::CreativeMeasurementAnnotation annotation;
    annotation.id = record.id;
    annotation.name = record.name;
    annotation.closePath = record.closePath;
    annotation.pointCount = record.points.size();
    if (!parseMeasurementMode(record.mode, annotation.mode) ||
        !parseMeasurementAxis(record.axis, annotation.axis)) {
      return false;
    }
    for (std::size_t index = 0U; index < record.points.size(); ++index) {
      const SaveCreativeDocumentMeasurementAnnotationPointRecord& point =
          record.points[index];
      creative::CreativeMeasurementAnnotationPoint restoredPoint;
      restoredPoint.x = point.x;
      restoredPoint.y = point.y;
      restoredPoint.z = point.z;
      if (!parseMeasurementSnapKind(point.snapKind, restoredPoint.snapKind)) {
        return false;
      }
      annotation.points[index] = restoredPoint;
    }
    restored.annotations.push_back(std::move(annotation));
  }
  if (!creative::validateCreativeMeasurementAnnotationStore(restored)) {
    return false;
  }
  output = std::move(restored);
  return true;
}

[[nodiscard]] std::vector<SaveCreativeDocumentTerrainMaterialRecord>
toSaveTerrainMaterials(const creative::CreativeTerrainMaterialField& field) {
  std::vector<SaveCreativeDocumentTerrainMaterialRecord> records;
  records.reserve(static_cast<std::size_t>(field.overrideCount()));
  for (const creative::CreativeTerrainMaterialOverride& value :
       field.overrides()) {
    SaveCreativeDocumentTerrainMaterialRecord record;
    record.x = value.coord.x;
    record.z = value.coord.z;
    record.material = std::string(creative::toString(value.material));
    record.hasWeights = true;
    record.grassWeight = value.weights[static_cast<std::size_t>(
        creative::CreativeTerrainMaterial::Grass)];
    record.dirtWeight = value.weights[static_cast<std::size_t>(
        creative::CreativeTerrainMaterial::Dirt)];
    record.stoneWeight = value.weights[static_cast<std::size_t>(
        creative::CreativeTerrainMaterial::Stone)];
    record.sandWeight = value.weights[static_cast<std::size_t>(
        creative::CreativeTerrainMaterial::Sand)];
    records.push_back(std::move(record));
  }
  return records;
}

[[nodiscard]] bool toCreativeTerrainMaterialField(
    std::span<const SaveCreativeDocumentTerrainMaterialRecord> records,
    creative::CreativeTerrainMaterialField& output) {
  if (records.size() > creative::kCreativeTerrainMaterialOverrideCapacity) {
    return false;
  }
  std::vector<creative::CreativeTerrainMaterialEdit> edits;
  edits.reserve(records.size());
  for (const SaveCreativeDocumentTerrainMaterialRecord& record : records) {
    creative::CreativeTerrainMaterial material =
        creative::CreativeTerrainMaterial::Count;
    if (!creative::parseCreativeTerrainMaterial(record.material, material)) {
      return false;
    }
    if (!record.hasWeights) {
      if (material == creative::CreativeTerrainMaterial::Grass) {
        return false;
      }
      edits.push_back({creative::CreativeTerrainMaterialEditKind::Set,
                       {record.x, record.z}, material});
      continue;
    }
    const creative::CreativeTerrainMaterialWeights weights{{
        static_cast<std::uint8_t>(record.grassWeight),
        static_cast<std::uint8_t>(record.dirtWeight),
        static_cast<std::uint8_t>(record.stoneWeight),
        static_cast<std::uint8_t>(record.sandWeight),
    }};
    if (record.grassWeight > 255U || record.dirtWeight > 255U ||
        record.stoneWeight > 255U || record.sandWeight > 255U ||
        !creative::isValidCreativeTerrainMaterialWeights(weights) ||
        weights == creative::creativeTerrainMaterialSolidWeights(
                       creative::CreativeTerrainMaterial::Grass) ||
        creative::dominantCreativeTerrainMaterial(weights) != material) {
      return false;
    }
    edits.push_back(
        creative::makeCreativeTerrainMaterialWeightEdit({record.x, record.z},
                                                        weights));
  }
  creative::CreativeTerrainMaterialField restored;
  const creative::CreativeTerrainMaterialMutationReceipt receipt =
      restored.apply(edits);
  if (!receipt.accepted || (edits.empty() ? receipt.changed
                                         : !receipt.changed)) {
    return false;
  }
  output = std::move(restored);
  return true;
}

}  // namespace iggy3d::document_section_internal
