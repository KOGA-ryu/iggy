#pragma once

#include "EditorWorldLayoutPanel.hpp"

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutSelectionProperties(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutCreateTools(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    cr::CreativeGridSettings grid,
    CreativeDesktopCommandFrame& commands, bool unavailable);

// The Create tab's Building Blockout pattern choices, in display order. Pure
// so the one-to-one mapping between the four UI choices and the planner enum
// stays headless-testable.
struct CreativeEditorWorldLayoutBlockoutPatternChoice {
  const char* label = "";
  cr::CreativeWorldLayoutBuildingBlockoutPattern pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
};

[[nodiscard]] std::span<const CreativeEditorWorldLayoutBlockoutPatternChoice>
creativeEditorWorldLayoutBlockoutPatternChoices() noexcept;

// Whether the selected-building blockout Edit draft still reflects the
// source: active, pointing at a live building, read at the current revision.
// Pure so the drawer's sync line is headless-testable.
[[nodiscard]] bool creativeEditorWorldLayoutBlockoutEditInSync(
    const CreativeEditorDesktopBlockoutEditDraft& draft,
    const CreativeEditorWorldLayoutState& state) noexcept;

void drawCreativeEditorWorldLayoutToolboxStrip(
    CreativeEditorState& editor, CreativeDesktopCommandFrame& commands);

bool drawCreativeEditorWorldLayoutGlyphButton(
    const char* id, CreativeEditorToolGlyph glyph, float tileSize,
    bool active, std::string_view tooltip);

void drawCreativeEditorWorldLayoutTerrainTab(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands, bool unavailable);

void drawCreativeEditorWorldLayoutTerrainRegionProperties(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands, bool unavailable);

void drawCreativeEditorWorldLayoutTerrainRegionBuild(
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    CreativeDesktopCommandFrame& commands, bool unavailable);

void drawCreativeEditorWorldLayoutToolOptions(
    CreativeEditorState& editor, CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutStatusBar(
    const CreativeEditorState& editor,
    const CreativeEditorWorldLayoutCanvasHoverStatus& hover);

void cancelCreativeEditorWorldLayoutPanelManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutViewControls(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutTopographyState& topography,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutCanvas(
    CreativeEditorState& editor, const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& selection,
    const cr::CreativeMeasurementState& measurement,
    CreativeDesktopCommandFrame& commands, bool interactionEnabled,
    const CreativeEditorUiInputFrame& input,
    CreativeEditorWorldLayoutCanvasHoverStatus* hoverStatus);

}  // namespace iggy3d_creative_app
