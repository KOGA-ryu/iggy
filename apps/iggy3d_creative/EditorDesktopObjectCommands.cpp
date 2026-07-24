#include "EditorDesktopCommandsInternal.hpp"

#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorObjectActions.hpp"
#include "EditorPathEditing.hpp"
#include "EditorWorldLayout.hpp"

#include <span>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

bool dispatchCreativeDesktopObjectCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(editor, appState);
  const auto synchronizeSelection = [&]() {
    return synchronizeCreativeEditorWorldLayoutSelection(
        editor.worldLayout, activeAppState.facade.document(),
        activeAppState.facade.selectionState());
  };
  const auto applyObjectAction =
      [&](CreativeEditorObjectActionOutcome outcome) {
        result.objectAction = std::move(outcome);
        result.accepted =
            creativeEditorObjectActionOutcomeAccepted(result.objectAction);
        result.changed =
            creativeEditorObjectActionOutcomeChanged(result.objectAction);
        result.affectedObjectCount =
            result.objectAction.affectedObjectCount;
        result.message =
            formatCreativeEditorObjectActionOutcome(result.objectAction);
      };
  switch (command.id) {
    case CreativeDesktopCommandId::DuplicateSelection: {
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorDuplicateReceipt receipt =
          duplicateCreativeEditorSelectionWithUndo(
              activeAppState, activeAppState.history,
              creative::CreativeDuplicateCommandRequest{}, "desktop_duplicate",
              &editor.worldLayout);
      applyObjectAction(makeCreativeEditorObjectActionOutcome(
          creative::CreativeSemanticObjectAction::Duplicate,
          CreativeEditorObjectActionTarget::Selection, receipt.accepted,
          receipt.changed, receipt.affectedObjectCount,
          receipt.reasonCode));
      result.worldLayoutChanged = receipt.worldLayoutSourceDuplicated;
      result.sceneChanged =
          receipt.worldLayoutSourceDuplicated && previewWasActive;
      if (receipt.accepted && !receipt.worldLayoutSourceDuplicated) {
        static_cast<void>(synchronizeSelection());
      }
      break;
    }
    case CreativeDesktopCommandId::DeleteSelection: {
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorDeleteReceipt receipt =
          deleteCreativeEditorSelectionWithUndo(
              activeAppState, "desktop_delete", &activeAppState.history,
              &editor.worldLayout);
      applyObjectAction(makeCreativeEditorObjectActionOutcome(
          creative::CreativeSemanticObjectAction::Delete,
          CreativeEditorObjectActionTarget::Selection, receipt.accepted,
          receipt.changed, receipt.affectedObjectCount,
          receipt.reasonCode));
      result.worldLayoutChanged = receipt.worldLayoutSourceDeleted;
      result.sceneChanged = receipt.worldLayoutSourceDeleted && previewWasActive;
      if (receipt.accepted) {
        static_cast<void>(synchronizeSelection());
      }
      break;
    }
    case CreativeDesktopCommandId::SelectObjects: {
      const auto* payload = payloadAs<CreativeDesktopSelectPayload>(command);
      if (payload == nullptr) {
        result.message = "select: payload mismatch";
        break;
      }
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(payload->objectIds,
                                              payload->primaryObjectId);
      const CreativeEditorSelectionSynchronizationReceipt synchronized =
          receipt.accepted ? synchronizeSelection()
                           : CreativeEditorSelectionSynchronizationReceipt{};
      result.accepted = receipt.accepted && synchronized.accepted;
      result.changed = receipt.changed || synchronized.changed;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = receipt.accepted ? "selection updated"
                                        : std::string(receipt.message);
      break;
    }
    case CreativeDesktopCommandId::FocusObject: {
      const auto* payload = payloadAs<CreativeDesktopSelectPayload>(command);
      if (payload == nullptr) {
        result.message = "focus: payload mismatch";
        break;
      }
      const creative::CreativeObjectId objectId =
          payload->primaryObjectId != creative::kInvalidObjectId
              ? payload->primaryObjectId
              : payload->objectIds.empty() ? creative::kInvalidObjectId
                                           : payload->objectIds.front();
      const creative::CreativeObject* object =
          activeAppState.facade.findObject(objectId);
      if (object == nullptr || !focusEditorCameraOnObject(editor, *object)) {
        result.message = "focus: object unavailable";
        break;
      }
      const creative::CreativeSelectionReceipt receipt =
          activeAppState.facade.selectTargets(
              std::span<const creative::CreativeObjectId>{&objectId, 1U},
              objectId);
      const CreativeEditorSelectionSynchronizationReceipt synchronized =
          receipt.accepted ? synchronizeSelection()
                           : CreativeEditorSelectionSynchronizationReceipt{};
      result.accepted = receipt.accepted && synchronized.accepted;
      result.changed = true;
      result.affectedObjectCount = receipt.selectedCountAfter;
      result.message = "object focused";
      break;
    }
    case CreativeDesktopCommandId::FrameSelection3D: {
      const creative::CreativeSelectionState& selection =
          activeAppState.facade.selectionState();
      if (!focusEditorCameraOnSelection(
              editor, activeAppState.facade.document(), selection)) {
        result.message = "frame selection: no visible selection";
        break;
      }
      result.accepted = true;
      result.changed = true;
      result.affectedObjectCount = creative::selectedTargetCount(selection);
      result.message = "selection framed in 3D";
      break;
    }
    case CreativeDesktopCommandId::FrameAll3D: {
      const creative::CreativeDocument& document =
          activeAppState.facade.document();
      if (!focusEditorCameraOnDocument(editor, document)) {
        result.message = "frame all: no visible objects";
        break;
      }
      result.accepted = true;
      result.changed = true;
      result.affectedObjectCount = document.objectCount();
      result.message = "visible scene framed in 3D";
      break;
    }
    case CreativeDesktopCommandId::SetLogicSource: {
      const auto* payload = payloadAs<CreativeDesktopLogicLinkPayload>(command);
      if (payload == nullptr) {
        result.message = "logic source: payload mismatch";
        break;
      }
      const creative::CreativeObjectId sourceBefore =
          editor.logicLinks.sourceObjectId;
      const CreativeEditorLogicLinkReceipt receipt =
          selectCreativeEditorLogicLinkSource(
              activeAppState, editor.logicLinks, payload->sourceObjectId);
      result.accepted = receipt.accepted;
      result.changed = receipt.accepted &&
                       sourceBefore != editor.logicLinks.sourceObjectId;
      result.affectedObjectCount = receipt.accepted ? 1U : 0U;
      result.message = receipt.accepted
                           ? "logic source selected"
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::ClearLogicSource:
      result.accepted = true;
      result.changed =
          clearCreativeEditorLogicLinkSource(editor.logicLinks);
      result.message = result.changed ? "logic source cleared"
                                      : "logic source already clear";
      break;
    case CreativeDesktopCommandId::SetLogicLink: {
      const auto* payload = payloadAs<CreativeDesktopLogicLinkPayload>(command);
      if (payload == nullptr) {
        result.message = "set logic link: payload mismatch";
        break;
      }
      const CreativeEditorLogicLinkReceipt receipt =
          setCreativeEditorLogicLink(
              activeAppState, editor.logicLinks, payload->sourceObjectId,
              payload->targetObjectId, payload->action,
              "desktop_set_logic_link");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.accepted ? 1U : 0U;
      result.message = receipt.accepted
                           ? (receipt.changed ? "logic link updated"
                                              : "logic link unchanged")
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::RemoveLogicLink: {
      const auto* payload = payloadAs<CreativeDesktopLogicLinkPayload>(command);
      if (payload == nullptr) {
        result.message = "remove logic link: payload mismatch";
        break;
      }
      const CreativeEditorLogicLinkReceipt receipt =
          removeCreativeEditorLogicLink(
              activeAppState, editor.logicLinks, payload->sourceObjectId,
              payload->targetObjectId, "desktop_remove_logic_link");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.changed ? 1U : 0U;
      result.message = receipt.accepted
                           ? "logic link removed"
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::RenameObject: {
      const auto* payload = payloadAs<CreativeDesktopRenamePayload>(command);
      if (payload == nullptr) {
        applyObjectAction(rejectCreativeEditorObjectAction(
            creative::CreativeSemanticObjectAction::Rename,
            CreativeEditorObjectActionTarget::Object,
            CreativeEditorObjectActionOutcomeStatus::PayloadMismatch,
            "creative_desktop_rename_payload_mismatch"));
        break;
      }
      if (payload->objectId == creative::kInvalidObjectId ||
          payload->name.empty()) {
        applyObjectAction(rejectCreativeEditorObjectAction(
            creative::CreativeSemanticObjectAction::Rename,
            CreativeEditorObjectActionTarget::Object,
            CreativeEditorObjectActionOutcomeStatus::InvalidRequest,
            "creative_desktop_rename_invalid_request"));
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorSemanticEditReceipt receipt =
          renameCreativeEditorObjectWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->name, "desktop_rename", &editor.worldLayout);
      applyObjectAction(makeCreativeEditorObjectActionOutcome(
          creative::CreativeSemanticObjectAction::Rename,
          CreativeEditorObjectActionTarget::Object, receipt.accepted,
          receipt.changed, receipt.affectedObjectCount,
          receipt.reasonCode));
      result.worldLayoutChanged = receipt.worldLayoutSourceChanged;
      result.sceneChanged =
          receipt.worldLayoutSourceChanged && previewWasActive;
      break;
    }
    case CreativeDesktopCommandId::SetObjectsVisible: {
      const auto* payload = payloadAs<CreativeDesktopObjectFlagPayload>(command);
      if (payload == nullptr) {
        applyObjectAction(rejectCreativeEditorObjectAction(
            creative::CreativeSemanticObjectAction::SetVisible,
            CreativeEditorObjectActionTarget::Objects,
            CreativeEditorObjectActionOutcomeStatus::PayloadMismatch,
            "creative_desktop_visibility_payload_mismatch"));
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorObjectsVisibleWithUndo(
              activeAppState, activeAppState.history, payload->objectIds,
              payload->value, "desktop_set_visible", &editor.worldLayout);
      applyObjectAction(makeCreativeEditorObjectActionOutcome(
          creative::CreativeSemanticObjectAction::SetVisible,
          CreativeEditorObjectActionTarget::Objects, receipt.accepted,
          receipt.changed, receipt.affectedObjectCount,
          receipt.reasonCode));
      result.worldLayoutChanged = receipt.worldLayoutSourceChanged;
      result.sceneChanged =
          receipt.worldLayoutSourceChanged && previewWasActive;
      break;
    }
    case CreativeDesktopCommandId::SetObjectsLocked: {
      const auto* payload = payloadAs<CreativeDesktopObjectFlagPayload>(command);
      if (payload == nullptr) {
        applyObjectAction(rejectCreativeEditorObjectAction(
            creative::CreativeSemanticObjectAction::SetLocked,
            CreativeEditorObjectActionTarget::Objects,
            CreativeEditorObjectActionOutcomeStatus::PayloadMismatch,
            "creative_desktop_lock_payload_mismatch"));
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorObjectsLockedWithUndo(
              activeAppState, activeAppState.history, payload->objectIds,
              payload->value, "desktop_set_locked", &editor.worldLayout);
      applyObjectAction(makeCreativeEditorObjectActionOutcome(
          creative::CreativeSemanticObjectAction::SetLocked,
          CreativeEditorObjectActionTarget::Objects, receipt.accepted,
          receipt.changed, receipt.affectedObjectCount,
          receipt.reasonCode));
      break;
    }
    case CreativeDesktopCommandId::SetObjectTransform: {
      const auto* payload = payloadAs<CreativeDesktopTransformPayload>(command);
      if (payload == nullptr) {
        applyObjectAction(rejectCreativeEditorObjectAction(
            creative::CreativeSemanticObjectAction::SetTransform,
            CreativeEditorObjectActionTarget::Object,
            CreativeEditorObjectActionOutcomeStatus::PayloadMismatch,
            "creative_desktop_transform_payload_mismatch"));
        break;
      }
      const creative::CreativeObject* object =
          activeAppState.facade.findObject(payload->objectId);
      if (object != nullptr &&
          creative::creativeObjectIsHierarchyContainer(object->kind)) {
        applyObjectAction(rejectCreativeEditorObjectAction(
            creative::CreativeSemanticObjectAction::SetTransform,
            CreativeEditorObjectActionTarget::Object,
            CreativeEditorObjectActionOutcomeStatus::InvalidRequest,
            "creative_editor_transform_container_requires_selection_transform"));
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorObjectTransformWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->transform, payload->setPosition, payload->setRotation,
              payload->setScale, "desktop_set_transform",
              &editor.worldLayout);
      applyObjectAction(makeCreativeEditorObjectActionOutcome(
          creative::CreativeSemanticObjectAction::SetTransform,
          CreativeEditorObjectActionTarget::Object, receipt.accepted,
          receipt.changed, receipt.affectedObjectCount,
          receipt.reasonCode, receipt.requiresAdoption));
      break;
    }
    case CreativeDesktopCommandId::SetGroupPivot: {
      const auto* payload = payloadAs<CreativeDesktopGroupPivotPayload>(command);
      if (payload == nullptr) {
        result.message = "group pivot: payload mismatch";
        break;
      }
      const creative::CreativeGroupPivotReceipt receipt =
          setCreativeEditorGroupPivotWithHistory(
              activeAppState, payload->groupObjectId, payload->pivot,
              "desktop_set_group_pivot");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.changed ? 1U : 0U;
      result.message = receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SetMovingPlatformSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopMovingPlatformPayload>(command);
      if (payload == nullptr) {
        result.message = "moving platform settings: payload mismatch";
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorMovingPlatformSettingsWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->settings, "desktop_set_moving_platform_settings",
              &editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = result.accepted
                           ? (result.changed ? "platform settings updated"
                                             : "platform settings unchanged")
                           : receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SetPlayerSpawnSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopPlayerSpawnPayload>(command);
      if (payload == nullptr) {
        result.message = "player spawn settings: payload mismatch";
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorPlayerSpawnSettingsWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->settings, "desktop_set_player_spawn_settings",
              &editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = result.accepted
                           ? (result.changed ? "player spawn settings updated"
                                             : "player spawn settings unchanged")
                           : receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SetNpcSpawnSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopNpcSpawnPayload>(command);
      if (payload == nullptr) {
        result.message = "npc spawn settings: payload mismatch";
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorNpcSpawnSettingsWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->settings, "desktop_set_npc_spawn_settings",
              &editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message = result.accepted
                           ? (result.changed ? "npc spawn settings updated"
                                             : "npc spawn settings unchanged")
                           : receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SetLootPointSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopLootPointPayload>(command);
      if (payload == nullptr) {
        result.message = "loot point settings: payload mismatch";
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorLootPointSettingsWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->settings, "desktop_set_loot_point_settings",
              &editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message =
          result.accepted
              ? (result.changed ? "loot point settings updated"
                                : "loot point settings unchanged")
              : receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::SetExitPointSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopExitPointPayload>(command);
      if (payload == nullptr) {
        result.message = "exit point settings: payload mismatch";
        break;
      }
      const CreativeEditorSemanticEditReceipt receipt =
          setCreativeEditorExitPointSettingsWithUndo(
              activeAppState, activeAppState.history, payload->objectId,
              payload->settings, "desktop_set_exit_point_settings",
              &editor.worldLayout);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.affectedObjectCount;
      result.message =
          result.accepted
              ? (result.changed ? "exit point settings updated"
                                : "exit point settings unchanged")
              : receipt.reasonCode;
      break;
    }
    case CreativeDesktopCommandId::ToggleMovingPlatformPreview:
    case CreativeDesktopCommandId::RestartMovingPlatformPreview:
    case CreativeDesktopCommandId::SeekMovingPlatformPreview: {
      const auto* payload =
          payloadAs<CreativeDesktopMovingPlatformPreviewPayload>(command);
      if (payload == nullptr) {
        result.message = "moving platform preview: payload mismatch";
        break;
      }
      CreativeMovingPlatformPreviewCommand previewCommand =
          CreativeMovingPlatformPreviewCommand::TogglePlayback;
      if (command.id ==
          CreativeDesktopCommandId::RestartMovingPlatformPreview) {
        previewCommand = CreativeMovingPlatformPreviewCommand::Restart;
      } else if (command.id ==
                 CreativeDesktopCommandId::SeekMovingPlatformPreview) {
        previewCommand = CreativeMovingPlatformPreviewCommand::Seek;
      }
      const CreativeMovingPlatformPreviewReceipt receipt =
          applyCreativeMovingPlatformPreviewCommand(
              editor.movingPlatformPreview, previewCommand,
              payload->objectId, payload->normalizedProgress);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.accepted ? 1U : 0U;
      result.message = receipt.accepted
                           ? std::string(editor.movingPlatformPreview.reasonCode)
                           : std::string(receipt.reasonCode);
      break;
    }
    case CreativeDesktopCommandId::SelectMovingPlatformWaypoint:
    case CreativeDesktopCommandId::SetMovingPlatformWaypointDwell: {
      const auto* payload =
          payloadAs<CreativeDesktopMovingPlatformWaypointPayload>(command);
      if (payload == nullptr) {
        result.message = "moving platform waypoint: payload mismatch";
        break;
      }
      syncCreativeMovingPlatformPathEditState(
          activeAppState, editor.interaction.movingPlatformPathEdit);
      if (editor.interaction.movingPlatformPathEdit.objectId !=
          payload->objectId) {
        result.message = "moving platform waypoint: target mismatch";
        break;
      }
      if (command.id ==
          CreativeDesktopCommandId::SelectMovingPlatformWaypoint) {
        result.accepted = selectCreativeMovingPlatformPathPoint(
            editor.interaction.movingPlatformPathEdit, payload->pointIndex);
        result.changed = result.accepted;
        result.message = result.accepted
                             ? "moving platform waypoint selected"
                             : "moving platform waypoint selection rejected";
        break;
      }
      const CreativeMovingPlatformPathEditReceipt receipt =
          setCreativeMovingPlatformWaypointDwellWithUndo(
              activeAppState, payload->objectId, payload->pointIndex,
              payload->dwellSeconds, "desktop_inspector_waypoint_dwell");
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.affectedObjectCount = receipt.changed ? 1U : 0U;
      result.message = std::string(receipt.reasonCode);
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
