#include "EditorDesktopCommandsInternal.hpp"

#include "EditorTerrain.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorTerrainStampLibrary.hpp"
#include "EditorTransform.hpp"
#include "EditorWorldLayoutLifecycle.hpp"

#include <string>
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
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
        result.message = "terrain region unavailable in this workspace";
        break;
      }
      const CreativeEditorTerrainGenerationPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutTerrainRegion(
              editor.worldLayoutTopography.region, editor.terrainGeneration,
              activeAppState.facade.document());
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted;
      result.affectedObjectCount =
          editor.terrainGeneration.operationPreview.receipt.replay
              .modifiedCellCount;
      result.message = editor.worldLayoutTopography.region.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionApply: {
      const CreativeEditorTerrainGenerationApplyReceipt receipt =
          applyCreativeEditorWorldLayoutTerrainRegion(
              editor.worldLayoutTopography.region, editor.terrainGeneration,
              activeAppState);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.affectedObjectCount = receipt.operation.replay.outputCellCount;
      result.message = editor.worldLayoutTopography.region.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel:
      result.changed = cancelCreativeEditorWorldLayoutTerrainRegion(
          editor.worldLayoutTopography.region, editor.terrainGeneration);
      result.accepted = true;
      result.message = editor.worldLayoutTopography.region.statusMessage;
      break;
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
    case CreativeDesktopCommandId::TerrainStampSaveSelection: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
        result.message = "terrain stamps unavailable in this workspace";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainStampPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain stamp save: payload mismatch";
        break;
      }
      const CreativeEditorTerrainStampAssetReceipt receipt =
          saveCreativeEditorTerrainSelectionAsStamp(
              activeAppState, editor, payload->label);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.terrainStamps.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainStampSelect: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
        result.message = "terrain stamps unavailable in this workspace";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainStampPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain stamp select: payload mismatch";
        break;
      }
      const CreativeEditorTerrainStampAssetReceipt receipt =
          selectCreativeEditorTerrainStampAsset(
              activeAppState, editor, payload->assetId);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.accepted;
      result.message = editor.terrainStamps.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainStampDelete: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
        result.message = "terrain stamps unavailable in this workspace";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainStampPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain stamp delete: payload mismatch";
        break;
      }
      const CreativeEditorTerrainStampAssetReceipt receipt =
          removeCreativeEditorTerrainStampAsset(
              activeAppState, editor, payload->assetId);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.terrainStamps.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainStampRepairSource: {
      if (editor.assetEdit.active ||
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
        result.message = "terrain stamps unavailable in this workspace";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainStampPayload>(command);
      const creative::CreativeTerrainOperation* operation =
          payload == nullptr
              ? nullptr
              : creative::findCreativeTerrainOperation(
                    activeAppState.facade.document().terrainOperationStack(),
                    payload->operationId);
      if (payload == nullptr || operation == nullptr ||
          operation->kind != creative::CreativeTerrainOperationKind::Stamp) {
        result.message = "terrain stamp repair: source unavailable";
        break;
      }
      const CreativeEditorTerrainStampAssetReceipt receipt =
          repairCreativeEditorTerrainStampAssetSource(
              editor.terrainStamps, operation->stamp,
              payload->replaceExisting);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.message = editor.terrainStamps.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainOperationNew:
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      result.accepted = true;
      result.changed =
          beginNewCreativeEditorTerrainOperation(editor.terrainGeneration);
      editor.terrain.profile.editingOperationId =
          creative::kInvalidCreativeTerrainOperationId;
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
      const creative::CreativeTerrainOperation* operation =
          creative::findCreativeTerrainOperation(
              activeAppState.facade.document().terrainOperationStack(),
              payload->operationId);
      const bool selectable =
          operation != nullptr &&
          (operation->kind ==
               creative::CreativeTerrainOperationKind::GeneratedTerrain ||
           operation->kind == creative::CreativeTerrainOperationKind::Region ||
           (operation->kind ==
                creative::CreativeTerrainOperationKind::Profile &&
            operation->owner ==
                creative::CreativeTerrainOperationOwner::Manual));
      if (selectable && operation->kind ==
                            creative::CreativeTerrainOperationKind::Profile) {
        const CreativeEditorTerrainProfileReceipt selected =
            selectCreativeEditorTerrainProfileOperation(
                activeAppState.facade.document(), editor,
                payload->operationId);
        result.accepted = selected.accepted;
        result.changed = selected.changed;
        result.message = std::string(selected.reasonCode);
        break;
      }
      result.changed = selectable &&
                       (operation->kind ==
                                creative::CreativeTerrainOperationKind::Region
                            ? selectCreativeEditorWorldLayoutTerrainRegionOperation(
                                  editor.worldLayoutTopography.region,
                                  editor.terrainGeneration,
                                  activeAppState.facade.document(),
                                  payload->operationId)
                            : selectCreativeEditorTerrainOperation(
                                  editor.terrainGeneration,
                                  activeAppState.facade.document(),
                                  payload->operationId));
      result.accepted = selectable;
      if (result.accepted) {
        editor.terrain.profile.editingOperationId =
            creative::kInvalidCreativeTerrainOperationId;
      }
      result.message = editor.terrainGeneration.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::TerrainOperationTransform: {
      if (editor.transform.active) {
        result.message = "finish the current transform first";
        break;
      }
      if (editor.terrainGeneration.previewActive ||
          editor.terrain.profile.editingOperationId !=
              creative::kInvalidCreativeTerrainOperationId ||
          editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the active terrain edit first";
        break;
      }
      const auto* payload =
          payloadAs<CreativeDesktopTerrainOperationPayload>(command);
      if (payload == nullptr) {
        result.message = "terrain operation transform: payload mismatch";
        break;
      }
      result.accepted = beginCreativeEditorTerrainOperationTransformPreview(
          activeAppState, payload->operationId, editor.transform,
          "desktop_terrain_operation_transform");
      result.changed = result.accepted;
      result.message = result.accepted
                           ? "Terrain operation transform active"
                           : editor.transform.preflight.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::TerrainOperationSetEnabled:
    case CreativeDesktopCommandId::TerrainOperationMove:
    case CreativeDesktopCommandId::TerrainOperationDuplicate:
    case CreativeDesktopCommandId::TerrainOperationDelete:
    case CreativeDesktopCommandId::TerrainOperationBakeAll: {
      if (editor.worldLayoutTopography.region.editingEnabled ||
          editor.worldLayoutTopography.region.ownsPreview) {
        result.message = "finish the World Layout terrain region first";
        break;
      }
      const bool bakeAll =
          command.id == CreativeDesktopCommandId::TerrainOperationBakeAll;
      const auto* payload = bakeAll
                                ? nullptr
                                : payloadAs<CreativeDesktopTerrainOperationPayload>(
                                      command);
      if (!bakeAll && payload == nullptr) {
        result.message = "terrain operation edit: payload mismatch";
        break;
      }
      creative::CreativeTerrainOperationMutationRequest request;
      request.operationId =
          payload == nullptr ? creative::kInvalidCreativeTerrainOperationId
                             : payload->operationId;
      std::string_view source = "desktop_terrain_operation_edit";
      creative::CreativeTerrainOperationKind editedKind =
          creative::CreativeTerrainOperationKind::Count;
      if (bakeAll) {
        request.kind = creative::CreativeTerrainOperationMutationKind::BakeAll;
        source = "desktop_terrain_operation_bake_all";
      } else if (command.id ==
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
        editedKind = operation->kind;
        request.kind = creative::CreativeTerrainOperationMutationKind::Add;
        request.operationKind = operation->kind;
        request.generation = operation->generation;
        request.composition = operation->composition;
        request.region = operation->region;
        request.grade = operation->grade;
        request.profile = operation->profile;
        request.path = operation->path;
        request.stamp = operation->stamp;
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
      if (receipt.accepted) {
        if (bakeAll ||
            (request.kind ==
                 creative::CreativeTerrainOperationMutationKind::Remove &&
             editor.terrain.profile.editingOperationId ==
                 request.operationId)) {
          editor.terrain.profile.editingOperationId =
              creative::kInvalidCreativeTerrainOperationId;
        } else if (request.kind ==
                       creative::CreativeTerrainOperationMutationKind::Add &&
                   editedKind ==
                       creative::CreativeTerrainOperationKind::Profile) {
          const CreativeEditorTerrainProfileReceipt selected =
              selectCreativeEditorTerrainProfileOperation(
                  activeAppState.facade.document(), editor,
                  receipt.operation.operationId);
          if (!selected.accepted) {
            result.accepted = false;
            result.message = std::string(selected.reasonCode);
          }
        }
      }
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
