#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/tools/Transform.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

using StandaloneEditHistory = cr::CreativeDocumentHistory;
using StandaloneEditTransaction = cr::CreativeDocumentHistoryTransaction;

struct CreativeEditorClipboardPasteState {
  bool active = false;
  bool targetValid = false;
  bool commitRequested = false;
  cr::CreativeVec3 targetAnchor{};
  cr::CreativeClipboardPasteRequest request{};
  cr::CreativeClipboardPasteReceipt lastReceipt;
};

void clearEditHistory(StandaloneEditHistory& history, std::string_view source);

[[nodiscard]] StandaloneEditTransaction beginEditTransaction(
    const cr::Facade& facade,
    std::string_view source);

[[nodiscard]] cr::CreativeHistoryRecordReceipt completeEditTransaction(
    StandaloneEditHistory& history,
    StandaloneEditTransaction transaction,
    const cr::Facade& facade,
    bool changed,
    std::string_view reasonCode);

[[nodiscard]] bool undoLastEdit(cr::CreativeAppState& appState,
                                std::string_view source);
[[nodiscard]] bool redoLastEdit(cr::CreativeAppState& appState,
                                std::string_view source);

[[nodiscard]] iggy3d::creative::CreativeDocumentRemoveReceipt
deleteSelectedObject(iggy3d::creative::CreativeAppState& appState,
                     std::string_view source,
                     StandaloneEditHistory* history = nullptr);

[[nodiscard]] cr::CreativeTransformCommandReceipt
transformSelectedObjectsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const cr::CreativeTransformCommandRequest& request,
    std::string_view source);

[[nodiscard]] cr::CreativeDuplicateCommandReceipt
duplicateSelectedObjectsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const cr::CreativeDuplicateCommandRequest& request,
    std::string_view source);

[[nodiscard]] cr::CreativeClipboardCopyReceipt copySelectionToClipboard(
    cr::CreativeAppState& appState,
    std::string_view source);

[[nodiscard]] cr::CreativeClipboardCutReceipt cutSelectionToClipboardWithHistory(
    cr::CreativeAppState& appState,
    std::string_view source);

[[nodiscard]] cr::CreativeClipboardPasteReceipt pasteClipboardWithHistory(
    cr::CreativeAppState& appState,
    const cr::CreativeClipboardPasteRequest& request,
    std::string_view source);

[[nodiscard]] bool beginCreativeEditorClipboardPastePreview(
    const cr::CreativeClipboard& clipboard,
    CreativeEditorClipboardPasteState& state,
    std::string_view source);
[[nodiscard]] bool requestCreativeEditorClipboardPasteCommit(
    CreativeEditorClipboardPasteState& state) noexcept;
[[nodiscard]] bool cancelCreativeEditorClipboardPastePreview(
    CreativeEditorClipboardPasteState& state,
    std::string_view source);
[[nodiscard]] cr::CreativeClipboardPasteReceipt
processCreativeEditorClipboardPastePreview(
    cr::CreativeAppState& appState,
    CreativeEditorClipboardPasteState& state,
    bool targetValid,
    cr::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source);
[[nodiscard]] std::size_t appendCreativeEditorClipboardPastePreview(
    const cr::CreativeClipboard& clipboard,
    const CreativeEditorClipboardPasteState& state,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
