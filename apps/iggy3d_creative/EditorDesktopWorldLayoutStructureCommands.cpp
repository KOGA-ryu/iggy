#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutStructureCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutCanvasPoint: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutPointPayload>(command);
      if (payload == nullptr) {
        result.message = "layout point: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return applyCreativeEditorWorldLayoutPoint(
                    target, payload->point,
                    activeAppState.facade.document().gridSettings());
              },
              "desktop_world_layout_point_create");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutCanvasGesture: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutGesturePayload>(command);
      if (payload == nullptr) {
        result.message = "layout gesture: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, activeAppState, payload->phase,
              CreativeEditorWorldLayoutGesturePhase::Begin,
              CreativeEditorWorldLayoutGesturePhase::Update,
              CreativeEditorWorldLayoutGesturePhase::Commit,
              CreativeEditorWorldLayoutGesturePhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutGesturePhase phase) {
                return applyCreativeEditorWorldLayoutGesture(
                    target, phase, payload->point);
              },
              "desktop_world_layout_gesture_create",
              "plan creation preview ready in 3D",
              "plan element created in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetRoomSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutRoomSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutRoomSettings(
              editor.worldLayout, payload->roomIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedRoomSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated room settings: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated room settings: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Room, payload->roomIndex,
              payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated room settings: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated room settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutApplyReceipt receipt =
          applyCreativeEditorWorldLayoutRoomSettingsToDocument(
              editor.worldLayout, appState, payload->roomIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive || receipt.apply.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopGeneratedRoomSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "generated room preview: payload mismatch";
        break;
      }
      const creative::CreativeObject* object =
          appState.facade.findObject(payload->objectId);
      if (object == nullptr) {
        result.message = "generated room preview: target missing";
        break;
      }
      const GeneratedSourceScopeResolution scope =
          resolveGeneratedSourceScope(
              editor.worldLayout.source, *object,
              creative::CreativeWorldLayoutTable::Room, payload->roomIndex,
              payload->stableKey);
      if (!scope.ancestor) {
        result.message = "generated room preview: source mismatch";
        break;
      }
      if (!scope.stable) {
        result.message = "generated room preview: stale target";
        break;
      }
      const CreativeEditorWorldLayoutPreviewReceipt receipt =
          previewCreativeEditorWorldLayoutRoomSettings(
              editor.worldLayout, appState.facade.document(),
              payload->roomIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.sceneChanged = receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateRoom: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutRoomManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, appState, payload->phase,
              CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
              CreativeEditorWorldLayoutRoomManipulationPhase::Update,
              CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
              CreativeEditorWorldLayoutRoomManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutRoomManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutRoomManipulation(
                    target, phase, payload->point, payload->toleranceCells);
              },
              "desktop_world_layout_room_drag",
              "room drag preview ready in 3D", "room updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetVerticalConnectorSettings: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutVerticalConnectorSettingsPayload>(
          command);
      if (payload == nullptr) {
        result.message = "layout vertical connector settings: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutVerticalConnectorSettings(
              editor.worldLayout, payload->connectorIndex,
              payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
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
                    target, phase, payload->point, payload->toleranceCells);
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
    case CreativeDesktopCommandId::WorldLayoutRepairAsset: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutAssetRepairPayload>(command);
      if (payload == nullptr) {
        result.message = "layout asset repair: payload mismatch";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          repairWorldLayoutAsset(
              editor.worldLayout, editor.catalog.model,
              context.appState.facade.document().gridSettings().cellSizeMeters,
              *payload);
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
