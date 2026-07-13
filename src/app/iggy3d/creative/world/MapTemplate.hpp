#pragma once

#include "app/iggy3d/creative/document/Document.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::string_view kDitchHouseMapTemplateId = "ditch_house";

enum class CreativeMapTemplateStatus : std::uint8_t {
  NotRequested,
  UnknownTemplate,
  InvalidDocumentId,
  DocumentSetupFailed,
  TerrainFailed,
  TerrainMaterialFailed,
  ObjectBatchFailed,
  Ready,
};

struct CreativeMapTemplateResult {
  bool requested = false;
  bool accepted = false;
  CreativeMapTemplateStatus status =
      CreativeMapTemplateStatus::NotRequested;
  std::string_view templateId;
  std::string_view reasonCode = "creative_map_template_not_requested";
  CreativeObjectId primaryFloorObjectId = kInvalidObjectId;
  std::uint64_t objectCount = 0U;
  std::uint64_t terrainControlCount = 0U;
  std::uint64_t terrainMaterialOverrideCount = 0U;
  CreativeDocument document;
};

[[nodiscard]] bool isCreativeMapTemplateId(std::string_view templateId)
    noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMapTemplateStatus status) noexcept;
[[nodiscard]] CreativeMapTemplateResult buildCreativeMapTemplate(
    std::string_view templateId,
    CreativeDocumentId documentId = 1U);

}  // namespace iggy3d::creative
