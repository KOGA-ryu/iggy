#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include "EditorWorldLayoutPlan.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutElementCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedVerticalConnectorSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopGeneratedVerticalConnectorSettingsPayload>(command);
      if (payload == nullptr) {
        result.message =
            "generated vertical connector settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated vertical connector settings: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table !=
              creative::CreativeWorldLayoutTable::VerticalConnector) {
        result.message = "generated vertical connector settings: source mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutVerticalConnectorSettingsToDocument(
              editor.worldLayout, appState, provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutPreviewGeneratedVerticalConnectorSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopGeneratedVerticalConnectorSettingsPayload>(command);
      if (payload == nullptr) {
        result.message =
            "generated vertical connector preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated vertical connector preview: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table !=
              creative::CreativeWorldLayoutTable::VerticalConnector) {
        result.message = "generated vertical connector preview: source mismatch";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutVerticalConnectorSettings(
              editor.worldLayout, appState.facade.document(), provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload>(
          command);
      if (payload == nullptr) {
        result.message =
            "layout vertical connector manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                  Begin,
              CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                  Update,
              CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                  Commit,
              CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                  Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutVerticalConnectorManipulationPhase
                      phase) {
                return applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
                    target, phase, payload->point, payload->toleranceCells,
                    appState.facade.document().gridSettings(),
                    payload->target);
              },
              "desktop_world_layout_vertical_connector_drag",
              "connector drag preview ready in 3D",
              "vertical connector updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetBoxSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBoxSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout floor settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutBoxSettings(
              editor.worldLayout, payload->boxIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateBox: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBoxManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout floor manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
              CreativeEditorWorldLayoutBoxManipulationPhase::Update,
              CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
              CreativeEditorWorldLayoutBoxManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutBoxManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutBoxManipulation(
                    target, phase, payload->point, payload->toleranceCells);
              },
              "desktop_world_layout_floor_drag",
              "floor drag preview ready in 3D", "floor updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
