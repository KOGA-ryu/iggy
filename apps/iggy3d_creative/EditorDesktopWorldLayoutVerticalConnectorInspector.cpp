#include "EditorDesktopWorldLayoutInspector.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

bool sameSettings(
    const CreativeEditorWorldLayoutVerticalConnectorSettings& lhs,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& rhs) noexcept {
  return lhs.footprint.minimum == rhs.footprint.minimum &&
         lhs.footprint.maximum == rhs.footprint.maximum &&
         lhs.kind == rhs.kind && lhs.direction == rhs.direction;
}

const char* kindLabel(
    cr::CreativeWorldLayoutVerticalConnectorKind kind) noexcept {
  switch (kind) {
    case cr::CreativeWorldLayoutVerticalConnectorKind::Stair:
      return "Stair";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Ramp:
      return "Ramp";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Count:
      break;
  }
  return "Invalid";
}

const char* directionLabel(
    cr::CreativeWorldLayoutVerticalDirection direction) noexcept {
  switch (direction) {
    case cr::CreativeWorldLayoutVerticalDirection::PositiveX:
      return "Rise toward +X";
    case cr::CreativeWorldLayoutVerticalDirection::NegativeX:
      return "Rise toward -X";
    case cr::CreativeWorldLayoutVerticalDirection::PositiveZ:
      return "Rise toward +Z";
    case cr::CreativeWorldLayoutVerticalDirection::NegativeZ:
      return "Rise toward -Z";
    case cr::CreativeWorldLayoutVerticalDirection::Count:
      break;
  }
  return "Invalid";
}

cr::CreativeWorldLayoutVerticalDirection oppositeDirection(
    cr::CreativeWorldLayoutVerticalDirection direction) noexcept {
  switch (direction) {
    case cr::CreativeWorldLayoutVerticalDirection::PositiveX:
      return cr::CreativeWorldLayoutVerticalDirection::NegativeX;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeX:
      return cr::CreativeWorldLayoutVerticalDirection::PositiveX;
    case cr::CreativeWorldLayoutVerticalDirection::PositiveZ:
      return cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeZ:
      return cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
    case cr::CreativeWorldLayoutVerticalDirection::Count:
      break;
  }
  return direction;
}

}  // namespace

void drawCreativeEditorWorldLayoutVerticalConnectorInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::VerticalConnector ||
      state.selection.index >= state.source.verticalConnectors.size()) {
    state.verticalConnectorSettingsDraft = {};
    return;
  }
  const std::size_t connectorIndex = state.selection.index;
  CreativeEditorWorldLayoutVerticalConnectorSettings current;
  if (!readCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, connectorIndex, current)) {
    state.verticalConnectorSettingsDraft = {};
    return;
  }
  if (!state.verticalConnectorSettingsDraft.active ||
      state.verticalConnectorSettingsDraft.connectorIndex != connectorIndex ||
      state.verticalConnectorSettingsDraft.sourceRevision != state.revision) {
    state.verticalConnectorSettingsDraft = {
        true, connectorIndex, state.revision, current};
  }

  CreativeEditorWorldLayoutVerticalConnectorSettings& settings =
      state.verticalConnectorSettingsDraft.settings;
  const cr::CreativeWorldLayoutVerticalConnector& connector =
      state.source.verticalConnectors[connectorIndex];
  const char* lowerName =
      connector.lowerRoomIndex < state.source.rooms.size()
          ? state.source.rooms[connector.lowerRoomIndex].name.c_str()
          : "Unavailable";
  const char* upperName =
      connector.upperRoomIndex < state.source.rooms.size()
          ? state.source.rooms[connector.upperRoomIndex].name.c_str()
          : "Unavailable";
  ImGui::TextUnformatted("Vertical connector settings");
  ImGui::TextDisabled("%s -> %s", lowerName, upperName);

  if (ImGui::BeginCombo("Kind##layout_connector", kindLabel(settings.kind))) {
    for (const cr::CreativeWorldLayoutVerticalConnectorKind kind :
         {cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp}) {
      const bool selected = settings.kind == kind;
      if (ImGui::Selectable(kindLabel(kind), selected)) {
        settings.kind = kind;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::BeginCombo("Direction##layout_connector",
                        directionLabel(settings.direction))) {
    for (const cr::CreativeWorldLayoutVerticalDirection direction :
         {cr::CreativeWorldLayoutVerticalDirection::PositiveX,
          cr::CreativeWorldLayoutVerticalDirection::NegativeX,
          cr::CreativeWorldLayoutVerticalDirection::PositiveZ,
          cr::CreativeWorldLayoutVerticalDirection::NegativeZ}) {
      const bool selected = settings.direction == direction;
      if (ImGui::Selectable(directionLabel(direction), selected)) {
        settings.direction = direction;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  if (ImGui::Button("Flip rise")) {
    settings.direction = oppositeDirection(settings.direction);
  }

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
  ImGui::SetNextItemWidth(84.0F);
  bool edited = ImGui::InputInt("X##layout_connector", &originX, 1, 4);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Z##layout_connector", &originZ, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited =
      ImGui::InputInt("Width##layout_connector", &width, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited =
      ImGui::InputInt("Depth##layout_connector", &depth, 1, 4) || edited;

  const std::int64_t maximumX =
      static_cast<std::int64_t>(originX) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(originZ) + depth;
  const bool valuesRepresentable =
      maximumX >= std::numeric_limits<std::int32_t>::min() &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ >= std::numeric_limits<std::int32_t>::min() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() && width > 0 &&
      depth > 0 &&
      settings.kind < cr::CreativeWorldLayoutVerticalConnectorKind::Count &&
      settings.direction < cr::CreativeWorldLayoutVerticalDirection::Count;
  if (edited && valuesRepresentable) {
    settings.footprint.minimum = {static_cast<std::int32_t>(originX),
                                  static_cast<std::int32_t>(originZ)};
    settings.footprint.maximum = {static_cast<std::int32_t>(maximumX),
                                  static_cast<std::int32_t>(maximumZ)};
  }
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "connector footprint must be positive and in range");
  }

  const bool dirty = !sameSettings(current, settings);
  ImGui::BeginDisabled(!dirty || !valuesRepresentable ||
                       state.verticalConnectorManipulation.active);
  if (ImGui::Button("Apply connector")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetVerticalConnectorSettings,
        CreativeDesktopWorldLayoutVerticalConnectorSettingsPayload{
            connectorIndex, settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset connector")) {
    state.verticalConnectorSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
