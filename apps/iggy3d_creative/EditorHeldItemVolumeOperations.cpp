#include "EditorHeldItemWorldOperationsInternal.hpp"

#include "EditorConnectedFill.hpp"
#include "EditorInteractionInternal.hpp"
#include "EditorPattern.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void setVolumeFirstCorner(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First,
      editor.interaction.target.grid.targetCell));
  editor.volume.lastReceipt = {};
}

void setVolumeSecondCorner(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::Second,
      editor.interaction.target.grid.targetCell));
  editor.volume.lastReceipt = {};
}

void advanceVolumeSelection(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  static_cast<void>(cr::advanceCreativeVolumeSelection(
      editor.volume.selection,
      editor.interaction.target.grid.targetCell));
  editor.volume.lastReceipt = {};
}

void expandVolumeSelection(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  if (cr::expandCreativeVolumeSelectionToCell(
          editor.volume.selection,
          editor.interaction.target.grid.targetCell)) {
    editor.volume.lastReceipt = {};
  }
}

void applyHeldVolumeOperation(
    CreativeHeldItemWorldOperationContext& context) {
  const cr::CreativeVolumeOperationKind operation =
      cr::creativeVolumeOperationForHeldItem(context.held.kind);
  context.request.editor.volume.operation = operation;
  static_cast<void>(applyCreativeEditorVolumeOperationWithHistory(
      context.request.appState, context.request.editor.volume,
      context.request.editor.placeBrush, operation,
      context.request.editor.toolSettings,
      "minecraft_secondary_volume_apply"));
}

void setVolumeGestureFeedback(CreativeEditorState& editor,
                              bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex, editor.placeBrush);
}

void beginHeldShapeVolume(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorVolumeGestureReceipt receipt =
      stepCreativeEditorVolumeGesture(
          editor.volume, CreativeEditorVolumeGestureAction::Begin,
          editor.interaction.target.grid.valid,
          editor.interaction.target.grid.targetCell);
  if (receipt.accepted) {
    clearCreativeEditorPlacementFeedback(editor.interaction);
  } else {
    setVolumeGestureFeedback(editor, false);
  }
}

void completeHeldShapeVolume(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorVolumeGestureReceipt gesture =
      stepCreativeEditorVolumeGesture(
          editor.volume, CreativeEditorVolumeGestureAction::Commit,
          editor.interaction.target.grid.valid,
          editor.interaction.target.grid.targetCell);
  if (!gesture.accepted) {
    setVolumeGestureFeedback(editor, false);
    return;
  }
  clearCreativeEditorPlacementFeedback(editor.interaction);
}

void applyHeldShapeVolume(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const cr::CreativeVolumeOperationKind operation =
      cr::creativeVolumeOperationForHeldItem(context.held.kind);
  editor.volume.operation = operation;
  const cr::CreativeVolumeOperationReceipt receipt =
      applyCreativeEditorVolumeOperationWithHistory(
          context.request.appState, editor.volume, editor.placeBrush,
          operation, editor.toolSettings, "minecraft_shape_tool_commit");
  setVolumeGestureFeedback(editor, receipt.accepted);
}

void advanceHeldShapeVolume(CreativeHeldItemWorldOperationContext& context) {
  switch (context.request.editor.volume.selection.phase) {
    case cr::CreativeVolumeSelectionPhase::Empty:
      beginHeldShapeVolume(context);
      return;
    case cr::CreativeVolumeSelectionPhase::FirstCorner:
      completeHeldShapeVolume(context);
      return;
    case cr::CreativeVolumeSelectionPhase::Complete:
      if (context.request.editor.volume.lastReceipt.requested &&
          context.request.editor.volume.lastReceipt.accepted) {
        beginHeldShapeVolume(context);
        return;
      }
      applyHeldShapeVolume(context);
      return;
  }
}

void applyHeldArray(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorArrayWithHistory(
      context.request.appState, context.request.editor.pattern,
      context.request.editor.toolSettings,
      context.request.editor.placeCellSize,
      context.request.editor.interaction.target.grid.valid,
      context.request.editor.interaction.target.grid.placementAnchor,
      "minecraft_secondary_array"));
}

void acceptHeldArray(CreativeHeldItemWorldOperationContext& context) {
  const cr::CreativeSelectionState& selection =
      context.request.appState.facade.selectionState();
  const CreativeEditorWorldTarget& target =
      context.request.editor.interaction.target;
  const bool targetAlreadySelected =
      target.objectHit &&
      cr::selectionContainsTarget(
          selection, cr::TargetRef{static_cast<cr::Id>(target.objectId)});
  if (cr::selectedTargetList(selection).empty() ||
      (target.objectHit &&
       (!targetAlreadySelected ||
        context.request.modifiers != cr::kCreativeInputModifierNone))) {
    selectCreativeHeldItemTarget(context);
  } else {
    applyHeldArray(context);
  }
}

void paintConnectedFill(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorConnectedFillWithHistory(
      context.request.appState, context.request.editor,
      CreativeConnectedFillEditKind::Paint,
      "minecraft_connected_fill_paint"));
}

void eraseConnectedFill(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorConnectedFillWithHistory(
      context.request.appState, context.request.editor,
      CreativeConnectedFillEditKind::Erase,
      "minecraft_connected_fill_erase"));
}

void extrudeSurface(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorSurfaceExtrudeWithHistory(
      context.request.appState, context.request.editor,
      cr::CreativeSurfaceExtrudeKind::Extrude,
      "minecraft_surface_extrude"));
}

void insetSurface(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorSurfaceExtrudeWithHistory(
      context.request.appState, context.request.editor,
      cr::CreativeSurfaceExtrudeKind::Inset,
      "minecraft_surface_inset"));
}

}  // namespace

void advanceCreativeHeldItemVolumeSelection(
    CreativeHeldItemWorldOperationContext& context) {
  advanceVolumeSelection(context);
}

void executeCreativeHeldItemVolumeSurfaceOperation(
    cr::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context) {
  switch (operation) {
    case cr::CreativeHeldItemWorldOperation::SetVolumeFirstCorner:
      setVolumeFirstCorner(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SetVolumeSecondCorner:
      setVolumeSecondCorner(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ExpandVolumeSelection:
      expandVolumeSelection(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceVolumeSelection:
      advanceVolumeSelection(context);
      return;
    case cr::CreativeHeldItemWorldOperation::BeginShapeVolume:
    case cr::CreativeHeldItemWorldOperation::CommitShapeVolume:
    case cr::CreativeHeldItemWorldOperation::AdvanceShapeVolume:
      advanceHeldShapeVolume(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyVolumeOperation:
      applyHeldVolumeOperation(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyArray:
      applyHeldArray(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AcceptArray:
      acceptHeldArray(context);
      return;
    case cr::CreativeHeldItemWorldOperation::PaintConnectedFill:
      paintConnectedFill(context);
      return;
    case cr::CreativeHeldItemWorldOperation::EraseConnectedFill:
      eraseConnectedFill(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ExtrudeSurface:
      extrudeSurface(context);
      return;
    case cr::CreativeHeldItemWorldOperation::InsetSurface:
      insetSurface(context);
      return;
    default:
      return;
  }
}

}  // namespace iggy3d_creative_app
