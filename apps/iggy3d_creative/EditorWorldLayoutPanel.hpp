#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorState.hpp"
#include "EditorToolGlyphs.hpp"

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

// The drafting status bar under the canvas. The plan canvas captures one
// hover sample per frame; composing the bar's segments from editor state is
// pure so the wording stays headless-testable. Zoom reads relative to the
// canvas's default pixels-per-cell.
inline constexpr float kCreativeEditorWorldLayoutStatusZoomBaselinePixels =
    28.0F;

struct CreativeEditorWorldLayoutCanvasHoverStatus {
  bool present = false;
  double cellX = 0.0;
  double cellZ = 0.0;
  std::string_view semanticRole;
};

struct CreativeEditorWorldLayoutStatusLine {
  std::string cursor;
  std::string zoom;
  std::string snap;
  std::string cells;
  std::string message;
  CreativeEditorWorldLayoutTerrainRegionPhase phase =
      CreativeEditorWorldLayoutTerrainRegionPhase::Idle;
  bool regionMessage = false;
};

[[nodiscard]] CreativeEditorWorldLayoutStatusLine
composeCreativeEditorWorldLayoutStatusLine(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    const CreativeEditorWorldLayoutCanvasHoverStatus& hover);

// ImGui-only projection of EditorWorldLayout. It may update canvas pan/zoom,
// but every semantic source edit is emitted through the desktop dispatcher.
void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& document,
    bool playModeActive, CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
