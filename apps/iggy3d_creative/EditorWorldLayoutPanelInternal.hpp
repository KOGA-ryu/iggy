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
    CreativeDesktopCommandFrame& commands, bool unavailable);

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

}  // namespace iggy3d_creative_app
