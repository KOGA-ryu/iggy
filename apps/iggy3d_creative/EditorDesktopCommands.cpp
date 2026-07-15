#include "EditorDesktopCommands.hpp"

#include <span>
#include <string>
#include <utility>
#include <variant>

#include "EditorAssetLibrary.hpp"
#include "EditorAuthoredAssets.hpp"
#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorObjectActions.hpp"
#include "EditorPersistence.hpp"
#include "EditorPlayMode.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Facade.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].payload = std::monostate{};
  ++count;
}

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       std::string saveId) {
  push(id, CreativeDesktopCommandPayload{
               CreativeDesktopSaveAsPayload{std::move(saveId)}});
}

void CreativeDesktopCommandFrame::push(CreativeDesktopCommandId id,
                                       CreativeDesktopCommandPayload payload) {
  if (count >= kCreativeDesktopCommandCapacity) {
    overflowed = true;
    return;
  }
  commands[count].id = id;
  commands[count].payload = std::move(payload);
  ++count;
}

void CreativeDesktopCommandFrame::clear() noexcept {
  count = 0U;
  overflowed = false;
}

namespace {

// A mismatched id/payload is an explicit no-op failure — never a
// reinterpretation. Returns nullptr when the variant holds a different type.
template <typename Payload>
[[nodiscard]] const Payload* payloadAs(const CreativeDesktopCommand& command) {
  return std::get_if<Payload>(&command.payload);
}

void dispatchOne(const CreativeDesktopCommand& command,
                 const CreativeDesktopCommandContext& context,
                 CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  // Edit mutations run against whichever document is live (map document, or an
  // open authored-asset edit workspace) so an asset-edit session is never
  // corrupted. Document-lifecycle and library/instance ops stay on the map.
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  result.lastCommand = command.id;
  result.accepted = false;
  result.changed = false;
  result.documentReplaced = false;
  result.affectedObjectCount = 0U;
  result.message.clear();

  if (context.playMode != nullptr &&
      creativeEditorPlayModeActive(*context.playMode) &&
      command.id != CreativeDesktopCommandId::Play &&
      command.id != CreativeDesktopCommandId::None) {
    result.message = "stop play before editing";
    return;
  }

  switch (command.id) {
    case CreativeDesktopCommandId::NewDocument:
      clearToBlankScene(appState);
      clearEditHistory(appState.history, "desktop_new");
      resetCreativeEditorForDocumentReplacement(
          editor, appState.facade.document().id());
      result.accepted = true;
      result.changed = true;
      result.documentReplaced = true;
      result.message = "new document";
      break;
    case CreativeDesktopCommandId::OpenDocument: {
      const std::string saveId =
          context.activeSaveId != nullptr ? *context.activeSaveId : std::string{};
      const bool loaded = loadStandaloneScene(appState, context.saveRoot, saveId);
      if (loaded) {
        clearEditHistory(appState.history, "desktop_open");
        resetCreativeEditorForDocumentReplacement(
            editor, appState.facade.document().id());
      }
      result.accepted = loaded;
      result.changed = loaded;
      result.documentReplaced = loaded;
      result.message = loaded ? "opened " + saveId : "open failed";
      break;
    }
    case CreativeDesktopCommandId::SaveDocument: {
      const std::string saveId =
          context.activeSaveId != nullptr ? *context.activeSaveId : std::string{};
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, saveId);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        clearEditHistory(appState.history, "desktop_save");
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved " + saveId : "save failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SaveDocumentAs: {
      const auto* payload = payloadAs<CreativeDesktopSaveAsPayload>(command);
      const std::string saveId = payload != nullptr ? payload->saveId : std::string{};
      if (saveId.empty()) {
        result.message = "save as: empty name";
        break;
      }
      const iggy3d::CreativeWorldSaveResult saveResult =
          saveStandaloneScene(appState.facade, context.saveRoot, saveId);
      const bool ok = saveResult.accepted && saveResult.saved;
      if (ok) {
        if (context.activeSaveId != nullptr) {
          *context.activeSaveId = saveId;
        }
        clearEditHistory(appState.history, "desktop_save_as");
      }
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "saved as " + saveId
                          : "save as failed: " + saveResult.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::Undo: {
      const bool ok = undoLastEdit(activeAppState, "desktop_undo");
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "undo" : "nothing to undo";
      break;
    }
    case CreativeDesktopCommandId::Redo: {
      const bool ok = redoLastEdit(activeAppState, "desktop_redo");
      result.accepted = ok;
      result.changed = ok;
      result.message = ok ? "redo" : "nothing to redo";
      break;
    }
    case CreativeDesktopCommandId::DuplicateSelection: {
      const creative::CreativeDuplicateCommandReceipt receipt =
          duplicateSelectedObjectsWithUndo(
              activeAppState, activeAppState.history,
              creative::CreativeDuplicateCommandRequest{}, "desktop_duplicate");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = receipt.changed ? "duplicated selection"
                                       : "nothing to duplicate";
      break;
    }
    case CreativeDesktopCommandId::DeleteSelection: {
      const creative::CreativeDocumentRemoveReceipt receipt =
          deleteSelectedObject(activeAppState, "desktop_delete",
                               &activeAppState.history);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = receipt.changed ? "deleted selection"
                                       : "nothing to delete";
      break;
    }
    case CreativeDesktopCommandId::SelectObjects: {
      const auto* payload = payloadAs<CreativeDesktopSelectPayload>(command);
      if (payload == nullptr) {
        result.message = "select: payload mismatch";
        break;
      }
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(payload->objectIds,
                                              payload->primaryObjectId);
      result.accepted = true;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = "selection updated";
      break;
    }
    case CreativeDesktopCommandId::ClearSelection: {
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{},
              creative::kInvalidObjectId);
      result.accepted = true;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = "selection cleared";
      break;
    }
    case CreativeDesktopCommandId::DeleteObjects: {
      const auto* payload = payloadAs<CreativeDesktopDeletePayload>(command);
      if (payload == nullptr) {
        result.message = "delete objects: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt = deleteObjectsWithUndo(
          activeAppState, activeAppState.history, payload->objectIds,
          "desktop_delete_objects");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.changed ? "deleted objects" : "nothing deleted";
      break;
    }
    case CreativeDesktopCommandId::RenameObject: {
      const auto* payload = payloadAs<CreativeDesktopRenamePayload>(command);
      if (payload == nullptr) {
        result.message = "rename: payload mismatch";
        break;
      }
      if (payload->objectId == creative::kInvalidObjectId ||
          payload->name.empty()) {
        result.message = "rename: invalid request";
        break;
      }
      const creative::CreativeDocumentMutationReceipt receipt =
          renameObjectWithUndo(activeAppState, activeAppState.history,
                               payload->objectId, payload->name,
                               "desktop_rename");
      const bool applied =
          receipt.status == creative::CreativeDocumentMutationStatus::Applied;
      result.accepted = applied;
      result.changed = applied && receipt.changed;
      result.affectedObjectCount = result.changed ? 1U : 0U;
      result.message = applied ? "renamed object" : "rename failed";
      break;
    }
    case CreativeDesktopCommandId::SetObjectsVisible: {
      const auto* payload = payloadAs<CreativeDesktopObjectFlagPayload>(command);
      if (payload == nullptr) {
        result.message = "visibility: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt =
          setObjectsVisibleWithUndo(activeAppState, activeAppState.history,
                                    payload->objectIds, payload->value,
                                    "desktop_set_visible");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.changed ? "visibility updated"
                                       : "visibility unchanged";
      break;
    }
    case CreativeDesktopCommandId::SetObjectsLocked: {
      const auto* payload = payloadAs<CreativeDesktopObjectFlagPayload>(command);
      if (payload == nullptr) {
        result.message = "lock: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt =
          setObjectsLockedWithUndo(activeAppState, activeAppState.history,
                                   payload->objectIds, payload->value,
                                   "desktop_set_locked");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.changed ? "lock updated" : "lock unchanged";
      break;
    }
    case CreativeDesktopCommandId::SetObjectTransform: {
      const auto* payload = payloadAs<CreativeDesktopTransformPayload>(command);
      if (payload == nullptr) {
        result.message = "transform: payload mismatch";
        break;
      }
      const CreativeStandaloneBatchEditReceipt receipt =
          setObjectTransformWithUndo(activeAppState, activeAppState.history,
                                     payload->objectId, payload->transform,
                                     payload->setPosition, payload->setRotation,
                                     payload->setScale, "desktop_set_transform");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = receipt.accepted
                           ? (receipt.changed ? "transform set"
                                              : "transform unchanged")
                           : receipt.message;
      break;
    }
    case CreativeDesktopCommandId::EquipAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "equip: payload mismatch";
        break;
      }
      const creative::CreativeAuthoredAssetDefinition* definition =
          findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                          payload->assetId);
      if (definition == nullptr) {
        result.message = "equip: unknown asset";
        break;
      }
      const bool equipped =
          equipCreativeEditorAuthoredAssetToHotbar(appState, editor, *definition);
      result.accepted = equipped;
      result.changed = equipped;
      result.message = equipped ? "equipped " + definition->label
                                : "equip failed";
      break;
    }
    case CreativeDesktopCommandId::EditAssetSource: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "edit asset: payload mismatch";
        break;
      }
      switch (payload->editPhase) {
        case CreativeDesktopAssetEditPhase::Begin: {
          const CreativeEditorAuthoredAssetMutationReceipt receipt =
              beginCreativeEditorAuthoredAssetEdit(editor, payload->assetId);
          result.accepted = receipt.accepted;
          result.changed = receipt.accepted;
          result.message =
              receipt.accepted ? "edit session started" : "edit begin failed";
          break;
        }
        case CreativeDesktopAssetEditPhase::Save: {
          const CreativeEditorAuthoredAssetMutationReceipt receipt =
              saveCreativeEditorAuthoredAssetEdit(editor);
          result.accepted = receipt.accepted;
          result.changed = receipt.accepted;
          result.message =
              receipt.accepted ? "edit session saved" : "edit save failed";
          break;
        }
        case CreativeDesktopAssetEditPhase::Cancel: {
          const bool ok = cancelCreativeEditorAuthoredAssetEdit(editor);
          result.accepted = ok;
          result.changed = ok;
          result.message = ok ? "edit session cancelled" : "no edit session";
          break;
        }
        case CreativeDesktopAssetEditPhase::None:
          result.message = "edit asset: missing phase";
          break;
      }
      break;
    }
    case CreativeDesktopCommandId::RenameAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "rename asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetMutationReceipt receipt =
          renameCreativeEditorAuthoredAsset(editor.authoredAssets,
                                            payload->assetId, payload->name);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message =
          receipt.accepted ? "asset renamed" : "asset rename failed";
      break;
    }
    case CreativeDesktopCommandId::DuplicateAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "duplicate asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetMutationReceipt receipt =
          duplicateCreativeEditorAuthoredAsset(editor.authoredAssets,
                                               payload->assetId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message =
          receipt.accepted ? "asset duplicated" : "asset duplicate failed";
      break;
    }
    case CreativeDesktopCommandId::DeleteAsset: {
      const auto* payload = payloadAs<CreativeDesktopAssetOpPayload>(command);
      if (payload == nullptr) {
        result.message = "delete asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetMutationReceipt receipt =
          deleteCreativeEditorAuthoredAsset(appState.facade.document(),
                                            editor.authoredAssets,
                                            payload->assetId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message = receipt.accepted ? "asset deleted"
                                        : "asset delete blocked: " +
                                              receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::RefreshInstances: {
      const auto* payload =
          payloadAs<CreativeDesktopInstanceRefreshPayload>(command);
      if (payload == nullptr) {
        result.message = "refresh instances: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetInstanceRefreshReceipt receipt =
          refreshCreativeEditorAuthoredAssetInstances(
              appState, editor.authoredAssets, payload->instanceRootObjectId,
              payload->mode);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message = receipt.accepted ? "instances refreshed"
                                        : "refresh failed: " + receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::UpdateAssetFromInstance: {
      const auto* payload =
          payloadAs<CreativeDesktopInstanceRefreshPayload>(command);
      if (payload == nullptr) {
        result.message = "update asset: payload mismatch";
        break;
      }
      const CreativeEditorAuthoredAssetUpdateReceipt receipt =
          updateCreativeEditorAuthoredAssetFromInstance(
              appState, editor.authoredAssets, payload->instanceRootObjectId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.message = receipt.accepted ? "asset updated from instance"
                                        : "update failed: " + receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::Play:
      if (context.playMode == nullptr) {
        result.message = "play owner is unavailable";
        break;
      }
      if (creativeEditorPlayModeActive(*context.playMode)) {
        const creative::CreativeRuntimeSandboxStopReceipt stopped =
            stopCreativeEditorPlayMode(*context.playMode);
        result.accepted = stopped.stopped;
        result.changed = stopped.stopped;
        result.message = stopped.stopped ? "play stopped"
                                         : "play stop failed";
        break;
      }
      {
        CreativeEditorPlayStartRequest request;
        request.document = &appState.facade.document();
        request.staticMeshAssetCatalog = context.staticMeshAssetCatalog;
        const CreativeEditorPlayStartReceipt started =
            startCreativeEditorPlayMode(*context.playMode, std::move(request));
        result.accepted = started.accepted;
        result.changed = started.accepted;
        result.message = started.accepted
                             ? "play started"
                             : "play failed: " + started.reasonCode;
      }
      break;
    case CreativeDesktopCommandId::None:
    case CreativeDesktopCommandId::Count:
      break;
  }
}

}  // namespace

CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context) {
  CreativeDesktopCommandResult result;
  const std::size_t count =
      frame.count < kCreativeDesktopCommandCapacity ? frame.count
                                                    : kCreativeDesktopCommandCapacity;
  for (std::size_t index = 0U; index < count; ++index) {
    dispatchOne(frame.commands[index], context, result);
  }
  return result;
}

}  // namespace iggy3d_creative_app
