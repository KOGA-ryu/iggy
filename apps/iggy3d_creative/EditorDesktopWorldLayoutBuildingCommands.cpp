#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <span>
#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutBuildingCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingBlockoutPayload>(command);
      if (payload == nullptr) {
        result.message = "layout building blockout: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          createCreativeEditorWorldLayoutBuildingBlockout(
              editor.worldLayout, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingManipulationPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building manipulation: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingManipulation(
              editor.worldLayout, payload->phase, payload->point,
              payload->toleranceCells);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingManipulationPhase::Commit &&
          receipt.changed;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged = previewWasActive && sourceChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutDuplicateBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingDuplicatePayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building duplicate: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          duplicateCreativeEditorWorldLayoutBuilding(
              editor.worldLayout, payload->buildingIndex,
              payload->deltaXCells, payload->deltaZCells);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTransformBuilding: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTransformPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout building transform: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingTransform(
              editor.worldLayout, payload->phase, payload->operation);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTransformPhase::Commit &&
          receipt.changed;
      const bool exactPreviewClosed =
          previewWasActive &&
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTransformPhase::Preview &&
          receipt.accepted;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged =
          exactPreviewClosed || (previewWasActive && sourceChanged);
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutPreviewGeneratedBuildingOperation: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedBuildingOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "generated building preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated building preview: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated building preview: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated building preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutGeneratedBuildingOperation(
              editor.worldLayout, appState.facade.document(),
              payload->buildingIndex, payload->operation,
              payload->deltaXCells, payload->deltaZCells);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedBuildingOperation: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedBuildingOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "generated building operation: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated building operation: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated building operation: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated building operation: stale target";
        break;
      }
      const creative::CreativeObjectKind sourceObjectKind = object->kind;
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutGeneratedBuildingOperationToDocument(
              editor.worldLayout, appState, payload->buildingIndex,
              payload->operation, payload->deltaXCells,
              payload->deltaZCells);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      if (receipt.accepted &&
          payload->operation ==
              CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate) {
        const std::size_t duplicateBuildingIndex =
            editor.worldLayout.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::Building
                ? editor.worldLayout.selection.index
                : creative::kInvalidCreativeWorldLayoutIndex;
        const creative::CreativeObjectId duplicateObjectId =
            findGeneratedSourceScopeObject(
                appState.facade.document(), editor.worldLayout.source,
                creative::CreativeWorldLayoutTable::Building,
                duplicateBuildingIndex, sourceObjectKind);
        if (duplicateObjectId != creative::kInvalidObjectId) {
          static_cast<void>(appState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{&duplicateObjectId,
                                                          1U},
              duplicateObjectId));
        } else {
          editor.worldLayout.selection = {
              CreativeEditorWorldLayoutSelectionKind::Building,
              payload->buildingIndex};
        }
      }
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedBuildingGrounding: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutBuildingGroundingPayload>(command);
      if (payload == nullptr) {
        result.message = "generated building grounding: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated building grounding: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Building,
              payload->buildingIndex, payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated building grounding: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated building grounding: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingGroundingSettingsToDocument(
              editor.worldLayout, appState, payload->buildingIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateCapturePayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template capture: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          captureCreativeEditorWorldLayoutBuildingTemplate(
              editor.worldLayout, payload->buildingIndex, payload->label);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSyncPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template update: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(
              editor.worldLayout, payload->buildingIndex);
      result.accepted = receipt.accepted;
      result.changed =
          receipt.accepted &&
          receipt.reasonCode !=
              "creative_editor_world_layout_building_template_update_no_change";
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::
        WorldLayoutRefreshBuildingTemplateInstances: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSyncPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template refresh: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
              editor.worldLayout, payload->buildingIndex, payload->mode);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template selection: payload mismatch";
        break;
      }
      const CreativeEditorWorldLayoutEditReceipt receipt =
          selectCreativeEditorWorldLayoutBuildingTemplate(
              editor.worldLayout, payload->templateIndex);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout template placement: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
              editor.worldLayout, payload->phase, payload->point,
              payload->operation);
      const bool sourceChanged =
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit &&
          receipt.changed;
      const bool exactPreviewClosed =
          previewWasActive &&
          payload->phase ==
              CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin &&
          receipt.accepted;
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = sourceChanged;
      result.sceneChanged =
          exactPreviewClosed || (previewWasActive && sourceChanged);
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
