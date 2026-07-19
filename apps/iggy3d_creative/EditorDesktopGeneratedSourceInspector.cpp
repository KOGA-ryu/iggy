#include "EditorDesktopWidgets.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorDesktopWorldLayoutInspector.hpp"

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

bool sameConnectorSettings(
    const CreativeEditorWorldLayoutVerticalConnectorSettings& lhs,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& rhs) noexcept {
  return lhs.footprint.minimum == rhs.footprint.minimum &&
         lhs.footprint.maximum == rhs.footprint.maximum &&
         lhs.kind == rhs.kind && lhs.direction == rhs.direction;
}

void appendGeneratedVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutVerticalConnectorSettings current;
  if (provenance.table != cr::CreativeWorldLayoutTable::VerticalConnector ||
      provenance.index >= worldLayout.source.verticalConnectors.size() ||
      !readCreativeEditorWorldLayoutVerticalConnectorSettings(
          worldLayout, provenance.index, current)) {
    return;
  }
  const bool draftChanged =
      !worldLayout.verticalConnectorSettingsDraft.active ||
      worldLayout.verticalConnectorSettingsDraft.connectorIndex !=
          provenance.index ||
      worldLayout.verticalConnectorSettingsDraft.sourceRevision !=
          worldLayout.revision;
  if (draftChanged) {
    if (worldLayout.verticalConnectorSettingsDraft.active &&
        creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.verticalConnectorSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  const cr::CreativeWorldLayoutVerticalConnector& connector =
      worldLayout.source.verticalConnectors[provenance.index];
  const char* lowerName =
      connector.lowerRoomIndex < worldLayout.source.rooms.size()
          ? worldLayout.source.rooms[connector.lowerRoomIndex].name.c_str()
          : "Unavailable";
  const char* upperName =
      connector.upperRoomIndex < worldLayout.source.rooms.size()
          ? worldLayout.source.rooms[connector.upperRoomIndex].name.c_str()
          : "Unavailable";
  ImGui::TextDisabled("%s -> %s", lowerName, upperName);

  CreativeEditorWorldLayoutVerticalConnectorSettings& settings =
      worldLayout.verticalConnectorSettingsDraft.settings;
  bool edited = false;
  ImGui::BeginDisabled(disabled);
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::BeginCombo(
          "Kind##generated_vertical_connector",
          creativeEditorWorldLayoutVerticalConnectorKindLabel(settings.kind))) {
    for (const cr::CreativeWorldLayoutVerticalConnectorKind kind :
         {cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp}) {
      const bool selected = settings.kind == kind;
      if (ImGui::Selectable(
              creativeEditorWorldLayoutVerticalConnectorKindLabel(kind),
              selected)) {
        settings.kind = kind;
        edited = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SetNextItemWidth(188.0F);
  if (ImGui::BeginCombo(
          "Direction##generated_vertical_connector",
          creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
              settings.direction))) {
    for (const cr::CreativeWorldLayoutVerticalDirection direction :
         {cr::CreativeWorldLayoutVerticalDirection::PositiveX,
          cr::CreativeWorldLayoutVerticalDirection::NegativeX,
          cr::CreativeWorldLayoutVerticalDirection::PositiveZ,
          cr::CreativeWorldLayoutVerticalDirection::NegativeZ}) {
      const bool selected = settings.direction == direction;
      if (ImGui::Selectable(
              creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
                  direction),
              selected)) {
        settings.direction = direction;
        edited = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::Button("Flip rise##generated_vertical_connector")) {
    settings.direction =
        oppositeCreativeEditorWorldLayoutVerticalConnectorDirection(
            settings.direction);
    edited = true;
  }

  std::array<int, 2U> origin = {settings.footprint.minimum.x,
                                settings.footprint.minimum.z};
  const std::int64_t width64 =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depth64 =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  std::array<int, 2U> size = {
      static_cast<int>(std::clamp<std::int64_t>(
          width64, std::numeric_limits<int>::min(),
          std::numeric_limits<int>::max())),
      static_cast<int>(std::clamp<std::int64_t>(
          depth64, std::numeric_limits<int>::min(),
          std::numeric_limits<int>::max()))};
  ImGui::SetNextItemWidth(188.0F);
  const bool originEdited = ImGui::InputInt2(
      "Origin X/Z##generated_vertical_connector", origin.data());
  ImGui::SetNextItemWidth(188.0F);
  const bool sizeEdited = ImGui::InputInt2(
      "Size W/D##generated_vertical_connector", size.data());
  edited = originEdited || sizeEdited || edited;
  ImGui::EndDisabled();

  const std::int64_t maximumX =
      static_cast<std::int64_t>(origin[0]) + size[0];
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(origin[1]) + size[1];
  const bool representable =
      maximumX >= std::numeric_limits<std::int32_t>::min() &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ >= std::numeric_limits<std::int32_t>::min() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() && size[0] > 0 &&
      size[1] > 0 &&
      settings.kind < cr::CreativeWorldLayoutVerticalConnectorKind::Count &&
      settings.direction < cr::CreativeWorldLayoutVerticalDirection::Count;
  if ((originEdited || sizeEdited) && representable) {
    settings.footprint.minimum = {
        static_cast<std::int32_t>(origin[0]),
        static_cast<std::int32_t>(origin[1])};
    settings.footprint.maximum = {
        static_cast<std::int32_t>(maximumX),
        static_cast<std::int32_t>(maximumZ)};
  }
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Connector footprint must be positive and in range");
  }
  if (edited) {
    if (representable) {
      commands.push(
          CreativeDesktopCommandId::
              WorldLayoutPreviewGeneratedVerticalConnectorSettings,
          CreativeDesktopGeneratedVerticalConnectorSettingsPayload{objectId,
                                                                    settings});
    } else if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  const bool dirty = !sameConnectorSettings(current, settings);
  ImGui::BeginDisabled(disabled || !dirty || !representable ||
                       worldLayout.verticalConnectorManipulation.active);
  if (ImGui::Button("Update connector in 3D")) {
    commands.push(
        CreativeDesktopCommandId::
            WorldLayoutApplyGeneratedVerticalConnectorSettings,
        CreativeDesktopGeneratedVerticalConnectorSettingsPayload{objectId,
                                                                  settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_vertical_connector")) {
    worldLayout.verticalConnectorSettingsDraft.settings = current;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

void appendGeneratedWallSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutWallSettings current;
  if (provenance.table != cr::CreativeWorldLayoutTable::Wall ||
      !readCreativeEditorWorldLayoutWallSettings(
          worldLayout, provenance.index, current)) {
    return;
  }
  const bool draftChanged =
      !worldLayout.wallSettingsDraft.active ||
      worldLayout.wallSettingsDraft.wallIndex != provenance.index ||
      worldLayout.wallSettingsDraft.sourceRevision != worldLayout.revision;
  if (draftChanged) {
    if (worldLayout.wallSettingsDraft.active &&
        creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.wallSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  CreativeEditorWorldLayoutWallSettings& settings =
      worldLayout.wallSettingsDraft.settings;
  double baseLayer = settings.baseLayer;
  int heightCells = settings.heightCells;
  double thicknessCells = settings.thicknessCells;
  ImGui::TextDisabled("From (%d, %d) to (%d, %d)", settings.start.x,
                      settings.start.z, settings.end.x, settings.end.z);
  ImGui::BeginDisabled(disabled);
  ImGui::SetNextItemWidth(112.0F);
  const bool baseEdited = ImGui::InputDouble(
      "Base layer##generated_wall", &baseLayer, 0.5, 1.0, "%.2f");
  ImGui::SetNextItemWidth(112.0F);
  const bool heightEdited = ImGui::InputInt(
      "Height##generated_wall", &heightCells, 1, 2);
  ImGui::SetNextItemWidth(112.0F);
  const bool thicknessEdited = ImGui::InputDouble(
      "Thickness##generated_wall", &thicknessCells, 0.05, 0.25, "%.3f");
  ImGui::EndDisabled();
  const bool representable =
      std::isfinite(baseLayer) && std::isfinite(thicknessCells) &&
      thicknessCells > 0.0 && heightCells > 0 && heightCells <= 65535;
  if ((baseEdited || heightEdited || thicknessEdited) && representable) {
    settings.baseLayer = baseLayer;
    settings.heightCells = static_cast<std::uint16_t>(heightCells);
    settings.thicknessCells = thicknessCells;
  }
  const bool edited = baseEdited || heightEdited || thicknessEdited;
  if (edited) {
    if (representable) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutPreviewGeneratedWallSettings,
          CreativeDesktopGeneratedWallSettingsPayload{objectId, settings});
    } else if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Height and thickness must be positive");
  }
  const bool dirty =
      current.start != settings.start || current.end != settings.end ||
      current.baseLayer != settings.baseLayer ||
      current.heightCells != settings.heightCells ||
      current.thicknessCells != settings.thicknessCells;
  ImGui::BeginDisabled(disabled || !dirty || !representable);
  if (ImGui::Button("Update partition in 3D")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings,
        CreativeDesktopGeneratedWallSettingsPayload{objectId, settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_wall")) {
    worldLayout.wallSettingsDraft.settings = current;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

void appendGeneratedOpeningSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutOpeningSettings current;
  if (provenance.table != cr::CreativeWorldLayoutTable::Opening ||
      provenance.index >= worldLayout.source.openings.size() ||
      !readCreativeEditorWorldLayoutOpeningSettings(
          worldLayout, provenance.index, current)) {
    return;
  }
  const bool draftChanged =
      !worldLayout.openingSettingsDraft.active ||
      worldLayout.openingSettingsDraft.openingIndex != provenance.index ||
      worldLayout.openingSettingsDraft.sourceRevision != worldLayout.revision;
  if (draftChanged) {
    if (worldLayout.openingSettingsDraft.active &&
        creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.openingSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  const cr::CreativeWorldLayoutOpening& opening =
      worldLayout.source.openings[provenance.index];
  CreativeEditorWorldLayoutOpeningSettings& settings =
      worldLayout.openingSettingsDraft.settings;
  const bool door = opening.kind == cr::CreativeBuildingOpeningKind::Door;
  ImGui::TextDisabled("%s source", door ? "Door" : "Window");
  ImGui::BeginDisabled(disabled);
  ImGui::SetNextItemWidth(112.0F);
  bool edited = ImGui::InputDouble("Offset##generated_opening",
                                   &settings.centerOffsetCells, 0.25, 1.0,
                                   "%.2f");
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Width##generated_opening",
                              &settings.widthCells, 0.25, 1.0, "%.2f") ||
           edited;
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Height##generated_opening",
                              &settings.heightCells, 0.25, 1.0, "%.2f") ||
           edited;
  if (door) {
    settings.sillHeightCells = 0.0;
    constexpr std::array<const char*, 5U> kPoseLabels = {
        "Closed", "Start hinge / side A", "Start hinge / side B",
        "End hinge / side A", "End hinge / side B"};
    int pose = static_cast<int>(settings.pose);
    ImGui::SetNextItemWidth(188.0F);
    if (ImGui::Combo("Pose##generated_opening", &pose, kPoseLabels.data(),
                     static_cast<int>(kPoseLabels.size()))) {
      settings.pose = static_cast<cr::CreativeBuildingOpeningPose>(pose);
      edited = true;
    }
  } else {
    settings.pose = cr::CreativeBuildingOpeningPose::Closed;
    ImGui::SetNextItemWidth(112.0F);
    edited = ImGui::InputDouble("Sill##generated_opening",
                                &settings.sillHeightCells, 0.25, 1.0,
                                "%.2f") ||
             edited;
  }
  edited = ImGui::Checkbox("Include insert##generated_opening",
                           &settings.includeInsert) ||
           edited;
  ImGui::EndDisabled();

  const bool representable =
      std::isfinite(settings.centerOffsetCells) &&
      std::isfinite(settings.widthCells) && settings.widthCells > 0.0 &&
      std::isfinite(settings.sillHeightCells) &&
      settings.sillHeightCells >= 0.0 &&
      std::isfinite(settings.heightCells) && settings.heightCells > 0.0;
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Opening dimensions must be finite and positive");
  }
  if (edited) {
    if (representable) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutPreviewGeneratedOpeningSettings,
          CreativeDesktopGeneratedOpeningSettingsPayload{objectId, settings});
    } else if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  const bool dirty =
      current.centerOffsetCells != settings.centerOffsetCells ||
      current.widthCells != settings.widthCells ||
      current.sillHeightCells != settings.sillHeightCells ||
      current.heightCells != settings.heightCells ||
      current.pose != settings.pose ||
      current.includeInsert != settings.includeInsert;
  ImGui::BeginDisabled(disabled || !dirty || !representable);
  if (ImGui::Button("Update opening in 3D")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings,
        CreativeDesktopGeneratedOpeningSettingsPayload{objectId, settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_opening")) {
    worldLayout.openingSettingsDraft.settings = current;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

}  // namespace

void appendCreativeDesktopGeneratedSourceSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    CreativeDesktopGeneratedSourceScopeCache& scopeCache,
    const cr::CreativeDocument& document,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  const CreativeDesktopGeneratedSourceScopeModel scopes =
      buildCreativeDesktopGeneratedSourceScopeModel(worldLayout.source,
                                                     provenance);
  if (scopes.count == 0U) {
    return;
  }
  const cr::CreativeWorldLayoutTable selectedTable =
      creativeEditorWorldLayoutSelectionTable(worldLayout.selection.kind);
  const std::size_t activeScope =
      resolveCreativeDesktopGeneratedSourceActiveScope(
          scopes, selectedTable, worldLayout.selection.index);

  ImGui::TextDisabled("Source scope");
  for (std::size_t scopeIndex = 0U; scopeIndex < scopes.count; ++scopeIndex) {
    const CreativeDesktopGeneratedSourceScopeEntry& entry =
        scopes.entries[scopeIndex];
    if (scopeIndex > 0U) {
      ImGui::SameLine();
      ImGui::TextDisabled(">");
      ImGui::SameLine();
    }
    ImGui::PushID(static_cast<int>(scopeIndex));
    if (scopeIndex == activeScope) {
      const CreativeDesktopGeneratedSourceScopeTint tint =
          creativeDesktopGeneratedSourceScopeTint(entry.table);
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4{tint.r * 0.42F, tint.g * 0.42F,
                                   tint.b * 0.42F, tint.a});
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4{tint.r * 0.60F, tint.g * 0.60F,
                                   tint.b * 0.60F, tint.a});
    }
    const std::string label =
        entry.name.empty() ? std::string(cr::toString(entry.table))
                           : std::string(entry.name);
    const bool selected = ImGui::SmallButton(label.c_str());
    if (scopeIndex == activeScope) {
      ImGui::PopStyleColor(2);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s source", cr::toString(entry.table).data());
    }
    if (selected && scopeIndex != activeScope) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
          CreativeDesktopWorldLayoutSourcePayload{
              entry.table, entry.index, std::string(entry.stableKey)});
    }
    ImGui::PopID();
  }

  const CreativeDesktopGeneratedSourceScopeEntry& scope =
      scopes.entries[activeScope];
  ImGui::BeginDisabled(disabled);
  if (ImGui::Button("Focus selected scope in 2D")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutFocusSource,
                  CreativeDesktopWorldLayoutSourcePayload{
                      scope.table, scope.index,
                      std::string(scope.stableKey)});
  }
  ImGui::EndDisabled();

  if (worldLayout.generatedRevision != worldLayout.revision) {
    ImGui::TextDisabled(
        "Generate pending 2D edits to refresh scope impact");
  } else {
    static_cast<void>(refreshCreativeDesktopGeneratedSourceScopeCache(
        scopeCache, document, worldLayout.source, worldLayout.sourceEpoch,
        worldLayout.revision, worldLayout.generatedRevision, scope.table,
        scope.index));
    const CreativeDesktopGeneratedSourceScopeSummary& summary =
        scopeCache.summary;
    if (summary.valid) {
      ImGui::TextDisabled("%zu generated objects (%zu visible, %zu hidden)",
                          summary.objectCount, summary.visibleObjectCount,
                          summary.hiddenObjectCount);
      if (summary.hasBounds) {
        const cr::CreativeVec3 size{
            summary.worldBounds.max.x - summary.worldBounds.min.x,
            summary.worldBounds.max.y - summary.worldBounds.min.y,
            summary.worldBounds.max.z - summary.worldBounds.min.z};
        ImGui::TextDisabled("Scope bounds %.2f x %.2f x %.2f m", size.x,
                            size.y, size.z);
      }
    } else {
      ImGui::TextDisabled("No generated objects in this scope");
    }
  }

  bool clearedDraft = false;
  if (scope.table != cr::CreativeWorldLayoutTable::Building &&
      worldLayout.generatedBuildingDraft.active) {
    worldLayout.generatedBuildingDraft = {};
    clearedDraft = true;
  }
  if (scope.table != cr::CreativeWorldLayoutTable::Room &&
      worldLayout.roomSettingsDraft.active) {
    worldLayout.roomSettingsDraft = {};
    clearedDraft = true;
  }
  if (scope.table != cr::CreativeWorldLayoutTable::Level &&
      worldLayout.generatedLevelSettingsDraft.active) {
    worldLayout.generatedLevelSettingsDraft = {};
    clearedDraft = true;
  }
  if (scope.table != cr::CreativeWorldLayoutTable::VerticalConnector &&
      worldLayout.verticalConnectorSettingsDraft.active) {
    worldLayout.verticalConnectorSettingsDraft = {};
    clearedDraft = true;
  }
  if (scope.table != cr::CreativeWorldLayoutTable::Wall &&
      worldLayout.wallSettingsDraft.active) {
    worldLayout.wallSettingsDraft = {};
    clearedDraft = true;
  }
  if (scope.table != cr::CreativeWorldLayoutTable::Opening &&
      worldLayout.openingSettingsDraft.active) {
    worldLayout.openingSettingsDraft = {};
    clearedDraft = true;
  }
  if (clearedDraft && creativeEditorWorldLayoutPreviewActive(worldLayout)) {
    commands.push(CreativeDesktopCommandId::
                      WorldLayoutCancelGeneratedSettingsPreview);
  }

  cr::CreativeWorldLayoutObjectProvenance scopedProvenance = provenance;
  scopedProvenance.table = scope.table;
  scopedProvenance.index = scope.index;
  if (scope.table != provenance.table || scope.index != provenance.index) {
    scopedProvenance.roomEdge = cr::CreativeWorldLayoutRoomEdge::Count;
    scopedProvenance.contributorCount = 1U;
  }
  switch (scope.table) {
    case cr::CreativeWorldLayoutTable::Building:
      appendCreativeDesktopGeneratedBuildingSettings(
          worldLayout, objectId, scope.index, disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::Level:
      appendCreativeDesktopGeneratedLevelSettings(
          worldLayout, objectId, scopedProvenance, disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::Room:
      appendCreativeDesktopGeneratedRoomSettings(
          worldLayout, objectId, scopedProvenance, disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      appendGeneratedVerticalConnectorSettings(
          worldLayout, objectId, scopedProvenance, disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      appendGeneratedWallSettings(worldLayout, objectId, scopedProvenance,
                                  disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::Opening:
      appendGeneratedOpeningSettings(worldLayout, objectId, scopedProvenance,
                                     disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::Box:
    case cr::CreativeWorldLayoutTable::Object:
      ImGui::TextDisabled(
          "One-to-one source; use the transform fields and Adopt 3D Edit");
      break;
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
    case cr::CreativeWorldLayoutTable::None:
    default:
      break;
  }
}

}  // namespace iggy3d_creative_app
