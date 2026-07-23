#include "EditorDesktopWidgets.hpp"
#include "EditorWorldLayout.hpp"

#include "EditorDesktopWorldLayoutInspector.hpp"

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

bool sameRoomSettings(
    const CreativeEditorWorldLayoutRoomSettings& lhs,
    const CreativeEditorWorldLayoutRoomSettings& rhs) noexcept {
  return lhs.footprint.minimum == rhs.footprint.minimum &&
         lhs.footprint.maximum == rhs.footprint.maximum &&
         lhs.wallThicknessCells == rhs.wallThicknessCells;
}

}  // namespace

void appendCreativeDesktopGeneratedRoomSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutRoomSettings current;
  if (provenance.table != cr::CreativeWorldLayoutTable::Room ||
      provenance.index >= worldLayout.source.rooms.size() ||
      !readCreativeEditorWorldLayoutRoomSettings(
          worldLayout, provenance.index, current)) {
    return;
  }
  const cr::CreativeWorldLayoutRoom& room =
      worldLayout.source.rooms[provenance.index];
  if (room.levelIndex >= worldLayout.source.levels.size()) {
    return;
  }
  const bool draftChanged =
      !worldLayout.roomSettingsDraft.active ||
      worldLayout.roomSettingsDraft.roomIndex != provenance.index ||
      worldLayout.roomSettingsDraft.sourceRevision != worldLayout.revision;
  if (draftChanged) {
    if (worldLayout.roomSettingsDraft.active &&
        creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.roomSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  const cr::CreativeWorldLayoutLevel& level =
      worldLayout.source.levels[room.levelIndex];
  ImGui::TextDisabled("%s  |  %s", room.name.c_str(), level.name.c_str());
  if (provenance.roomEdge < cr::CreativeWorldLayoutRoomEdge::Count) {
    const std::string edgeLabel(cr::toString(provenance.roomEdge));
    if (provenance.contributorCount > 1U) {
      ImGui::TextColored({0.94F, 0.72F, 0.28F, 1.0F},
                         "Shared %s edge; editing %s", edgeLabel.c_str(),
                         room.name.c_str());
    } else {
      ImGui::TextDisabled("%s shell fragment", edgeLabel.c_str());
    }
  }

  CreativeEditorWorldLayoutRoomSettings& settings =
      worldLayout.roomSettingsDraft.settings;
  CreativeDesktopPropertyEditActivity editActivity;
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
  const bool roomEditActive = worldLayout.roomManipulation.active ||
                              worldLayout.roomCornerManipulation.active ||
                              worldLayout.roomBoundaryManipulation.active;
  const bool explicitTopology = !worldLayout.source.roomBoundaries.empty();

  ImGui::BeginDisabled(disabled || roomEditActive);
  ImGui::SeparatorText("Room footprint");
  ImGui::BeginDisabled(explicitTopology);
  ImGui::SetNextItemWidth(188.0F);
  const bool originEdited =
      ImGui::InputInt2("Origin X/Z##generated_room", origin.data());
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, originEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(188.0F);
  const bool sizeEdited =
      ImGui::InputInt2("Size W/D##generated_room", size.data());
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, sizeEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool wallThicknessEdited = ImGui::InputDouble(
      "Wall thickness##generated_room", &settings.wallThicknessCells, 0.05,
      0.25, "%.3f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, wallThicknessEdited,
      ImGui::IsItemDeactivatedAfterEdit());
  ImGui::EndDisabled();
  if (explicitTopology) {
    ImGui::TextDisabled("Shape and thickness use floor-plan boundary controls");
  }

  ImGui::EndDisabled();

  const std::int64_t maximumX =
      static_cast<std::int64_t>(origin[0]) + size[0];
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(origin[1]) + size[1];
  const bool footprintRepresentable =
      maximumX >= std::numeric_limits<std::int32_t>::min() &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ >= std::numeric_limits<std::int32_t>::min() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() && size[0] > 0 &&
      size[1] > 0;
  if ((originEdited || sizeEdited) && footprintRepresentable) {
    settings.footprint = {
        {static_cast<std::int32_t>(origin[0]),
         static_cast<std::int32_t>(origin[1])},
        {static_cast<std::int32_t>(maximumX),
         static_cast<std::int32_t>(maximumZ)}};
  }
  const bool representable =
      footprintRepresentable &&
      std::isfinite(settings.wallThicknessCells) &&
      settings.wallThicknessCells > 0.0 &&
      static_cast<double>(size[0]) > settings.wallThicknessCells * 2.0 &&
      static_cast<double>(size[1]) > settings.wallThicknessCells * 2.0;
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Room shell settings are outside valid bounds");
  }
  bool dirty = !sameRoomSettings(current, settings);
  bool resetRequested = false;
  ImGui::BeginDisabled(disabled || (!dirty && representable));
  if (ImGui::Button("Reset##generated_room")) {
    worldLayout.roomSettingsDraft.settings = current;
    dirty = false;
    resetRequested = true;
  }
  ImGui::EndDisabled();

  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          editActivity, dirty,
          representable && !roomEditActive,
          creativeEditorWorldLayoutPreviewActive(worldLayout),
          resetRequested);
  queueCreativeDesktopGeneratedPropertyEdit(
      intent,
      CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings,
      CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
      CreativeDesktopGeneratedRoomSettingsPayload{
          objectId, provenance.index, room.stableKey,
          worldLayout.roomSettingsDraft.settings},
      commands);
}

}  // namespace iggy3d_creative_app
