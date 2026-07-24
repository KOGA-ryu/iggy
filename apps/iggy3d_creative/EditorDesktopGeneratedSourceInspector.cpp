#include "EditorDesktopWidgets.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutPlan.hpp"
#include "EditorWorldLayoutOpenings.hpp"

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
         lhs.kind == rhs.kind && lhs.direction == rhs.direction &&
         lhs.material == rhs.material;
}

void appendGeneratedVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    const cr::CreativeDocument& document,
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
      commands.enqueue(CreativeDesktopCommandId::
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
  CreativeDesktopPropertyEditActivity editActivity;
  ImGui::BeginDisabled(
      disabled || worldLayout.verticalConnectorManipulation.active);
  ImGui::SetNextItemWidth(188.0F);
  bool kindEdited = false;
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
        kindEdited = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  observeCreativeDesktopDiscretePropertyEdit(editActivity, kindEdited);
  ImGui::SetNextItemWidth(188.0F);
  bool directionEdited = false;
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
        directionEdited = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  observeCreativeDesktopDiscretePropertyEdit(editActivity, directionEdited);
  const bool flipEdited =
      ImGui::Button("Flip rise##generated_vertical_connector");
  if (flipEdited) {
    settings.direction =
        oppositeCreativeEditorWorldLayoutVerticalConnectorDirection(
            settings.direction);
  }
  observeCreativeDesktopDiscretePropertyEdit(editActivity, flipEdited);
  ImGui::SetNextItemWidth(188.0F);
  const bool materialEdited =
      drawCreativeEditorWorldLayoutVerticalConnectorMaterial(
          "Material##generated_vertical_connector", settings.material);
  observeCreativeDesktopDiscretePropertyEdit(editActivity, materialEdited);

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
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, originEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(188.0F);
  const bool sizeEdited = ImGui::InputInt2(
      "Size W/D##generated_vertical_connector", size.data());
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, sizeEdited, ImGui::IsItemDeactivatedAfterEdit());
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
      settings.direction < cr::CreativeWorldLayoutVerticalDirection::Count &&
      settings.material < cr::CreativeStructuralMaterial::Count;
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
  if (representable) {
    drawCreativeEditorWorldLayoutVerticalConnectorPlan(
        planCreativeEditorWorldLayoutVerticalConnectorSettings(
            worldLayout, document, provenance.index, settings),
        settings.kind);
  }
  bool dirty = !sameConnectorSettings(current, settings);
  bool resetRequested = false;
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_vertical_connector")) {
    worldLayout.verticalConnectorSettingsDraft.settings = current;
    dirty = false;
    resetRequested = true;
  }
  ImGui::EndDisabled();

  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          editActivity, dirty,
          representable &&
              !worldLayout.verticalConnectorManipulation.active,
          creativeEditorWorldLayoutPreviewActive(worldLayout),
          resetRequested);
  queueCreativeDesktopGeneratedPropertyEdit(
      intent,
      CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedVerticalConnectorSettings,
      CreativeDesktopCommandId::
          WorldLayoutApplyGeneratedVerticalConnectorSettings,
      CreativeDesktopGeneratedVerticalConnectorSettingsPayload{
          objectId, worldLayout.verticalConnectorSettingsDraft.settings},
      commands);
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
      commands.enqueue(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.wallSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  CreativeEditorWorldLayoutWallSettings& settings =
      worldLayout.wallSettingsDraft.settings;
  CreativeDesktopPropertyEditActivity editActivity;
  double baseLayer = settings.baseLayer;
  int heightCells = settings.heightCells;
  double thicknessCells = settings.thicknessCells;
  ImGui::TextDisabled("From (%d, %d) to (%d, %d)", settings.start.x,
                      settings.start.z, settings.end.x, settings.end.z);
  ImGui::BeginDisabled(disabled);
  ImGui::SetNextItemWidth(112.0F);
  const bool baseEdited = ImGui::InputDouble(
      "Base layer##generated_wall", &baseLayer, 0.5, 1.0, "%.2f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, baseEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool heightEdited = ImGui::InputInt(
      "Height##generated_wall", &heightCells, 1, 2);
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, heightEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool thicknessEdited = ImGui::InputDouble(
      "Thickness##generated_wall", &thicknessCells, 0.05, 0.25, "%.3f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, thicknessEdited,
      ImGui::IsItemDeactivatedAfterEdit());
  ImGui::EndDisabled();
  const bool representable =
      std::isfinite(baseLayer) && std::isfinite(thicknessCells) &&
      thicknessCells > 0.0 && heightCells > 0 && heightCells <= 65535;
  if ((baseEdited || heightEdited || thicknessEdited) && representable) {
    settings.baseLayer = baseLayer;
    settings.heightCells = static_cast<std::uint16_t>(heightCells);
    settings.thicknessCells = thicknessCells;
  }
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Height and thickness must be positive");
  }
  bool dirty =
      current.start != settings.start || current.end != settings.end ||
      current.baseLayer != settings.baseLayer ||
      current.heightCells != settings.heightCells ||
      current.thicknessCells != settings.thicknessCells;
  bool resetRequested = false;
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_wall")) {
    worldLayout.wallSettingsDraft.settings = current;
    dirty = false;
    resetRequested = true;
  }
  ImGui::EndDisabled();

  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          editActivity, dirty, representable,
          creativeEditorWorldLayoutPreviewActive(worldLayout),
          resetRequested);
  queueCreativeDesktopGeneratedPropertyEdit(
      intent,
      CreativeDesktopCommandId::WorldLayoutPreviewGeneratedWallSettings,
      CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings,
      CreativeDesktopGeneratedWallSettingsPayload{
          objectId, worldLayout.wallSettingsDraft.settings},
      commands);
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
      commands.enqueue(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.openingSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  const cr::CreativeWorldLayoutOpening& opening =
      worldLayout.source.openings[provenance.index];
  CreativeEditorWorldLayoutOpeningSettings& settings =
      worldLayout.openingSettingsDraft.settings;
  CreativeDesktopPropertyEditActivity editActivity;
  const bool door = opening.kind == cr::CreativeBuildingOpeningKind::Door;
  ImGui::TextDisabled("%s source", door ? "Door" : "Window");
  ImGui::BeginDisabled(disabled);
  ImGui::SetNextItemWidth(112.0F);
  const bool offsetEdited = ImGui::InputDouble(
      "Offset##generated_opening", &settings.centerOffsetCells, 0.25, 1.0,
      "%.2f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, offsetEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool widthEdited = ImGui::InputDouble(
      "Width##generated_opening", &settings.widthCells, 0.25, 1.0,
      "%.2f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, widthEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool heightEdited = ImGui::InputDouble(
      "Height##generated_opening", &settings.heightCells, 0.25, 1.0,
      "%.2f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, heightEdited, ImGui::IsItemDeactivatedAfterEdit());
  if (door) {
    settings.sillHeightCells = 0.0;
    const CreativeDoorSettingsWidgetActivity doorActivity =
        drawCreativeDoorSettingsWidgets(settings.door,
                                        "generated_world_layout_opening");
    observeCreativeDesktopDiscretePropertyEdit(
        editActivity, doorActivity.discreteChanged);
    observeCreativeDesktopContinuousPropertyEdit(
        editActivity, doorActivity.continuousChanged,
        doorActivity.continuousDeactivated);
  } else {
    ImGui::SetNextItemWidth(112.0F);
    const bool sillEdited = ImGui::InputDouble(
        "Sill##generated_opening", &settings.sillHeightCells, 0.25, 1.0,
        "%.2f");
    observeCreativeDesktopContinuousPropertyEdit(
        editActivity, sillEdited, ImGui::IsItemDeactivatedAfterEdit());
    constexpr std::array<const char*, 2U> kWindowInsertLabels = {
        "Glazing", "Paired shutters"};
    int insertKind = static_cast<int>(settings.window.insertKind);
    ImGui::SetNextItemWidth(188.0F);
    const bool treatmentEdited = ImGui::Combo(
        "Treatment##generated_opening", &insertKind,
        kWindowInsertLabels.data(),
        static_cast<int>(kWindowInsertLabels.size()));
    if (treatmentEdited) {
      settings.window.insertKind =
          static_cast<cr::CreativeWindowInsertKind>(insertKind);
    }
    observeCreativeDesktopDiscretePropertyEdit(editActivity,
                                                treatmentEdited);
  }
  constexpr std::array<const char*, 2U> kFacingLabels = {
      "Side A (+ normal)", "Side B (- normal)"};
  int facing = static_cast<int>(settings.facing);
  ImGui::SetNextItemWidth(188.0F);
  const bool facingEdited = ImGui::Combo(
      "Front side##generated_opening", &facing, kFacingLabels.data(),
      static_cast<int>(kFacingLabels.size()));
  if (facingEdited) {
    settings.facing =
        static_cast<cr::CreativeBuildingOpeningFacing>(facing);
  }
  observeCreativeDesktopDiscretePropertyEdit(editActivity, facingEdited);
  const bool insertEdited = ImGui::Checkbox(
      "Include insert##generated_opening", &settings.includeInsert);
  observeCreativeDesktopDiscretePropertyEdit(editActivity, insertEdited);
  ImGui::EndDisabled();

  const bool representable =
      std::isfinite(settings.centerOffsetCells) &&
      std::isfinite(settings.widthCells) && settings.widthCells > 0.0 &&
      std::isfinite(settings.sillHeightCells) &&
      settings.sillHeightCells >= 0.0 &&
      std::isfinite(settings.heightCells) && settings.heightCells > 0.0 &&
      (!door || cr::isValidCreativeDoorSettings(settings.door)) &&
      (door || cr::isValidCreativeWindowSettings(settings.window)) &&
      settings.facing < cr::CreativeBuildingOpeningFacing::Count;
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Opening dimensions must be finite and positive");
  }
  bool dirty =
      current.centerOffsetCells != settings.centerOffsetCells ||
      current.widthCells != settings.widthCells ||
      current.sillHeightCells != settings.sillHeightCells ||
      current.heightCells != settings.heightCells ||
      !(current.door == settings.door) ||
      !(current.window == settings.window) ||
      current.facing != settings.facing ||
      current.includeInsert != settings.includeInsert;
  bool resetRequested = false;
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_opening")) {
    worldLayout.openingSettingsDraft.settings = current;
    dirty = false;
    resetRequested = true;
  }
  ImGui::EndDisabled();

  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          editActivity, dirty, representable,
          creativeEditorWorldLayoutPreviewActive(worldLayout),
          resetRequested);
  queueCreativeDesktopGeneratedPropertyEdit(
      intent,
      CreativeDesktopCommandId::WorldLayoutPreviewGeneratedOpeningSettings,
      CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings,
      CreativeDesktopGeneratedOpeningSettingsPayload{
          objectId, worldLayout.openingSettingsDraft.settings},
      commands);
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
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
          CreativeDesktopWorldLayoutSourcePayload{
              entry.table, entry.index, std::string(entry.stableKey)});
    }
    ImGui::PopID();
  }

  const CreativeDesktopGeneratedSourceScopeEntry& scope =
      scopes.entries[activeScope];
  const bool sourceSynchronized =
      worldLayout.generatedRevision == worldLayout.revision;
  const CreativeDesktopGeneratedSourceScopeSummary* summary = nullptr;
  if (sourceSynchronized) {
    static_cast<void>(refreshCreativeDesktopGeneratedSourceScopeCache(
        scopeCache, document, worldLayout.source, worldLayout.sourceEpoch,
        worldLayout.revision, worldLayout.generatedRevision, scope.table,
        scope.index));
    summary = &scopeCache.summary;
  }

  ImGui::BeginDisabled(disabled);
  if (ImGui::Button("Focus in 2D")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutFocusSource,
                  CreativeDesktopWorldLayoutSourcePayload{
                      scope.table, scope.index,
                      std::string(scope.stableKey)});
  }
  ImGui::EndDisabled();
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Center this source on the 2D layout canvas");
  }
  ImGui::SameLine();
  const bool frameUnavailable =
      summary == nullptr || !summary->valid || !summary->hasBounds;
  ImGui::BeginDisabled(disabled || frameUnavailable);
  if (ImGui::Button("Frame in 3D")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D,
                  CreativeDesktopWorldLayoutSourcePayload{
                      scope.table, scope.index,
                      std::string(scope.stableKey)});
  }
  ImGui::EndDisabled();
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Frame every generated object in this source scope");
  }

  if (!sourceSynchronized) {
    ImGui::TextDisabled(
        "Generate pending 2D edits to refresh scope impact");
  } else if (summary != nullptr) {
    if (summary->valid) {
      ImGui::TextDisabled("%zu generated objects (%zu visible, %zu hidden)",
                          summary->objectCount, summary->visibleObjectCount,
                          summary->hiddenObjectCount);
      if (summary->hasBounds) {
        const cr::CreativeVec3 size{
            summary->worldBounds.max.x - summary->worldBounds.min.x,
            summary->worldBounds.max.y - summary->worldBounds.min.y,
            summary->worldBounds.max.z - summary->worldBounds.min.z};
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
  if (scope.table != cr::CreativeWorldLayoutTable::TopologyEdge &&
      worldLayout.roomEdgeSettingsDraft.topologyEdgeIndex !=
          cr::kInvalidCreativeWorldLayoutIndex) {
    worldLayout.roomEdgeSettingsDraft = {};
    worldLayout.selectedRoomTopologyEdgeStableKey.clear();
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
  if (scope.table != cr::CreativeWorldLayoutTable::RoofAperture &&
      worldLayout.roofApertureSettingsDraft.active) {
    worldLayout.roofApertureSettingsDraft = {};
    clearedDraft = true;
  }
  if (clearedDraft && creativeEditorWorldLayoutPreviewActive(worldLayout)) {
    commands.enqueue(CreativeDesktopCommandId::
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
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      ImGui::BeginDisabled(disabled);
      appendCreativeDesktopTopologyEdgeSettings(worldLayout, scope.index,
                                                commands);
      ImGui::EndDisabled();
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      appendGeneratedVerticalConnectorSettings(
          worldLayout, document, objectId, scopedProvenance, disabled,
          commands);
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      appendGeneratedWallSettings(worldLayout, objectId, scopedProvenance,
                                  disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::Opening:
      appendGeneratedOpeningSettings(worldLayout, objectId, scopedProvenance,
                                     disabled, commands);
      break;
    case cr::CreativeWorldLayoutTable::RoofAperture:
      ImGui::BeginDisabled(disabled);
      drawCreativeEditorWorldLayoutRoofApertureInspector(
          worldLayout, scope.index, commands);
      ImGui::EndDisabled();
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
