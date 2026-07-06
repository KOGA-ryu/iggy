#pragma once

#include "app/iggy3d/creative/Facade.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative {

// Identity + save-status of the creative world the product app currently has
// live. SLICE 2 moves this off ProductAppWindowState (the god-struct) into
// creative's own container; the god-struct mirror fields are kept in lockstep
// (additive) so the receipt + identity tests stay green while production
// readers migrate onto this.
struct CreativeActiveIdentity {
  std::string saveId = "none";
  std::string savePath = "none";
  std::string worldId = "none";
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  CreativeObjectId nextObjectId = kInvalidObjectId;

  std::string saveStatus = "creative_world_save_not_requested";
  std::string saveReasonCode = "creative_world_save_not_requested";
  std::uint64_t saveDirtyFlagsBefore = 0;
  std::uint64_t saveDirtyFlagsDrained = 0;
  std::uint64_t saveDirtyFlagsAfter = 0;
  std::string saveSavedAtUtc = "none";

  // Reset to the "no live creative world" baseline. The single owner of the
  // clear body: the product app calls this instead of triplicating the field
  // writes across Operations / save::Flow / the pause flow.
  void clear() noexcept {
    saveId = "none";
    savePath = "none";
    worldId = "none";
    documentId = kInvalidDocumentId;
    objectCount = 0;
    nextObjectId = kInvalidObjectId;
    saveStatus = "creative_world_save_not_requested";
    saveReasonCode = "creative_world_save_not_requested";
    saveDirtyFlagsBefore = 0;
    saveDirtyFlagsDrained = 0;
    saveDirtyFlagsAfter = 0;
    saveSavedAtUtc = "none";
  }

  // True when a creative world is live (a save id, a world id, or a document).
  [[nodiscard]] bool worldActive() const noexcept {
    return (!saveId.empty() && saveId != "none") ||
           (!worldId.empty() && worldId != "none") ||
           documentId != kInvalidDocumentId;
  }
};

struct CreativeDocumentUndoStack {
  std::vector<CreativeDocument> documents;
  std::size_t maxDepth = 32;
};

struct CreativeDocumentUndoApplyReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool hadSnapshot = false;
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t objectCountBefore = 0;
  std::uint64_t objectCountAfter = 0;
  std::uint64_t depthBefore = 0;
  std::uint64_t depthAfter = 0;
  std::string status = "creative_undo_not_requested";
  std::string reasonCode = "creative_undo_not_requested";
  std::string message = "creative_undo_not_requested";
  CreativeFacadeDocumentInstallReceipt installReceipt;
};

[[nodiscard]] inline bool creativeUndoAvailable(
    const CreativeDocumentUndoStack& undoStack) noexcept {
  return !undoStack.documents.empty();
}

[[nodiscard]] inline std::uint64_t creativeUndoDepth(
    const CreativeDocumentUndoStack& undoStack) noexcept {
  return static_cast<std::uint64_t>(undoStack.documents.size());
}

inline void clearCreativeUndoStack(
    CreativeDocumentUndoStack& undoStack) noexcept {
  undoStack.documents.clear();
}

inline void pushCreativeUndoSnapshot(CreativeDocumentUndoStack& undoStack,
                                     const CreativeDocument& document) {
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    return;
  }
  if (undoStack.documents.size() >= undoStack.maxDepth) {
    undoStack.documents.erase(undoStack.documents.begin());
  }
  undoStack.documents.push_back(document);
}

inline bool discardCreativeUndoSnapshot(CreativeDocumentUndoStack& undoStack,
                                        std::uint64_t depthBefore) {
  if (creativeUndoDepth(undoStack) <= depthBefore) {
    return false;
  }
  undoStack.documents.pop_back();
  return true;
}

// Self-contained home for creative's app-scoped state. SLICE 1 wrapped only the
// logical Facade; SLICE 2 adds the active-world identity so the product app can
// read creative's own state for routing/save instead of the god-struct.
struct CreativeAppState {
  Facade facade;
  CreativeActiveIdentity identity;
  CreativeDocumentUndoStack undoStack;
};

[[nodiscard]] inline CreativeDocumentUndoApplyReceipt
applyLastCreativeUndoSnapshot(Facade& facade,
                              CreativeDocumentUndoStack& undoStack) {
  CreativeDocumentUndoApplyReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = facade.document().revision();
  receipt.objectCountBefore = facade.document().objectCount();
  receipt.depthBefore = creativeUndoDepth(undoStack);

  if (!creativeUndoAvailable(undoStack)) {
    receipt.depthAfter = receipt.depthBefore;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.objectCountAfter = receipt.objectCountBefore;
    receipt.status = "creative_undo_empty";
    receipt.reasonCode = "creative_undo_empty";
    receipt.message = "creative_undo_empty";
    return receipt;
  }

  receipt.hadSnapshot = true;
  CreativeDocument snapshot = undoStack.documents.back();
  receipt.documentId = snapshot.id();
  receipt.installReceipt = facade.installDocument(std::move(snapshot));
  receipt.accepted = receipt.installReceipt.accepted;
  receipt.changed = receipt.installReceipt.changed;
  if (receipt.installReceipt.accepted) {
    undoStack.documents.pop_back();
  }
  receipt.revisionAfter = facade.document().revision();
  receipt.objectCountAfter = facade.document().objectCount();
  receipt.depthAfter = creativeUndoDepth(undoStack);
  receipt.status = receipt.accepted ? "creative_undo_applied"
                                    : "creative_undo_rejected";
  receipt.reasonCode =
      receipt.accepted ? "creative_undo_applied"
                       : std::string(receipt.installReceipt.reasonCode);
  receipt.message =
      receipt.accepted ? "creative_undo_applied"
                       : std::string(receipt.installReceipt.message);
  return receipt;
}

[[nodiscard]] inline CreativeDocumentUndoApplyReceipt
applyLastCreativeUndoSnapshot(CreativeAppState& appState) {
  return applyLastCreativeUndoSnapshot(appState.facade, appState.undoStack);
}

}  // namespace iggy3d::creative
