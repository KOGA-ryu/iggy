#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutWallOpeningCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutSetWallSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutWallSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout partition settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutWallSettings(
              editor.worldLayout, payload->wallIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedWallSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated partition settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated partition settings: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Wall) {
        result.message = "generated partition settings: source mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutWallSettingsToDocument(
              editor.worldLayout, appState, provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedWallSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedWallSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated partition preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated partition preview: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Wall) {
        result.message = "generated partition preview: source mismatch";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutWallSettings(
              editor.worldLayout, appState.facade.document(),
              provenance.index, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateWall: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutWallManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout partition manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutWallManipulationPhase::Begin,
              CreativeEditorWorldLayoutWallManipulationPhase::Update,
              CreativeEditorWorldLayoutWallManipulationPhase::Commit,
              CreativeEditorWorldLayoutWallManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutWallManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutWallManipulation(
                    target, phase, payload->point, payload->toleranceCells);
              },
              "desktop_world_layout_partition_drag",
              "partition drag preview ready in 3D",
              "partition updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetOpeningSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutOpeningSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout opening settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutOpeningSettings(
              editor.worldLayout, payload->openingIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedOpeningSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated opening settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated opening settings: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Opening) {
        result.message = "generated opening settings: source mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutOpeningSettingsToDocument(
              editor.worldLayout, appState, provenance.index,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedOpeningSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedOpeningSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated opening preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated opening preview: target missing";
        break;
      }
      const creative::CreativeWorldLayoutObjectProvenance provenance =
          creative::resolveCreativeWorldLayoutObjectProvenance(
              editor.worldLayout.source, *object);
      if (!provenance.owned ||
          provenance.table != creative::CreativeWorldLayoutTable::Opening) {
        result.message = "generated opening preview: source mismatch";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutOpeningSettings(
              editor.worldLayout, appState.facade.document(),
              provenance.index, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetOpeningInsert: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutOpeningInsertPayload>(command);
      if (payload == nullptr) {
        result.message = "layout opening insert: payload mismatch";
        break;
      }
      CreativeEditorWorldLayoutOpeningInsertRequest request;
      request.openingIndex = payload->openingIndex;
      request.operation = payload->operation;
      request.assetScale = payload->assetScale;
      request.gridCellSizeMeters =
          context.appState.facade.document().gridSettings().cellSizeMeters;
      if (payload->operation !=
          CreativeEditorWorldLayoutOpeningInsertOperation::
              UseProceduralInsert) {
        const creative::CreativeCatalogEntry* found =
            findCatalogAsset(editor.catalog.model, payload->assetId);
        if (found == nullptr ||
            !creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
                found->assetAuthoringMetadata.categoryId)) {
          result.message = "layout opening insert: catalog entry missing";
          break;
        }
        request.assetId = creative::creativeHotbarAssetId(found->hotbarEntry);
        request.assetSourceBoundsMeters =
            found->hotbarEntry.assetSourceBounds;
        request.assetKind = found->assetAuthoringMetadata.categoryId ==
                                    "window"
                                ? creative::CreativeBuildingOpeningKind::Window
                                : creative::CreativeBuildingOpeningKind::Door;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          applyCreativeEditorWorldLayoutOpeningInsert(editor.worldLayout,
                                                      request);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateOpening: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutOpeningManipulationPayload>(
              command);
      if (payload == nullptr) {
        result.message = "layout opening manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
              CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
              CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
              CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutOpeningManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutOpeningManipulation(
                    target, phase, payload->point, payload->toleranceCells);
              },
              "desktop_world_layout_opening_drag",
              "opening drag preview ready in 3D", "opening updated in 3D");
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
