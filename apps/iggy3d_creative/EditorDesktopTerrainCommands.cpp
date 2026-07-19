#include "EditorDesktopCommandsInternal.hpp"

#include "EditorTerrainGeneration.hpp"

#include <string_view>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopTerrainCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  switch (command.id) {
    case CreativeDesktopCommandId::TerrainGenerationPreview:
    case CreativeDesktopCommandId::TerrainGenerationRegenerate: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout) ||
          editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "terrain generator unavailable in this workspace";
        break;
      }
      const bool wasActive = editor.terrainGeneration.previewActive;
      const bool regenerate =
          command.id == CreativeDesktopCommandId::TerrainGenerationRegenerate;
      const CreativeEditorTerrainGenerationPreviewReceipt receipt =
          previewCreativeEditorTerrainGeneration(
              editor.terrainGeneration, activeAppState.facade.document(),
              regenerate);
      if (receipt.accepted && !wasActive) {
        static_cast<void>(focusEditorCameraOnTerrainGeneration(
            editor, activeAppState.facade.document(),
            editor.terrainGeneration.generation));
      }
      result.accepted = receipt.accepted;
      result.changed = true;
      result.affectedObjectCount =
          editor.terrainGeneration.operationPreview.receipt.replay
              .modifiedCellCount;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainGenerationApply: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout) ||
          editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "terrain generator unavailable in this workspace";
        break;
      }
      const CreativeEditorTerrainGenerationApplyReceipt receipt =
          applyCreativeEditorTerrainGeneration(activeAppState,
                                               editor.terrainGeneration);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.operation.replay.outputCellCount;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainGenerationCancel:
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "cancel the terrain region from World Layout";
        break;
      }
      result.changed = cancelCreativeEditorTerrainGeneration(
          editor.terrainGeneration, "Terrain preview canceled");
      result.accepted = true;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    case CreativeDesktopCommandId::TerrainOperationNew:
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      result.accepted = true;
      result.changed =
          beginNewCreativeEditorTerrainOperation(editor.terrainGeneration);
      result.message = editor.terrainGeneration.statusMessage;
      break;
    case CreativeDesktopCommandId::TerrainOperationSelect: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain operation select: payload mismatch";
        break;
      }
      const bool exists = creative::findCreativeTerrainOperation(
                              activeAppState.facade.document()
                                  .terrainOperationStack(),
                              payload->operationId) != nullptr;
      result.changed = exists && selectCreativeEditorTerrainOperation(
                                     editor.terrainGeneration,
                                     activeAppState.facade.document(),
                                     payload->operationId);
      result.accepted = exists;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainOperationSetEnabled:
    case CreativeDesktopCommandId::TerrainOperationMove:
    case CreativeDesktopCommandId::TerrainOperationDuplicate:
    case CreativeDesktopCommandId::TerrainOperationDelete: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain operation edit: payload mismatch";
        break;
      }
      creative::CreativeTerrainOperationMutationRequest request;
      request.operationId = payload->operationId;
      std::string_view source = "desktop_terrain_operation_edit";
      if (command.id ==
          CreativeDesktopCommandId::TerrainOperationSetEnabled) {
        request.kind =
            creative::CreativeTerrainOperationMutationKind::SetEnabled;
        request.enabled = payload->enabled;
        source = "desktop_terrain_operation_enable";
      } else if (command.id ==
                 CreativeDesktopCommandId::TerrainOperationMove) {
        request.kind = creative::CreativeTerrainOperationMutationKind::Move;
        request.targetIndex = payload->targetIndex;
        source = "desktop_terrain_operation_move";
      } else if (command.id ==
                 CreativeDesktopCommandId::TerrainOperationDelete) {
        request.kind = creative::CreativeTerrainOperationMutationKind::Remove;
        source = "desktop_terrain_operation_delete";
      } else {
        const creative::CreativeTerrainOperation* operation =
            creative::findCreativeTerrainOperation(
                activeAppState.facade.document().terrainOperationStack(),
                payload->operationId);
        if (operation == nullptr) {
          result.message = "terrain operation duplicate: source unavailable";
          break;
        }
        request.kind = creative::CreativeTerrainOperationMutationKind::Add;
        request.generation = operation->generation;
        request.composition = operation->composition;
        request.enabled = operation->enabled;
        source = "desktop_terrain_operation_duplicate";
      }
      const CreativeEditorTerrainOperationEditReceipt receipt =
          editCreativeEditorTerrainOperation(
              activeAppState, editor.terrainGeneration, request, source);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.operation.replay.outputCellCount;
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
