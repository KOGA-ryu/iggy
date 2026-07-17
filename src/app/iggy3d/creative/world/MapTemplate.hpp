#pragma once

#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::string_view kDitchHouseMapTemplateId = "ditch_house";
inline constexpr std::string_view kBuilderEstateMapTemplateId =
    "builder_estate";
inline constexpr std::string_view kBuilderEstateHouseTemplateId =
    "builder_estate.house";

enum class CreativeMapTemplateStatus : std::uint8_t {
  NotRequested,
  UnknownTemplate,
  InvalidDocumentId,
  DocumentSetupFailed,
  TerrainFailed,
  TerrainMaterialFailed,
  ObjectBatchFailed,
  BuildingTemplateFailed,
  WorldLayoutFailed,
  ObjectRecipeFailed,
  Ready,
};

struct CreativeMapTemplateResult {
  bool requested = false;
  bool accepted = false;
  CreativeMapTemplateStatus status =
      CreativeMapTemplateStatus::NotRequested;
  std::string templateId;
  std::string reasonCode = "creative_map_template_not_requested";
  CreativeObjectId primaryFloorObjectId = kInvalidObjectId;
  std::uint64_t objectCount = 0U;
  std::uint64_t terrainControlCount = 0U;
  std::uint64_t terrainMaterialOverrideCount = 0U;
  std::uint64_t linkedBuildingInstanceCount = 0U;
  std::uint64_t roomSymbolCount = 0U;
  std::uint64_t openingSymbolCount = 0U;
  std::uint64_t supplementalRecipeCount = 0U;
  bool worldLayoutPresent = false;
  CreativeWorldLayout worldLayout;
  std::vector<CreativeWorldLayoutBuildingTemplate> buildingTemplates;
  CreativeDocument document;
};

[[nodiscard]] bool isCreativeMapTemplateId(std::string_view templateId)
    noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMapTemplateStatus status) noexcept;
[[nodiscard]] CreativeMapTemplateResult buildCreativeMapTemplate(
    std::string_view templateId,
    CreativeDocumentId documentId = 1U);

[[nodiscard]] std::span<const std::string_view>
creativeBuiltInBuildingTemplateIds() noexcept;
[[nodiscard]] CreativeWorldLayoutBuildingTemplateResult
buildCreativeBuiltInBuildingTemplate(std::string_view templateId);

}  // namespace iggy3d::creative
