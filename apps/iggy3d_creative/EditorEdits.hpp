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

struct CreativeEditorWorldLayoutState;
struct CreativeEditorObjectReattachmentPlan;
struct CreativeEditorObjectReattachmentReceipt;

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
                                std::string_view source,
                                CreativeEditorWorldLayoutState* worldLayout =
                                    nullptr);
[[nodiscard]] bool redoLastEdit(cr::CreativeAppState& appState,
                                std::string_view source,
                                CreativeEditorWorldLayoutState* worldLayout =
                                    nullptr);

[[nodiscard]] cr::CreativeSemanticDeleteReceipt deleteSelectedObjectsWithUndo(
    cr::CreativeAppState& appState,
    std::string_view source,
    StandaloneEditHistory* history = nullptr);

struct CreativeEditorDeleteReceipt {
  bool accepted = false;
  bool changed = false;
  bool worldLayoutSourceDeleted = false;
  std::uint64_t affectedObjectCount = 0U;
  std::string reasonCode = "creative_editor_delete_not_requested";
};

// Routes generated output to its nearest editable owner. Pattern outputs use
// semantic document deletion; synchronized World Layout output edits the 2D
// source; ordinary authored objects use semantic document deletion directly.
[[nodiscard]] CreativeEditorDeleteReceipt
deleteCreativeEditorSelectionWithUndo(
    cr::CreativeAppState& appState,
    std::string_view source,
    StandaloneEditHistory* history = nullptr,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorDeleteReceipt
deleteCreativeEditorObjectsWithUndo(
    cr::CreativeAppState& appState,
    std::span<const cr::CreativeObjectId> objectIds,
    std::string_view source,
    StandaloneEditHistory* history = nullptr,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

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

struct CreativeEditorDuplicateReceipt {
  bool accepted = false;
  bool changed = false;
  bool worldLayoutSourceDuplicated = false;
  std::uint64_t affectedObjectCount = 0U;
  std::string reasonCode = "creative_editor_duplicate_not_requested";
};

// Routes generated output to its nearest editable owner. Synchronized World
// Layout output duplicates source truth; pattern output and ordinary authored
// objects continue through the atomic document duplicate kernel.
[[nodiscard]] CreativeEditorDuplicateReceipt
duplicateCreativeEditorSelectionWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const cr::CreativeDuplicateCommandRequest& request,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

struct CreativeEditorSemanticEditReceipt {
  bool accepted = false;
  bool changed = false;
  bool worldLayoutSourceChanged = false;
  bool requiresAdoption = false;
  std::uint64_t affectedObjectCount = 0U;
  std::string reasonCode = "creative_editor_semantic_edit_not_requested";
};

[[nodiscard]] CreativeEditorSemanticEditReceipt
transformCreativeEditorSelectionWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const cr::CreativeTransformCommandRequest& request,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorSemanticEditReceipt
renameCreativeEditorObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    std::string name,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorObjectsVisibleWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const cr::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorObjectsLockedWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const cr::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorObjectTransformWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    const cr::CreativeTransform& transform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorSemanticEditReceipt
toggleCreativeEditorSelectionVisibilityWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);
[[nodiscard]] CreativeEditorSemanticEditReceipt
toggleCreativeEditorSelectionLockedWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

// Clears an authored attachment relationship without changing the object's
// stored world transform. Generated output rejects before history begins.
[[nodiscard]] CreativeEditorSemanticEditReceipt
detachCreativeEditorObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorObjectReattachmentReceipt
reattachCreativeEditorObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const CreativeEditorObjectReattachmentPlan& plan,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] cr::CreativeClipboardCopyReceipt copySelectionToClipboard(
    cr::CreativeAppState& appState,
    std::string_view source);

[[nodiscard]] cr::CreativeClipboardCutReceipt cutSelectionToClipboardWithHistory(
    cr::CreativeAppState& appState,
    std::string_view source);
[[nodiscard]] CreativeEditorSemanticEditReceipt
cutCreativeEditorSelectionToClipboardWithHistory(
    cr::CreativeAppState& appState,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

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

// K-2: atomically removes an explicit id list (or the current selection when
// the span is empty) under one history transaction. Each target root expands
// to its full hierarchy; any missing or locked object rejects the whole batch.
[[nodiscard]] CreativeStandaloneBatchEditReceipt deleteObjectsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const cr::CreativeObjectId> objectIds,
    std::string_view source);

[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorMovingPlatformSettingsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeMovingPlatformSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorPlayerSpawnSettingsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativePlayerSpawnSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorNpcSpawnSettingsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeNpcSpawnSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorLootPointSettingsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeLootPointSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorSemanticEditReceipt
setCreativeEditorExitPointSettingsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeExitPointSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout = nullptr);

}  // namespace iggy3d_creative_app
