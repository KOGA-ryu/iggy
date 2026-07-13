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
[[nodiscard]] std::vector<SaveCreativeDocumentTerrainMaterialRecord>
toSaveTerrainMaterials(
    const creative::CreativeTerrainMaterialField& field);
[[nodiscard]] bool toCreativeTerrainMaterialField(
    std::span<const SaveCreativeDocumentTerrainMaterialRecord> records,
    creative::CreativeTerrainMaterialField& output);

}  // namespace iggy3d::document_section_internal
