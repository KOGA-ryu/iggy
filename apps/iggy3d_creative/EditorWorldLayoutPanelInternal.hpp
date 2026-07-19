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

}  // namespace iggy3d_creative_app
