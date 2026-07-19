#pragma once

#include <cstdint>
#include <string_view>

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

// Display classification of the terrain-region workflow, derived from the
// authoritative region state (bools + the exact status messages the terrain
// state already produces). Pure and ImGui-free so the workflow presentation
// is headless-testable; AwaitingPreview is the transient valid-draft moment
// between selection release / parameter change and the dispatcher's preview
// result.
enum class CreativeEditorWorldLayoutTerrainRegionPhase : std::uint8_t {
  Idle,
  Selecting,
  AwaitingPreview,
  Ready,
  Rejected,
  Stale,
};

struct CreativeEditorWorldLayoutTerrainRegionMetrics {
  bool present = false;
  std::int32_t minimumX = 0;
  std::int32_t minimumZ = 0;
  std::uint16_t widthCells = 0U;
  std::uint16_t depthCells = 0U;
  std::uint32_t candidateCellCount = 0U;
};

[[nodiscard]] CreativeEditorWorldLayoutTerrainRegionPhase
classifyCreativeEditorWorldLayoutTerrainRegionPhase(
    const CreativeEditorWorldLayoutTerrainRegionState& region) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeEditorWorldLayoutTerrainRegionPhase phase) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutTerrainRegionMetrics
measureCreativeEditorWorldLayoutTerrainRegion(
    const CreativeEditorWorldLayoutTerrainRegionState& region) noexcept;

// ImGui-only projection of EditorWorldLayout. It may update canvas pan/zoom,
// but every semantic source edit is emitted through the desktop dispatcher.
void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& document,
    bool playModeActive, CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
