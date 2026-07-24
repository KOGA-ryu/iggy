#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include "EditorWorldLayoutPlan.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopWorldLayoutPlanCommand(
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
      CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return applyCreativeEditorWorldLayoutPoint(
                    target, payload->point,
                    activeAppState.facade.document().gridSettings());
              },
              "desktop_world_layout_point_create");
      if (liveEdit.accepted && editor.worldLayout.terrainPathDraft.active) {
        const CreativeEditorWorldLayoutPreviewReceipt preview =
            previewCreativeEditorWorldLayoutTerrainPathDraft(
                editor.worldLayout, activeAppState.facade.document());
        liveEdit.sceneChanged = liveEdit.sceneChanged || preview.changed;
      }
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
                    target, phase, payload->point,
                    activeAppState.facade.document().gridSettings());
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
    case CreativeDesktopCommandId::WorldLayoutSplitRoom: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutRoomSplitPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room split: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return splitCreativeEditorWorldLayoutRoom(
                    target, payload->roomIndex, payload->axis,
                    payload->coordinate);
              },
              "desktop_world_layout_room_split");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutMergeRooms: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutRoomMergePayload>(command);
      if (payload == nullptr) {
        result.message = "layout room merge: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return mergeCreativeEditorWorldLayoutRooms(
                    target, payload->primaryRoomIndex,
                    payload->secondaryRoomIndex);
              },
              "desktop_world_layout_room_merge");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSplitWall: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutWallSplitPayload>(command);
      if (payload == nullptr) {
        result.message = "layout wall split: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return splitCreativeEditorWorldLayoutWall(
                    target, payload->topologyEdgeIndex,
                    payload->offsetCells);
              },
              "desktop_world_layout_wall_split");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutMergeWalls: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutWallMergePayload>(command);
      if (payload == nullptr) {
        result.message = "layout wall merge: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutImmediateEdit(
              editor.worldLayout, activeAppState,
              [&](CreativeEditorWorldLayoutState& target) {
                return mergeCreativeEditorWorldLayoutWalls(
                    target, payload->primaryTopologyEdgeIndex,
                    payload->secondaryTopologyEdgeIndex);
              },
              "desktop_world_layout_wall_merge");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
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
    case CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room boundary manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, activeAppState, payload->phase,
              CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
              CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update,
              CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Commit,
              CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutRoomBoundaryManipulationPhase
                      phase) {
                return applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
                    target, phase, payload->point,
                    payload->toleranceCells);
              },
              "desktop_world_layout_room_boundary_drag",
              "room boundary preview ready in 3D",
              "room boundary updated in 3D");
      result.accepted = liveEdit.accepted;
      result.changed = liveEdit.changed;
      result.worldLayoutChanged = liveEdit.worldLayoutChanged;
      result.sceneChanged = liveEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner: {
      const auto* payload = payloadAs<
          CreativeDesktopWorldLayoutRoomCornerManipulationPayload>(command);
      if (payload == nullptr) {
        result.message = "layout room corner manipulation: payload mismatch";
        break;
      }
      const CreativeDesktopWorldLayoutLiveEditResult liveEdit =
          dispatchCreativeDesktopWorldLayoutLiveEdit(
              editor.worldLayout, activeAppState, payload->phase,
              CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin,
              CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update,
              CreativeEditorWorldLayoutRoomCornerManipulationPhase::Commit,
              CreativeEditorWorldLayoutRoomCornerManipulationPhase::Cancel,
              [&](CreativeEditorWorldLayoutState& target,
                  CreativeEditorWorldLayoutRoomCornerManipulationPhase phase) {
                return applyCreativeEditorWorldLayoutRoomCornerManipulation(
                    target, phase, payload->point,
                    payload->toleranceCells);
              },
              "desktop_world_layout_room_corner_drag",
              "room corner preview ready in 3D",
              "room corner updated in 3D");
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
