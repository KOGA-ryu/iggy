#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorToolPresentation.hpp"
#include "EditorWorldLayout.hpp"

#include "EditorDesktopModel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* value = static_cast<std::string*>(data->UserData);
    value->resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = value->data();
  }
  return 0;
}

bool inputText(const char* label, std::string& value) {
  return ImGui::InputText(label, value.data(), value.capacity() + 1U,
                          ImGuiInputTextFlags_CallbackResize,
                          inputTextResizeCallback, &value);
}

void queueTool(CreativeDesktopCommandFrame& commands,
               CreativeEditorWorldLayoutTool tool) {
  commands.push(CreativeDesktopCommandId::WorldLayoutSetTool,
                CreativeDesktopWorldLayoutToolPayload{tool});
}

void queueLevelOperation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex,
    std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutLevelOperation,
      CreativeDesktopWorldLayoutLevelOperationPayload{operation,
                                                      buildingIndex,
                                                      levelIndex});
}

std::size_t findBuildingTemplate(
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    std::string_view templateId) noexcept {
  const auto found = std::find_if(
      library.templates.begin(), library.templates.end(),
      [templateId](const cr::CreativeWorldLayoutBuildingTemplate& value) {
        return value.templateId == templateId;
      });
  return found == library.templates.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(found - library.templates.begin());
}

void drawWorldLayoutPalette(CreativeEditorWorldLayoutState& state,
                            CreativeDesktopCommandFrame& commands) {
  if (!ImGui::BeginTabBar("##world_layout_palette")) {
    return;
  }
  for (std::uint8_t categoryValue = 0U;
       categoryValue < static_cast<std::uint8_t>(
                           CreativeEditorWorldLayoutPaletteCategory::Count);
       ++categoryValue) {
    const auto category =
        static_cast<CreativeEditorWorldLayoutPaletteCategory>(categoryValue);
    if (!ImGui::BeginTabItem(
            creativeEditorWorldLayoutPaletteCategoryLabel(category))) {
      continue;
    }
    bool first = true;
    for (const CreativeEditorWorldLayoutPaletteEntry& entry :
         creativeEditorWorldLayoutPaletteEntries()) {
      if (entry.category != category) {
        continue;
      }
      if (!first) {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float buttonWidth = ImGui::CalcTextSize(entry.label.data()).x +
                                  (2.0F * style.FramePadding.x);
        const float nextButtonRight = ImGui::GetItemRectMax().x +
                                      style.ItemSpacing.x + buttonWidth;
        const float contentRight = ImGui::GetWindowPos().x +
                                   ImGui::GetWindowContentRegionMax().x;
        if (nextButtonRight <= contentRight) {
          ImGui::SameLine();
        }
      }
      first = false;
      const bool toolActive =
          entry.activation == CreativeEditorWorldLayoutPaletteActivation::Tool &&
          state.tool == entry.tool && !state.buildingTemplatePlacement.active;
      if (toolActive) {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4{0.16F, 0.47F, 0.25F, 1.0F});
      }
      const std::size_t templateIndex =
          entry.activation ==
                  CreativeEditorWorldLayoutPaletteActivation::BuildingTemplate
              ? findBuildingTemplate(state.buildingTemplates,
                                     entry.buildingTemplateId)
              : cr::kInvalidCreativeWorldLayoutIndex;
      const bool unavailable =
          entry.activation ==
              CreativeEditorWorldLayoutPaletteActivation::BuildingTemplate &&
          templateIndex == cr::kInvalidCreativeWorldLayoutIndex;
      ImGui::BeginDisabled(unavailable);
      if (ImGui::Button(entry.label.data())) {
        if (entry.activation ==
            CreativeEditorWorldLayoutPaletteActivation::Tool) {
          queueTool(commands, entry.tool);
        } else {
          if (state.tool != CreativeEditorWorldLayoutTool::Select) {
            queueTool(commands, CreativeEditorWorldLayoutTool::Select);
          }
          commands.push(
              CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload{
                  templateIndex});
          commands.push(
              CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
                  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
                  {0.0, 0.0},
                  cr::CreativeWorldLayoutBuildingTransformOperation::
                      RotateRight90});
        }
      }
      ImGui::EndDisabled();
      if (toolActive) {
        ImGui::PopStyleColor();
      }
    }
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
}

void drawWorldLayoutAssetPlacementControls(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutCatalogPlacementState& placement =
      state.catalogPlacement;
  if (!placement.active) {
    return;
  }
  ImGui::SeparatorText("Placement");
  ImGui::TextUnformatted(placement.label.c_str());
  ImGui::TextUnformatted("Snap");
  const bool hostedOpening =
      creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
          placement.categoryId);
  if (hostedOpening) {
    ImGui::RadioButton("Wall", true);
  } else {
    constexpr std::array kSnapModes{
        CreativeEditorWorldLayoutCatalogSnapMode::Grid,
        CreativeEditorWorldLayoutCatalogSnapMode::Floor,
        CreativeEditorWorldLayoutCatalogSnapMode::Wall,
    };
    for (std::size_t index = 0U; index < kSnapModes.size(); ++index) {
      const CreativeEditorWorldLayoutCatalogSnapMode mode = kSnapModes[index];
      if (index > 0U) {
        ImGui::SameLine();
      }
      const bool wallUnsupported =
          mode == CreativeEditorWorldLayoutCatalogSnapMode::Wall &&
          !creativeEditorWorldLayoutCatalogAssetSupportsWallSnap(
              placement.categoryId);
      ImGui::BeginDisabled(wallUnsupported);
      const bool selected = placement.snapMode == mode;
      if (ImGui::RadioButton(
              creativeEditorWorldLayoutCatalogSnapModeLabel(mode), selected) &&
          !selected) {
        placement.snapMode = mode;
      }
      ImGui::EndDisabled();
      if (wallUnsupported &&
          ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Architecture assets only");
      }
    }
    ImGui::BeginDisabled(
        placement.snapMode != CreativeEditorWorldLayoutCatalogSnapMode::Grid);
    ImGui::InputDouble("Grid elevation##layout_asset",
                       &placement.elevationCells, 0.25, 1.0, "%.3f");
    ImGui::EndDisabled();
    const char* yawLabel =
        placement.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall
            ? "Yaw offset##layout_asset"
            : "Yaw##layout_asset";
    ImGui::InputDouble(yawLabel, &placement.yawDegrees, 15.0, 90.0,
                       "%.1f deg");
    if (placement.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall) {
      ImGui::Checkbox("Flip wall side##layout_asset",
                      &placement.wallSideFlipped);
    }
  }
  ImGui::InputDouble("Scale X##layout_asset", &placement.scale.x, 0.1, 1.0,
                     "%.3f");
  ImGui::InputDouble("Scale Y##layout_asset", &placement.scale.y, 0.1, 1.0,
                     "%.3f");
  ImGui::InputDouble("Scale Z##layout_asset", &placement.scale.z, 0.1, 1.0,
                     "%.3f");
  if (hostedOpening) {
    ImGui::SeparatorText("Selected opening");
    const bool hasOpeningSelection =
        state.selection.kind ==
            CreativeEditorWorldLayoutSelectionKind::Opening &&
        state.selection.index < state.source.openings.size();
    const bool compatible =
        hasOpeningSelection &&
        creativeEditorWorldLayoutCatalogAssetMatchesOpening(
            placement.categoryId,
            state.source.openings[state.selection.index].kind);
    if (!hasOpeningSelection) {
      ImGui::TextDisabled("Select an opening to replace");
    } else if (!compatible) {
      ImGui::TextDisabled("Selected opening requires a matching asset kind");
    }
    ImGui::BeginDisabled(!compatible);
    if (ImGui::Button("Fit asset to opening")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              state.selection.index,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  FitAssetToOpening,
              placement.assetId,
              placement.scale});
    }
    if (ImGui::Button("Resize opening to asset")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              state.selection.index,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  ResizeOpeningToAsset,
              placement.assetId,
              placement.scale});
    }
    ImGui::EndDisabled();
  }
  if (!hostedOpening) {
    if (ImGui::Button("Rotate left##layout_asset")) {
      placement.yawDegrees -= 90.0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate right##layout_asset")) {
      placement.yawDegrees += 90.0;
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Reset##layout_asset")) {
    placement.elevationCells = 0.0;
    placement.yawDegrees = 0.0;
    placement.scale = {1.0, 1.0, 1.0};
    placement.wallSideFlipped = false;
  }
}

bool drawWorldLayoutGlyphButton(const char* id,
                                CreativeEditorToolGlyph glyph, float tileSize,
                                bool active, std::string_view tooltip) {
  constexpr float kGlyphButtonPadding = 2.0F;
  constexpr float kGlyphButtonRounding = 2.0F;
  const ImVec2 tilePosition = ImGui::GetCursorScreenPos();
  const bool pressed =
      ImGui::InvisibleButton(id, ImVec2{tileSize, tileSize});
  const bool hovered = ImGui::IsItemHovered();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  const ImVec2 tileEnd{tilePosition.x + tileSize, tilePosition.y + tileSize};
  if (active) {
    drawList->AddRectFilled(
        tilePosition, tileEnd,
        ImGui::GetColorU32(ImVec4{0.16F, 0.47F, 0.25F, 1.0F}),
        kGlyphButtonRounding);
  } else if (hovered) {
    drawList->AddRectFilled(tilePosition, tileEnd,
                            ImGui::GetColorU32(ImGuiCol_ButtonHovered),
                            kGlyphButtonRounding);
  }
  drawCreativeEditorToolGlyph(*drawList, glyph,
                              tilePosition.x + kGlyphButtonPadding,
                              tilePosition.y + kGlyphButtonPadding,
                              tileSize - (2.0F * kGlyphButtonPadding),
                              ImGui::GetColorU32(ImGuiCol_Text));
  if (!tooltip.empty() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("%s", tooltip.data());
  }
  return pressed;
}

void drawWorldLayoutToolboxStrip(CreativeEditorState& editor,
                                 CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
  constexpr float kTileSize = 24.0F;
  const ImGuiStyle& style = ImGui::GetStyle();
  const float stripWidth = (2.0F * kTileSize) + style.ItemSpacing.x +
                           (2.0F * style.WindowPadding.x);
  const bool open =
      ImGui::BeginChild("##world_layout_toolbox", ImVec2{stripWidth, 0.0F},
                        ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
  if (!open) {
    ImGui::EndChild();
    return;
  }
  auto lastCategory = CreativeEditorWorldLayoutPaletteCategory::Count;
  int column = 0;
  int buttonId = 0;
  for (const CreativeEditorToolPresentation& presentation :
       creativeEditorToolPresentations()) {
    if (!presentation.toolbox) {
      continue;
    }
    if (presentation.category != lastCategory) {
      if (lastCategory != CreativeEditorWorldLayoutPaletteCategory::Count) {
        ImGui::Separator();
      }
      lastCategory = presentation.category;
      column = 0;
    }
    if (column == 1) {
      ImGui::SameLine();
    }
    const CreativeEditorToolPresentationStatus status =
        evaluateCreativeEditorToolPresentation(presentation, state, topography,
                                               editor.terrainGeneration);
    ImGui::PushID(buttonId++);
    ImGui::BeginDisabled(status.unavailable);
    const bool pressed = drawWorldLayoutGlyphButton(
        "##tool", presentation.glyph, kTileSize, status.active,
        presentation.name);
    ImGui::EndDisabled();
    if (pressed && !status.unavailable) {
      switch (presentation.activation) {
        case CreativeEditorToolActivation::WorldLayoutTool: {
          queueTool(commands, presentation.tool);
          break;
        }
        case CreativeEditorToolActivation::BuildingTemplate: {
          const std::size_t templateIndex = findBuildingTemplate(
              state.buildingTemplates, presentation.buildingTemplateId);
          if (templateIndex == cr::kInvalidCreativeWorldLayoutIndex) {
            break;
          }
          if (state.tool != CreativeEditorWorldLayoutTool::Select) {
            queueTool(commands, CreativeEditorWorldLayoutTool::Select);
          }
          commands.push(
              CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload{
                  templateIndex});
          commands.push(
              CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
                  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                      Begin,
                  {0.0, 0.0},
                  cr::CreativeWorldLayoutBuildingTransformOperation::
                      RotateRight90});
          break;
        }
        case CreativeEditorToolActivation::TerrainRegionSession: {
          if (topography.region.editingEnabled) {
            topography.region.editingEnabled = false;
            commands.push(
                CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
          } else {
            topography.region.editingEnabled = true;
            topography.visible = true;
            topography.elevationBandsVisible = true;
          }
          break;
        }
        case CreativeEditorToolActivation::Count:
          break;
      }
    }
    ImGui::PopID();
    column = (column + 1) % 2;
  }
  ImGui::EndChild();
}

void drawWorldLayoutAssetPalette(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands) {
  ImGui::SeparatorText("Asset library");
  inputText("Search##world_layout_assets", state.assetQuery);
  if (!ImGui::BeginTabBar("##world_layout_asset_categories")) {
    return;
  }
  for (std::uint8_t categoryValue = 0U;
       categoryValue < static_cast<std::uint8_t>(
                           CreativeEditorWorldLayoutAssetCategory::Count);
       ++categoryValue) {
    const auto category =
        static_cast<CreativeEditorWorldLayoutAssetCategory>(categoryValue);
    if (!ImGui::BeginTabItem(
            creativeEditorWorldLayoutAssetCategoryLabel(category))) {
      continue;
    }
    state.assetCategory = category;
    std::size_t visibleCount = 0U;
    if (ImGui::BeginChild("##world_layout_asset_list", {0.0F, 210.0F}, true)) {
      for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
        if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
            classifyCreativeEditorWorldLayoutAsset(entry) != category ||
            !creativeEditorWorldLayoutAssetMatchesQuery(entry,
                                                        state.assetQuery)) {
          continue;
        }
        ++visibleCount;
        const std::string_view assetId =
            cr::creativeHotbarAssetId(entry.hotbarEntry);
        const bool selected =
            state.tool == CreativeEditorWorldLayoutTool::CatalogAsset &&
            state.catalogPlacement.active &&
            state.catalogPlacement.assetId == assetId;
        ImGui::PushID(assetId.data(), assetId.data() + assetId.size());
        if (ImGui::Selectable(entry.label.c_str(), selected)) {
          commands.push(
              CreativeDesktopCommandId::WorldLayoutSelectCatalogAsset,
              CreativeDesktopWorldLayoutCatalogAssetPayload{
                  std::string(assetId)});
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%.*s\n%s", static_cast<int>(assetId.size()),
                            assetId.data(),
                            entry.assetAuthoringMetadata.categoryId.c_str());
        }
        ImGui::PopID();
      }
      if (visibleCount == 0U) {
        ImGui::TextDisabled("No matching assets");
      }
    }
    ImGui::EndChild();
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
  drawWorldLayoutAssetPlacementControls(state, commands);
}

void drawWorldLayoutLevels(CreativeEditorDesktopUiState& desktopUi,
                           CreativeEditorWorldLayoutState& state,
                           CreativeDesktopCommandFrame& commands) {
  std::size_t buildingIndex = creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      state.activeLevelIndex < state.source.levels.size()) {
    buildingIndex = state.source.levels[state.activeLevelIndex].buildingIndex;
  }
  if (buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      state.source.buildings.size() == 1U) {
    buildingIndex = 0U;
  }

  ImGui::TextUnformatted("Levels");
  ImGui::Separator();
  bool anyLevel = false;
  for (std::size_t index = 0U; index < state.source.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& level = state.source.levels[index];
    if (level.buildingIndex != buildingIndex) {
      continue;
    }
    anyLevel = true;
    const bool active = index == state.activeLevelIndex;
    const std::string label = level.name + "##layout_level_" +
                              std::to_string(index);
    if (ImGui::Selectable(label.c_str(), active)) {
      queueLevelOperation(commands,
                          CreativeEditorWorldLayoutLevelOperation::Select,
                          buildingIndex, index);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Floor top %.3f", level.floorTopLayer);
    }
  }
  if (!anyLevel && buildingIndex != cr::kInvalidCreativeWorldLayoutIndex) {
    ImGui::TextDisabled("No levels");
  }

  const bool hasBuilding = buildingIndex < state.source.buildings.size();
  const bool hasLevel = state.activeLevelIndex < state.source.levels.size() &&
                        state.source.levels[state.activeLevelIndex]
                                .buildingIndex == buildingIndex;
  ImGui::BeginDisabled(!hasBuilding);
  if (ImGui::SmallButton("+##layout_level_add")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::Add,
                        buildingIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Add level above this building");
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!hasLevel);
  if (ImGui::SmallButton("Copy##layout_level_copy")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::Duplicate,
                        buildingIndex, state.activeLevelIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Duplicate this level and its rooms above the building");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Up##layout_level_earlier")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::MoveEarlier,
                        buildingIndex, state.activeLevelIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Move level earlier in the tab order");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Down##layout_level_later")) {
    queueLevelOperation(commands,
                        CreativeEditorWorldLayoutLevelOperation::MoveLater,
                        buildingIndex, state.activeLevelIndex);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("Move level later in the tab order");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Delete##layout_level_delete")) {
    desktopUi.worldLayoutDeleteLevelIndex = state.activeLevelIndex;
    ImGui::OpenPopup("Delete building level");
  }
  ImGui::EndDisabled();
}

void drawWorldLayoutLevelDeleteModal(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  if (ImGui::BeginPopupModal("Delete building level", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    const std::size_t levelIndex = desktopUi.worldLayoutDeleteLevelIndex;
    std::size_t roomCount = 0U;
    std::size_t openingCount = 0U;
    std::array<std::size_t,
               static_cast<std::size_t>(
                   cr::CreativeWorldLayoutVerticalConnectorKind::Count)>
        connectorCounts{};
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
    if (levelIndex < state.source.levels.size()) {
      buildingIndex = state.source.levels[levelIndex].buildingIndex;
      roomCount = static_cast<std::size_t>(std::count_if(
          state.source.rooms.begin(), state.source.rooms.end(),
          [levelIndex](const cr::CreativeWorldLayoutRoom& room) {
            return room.levelIndex == levelIndex;
          }));
      openingCount = static_cast<std::size_t>(std::count_if(
          state.source.openings.begin(), state.source.openings.end(),
          [&state, levelIndex](const cr::CreativeWorldLayoutOpening& opening) {
            return opening.hostKind ==
                       cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                   opening.roomIndex < state.source.rooms.size() &&
                   state.source.rooms[opening.roomIndex].levelIndex ==
                       levelIndex;
          }));
      for (const cr::CreativeWorldLayoutVerticalConnector& connector :
           state.source.verticalConnectors) {
        const bool touchesLevel =
            (connector.lowerRoomIndex < state.source.rooms.size() &&
             state.source.rooms[connector.lowerRoomIndex].levelIndex ==
                 levelIndex) ||
            (connector.upperRoomIndex < state.source.rooms.size() &&
             state.source.rooms[connector.upperRoomIndex].levelIndex ==
                 levelIndex);
        const std::size_t kind = static_cast<std::size_t>(connector.kind);
        if (touchesLevel && kind < connectorCounts.size()) {
          ++connectorCounts[kind];
        }
      }
    }
    ImGui::Text("Delete this level, %llu room(s), %llu opening(s), %llu "
                "stair(s), and %llu ramp(s)?",
                static_cast<unsigned long long>(roomCount),
                static_cast<unsigned long long>(openingCount),
                static_cast<unsigned long long>(
                    connectorCounts[static_cast<std::size_t>(
                        cr::CreativeWorldLayoutVerticalConnectorKind::Stair)]),
                static_cast<unsigned long long>(
                    connectorCounts[static_cast<std::size_t>(
                        cr::CreativeWorldLayoutVerticalConnectorKind::Ramp)]));
    ImGui::BeginDisabled(levelIndex >= state.source.levels.size());
    if (ImGui::Button("Delete level")) {
      queueLevelOperation(commands,
                          CreativeEditorWorldLayoutLevelOperation::Delete,
                          buildingIndex, levelIndex);
      desktopUi.worldLayoutDeleteLevelIndex =
          std::numeric_limits<std::size_t>::max();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      desktopUi.worldLayoutDeleteLevelIndex =
          std::numeric_limits<std::size_t>::max();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}


}  // namespace

bool drawCreativeEditorWorldLayoutGlyphButton(
    const char* id, CreativeEditorToolGlyph glyph, float tileSize,
    bool active, std::string_view tooltip) {
  return drawWorldLayoutGlyphButton(id, glyph, tileSize, active, tooltip);
}

namespace {

constexpr std::array<CreativeEditorWorldLayoutBlockoutPatternChoice, 4U>
    kBlockoutPatternChoices = {{
        {"1 room", cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom},
        {"Split X", cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitX},
        {"Split Z", cr::CreativeWorldLayoutBuildingBlockoutPattern::SplitZ},
        {"2 x 2", cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2},
    }};

// Create-tab Building Blockout section: edits one transient draft in the
// desktop UI state and emits exactly one typed blockout command on Stage.
// The section never touches the source, never recomputes planner split math,
// and leaves preview/confirm to the existing bottom Build panel.
void drawWorldLayoutBuildingBlockoutSection(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutBuildingBlockoutSettings& draft =
      desktopUi.worldLayoutBlockoutDraft;
  ImGui::SeparatorText("Building Blockout");

  int minimumX = static_cast<int>(draft.shell.footprint.minimum.x);
  int minimumZ = static_cast<int>(draft.shell.footprint.minimum.z);
  int maximumX = static_cast<int>(draft.shell.footprint.maximum.x);
  int maximumZ = static_cast<int>(draft.shell.footprint.maximum.z);
  ImGui::SetNextItemWidth(112.0F);
  if (ImGui::InputInt("Min X##blockout", &minimumX)) {
    draft.shell.footprint.minimum.x = static_cast<std::int32_t>(minimumX);
  }
  ImGui::SetNextItemWidth(112.0F);
  if (ImGui::InputInt("Min Z##blockout", &minimumZ)) {
    draft.shell.footprint.minimum.z = static_cast<std::int32_t>(minimumZ);
  }
  ImGui::SetNextItemWidth(112.0F);
  if (ImGui::InputInt("Max X##blockout", &maximumX)) {
    draft.shell.footprint.maximum.x = static_cast<std::int32_t>(maximumX);
  }
  ImGui::SetNextItemWidth(112.0F);
  if (ImGui::InputInt("Max Z##blockout", &maximumZ)) {
    draft.shell.footprint.maximum.z = static_cast<std::int32_t>(maximumZ);
  }

  bool firstChoice = true;
  for (const CreativeEditorWorldLayoutBlockoutPatternChoice& choice :
       kBlockoutPatternChoices) {
    if (!firstChoice) {
      ImGui::SameLine();
    }
    firstChoice = false;
    if (ImGui::RadioButton(choice.label, draft.pattern == choice.pattern)) {
      draft.pattern = choice.pattern;
    }
  }

  ImGui::SetNextItemWidth(140.0F);
  ImGui::InputDouble("Floor top##blockout", &draft.shell.floorTopLayer, 0.25,
                     1.0, "%.3f");
  ImGui::SetNextItemWidth(140.0F);
  ImGui::InputScalar("Wall height##blockout", ImGuiDataType_U16,
                     &draft.shell.wallHeightCells);
  ImGui::SetNextItemWidth(112.0F);
  ImGui::InputDouble("Wall thickness##blockout",
                     &draft.shell.wallThicknessCells, 0.05, 0.25, "%.3f");
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Floor layers##blockout", ImGuiDataType_U16,
                     &draft.shell.floorThicknessLayers);
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Roof layers##blockout", ImGuiDataType_U16,
                     &draft.shell.roofThicknessLayers);

  if (ImGui::TreeNode("Roof##blockout")) {
    constexpr std::array roofStyles{cr::CreativeStructuralRoofStyle::Flat,
                                    cr::CreativeStructuralRoofStyle::Gable};
    constexpr std::array ridgeAxes{cr::CreativeStructuralRoofRidgeAxis::X,
                                   cr::CreativeStructuralRoofRidgeAxis::Z};
    const auto roofStyleLabel = [](cr::CreativeStructuralRoofStyle style) {
      return style == cr::CreativeStructuralRoofStyle::Gable ? "Gable"
                                                             : "Flat";
    };
    const auto ridgeLabel = [](cr::CreativeStructuralRoofRidgeAxis axis) {
      return axis == cr::CreativeStructuralRoofRidgeAxis::Z ? "Z axis"
                                                            : "X axis";
    };
    ImGui::SetNextItemWidth(140.0F);
    if (ImGui::BeginCombo("Style##blockout",
                          roofStyleLabel(draft.shell.roofStyle))) {
      for (const cr::CreativeStructuralRoofStyle style : roofStyles) {
        const bool selected = style == draft.shell.roofStyle;
        if (ImGui::Selectable(roofStyleLabel(style), selected)) {
          draft.shell.roofStyle = style;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(140.0F);
    ImGui::InputDouble("Overhang##blockout", &draft.shell.roofOverhangCells,
                       0.25, 1.0, "%.2f");
    if (draft.shell.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
      ImGui::SetNextItemWidth(140.0F);
      if (ImGui::BeginCombo("Ridge##blockout",
                            ridgeLabel(draft.shell.roofRidgeAxis))) {
        for (const cr::CreativeStructuralRoofRidgeAxis axis : ridgeAxes) {
          const bool selected = axis == draft.shell.roofRidgeAxis;
          if (ImGui::Selectable(ridgeLabel(axis), selected)) {
            draft.shell.roofRidgeAxis = axis;
          }
          if (selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::SetNextItemWidth(140.0F);
      ImGui::InputDouble("Pitch##blockout", &draft.shell.roofPitchDegrees,
                         1.0, 5.0, "%.1f deg");
    }
    ImGui::TreePop();
  }

  ImGui::BeginDisabled(creativeEditorWorldLayoutPreviewActive(state));
  if (ImGui::Button("Stage blockout")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
        CreativeDesktopWorldLayoutBuildingBlockoutPayload{draft});
  }
  ImGui::EndDisabled();
  ImGui::TextWrapped("%s", state.statusMessage.c_str());
}

}  // namespace

std::span<const CreativeEditorWorldLayoutBlockoutPatternChoice>
creativeEditorWorldLayoutBlockoutPatternChoices() noexcept {
  return kBlockoutPatternChoices;
}

void drawCreativeEditorWorldLayoutCreateTools(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands, bool unavailable) {
  ImGui::BeginDisabled(unavailable);
  drawWorldLayoutBuildingBlockoutSection(desktopUi, state, commands);
  ImGui::Spacing();
  ImGui::Separator();
  drawWorldLayoutPalette(state, commands);
  ImGui::Spacing();
  drawWorldLayoutAssetPalette(state, catalog, commands);
  ImGui::Spacing();
  drawWorldLayoutLevels(desktopUi, state, commands);
  ImGui::EndDisabled();
  drawWorldLayoutLevelDeleteModal(desktopUi, state, commands);
}

void drawCreativeEditorWorldLayoutToolboxStrip(
    CreativeEditorState& editor, CreativeDesktopCommandFrame& commands) {
  drawWorldLayoutToolboxStrip(editor, commands);
}

}  // namespace iggy3d_creative_app
