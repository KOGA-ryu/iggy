#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <span>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutLifecycleCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutDeleteSelection: {
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          deleteCreativeEditorWorldLayoutSelection(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreview: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the terrain region before layout preview";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayout(editor.worldLayout,
                                           appState.facade.document());
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.accepted;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted) {
        editor.desktopUi.showWorldLayout = false;
      }
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutConfirm: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the terrain region before layout generation";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutConfirmPayload>(command);
      if (payload == nullptr &&
          !std::holds_alternative<std::monostate>(command.payload)) {
        result.message = "layout confirm: payload mismatch";
        break;
      }
      const std::span<const creative::CreativeWorldLayoutConflictDecision>
          conflictDecisions =
              payload != nullptr
                  ? std::span<const creative::CreativeWorldLayoutConflictDecision>(
                        payload->conflictDecisions)
                  : std::span<const
                        creative::CreativeWorldLayoutConflictDecision>{};
      const std::span<
          const creative::CreativeWorldLayoutTerrainConflictDecision>
          terrainConflictDecisions =
              payload != nullptr
                  ? std::span<const creative::
                                  CreativeWorldLayoutTerrainConflictDecision>(
                        payload->terrainConflictDecisions)
                  : std::span<const creative::
                                  CreativeWorldLayoutTerrainConflictDecision>{};
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          confirmCreativeEditorWorldLayout(editor.worldLayout, appState,
                                           conflictDecisions,
                                           terrainConflictDecisions);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.changed;
      result.affectedObjectCount = receipt.apply.installReceipt.nextObjectCount;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted) {
        editor.desktopUi.showWorldLayout = false;
      }
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCancelPreview: {
      const CreativeEditorWorldLayoutEditReceipt receipt =
          cancelCreativeEditorWorldLayoutPreview(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      editor.desktopUi.showWorldLayout = true;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCancelGeneratedSettingsPreview: {
      const CreativeEditorWorldLayoutEditReceipt receipt =
          cancelCreativeEditorWorldLayoutPreview(editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
