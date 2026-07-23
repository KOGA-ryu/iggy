#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/world/DocumentSection.hpp"

namespace iggy3d::document_section_internal {

[[nodiscard]] SaveCreativeDocumentBoundsRecord toSaveBounds(
    creative::CreativeBounds bounds) noexcept;
[[nodiscard]] creative::CreativeBounds toCreativeBounds(
    SaveCreativeDocumentBoundsRecord record) noexcept;
[[nodiscard]] std::string_view toSaveUnits(
    creative::CreativeUnits units) noexcept;
[[nodiscard]] bool parseUnits(
    std::string_view value,
    creative::CreativeUnits& out) noexcept;
[[nodiscard]] bool toSaveGridSettings(
    creative::CreativeGridSettings settings,
    SaveCreativeDocumentSection& section) noexcept;
[[nodiscard]] bool toCreativeGridSettings(
    const SaveCreativeDocumentSection& section,
    creative::CreativeGridSettings& out) noexcept;
[[nodiscard]] bool toSaveSnapSettings(
    creative::CreativeDocumentSnapSettings settings,
    SaveCreativeDocumentSection& section) noexcept;
[[nodiscard]] bool toCreativeSnapSettings(
    const SaveCreativeDocumentSection& section,
    creative::CreativeDocumentSnapSettings& out) noexcept;
[[nodiscard]] SaveCreativeDocumentObjectRecord toSaveObject(
    const creative::CreativeObject& object);
[[nodiscard]] bool toCreativeObject(
    const SaveCreativeDocumentObjectRecord& record,
    creative::CreativeObject& out) noexcept;
[[nodiscard]] SaveCreativeDocumentLogicLinkRecord toSaveLogicLink(
    const creative::CreativeLogicLink& link);
[[nodiscard]] bool toCreativeLogicLink(
    const SaveCreativeDocumentLogicLinkRecord& record,
    creative::CreativeLogicLink& out) noexcept;

[[nodiscard]] std::vector<SaveCreativeDocumentVoxelChunkRecord>
toSaveVoxelChunks(const creative::CreativeVoxelField& field);
[[nodiscard]] bool toCreativeVoxelField(
    std::span<const SaveCreativeDocumentVoxelChunkRecord> records,
    creative::CreativeVoxelField& out);
[[nodiscard]] std::vector<SaveCreativeDocumentTerrainControlRecord>
toSaveTerrainControls(const creative::CreativeTerrainField& field);
[[nodiscard]] bool toCreativeTerrainField(
    std::span<const SaveCreativeDocumentTerrainControlRecord> records,
    creative::CreativeTerrainField& out);
[[nodiscard]] SaveCreativeDocumentTerrainHeightFieldRecord
toSaveTerrainHeightField(
    const creative::CreativeTerrainHeightField& field);
[[nodiscard]] bool toCreativeTerrainHeightField(
    const SaveCreativeDocumentTerrainHeightFieldRecord& record,
    creative::CreativeTerrainHeightField& output);
[[nodiscard]] std::vector<SaveCreativeDocumentTerrainHardEdgeRecord>
toSaveTerrainHardEdges(
    std::span<const creative::CreativeTerrainHardEdge> edges);
[[nodiscard]] bool toCreativeTerrainHardEdges(
    std::span<const SaveCreativeDocumentTerrainHardEdgeRecord> records,
    std::vector<creative::CreativeTerrainHardEdge>& output);
[[nodiscard]] std::vector<SaveCreativeDocumentTerrainOperationRecord>
toSaveTerrainOperations(
    const creative::CreativeTerrainOperationStack& stack);
[[nodiscard]] bool toCreativeTerrainOperationStack(
    std::span<const SaveCreativeDocumentTerrainOperationRecord> records,
    std::uint32_t sectionVersion,
    std::uint32_t stackVersion,
    creative::CreativeTerrainOperationId nextOperationId,
    const SaveCreativeDocumentTerrainHeightFieldRecord& baseHeightField,
    std::span<const SaveCreativeDocumentTerrainHardEdgeRecord> baseHardEdges,
    std::span<const SaveCreativeDocumentTerrainMaterialRecord> baseMaterials,
    const creative::CreativeTerrainMaterialField& legacyFinalMaterial,
    creative::CreativeTerrainOperationStack& output);
[[nodiscard]] std::vector<SaveCreativeDocumentPatternRecipeRecord>
toSavePatternRecipes(const creative::CreativePatternRecipeStore& store);
[[nodiscard]] bool toCreativePatternRecipeStore(
    std::span<const SaveCreativeDocumentPatternRecipeRecord> records,
    std::uint32_t storeVersion,
    creative::CreativePatternRecipeId nextRecipeId,
    creative::CreativePatternRecipeStore& output);
[[nodiscard]] std::vector<SaveCreativeDocumentMeasurementAnnotationRecord>
toSaveMeasurementAnnotations(
    const creative::CreativeMeasurementAnnotationStore& store);
[[nodiscard]] bool toCreativeMeasurementAnnotationStore(
    std::span<const SaveCreativeDocumentMeasurementAnnotationRecord> records,
    std::uint32_t storeVersion,
    creative::CreativeMeasurementAnnotationId nextAnnotationId,
    creative::CreativeMeasurementAnnotationStore& output);
[[nodiscard]] std::vector<SaveCreativeDocumentTerrainMaterialRecord>
toSaveTerrainMaterials(
    const creative::CreativeTerrainMaterialField& field);
[[nodiscard]] bool toCreativeTerrainMaterialField(
    std::span<const SaveCreativeDocumentTerrainMaterialRecord> records,
    creative::CreativeTerrainMaterialField& output);

}  // namespace iggy3d::document_section_internal
