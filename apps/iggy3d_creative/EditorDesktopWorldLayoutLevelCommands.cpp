#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutLevelCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutLevelOperation: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutLevelOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout level operation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutLevelOperation(
              editor.worldLayout, payload->operation, payload->buildingIndex,
              payload->levelIndex);
      const bool sourceChanged =
          payload->operation !=
              CreativeEditorWorldLayoutLevelOperation::Select &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetLevelSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutLevelSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout level settings: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey)) {
        result.message = "layout level settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutLevelSettings(
              editor.worldLayout, payload->levelIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetLevelDatum: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutLevelDatumPayload>(command);
      if (payload == nullptr) {
        result.message = "layout level datum: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey)) {
        result.message = "layout level datum: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutLevelDatum(
              editor.worldLayout,
              {payload->levelIndex, payload->scope,
               payload->floorTopLayer});
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedLevelSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated level settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated level settings: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated level settings: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated level settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutLevelSettingsToDocument(
              editor.worldLayout, appState, payload->levelIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedLevelSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated level preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated level preview: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Level,
              payload->levelIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated level preview: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated level preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutLevelSettings(
              editor.worldLayout, appState.facade.document(),
              payload->levelIndex, payload->settings);
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
