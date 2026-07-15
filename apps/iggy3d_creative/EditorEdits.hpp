#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/tools/SelectionTransformCommands.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

using StandaloneEditHistory = cr::CreativeDocumentHistory;
using StandaloneEditTransaction = cr::CreativeDocumentHistoryTransaction;

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

[[nodiscard]] cr::CreativeFacadeMutationReceipt
toggleSelectedObjectVisibilityWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source);

[[nodiscard]] cr::CreativeFacadeMutationReceipt
toggleSelectedObjectLockedWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source);

// Clears an attachment relationship without changing the object's stored
// world transform. The relationship change and its undo snapshot are one edit.
[[nodiscard]] cr::CreativeDocumentMutationReceipt detachObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
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
[[nodiscard]] cr::CreativeClipboardPasteReceipt pasteClipboardWithHistory(
    cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    const cr::CreativeClipboardPasteRequest& request,
    std::string_view source);

// Aggregate outcome for the desktop batch/multi-component edit kernels that do
// not map onto a single existing receipt (multi-delete, absolute transform,
// batch visibility/lock). One history transaction spans the whole batch.
// accepted = the command had valid targets and executed; changed = it altered
// the document.
struct CreativeStandaloneBatchEditReceipt {
  bool accepted = false;
  bool changed = false;
  std::uint64_t affectedObjectCount = 0U;
  std::string message;
};

// K-2: removes an explicit id list (or the current selection when the span is
// empty) under a single history transaction. Each target root expands to its
// full hierarchy and objects are removed deepest-first so a group root never
// trips CreativeDocumentRemoveStatus::ParentHasChildren.
[[nodiscard]] CreativeStandaloneBatchEditReceipt deleteObjectsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const cr::CreativeObjectId> objectIds,
    std::string_view source);

// K-3: history-wrapped absolute rename of a single object.
[[nodiscard]] cr::CreativeDocumentMutationReceipt renameObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    std::string name,
    std::string_view source);

// Absolute set-visible / set-locked over an explicit id list (or the current
// selection when the span is empty), one transaction for the whole batch.
// Distinct from the toggle*WithUndo helpers: honors the absolute bool the
// desktop panels emit rather than flipping current state.
[[nodiscard]] CreativeStandaloneBatchEditReceipt setObjectsVisibleWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const cr::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source);
[[nodiscard]] CreativeStandaloneBatchEditReceipt setObjectsLockedWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const cr::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source);

// K-7: absolute transform of a single object. The component flags select which
// of position/rotation/scale to write; all three set collapses to one
// SetTransform mutation, otherwise the requested components apply as
// Move/Rotate/Scale under a single transaction.
[[nodiscard]] CreativeStandaloneBatchEditReceipt setObjectTransformWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    const cr::CreativeTransform& transform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    std::string_view source);

}  // namespace iggy3d_creative_app
