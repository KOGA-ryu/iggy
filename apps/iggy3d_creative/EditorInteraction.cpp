#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <algorithm>
#include <array>

#include "EditorGroup.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

using ContinuousGestureFinalizer = void (*)(
    cr::CreativeAppState&, CreativeEditorState&, std::string_view);

struct ContinuousGestureFinalizerRow {
  CreativeEditorContinuousGestureOwner owner =
      CreativeEditorContinuousGestureOwner::None;
  ContinuousGestureFinalizer finalize = nullptr;
};

constexpr std::array kContinuousGestureFinalizers{
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::Material,
        finalizeCreativeMaterialStroke},
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::AuthoredAsset,
        finalizeCreativeAuthoredAssetStroke},
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::AssetScatter,
        finalizeCreativeAssetScatterStroke},
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::TerrainControl,
        finalizeCreativeTerrainStroke},
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::TerrainPaint,
        finalizeCreativeEditorTerrainPaintStroke},
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::TerrainSculpt,
        finalizeCreativeTerrainSculptStroke},
};

}  // namespace

void clearCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction) noexcept {
  interaction.placementFeedback = {};
}

void setCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    CreativeEditorPlacementFeedbackStatus status,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    cr::CreativeObjectId objectId) noexcept {
  interaction.placementFeedback = {};
  interaction.placementFeedback.status = status;
  interaction.placementFeedback.objectId = objectId;
  interaction.placementFeedback.objectKind = objectKind;
  interaction.placementFeedback.frameIndex = frameIndex;
}

void setCreativeEditorVoxelPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    cr::CreativeGridCoord3 voxelCell,
    cr::CreativeBounds voxelBounds) noexcept {
  setCreativeEditorPlacementFeedback(
      interaction, CreativeEditorPlacementFeedbackStatus::Placed, frameIndex,
      objectKind);
  interaction.placementFeedback.voxelPlaced = true;
  interaction.placementFeedback.voxelCell = voxelCell;
  interaction.placementFeedback.voxelBounds = voxelBounds;
}

void resetCreativeMaterialBrushPivot(
    CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId) noexcept {
  state = {};
  state.documentId = documentId;
}

void synchronizeCreativeMaterialBrushPivotDocument(
    CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId) noexcept {
  if (state.documentId != documentId) {
    resetCreativeMaterialBrushPivot(state, documentId);
  }
}

void updateCreativeMaterialBrushPivotAim(
    CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId,
    bool aimAvailable,
    cr::CreativeGridCoord3 aimCell) noexcept {
  synchronizeCreativeMaterialBrushPivotDocument(state, documentId);
  state.aimAvailable =
      aimAvailable && documentId != cr::kInvalidDocumentId;
  if (state.aimAvailable) {
    state.aimCell = aimCell;
  }
}

bool lockCreativeMaterialBrushPivotFromAim(
    CreativeMaterialBrushPivotState& state) noexcept {
  if (!state.aimAvailable ||
      state.documentId == cr::kInvalidDocumentId) {
    return false;
  }
  state.locked = true;
  state.lockedCell = state.aimCell;
  return true;
}

bool clearCreativeMaterialBrushPivot(
    CreativeMaterialBrushPivotState& state) noexcept {
  if (!state.locked) {
    return false;
  }
  state.locked = false;
  state.lockedCell = {};
  return true;
}

bool creativeMaterialBrushLockedPivot(
    const CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId,
    cr::CreativeGridCoord3& pivot) noexcept {
  if (!state.locked || documentId == cr::kInvalidDocumentId ||
      state.documentId != documentId) {
    return false;
  }
  pivot = state.lockedCell;
  return true;
}

bool storeSelectedCreativeMaterialBrushPreset(
    CreativeMaterialBrushPresetBank& presets,
    const cr::CreativeHotbarState& hotbar,
    const cr::CreativeToolSettings& settings) noexcept {
  const std::size_t slot = std::min<std::size_t>(
      hotbar.selectedSlot, cr::kCreativeHotbarSlotCount - 1U);
  if (hotbar.entries[slot].kind !=
          cr::CreativeHeldItemKind::MaterialBrush ||
      !cr::isValidCreativeToolSettings(settings)) {
    return false;
  }
  presets.slots[slot] = creativeMaterialBrushGestureConfig(settings);
  presets.initialized[slot] = 1U;
  return true;
}

bool activateSelectedCreativeMaterialBrushPreset(
    CreativeMaterialBrushPresetBank& presets,
    const cr::CreativeHotbarState& hotbar,
    cr::CreativeToolSettings& settings) noexcept {
  const std::size_t slot = std::min<std::size_t>(
      hotbar.selectedSlot, cr::kCreativeHotbarSlotCount - 1U);
  if (hotbar.entries[slot].kind !=
      cr::CreativeHeldItemKind::MaterialBrush) {
    return false;
  }
  if (presets.initialized[slot] == 0U) {
    return storeSelectedCreativeMaterialBrushPreset(presets, hotbar,
                                                    settings);
  }
  cr::CreativeToolSettings adjusted = settings;
  applyCreativeMaterialBrushGestureConfig(adjusted, presets.slots[slot]);
  if (!cr::isValidCreativeToolSettings(adjusted)) {
    return false;
  }
  settings = adjusted;
  return true;
}

void clearCreativeMaterialBrushPresetSlot(
    CreativeMaterialBrushPresetBank& presets,
    std::size_t slot) noexcept {
  if (slot >= cr::kCreativeHotbarSlotCount) {
    return;
  }
  presets.slots[slot] = {};
  presets.initialized[slot] = 0U;
}

bool creativeMaterialBrushPresetForSlot(
    const CreativeMaterialBrushPresetBank& presets,
    std::size_t slot,
    CreativeMaterialBrushGestureConfig& preset) noexcept {
  if (slot >= cr::kCreativeHotbarSlotCount ||
      presets.initialized[slot] == 0U) {
    return false;
  }
  preset = presets.slots[slot];
  return true;
}

void syncCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                CreativeEditorState& editor) {
  finalizeCreativeEditorContinuousGestures(
      appState, editor, "creative_continuous_gesture_tool_changed");
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  const cr::CreativeHeldItemKind previousKind =
      editor.interaction.synchronizedHeldItemKind;
  const cr::CreativeHeldItemDefinition& previousDefinition =
      cr::describeCreativeHeldItem(previousKind);
  const bool interactionChanged =
      previousKind == cr::CreativeHeldItemKind::Count ||
      previousDefinition.interactionMode != definition.interactionMode;
  clearCreativeEditorPlacementFeedback(editor.interaction);
  editor.placeMode = definition.placeMode;
  if (held.objectKind != cr::CreativeObjectKind::Unknown &&
      cr::creativeHeldItemUsesMaterial(held.kind)) {
    editor.placeBrush = held.objectKind;
  }
  if (definition.volumeMode) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    activateCreativeEditorVolumeMode(
        editor.volume, creativeEditorTargetCellSize(
                           appState.facade.document(), editor),
        grid.origin);
    editor.volume.operation = definition.volumeOperation;
  } else {
    deactivateCreativeEditorVolumeMode(editor.volume);
  }
  if (interactionChanged &&
      definition.interactionMode !=
          cr::CreativeHeldItemInteractionMode::ObjectMove) {
    editor.interaction.moveTargetId = cr::kInvalidObjectId;
    editor.interaction.movingPlatformPathEdit = {};
  }
  if (definition.interactionMode ==
      cr::CreativeHeldItemInteractionMode::LogicLink) {
    syncCreativeEditorLogicLinkState(editor.logicLinks,
                                     appState.facade.document());
  } else if (interactionChanged) {
    static_cast<void>(clearCreativeEditorLogicLinkSource(editor.logicLinks));
    editor.logicLinks.documentId = appState.facade.document().id();
  }
  if (interactionChanged) {
    clearCreativeEditorTerrainInteraction(editor.terrain,
                                          appState.facade.document().id());
  }
  editor.interaction.synchronizedHeldItemKind = held.kind;
  syncCreativeEditorQuickEdit(editor);
  static_cast<void>(appState.facade.setActiveTool(definition.facadeTool));
}

bool selectCreativeEditorHotbarSlot(cr::CreativeAppState& appState,
                                    CreativeEditorState& editor,
                                    std::size_t slot) {
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  const bool changed =
      cr::selectCreativeHotbarSlot(editor.interaction.hotbar, slot);
  if (changed) {
    static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
        editor.interaction.materialBrushPresets,
        editor.interaction.hotbar, editor.toolSettings));
  }
  syncCreativeEditorHeldItem(appState, editor);
  return changed;
}

void processCreativeEditorWorldInteractionFrame(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHeldItemKind heldKind =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind;
  if (editor.interaction.synchronizedHeldItemKind != heldKind) {
    syncCreativeEditorHeldItem(request.appState, editor);
  }
  const cr::CreativeDocument& document = request.appState.facade.document();
  syncCreativeEditorLogicLinkState(editor.logicLinks, document);
  static_cast<void>(syncCreativeEditorGroupFocus(editor.groupFocus, document));
  const double targetCellSize =
      creativeEditorTargetCellSize(document, editor);
  const cr::CreativeGridSettings documentGrid = document.gridSettings();
  const cr::CreativeVolumeSelection& volumeSelection =
      editor.volume.selection;
  const bool volumeGridChanged =
      volumeSelection.cellSize != targetCellSize ||
      volumeSelection.origin.x != documentGrid.origin.x ||
      volumeSelection.origin.y != documentGrid.origin.y ||
      volumeSelection.origin.z != documentGrid.origin.z;
  if (editor.volume.active && volumeGridChanged) {
    activateCreativeEditorVolumeMode(editor.volume, targetCellSize,
                                     documentGrid.origin);
  }
  editor.interaction.target = resolveCreativeEditorWorldTarget(
      document, request.camera, request.pickFrame, request.contentRegion,
      targetCellSize);
  const cr::CreativeHotbarEntry& aimedHeld =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (aimedHeld.kind == cr::CreativeHeldItemKind::ObjectMove) {
    syncCreativeMovingPlatformPathEditState(
        request.appState, editor.interaction.movingPlatformPathEdit);
  } else {
    editor.interaction.movingPlatformPathEdit = {};
  }
  const bool hierarchySelectionTool =
      cr::describeCreativeHeldItem(aimedHeld.kind).hierarchySelectionTool;
  if (hierarchySelectionTool && editor.interaction.target.objectHit) {
    const cr::CreativeObjectId resolvedObjectId =
        resolveCreativeEditorGroupSelectionTarget(
            document, editor.groupFocus, editor.interaction.target.objectId);
    const cr::CreativeObject* resolvedObject =
        document.findObject(resolvedObjectId);
    if (resolvedObject == nullptr) {
      editor.interaction.target.objectHit = false;
      editor.interaction.target.objectId = cr::kInvalidObjectId;
      editor.interaction.target.objectKind = cr::CreativeObjectKind::Unknown;
    } else {
      editor.interaction.target.objectId = resolvedObject->id;
      editor.interaction.target.objectKind = resolvedObject->kind;
    }
  }
  if (cr::creativeHeldItemIsTerrainTool(aimedHeld.kind)) {
    updateCreativeEditorTerrainAim(
        editor.terrain, document, editor.interaction.target.ray,
        editor.interaction.target.valid ? editor.interaction.target.distanceMeters
                                        : kCreativeEditorReachMeters);
  } else {
    clearCreativeEditorTerrainInteraction(editor.terrain, document.id());
  }
  cr::CreativeGridTarget brushPivotAim;
  if (editor.interaction.target.grid.valid) {
    const cr::CreativeGridTarget& targetGrid = editor.interaction.target.grid;
    brushPivotAim = cr::resolveCreativeGridTargetFromHit(
        targetGrid.hitPoint, targetGrid.faceNormal,
        documentGrid.cellSizeMeters, documentGrid.origin,
        targetGrid.placerForward);
  }
  updateCreativeMaterialBrushPivotAim(
      editor.interaction.materialBrushPivot, document.id(),
      brushPivotAim.valid, brushPivotAim.adjacentCell);
  editor.volume.cursorValid = editor.interaction.target.grid.valid;
  if (editor.volume.cursorValid) {
    editor.volume.cursorCell = editor.interaction.target.grid.targetCell;
  }
  if (request.captureMode) {
    editor.interaction.movingPlatformPathEdit.pending =
        CreativeMovingPlatformPathEditCommand::None;
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_capture");
    return;
  }
  if (creativeEditorGroupFocusActive(editor.groupFocus) &&
      cr::creativeWorldActionPressed(
          request.actions, cr::CreativeWorldActionId::Reject)) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_group_focus_exit");
    static_cast<void>(exitCreativeEditorGroupFocus(
        request.appState, editor.groupFocus));
    editor.interaction.target = {};
    return;
  }
  if (editor.transform.active) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_transform");
    const bool secondaryPressed =
        cr::creativeWorldActionPressed(
            request.actions, cr::CreativeWorldActionId::Secondary) ||
        cr::creativeWorldActionPressed(
            request.actions, cr::CreativeWorldActionId::Accept);
    static_cast<void>(processCreativeEditorSelectionTransformPreview(
        request.appState, editor.transform,
        editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor, secondaryPressed,
        "selection_transform_commit",
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement)));
    return;
  }

  if (editor.interaction.movingPlatformPathEdit.pending !=
      CreativeMovingPlatformPathEditCommand::None) {
    const CreativeMovingPlatformPathEditReceipt receipt =
        consumeCreativeMovingPlatformPathEdit(
            request.appState, editor.interaction.movingPlatformPathEdit,
            editor.interaction.target.grid.valid,
            editor.interaction.target.grid.placementAnchor,
            "creative_platform_path_quick_edit",
            editor.toolSettings.moveConstraint);
    setCreativeEditorPlacementFeedback(
        editor.interaction,
        receipt.accepted ? CreativeEditorPlacementFeedbackStatus::Placed
                         : CreativeEditorPlacementFeedbackStatus::Rejected,
        editor.frameIndex, cr::CreativeObjectKind::MovingPlatform,
        receipt.objectId);
    return;
  }

  std::int32_t hotbarSteps = -request.actions.hotbarWheelSteps;
  if (cr::creativeWorldActionPressed(
          request.actions, cr::CreativeWorldActionId::HotbarPrevious)) {
    --hotbarSteps;
  }
  if (cr::creativeWorldActionPressed(
          request.actions, cr::CreativeWorldActionId::HotbarNext)) {
    ++hotbarSteps;
  }
  if (hotbarSteps != 0) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_hotbar");
    static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
        editor.interaction.materialBrushPresets,
        editor.interaction.hotbar, editor.toolSettings));
    if (cr::cycleCreativeHotbar(editor.interaction.hotbar, hotbarSteps)) {
      static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
          editor.interaction.materialBrushPresets,
          editor.interaction.hotbar, editor.toolSettings));
      syncCreativeEditorHeldItem(request.appState, editor);
    }
  }

  processCreativeEditorHeldItemFrame(request);
}

void finalizeCreativeEditorContinuousGestures(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode) {
  finalizeCreativeEditorContinuousGesturesExcept(
      appState, editor, CreativeEditorContinuousGestureOwner::None,
      reasonCode);
}

CreativeEditorContinuousGestureOwner creativeEditorContinuousGestureOwner(
    const CreativeEditorState& editor) noexcept {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  switch (cr::describeCreativeHeldItem(held.kind).frameMode) {
    case cr::CreativeHeldItemFrameMode::MaterialStroke:
      if (creativeEditorUsesAuthoredAsset(held, editor.authoredAssets)) {
        return CreativeEditorContinuousGestureOwner::AuthoredAsset;
      }
      if (creativeEditorUsesAssetScatter(held, editor.toolSettings)) {
        return CreativeEditorContinuousGestureOwner::AssetScatter;
      }
      return CreativeEditorContinuousGestureOwner::Material;
    case cr::CreativeHeldItemFrameMode::TerrainControlStroke:
      return CreativeEditorContinuousGestureOwner::TerrainControl;
    case cr::CreativeHeldItemFrameMode::TerrainPaint:
      return CreativeEditorContinuousGestureOwner::TerrainPaint;
    case cr::CreativeHeldItemFrameMode::TerrainSculpt:
      return CreativeEditorContinuousGestureOwner::TerrainSculpt;
    case cr::CreativeHeldItemFrameMode::Standard:
    case cr::CreativeHeldItemFrameMode::ObjectMove:
    case cr::CreativeHeldItemFrameMode::Count:
      return CreativeEditorContinuousGestureOwner::None;
  }
  return CreativeEditorContinuousGestureOwner::None;
}

void finalizeCreativeEditorContinuousGesturesExcept(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorContinuousGestureOwner owner,
    std::string_view reasonCode) {
  for (const ContinuousGestureFinalizerRow& row :
       kContinuousGestureFinalizers) {
    if (row.owner != owner) {
      row.finalize(appState, editor, reasonCode);
    }
  }
}

}  // namespace iggy3d_creative_app
