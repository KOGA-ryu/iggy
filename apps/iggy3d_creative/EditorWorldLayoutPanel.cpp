#include "EditorWorldLayoutPanel.hpp"

#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayoutElevationPanel.hpp"
#include "EditorWorldLayoutHierarchyPanel.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutReviewModel.hpp"
#include "EditorWorldLayoutReviewPanel.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include <cstddef>
#include <utility>

#include "imgui.h"

namespace iggy3d_creative_app {

void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& selection,
    const cr::CreativeMeasurementState& measurement, bool playModeActive,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
  if (synchronizeCreativeEditorWorldLayoutTerrainRegion(
          topography.region, editor.terrainGeneration, document)) {
    topography.cacheValid = false;
  }
  if (!desktopUi.showWorldLayout) {
    cancelCreativeEditorWorldLayoutPanelManipulation(state, commands);
    if (topography.region.editingEnabled || topography.region.ownsPreview) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
    return;
  }

  const bool editingDisabled = playModeActive || editor.assetEdit.active;
  const bool exactPreviewActive =
      creativeEditorWorldLayoutPreviewActive(state);
  const CreativeEditorWorldLayoutDiagnosticReport& diagnostics =
      refreshCreativeEditorWorldLayoutDiagnostics(
          state.diagnosticCache, document, state.source, state.revision,
          &editor.catalog.model, state.sourceEpoch, state.generatedRevision,
          &state.generatedBaseline.source);
  CreativeEditorWorldLayoutConflictReviewSynchronization synchronization =
      planCreativeEditorWorldLayoutConflictReviewSynchronization(
          state.conflictReview, state.diagnosticCache.buildCount, diagnostics);
  if (synchronization.replace) {
    state.conflictReview = std::move(synchronization.review);
  }
  if (editingDisabled) {
    cancelCreativeEditorWorldLayoutPanelManipulation(state, commands);
    if (topography.region.editingEnabled || topography.region.ownsPreview) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
  }
  if (state.buildingTransform.active && input.cancelPressed) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutTransformBuilding,
        CreativeDesktopWorldLayoutBuildingTransformPayload{
            CreativeEditorWorldLayoutBuildingTransformPhase::Cancel,
            state.buildingTransform.operation});
  }

  // The World Layout workspace reuses the shell's existing dock identities.
  // The ### suffix keeps the persisted ImGui IDs stable while presenting
  // task-specific titles instead of leaving unrelated panels beside a
  // crowded center canvas.
  if (ImGui::Begin("World Layout Tools###Project", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (ImGui::BeginTabBar("##world_layout_left_tabs")) {
      if (ImGui::BeginTabItem("Source")) {
        drawCreativeEditorWorldLayoutHierarchy(
            desktopUi, state, commands,
            editingDisabled || topography.region.editingEnabled);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Create")) {
        drawCreativeEditorWorldLayoutCreateTools(
            desktopUi, state, editor.catalog.model, document.gridSettings(),
            commands,
            editingDisabled || state.buildingTransform.active ||
                state.buildingTemplatePlacement.active ||
                topography.region.editingEnabled);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Terrain")) {
        drawCreativeEditorWorldLayoutTerrainTab(
            topography, editor.terrainGeneration, commands,
            editingDisabled || exactPreviewActive ||
                state.buildingTransform.active ||
                state.buildingTemplatePlacement.active);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();

  if (ImGui::Begin("World Layout Properties###Inspector", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (topography.region.editingEnabled) {
      drawCreativeEditorWorldLayoutTerrainRegionProperties(
          topography, editor.terrainGeneration, commands,
          editingDisabled || exactPreviewActive ||
              state.buildingTransform.active ||
              state.buildingTemplatePlacement.active);
    } else {
      ImGui::BeginDisabled(editingDisabled);
      drawCreativeEditorWorldLayoutSourceInspector(state, document, commands);
      drawCreativeEditorWorldLayoutStructureInspector(
          desktopUi, state, document, commands);
      drawCreativeEditorWorldLayoutSelectionProperties(
          state, editor.catalog.model, commands);
      if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::None) {
        ImGui::TextDisabled("Select a World Layout source to edit it.");
      }
      ImGui::EndDisabled();
    }
  }
  ImGui::End();

  const bool hasSelection =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::None;
  const bool buildingSelected =
      state.selection.kind ==
      CreativeEditorWorldLayoutSelectionKind::Building;
  if (ImGui::Begin("World Layout Build###Diagnostics##bottom", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (topography.region.editingEnabled) {
      drawCreativeEditorWorldLayoutTerrainRegionBuild(
          topography, editor.terrainGeneration, commands, editingDisabled);
    } else {
      ImGui::BeginDisabled(editingDisabled ||
                           state.buildingTransform.active ||
                           state.buildingTemplatePlacement.active);
      ImGui::BeginDisabled(!diagnostics.ready);
      if (ImGui::Button(exactPreviewActive ? "Refresh 3D Preview"
                                           : "Preview 3D")) {
        commands.enqueue(CreativeDesktopCommandId::WorldLayoutPreview);
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(!diagnostics.canGenerate);
      if (ImGui::Button(exactPreviewActive ? "Confirm Preview"
                                           : "Confirm & Generate")) {
        commands.enqueue(CreativeDesktopCommandId::WorldLayoutConfirm);
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(!hasSelection);
      if (ImGui::Button(buildingSelected ? "Delete building" : "Delete")) {
        if (buildingSelected) {
          ImGui::OpenPopup("Delete building group");
        } else {
          commands.enqueue(
              CreativeDesktopCommandId::WorldLayoutDeleteSelection);
        }
      }
      ImGui::EndDisabled();
      ImGui::EndDisabled();

      if (ImGui::BeginPopupModal("Delete building group", nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize)) {
        const char* buildingName =
            buildingSelected &&
                    state.selection.index < state.source.buildings.size()
                ? state.source.buildings[state.selection.index].name.c_str()
                : "selected building";
        ImGui::Text("Delete %s and all owned layout symbols?", buildingName);
        if (ImGui::Button("Delete building")) {
          commands.enqueue(
              CreativeDesktopCommandId::WorldLayoutDeleteSelection);
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::Separator();
      ImGui::TextColored(
          diagnostics.canGenerate ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                                  : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
          "%s", diagnostics.canGenerate ? "READY" : "BLOCKED");
      ImGui::SameLine();
      if (diagnostics.ready) {
        ImGui::TextDisabled(
            "%s", !diagnostics.canGenerate
                      ? "Refined terrain requires an explicit decision"
                      : diagnostics.hasChanges
                            ? "Changes are ready to generate"
                            : "Generated output already matches");
        const cr::CreativeWorldLayoutReceipt& generation =
            diagnostics.compileReceipt;
        ImGui::TextDisabled(
            "Recipe groups: +%llu  patch %llu  replace %llu  keep %llu  "
            "refined %llu  |  detach %llu  remove %llu objects",
            static_cast<unsigned long long>(
                generation.objectRecipeCreateCount),
            static_cast<unsigned long long>(
                generation.objectRecipePatchCount),
            static_cast<unsigned long long>(
                generation.objectRecipeReplaceCount),
            static_cast<unsigned long long>(
                generation.objectRecipeKeepCount),
            static_cast<unsigned long long>(
                generation.objectRecipeRefinedCount),
            static_cast<unsigned long long>(generation.objectDetachCount),
            static_cast<unsigned long long>(generation.objectRemoveCount));
      }
      drawCreativeEditorWorldLayoutReview(
          state, diagnostics, editor.catalog.model, commands, editingDisabled,
          editingDisabled || state.buildingTransform.active ||
              state.buildingTemplatePlacement.active ||
              topography.region.editingEnabled);

      ImGui::Separator();
      if (ImGui::BeginTable("##world_layout_counts", 4,
                            ImGuiTableFlags_SizingStretchSame |
                                ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextColumn();
        ImGui::Text("Buildings  %llu", static_cast<unsigned long long>(
                                          state.source.buildings.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Levels  %llu", static_cast<unsigned long long>(
                                       state.source.levels.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Rooms  %llu", static_cast<unsigned long long>(
                                      state.source.rooms.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Floors  %llu", static_cast<unsigned long long>(
                                       state.source.boxes.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Partitions  %llu",
                    static_cast<unsigned long long>(
                        state.source.walls.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Openings  %llu", static_cast<unsigned long long>(
                                         state.source.openings.size()));
        ImGui::TableNextColumn();
        ImGui::Text(
            "Terrain  %llu",
            static_cast<unsigned long long>(
                state.source.terrainProfiles.size() +
                state.source.terrainPaths.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Objects  %llu", static_cast<unsigned long long>(
                                        state.source.objects.size()));
        ImGui::EndTable();
      }

      ImGui::TextDisabled("Revision %llu%s",
                          static_cast<unsigned long long>(state.revision),
                          creativeEditorWorldLayoutDirty(state) ? " *" : "");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4{0.32F, 0.95F, 0.43F, 1.0F}, "%s",
                         state.statusMessage.c_str());
    }
  }
  ImGui::End();

  if (ImGui::Begin("World Layout", &desktopUi.showWorldLayout,
                   ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse)) {
    drawCreativeEditorWorldLayoutViewControls(
        state, editor.worldLayoutTopography, commands);
    ImGui::Separator();
    ImGui::BeginDisabled(editingDisabled);
    const bool canvasInteractionEnabled =
        !editingDisabled && !state.buildingTransform.active;
    drawCreativeEditorWorldLayoutToolOptions(editor, commands);
    ImGui::Separator();
    CreativeEditorWorldLayoutCanvasHoverStatus canvasHover;
    const float statusBarHeight = ImGui::GetFrameHeight();
    if (ImGui::BeginChild("##world_layout_canvas_host",
                          ImVec2{0.0F, -statusBarHeight}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
      drawCreativeEditorWorldLayoutToolboxStrip(editor, commands);
      ImGui::SameLine();
      if (state.viewMode == CreativeEditorWorldLayoutViewMode::Elevation) {
        drawCreativeEditorWorldLayoutElevationCanvas(
            editor, document.gridSettings(),
            document.measurementAnnotationStore(), measurement, commands,
            canvasInteractionEnabled, input);
      } else {
        drawCreativeEditorWorldLayoutCanvas(
            editor, document, selection, measurement, commands,
            canvasInteractionEnabled, input, &canvasHover);
      }
    }
    ImGui::EndChild();
    ImGui::EndDisabled();
    drawCreativeEditorWorldLayoutStatusBar(editor, canvasHover);
  } else {
    cancelCreativeEditorWorldLayoutPanelManipulation(state, commands);
    if (topography.region.editingEnabled || topography.region.ownsPreview) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
  }
  ImGui::End();
}

}  // namespace iggy3d_creative_app
