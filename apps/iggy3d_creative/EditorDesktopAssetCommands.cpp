#include "EditorDesktopCommandsInternal.hpp"

#include "EditorAssetLibrary.hpp"
#include "EditorAuthoredAssets.hpp"
#include "EditorObjectActions.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopAssetCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
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
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
