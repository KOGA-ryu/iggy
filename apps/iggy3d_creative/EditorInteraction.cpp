#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "EditorGroup.hpp"
#include "EditorRoomPlacement.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayoutRoofs.hpp"
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

void finalizeCreativeMaterialInteraction(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode) {
  finalizeCreativeMaterialStroke(appState, editor, reasonCode);
  cancelCreativeEditorStructuralSpan(editor);
}

constexpr std::array kContinuousGestureFinalizers{
    ContinuousGestureFinalizerRow{
        CreativeEditorContinuousGestureOwner::Material,
        finalizeCreativeMaterialInteraction},
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

[[nodiscard]] const CreativeEditorWorldLayoutRoofHandle* findRoofHandle(
    const CreativeEditorWorldLayoutRoofHandleFrame& frame,
    CreativeEditorWorldLayoutRoofTarget target) noexcept {
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    if (frame.handles[index].valid &&
        frame.handles[index].target == target) {
      return &frame.handles[index];
    }
  }
  return nullptr;
}

[[nodiscard]] bool sampleRoofHandleCoordinate(
    const CreativeEditorWorldLayoutRoofHandle& handle,
    const CreativeEditorWorldTarget& worldTarget,
    double cellSizeMeters,
    double& coordinateCells) noexcept {
  if (!worldTarget.ray.valid || !std::isfinite(cellSizeMeters) ||
      cellSizeMeters <= 0.0) {
    return false;
  }
  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(
          cr::creativeVec3FromCore(worldTarget.ray.origin),
          cr::creativeVec3FromCore(worldTarget.ray.direction),
          cr::creativeVec3FromCore(handle.worldPosition),
          cr::creativeVec3FromCore(handle.worldAxis));
  if (!sample.valid) {
    return false;
  }
  coordinateCells = sample.axisParameter / cellSizeMeters;
  return std::isfinite(coordinateCells);
}

}  // namespace

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
    editor.interaction.movingPlatformPathEdit = {};
    resetCreativeEditorStructuralSpanEdit(
        editor.interaction.structuralSpanEdit);
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
    if (definition.interactionMode !=
        cr::CreativeHeldItemInteractionMode::BuildingRoom) {
      static_cast<void>(cancelCreativeEditorRoomPlacement(editor));
    }
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
  const cr::CreativePlacementGridFrame placementGrid =
      creativeEditorPlacementGridFrame(document, editor);
  editor.interaction.target = resolveCreativeEditorWorldTarget(
      document, request.camera, request.pickFrame, request.contentRegion,
      placementGrid, &editor.interaction.target);
  const cr::CreativeHotbarEntry& aimedHeld =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& aimedDefinition =
      cr::describeCreativeHeldItem(aimedHeld.kind);
  if (aimedDefinition.interactionMode ==
      cr::CreativeHeldItemInteractionMode::ObjectMove) {
    syncCreativeMovingPlatformPathEditState(
        request.appState, editor.interaction.movingPlatformPathEdit);
  } else {
    editor.interaction.movingPlatformPathEdit = {};
    resetCreativeEditorStructuralSpanEdit(
        editor.interaction.structuralSpanEdit);
  }
  const bool hierarchySelectionTool =
      aimedDefinition.hierarchySelectionTool;
  if (hierarchySelectionTool && editor.interaction.target.objectHit) {
    ObjectVisualPickStack& stack =
        editor.interaction.target.objectPickStack;
    std::size_t resolvedCount = 0U;
    for (std::size_t hitIndex = 0U; hitIndex < stack.count; ++hitIndex) {
      ObjectVisualPickHit hit = stack.items[hitIndex];
      hit.objectId = resolveCreativeEditorGroupSelectionTarget(
          document, editor.groupFocus, hit.objectId);
      const bool duplicate = std::any_of(
          stack.items.begin(), stack.items.begin() + resolvedCount,
          [hit](const ObjectVisualPickHit& existing) {
            return existing.objectId == hit.objectId;
          });
      if (hit.objectId != cr::kInvalidObjectId && !duplicate) {
        stack.items[resolvedCount++] = hit;
      }
    }
    stack.count = resolvedCount;
    const cr::CreativeObjectId resolvedObjectId =
        stack.count > 0U
            ? stack.items[0].objectId
            : resolveCreativeEditorGroupSelectionTarget(
                  document, editor.groupFocus,
                  editor.interaction.target.objectId);
    const cr::CreativeObject* resolvedObject =
        document.findObject(resolvedObjectId);
    if (resolvedObject == nullptr) {
      editor.interaction.target.objectHit = false;
      editor.interaction.target.objectId = cr::kInvalidObjectId;
      editor.interaction.target.objectKind = cr::CreativeObjectKind::Unknown;
      editor.interaction.target.grid.targetFacts = {};
    } else {
      editor.interaction.target.objectId = resolvedObject->id;
      editor.interaction.target.objectKind = resolvedObject->kind;
      editor.interaction.target.grid.targetFacts =
          cr::makeCreativePlacementTargetFacts(
              cr::CreativePlacementTargetSource::AuthoredObject,
              resolvedObject->kind, resolvedObject->id);
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
    cr::CreativePlacementGridFrameRequest brushGridRequest;
    brushGridRequest.documentGrid = documentGrid;
    brushGridRequest.documentSnap = document.documentSnapSettings();
    brushGridRequest.documentWorldBounds = document.worldBounds();
    brushGridRequest.storageAligned = true;
    brushPivotAim = cr::resolveCreativeGridTargetFromHit(
        targetGrid.hitPoint, cr::creativeGridTargetExactSurfaceNormal(targetGrid),
        cr::makeCreativePlacementGridFrame(brushGridRequest),
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
    if (editor.worldLayout.roofManipulation.active) {
      static_cast<void>(
          applyCreativeEditorWorldLayoutRoofManipulationToDocument(
              editor.worldLayout, request.appState,
              CreativeEditorWorldLayoutRoofManipulationPhase::Cancel));
    }
    static_cast<void>(
        finishCreativeEditorVolumeHandleGesture(editor.volume, false));
    editor.interaction.movingPlatformPathEdit.pending =
        CreativeMovingPlatformPathEditCommand::None;
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_capture");
    return;
  }

  if (editor.worldLayout.roofManipulation.active) {
    const CreativeEditorWorldLayoutRoofTarget target =
        editor.worldLayout.roofManipulation.target;
    const CreativeEditorWorldLayoutRoofHandleFrame handles =
        buildCreativeEditorWorldLayoutRoofHandleFrame(
            editor.worldLayout, documentGrid, target.levelIndex, false);
    const CreativeEditorWorldLayoutRoofHandle* handle =
        findRoofHandle(handles, target);
    const bool cancel =
        handle == nullptr ||
        editor.worldLayout.roofManipulation.sourceRevision !=
            editor.worldLayout.revision ||
        cr::creativeWorldActionPressed(request.actions,
                                       cr::CreativeWorldActionId::Reject);
    if (cancel) {
      static_cast<void>(
          applyCreativeEditorWorldLayoutRoofManipulationToDocument(
              editor.worldLayout, request.appState,
              CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
              target));
      return;
    }

    double coordinateCells = 0.0;
    if (sampleRoofHandleCoordinate(*handle, editor.interaction.target,
                                   documentGrid.cellSizeMeters,
                                   coordinateCells)) {
      static_cast<void>(
          applyCreativeEditorWorldLayoutRoofManipulationToDocument(
              editor.worldLayout, request.appState,
              CreativeEditorWorldLayoutRoofManipulationPhase::Update,
              target, coordinateCells));
    }
    const bool released =
        cr::creativeWorldActionReleased(request.actions,
                                        cr::CreativeWorldActionId::Primary) ||
        cr::creativeWorldActionReleased(request.actions,
                                        cr::CreativeWorldActionId::Accept);
    if (released) {
      static_cast<void>(
          applyCreativeEditorWorldLayoutRoofManipulationToDocument(
              editor.worldLayout, request.appState,
              CreativeEditorWorldLayoutRoofManipulationPhase::Commit,
              target, coordinateCells));
    }
    return;
  }

  const bool roofHandlePressed =
      hierarchySelectionTool &&
      editor.worldLayout.tool == CreativeEditorWorldLayoutTool::Select &&
      editor.worldLayout.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::Level &&
      editor.worldLayout.selection.index <
          editor.worldLayout.source.levels.size() &&
      editor.interaction.target.ray.valid &&
      (cr::creativeWorldActionPressed(request.actions,
                                      cr::CreativeWorldActionId::Primary) ||
       cr::creativeWorldActionPressed(request.actions,
                                      cr::CreativeWorldActionId::Accept));
  if (roofHandlePressed) {
    const std::size_t levelIndex = editor.worldLayout.selection.index;
    const CreativeEditorWorldLayoutRoofHandleFrame handles =
        buildCreativeEditorWorldLayoutRoofHandleFrame(
            editor.worldLayout, documentGrid, levelIndex, false);
    const float centerX = static_cast<float>(request.contentRegion.x) +
                          static_cast<float>(request.contentRegion.width) *
                              0.5F;
    const float centerY = static_cast<float>(request.contentRegion.y) +
                          static_cast<float>(request.contentRegion.height) *
                              0.5F;
    const CreativeEditorWorldLayoutRoofHandlePick picked =
        pickCreativeEditorWorldLayoutRoofHandleAtPixel(
            handles, request.camera, request.contentRegion, centerX, centerY);
    if (picked.hit && picked.handleIndex < handles.handleCount) {
      const CreativeEditorWorldLayoutRoofHandle& handle =
          handles.handles[picked.handleIndex];
      double coordinateCells = 0.0;
      if (sampleRoofHandleCoordinate(handle, editor.interaction.target,
                                     documentGrid.cellSizeMeters,
                                     coordinateCells)) {
        const CreativeEditorWorldLayoutRoofLiveEditReceipt begun =
            applyCreativeEditorWorldLayoutRoofManipulationToDocument(
                editor.worldLayout, request.appState,
                CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
                handle.target, coordinateCells);
        if (begun.accepted) {
          finalizeCreativeEditorContinuousGestures(
              request.appState, editor, "creative_roof_handle_begin");
          return;
        }
      }
    }
  }

  if (editor.volume.handleGesture.active) {
    const bool cancel = cr::creativeWorldActionPressed(
        request.actions, cr::CreativeWorldActionId::Reject);
    if (cancel) {
      static_cast<void>(
          finishCreativeEditorVolumeHandleGesture(editor.volume, false));
      return;
    }
    if (editor.interaction.target.ray.valid) {
      static_cast<void>(updateCreativeEditorVolumeHandleGesture(
          editor.volume,
          cr::creativeVec3FromCore(editor.interaction.target.ray.origin),
          cr::creativeVec3FromCore(editor.interaction.target.ray.direction)));
    }
    const bool released = cr::creativeWorldActionReleased(
                              request.actions,
                              cr::CreativeWorldActionId::Primary) ||
                          cr::creativeWorldActionReleased(
                              request.actions,
                              cr::CreativeWorldActionId::Accept);
    if (released) {
      static_cast<void>(
          finishCreativeEditorVolumeHandleGesture(editor.volume, true));
    }
    return;
  }
  const bool volumeHandlePressed =
      editor.volume.active &&
      aimedDefinition.interactionMode !=
          cr::CreativeHeldItemInteractionMode::TerrainRegion &&
      editor.interaction.target.ray.valid &&
      (cr::creativeWorldActionPressed(request.actions,
                                      cr::CreativeWorldActionId::Primary) ||
       cr::creativeWorldActionPressed(request.actions,
                                      cr::CreativeWorldActionId::Accept));
  if (volumeHandlePressed) {
    const CreativeEditorVolumeHandleFrame handles =
        buildCreativeEditorVolumeHandleFrame(editor.volume, request.camera,
                                             request.contentRegion);
    const float centerX = static_cast<float>(request.contentRegion.x) +
                          static_cast<float>(request.contentRegion.width) * 0.5F;
    const float centerY = static_cast<float>(request.contentRegion.y) +
                          static_cast<float>(request.contentRegion.height) * 0.5F;
    const CreativeEditorVolumeHandlePick picked =
        pickCreativeEditorVolumeHandle(handles, centerX, centerY);
    if (picked.hit && beginCreativeEditorVolumeHandleGesture(
                          editor.volume, handles.handles[picked.index],
                          cr::creativeVec3FromCore(
                              editor.interaction.target.ray.origin),
                          cr::creativeVec3FromCore(
                              editor.interaction.target.ray.direction))) {
      finalizeCreativeEditorContinuousGestures(
          request.appState, editor, "creative_volume_handle_begin");
      return;
    }
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
    static_cast<void>(cancelCreativeEditorStructuralSpanEdit(
        editor.interaction.structuralSpanEdit));
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_transform");
    const bool gestureActive =
        editor.transform.pointerGesture.kind !=
        CreativeEditorTransformPointerGestureKind::None;
    if (gestureActive) {
      const cr::CreativeVec3 rayOrigin =
          cr::creativeVec3FromCore(editor.interaction.target.ray.origin);
      const cr::CreativeVec3 rayDirection =
          cr::creativeVec3FromCore(editor.interaction.target.ray.direction);
      static_cast<void>(updateCreativeEditorTransformPointerGesture(
          request.appState, editor.transform,
          editor.interaction.target.grid.valid,
          editor.interaction.target.grid.placementAnchor, rayOrigin,
          rayDirection));
      const bool released =
          cr::creativeWorldActionReleased(
              request.actions, cr::CreativeWorldActionId::Primary) ||
          cr::creativeWorldActionReleased(
              request.actions, cr::CreativeWorldActionId::Accept);
      if (released) {
        static_cast<void>(finishCreativeEditorTransformPointerGesture(
            editor.transform, "transform_pointer_release"));
        if (!editor.transform.active) {
          return;
        }
      }
    }
    const bool secondaryPressed =
        !gestureActive &&
        (cr::creativeWorldActionPressed(
             request.actions, cr::CreativeWorldActionId::Secondary) ||
         cr::creativeWorldActionPressed(
             request.actions, cr::CreativeWorldActionId::Accept));
    static_cast<void>(processCreativeEditorSelectionTransformPreview(
        request.appState, editor.transform,
        editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor, secondaryPressed,
        "selection_transform_commit",
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement),
        &editor.worldLayout, request.placementClearanceCache));
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
  static_cast<void>(cancelCreativeEditorStructuralSpanEdit(
      editor.interaction.structuralSpanEdit));
  static_cast<void>(cancelCreativeEditorRoomPlacement(editor));
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
