#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorDesktopWidgets.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

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

const char* creativeEditorWorldLayoutVerticalConnectorKindLabel(
    cr::CreativeWorldLayoutVerticalConnectorKind kind) noexcept {
  return kindLabel(kind);
}

const char* creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
    cr::CreativeWorldLayoutVerticalDirection direction) noexcept {
  return directionLabel(direction);
}

cr::CreativeWorldLayoutVerticalDirection
oppositeCreativeEditorWorldLayoutVerticalConnectorDirection(
    cr::CreativeWorldLayoutVerticalDirection direction) noexcept {
  return oppositeDirection(direction);
}

bool drawCreativeEditorWorldLayoutVerticalConnectorMaterial(
    const char* label, cr::CreativeStructuralMaterial& material) {
  bool changed = false;
  if (ImGui::BeginCombo(label, cr::toString(material).data())) {
    for (const cr::CreativeStructuralMaterial candidate :
         {cr::CreativeStructuralMaterial::Blockout,
          cr::CreativeStructuralMaterial::Plaster,
          cr::CreativeStructuralMaterial::Timber,
          cr::CreativeStructuralMaterial::Stone,
          cr::CreativeStructuralMaterial::Brick}) {
      const bool selected = material == candidate;
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

cr::CreativeWorldLayoutVerticalConnectorPlan
planCreativeEditorWorldLayoutVerticalConnectorSettings(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t connectorIndex,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& settings) {
  if (connectorIndex >= state.source.verticalConnectors.size()) {
    return {};
  }
  cr::CreativeWorldLayoutVerticalConnector candidate =
      state.source.verticalConnectors[connectorIndex];
  candidate.footprint = settings.footprint;
  candidate.kind = settings.kind;
  candidate.direction = settings.direction;
  candidate.material = settings.material;
  return cr::planCreativeWorldLayoutVerticalConnector(
      document.gridSettings(), state.source, connectorIndex, candidate);
}

void drawCreativeEditorWorldLayoutVerticalConnectorPlan(
    const cr::CreativeWorldLayoutVerticalConnectorPlan& plan,
    cr::CreativeWorldLayoutVerticalConnectorKind kind) {
  if (kind != cr::CreativeWorldLayoutVerticalConnectorKind::Ramp) {
    return;
  }
  const cr::CreativeRampRecipeResult& ramp = plan.ramp;
  if (std::isfinite(ramp.slopeAngleDegrees) &&
      ramp.slopeAngleDegrees > 0.0) {
    const bool walkable = plan.accepted && ramp.walkable;
    ImGui::TextColored(
        walkable ? ImVec4{0.31F, 0.82F, 0.43F, 1.0F}
                 : ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
        "Slope %.1f deg / %.1f deg max | %s", ramp.slopeAngleDegrees,
        ramp.maximumWalkableSlopeDegrees,
        walkable ? "Walkable" : "Blocked");
    ImGui::TextDisabled("Width %.2f m | Rise %.2f m | Run %.2f m",
                        ramp.widthMeters, ramp.riseMeters, ramp.runMeters);
    ImGui::TextDisabled("Headroom %.2f m / %.2f m required",
                        ramp.headroomMeters,
                        cr::kCreativeRampMinimumHeadroomMeters);
    if (walkable) {
      ImGui::TextDisabled("2 landings | 2 side edges | 2 edge sockets");
    }
    return;
  }
  ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                     "Ramp geometry is not currently usable");
}

void drawCreativeEditorWorldLayoutVerticalConnectorInspector(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
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
  CreativeDesktopPropertyEditActivity activity;
  bool discreteChanged = false;

  ImGui::BeginDisabled(state.verticalConnectorManipulation.active);
  if (ImGui::BeginCombo("Kind##layout_connector", kindLabel(settings.kind))) {
    for (const cr::CreativeWorldLayoutVerticalConnectorKind kind :
         {cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp}) {
      const bool selected = settings.kind == kind;
      if (ImGui::Selectable(kindLabel(kind), selected)) {
        settings.kind = kind;
        discreteChanged = true;
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
        discreteChanged = true;
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
    discreteChanged = true;
  }
  ImGui::SetNextItemWidth(150.0F);
  discreteChanged =
      drawCreativeEditorWorldLayoutVerticalConnectorMaterial(
          "Material##layout_connector", settings.material) ||
      discreteChanged;
  observeCreativeDesktopDiscretePropertyWidget(activity, discreteChanged);

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
  bool edited = false;
  const auto observe = [&](bool changed) {
    edited = edited || changed;
    observeCreativeDesktopContinuousPropertyWidget(activity, changed);
  };
  observe(ImGui::InputInt("X##layout_connector", &originX, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Z##layout_connector", &originZ, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Width##layout_connector", &width, 1, 4));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  observe(ImGui::InputInt("Depth##layout_connector", &depth, 1, 4));
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
      depth > 0 &&
      settings.kind < cr::CreativeWorldLayoutVerticalConnectorKind::Count &&
      settings.direction < cr::CreativeWorldLayoutVerticalDirection::Count &&
      settings.material < cr::CreativeStructuralMaterial::Count;
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
  if (valuesRepresentable) {
    drawCreativeEditorWorldLayoutVerticalConnectorPlan(
        planCreativeEditorWorldLayoutVerticalConnectorSettings(
            state, document, connectorIndex, settings),
        settings.kind);
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, settings, valuesRepresentable, "Reset connector",
      state, connectorIndex, connector.stableKey, commands);
}

}  // namespace iggy3d_creative_app
