#include "EditorInteraction.hpp"

#include <string_view>

#include "EditorConnectedFill.hpp"
#include "EditorGroup.hpp"
#include "EditorPattern.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

struct HeldItemCommandResult {
  bool handled = false;
  bool changed = false;
};

HeldItemCommandResult confirmArrayCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorArrayWithHistory(
                    appState, editor.pattern, editor.toolSettings,
                    editor.placeCellSize,
                    editor.interaction.target.grid.valid,
                    editor.interaction.target.grid.placementAnchor, source)};
}

HeldItemCommandResult confirmConnectedFillCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorConnectedFillWithHistory(
                    appState, editor, CreativeConnectedFillEditKind::Paint,
                    source)
                    .accepted};
}

HeldItemCommandResult confirmSurfaceExtrudeCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorSurfaceExtrudeWithHistory(
                    appState, editor,
                    cr::CreativeSurfaceExtrudeKind::Extrude, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainControlCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainEditWithHistory(
                    appState, editor,
                    CreativeEditorTerrainEditKind::Upsert, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainGradeCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainGradeWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainSculptCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainSculptWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainProfileCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainProfileWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainPathCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainPathWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainRegionCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  if (editor.terrain.region.stamp.active) {
    const CreativeEditorTerrainStampReceipt receipt =
        applyCreativeEditorTerrainStampWithHistory(appState, editor, source);
    return {true, receipt.accepted};
  }
  return {true, applyCreativeEditorTerrainRegionWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmVolumeCommand(
    cr::CreativeHeldItemKind kind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  const cr::CreativeVolumeOperationReceipt receipt =
      applyCreativeEditorVolumeOperationWithHistory(
          appState, editor.volume, editor.placeBrush,
          cr::creativeVolumeOperationForHeldItem(kind), editor.toolSettings,
          source);
  return {true, receipt.accepted};
}

HeldItemCommandResult confirmGroupCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState&,
    std::string_view source) {
  const cr::CreativeGroupCommandReceipt receipt =
      applyCreativeEditorGroupCommandWithHistory(appState, source);
  return {true, receipt.accepted && receipt.changed};
}

HeldItemCommandResult cancelTerrainControlCommand(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  if (!editor.terrain.selectionValid) {
    return {};
  }
  return {true, applyCreativeEditorTerrainEditWithHistory(
                    appState, editor, CreativeEditorTerrainEditKind::Remove,
                    "creative_terrain_cancel_active_tool")
                    .accepted};
}

HeldItemCommandResult cancelTerrainGradeCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  return {true, cancelCreativeEditorTerrainGrade(editor).accepted};
}

HeldItemCommandResult cancelTerrainSculptCommand(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  finalizeCreativeTerrainSculptStroke(
      appState, editor, "creative_terrain_sculpt_cancel_active_tool");
  return {true, cancelCreativeEditorTerrainSculpt(editor).accepted};
}

HeldItemCommandResult cancelTerrainProfileCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  return {true, unlockCreativeEditorTerrainProfileBase(editor).accepted};
}

HeldItemCommandResult cancelTerrainPathCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  return {true, removeCreativeEditorTerrainPathPoint(editor).accepted};
}

HeldItemCommandResult cancelTerrainRegionCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  if (editor.terrain.region.stamp.active) {
    return {true, cancelCreativeEditorTerrainStamp(editor).changed};
  }
  return {true, cancelCreativeEditorTerrainRegion(editor).changed};
}

HeldItemCommandResult dispatchHeldItemCommand(
    cr::CreativeHeldItemCommandOperation operation,
    cr::CreativeHeldItemKind kind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  switch (operation) {
    case cr::CreativeHeldItemCommandOperation::None:
      return {};
    case cr::CreativeHeldItemCommandOperation::ConfirmArray:
      return confirmArrayCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmConnectedFill:
      return confirmConnectedFillCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmSurfaceExtrude:
      return confirmSurfaceExtrudeCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainControl:
      return confirmTerrainControlCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainGrade:
      return confirmTerrainGradeCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainSculpt:
      return confirmTerrainSculptCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainProfile:
      return confirmTerrainProfileCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainPath:
      return confirmTerrainPathCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainRegion:
      return confirmTerrainRegionCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmVolume:
      return confirmVolumeCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmGroup:
      return confirmGroupCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainControl:
      return cancelTerrainControlCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainGrade:
      return cancelTerrainGradeCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainSculpt:
      return cancelTerrainSculptCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainProfile:
      return cancelTerrainProfileCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainPath:
      return cancelTerrainPathCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainRegion:
      return cancelTerrainRegionCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::Count:
      return {};
  }
  return {};
}

}  // namespace

bool confirmCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                   CreativeEditorState& editor,
                                   std::string_view source) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  return dispatchHeldItemCommand(definition.confirmCommand, held.kind,
                                 appState, editor, source)
      .changed;
}

bool cancelCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                  CreativeEditorState& editor) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  const HeldItemCommandResult command = dispatchHeldItemCommand(
      definition.cancelCommand, held.kind, appState, editor, {});
  if (command.handled) {
    return command.changed;
  }
  if (editor.volume.active &&
      editor.volume.selection.phase != cr::CreativeVolumeSelectionPhase::Empty) {
    cr::clearCreativeVolumeSelection(editor.volume.selection);
    editor.volume.lastReceipt = {};
    return true;
  }
  cr::CreativeToolInputPacket cancel;
  cancel.kind = cr::CreativeToolInputKind::Cancel;
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      appState.facade.dispatchToolInput(cancel);
  editor.interaction.moveTargetId = cr::kInvalidObjectId;
  return receipt.changed;
}

}  // namespace iggy3d_creative_app
