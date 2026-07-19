#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeTerrainGeneratorRecipe
makeDefaultCreativeEditorTerrainGeneratorRecipe() noexcept;

struct CreativeEditorTerrainGenerationState {
  iggy3d::creative::CreativeTerrainGeneratorRecipe recipe =
      makeDefaultCreativeEditorTerrainGeneratorRecipe();
  iggy3d::creative::CreativeTerrainGenerationResult generation{};
  iggy3d::creative::CreativeDocumentId sourceDocumentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t generationCount = 0U;
  bool previewActive = false;
  std::string statusMessage;
};

struct CreativeEditorTerrainGenerationPreviewReceipt {
  bool requested = false;
  bool accepted = false;
  bool regenerated = false;
  bool previewActive = false;
  std::uint64_t seed = 0U;
  std::uint64_t generationCount = 0U;
  std::string_view reasonCode =
      "creative_editor_terrain_generation_preview_not_requested";
};

struct CreativeEditorTerrainGenerationApplyReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  iggy3d::creative::CreativeTerrainHeightFieldReplaceReceipt replacement{};
  iggy3d::creative::CreativeHistoryRecordReceipt history{};
  std::string_view reasonCode =
      "creative_editor_terrain_generation_apply_not_requested";
};

void resetCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state) noexcept;

[[nodiscard]] bool creativeEditorTerrainGenerationPreviewMatches(
    const CreativeEditorTerrainGenerationState& state,
    const iggy3d::creative::CreativeDocument& document) noexcept;

// Cancels a transient preview when document truth changes underneath it.
// Returns true only when an active preview was invalidated.
[[nodiscard]] bool synchronizeCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    const iggy3d::creative::CreativeDocument& document);

[[nodiscard]] CreativeEditorTerrainGenerationPreviewReceipt
previewCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    const iggy3d::creative::CreativeDocument& document,
    bool advanceSeed);

[[nodiscard]] bool cancelCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    std::string_view reasonCode =
        "creative_editor_terrain_generation_preview_canceled");

// Commits the exact quantized preview through Facade under one history record.
[[nodiscard]] CreativeEditorTerrainGenerationApplyReceipt
applyCreativeEditorTerrainGeneration(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorTerrainGenerationState& state);

}  // namespace iggy3d_creative_app
