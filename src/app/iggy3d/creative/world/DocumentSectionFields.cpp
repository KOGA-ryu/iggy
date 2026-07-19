#include "app/iggy3d/creative/world/DocumentSectionInternal.hpp"

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

[[nodiscard]] std::vector<SaveCreativeDocumentTerrainMaterialRecord>
toSaveTerrainMaterials(const creative::CreativeTerrainMaterialField& field) {
  std::vector<SaveCreativeDocumentTerrainMaterialRecord> records;
  records.reserve(static_cast<std::size_t>(field.overrideCount()));
  for (const creative::CreativeTerrainMaterialOverride& value :
       field.overrides()) {
    records.push_back(
        {value.coord.x, value.coord.z,
         std::string(creative::toString(value.material))});
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
    if (!creative::parseCreativeTerrainMaterial(record.material, material) ||
        material == creative::CreativeTerrainMaterial::Grass) {
      return false;
    }
    edits.push_back({creative::CreativeTerrainMaterialEditKind::Set,
                     {record.x, record.z}, material});
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
