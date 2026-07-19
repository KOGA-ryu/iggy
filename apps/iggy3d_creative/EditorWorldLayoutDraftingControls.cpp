#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorToolPresentation.hpp"
#include "EditorWorldLayout.hpp"

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

// Canvas-top tool options strip: one row that follows the active tool
// presentation. The strip is a generic consumer of the declarative
// tool-presentation model — it renders whatever options and actions the
// active presentation declares (choices, drags, seed, right-aligned actions)
// and dispatches the model's commands; it holds no per-tool branches.
void drawWorldLayoutToolOptionsStrip(CreativeEditorState& editor,
                                     CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
  const float tile = ImGui::GetFrameHeight();
  const CreativeEditorToolPresentation& presentation =
      activeCreativeEditorToolPresentation(state, topography);
  const CreativeEditorToolPresentationStatus status =
      evaluateCreativeEditorToolPresentation(presentation, state, topography,
                                             editor.terrainGeneration);
  if (presentation.options.empty() && presentation.actions.empty()) {
    std::string_view label = presentation.name;
    if (presentation.tool == CreativeEditorWorldLayoutTool::CatalogAsset &&
        !state.catalogPlacement.label.empty()) {
      label = state.catalogPlacement.label;
    }
    const ImVec2 glyphPosition = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2{tile, tile});
    drawCreativeEditorToolGlyph(*ImGui::GetWindowDrawList(),
                                presentation.glyph, glyphPosition.x + 2.0F,
                                glyphPosition.y + 2.0F, tile - 4.0F,
                                ImGui::GetColorU32(ImGuiCol_Text));
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    return;
  }

  ImGui::BeginDisabled(status.unavailable);
  bool settingsChanged = false;
  int widgetId = 0;
  bool firstGroup = true;
  for (const CreativeEditorToolOptionSpec& option : presentation.options) {
    if (!creativeEditorToolOptionVisible(option, topography)) {
      continue;
    }
    if (!firstGroup) {
      ImGui::SameLine(0.0F, 12.0F);
    }
    firstGroup = false;
    ImGui::PushID(widgetId++);
    switch (option.widget) {
      case CreativeEditorToolOptionWidget::GlyphChoice: {
        const std::uint8_t current =
            creativeEditorToolOptionChoiceValue(option, topography);
        for (std::size_t index = 0U; index < option.choices.size(); ++index) {
          const CreativeEditorToolOptionChoice& choice =
              option.choices[index];
          if (index > 0U) {
            ImGui::SameLine();
          }
          ImGui::PushID(static_cast<int>(index));
          if (drawCreativeEditorWorldLayoutGlyphButton(
                  "##choice", choice.glyph, tile, current == choice.value,
                  choice.label) &&
              setCreativeEditorToolOptionChoiceValue(option, topography,
                                                     choice.value)) {
            settingsChanged = true;
          }
          ImGui::PopID();
        }
        break;
      }
      case CreativeEditorToolOptionWidget::IntDrag: {
        const std::string_view compact =
            creativeEditorToolOptionCompactLabel(option, topography);
        const std::string_view full =
            creativeEditorToolOptionFullLabel(option, topography);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", compact.data());
        if (!full.empty() && ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", full.data());
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(64.0F);
        int value = static_cast<int>(
            creativeEditorToolOptionScalarValue(option, topography));
        if (ImGui::DragInt("##value", &value, 0.25F,
                           static_cast<int>(option.minimum),
                           static_cast<int>(option.maximum), "%d",
                           ImGuiSliderFlags_AlwaysClamp) &&
            setCreativeEditorToolOptionScalarValue(
                option, topography, static_cast<double>(value))) {
          settingsChanged = true;
        }
        break;
      }
      case CreativeEditorToolOptionWidget::DoubleDrag: {
        const std::string_view compact =
            creativeEditorToolOptionCompactLabel(option, topography);
        const std::string_view full =
            creativeEditorToolOptionFullLabel(option, topography);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", compact.data());
        if (!full.empty() && ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", full.data());
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(64.0F);
        double value =
            creativeEditorToolOptionScalarValue(option, topography);
        if (ImGui::DragScalar("##value", ImGuiDataType_Double, &value, 0.25F,
                              &option.minimum, &option.maximum, "%.1f",
                              ImGuiSliderFlags_AlwaysClamp) &&
            setCreativeEditorToolOptionScalarValue(option, topography,
                                                   value)) {
          settingsChanged = true;
        }
        break;
      }
      case CreativeEditorToolOptionWidget::SeedButton: {
        if (drawCreativeEditorWorldLayoutGlyphButton(
                "##seed", CreativeEditorToolGlyph::ActionNewSeed, tile, false,
                option.label)) {
          advanceCreativeEditorToolOptionSeed(topography);
          settingsChanged = true;
        }
        break;
      }
      case CreativeEditorToolOptionWidget::Count:
        break;
    }
    ImGui::PopID();
  }

  if (settingsChanged) {
    for (const CreativeEditorToolActionSpec& action : presentation.actions) {
      if (action.runOnOptionChange &&
          creativeEditorToolActionEnabled(action, topography)) {
        commands.push(action.command);
      }
    }
  }

  if (!presentation.actions.empty()) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const auto actionCount =
        static_cast<float>(presentation.actions.size());
    const float actionsWidth =
        (actionCount * tile) + ((actionCount - 1.0F) * style.ItemSpacing.x);
    ImGui::SameLine();
    const float slack = ImGui::GetContentRegionAvail().x - actionsWidth;
    if (slack > style.ItemSpacing.x) {
      ImGui::Dummy(ImVec2{slack - style.ItemSpacing.x, 0.0F});
      ImGui::SameLine();
    }
    int actionId = 0;
    for (const CreativeEditorToolActionSpec& action : presentation.actions) {
      if (actionId > 0) {
        ImGui::SameLine();
      }
      ImGui::PushID(actionId++);
      ImGui::BeginDisabled(
          !creativeEditorToolActionEnabled(action, topography));
      if (drawCreativeEditorWorldLayoutGlyphButton(
              "##action", action.glyph, tile, false, action.label)) {
        commands.push(action.command);
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }
  }
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
