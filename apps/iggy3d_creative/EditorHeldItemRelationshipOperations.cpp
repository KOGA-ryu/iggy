#include "EditorHeldItemWorldOperationsInternal.hpp"

#include "EditorGroup.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorMeasurement.hpp"
#include "EditorRoomPlacement.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void applyObjectGroup(CreativeHeldItemWorldOperationContext& context) {
  if (cr::selectedTargetCount(
          context.request.appState.facade.selectionState()) == 0U &&
      context.request.editor.interaction.target.objectHit) {
    static_cast<void>(context.request.appState.facade.dispatchToolInput(
        creativeHeldItemSelectionPacket(
            context.request.editor.interaction.target,
            cr::kCreativeToolModifierNone)));
  }
  static_cast<void>(applyCreativeEditorGroupCommandWithHistory(
      context.request.appState, "creative_group_world_action"));
}

void advanceLogicLink(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.objectHit) {
    editor.logicLinks.status =
        CreativeEditorLogicLinkStatus::InvalidTarget;
    return;
  }
  static_cast<void>(advanceCreativeEditorLogicLink(
      context.request.appState, editor.logicLinks,
      editor.interaction.target.objectId,
      "creative_logic_link_world_action"));
}

void clearLogicLinkSource(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      clearCreativeEditorLogicLinkSource(context.request.editor.logicLinks));
}

void advanceBuildingRoom(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(advanceCreativeEditorRoomPlacement(
      context.request.appState, context.request.editor,
      "creative_viewport_room_apply"));
}

void cancelBuildingRoom(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      cancelCreativeEditorRoomPlacement(context.request.editor));
}

void appendMeasurementPoint(
    CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      appendCreativeEditorMeasurementPoint(context.request));
}

void completeMeasurement(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(completeCreativeEditorMeasurement(context.request));
}

void cancelMeasurement(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(cancelCreativeEditorMeasurement(context.request));
}

}  // namespace

void executeCreativeHeldItemRelationshipOperation(
    cr::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context) {
  switch (operation) {
    case cr::CreativeHeldItemWorldOperation::ApplyObjectGroup:
      applyObjectGroup(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceLogicLink:
      advanceLogicLink(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ClearLogicLinkSource:
      clearLogicLinkSource(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceBuildingRoom:
      advanceBuildingRoom(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelBuildingRoom:
      cancelBuildingRoom(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AppendMeasurementPoint:
      appendMeasurementPoint(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CompleteMeasurement:
      completeMeasurement(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelMeasurement:
      cancelMeasurement(context);
      return;
    default:
      return;
  }
}

}  // namespace iggy3d_creative_app
