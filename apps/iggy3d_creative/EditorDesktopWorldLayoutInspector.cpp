#include "EditorDesktopWorldLayoutInspector.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

bool sameBoxSettings(const CreativeEditorWorldLayoutBoxSettings& lhs,
                     const CreativeEditorWorldLayoutBoxSettings& rhs) noexcept {
  return lhs.footprint.minimum == rhs.footprint.minimum &&
         lhs.footprint.maximum == rhs.footprint.maximum &&
         lhs.anchorLayer == rhs.anchorLayer && lhs.layerCount == rhs.layerCount;
}

bool sameWallSettings(
    const CreativeEditorWorldLayoutWallSettings& lhs,
    const CreativeEditorWorldLayoutWallSettings& rhs) noexcept {
  return lhs.start == rhs.start && lhs.end == rhs.end &&
         lhs.baseLayer == rhs.baseLayer &&
         lhs.heightCells == rhs.heightCells &&
         lhs.thicknessCells == rhs.thicknessCells;
}

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

const char* boxApplyLabel(cr::CreativeObjectKind kind) noexcept {
  switch (kind) {
    case cr::CreativeObjectKind::Floor: return "Apply floor";
    case cr::CreativeObjectKind::Ceiling: return "Apply ceiling";
    case cr::CreativeObjectKind::Roof: return "Apply roof";
    default: return "Apply volume";
  }
}

void drawBuildingActions(CreativeEditorWorldLayoutState& state,
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
      commands.push(
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

  ImGui::SeparatorText("Terrain placement");
  int groundingMode = static_cast<int>(building.groundingMode);
  constexpr const char* kGroundingModes[] = {"Absolute elevation",
                                              "Grounded foundation"};
  ImGui::BeginDisabled(state.buildingManipulation.active ||
                       state.buildingTransform.active);
  if (ImGui::Combo("Placement##layout_building", &groundingMode,
                   kGroundingModes, 2)) {
    commands.push(
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
      commands.push(
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
        commands.push(
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
      commands.push(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
                    CreativeDesktopWorldLayoutBuildingTransformPayload{
                        CreativeEditorWorldLayoutBuildingTransformPhase::Commit,
                        state.buildingTransform.operation});
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel transform")) {
      commands.push(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
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
    commands.push(
        CreativeDesktopCommandId::WorldLayoutDuplicateBuilding,
        CreativeDesktopWorldLayoutBuildingDuplicatePayload{
            buildingIndex, deltaXCells, deltaZCells});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(state.buildingManipulation.active ||
                       state.buildingTransform.active);
  if (ImGui::Button("Edit contents")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutClearSelection);
  }
  ImGui::EndDisabled();
  ImGui::Separator();
}

void drawBuildingTemplateActions(CreativeEditorWorldLayoutState& state,
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
    commands.push(
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
  }

  const bool linkedSourceAvailable =
      buildingAvailable && sync.provenance.valid &&
      sync.state !=
          cr::CreativeWorldLayoutBuildingTemplateSyncState::SourceMissing;
  const bool templateActionsBlocked =
      state.buildingTransform.active || state.buildingManipulation.active ||
      state.buildingTemplatePlacement.active;
  ImGui::BeginDisabled(!linkedSourceAvailable || templateActionsBlocked);
  if (ImGui::Button("Update template from selected")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                SelectedInstance});
  }
  const bool selectedNeedsRefresh =
      linkedSourceAvailable &&
      sync.state != cr::CreativeWorldLayoutBuildingTemplateSyncState::Current;
  ImGui::BeginDisabled(!selectedNeedsRefresh);
  if (ImGui::Button("Refresh selected")) {
    commands.push(
        CreativeDesktopCommandId::
            WorldLayoutRefreshBuildingTemplateInstances,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                SelectedInstance});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Refresh safe instances")) {
    commands.push(
        CreativeDesktopCommandId::
            WorldLayoutRefreshBuildingTemplateInstances,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                SafeInstances});
  }
  ImGui::SameLine();
  if (ImGui::Button("Force refresh all")) {
    commands.push(
        CreativeDesktopCommandId::
            WorldLayoutRefreshBuildingTemplateInstances,
        CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
            selectedBuilding,
            cr::CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll});
  }
  ImGui::EndDisabled();

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
        commands.push(
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

  if (!state.buildingTemplatePlacement.active) {
    ImGui::BeginDisabled(
        state.buildingTransform.active || state.buildingManipulation.active ||
        library.selectedIndex >= library.templates.size());
    if (ImGui::Button("Place template")) {
      if (state.tool != CreativeEditorWorldLayoutTool::Select) {
        commands.push(CreativeDesktopCommandId::WorldLayoutSetTool,
                      CreativeDesktopWorldLayoutToolPayload{
                          CreativeEditorWorldLayoutTool::Select});
      }
      commands.push(
          CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
          CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
              {0.0, 0.0},
              cr::CreativeWorldLayoutBuildingTransformOperation::
                  RotateRight90});
    }
    ImGui::EndDisabled();
  } else {
    const auto transform =
        [&](cr::CreativeWorldLayoutBuildingTransformOperation operation) {
          commands.push(
              CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
              CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
                  CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                      Transform,
                  {}, operation});
        };
    ImGui::TextColored(ImVec4{0.20F, 0.78F, 0.38F, 1.0F},
                       "Move over the canvas, then click to place");
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
      commands.push(
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
  const cr::CreativeObjectKind kind = state.source.boxes[boxIndex].kind;
  ImGui::TextUnformatted(boxSettingsTitle(kind));
  ImGui::SetNextItemWidth(84.0F);
  bool edited = ImGui::InputInt("X##layout_box", &originX, 1, 4);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Z##layout_box", &originZ, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Width##layout_box", &width, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Depth##layout_box", &depth, 1, 4) || edited;

  ImGui::SetNextItemWidth(92.0F);
  edited = ImGui::InputDouble(boxAnchorLabel(kind), &anchorLayer, 0.25, 1.0,
                              "%.2f") ||
           edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Layers##layout_box", &layerCount, 1, 2) || edited;

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

  const bool dirty = !sameBoxSettings(current, settings);
  ImGui::BeginDisabled(!dirty || !valuesRepresentable ||
                       state.boxManipulation.active);
  if (ImGui::Button(boxApplyLabel(kind))) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetBoxSettings,
                  CreativeDesktopWorldLayoutBoxSettingsPayload{boxIndex,
                                                               settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset floor")) {
    state.boxSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
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
  ImGui::TextUnformatted("Partition settings");
  ImGui::SetNextItemWidth(82.0F);
  bool edited = ImGui::InputInt("Start X##layout_wall", &startX, 1, 4);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  edited = ImGui::InputInt("Start Z##layout_wall", &startZ, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  edited = ImGui::InputInt("End X##layout_wall", &endX, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  edited = ImGui::InputInt("End Z##layout_wall", &endZ, 1, 4) || edited;

  ImGui::SetNextItemWidth(92.0F);
  edited = ImGui::InputDouble("Base##layout_wall", &baseLayer, 0.5, 1.0,
                              "%.2f") ||
           edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(92.0F);
  edited = ImGui::InputInt("Height##layout_wall", &height, 1, 2) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(110.0F);
  edited = ImGui::InputDouble("Thickness##layout_wall", &thicknessCells, 0.05,
                              0.25, "%.3f") ||
           edited;

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

  const bool dirty = !sameWallSettings(current, settings);
  ImGui::BeginDisabled(!dirty || !valuesRepresentable ||
                       state.wallManipulation.active);
  if (ImGui::Button("Apply partition")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetWallSettings,
                  CreativeDesktopWorldLayoutWallSettingsPayload{wallIndex,
                                                                settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset partition")) {
    state.wallSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
}

}  // namespace

void drawCreativeEditorWorldLayoutStructureInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  switch (state.selection.kind) {
    case CreativeEditorWorldLayoutSelectionKind::Building:
      drawBuildingActions(state, commands);
      drawBuildingTemplateActions(state, commands);
      break;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      drawCreativeEditorWorldLayoutVerticalConnectorInspector(state,
                                                              commands);
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
    case CreativeEditorWorldLayoutSelectionKind::Opening:
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
    case CreativeEditorWorldLayoutSelectionKind::Object:
      break;
  }
}

}  // namespace iggy3d_creative_app
