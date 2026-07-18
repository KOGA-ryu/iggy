#include "EditorDesktopWidgets.hpp"

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

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
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  if (provenance.table == cr::CreativeWorldLayoutTable::Wall) {
    if (worldLayout.openingSettingsDraft.active) {
      worldLayout.openingSettingsDraft = {};
      if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
        commands.push(CreativeDesktopCommandId::
                          WorldLayoutCancelGeneratedSettingsPreview);
      }
    }
    appendGeneratedWallSettings(worldLayout, objectId, provenance, disabled,
                                commands);
  } else if (provenance.table == cr::CreativeWorldLayoutTable::Opening) {
    if (worldLayout.wallSettingsDraft.active) {
      worldLayout.wallSettingsDraft = {};
      if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
        commands.push(CreativeDesktopCommandId::
                          WorldLayoutCancelGeneratedSettingsPreview);
      }
    }
    appendGeneratedOpeningSettings(worldLayout, objectId, provenance, disabled,
                                   commands);
  }
}

}  // namespace iggy3d_creative_app
