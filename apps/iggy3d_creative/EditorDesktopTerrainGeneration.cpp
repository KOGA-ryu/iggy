#include "EditorDesktopWidgets.hpp"

#include "EditorTerrainGeneration.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

namespace {

bool dragDouble(const char* label, double& value, float speed,
                double minimum, double maximum, const char* format) {
  return ImGui::DragScalar(label, ImGuiDataType_Double, &value, speed,
                           &minimum, &maximum, format,
                           ImGuiSliderFlags_AlwaysClamp);
}

void assignRegionOrigin(cr::CreativeTerrainHeightFieldBounds& bounds,
                        const std::array<int, 2U>& values) noexcept {
  bounds.minimum.x = static_cast<std::int32_t>(values[0]);
  bounds.minimum.z = static_cast<std::int32_t>(values[1]);
}

void assignRegionSize(cr::CreativeTerrainHeightFieldBounds& bounds,
                      const std::array<int, 2U>& values) noexcept {
  bounds.widthCells = static_cast<std::uint16_t>(std::clamp(
      values[0], 1, static_cast<int>(std::numeric_limits<std::uint16_t>::max())));
  bounds.depthCells = static_cast<std::uint16_t>(std::clamp(
      values[1], 1, static_cast<int>(std::numeric_limits<std::uint16_t>::max())));
}

}  // namespace

void buildCreativeEditorDesktopTerrainGenerationPanel(
    CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorTerrainGenerationState& state = editor.terrainGeneration;
  cr::CreativeTerrainGeneratorRecipe& recipe = state.recipe;
  const cr::CreativeDocument& document = appState.facade.document();
  const bool blocked = playModeActive || editor.assetEdit.active ||
                       creativeEditorWorldLayoutPreviewActive(
                           editor.worldLayout);

  bool recipeChanged = false;
  ImGui::BeginDisabled(blocked);
  ImGui::SeparatorText("Operations");
  if (ImGui::Button("New Operation")) {
    commands.push(CreativeDesktopCommandId::TerrainOperationNew);
  }
  ImGui::SameLine();
  ImGui::Text("%zu / %zu",
              document.terrainOperationStack().operations.size(),
              cr::kCreativeTerrainOperationCapacity);
  if (document.terrainOperationStack().operations.empty()) {
    ImGui::TextDisabled("No terrain operations");
  } else {
    const bool childVisible =
        ImGui::BeginChild("TerrainOperations", ImVec2{0.0F, 150.0F},
                          true);
    if (childVisible) {
      const std::vector<cr::CreativeTerrainOperation>& operations =
          document.terrainOperationStack().operations;
      for (std::size_t index = 0U; index < operations.size(); ++index) {
        const cr::CreativeTerrainOperation& operation = operations[index];
        ImGui::PushID(static_cast<int>(index));
        bool enabled = operation.enabled;
        if (ImGui::Checkbox("##enabled", &enabled)) {
          commands.push(
              CreativeDesktopCommandId::TerrainOperationSetEnabled,
              CreativeDesktopTerrainOperationPayload{operation.id, enabled,
                                                     index});
        }
        ImGui::SameLine();
        const std::string label =
            "#" + std::to_string(operation.id) + "  " +
            std::string(cr::toString(operation.composition.mode));
        const bool selected = state.editingOperationId == operation.id;
        if (ImGui::Selectable(label.c_str(), selected)) {
          commands.push(
              CreativeDesktopCommandId::TerrainOperationSelect,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        ImGui::BeginDisabled(index == 0U);
        if (ImGui::SmallButton("Up")) {
          commands.push(
              CreativeDesktopCommandId::TerrainOperationMove,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled,
                                                     index - 1U});
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(index + 1U >= operations.size());
        if (ImGui::SmallButton("Down")) {
          commands.push(
              CreativeDesktopCommandId::TerrainOperationMove,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled,
                                                     index + 1U});
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::SmallButton("Duplicate")) {
          commands.push(
              CreativeDesktopCommandId::TerrainOperationDuplicate,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Delete")) {
          commands.push(
              CreativeDesktopCommandId::TerrainOperationDelete,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        ImGui::PopID();
      }
    }
    ImGui::EndChild();
  }

  ImGui::SeparatorText("Region");
  std::array<int, 2U> origin{
      static_cast<int>(recipe.bounds.minimum.x),
      static_cast<int>(recipe.bounds.minimum.z),
  };
  if (ImGui::InputInt2("Origin X / Z", origin.data())) {
    assignRegionOrigin(recipe.bounds, origin);
    recipeChanged = true;
  }

  std::array<int, 2U> size{
      static_cast<int>(recipe.bounds.widthCells),
      static_cast<int>(recipe.bounds.depthCells),
  };
  if (ImGui::InputInt2("Width / Depth", size.data())) {
    assignRegionSize(recipe.bounds, size);
    recipeChanged = true;
  }

  ImGui::SeparatorText("Shape");
  recipeChanged =
      ImGui::InputScalar("Seed", ImGuiDataType_U64, &recipe.seed) ||
      recipeChanged;
  int baseHeight = static_cast<int>(recipe.baseHeightCells);
  if (ImGui::SliderInt(
          "Base height", &baseHeight,
          static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
          static_cast<int>(cr::kCreativeTerrainMaximumHeightCells))) {
    recipe.baseHeightCells = static_cast<std::uint16_t>(baseHeight);
    recipeChanged = true;
  }
  int relief = static_cast<int>(recipe.reliefCells);
  if (ImGui::SliderInt(
          "Relief", &relief, 0,
          static_cast<int>(cr::kCreativeTerrainMaximumHeightCells))) {
    recipe.reliefCells = static_cast<std::uint16_t>(relief);
    recipeChanged = true;
  }
  recipeChanged =
      dragDouble("Feature scale", recipe.horizontalScaleCells, 0.25F,
                 cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells,
                 cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells,
                 "%.2f cells") ||
      recipeChanged;
  int octaves = static_cast<int>(recipe.octaveCount);
  if (ImGui::SliderInt(
          "Detail layers", &octaves, 1,
          static_cast<int>(cr::kCreativeTerrainGeneratorMaximumOctaves))) {
    recipe.octaveCount = static_cast<std::uint8_t>(octaves);
    recipeChanged = true;
  }

  if (ImGui::CollapsingHeader("Advanced")) {
    recipeChanged =
        dragDouble("Persistence", recipe.persistence, 0.01F,
                   cr::kCreativeTerrainGeneratorMinimumPersistence,
                   cr::kCreativeTerrainGeneratorMaximumPersistence, "%.2f") ||
        recipeChanged;
    recipeChanged =
        dragDouble("Lacunarity", recipe.lacunarity, 0.01F,
                   cr::kCreativeTerrainGeneratorMinimumLacunarity,
                   cr::kCreativeTerrainGeneratorMaximumLacunarity, "%.2f") ||
        recipeChanged;
    recipeChanged =
        dragDouble("Slope damping", recipe.slopeDamping, 0.01F, 0.0,
                   cr::kCreativeTerrainGeneratorMaximumSlopeDamping, "%.2f") ||
        recipeChanged;
  }

  ImGui::SeparatorText("Composition");
  constexpr std::array<const char*, 2U> maskLabels{
      "Rectangle",
      "Ellipse",
  };
  int mask = static_cast<int>(state.compositionRecipe.mask);
  if (ImGui::Combo("Mask", &mask, maskLabels.data(),
                   static_cast<int>(maskLabels.size()))) {
    state.compositionRecipe.mask =
        static_cast<cr::CreativeTerrainCompositionMask>(mask);
    recipeChanged = true;
  }
  constexpr std::array<const char*, 3U> modeLabels{
      "Replace",
      "Raise",
      "Lower",
  };
  int mode = static_cast<int>(state.compositionRecipe.mode);
  if (ImGui::Combo("Mode", &mode, modeLabels.data(),
                   static_cast<int>(modeLabels.size()))) {
    state.compositionRecipe.mode =
        static_cast<cr::CreativeTerrainCompositionMode>(mode);
    recipeChanged = true;
  }
  int feather = static_cast<int>(state.compositionRecipe.featherCells);
  if (ImGui::DragInt(
          "Feather", &feather, 0.25F, 0,
          static_cast<int>(
              cr::kCreativeTerrainCompositionMaximumFeatherCells),
          "%d cells", ImGuiSliderFlags_AlwaysClamp)) {
    state.compositionRecipe.featherCells =
        static_cast<std::uint16_t>(feather);
    recipeChanged = true;
  }

  const std::uint64_t cellCount =
      static_cast<std::uint64_t>(recipe.bounds.widthCells) *
      recipe.bounds.depthCells;
  const bool recipeValid =
      cr::isValidCreativeTerrainGeneratorRecipe(recipe) &&
      cr::isValidCreativeTerrainCompositionRecipe(state.compositionRecipe);
  ImGui::Text("Cells  %llu / %llu",
              static_cast<unsigned long long>(cellCount),
              static_cast<unsigned long long>(
                  cr::kCreativeTerrainHeightFieldCellCapacity));
  if (!recipeValid) {
    ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                       "INVALID RECIPE");
  }

  if (recipeChanged && state.previewActive) {
    commands.push(CreativeDesktopCommandId::TerrainGenerationPreview);
  }
  state.draftDirty = state.draftDirty || recipeChanged;

  ImGui::Separator();
  if (!state.previewActive) {
    ImGui::BeginDisabled(!recipeValid);
    if (ImGui::Button("Preview Terrain")) {
      commands.push(CreativeDesktopCommandId::TerrainGenerationPreview);
    }
    ImGui::EndDisabled();
  } else {
    ImGui::BeginDisabled(!recipeValid);
    if (ImGui::Button("Regenerate")) {
      commands.push(CreativeDesktopCommandId::TerrainGenerationRegenerate);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool canApply =
        creativeEditorTerrainGenerationPreviewMatches(state, document);
    ImGui::BeginDisabled(!canApply);
    if (ImGui::Button(
            state.editingOperationId ==
                    cr::kInvalidCreativeTerrainOperationId
                ? "Add Operation"
                : "Update Operation")) {
      commands.push(CreativeDesktopCommandId::TerrainGenerationApply);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      commands.push(CreativeDesktopCommandId::TerrainGenerationCancel);
    }
  }
  ImGui::EndDisabled();

  if (state.generation.receipt.accepted) {
    const cr::CreativeTerrainGenerationReceipt& generation =
        state.generation.receipt;
    ImGui::Text("Height  %u - %u cells", generation.minimumHeightCells,
                generation.maximumHeightCells);
    ImGui::Text("Seed  %llu", static_cast<unsigned long long>(recipe.seed));
    if (state.operationPreview.receipt.accepted) {
      ImGui::TextColored(ImVec4{0.20F, 1.0F, 0.35F, 1.0F}, "READY");
      ImGui::Text("Modified  %llu  Feathered  %llu",
                  static_cast<unsigned long long>(
                      state.operationPreview.receipt.replay.modifiedCellCount),
                  static_cast<unsigned long long>(
                      state.operationPreview.receipt.replay.featheredCellCount));
      ImGui::Text(
          "Hash  %016llx",
          static_cast<unsigned long long>(
              state.operationPreview.receipt.replay.heightHash));
    }
  } else if (state.previewActive && state.generation.receipt.requested) {
    const std::string_view status =
        cr::toString(state.generation.receipt.status);
    ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%.*s",
                       static_cast<int>(status.size()), status.data());
  }
  if (state.previewActive && state.generation.receipt.accepted &&
      state.operationPreview.receipt.requested &&
      !state.operationPreview.receipt.accepted) {
    const std::string_view status =
        cr::toString(state.operationPreview.receipt.status);
    ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%.*s",
                       static_cast<int>(status.size()), status.data());
  }
  if (!state.statusMessage.empty()) {
    ImGui::TextDisabled("%s", state.statusMessage.c_str());
  }
}

}  // namespace iggy3d_creative_app
