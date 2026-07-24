#include "EditorDesktopWorldLayoutInspector.hpp"

#include "EditorDesktopUi.hpp"
#include "EditorDesktopWidgets.hpp"
#include "EditorDesktopWorldLayoutArchitectureInspector.hpp"

#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

const char* boxSettingsTitle(cr::CreativeObjectKind kind) noexcept {
  switch (kind) {
    case cr::CreativeObjectKind::Floor: return "Floor settings";
    case cr::CreativeObjectKind::Ceiling: return "Ceiling settings";
    case cr::CreativeObjectKind::Roof: return "Roof settings";
    default: return "Volume settings";
  }
}

const char* boxAnchorLabel(cr::CreativeObjectKind kind) noexcept {
  switch (kind) {
    case cr::CreativeObjectKind::Floor: return "Top##layout_box";
    case cr::CreativeObjectKind::Ceiling:
    case cr::CreativeObjectKind::Roof: return "Support##layout_box";
    default: return "Base##layout_box";
  }
}

void drawBuildingActions(CreativeEditorDesktopUiState& desktopUi,
                         CreativeEditorWorldLayoutState& state,
                         const cr::CreativeDocument& document,
                         CreativeDesktopCommandFrame& commands) {
  const std::size_t buildingIndex =
      creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex >= state.source.buildings.size()) {
    return;
  }
  const cr::CreativeWorldLayoutBuilding& building =
      state.source.buildings[buildingIndex];
  ImGui::Text("Building: %s", building.name.c_str());
  ImGui::SameLine();
  if (state.selection.kind !=
      CreativeEditorWorldLayoutSelectionKind::Building) {
    if (ImGui::Button("Select building")) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutSelectBuilding,
          CreativeDesktopWorldLayoutBuildingSelectionPayload{buildingIndex});
    }
    ImGui::Separator();
    return;
  }

  std::uint64_t roomCount = 0U;
  std::uint64_t boxCount = 0U;
  std::uint64_t wallCount = 0U;
  for (const cr::CreativeWorldLayoutRoom& room : state.source.rooms) {
    roomCount += room.buildingIndex == buildingIndex ? 1U : 0U;
  }
  for (const cr::CreativeWorldLayoutBox& box : state.source.boxes) {
    boxCount += box.buildingIndex == buildingIndex ? 1U : 0U;
  }
  for (const cr::CreativeWorldLayoutWall& wall : state.source.walls) {
    wallCount += wall.buildingIndex == buildingIndex ? 1U : 0U;
  }
  ImGui::TextDisabled("rooms %llu  floors %llu  partitions %llu",
                      static_cast<unsigned long long>(roomCount),
                      static_cast<unsigned long long>(boxCount),
                      static_cast<unsigned long long>(wallCount));

  ImGui::SeparatorText("Measured architecture");
  const cr::CreativeWorldLayoutBuildingDimensions dimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          document.gridSettings(), state.source, buildingIndex);
  if (dimensions.accepted) {
    ImGui::Text("Footprint  %.2f x %.2f m",
                dimensions.footprintWidthMeters,
                dimensions.footprintDepthMeters);
    ImGui::Text("Exterior facade  %.2f m",
                dimensions.exteriorFacadeHeightMeters);
    ImGui::Text("Total with roof  %.2f m", dimensions.totalHeightMeters);
    if (dimensions.occupiedLevelCount > 1U) {
      if (dimensions.uniformFloorToFloor) {
        ImGui::Text("Floor to floor  %.2f m",
                    dimensions.minimumFloorToFloorMeters);
      } else {
        ImGui::Text("Floor to floor  %.2f - %.2f m",
                    dimensions.minimumFloorToFloorMeters,
                    dimensions.maximumFloorToFloorMeters);
      }
    }
    if (dimensions.uniformWallHeight) {
      ImGui::Text("Interior wall  %.2f m",
                  dimensions.minimumWallHeightMeters);
    } else {
      ImGui::Text("Interior wall  %.2f - %.2f m",
                  dimensions.minimumWallHeightMeters,
                  dimensions.maximumWallHeightMeters);
    }
    if (dimensions.uniformFloorThickness) {
      ImGui::Text("Floor slab  %.2f m",
                  dimensions.minimumFloorThicknessMeters);
    } else {
      ImGui::Text("Floor slab  %.2f - %.2f m",
                  dimensions.minimumFloorThicknessMeters,
                  dimensions.maximumFloorThicknessMeters);
    }
    ImGui::Text("Roof base Y %.2f m  top Y %.2f m",
                dimensions.roofBaseMeters, dimensions.roofTopMeters);
    ImGui::TextDisabled("Human %.2f m  |  building %.1fx human height",
                        cr::kCreativeArchitecturalHumanReferenceHeightMeters,
                        dimensions.totalHeightMeters /
                            cr::kCreativeArchitecturalHumanReferenceHeightMeters);
    if (!dimensions.uniformFloorToFloor || !dimensions.uniformWallHeight ||
        !dimensions.uniformFloorThickness) {
      ImGui::TextColored(ImVec4{0.92F, 0.72F, 0.20F, 1.0F},
                         "Storey dimensions are not uniform");
    }
  } else {
    ImGui::TextDisabled("Dimensions unavailable: %.*s",
                        static_cast<int>(dimensions.reasonCode.size()),
                        dimensions.reasonCode.data());
  }

  drawCreativeEditorWorldLayoutArchitectureInspector(
      desktopUi, state, document, dimensions, buildingIndex, building,
      commands);

  ImGui::SeparatorText("Terrain placement");
  int groundingMode = static_cast<int>(building.groundingMode);
  constexpr const char* kGroundingModes[] = {"Absolute elevation",
                                              "Grounded foundation"};
  ImGui::BeginDisabled(state.buildingManipulation.active ||
                       state.buildingTransform.active);
  if (ImGui::Combo("Placement##layout_building", &groundingMode,
                   kGroundingModes, 2)) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding,
        CreativeDesktopWorldLayoutBuildingGroundingPayload{
            cr::kInvalidObjectId, buildingIndex, building.stableKey,
            {static_cast<cr::CreativeWorldLayoutGroundingMode>(groundingMode),
             building.maximumGroundReliefCells}});
  }
  if (building.groundingMode ==
      cr::CreativeWorldLayoutGroundingMode::Foundation) {
    std::uint16_t maximumRelief = building.maximumGroundReliefCells;
    constexpr std::uint16_t one = 1U;
    ImGui::SetNextItemWidth(112.0F);
    static_cast<void>(ImGui::InputScalar(
        "Max relief##layout_building", ImGuiDataType_U16, &maximumRelief,
        &one));
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding,
          CreativeDesktopWorldLayoutBuildingGroundingPayload{
              cr::kInvalidObjectId, buildingIndex, building.stableKey,
              {building.groundingMode, maximumRelief}});
    }
    ImGui::TextDisabled("cells; higher relief adds a buried foundation");
  }
  ImGui::EndDisabled();

  const auto previewTransform =
      [&](cr::CreativeWorldLayoutBuildingTransformOperation operation) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutTransformBuilding,
            CreativeDesktopWorldLayoutBuildingTransformPayload{
                CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
                operation});
      };
  ImGui::BeginDisabled(state.buildingManipulation.active);
  if (ImGui::Button("Rotate left")) {
    previewTransform(
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateLeft90);
  }
  ImGui::SameLine();
  if (ImGui::Button("Rotate right")) {
    previewTransform(
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
  }
  ImGui::SameLine();
  if (ImGui::Button("Mirror X")) {
    previewTransform(
        cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX);
  }
  ImGui::SameLine();
  if (ImGui::Button("Mirror Z")) {
    previewTransform(
        cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ);
  }
  ImGui::EndDisabled();

  if (state.buildingTransform.active) {
    ImGui::TextColored(ImVec4{0.20F, 0.78F, 0.38F, 1.0F}, "Preview: %s",
                       cr::toString(state.buildingTransform.operation).data());
    if (ImGui::Button("Apply transform")) {
      commands.enqueue(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
                    CreativeDesktopWorldLayoutBuildingTransformPayload{
                        CreativeEditorWorldLayoutBuildingTransformPhase::Commit,
                        state.buildingTransform.operation});
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel transform")) {
      commands.enqueue(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
                    CreativeDesktopWorldLayoutBuildingTransformPayload{
                        CreativeEditorWorldLayoutBuildingTransformPhase::Cancel,
                        state.buildingTransform.operation});
    }
  }

  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
  const bool canDuplicate =
      defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          state, buildingIndex, deltaXCells, deltaZCells);
  ImGui::BeginDisabled(!canDuplicate || state.buildingManipulation.active ||
                       state.buildingTransform.active);
  if (ImGui::Button("Duplicate building")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutDuplicateBuilding,
        CreativeDesktopWorldLayoutBuildingDuplicatePayload{
            buildingIndex, deltaXCells, deltaZCells});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(state.buildingManipulation.active ||
                       state.buildingTransform.active);
  if (ImGui::Button("Edit contents")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutClearSelection);
  }
  ImGui::EndDisabled();
  ImGui::Separator();
}

void drawBuildingTemplateActions(CreativeEditorDesktopUiState& desktopUi,
                                 CreativeEditorWorldLayoutState& state,
                                 CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary& library =
      state.buildingTemplates;
  ImGui::TextUnformatted("Building templates");
  const std::size_t selectedBuilding =
      creativeEditorWorldLayoutSelectedBuilding(state);
  const bool buildingAvailable =
      selectedBuilding < state.source.buildings.size();
  ImGui::BeginDisabled(!buildingAvailable || state.buildingTransform.active ||
                       state.buildingManipulation.active ||
                       state.buildingTemplatePlacement.active ||
                       library.root.empty());
  if (ImGui::Button("Save selected building")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate,
        CreativeDesktopWorldLayoutBuildingTemplateCapturePayload{
            selectedBuilding, {}});
  }
  ImGui::EndDisabled();

  cr::CreativeWorldLayoutBuildingTemplateSyncReceipt sync;
  if (buildingAvailable) {
    sync = inspectCreativeEditorWorldLayoutBuildingTemplateSync(
        state, selectedBuilding);
    ImVec4 syncColor{0.55F, 0.58F, 0.62F, 1.0F};
    switch (sync.state) {
      case cr::CreativeWorldLayoutBuildingTemplateSyncState::Current:
        syncColor = {0.20F, 0.78F, 0.38F, 1.0F};
        break;
      case cr::CreativeWorldLayoutBuildingTemplateSyncState::SourceChanged:
        syncColor = {0.88F, 0.72F, 0.20F, 1.0F};
        break;
      case cr::CreativeWorldLayoutBuildingTemplateSyncState::LocallyModified:
        syncColor = {0.94F, 0.52F, 0.18F, 1.0F};
        break;
      case cr::CreativeWorldLayoutBuildingTemplateSyncState::Conflict:
      case cr::CreativeWorldLayoutBuildingTemplateSyncState::SourceMissing:
        syncColor = {0.92F, 0.28F, 0.24F, 1.0F};
        break;
      case cr::CreativeWorldLayoutBuildingTemplateSyncState::Unlinked:
        break;
    }
    ImGui::TextColored(syncColor, "Instance: %s",
                       cr::toString(sync.state).data());
    if (sync.provenance.valid) {
      ImGui::TextDisabled("Template: %s", sync.provenance.templateId.c_str());
      ImGui::TextDisabled(
          "Version: %016llx  Pose: %s @ %d, %d",
          static_cast<unsigned long long>(sync.provenance.sourceFingerprint),
          cr::toString(sync.provenance.orientation).data(),
          sync.provenance.anchor.x, sync.provenance.anchor.z);
      if (sync.state ==
              cr::CreativeWorldLayoutBuildingTemplateSyncState::LocallyModified ||
          sync.state ==
              cr::CreativeWorldLayoutBuildingTemplateSyncState::Conflict) {
        ImGui::TextColored(ImVec4{0.94F, 0.52F, 0.18F, 1.0F},
                           "Local refinements retained");
      }
    }
  }

  const bool linkedSourceAvailable =
      buildingAvailable && sync.provenance.valid &&
      sync.state !=
          cr::CreativeWorldLayoutBuildingTemplateSyncState::SourceMissing;
  const bool linkedInstanceAvailable =
      buildingAvailable && sync.provenance.valid;
  const bool templateActionsBlocked =
      state.buildingTransform.active || state.buildingManipulation.active ||
      state.buildingTemplatePlacement.active;
  ImGui::BeginDisabled(!linkedSourceAvailable || templateActionsBlocked);
  if (ImGui::Button("Update template from selected")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                SelectedInstance});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!linkedInstanceAvailable || templateActionsBlocked);
  if (ImGui::Button("Detach instance")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutDetachBuildingTemplateInstance,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                SelectedInstance});
  }
  ImGui::EndDisabled();
  const bool selectedNeedsRefresh =
      linkedSourceAvailable &&
      sync.state != cr::CreativeWorldLayoutBuildingTemplateSyncState::Current;
  const bool selectedRefreshDestructive =
      sync.state ==
          cr::CreativeWorldLayoutBuildingTemplateSyncState::LocallyModified ||
      sync.state ==
          cr::CreativeWorldLayoutBuildingTemplateSyncState::Conflict;
  ImGui::BeginDisabled(!selectedNeedsRefresh || templateActionsBlocked);
  if (ImGui::Button("Upgrade selected")) {
    if (selectedRefreshDestructive) {
      desktopUi.pendingBuildingTemplateRebuildIndex = selectedBuilding;
      desktopUi.pendingBuildingTemplateRebuildMode =
          cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
              SelectedInstance;
      desktopUi.buildingTemplateRebuildModalOpen = true;
    } else {
      commands.enqueue(
          CreativeDesktopCommandId::
              WorldLayoutRefreshBuildingTemplateInstances,
          CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
              selectedBuilding,
              cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                  SelectedInstance});
    }
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!linkedSourceAvailable || templateActionsBlocked);
  if (ImGui::Button("Upgrade safe instances")) {
    commands.enqueue(
        CreativeDesktopCommandId::
            WorldLayoutRefreshBuildingTemplateInstances,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                SafeInstances});
  }
  ImGui::SameLine();
  if (ImGui::Button("Force upgrade all...")) {
    desktopUi.pendingBuildingTemplateRebuildIndex = selectedBuilding;
    desktopUi.pendingBuildingTemplateRebuildMode =
        cr::CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll;
    desktopUi.buildingTemplateRebuildModalOpen = true;
  }
  ImGui::EndDisabled();

  if (desktopUi.buildingTemplateRebuildModalOpen) {
    ImGui::OpenPopup("Upgrade template instances##world_layout");
    desktopUi.buildingTemplateRebuildModalOpen = false;
  }
  if (ImGui::BeginPopupModal("Upgrade template instances##world_layout",
                             nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    const bool forceAll =
        desktopUi.pendingBuildingTemplateRebuildMode ==
        cr::CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll;
    ImGui::TextUnformatted(
        forceAll
            ? "Replace every modified instance with the current template?"
            : "Replace this building's local refinements with the current template?");
    ImGui::TextDisabled("The upgrade is one undoable 3D edit.");
    if (ImGui::Button(forceAll ? "Force upgrade all" : "Upgrade selected")) {
      commands.enqueue(
          CreativeDesktopCommandId::
              WorldLayoutRefreshBuildingTemplateInstances,
          CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
              desktopUi.pendingBuildingTemplateRebuildIndex,
              desktopUi.pendingBuildingTemplateRebuildMode});
      desktopUi.pendingBuildingTemplateRebuildIndex =
          cr::kInvalidCreativeWorldLayoutIndex;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      desktopUi.pendingBuildingTemplateRebuildIndex =
          cr::kInvalidCreativeWorldLayoutIndex;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (library.templates.empty()) {
    ImGui::TextDisabled("No saved building templates");
    ImGui::TextDisabled("%s", library.statusMessage.c_str());
    ImGui::Separator();
    return;
  }

  const std::size_t selectedTemplateIndex =
      library.selectedIndex < library.templates.size()
          ? library.selectedIndex
          : 0U;
  const char* previewLabel =
      library.templates[selectedTemplateIndex].label.c_str();
  ImGui::SetNextItemWidth(220.0F);
  if (ImGui::BeginCombo("Template", previewLabel)) {
    for (std::size_t index = 0U; index < library.templates.size(); ++index) {
      const bool isSelected = index == selectedTemplateIndex;
      ImGui::PushID(static_cast<int>(index));
      if (ImGui::Selectable(library.templates[index].label.c_str(),
                            isSelected)) {
        commands.enqueue(
            CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
            CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload{
                index});
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
      ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  const cr::CreativeWorldLayoutBuildingTemplate& selectedTemplate =
      library.templates[selectedTemplateIndex];
  ImGui::TextDisabled(
      "%s  v%016llx  %d x %d cells", selectedTemplate.templateId.c_str(),
      static_cast<unsigned long long>(
          selectedTemplate.sourceFingerprint.value),
      selectedTemplate.bounds.maximum.x - selectedTemplate.bounds.minimum.x,
      selectedTemplate.bounds.maximum.z - selectedTemplate.bounds.minimum.z);

  if (!state.buildingTemplatePlacement.active) {
    ImGui::BeginDisabled(
        state.buildingTransform.active || state.buildingManipulation.active ||
        library.selectedIndex >= library.templates.size());
    if (ImGui::Button("Place template")) {
      if (state.tool != CreativeEditorWorldLayoutTool::Select) {
        commands.enqueue(CreativeDesktopCommandId::WorldLayoutSetTool,
                      CreativeDesktopWorldLayoutToolPayload{
                          CreativeEditorWorldLayoutTool::Select});
      }
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
          CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
              {0.0, 0.0},
              cr::CreativeWorldLayoutBuildingTransformOperation::
                  RotateRight90});
    }
    ImGui::EndDisabled();
  } else {
    const auto& analysis = state.buildingTemplatePlacement.analysis;
    const auto transform =
        [&](cr::CreativeWorldLayoutBuildingTransformOperation operation) {
          commands.enqueue(
              CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
                  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                      Transform,
                  {}, operation});
        };
    const ImVec4 placementColor =
        state.buildingTemplatePlacement.previewValid
            ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
            : ImVec4{0.92F, 0.29F, 0.24F, 1.0F};
    ImGui::TextColored(placementColor, "%s",
                       state.buildingTemplatePlacement.previewValid
                           ? "Placement clear"
                           : "Placement blocked");
    if (analysis.requested && analysis.bounds.valid) {
      ImGui::TextDisabled(
          "Footprint %d x %d | Levels %zu | Entrances %zu",
          analysis.footprint.maximum.x - analysis.footprint.minimum.x,
          analysis.footprint.maximum.z - analysis.footprint.minimum.z,
          analysis.levelCount, analysis.entranceCount);
      if (analysis.terrainImpact ==
          cr::CreativeWorldLayoutBuildingTemplateTerrainImpact::Foundation) {
        ImGui::TextDisabled("Terrain: foundation, relief %u cells",
                            analysis.grounding.reliefCells);
      } else {
        ImGui::TextDisabled("Terrain: %s",
                            cr::toString(analysis.terrainImpact).data());
      }
      if (analysis.conflictingBuildingIndex !=
          cr::kInvalidCreativeWorldLayoutIndex) {
        ImGui::TextColored(placementColor, "Conflict: building #%zu",
                           analysis.conflictingBuildingIndex);
      } else {
        ImGui::TextDisabled("Conflict: none");
      }
    }
    if (ImGui::Button("Rotate left##template")) {
      transform(cr::CreativeWorldLayoutBuildingTransformOperation::RotateLeft90);
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate right##template")) {
      transform(
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
    }
    ImGui::SameLine();
    if (ImGui::Button("Mirror X##template")) {
      transform(cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX);
    }
    ImGui::SameLine();
    if (ImGui::Button("Mirror Z##template")) {
      transform(cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ);
    }
    if (ImGui::Button("Cancel placement")) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
          CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel,
              {},
              cr::CreativeWorldLayoutBuildingTransformOperation::
                  RotateRight90});
    }
  }
  ImGui::TextDisabled("%s", library.statusMessage.c_str());
  ImGui::Separator();
}

void drawBoxSettings(CreativeEditorWorldLayoutState& state,
                     CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Box ||
      state.selection.index >= state.source.boxes.size()) {
    state.boxSettingsDraft = {};
    return;
  }
  const std::size_t boxIndex = state.selection.index;
  CreativeEditorWorldLayoutBoxSettings current;
  if (!readCreativeEditorWorldLayoutBoxSettings(state, boxIndex, current)) {
    state.boxSettingsDraft = {};
    return;
  }
  if (!state.boxSettingsDraft.active ||
      state.boxSettingsDraft.boxIndex != boxIndex ||
      state.boxSettingsDraft.sourceRevision != state.revision) {
    state.boxSettingsDraft = {true, boxIndex, state.revision, current};
  }

  CreativeEditorWorldLayoutBoxSettings& settings =
      state.boxSettingsDraft.settings;
  int originX = settings.footprint.minimum.x;
  int originZ = settings.footprint.minimum.z;
  const std::int64_t width64 =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depth64 =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  int width = static_cast<int>(std::clamp<std::int64_t>(
      width64, std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max()));
  int depth = static_cast<int>(std::clamp<std::int64_t>(
      depth64, std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max()));
  double anchorLayer = settings.anchorLayer;
  int layerCount = settings.layerCount;
  const cr::CreativeWorldLayoutBox& box = state.source.boxes[boxIndex];
  const cr::CreativeObjectKind kind = box.kind;
  CreativeDesktopPropertyEditActivity activity;
  bool edited = false;
  const auto observe = [&](bool changed) {
    edited = edited || changed;
    observeCreativeDesktopContinuousPropertyWidget(activity, changed);
  };

  ImGui::BeginDisabled(state.boxManipulation.active);
  ImGui::TextUnformatted(boxSettingsTitle(kind));
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("X##layout_box", &originX, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Z##layout_box", &originZ, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Width##layout_box", &width, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Depth##layout_box", &depth, 1, 4));

  ImGui::SetNextItemWidth(92.0F);
  observe(ImGui::InputDouble(boxAnchorLabel(kind), &anchorLayer, 0.25, 1.0,
                             "%.2f"));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Layers##layout_box", &layerCount, 1, 2));
  ImGui::EndDisabled();

  const std::int64_t maximumX =
      static_cast<std::int64_t>(originX) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(originZ) + depth;
  const bool valuesRepresentable =
      maximumX >= std::numeric_limits<std::int32_t>::min() &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ >= std::numeric_limits<std::int32_t>::min() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() && width > 0 &&
      depth > 0 && std::isfinite(anchorLayer) && layerCount > 0 &&
      layerCount <= std::numeric_limits<std::uint16_t>::max();
  if (edited && valuesRepresentable) {
    settings.footprint.minimum = {static_cast<std::int32_t>(originX),
                                  static_cast<std::int32_t>(originZ)};
    settings.footprint.maximum = {static_cast<std::int32_t>(maximumX),
                                  static_cast<std::int32_t>(maximumZ)};
    settings.anchorLayer = anchorLayer;
    settings.layerCount = static_cast<std::uint16_t>(layerCount);
  }
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "surface dimensions must be finite and positive");
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, settings, valuesRepresentable, "Reset surface",
      state, boxIndex, box.stableKey, commands);
}

void drawWallSettings(CreativeEditorWorldLayoutState& state,
                      CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Wall ||
      state.selection.index >= state.source.walls.size()) {
    state.wallSettingsDraft = {};
    return;
  }
  const std::size_t wallIndex = state.selection.index;
  CreativeEditorWorldLayoutWallSettings current;
  if (!readCreativeEditorWorldLayoutWallSettings(state, wallIndex, current)) {
    state.wallSettingsDraft = {};
    return;
  }
  if (!state.wallSettingsDraft.active ||
      state.wallSettingsDraft.wallIndex != wallIndex ||
      state.wallSettingsDraft.sourceRevision != state.revision) {
    state.wallSettingsDraft = {true, wallIndex, state.revision, current};
  }

  CreativeEditorWorldLayoutWallSettings& settings =
      state.wallSettingsDraft.settings;
  int startX = settings.start.x;
  int startZ = settings.start.z;
  int endX = settings.end.x;
  int endZ = settings.end.z;
  double baseLayer = settings.baseLayer;
  int height = settings.heightCells;
  double thicknessCells = settings.thicknessCells;
  const cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
  CreativeDesktopPropertyEditActivity activity;
  bool edited = false;
  const auto observe = [&](bool changed) {
    edited = edited || changed;
    observeCreativeDesktopContinuousPropertyWidget(activity, changed);
  };

  ImGui::BeginDisabled(state.wallManipulation.active);
  ImGui::TextUnformatted("Partition settings");
  ImGui::SetNextItemWidth(82.0F);
  observe(ImGui::InputInt("Start X##layout_wall", &startX, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  observe(ImGui::InputInt("Start Z##layout_wall", &startZ, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  observe(ImGui::InputInt("End X##layout_wall", &endX, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  observe(ImGui::InputInt("End Z##layout_wall", &endZ, 1, 4));

  ImGui::SetNextItemWidth(92.0F);
  observe(ImGui::InputDouble("Base##layout_wall", &baseLayer, 0.5, 1.0,
                             "%.2f"));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(92.0F);
  observe(ImGui::InputInt("Height##layout_wall", &height, 1, 2));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(110.0F);
  observe(ImGui::InputDouble("Thickness##layout_wall", &thicknessCells,
                             0.05, 0.25, "%.3f"));
  ImGui::EndDisabled();

  const bool valuesRepresentable =
      height > 0 && height <= std::numeric_limits<std::uint16_t>::max() &&
      ((startX == endX) != (startZ == endZ)) &&
      std::isfinite(baseLayer) && std::isfinite(thicknessCells) &&
      thicknessCells > 0.0;
  if (edited && valuesRepresentable) {
    settings.start = {static_cast<std::int32_t>(startX),
                      static_cast<std::int32_t>(startZ)};
    settings.end = {static_cast<std::int32_t>(endX),
                    static_cast<std::int32_t>(endZ)};
    settings.baseLayer = baseLayer;
    settings.heightCells = static_cast<std::uint16_t>(height);
    settings.thicknessCells = thicknessCells;
  }
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "partition must be cardinal, positive, and in range");
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, settings, valuesRepresentable, "Reset partition",
      state, wallIndex, wall.stableKey, commands);
}

}  // namespace

void drawCreativeEditorWorldLayoutStructureInspector(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands) {
  switch (state.selection.kind) {
    case CreativeEditorWorldLayoutSelectionKind::Building:
      drawBuildingActions(desktopUi, state, document, commands);
      drawBuildingTemplateActions(desktopUi, state, commands);
      break;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      drawCreativeEditorWorldLayoutVerticalConnectorInspector(
          state, document, commands);
      break;
    case CreativeEditorWorldLayoutSelectionKind::Box:
      drawBoxSettings(state, commands);
      break;
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      drawWallSettings(state, commands);
      break;
    case CreativeEditorWorldLayoutSelectionKind::None:
    case CreativeEditorWorldLayoutSelectionKind::Level:
    case CreativeEditorWorldLayoutSelectionKind::Room:
    case CreativeEditorWorldLayoutSelectionKind::TopologyEdge:
    case CreativeEditorWorldLayoutSelectionKind::Opening:
    case CreativeEditorWorldLayoutSelectionKind::RoofAperture:
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
    case CreativeEditorWorldLayoutSelectionKind::Object:
      break;
  }
}

}  // namespace iggy3d_creative_app
