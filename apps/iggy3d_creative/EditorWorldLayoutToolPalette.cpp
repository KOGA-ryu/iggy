#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorDesktopWidgets.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutBuildings.hpp"

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
  commands.enqueue(CreativeDesktopCommandId::WorldLayoutSetTool,
                CreativeDesktopWorldLayoutToolPayload{tool});
}

void queueLevelOperation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex,
    std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutLevelOperation,
      CreativeDesktopWorldLayoutLevelOperationPayload{operation,
                                                      buildingIndex,
                                                      levelIndex});
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
    const auto drawTools = [&](bool experimental) {
      bool first = true;
      for (const CreativeEditorToolDescriptor& descriptor :
           creativeEditorToolDescriptors()) {
        if (descriptor.worldLayoutActivation !=
                CreativeEditorWorldLayoutToolActivation::Tool ||
            creativeEditorWorldLayoutPaletteCategory(descriptor) != category ||
            creativeEditorToolExperimental(descriptor) != experimental ||
            (!experimental && !creativeEditorToolDefaultVisible(descriptor))) {
          continue;
        }
        if (!first) {
          const ImGuiStyle& style = ImGui::GetStyle();
          const float buttonWidth = ImGui::CalcTextSize(descriptor.name.data()).x +
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
            state.tool == descriptor.worldLayoutTool &&
            !state.buildingTemplatePlacement.active;
        if (toolActive) {
          ImGui::PushStyleColor(ImGuiCol_Button,
                                ImVec4{0.16F, 0.47F, 0.25F, 1.0F});
        }
        if (ImGui::Button(descriptor.name.data())) {
          queueTool(commands, descriptor.worldLayoutTool);
        }
        if (toolActive) {
          ImGui::PopStyleColor();
        }
      }
    };
    drawTools(false);
    if (ImGui::CollapsingHeader("Experimental Tools")) {
      drawTools(true);
    }
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();

  if (state.buildingTemplates.templates.empty()) {
    return;
  }
  ImGui::SeparatorText("Building templates");
  bool first = true;
  for (std::size_t templateIndex = 0U;
       templateIndex < state.buildingTemplates.templates.size();
       ++templateIndex) {
    const cr::CreativeWorldLayoutBuildingTemplate& buildingTemplate =
        state.buildingTemplates.templates[templateIndex];
    if (!first) {
      ImGui::SameLine();
    }
    first = false;
    const std::string_view label = buildingTemplate.label.empty()
                                       ? std::string_view{buildingTemplate.templateId}
                                       : std::string_view{buildingTemplate.label};
    if (ImGui::Button(label.data())) {
      if (state.tool != CreativeEditorWorldLayoutTool::Select) {
        queueTool(commands, CreativeEditorWorldLayoutTool::Select);
      }
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
          CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload{
              templateIndex});
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
          CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
              {0.0, 0.0},
              cr::CreativeWorldLayoutBuildingTransformOperation::
                  RotateRight90});
    }
  }
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
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              state.selection.index,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  FitAssetToOpening,
              placement.assetId,
              placement.scale});
    }
    if (ImGui::Button("Resize opening to asset")) {
      commands.enqueue(
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
  return drawCreativeEditorToolGlyphButton(id, glyph, tileSize, active,
                                           tooltip);
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
  int buttonId = 0;
  const auto activate = [&](const CreativeEditorToolDescriptor& descriptor) {
    switch (descriptor.worldLayoutActivation) {
      case CreativeEditorWorldLayoutToolActivation::Tool:
        queueTool(commands, descriptor.worldLayoutTool);
        break;
      case CreativeEditorWorldLayoutToolActivation::TerrainRegionSession:
        if (topography.region.editingEnabled) {
          topography.region.editingEnabled = false;
          commands.enqueue(CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
        } else {
          topography.region.editingEnabled = true;
          topography.visible = true;
          topography.elevationBandsVisible = true;
        }
        break;
      case CreativeEditorWorldLayoutToolActivation::None:
      case CreativeEditorWorldLayoutToolActivation::Count:
        break;
    }
  };
  const auto drawDescriptorSet = [&](bool experimental) {
    auto lastCategory = CreativeEditorWorldLayoutPaletteCategory::Count;
    int column = 0;
    for (const CreativeEditorToolDescriptor& descriptor :
         creativeEditorToolDescriptors()) {
      if (!creativeEditorToolHasWorldLayoutSurface(descriptor) ||
          creativeEditorToolExperimental(descriptor) != experimental ||
          (!experimental && !creativeEditorToolDefaultVisible(descriptor))) {
        continue;
      }
      const CreativeEditorWorldLayoutPaletteCategory category =
          creativeEditorWorldLayoutPaletteCategory(descriptor);
      if (category != lastCategory) {
        if (lastCategory != CreativeEditorWorldLayoutPaletteCategory::Count) {
          ImGui::Separator();
        }
        lastCategory = category;
        column = 0;
      }
      if (column == 1) {
        ImGui::SameLine();
      }
      const CreativeEditorToolStatus status = evaluateCreativeEditorTool(
          descriptor, state, topography, editor.terrainGeneration);
      ImGui::PushID(buttonId++);
      ImGui::BeginDisabled(status.unavailable);
      const bool pressed = drawWorldLayoutGlyphButton(
          "##tool", descriptor.glyph, kTileSize, status.active,
          descriptor.name);
      ImGui::EndDisabled();
      if (pressed && !status.unavailable) {
        activate(descriptor);
      }
      ImGui::PopID();
      column = (column + 1) % 2;
    }
  };
  drawDescriptorSet(false);
  ImGui::Separator();
  if (drawWorldLayoutGlyphButton(
          "##experimental_tools", CreativeEditorToolGlyph::BadgeTemplate,
          kTileSize, state.experimentalToolsVisible,
          "Experimental Tools (M0-M2)")) {
    state.experimentalToolsVisible = !state.experimentalToolsVisible;
  }
  if (state.experimentalToolsVisible) {
    ImGui::Separator();
    drawDescriptorSet(true);
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
          commands.enqueue(
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

bool drawBlockoutMaterialCombo(const char* label,
                               cr::CreativeStructuralMaterial& material) {
  bool changed = false;
  ImGui::SetNextItemWidth(140.0F);
  if (ImGui::BeginCombo(label, cr::toString(material).data())) {
    for (const cr::CreativeStructuralMaterial candidate :
         {cr::CreativeStructuralMaterial::Blockout,
          cr::CreativeStructuralMaterial::Plaster,
          cr::CreativeStructuralMaterial::Timber,
          cr::CreativeStructuralMaterial::Stone,
          cr::CreativeStructuralMaterial::Brick}) {
      const bool selected = candidate == material;
      if (ImGui::Selectable(cr::toString(candidate).data(), selected)) {
        material = candidate;
        changed = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  return changed;
}

// The one blockout settings drawer, shared verbatim by the Create draft and
// the selected-building Edit draft so the two surfaces cannot drift. Only one
// mode is visible per frame, so the widget ids stay stable across modes.
void drawWorldLayoutBlockoutSettingsDrawer(
    CreativeEditorWorldLayoutBuildingBlockoutSettings& draft,
    cr::CreativeGridSettings grid) {
  if (draft.architecturalProfileKind !=
      cr::CreativeWorldLayoutArchitecturalProfileKind::Custom) {
    static_cast<void>(
        applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
            draft, grid, draft.architecturalProfileKind));
  }

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

  ImGui::SeparatorText("Architecture");
  ImGui::SetNextItemWidth(180.0F);
  if (ImGui::BeginCombo(
          "Profile##blockout",
          cr::toString(draft.architecturalProfileKind).data())) {
    for (const cr::CreativeWorldLayoutArchitecturalProfileKind kind :
         {cr::CreativeWorldLayoutArchitecturalProfileKind::Residential,
          cr::CreativeWorldLayoutArchitecturalProfileKind::Grand,
          cr::CreativeWorldLayoutArchitecturalProfileKind::Custom}) {
      CreativeEditorWorldLayoutBuildingBlockoutSettings candidate = draft;
      const bool available =
          kind == cr::CreativeWorldLayoutArchitecturalProfileKind::Custom ||
          applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
              candidate, grid, kind);
      const bool selected = draft.architecturalProfileKind == kind;
      ImGui::BeginDisabled(!available);
      if (ImGui::Selectable(cr::toString(kind).data(), selected)) {
        if (kind ==
            cr::CreativeWorldLayoutArchitecturalProfileKind::Custom) {
          draft.architecturalProfileKind = kind;
        } else {
          static_cast<void>(
              applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
                  draft, grid, kind));
        }
      }
      ImGui::EndDisabled();
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SetNextItemWidth(152.0F);
  if (ImGui::InputScalar("Floor-to-floor##blockout", ImGuiDataType_U16,
                         &draft.floorToFloorCells)) {
    draft.architecturalProfileKind =
        cr::CreativeWorldLayoutArchitecturalProfileKind::Custom;
  }
  ImGui::SameLine();
  ImGui::TextDisabled("%.2f m",
                      draft.floorToFloorCells * grid.cellSizeMeters);
  ImGui::SetNextItemWidth(120.0F);
  if (ImGui::InputScalar("Floor slab##blockout", ImGuiDataType_U16,
                         &draft.shell.floorThicknessLayers)) {
    draft.architecturalProfileKind =
        cr::CreativeWorldLayoutArchitecturalProfileKind::Custom;
  }
  ImGui::SetNextItemWidth(120.0F);
  if (ImGui::InputScalar("Ceiling##blockout", ImGuiDataType_U16,
                         &draft.ceilingThicknessLayers)) {
    draft.architecturalProfileKind =
        cr::CreativeWorldLayoutArchitecturalProfileKind::Custom;
  }
  ImGui::SetNextItemWidth(120.0F);
  if (ImGui::InputScalar("Roof slab##blockout", ImGuiDataType_U16,
                         &draft.shell.roofThicknessLayers)) {
    draft.architecturalProfileKind =
        cr::CreativeWorldLayoutArchitecturalProfileKind::Custom;
  }

  ImGui::SeparatorText("Floor plan");
  ImGui::Checkbox("Interior doors##blockout", &draft.connectRooms);
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Storeys##blockout", ImGuiDataType_U16,
                     &draft.storeys.count);
  ImGui::Checkbox("Connect storeys##blockout", &draft.storeys.connectStoreys);
  if (draft.storeys.connectStoreys) {
    ImGui::SetNextItemWidth(140.0F);
    if (ImGui::BeginCombo(
            "Connector##blockout",
            creativeEditorWorldLayoutVerticalConnectorKindLabel(
                draft.storeys.connectorKind))) {
      for (const cr::CreativeWorldLayoutVerticalConnectorKind kind :
           {cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
            cr::CreativeWorldLayoutVerticalConnectorKind::Ramp}) {
        const bool selected = draft.storeys.connectorKind == kind;
        if (ImGui::Selectable(
                creativeEditorWorldLayoutVerticalConnectorKindLabel(kind),
                selected)) {
          draft.storeys.connectorKind = kind;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(188.0F);
    if (ImGui::BeginCombo(
            "Direction##blockout",
            creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
                draft.storeys.preferredDirection))) {
      for (const cr::CreativeWorldLayoutVerticalDirection direction :
           {cr::CreativeWorldLayoutVerticalDirection::PositiveX,
            cr::CreativeWorldLayoutVerticalDirection::NegativeX,
            cr::CreativeWorldLayoutVerticalDirection::PositiveZ,
            cr::CreativeWorldLayoutVerticalDirection::NegativeZ}) {
        const bool selected = draft.storeys.preferredDirection == direction;
        if (ImGui::Selectable(
                creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
                    direction),
                selected)) {
          draft.storeys.preferredDirection = direction;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }

  ImGui::SeparatorText("Facade");
  ImGui::Checkbox("Entrance##blockout", &draft.facade.includeEntrance);
  if (draft.facade.includeEntrance) {
    ImGui::SetNextItemWidth(140.0F);
    if (ImGui::BeginCombo("Entrance side##blockout",
                          cr::toString(draft.facade.entranceEdge).data())) {
      for (const cr::CreativeWorldLayoutRoomEdge edge :
           {cr::CreativeWorldLayoutRoomEdge::North,
            cr::CreativeWorldLayoutRoomEdge::East,
            cr::CreativeWorldLayoutRoomEdge::South,
            cr::CreativeWorldLayoutRoomEdge::West}) {
        const bool selected = draft.facade.entranceEdge == edge;
        if (ImGui::Selectable(cr::toString(edge).data(), selected)) {
          draft.facade.entranceEdge = edge;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(140.0F);
    ImGui::InputDouble("Entrance offset##blockout",
                       &draft.facade.entranceOffsetCells, 0.25, 1.0, "%.2f");
  }
  ImGui::Checkbox("Exterior windows##blockout",
                  &draft.facade.includeExteriorWindows);

  ImGui::SeparatorText("Structure");
  ImGui::SetNextItemWidth(140.0F);
  ImGui::InputDouble("Floor top##blockout", &draft.shell.floorTopLayer, 0.25,
                     1.0, "%.3f");
  ImGui::SetNextItemWidth(112.0F);
  ImGui::InputDouble("Wall thickness##blockout",
                     &draft.shell.wallThicknessCells, 0.05, 0.25, "%.3f");
  static_cast<void>(drawBlockoutMaterialCombo(
      "Exterior walls##blockout", draft.exteriorWallMaterial));
  static_cast<void>(drawBlockoutMaterialCombo(
      "Interior walls##blockout", draft.interiorWallMaterial));
  ImGui::TextDisabled(
      "Wall ownership follows the floor-plan topology automatically");

  if (ImGui::TreeNode("Roof##blockout")) {
    CreativeDesktopPropertyEditActivity roofActivity;
    drawCreativeStructuralRoofSettingsWidgets(
        draft.shell, roofActivity, "blockout_roof");
    ImGui::TreePop();
  }

}

// Create-tab Building Blockout section. Create mode edits the transient
// Create draft and stages one new building; Edit mode loads the selected
// building through the backend read contract into its own draft, shows
// whether that draft is still in sync with the source revision, and applies
// through the typed update command. Both modes render the same drawer and
// surface the raw status message; preview/confirm stay in the Build panel.
void drawWorldLayoutBuildingBlockoutSection(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorDesktopBlockoutEditDraft& edit =
      desktopUi.worldLayoutBlockoutEdit;
  if (edit.active && edit.buildingIndex >= state.source.buildings.size()) {
    edit = {};
  }
  ImGui::SeparatorText("Building Blockout");
  const bool previewActive = creativeEditorWorldLayoutPreviewActive(state);
  if (edit.active) {
    ImGui::TextDisabled(
        "Editing %s", state.source.buildings[edit.buildingIndex].name.c_str());
    ImGui::TextDisabled(
        "%s", creativeEditorWorldLayoutBlockoutEditInSync(edit, state)
                  ? "In sync with source."
                  : "Source changed since this draft was read.");
    drawWorldLayoutBlockoutSettingsDrawer(edit.settings, grid);
    ImGui::BeginDisabled(previewActive);
    if (ImGui::Button("Apply blockout")) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutUpdateBuildingBlockout,
          CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload{
              edit.buildingIndex, edit.settings});
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel##blockout_edit")) {
      edit = {};
    }
  } else {
    drawWorldLayoutBlockoutSettingsDrawer(desktopUi.worldLayoutBlockoutDraft,
                                          grid);
    ImGui::BeginDisabled(previewActive);
    if (ImGui::Button("Stage blockout")) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
          CreativeDesktopWorldLayoutBuildingBlockoutPayload{
              desktopUi.worldLayoutBlockoutDraft});
    }
    ImGui::EndDisabled();
    if (state.selection.kind ==
        CreativeEditorWorldLayoutSelectionKind::Building) {
      CreativeEditorWorldLayoutBuildingBlockoutSettings readSettings;
      const bool readable =
          readCreativeEditorWorldLayoutBuildingBlockoutSettings(
              state, state.selection.index, readSettings);
      ImGui::SameLine();
      ImGui::BeginDisabled(!readable);
      if (ImGui::Button("Edit selected##blockout")) {
        edit = {true, state.selection.index, state.revision, readSettings};
      }
      ImGui::EndDisabled();
    }
  }
  ImGui::TextWrapped("%s", state.statusMessage.c_str());
}

}  // namespace

bool creativeEditorWorldLayoutBlockoutEditInSync(
    const CreativeEditorDesktopBlockoutEditDraft& draft,
    const CreativeEditorWorldLayoutState& state) noexcept {
  return draft.active && draft.buildingIndex < state.source.buildings.size() &&
         draft.sourceRevision == state.revision;
}

std::span<const CreativeEditorWorldLayoutBlockoutPatternChoice>
creativeEditorWorldLayoutBlockoutPatternChoices() noexcept {
  return kBlockoutPatternChoices;
}

void drawCreativeEditorWorldLayoutCreateTools(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    cr::CreativeGridSettings grid,
    CreativeDesktopCommandFrame& commands, bool unavailable) {
  ImGui::BeginDisabled(unavailable);
  drawWorldLayoutBuildingBlockoutSection(desktopUi, state, grid, commands);
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
