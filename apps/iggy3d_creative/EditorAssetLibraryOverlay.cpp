#include "EditorAssetLibrary.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

#include "EditorState.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] const cr::CreativeAuthoredAssetDefinition*
selectedDefinition(const CreativeEditorState& editor) noexcept {
  const CreativeEditorAssetLibraryState& state = editor.assetLibrary;
  if (state.selectedFilteredIndex >= state.filteredDefinitionIndices.size()) {
    return nullptr;
  }
  const std::size_t definitionIndex =
      state.filteredDefinitionIndices[state.selectedFilteredIndex];
  return definitionIndex < editor.authoredAssets.definitions.size()
             ? &editor.authoredAssets.definitions[definitionIndex]
             : nullptr;
}

[[nodiscard]] std::string_view statusText(std::string_view reasonCode) {
  if (reasonCode == "creative_authored_asset_edit_ready") {
    return {};
  }
  if (reasonCode == "creative_authored_asset_edit_file_action_blocked") {
    return "NEW AND LOAD ARE DISABLED WHILE EDITING AN ASSET";
  }
  if (reasonCode == "creative_authored_asset_recursive_definition") {
    return "SAVE BLOCKED: AN ASSET CANNOT CONTAIN ITSELF";
  }
  if (reasonCode == "creative_asset_library_ready") {
    return "READY";
  }
  if (reasonCode == "creative_asset_library_renamed") {
    return "NAME UPDATED";
  }
  if (reasonCode == "creative_asset_library_duplicated") {
    return "DUPLICATE CREATED";
  }
  if (reasonCode == "creative_asset_library_deleted") {
    return "ASSET DELETED";
  }
  if (reasonCode == "creative_asset_library_delete_confirm") {
    return "DELETE CONFIRMATION";
  }
  if (reasonCode == "creative_asset_library_delete_referenced") {
    return "DELETE BLOCKED: ASSET IS IN USE";
  }
  if (reasonCode == "creative_asset_library_label_invalid") {
    return "NAME MUST BE NONEMPTY AND UNIQUE";
  }
  if (reasonCode == "creative_asset_library_source_missing") {
    return "ASSET SOURCE IS MISSING";
  }
  return reasonCode;
}

void appendText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                std::string_view text, std::int32_t x, std::int32_t y,
                std::uint32_t width, std::uint32_t height, float r, float g,
                float b) {
  iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt(text, x, y, width, height);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = r;
    quad.g = g;
    quad.b = b;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

} // namespace

void appendCreativeEditorAssetLibraryOverlay(
    const CreativeEditorState& editor, std::uint32_t drawableWidth,
    std::uint32_t drawableHeight, std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  if (drawableWidth < 320U || drawableHeight < 120U) {
    return;
  }
  if (editor.assetLibrary.open) {
    const std::uint32_t panelWidth = std::min(720U, drawableWidth - 24U);
    const std::uint32_t panelHeight = std::min(440U, drawableHeight - 24U);
    const std::int32_t x =
        static_cast<std::int32_t>((drawableWidth - panelWidth) / 2U);
    const std::int32_t y =
        static_cast<std::int32_t>((drawableHeight - panelHeight) / 2U);
    uiRects.push_back(
        {0, 0, drawableWidth, drawableHeight, 0.01F, 0.015F, 0.02F, 0.72F});
    uiRects.push_back(
        {x, y, panelWidth, panelHeight, 0.05F, 0.06F, 0.07F, 0.98F});
    appendText(glyphs, "AUTHORED ASSET LIBRARY", x + 18, y + 16, drawableWidth,
               drawableHeight, 0.92F, 0.95F, 0.96F);
    const std::string search =
        editor.assetLibrary.mode == CreativeEditorAssetLibraryMode::Rename
            ? "NAME: " + editor.assetLibrary.renameDraft + "_"
            : "SEARCH: " + editor.assetLibrary.query + "_";
    appendText(glyphs, search, x + 18, y + 42, drawableWidth, drawableHeight,
               0.70F, 0.82F, 0.86F);
    const std::size_t rowCount = std::min<std::size_t>(
        8U, editor.assetLibrary.filteredDefinitionIndices.size());
    for (std::size_t row = 0U; row < rowCount; ++row) {
      const std::size_t filtered =
          (editor.assetLibrary.selectedFilteredIndex / 8U) * 8U + row;
      if (filtered >= editor.assetLibrary.filteredDefinitionIndices.size()) {
        break;
      }
      const std::size_t definitionIndex =
          editor.assetLibrary.filteredDefinitionIndices[filtered];
      if (definitionIndex >= editor.authoredAssets.definitions.size()) {
        continue;
      }
      const cr::CreativeAuthoredAssetDefinition& definition =
          editor.authoredAssets.definitions[definitionIndex];
      const bool selected =
          filtered == editor.assetLibrary.selectedFilteredIndex;
      const std::int32_t rowY = y + 76 + static_cast<std::int32_t>(row * 34U);
      uiRects.push_back({x + 16, rowY, panelWidth - 32U, 30U,
                         selected ? 0.84F : 0.08F, selected ? 0.74F : 0.09F,
                         selected ? 0.26F : 0.10F, 0.96F});
      appendText(glyphs, definition.label + "  [" + definition.assetId + "]",
                 x + 26, rowY + 8, drawableWidth, drawableHeight,
                 selected ? 0.05F : 0.86F, selected ? 0.06F : 0.90F,
                 selected ? 0.07F : 0.92F);
    }
    const cr::CreativeAuthoredAssetDefinition* selected =
        selectedDefinition(editor);
    const std::int32_t footerY =
        y + static_cast<std::int32_t>(panelHeight) - 92;
    if (selected != nullptr) {
      char facts[192];
      std::snprintf(
          facts, sizeof(facts),
          "%zu OBJECTS | %zu MAP INSTANCES | %zu ASSET REFERENCES",
          selected->content.objects.size(),
          editor.assetLibrary.selectedReferences.mapInstanceCount,
          editor.assetLibrary.selectedReferences.authoredAssetDependencyCount);
      appendText(glyphs, facts, x + 18, footerY, drawableWidth, drawableHeight,
                 0.66F, 0.73F, 0.77F);
    }
    const std::string action =
        editor.assetLibrary.mode ==
                CreativeEditorAssetLibraryMode::ConfirmDelete
            ? "CONFIRM DELETE"
            : std::string(toString(editor.assetLibrary.action));
    appendText(
        glyphs, action, x + 18, footerY + 28, drawableWidth, drawableHeight,
        editor.assetLibrary.action == CreativeEditorAssetLibraryAction::Delete
            ? 0.96F
            : 0.36F,
        editor.assetLibrary.action == CreativeEditorAssetLibraryAction::Delete
            ? 0.36F
            : 0.94F,
        0.42F);
    appendText(glyphs, statusText(editor.assetLibrary.statusLabel), x + 18,
               footerY + 54, drawableWidth, drawableHeight, 0.70F, 0.76F,
               0.80F);
    return;
  }

  if (!editor.assetEdit.active) {
    return;
  }
  if (editor.assetEdit.menuOpen) {
    const std::uint32_t panelWidth = std::min(420U, drawableWidth - 24U);
    const std::uint32_t panelHeight = std::min(218U, drawableHeight - 24U);
    const std::int32_t x =
        static_cast<std::int32_t>((drawableWidth - panelWidth) / 2U);
    const std::int32_t y =
        static_cast<std::int32_t>((drawableHeight - panelHeight) / 2U);
    uiRects.push_back(
        {0, 0, drawableWidth, drawableHeight, 0.01F, 0.015F, 0.02F, 0.68F});
    uiRects.push_back(
        {x, y, panelWidth, panelHeight, 0.05F, 0.06F, 0.07F, 0.98F});
    const std::string heading =
        std::string{"ASSET EDIT SESSION"} +
        (editor.assetEdit.dirty ? " | UNSAVED" : " | CURRENT");
    appendText(glyphs, heading, x + 18, y + 16, drawableWidth, drawableHeight,
               0.92F, 0.95F, 0.96F);
    appendText(glyphs, statusText(editor.assetEdit.statusLabel), x + 18, y + 40,
               drawableWidth, drawableHeight, 0.94F, 0.54F, 0.34F);
    for (std::size_t index = 0U;
         index <
         static_cast<std::size_t>(CreativeEditorAssetEditMenuAction::Count);
         ++index) {
      const auto action = static_cast<CreativeEditorAssetEditMenuAction>(index);
      const bool selected = editor.assetEdit.menuAction == action;
      const std::int32_t rowY = y + 68 + static_cast<std::int32_t>(index * 38U);
      uiRects.push_back({x + 14, rowY, panelWidth - 28U, 32U,
                         selected ? 0.84F : 0.08F, selected ? 0.74F : 0.09F,
                         selected ? 0.26F : 0.10F, 0.98F});
      appendText(glyphs, toString(action), x + 26, rowY + 9, drawableWidth,
                 drawableHeight, selected ? 0.05F : 0.86F,
                 selected ? 0.06F : 0.90F, selected ? 0.07F : 0.92F);
    }
    return;
  }

  const std::uint32_t width = std::min(620U, drawableWidth - 16U);
  const std::int32_t x =
      static_cast<std::int32_t>((drawableWidth - width) / 2U);
  const bool hasStatus = !statusText(editor.assetEdit.statusLabel).empty();
  uiRects.push_back(
      {x, 10, width, hasStatus ? 62U : 40U, 0.04F, 0.05F, 0.06F, 0.94F});
  const std::string title =
      "EDITING ASSET: " + editor.assetEdit.label +
      (editor.assetEdit.dirty ? " | UNSAVED" : " | CURRENT");
  appendText(glyphs, title, x + 14, 22, drawableWidth, drawableHeight,
             editor.assetEdit.dirty ? 0.98F : 0.42F,
             editor.assetEdit.dirty ? 0.76F : 0.94F, 0.34F);
  if (hasStatus) {
    appendText(glyphs, statusText(editor.assetEdit.statusLabel), x + 14, 44,
               drawableWidth, drawableHeight, 0.94F, 0.54F, 0.34F);
  }
}

} // namespace iggy3d_creative_app
