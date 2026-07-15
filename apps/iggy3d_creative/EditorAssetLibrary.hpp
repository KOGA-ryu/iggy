#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "EditorAuthoredAssets.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;

} // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorState;

inline constexpr std::size_t kCreativeEditorAssetLabelCapacity = 64U;

enum class CreativeEditorAssetLibraryAction : std::uint8_t {
  Edit,
  Rename,
  Duplicate,
  Delete,
  Count,
};

enum class CreativeEditorAssetLibraryMode : std::uint8_t {
  Browse,
  Rename,
  ConfirmDelete,
};

enum class CreativeEditorAssetEditMenuAction : std::uint8_t {
  ContinueEditing,
  SaveAndExit,
  DiscardAndExit,
  Count,
};

struct CreativeEditorAuthoredAssetReferenceSummary {
  std::size_t mapInstanceCount = 0U;
  std::size_t authoredAssetDependencyCount = 0U;

  [[nodiscard]] std::size_t total() const noexcept {
    return mapInstanceCount + authoredAssetDependencyCount;
  }
};

struct CreativeEditorAuthoredAssetMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool durableWriteOk = false;
  std::string assetId;
  std::string label;
  CreativeEditorAuthoredAssetReferenceSummary references;
  std::string reasonCode = "creative_asset_library_not_requested";
};

struct CreativeEditorAssetLibraryState {
  bool open = false;
  CreativeEditorAssetLibraryMode mode = CreativeEditorAssetLibraryMode::Browse;
  CreativeEditorAssetLibraryAction action =
      CreativeEditorAssetLibraryAction::Edit;
  std::string query;
  std::vector<std::size_t> filteredDefinitionIndices;
  std::size_t selectedFilteredIndex = 0U;
  std::string renameDraft;
  CreativeEditorAuthoredAssetReferenceSummary selectedReferences;
  std::string statusLabel;
};

struct CreativeEditorAssetLibraryFrameRequest {
  iggy3d::SdlWindow& window;
  iggy3d::creative::CreativeAppState& mapAppState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
};

struct CreativeEditorAssetLibraryFrameResult {
  bool blockWorldActions = false;
  bool activeDocumentChanged = false;
  iggy3d::creative::CreativeInputRouteResult remainingInput;
};

[[nodiscard]] std::string_view
toString(CreativeEditorAssetLibraryAction action) noexcept;
[[nodiscard]] std::string_view
toString(CreativeEditorAssetEditMenuAction action) noexcept;

[[nodiscard]] std::string
normalizeCreativeEditorAssetLibraryText(std::string_view text);

void refreshCreativeEditorAuthoredAssetEditDirtyState(
    CreativeEditorState& editor);

[[nodiscard]] CreativeEditorAuthoredAssetReferenceSummary
summarizeCreativeEditorAuthoredAssetReferences(
    const iggy3d::creative::CreativeDocument& mapDocument,
    const CreativeEditorAuthoredAssetLibrary& library,
    std::string_view assetId) noexcept;

[[nodiscard]] CreativeEditorAuthoredAssetMutationReceipt
renameCreativeEditorAuthoredAsset(CreativeEditorAuthoredAssetLibrary& library,
                                  std::string_view assetId,
                                  std::string_view label);

[[nodiscard]] CreativeEditorAuthoredAssetMutationReceipt
duplicateCreativeEditorAuthoredAsset(
    CreativeEditorAuthoredAssetLibrary& library, std::string_view assetId);

[[nodiscard]] CreativeEditorAuthoredAssetMutationReceipt
deleteCreativeEditorAuthoredAsset(
    const iggy3d::creative::CreativeDocument& mapDocument,
    CreativeEditorAuthoredAssetLibrary& library, std::string_view assetId);

[[nodiscard]] bool beginCreativeEditorAssetLibrary(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& mapDocument,
    std::string_view assetId);

[[nodiscard]] CreativeEditorAuthoredAssetMutationReceipt
beginCreativeEditorAuthoredAssetEdit(CreativeEditorState& editor,
                                     std::string_view assetId);

[[nodiscard]] CreativeEditorAuthoredAssetMutationReceipt
saveCreativeEditorAuthoredAssetEdit(CreativeEditorState& editor);

[[nodiscard]] bool cancelCreativeEditorAuthoredAssetEdit(
    CreativeEditorState& editor,
    std::string_view reasonCode =
        "creative_authored_asset_edit_cancelled");

[[nodiscard]] iggy3d::creative::CreativeAppState& activeCreativeEditorAppState(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeAppState& mapAppState) noexcept;

[[nodiscard]] const iggy3d::creative::CreativeAppState&
activeCreativeEditorAppState(
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& mapAppState) noexcept;

[[nodiscard]] CreativeEditorAssetLibraryFrameResult
processCreativeEditorAssetLibraryFrame(
    const CreativeEditorAssetLibraryFrameRequest& request);

void appendCreativeEditorAssetLibraryOverlay(
    const CreativeEditorState& editor, std::uint32_t drawableWidth,
    std::uint32_t drawableHeight, std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

} // namespace iggy3d_creative_app
