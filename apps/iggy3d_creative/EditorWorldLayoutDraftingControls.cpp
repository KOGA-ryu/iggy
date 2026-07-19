#include "EditorWorldLayoutPanelInternal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

void drawWorldLayoutTerrainTab(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands,
    bool unavailable) {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  const bool previewOwnedElsewhere =
      terrainGeneration.previewActive && !region.ownsPreview;
  ImGui::BeginDisabled(unavailable || previewOwnedElsewhere);
  bool editingEnabled = region.editingEnabled;
  if (ImGui::Checkbox("Edit terrain region", &editingEnabled)) {
    region.editingEnabled = editingEnabled;
    if (editingEnabled) {
      topography.visible = true;
      topography.elevationBandsVisible = true;
    } else {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
  }
  if (!region.editingEnabled) {
    if (previewOwnedElsewhere) {
      ImGui::TextDisabled("Another terrain preview is active.");
    }
    ImGui::EndDisabled();
    return;
  }

  constexpr std::array<const char*, 5U> operationLabels{
      "Flatten", "Raise", "Lower", "Smooth", "Noise"};
  bool settingsChanged = false;
  int operation = static_cast<int>(region.operation);
  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::Combo("Operation##terrain_region", &operation,
                   operationLabels.data(),
                   static_cast<int>(operationLabels.size()))) {
    region.operation =
        static_cast<CreativeEditorWorldLayoutTerrainRegionOperation>(
            operation);
    settingsChanged = true;
  }

  constexpr std::array<const char*, 2U> maskLabels{"Rectangle", "Ellipse"};
  int mask = static_cast<int>(region.mask);
  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::Combo("Shape##terrain_region", &mask, maskLabels.data(),
                   static_cast<int>(maskLabels.size()))) {
    region.mask = static_cast<cr::CreativeTerrainCompositionMask>(mask);
    settingsChanged = true;
  }

  if (settingsChanged && region.regionValid && !region.selecting) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  }

  ImGui::TextDisabled("Drag on the map to select a region.");
  ImGui::EndDisabled();
}

// Right Properties window while terrain-region editing is active:
// context-sensitive operation parameters plus read-only region metrics.
void drawWorldLayoutTerrainRegionInspector(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands,
    bool unavailable) {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  const bool previewOwnedElsewhere =
      terrainGeneration.previewActive && !region.ownsPreview;
  ImGui::SeparatorText("Terrain region");
  ImGui::BeginDisabled(unavailable || previewOwnedElsewhere);

  bool settingsChanged = false;
  const bool smooth = region.operation ==
                      CreativeEditorWorldLayoutTerrainRegionOperation::Smooth;
  if (smooth) {
    ImGui::TextDisabled("Smooth runs one fixed 3x3 pass.");
  } else {
    const char* targetLabel = "Height##terrain_region";
    switch (region.operation) {
      case CreativeEditorWorldLayoutTerrainRegionOperation::Raise:
        targetLabel = "Raise to at least##terrain_region";
        break;
      case CreativeEditorWorldLayoutTerrainRegionOperation::Lower:
        targetLabel = "Lower to at most##terrain_region";
        break;
      case CreativeEditorWorldLayoutTerrainRegionOperation::Noise:
        targetLabel = "Base height##terrain_region";
        break;
      default:
        break;
    }
    int target = static_cast<int>(region.targetHeightCells);
    if (ImGui::DragInt(
            targetLabel, &target, 0.25F,
            static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
            static_cast<int>(cr::kCreativeTerrainMaximumHeightCells),
            "%d cells", ImGuiSliderFlags_AlwaysClamp)) {
      region.targetHeightCells = static_cast<std::uint16_t>(target);
      settingsChanged = true;
    }
  }

  if (region.operation ==
      CreativeEditorWorldLayoutTerrainRegionOperation::Noise) {
    int relief = static_cast<int>(region.noiseReliefCells);
    if (ImGui::DragInt(
            "Relief##terrain_region", &relief, 0.25F, 0,
            static_cast<int>(cr::kCreativeTerrainMaximumHeightCells),
            "%d cells", ImGuiSliderFlags_AlwaysClamp)) {
      region.noiseReliefCells = static_cast<std::uint16_t>(relief);
      settingsChanged = true;
    }
    settingsChanged =
        ImGui::DragScalar(
            "Scale##terrain_region", ImGuiDataType_Double,
            &region.noiseScaleCells, 0.25F,
            &cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells,
            &cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells,
            "%.1f cells", ImGuiSliderFlags_AlwaysClamp) ||
        settingsChanged;
    ImGui::Text("Seed: %llu",
                static_cast<unsigned long long>(region.seed));
    ImGui::SameLine();
    if (ImGui::Button("New seed##terrain_region")) {
      region.seed = region.seed == std::numeric_limits<std::uint64_t>::max()
                        ? 0U
                        : region.seed + 1U;
      settingsChanged = true;
    }
  }

  int feather = static_cast<int>(region.featherCells);
  if (ImGui::DragInt(
          "Feather##terrain_region", &feather, 0.25F, 0,
          static_cast<int>(
              cr::kCreativeTerrainCompositionMaximumFeatherCells),
          "%d cells", ImGuiSliderFlags_AlwaysClamp)) {
    region.featherCells = static_cast<std::uint16_t>(feather);
    settingsChanged = true;
  }

  // A parameter change invalidates the owned preview by definition; the
  // fresh exact preview is requested through the dispatcher, never computed
  // by the panel.
  if (settingsChanged && region.regionValid && !region.selecting) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  }

  ImGui::SeparatorText("Region");
  const CreativeEditorWorldLayoutTerrainRegionMetrics metrics =
      measureCreativeEditorWorldLayoutTerrainRegion(region);
  if (metrics.present) {
    ImGui::Text("Bounds: (%d, %d)", metrics.minimumX, metrics.minimumZ);
    ImGui::Text("Width: %u cells", metrics.widthCells);
    ImGui::Text("Depth: %u cells", metrics.depthCells);
    ImGui::Text("Candidate cells: %u", metrics.candidateCellCount);
  } else {
    ImGui::TextDisabled("No region selected.");
  }
  ImGui::EndDisabled();
}

// Bottom Build window while terrain-region editing is active: preview state,
// the terrain state's exact status message, preview cell counts, and the
// sole Apply/Cancel controls.
void drawWorldLayoutTerrainRegionBuild(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands,
    bool unavailable) {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  const bool previewOwnedElsewhere =
      terrainGeneration.previewActive && !region.ownsPreview;
  const CreativeEditorWorldLayoutTerrainRegionPhase phase =
      classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  ImGui::BeginDisabled(unavailable || previewOwnedElsewhere);
  ImGui::Text("Preview: %s", toString(phase).data());
  ImVec4 statusTint{0.75F, 0.78F, 0.80F, 1.0F};
  if (phase == CreativeEditorWorldLayoutTerrainRegionPhase::Ready) {
    statusTint = ImVec4{0.32F, 0.95F, 0.43F, 1.0F};
  } else if (phase == CreativeEditorWorldLayoutTerrainRegionPhase::Rejected ||
             phase == CreativeEditorWorldLayoutTerrainRegionPhase::Stale) {
    statusTint = ImVec4{0.95F, 0.35F, 0.32F, 1.0F};
  }
  ImGui::TextColored(statusTint, "%s", region.statusMessage.c_str());
  if (region.ownsPreview) {
    const cr::CreativeTerrainOperationReplayReceipt& replay =
        terrainGeneration.operationPreview.receipt.replay;
    ImGui::Text("Changed cells: %llu",
                static_cast<unsigned long long>(replay.modifiedCellCount));
    ImGui::SameLine();
    ImGui::Text("Output cells: %llu",
                static_cast<unsigned long long>(replay.outputCellCount));
  }
  ImGui::BeginDisabled(!region.ownsPreview);
  if (ImGui::Button("Apply Region")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutTerrainRegionApply);
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!region.selecting && !region.regionValid &&
                       !region.ownsPreview);
  if (ImGui::Button("Cancel")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();
}

// Canvas-top tool options strip: one row that follows the active tool. While
// terrain-region editing is live it carries the whole region workflow —
// operation, mask, parameters, preview/apply/cancel — so a terrain edit never
// leaves the canvas; otherwise it names the active tool. Parameter widgets
// mirror the inspector exactly: same clamps, same state fields, and the same
// change-requests-preview rule through the dispatcher.
void drawWorldLayoutToolOptionsStrip(CreativeEditorState& editor,
                                     CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  const float tile = ImGui::GetFrameHeight();
  if (!region.editingEnabled) {
    const bool placingTemplate = state.buildingTemplatePlacement.active;
    const CreativeEditorToolGlyph glyph =
        placingTemplate ? CreativeEditorToolGlyph::EstateHouse
                        : creativeEditorToolGlyphForWorldLayoutTool(state.tool);
    std::string_view label = "Placing building template";
    if (!placingTemplate) {
      label = toString(glyph);
      if (state.tool == CreativeEditorWorldLayoutTool::CatalogAsset &&
          !state.catalogPlacement.label.empty()) {
        label = state.catalogPlacement.label;
      } else {
        for (const CreativeEditorWorldLayoutPaletteEntry& paletteEntry :
             creativeEditorWorldLayoutPaletteEntries()) {
          if (paletteEntry.activation ==
                  CreativeEditorWorldLayoutPaletteActivation::Tool &&
              paletteEntry.tool == state.tool) {
            label = paletteEntry.label;
            break;
          }
        }
      }
    }
    const ImVec2 glyphPosition = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2{tile, tile});
    drawCreativeEditorToolGlyph(*ImGui::GetWindowDrawList(), glyph,
                                glyphPosition.x + 2.0F, glyphPosition.y + 2.0F,
                                tile - 4.0F,
                                ImGui::GetColorU32(ImGuiCol_Text));
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    return;
  }

  const bool previewOwnedElsewhere =
      editor.terrainGeneration.previewActive && !region.ownsPreview;
  const bool locked = previewOwnedElsewhere ||
                      creativeEditorWorldLayoutPreviewActive(state) ||
                      state.buildingTransform.active ||
                      state.buildingTemplatePlacement.active;
  ImGui::BeginDisabled(locked);
  bool settingsChanged = false;

  constexpr std::array<CreativeEditorWorldLayoutTerrainRegionOperation, 5U>
      operations{CreativeEditorWorldLayoutTerrainRegionOperation::Flatten,
                 CreativeEditorWorldLayoutTerrainRegionOperation::Raise,
                 CreativeEditorWorldLayoutTerrainRegionOperation::Lower,
                 CreativeEditorWorldLayoutTerrainRegionOperation::Smooth,
                 CreativeEditorWorldLayoutTerrainRegionOperation::Noise};
  constexpr std::array<const char*, 5U> operationNames{
      "Flatten", "Raise", "Lower", "Smooth", "Noise"};
  for (std::size_t index = 0U; index < operations.size(); ++index) {
    if (index > 0U) {
      ImGui::SameLine();
    }
    ImGui::PushID(static_cast<int>(index));
    if (drawCreativeEditorWorldLayoutGlyphButton(
            "##options_operation",
            creativeEditorToolGlyphForTerrainRegionOperation(
                operations[index]),
            tile, region.operation == operations[index],
            operationNames[index]) &&
        region.operation != operations[index]) {
      region.operation = operations[index];
      settingsChanged = true;
    }
    ImGui::PopID();
  }

  ImGui::SameLine(0.0F, 12.0F);
  constexpr std::array<cr::CreativeTerrainCompositionMask, 2U> masks{
      cr::CreativeTerrainCompositionMask::Rectangle,
      cr::CreativeTerrainCompositionMask::Ellipse};
  constexpr std::array<const char*, 2U> maskNames{"Rectangle mask",
                                                  "Ellipse mask"};
  for (std::size_t index = 0U; index < masks.size(); ++index) {
    if (index > 0U) {
      ImGui::SameLine();
    }
    ImGui::PushID(static_cast<int>(index));
    if (drawCreativeEditorWorldLayoutGlyphButton(
            "##options_mask",
            creativeEditorToolGlyphForTerrainMask(masks[index]), tile,
            region.mask == masks[index], maskNames[index]) &&
        region.mask != masks[index]) {
      region.mask = masks[index];
      settingsChanged = true;
    }
    ImGui::PopID();
  }

  const auto compactParam = [&settingsChanged](
                                const char* compactLabel,
                                std::string_view fullLabel, const char* id,
                                std::uint16_t& valueCells, int minimum,
                                int maximum) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", compactLabel);
    if (!fullLabel.empty() && ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", fullLabel.data());
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.0F);
    int value = static_cast<int>(valueCells);
    if (ImGui::DragInt(id, &value, 0.25F, minimum, maximum, "%d",
                       ImGuiSliderFlags_AlwaysClamp)) {
      valueCells = static_cast<std::uint16_t>(value);
      settingsChanged = true;
    }
  };

  const CreativeEditorWorldLayoutTerrainRegionOperation operation =
      region.operation;
  if (creativeEditorWorldLayoutTerrainRegionFieldVisible(
          operation,
          CreativeEditorWorldLayoutTerrainRegionField::TargetHeight)) {
    const char* compactLabel = "Height";
    switch (operation) {
      case CreativeEditorWorldLayoutTerrainRegionOperation::Raise:
        compactLabel = "Min";
        break;
      case CreativeEditorWorldLayoutTerrainRegionOperation::Lower:
        compactLabel = "Max";
        break;
      case CreativeEditorWorldLayoutTerrainRegionOperation::Noise:
        compactLabel = "Base";
        break;
      default:
        break;
    }
    ImGui::SameLine(0.0F, 12.0F);
    compactParam(compactLabel,
                 creativeEditorWorldLayoutTerrainRegionTargetLabel(operation),
                 "##options_height", region.targetHeightCells,
                 static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
                 static_cast<int>(cr::kCreativeTerrainMaximumHeightCells));
  }
  if (creativeEditorWorldLayoutTerrainRegionFieldVisible(
          operation, CreativeEditorWorldLayoutTerrainRegionField::NoiseRelief)) {
    ImGui::SameLine(0.0F, 12.0F);
    compactParam("Relief", {}, "##options_relief", region.noiseReliefCells, 0,
                 static_cast<int>(cr::kCreativeTerrainMaximumHeightCells));
  }
  if (creativeEditorWorldLayoutTerrainRegionFieldVisible(
          operation, CreativeEditorWorldLayoutTerrainRegionField::NoiseScale)) {
    ImGui::SameLine(0.0F, 12.0F);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Scale");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.0F);
    settingsChanged =
        ImGui::DragScalar(
            "##options_scale", ImGuiDataType_Double, &region.noiseScaleCells,
            0.25F, &cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells,
            &cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells, "%.1f",
            ImGuiSliderFlags_AlwaysClamp) ||
        settingsChanged;
  }
  if (creativeEditorWorldLayoutTerrainRegionFieldVisible(
          operation, CreativeEditorWorldLayoutTerrainRegionField::Seed)) {
    ImGui::SameLine(0.0F, 12.0F);
    if (drawCreativeEditorWorldLayoutGlyphButton("##options_seed",
                                   CreativeEditorToolGlyph::ActionNewSeed,
                                   tile, false, "New seed")) {
      region.seed =
          region.seed == std::numeric_limits<std::uint64_t>::max()
              ? 0U
              : region.seed + 1U;
      settingsChanged = true;
    }
  }
  if (creativeEditorWorldLayoutTerrainRegionFieldVisible(
          operation, CreativeEditorWorldLayoutTerrainRegionField::Feather)) {
    ImGui::SameLine(0.0F, 12.0F);
    compactParam(
        "Feather", {}, "##options_feather", region.featherCells, 0,
        static_cast<int>(cr::kCreativeTerrainCompositionMaximumFeatherCells));
  }

  // A parameter change invalidates the owned preview by definition; the
  // fresh exact preview is requested through the dispatcher, never computed
  // by the panel.
  if (settingsChanged && region.regionValid && !region.selecting) {
    commands.push(CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  }

  const ImGuiStyle& style = ImGui::GetStyle();
  const float actionsWidth = (3.0F * tile) + (2.0F * style.ItemSpacing.x);
  ImGui::SameLine();
  const float slack = ImGui::GetContentRegionAvail().x - actionsWidth;
  if (slack > style.ItemSpacing.x) {
    ImGui::Dummy(ImVec2{slack - style.ItemSpacing.x, 0.0F});
    ImGui::SameLine();
  }
  ImGui::BeginDisabled(!region.regionValid || region.selecting);
  if (drawCreativeEditorWorldLayoutGlyphButton("##options_preview",
                                 CreativeEditorToolGlyph::ActionPreview, tile,
                                 false, "Preview")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview);
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!region.ownsPreview);
  if (drawCreativeEditorWorldLayoutGlyphButton("##options_apply",
                                 CreativeEditorToolGlyph::ActionApply, tile,
                                 false, "Apply region")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutTerrainRegionApply);
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!region.selecting && !region.regionValid &&
                       !region.ownsPreview);
  if (drawCreativeEditorWorldLayoutGlyphButton("##options_cancel",
                                 CreativeEditorToolGlyph::ActionCancel, tile,
                                 false, "Cancel")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();
}

// One text row under the canvas: hover cell, zoom, snap readout, region
// metrics, and the live status message tinted the same way the Build window
// tints it. Read-only; drawn outside the editing-disabled scope so it stays
// legible during play mode.
void drawWorldLayoutStatusBar(
    const CreativeEditorState& editor,
    const CreativeEditorWorldLayoutCanvasHoverStatus& hover) {
  const CreativeEditorWorldLayoutStatusLine line =
      composeCreativeEditorWorldLayoutStatusLine(
          editor.worldLayout, editor.worldLayoutTopography,
          editor.terrainGeneration, hover);
  ImGui::AlignTextToFramePadding();
  ImGui::TextDisabled("%s", line.cursor.c_str());
  ImGui::SameLine(0.0F, 16.0F);
  ImGui::TextDisabled("%s", line.zoom.c_str());
  if (!line.snap.empty()) {
    ImGui::SameLine(0.0F, 16.0F);
    ImGui::TextDisabled("%s", line.snap.c_str());
  }
  if (!line.cells.empty()) {
    ImGui::SameLine(0.0F, 16.0F);
    ImGui::TextDisabled("%s", line.cells.c_str());
  }
  ImGui::SameLine(0.0F, 16.0F);
  ImVec4 statusTint{0.32F, 0.95F, 0.43F, 1.0F};
  if (line.regionMessage) {
    if (line.phase == CreativeEditorWorldLayoutTerrainRegionPhase::Rejected ||
        line.phase == CreativeEditorWorldLayoutTerrainRegionPhase::Stale) {
      statusTint = ImVec4{0.95F, 0.35F, 0.32F, 1.0F};
    } else if (line.phase !=
               CreativeEditorWorldLayoutTerrainRegionPhase::Ready) {
      statusTint = ImVec4{0.75F, 0.78F, 0.80F, 1.0F};
    }
  }
  ImGui::TextColored(statusTint, "%s", line.message.c_str());
}


}  // namespace

void drawCreativeEditorWorldLayoutTerrainTab(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands, bool unavailable) {
  drawWorldLayoutTerrainTab(topography, terrainGeneration, commands,
                            unavailable);
}

void drawCreativeEditorWorldLayoutTerrainRegionProperties(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands, bool unavailable) {
  drawWorldLayoutTerrainRegionInspector(topography, terrainGeneration,
                                        commands, unavailable);
}

void drawCreativeEditorWorldLayoutTerrainRegionBuild(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands, bool unavailable) {
  drawWorldLayoutTerrainRegionBuild(topography, terrainGeneration, commands,
                                    unavailable);
}

void drawCreativeEditorWorldLayoutToolOptions(
    CreativeEditorState& editor, CreativeDesktopCommandFrame& commands) {
  drawWorldLayoutToolOptionsStrip(editor, commands);
}

void drawCreativeEditorWorldLayoutStatusBar(
    const CreativeEditorState& editor,
    const CreativeEditorWorldLayoutCanvasHoverStatus& hover) {
  drawWorldLayoutStatusBar(editor, hover);
}

}  // namespace iggy3d_creative_app
