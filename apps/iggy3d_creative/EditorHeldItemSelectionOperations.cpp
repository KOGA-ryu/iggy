#include "EditorHeldItemWorldOperationsInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <vector>

#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteractionInternal.hpp"
#include "EditorPathEditing.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "EditorTransform.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/WorldActionIntent.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] cr::CreativeToolModifierFlags toolModifiers(
    cr::CreativeInputModifierMask modifiers) noexcept {
  cr::CreativeToolModifierFlags output = cr::kCreativeToolModifierNone;
  if ((modifiers & cr::kCreativeInputModifierShift) != 0U) {
    output |= cr::kCreativeToolModifierShift;
  }
  if ((modifiers & cr::kCreativeInputModifierControl) != 0U) {
    output |= cr::kCreativeToolModifierControl;
  }
  if ((modifiers & cr::kCreativeInputModifierCommand) != 0U) {
    output |= cr::kCreativeToolModifierCommand;
  }
  return output;
}

[[nodiscard]] cr::CreativeToolInputPacket selectionPacket(
    const CreativeEditorWorldTarget& target,
    cr::CreativeToolModifierFlags modifiers) noexcept {
  cr::CreativeToolInputPacket packet;
  packet.kind = cr::CreativeToolInputKind::PointerPress;
  packet.pointer.button = cr::CreativeToolPointerButton::Primary;
  packet.pointer.modifiers = modifiers;
  if (target.objectHit) {
    packet.pointer.target =
        cr::TargetRef{static_cast<cr::Id>(target.objectId)};
  }
  return packet;
}

[[nodiscard]] cr::CreativeSelectionPlacementAxis placementAxisForGizmo(
    GizmoAxis axis) noexcept {
  switch (axis) {
    case GizmoAxis::X: return cr::CreativeSelectionPlacementAxis::X;
    case GizmoAxis::Y: return cr::CreativeSelectionPlacementAxis::Y;
    case GizmoAxis::Z: return cr::CreativeSelectionPlacementAxis::Z;
    case GizmoAxis::None: return cr::CreativeSelectionPlacementAxis::Count;
  }
  return cr::CreativeSelectionPlacementAxis::Count;
}

[[nodiscard]] bool beginTransformFromGizmoAxis(
    const CreativeEditorWorldInteractionFrameRequest& request,
    float targetX,
    float targetY) {
  if (request.gizmoFrame == nullptr ||
      !request.editor.interaction.target.ray.valid) {
    return false;
  }
  const GizmoAxisPickResult picked = pickCreativeEditorGizmoAxisAtPixel(
      request.gizmoFrame->axisHandles, targetX, targetY);
  if (!picked.hit) {
    return false;
  }

  CreativeEditorSelectionTransformState& transform = request.editor.transform;
  if (!beginCreativeEditorSelectionTransformPreview(
          request.appState, transform, "gizmo_axis_transform_begin",
          CreativeEditorTransformAnchorPolicy::FixedSource,
          &request.editor.worldLayout)) {
    return true;
  }
  const cr::CreativeSelectionPlacementAxis axis =
      placementAxisForGizmo(picked.axis);
  const auto shaft = std::find_if(
      request.gizmoFrame->shafts.begin(), request.gizmoFrame->shafts.end(),
      [picked](const GizmoAxisShaft& candidate) {
        return candidate.axis == picked.axis;
      });
  const cr::CreativeVec3 rayOrigin =
      cr::creativeVec3FromCore(request.editor.interaction.target.ray.origin);
  const cr::CreativeVec3 rayDirection =
      cr::creativeVec3FromCore(request.editor.interaction.target.ray.direction);
  const cr::CreativeVec3 axisOrigin =
      cr::creativeVec3FromCore(request.gizmoFrame->center);
  const cr::CreativeVec3 axisDirection =
      shaft != request.gizmoFrame->shafts.end()
          ? cr::creativeVec3FromCore(shaft->tip - request.gizmoFrame->center)
          : cr::CreativeVec3{};
  if (!beginCreativeEditorAxisTransformPointerGesture(
          request.appState, transform, axis, axisOrigin, axisDirection,
          rayOrigin, rayDirection)) {
    static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
        transform, "gizmo_axis_transform_invalid"));
  }
  return true;
}

[[nodiscard]] bool selectionContainsObject(
    const cr::CreativeSelectionState& selection,
    cr::CreativeObjectId objectId) noexcept {
  if (selection.selectedTarget.value == static_cast<cr::Id>(objectId)) {
    return true;
  }
  const std::span<const cr::TargetRef> targets =
      cr::selectedTargetList(selection);
  return std::any_of(
      targets.begin(), targets.end(),
      [objectId](cr::TargetRef target) {
        return target.value == static_cast<cr::Id>(objectId);
      });
}

[[nodiscard]] bool beginFreeTransformFromTarget(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if (!target.objectHit || !target.grid.valid) {
    return false;
  }

  const cr::CreativeSelectionState& selection =
      request.appState.facade.selectionState();
  std::vector<cr::CreativeObjectId> selectedObjectIds;
  const std::span<const cr::TargetRef> selectedTargets =
      cr::selectedTargetList(selection);
  selectedObjectIds.reserve(selectedTargets.size());
  for (cr::TargetRef selected : selectedTargets) {
    if (selected.value != cr::kInvalidId) {
      selectedObjectIds.push_back(
          static_cast<cr::CreativeObjectId>(selected.value));
    }
  }
  const cr::CreativeObjectId interactionRoot =
      cr::resolveCreativeHierarchyInteractionRoot(
          request.appState.facade.document(), selectedObjectIds,
          target.objectId);
  if (interactionRoot == cr::kInvalidObjectId) {
    return false;
  }
  if (!selectionContainsObject(selection, interactionRoot)) {
    const std::array selected{interactionRoot};
    if (!request.appState.facade
             .selectTargets(selected, interactionRoot)
             .accepted) {
      return false;
    }
  }
  if (!beginCreativeEditorSelectionTransformPreview(
          request.appState, editor.transform, "pointer_transform_begin",
          CreativeEditorTransformAnchorPolicy::FixedSource,
          &editor.worldLayout)) {
    return false;
  }
  if (!beginCreativeEditorFreeTransformPointerGesture(
          request.appState, editor.transform,
          target.grid.placementAnchor)) {
    static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
        editor.transform, "pointer_transform_invalid"));
    return false;
  }
  return true;
}

void selectObject(CreativeHeldItemWorldOperationContext& context) {
  const CreativeEditorWorldTarget& aimedTarget =
      context.request.editor.interaction.target;
  const cr::CreativeToolModifierFlags modifiers =
      toolModifiers(context.request.modifiers);
  CreativeEditorWorldTarget target = aimedTarget;
  cr::CreativeWorldLayoutSourceRef preferredSource;
  if (target.objectHit &&
      (modifiers & cr::kCreativeToolModifierShift) == 0U &&
      target.objectPickStack.count > 0U) {
    const cr::CreativeObjectId currentObjectId =
        context.request.appState.facade.selectionState().selectedTarget.value ==
                cr::kInvalidId
            ? cr::kInvalidObjectId
            : static_cast<cr::CreativeObjectId>(
                  context.request.appState.facade.selectionState()
                      .selectedTarget.value);
    const cr::CreativeObjectId cycled =
        cycleObjectVisualPick(target.objectPickStack, currentObjectId);
    if (cycled != cr::kInvalidObjectId) {
      target.objectId = cycled;
      for (const ObjectVisualPickHit& hit : target.objectPickStack.hits()) {
        if (hit.objectId == cycled) {
          target.distanceMeters = hit.entryDistance;
          break;
        }
      }
    }
  }
  if (target.objectHit) {
    const cr::CreativeDocument& document =
        context.request.appState.facade.document();
    CreativeEditorWorldLayoutState& worldLayout =
        context.request.editor.worldLayout;
    cr::CreativeSemanticSelectionResolution resolved;
    const cr::CreativeGridSettings grid = document.gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      cr::CreativeVec3 worldPoint = target.grid.hitPoint;
      if (target.ray.valid && std::isfinite(target.distanceMeters)) {
        worldPoint = cr::creativeVec3FromCore(
            target.ray.origin +
            target.ray.direction * target.distanceMeters);
      }
      const cr::CreativeVec3 sourcePointCells{
          (worldPoint.x - grid.origin.x) / grid.cellSizeMeters,
          (worldPoint.y - grid.origin.y) / grid.cellSizeMeters,
          (worldPoint.z - grid.origin.z) / grid.cellSizeMeters,
      };
      resolved = cr::resolveCreativeSemanticSelection(
          document, target.objectId, worldLayout.source, sourcePointCells);
    } else {
      resolved = cr::resolveCreativeSemanticSelection(
          document, target.objectId, &worldLayout.source);
    }
    if (worldLayout.generatedRevision == worldLayout.revision &&
        resolved.worldLayoutSource.owned) {
      preferredSource = {resolved.worldLayoutSource.table,
                         resolved.worldLayoutSource.index};
    }
  }
  static_cast<void>(
      context.request.appState.facade.setActiveTool(cr::Tool::Select));
  const cr::CreativeFacadeToolDispatchReceipt selection =
      context.request.appState.facade.dispatchToolInput(
          selectionPacket(target, modifiers));
  if (selection.accepted) {
    static_cast<void>(synchronizeCreativeEditorWorldLayoutSelection(
        context.request.editor.worldLayout,
        context.request.appState.facade.document(),
        context.request.appState.facade.selectionState(), preferredSource));
  }
}

void clearObjectSelection(CreativeHeldItemWorldOperationContext& context) {
  const cr::CreativeSelectionReceipt cleared =
      context.request.appState.facade.selectTargets(
          std::span<const cr::CreativeObjectId>{}, cr::kInvalidObjectId);
  if (cleared.accepted) {
    static_cast<void>(synchronizeCreativeEditorWorldLayoutSelection(
        context.request.editor.worldLayout,
        context.request.appState.facade.document(),
        context.request.appState.facade.selectionState()));
  }
}

void sampleTargetMaterial(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if ((!target.objectHit && !target.voxelHit) ||
      target.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }
  if (context.held.kind != cr::CreativeHeldItemKind::Material &&
      cr::creativeHeldItemUsesMaterial(context.held.kind)) {
    if (cr::creativeVolumeBrushSupported(target.objectKind)) {
      editor.placeBrush = target.objectKind;
      cr::CreativeHotbarEntry& selected =
          cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
      selected.objectKind = target.objectKind;
      cr::clearCreativeHotbarAsset(selected);
      syncCreativeEditorQuickEdit(editor);
    }
    return;
  }
  if (target.objectHit) {
    const cr::CreativeObject* object =
        context.request.appState.facade.document().findObject(target.objectId);
    if (object != nullptr &&
        cr::assignCreativeHotbarFromObject(editor.interaction.hotbar,
                                           *object)) {
      syncCreativeEditorHeldItem(context.request.appState, editor);
      return;
    }
  }
  static_cast<void>(
      cr::assignCreativeHotbarMaterial(editor.interaction.hotbar,
                                       target.objectKind));
  syncCreativeEditorHeldItem(context.request.appState, editor);
}

void rejectActiveInteraction(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      cancelCreativeEditorHeldItem(context.request.appState,
                                   context.request.editor));
}

}  // namespace

void selectCreativeHeldItemTarget(
    CreativeHeldItemWorldOperationContext& context) {
  selectObject(context);
}

cr::CreativeToolInputPacket creativeHeldItemSelectionPacket(
    const CreativeEditorWorldTarget& target,
    cr::CreativeToolModifierFlags modifiers) noexcept {
  return selectionPacket(target, modifiers);
}

void executeCreativeHeldItemSelectionTransformOperation(
    cr::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context) {
  switch (operation) {
    case cr::CreativeHeldItemWorldOperation::SelectObject:
      selectObject(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ClearSelection:
      clearObjectSelection(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SampleTargetMaterial:
      sampleTargetMaterial(context);
      return;
    case cr::CreativeHeldItemWorldOperation::RejectActiveInteraction:
      rejectActiveInteraction(context);
      return;
    default:
      return;
  }
}

void processCreativeEditorMoveInteractionInternal(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const float targetX = static_cast<float>(request.contentRegion.x) +
                        static_cast<float>(request.contentRegion.width) * 0.5F;
  const float targetY = static_cast<float>(request.contentRegion.y) +
                        static_cast<float>(request.contentRegion.height) * 0.5F;
  if (processCreativeEditorStructuralSpanEditInput(
          request.appState, editor, request.actions,
          request.pickFrame.structuralSpanEndpointHandles.items(), targetX,
          targetY)) {
    return;
  }
  const cr::CreativeWorldIntentFrame intents = cr::resolveCreativeWorldIntents(
      request.actions, cr::CreativeWorldIntentPolicy::Manipulation);
  CreativeMovingPlatformPathEditState& pathEdit =
      editor.interaction.movingPlatformPathEdit;
  const bool rejectPressed = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Negative);
  if (rejectPressed) {
    if (clearCreativeMovingPlatformPathPointSelection(pathEdit)) {
      return;
    }
    static_cast<void>(cancelCreativeEditorHeldItem(request.appState, editor));
    return;
  }
  const bool pressed = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Positive);
  const bool secondaryPressed = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Alternate);

  if (secondaryPressed && pathEdit.pointSelected) {
    static_cast<void>(clearCreativeMovingPlatformPathPointSelection(pathEdit));
    return;
  }

  if (pressed && pathEdit.available) {
    const PathPointHandlePickResult handle = pickPathPointHandleAtPixel(
        request.pickFrame.pathPointHandleHits, targetX, targetY);
    if (handle.hit && handle.objectId == pathEdit.objectId &&
        selectCreativeMovingPlatformPathPoint(pathEdit, handle.pointIndex)) {
      return;
    }
    if (pathEdit.pointSelected &&
        queueCreativeMovingPlatformPathEdit(
            request.appState, pathEdit,
            CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget)) {
      const CreativeMovingPlatformPathEditReceipt receipt =
          consumeCreativeMovingPlatformPathEdit(
              request.appState, pathEdit,
              editor.interaction.target.grid.valid,
              editor.interaction.target.grid.placementAnchor,
              "creative_platform_path_point_move",
              editor.toolSettings.moveConstraint);
      setCreativeEditorPlacementFeedback(
          editor.interaction,
          receipt.accepted ? CreativeEditorPlacementFeedbackStatus::Placed
                           : CreativeEditorPlacementFeedbackStatus::Rejected,
          editor.frameIndex, cr::CreativeObjectKind::MovingPlatform,
          receipt.objectId);
      return;
    }
  }

  if (pathEdit.pointSelected) {
    return;
  }

  if (secondaryPressed && !pressed &&
      beginCreativeEditorSelectionTransformPreview(
          request.appState, editor.transform,
          "minecraft_secondary_transform_begin",
          CreativeEditorTransformAnchorPolicy::FollowAim,
          &editor.worldLayout)) {
    static_cast<void>(processCreativeEditorSelectionTransformPreview(
        request.appState, editor.transform,
        editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor, false,
        "minecraft_secondary_transform_begin",
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement),
        &editor.worldLayout, request.placementClearanceCache));
    return;
  }

  if (pressed) {
    if (beginTransformFromGizmoAxis(request, targetX, targetY)) {
      return;
    }
    static_cast<void>(beginFreeTransformFromTarget(request));
  }
}

}  // namespace iggy3d_creative_app
