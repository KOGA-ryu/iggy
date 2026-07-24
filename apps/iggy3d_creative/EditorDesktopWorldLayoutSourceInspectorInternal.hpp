#pragma once

#include "EditorDesktopWidgets.hpp"
#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutPlan.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/play/NpcSpawn.hpp"
#include "app/iggy3d/creative/play/PlayerSpawn.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>

#include "imgui.h"

namespace iggy3d_creative_app::detail {

bool inputText(const char* label, std::string& value);

template <typename Enum, std::size_t Size, typename Label>
bool enumCombo(const char* id, Enum& value,
               const std::array<Enum, Size>& choices, Label label) {
  bool changed = false;
  const std::string preview(label(value));
  if (ImGui::BeginCombo(id, preview.c_str())) {
    for (const Enum choice : choices) {
      const bool selected = choice == value;
      const std::string choiceLabel(label(choice));
      if (ImGui::Selectable(choiceLabel.c_str(), selected)) {
        value = choice;
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

template <std::size_t Size>
bool u16Combo(const char* id, std::uint16_t& value,
              const std::array<std::uint16_t, Size>& choices,
              const char* suffix = "") {
  const auto label = [suffix](std::uint16_t choice) {
    return std::to_string(choice) + suffix;
  };
  return enumCombo(id, value, choices, label);
}

void drawStableKey(std::string_view stableKey, std::string_view type);
void drawRetainingEdgeSettings(
    std::string_view profileKey,
    cr::CreativeTerrainLandformRecipe& landform,
    bool& enabled,
    cr::CreativeRetainingEdgeSourceRecipe& source,
    CreativeDesktopPropertyEditActivity& activity);
void drawTerrainImpactSummary(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    cr::CreativeWorldLayoutTable table,
    std::size_t index,
    std::string_view stableKey,
    CreativeDesktopCommandFrame& commands);
void drawVec3Table(const char* id, const char* firstLabel,
                   cr::CreativeVec3& first,
                   CreativeDesktopPropertyEditActivity& activity,
                   const char* secondLabel = nullptr,
                   cr::CreativeVec3* second = nullptr);

void drawLevelInspector(CreativeEditorWorldLayoutState& state,
                        CreativeDesktopCommandFrame& commands);
void drawRoofApertureInspectorForIndex(
    CreativeEditorWorldLayoutState& state,
    std::size_t apertureIndex,
    CreativeDesktopCommandFrame& commands);
void drawTerrainProfileInspector(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document,
                                 CreativeDesktopCommandFrame& commands);
void drawTerrainPathInspector(CreativeEditorWorldLayoutState& state,
                              const cr::CreativeDocument& document,
                              CreativeDesktopCommandFrame& commands);
void drawObjectInspector(CreativeEditorWorldLayoutState& state,
                         CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app::detail
