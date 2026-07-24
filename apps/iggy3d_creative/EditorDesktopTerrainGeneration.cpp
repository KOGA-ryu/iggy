#include "EditorDesktopWidgets.hpp"

#include "EditorTerrainGeneration.hpp"
#include "EditorTerrainStampLibrary.hpp"
#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/tools/RecipeTransform.hpp"

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

bool materialCombo(const char* label,
                   cr::CreativeTerrainMaterial& material) {
  constexpr std::array<const char*, 4U> labels{
      "Grass", "Dirt", "Stone", "Sand"};
  int selected = static_cast<int>(material);
  if (!ImGui::Combo(label, &selected, labels.data(),
                    static_cast<int>(labels.size()))) {
    return false;
  }
  material = static_cast<cr::CreativeTerrainMaterial>(selected);
  return true;
}

void drawTerrainStampThumbnail(
    const cr::CreativeTerrainStampThumbnail& thumbnail,
    bool selected) {
  constexpr float kThumbnailSize = 48.0F;
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  static_cast<void>(ImGui::InvisibleButton(
      "##terrain_stamp_thumbnail", {kThumbnailSize, kThumbnailSize}));
  const ImVec2 maximum{minimum.x + kThumbnailSize,
                       minimum.y + kThumbnailSize};
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(minimum, maximum,
                          IM_COL32(24, 28, 32, 255));
  const float cellSize =
      kThumbnailSize / cr::kCreativeTerrainStampThumbnailEdge;
  for (std::size_t z = 0U;
       z < cr::kCreativeTerrainStampThumbnailEdge; ++z) {
    for (std::size_t x = 0U;
         x < cr::kCreativeTerrainStampThumbnailEdge; ++x) {
      const cr::CreativeTerrainStampThumbnailPixel& pixel =
          thumbnail.pixels[
              z * cr::kCreativeTerrainStampThumbnailEdge + x];
      if (!pixel.present) {
        continue;
      }
      const cr::CreativeVec3 source =
          cr::creativeTerrainMaterialRenderColor(
              cr::creativeTerrainMaterialSolidWeights(pixel.material));
      const float shade = 0.45F +
                          0.55F * static_cast<float>(pixel.height) / 255.0F;
      const ImU32 color = ImGui::ColorConvertFloat4ToU32(
          {static_cast<float>(source.x) * shade,
           static_cast<float>(source.y) * shade,
           static_cast<float>(source.z) * shade, 1.0F});
      const ImVec2 cellMinimum{
          minimum.x + static_cast<float>(x) * cellSize,
          minimum.y + static_cast<float>(z) * cellSize};
      const ImVec2 cellMaximum{cellMinimum.x + cellSize,
                               cellMinimum.y + cellSize};
      drawList->AddRectFilled(cellMinimum, cellMaximum, color);
    }
  }
  drawList->AddRect(
      minimum, maximum,
      selected ? IM_COL32(68, 224, 112, 255)
               : IM_COL32(105, 116, 128, 255),
      0.0F, 0, selected ? 2.0F : 1.0F);
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
  const bool profileEditing =
      editor.terrain.profile.editingOperationId !=
      cr::kInvalidCreativeTerrainOperationId;
  const bool generatedOperation =
      !profileEditing &&
      state.operationKind == cr::CreativeTerrainOperationKind::GeneratedTerrain;
  ImGui::BeginDisabled(blocked);
  ImGui::SeparatorText("Terrain Stamps");
  creativeDesktopInputTextStdString(
      "Name##terrain_stamp", &editor.terrainStamps.captureLabel);
  const bool stampNameValid =
      !editor.terrainStamps.captureLabel.empty() &&
      editor.terrainStamps.captureLabel.size() <=
          cr::kCreativeTerrainStampLabelCapacity;
  const bool regionSelected =
      cr::creativeVolumeSelectionValid(editor.volume.selection);
  const bool stampCapacityAvailable =
      editor.terrainStamps.library.stamps.size() <
      cr::kCreativeTerrainStampLibraryCapacity;
  ImGui::BeginDisabled(!stampNameValid || !regionSelected ||
                       !stampCapacityAvailable);
  if (ImGui::Button("Save Selected Region")) {
    commands.enqueue(
        CreativeDesktopCommandId::TerrainStampSaveSelection,
        CreativeDesktopTerrainStampPayload{
            {}, editor.terrainStamps.captureLabel,
            cr::kInvalidCreativeTerrainOperationId, false});
  }
  ImGui::EndDisabled();
  if (!regionSelected) {
    ImGui::TextDisabled("Select a terrain volume to save it as a stamp.");
  } else if (!stampNameValid) {
    ImGui::TextDisabled("Name must contain 1-%zu characters.",
                        cr::kCreativeTerrainStampLabelCapacity);
  } else if (!stampCapacityAvailable) {
    ImGui::TextDisabled("Terrain stamp library is full.");
  }

  const std::vector<cr::CreativeTerrainStampCatalogEntry> stampCatalog =
      cr::buildCreativeTerrainStampCatalog(editor.terrainStamps.library);
  if (stampCatalog.empty()) {
    ImGui::TextDisabled("No saved terrain stamps");
  }
  for (const cr::CreativeTerrainStampCatalogEntry& entry : stampCatalog) {
    ImGui::PushID(entry.assetId.c_str());
    drawTerrainStampThumbnail(
        entry.thumbnail,
        editor.terrainStamps.library.selectedAssetId == entry.assetId);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted(entry.label.c_str());
    ImGui::TextDisabled("%u x %u cells  |  %llu cells  |  v%llu",
                        entry.widthCells, entry.depthCells,
                        static_cast<unsigned long long>(entry.presentCellCount),
                        static_cast<unsigned long long>(entry.assetVersion));
    ImGui::BeginDisabled(!entry.compatible);
    if (ImGui::SmallButton("Place")) {
      commands.enqueue(
          CreativeDesktopCommandId::TerrainStampSelect,
          CreativeDesktopTerrainStampPayload{
              entry.assetId, {}, cr::kInvalidCreativeTerrainOperationId,
              false});
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::SmallButton("Delete")) {
      commands.enqueue(
          CreativeDesktopCommandId::TerrainStampDelete,
          CreativeDesktopTerrainStampPayload{
              entry.assetId, {}, cr::kInvalidCreativeTerrainOperationId,
              false});
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
          "Delete the catalog source. Existing operations keep their baked "
          "terrain and can restore the source later.");
    }
    ImGui::EndGroup();
    ImGui::Separator();
    ImGui::PopID();
  }
  if (!editor.terrainStamps.statusMessage.empty()) {
    ImGui::TextDisabled("%s", editor.terrainStamps.statusMessage.c_str());
  }

  ImGui::SeparatorText("Operations");
  if (ImGui::Button("New Operation")) {
    commands.enqueue(CreativeDesktopCommandId::TerrainOperationNew);
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
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationSetEnabled,
              CreativeDesktopTerrainOperationPayload{operation.id, enabled,
                                                     index});
        }
        ImGui::SameLine();
        const bool generated = operation.kind ==
                               cr::CreativeTerrainOperationKind::GeneratedTerrain;
        const bool region =
            operation.kind == cr::CreativeTerrainOperationKind::Region;
        const bool profile =
            operation.kind == cr::CreativeTerrainOperationKind::Profile;
        const bool stamp =
            operation.kind == cr::CreativeTerrainOperationKind::Stamp;
        cr::CreativeTerrainHeightFieldBounds transformBounds{};
        const bool transformable =
            operation.owner == cr::CreativeTerrainOperationOwner::Manual &&
            operation.kind !=
                cr::CreativeTerrainOperationKind::GeneratedTerrain &&
            cr::creativeTerrainOperationSpatialBounds(operation,
                                                      transformBounds);
        const cr::CreativeTerrainStampSourceStatus stampSourceStatus =
            stamp ? cr::creativeTerrainStampSourceStatus(
                        editor.terrainStamps.library, operation.stamp)
                  : cr::CreativeTerrainStampSourceStatus::Available;
        const bool selectable = generated || region ||
                                (profile && operation.owner ==
                                                cr::CreativeTerrainOperationOwner::Manual);
        const std::string label =
            "#" + std::to_string(operation.id) + "  " +
            std::string(cr::toString(operation.kind)) +
            (generated ? " / " +
                             std::string(cr::toString(operation.composition.mode))
                       : region ? " / " +
                                      std::string(cr::toString(operation.region.mode))
                       : profile ? " / " +
                                       std::string(cr::toString(
                                           operation.profile.profile))
                       : stamp ? " / " + operation.stamp.stamp.label +
                                     " v" +
                                     std::to_string(
                                         operation.stamp.stamp.assetVersion) +
                                     " / " +
                                     std::string(cr::toString(stampSourceStatus))
                               : "");
        const bool selected = profile
                                  ? editor.terrain.profile.editingOperationId ==
                                        operation.id
                                  : state.editingOperationId == operation.id;
        ImGui::BeginDisabled(!selectable);
        if (ImGui::Selectable(label.c_str(), selected)) {
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationSelect,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(editor.transform.active ||
                             editor.terrainGeneration.previewActive ||
                             editor.terrain.profile.editingOperationId !=
                                 cr::kInvalidCreativeTerrainOperationId ||
                             editor.worldLayoutTopography.region.editingEnabled ||
                             editor.worldLayoutTopography.region.ownsPreview ||
                             !transformable);
        if (ImGui::SmallButton("Transform")) {
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationTransform,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
          ImGui::SetTooltip(
              transformable
                  ? "Move this durable terrain recipe on the document grid."
                  : "Generated and World Layout terrain stays with its source editor.");
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(index == 0U);
        if (ImGui::SmallButton("Up")) {
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationMove,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled,
                                                     index - 1U});
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(index + 1U >= operations.size());
        if (ImGui::SmallButton("Down")) {
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationMove,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled,
                                                     index + 1U});
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::SmallButton("Duplicate")) {
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationDuplicate,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Delete")) {
          commands.enqueue(
              CreativeDesktopCommandId::TerrainOperationDelete,
              CreativeDesktopTerrainOperationPayload{operation.id,
                                                     operation.enabled, index});
        }
        if (stamp && stampSourceStatus !=
                         cr::CreativeTerrainStampSourceStatus::Available) {
          ImGui::SameLine();
          const bool replace =
              stampSourceStatus != cr::CreativeTerrainStampSourceStatus::Missing;
          if (ImGui::SmallButton(replace ? "Restore Baked Source"
                                         : "Repair Source")) {
            commands.enqueue(
                CreativeDesktopCommandId::TerrainStampRepairSource,
                CreativeDesktopTerrainStampPayload{
                    operation.stamp.stamp.assetId, {}, operation.id,
                    replace});
          }
        }
        ImGui::PopID();
      }
    }
    ImGui::EndChild();
  }
  ImGui::BeginDisabled(
      document.terrainOperationStack().operations.empty());
  if (ImGui::Button("Bake Terrain Stack")) {
    commands.enqueue(CreativeDesktopCommandId::TerrainOperationBakeAll);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Preserve the current terrain, remove all procedural operations. "
        "This is undoable.");
  }
  ImGui::EndDisabled();

  if (profileEditing) {
    ImGui::TextDisabled(
        "Profile operation selected. Edit exact values in Tool Options and "
        "move its center in the 3D viewport.");
  } else if (!generatedOperation) {
    ImGui::TextDisabled(
        "Region operation selected. Edit its shape in World Layout.");
  }
  ImGui::BeginDisabled(!generatedOperation);

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

  ImGui::SeparatorText("Surface Intent");
  recipeChanged =
      ImGui::Checkbox("Paint materials", &recipe.paintMaterials) ||
      recipeChanged;
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "When disabled, generation changes height without repainting the "
        "existing surface.");
  }
  ImGui::BeginDisabled(!recipe.paintMaterials);
  constexpr std::array<const char*, 5U> biomeLabels{
      "Temperate", "Alpine", "Arid", "Wetland", "Custom"};
  int biome = static_cast<int>(recipe.biomeIntent);
  if (ImGui::Combo("Biome", &biome, biomeLabels.data(),
                   static_cast<int>(biomeLabels.size()))) {
    cr::applyCreativeTerrainBiomeIntent(
        recipe, static_cast<cr::CreativeTerrainBiomeIntent>(biome));
    recipeChanged = true;
  }
  if (materialCombo("Lowland material", recipe.lowlandMaterial)) {
    recipe.biomeIntent = cr::CreativeTerrainBiomeIntent::Custom;
    recipeChanged = true;
  }
  if (materialCombo("Highland material", recipe.highlandMaterial)) {
    recipe.biomeIntent = cr::CreativeTerrainBiomeIntent::Custom;
    recipeChanged = true;
  }
  int transitionHeight =
      static_cast<int>(recipe.materialTransitionHeightCells);
  if (ImGui::SliderInt(
          "Material transition", &transitionHeight,
          static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
          static_cast<int>(cr::kCreativeTerrainMaximumHeightCells),
          "%d cells")) {
    recipe.materialTransitionHeightCells =
        static_cast<std::uint16_t>(transitionHeight);
    recipe.biomeIntent = cr::CreativeTerrainBiomeIntent::Custom;
    recipeChanged = true;
  }
  ImGui::EndDisabled();

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
  constexpr std::array<const char*, 4U> modeLabels{
      "Replace",
      "Raise",
      "Lower",
      "Smooth",
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

  ImGui::SeparatorText("Protected Regions");
  const auto& selectedRegion = editor.worldLayoutTopography.region;
  const bool selectedRegionValid =
      selectedRegion.regionValid &&
      cr::isValidCreativeTerrainHeightFieldBounds(
          selectedRegion.recipe.bounds) &&
      selectedRegion.recipe.mask < cr::CreativeTerrainCompositionMask::Count;
  ImGui::BeginDisabled(!selectedRegionValid);
  if (ImGui::Button("Lock Selected Terrain Region")) {
    const cr::CreativeTerrainProtectedRegionMutationReceipt locked =
        cr::addCreativeTerrainCompositionProtectedRegion(
            state.compositionRecipe,
            {selectedRegion.recipe.bounds, selectedRegion.recipe.mask});
    recipeChanged = recipeChanged || locked.changed;
    state.statusMessage = locked.accepted
                              ? (locked.changed ? "Terrain region protected"
                                                : "Terrain region already protected")
                              : "Could not protect terrain region: " +
                                    std::string(locked.reasonCode);
  }
  ImGui::EndDisabled();
  if (!selectedRegionValid) {
    ImGui::TextDisabled("Select a terrain region in Plan view to lock it.");
  }
  for (std::size_t index = 0U;
       index < state.compositionRecipe.protectedRegionCount; ++index) {
    const cr::CreativeTerrainCompositionProtectedRegion& region =
        state.compositionRecipe.protectedRegions[index];
    ImGui::PushID(static_cast<int>(index));
    ImGui::Text("%zu  %d,%d  %u x %u  %s", index + 1U,
                region.bounds.minimum.x, region.bounds.minimum.z,
                region.bounds.widthCells, region.bounds.depthCells,
                cr::toString(region.mask).data());
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove")) {
      const cr::CreativeTerrainProtectedRegionMutationReceipt removed =
          cr::removeCreativeTerrainCompositionProtectedRegion(
              state.compositionRecipe, index);
      recipeChanged = recipeChanged || removed.changed;
      ImGui::PopID();
      break;
    }
    ImGui::PopID();
  }
  ImGui::BeginDisabled(state.compositionRecipe.protectedRegionCount == 0U);
  if (ImGui::SmallButton("Clear Protected Regions")) {
    const cr::CreativeTerrainProtectedRegionMutationReceipt cleared =
        cr::clearCreativeTerrainCompositionProtectedRegions(
            state.compositionRecipe);
    recipeChanged = recipeChanged || cleared.changed;
  }
  ImGui::EndDisabled();

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
    commands.enqueue(CreativeDesktopCommandId::TerrainGenerationPreview);
  }
  state.draftDirty = state.draftDirty || recipeChanged;

  ImGui::Separator();
  if (!state.previewActive) {
    ImGui::BeginDisabled(!recipeValid);
    if (ImGui::Button("Preview Terrain")) {
      commands.enqueue(CreativeDesktopCommandId::TerrainGenerationPreview);
    }
    ImGui::EndDisabled();
  } else {
    ImGui::BeginDisabled(!recipeValid);
    if (ImGui::Button("Regenerate")) {
      commands.enqueue(CreativeDesktopCommandId::TerrainGenerationRegenerate);
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
      commands.enqueue(CreativeDesktopCommandId::TerrainGenerationApply);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      commands.enqueue(CreativeDesktopCommandId::TerrainGenerationCancel);
    }
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();

  CreativeTerrainContourDisplayState& contours = editor.terrain.contours;
  ImGui::SeparatorText("Topographic Display");
  ImGui::Checkbox("Contour lines", &contours.visible);
  int contourInterval = static_cast<int>(contours.intervalCells);
  if (ImGui::SliderInt(
          "Interval", &contourInterval, 1,
          static_cast<int>(cr::kCreativeTerrainContourMaximumIntervalCells),
          "%d cells")) {
    contours.intervalCells = static_cast<std::uint16_t>(contourInterval);
  }
  int contourMajorEvery = static_cast<int>(contours.majorEvery);
  if (ImGui::SliderInt(
          "Index every", &contourMajorEvery, 1,
          static_cast<int>(cr::kCreativeTerrainContourMaximumMajorEvery))) {
    contours.majorEvery = static_cast<std::uint16_t>(contourMajorEvery);
  }
  if (contours.visible && contours.cacheValid) {
    if (contours.plan.accepted) {
      ImGui::Text("Segments  %zu  Levels  %llu", contours.plan.segments.size(),
                  static_cast<unsigned long long>(
                      contours.plan.contourLevelCount));
    } else {
      const std::string_view contourStatus =
          cr::toString(contours.plan.status);
      ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%.*s",
                         static_cast<int>(contourStatus.size()),
                         contourStatus.data());
    }
  }

  if (state.generation.receipt.accepted) {
    const cr::CreativeTerrainGenerationReceipt& generation =
        state.generation.receipt;
    ImGui::Text("Height  %u - %u cells", generation.minimumHeightCells,
                generation.maximumHeightCells);
    ImGui::Text("Seed  %llu", static_cast<unsigned long long>(recipe.seed));
    ImGui::Text("Work  %llu cells x %u layers = %llu samples",
                static_cast<unsigned long long>(
                    generation.generatedCellCount),
                recipe.octaveCount,
                static_cast<unsigned long long>(
                    generation.evaluatedOctaveCount));
    ImGui::Text("Materials  %llu overrides",
                static_cast<unsigned long long>(
                    generation.generatedMaterialOverrideCount));
    if (state.operationPreview.receipt.accepted) {
      ImGui::TextColored(ImVec4{0.20F, 1.0F, 0.35F, 1.0F}, "READY");
      ImGui::Text("Modified  %llu  Feathered  %llu",
                  static_cast<unsigned long long>(
                      state.operationPreview.receipt.replay.modifiedCellCount),
                  static_cast<unsigned long long>(
                      state.operationPreview.receipt.replay.featheredCellCount));
      ImGui::Text("Protected  %llu  Material edits  %llu",
                  static_cast<unsigned long long>(
                      state.operationPreview.receipt.replay.protectedCellCount),
                  static_cast<unsigned long long>(
                      state.operationPreview.receipt.replay
                          .materialModifiedCellCount));
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
