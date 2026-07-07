#include "app/iggy3d/automation/AutomationRoomEditing.hpp"

#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/input/ActionState.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {
namespace {

template <typename T>
std::int64_t signedRoomEditorPreviewDelta(T before, T after) {
  return static_cast<std::int64_t>(after) - static_cast<std::int64_t>(before);
}

}  // namespace

void clearProductRoomEditorPreview(ProductAppWindowState& window) {
  window.roomEditorPreview.active = false;
  window.roomEditorPlacementPreview = {};
  window.roomEditorPreview.visible = false;
  window.roomEditorPreview.status = "room_editor_preview_not_requested";
  window.roomEditorPreview.reasonCode = "room_editor_preview_not_requested";
  window.roomEditorPreview.candidateId = "none";
  window.roomEditorPreview.tool = "floor";
  window.roomEditorPreview.gridX = 0;
  window.roomEditorPreview.gridZ = 0;
  window.roomEditorPreview.beforeDrawCount = 0;
  window.roomEditorPreview.afterDrawCount = 0;
  window.roomEditorPreview.avoidedDrawCountDelta = 0;
  window.roomEditorPreview.beforeTriangleCount = 0;
  window.roomEditorPreview.afterTriangleCount = 0;
  window.roomEditorPreview.avoidedTriangleCountDelta = 0;
  window.roomEditorPreview.optimizedDrawDelta = 0;
  window.roomEditorPreview.optimizedTriangleDelta = 0;
}

void markProductRoomEditorPreviewCleared(ProductAppWindowState& window,
                                         std::string_view status) {
  clearProductRoomEditorPreview(window);
  window.roomEditorPreview.status = std::string(status);
  window.roomEditorPreview.reasonCode = window.roomEditorPreview.status;
}

void recordProductRoomEditorPreviewResult(
    ProductAppWindowState& window,
    const ProductRoomEditorPlacementPreviewResult& result) {
  window.roomEditorPreview.active = true;
  window.roomEditorPlacementPreview = result;
  window.roomEditorPreview.visible = result.ok;
  window.roomEditorPreview.status = result.status;
  window.roomEditorPreview.reasonCode = result.reasonCode;
  window.roomEditorPreview.candidateId = result.primitiveId;
  window.roomEditorPreview.tool = result.tool;
  window.roomEditorPreview.gridX = result.gridX;
  window.roomEditorPreview.gridZ = result.gridZ;
  window.roomEditorPreview.beforeDrawCount = result.before.optimizedDrawCount;
  window.roomEditorPreview.afterDrawCount = result.after.optimizedDrawCount;
  window.roomEditorPreview.avoidedDrawCountDelta =
      signedRoomEditorPreviewDelta(result.before.drawCountAvoided,
                                   result.after.drawCountAvoided);
  window.roomEditorPreview.beforeTriangleCount =
      result.before.optimizedTriangleCount;
  window.roomEditorPreview.afterTriangleCount = result.after.optimizedTriangleCount;
  window.roomEditorPreview.avoidedTriangleCountDelta =
      signedRoomEditorPreviewDelta(result.before.triangleCountAvoided,
                                   result.after.triangleCountAvoided);
  window.roomEditorPreview.optimizedDrawDelta = result.optimizedDrawDelta;
  window.roomEditorPreview.optimizedTriangleDelta = result.optimizedTriangleDelta;
}

void copyRoomEditingStateToWindow(ProductAppWindowState& window,
                                  const ProductRoomEditingState& state) {
  window.roomEditing = state;
  // branch-gate: BG-1006
  if (state.ready) {
    activeRoom(window) = state.activeRoom;
    bumpActiveRoomRevision(window);
    (void)ensureActiveRoomCollisionFresh(window, nullptr);
  } else {
    clearProductRoomEditorPreview(window);
  }
}

void recordProductRoomEditingStart(ProductAppWindowState& window,
                                   const ProductRoomEditingStartResult& result,
                                   std::string_view operation) {
  window.roomEditingLastOperation = std::string(operation);
  window.roomEditingLastOperationStatus = result.status;
  window.roomEditingLastOperationReasonCode = result.reasonCode;
  window.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
  window.roomEditingLastOperationAccepted = result.ok;
  window.roomEditingLastPrimitiveId = "none";
  clearProductRoomEditorPreview(window);
  copyRoomEditingStateToWindow(window, result.state);
  // branch-gate: BG-1006
  if (result.ok) {
    window.interactionMode = ProductInteractionMode::Creative;
    window.roomEditorCursorReady = true;
    window.roomEditorCursor = ProductRoomEditorCursorState{};
    window.roomEditorStatus = "room_editor_cursor_ready";
    window.roomEditorReasonCode = "room_editor_cursor_ready";
    window.roomEditorLastOperation = "none";
    window.roomEditorLastOperationAccepted = false;
    window.roomEditorLastPrimitiveId = "none";
  }
}

bool recordProductRoomEditingLeave(const FrontendState& frontend,
                                   ProductAppWindowState& window,
                                   std::string_view operation) {
  window.roomEditingLastOperation = std::string(operation);
  window.roomEditingLastPrimitiveId = "none";
  if (!window.roomEditing.ready) {  // branch-gate: BG-1006
    window.roomEditingLastOperationStatus = "room_editor_not_ready";
    window.roomEditingLastOperationReasonCode = "room_editor_not_ready";
    window.roomEditingLastInputSource =
        productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
    window.roomEditingLastOperationAccepted = false;
    rejectProductRoomEditorNotReady(window, operation);
    return false;
  }

  ProductRoomEditingState leftState;
  leftState.status = "product_room_editing_left";
  leftState.reasonCode = leftState.status;
  copyRoomEditingStateToWindow(window, leftState);
  window.roomEditingLastOperationStatus = "product_room_editing_left";
  window.roomEditingLastOperationReasonCode = window.roomEditingLastOperationStatus;
  window.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
  window.roomEditingLastOperationAccepted = true;
  window.interactionMode = ProductInteractionMode::Player;
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  window.roomEditorCursorReady = false;
  window.roomEditorStatus = "room_editor_not_ready";
  window.roomEditorReasonCode = "room_editor_not_ready";
  window.roomEditorLastOperation = std::string(operation);
  window.roomEditorLastOperationAccepted = true;
  window.roomEditorLastPrimitiveId = "none";
  window.roomEditorOverlay.visible = false;
  window.roomEditorOverlay.status = "room_editor_overlay_not_ready";
  window.roomEditorOverlay.reasonCode = window.roomEditorOverlay.status;
  window.roomEditorOverlay.itemCount = 0;
  window.roomEditorHud.visible = false;
  window.roomEditorHud.status = "room_editor_hud_not_ready";
  window.roomEditorHud.reasonCode = window.roomEditorHud.status;
  window.roomEditorHud.lastOperation = std::string(operation);
  window.roomEditorHud.lastOperationAccepted = true;
  window.roomEditorHud.lastPrimitiveId = "none";
  window.viewport.productDrawRoomEditorCursorVisible = false;
  window.viewport.productDrawRoomEditorCursorCount = 0;
  window.viewport.productDrawRoomEditorPreviewVisible = false;
  window.viewport.productDrawRoomEditorPreviewCount = 0;
  window.viewport.productDrawPropVisible = false;
  window.viewport.productDrawPropTileCount = 0;
  window.viewport.productRenderBridgeRoomEditorCursorVisible = false;
  window.viewport.productRenderBridgeRoomEditorCursorCount = 0;
  window.viewport.productRenderBridgeRoomEditorPreviewVisible = false;
  window.viewport.productRenderBridgeRoomEditorPreviewCount = 0;
  window.viewport.productRenderBridgePropVisible = false;
  window.viewport.productRenderBridgePropTileCount = 0;
  return true;
}

void recordProductRoomEditingOperation(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditingOperationResult& result) {
  window.roomEditingLastOperation = std::string(operation);
  window.roomEditingLastOperationStatus = result.status;
  window.roomEditingLastOperationReasonCode = result.reasonCode;
  window.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(result.inputSource);
  window.roomEditingLastOperationAccepted = result.accepted;
  // branch-gate: BG-1006
  window.roomEditingLastPrimitiveId =
      result.edit.primitiveId.empty() ? std::string{"none"}
                                      : result.edit.primitiveId;
  clearProductRoomEditorPreview(window);
  copyRoomEditingStateToWindow(window, result.state);
}

void recordProductRoomEditorCursorResult(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditorCursorResult& result) {
  window.roomEditorCursorReady = window.roomEditing.ready;
  window.roomEditorCursor = result.state;
  window.roomEditorStatus = result.status;
  window.roomEditorReasonCode = result.reasonCode;
  window.roomEditorLastOperation = std::string(operation);
  window.roomEditorLastOperationAccepted = result.ok;
  window.roomEditorLastPrimitiveId = "none";
  clearProductRoomEditorPreview(window);
}

bool rejectProductRoomEditorNotReady(ProductAppWindowState& window,
                                     std::string_view operation) {
  window.roomEditorCursorReady = false;
  window.roomEditorStatus = "room_editor_not_ready";
  window.roomEditorReasonCode = "room_editor_not_ready";
  window.roomEditorLastOperation = std::string(operation);
  window.roomEditorLastOperationAccepted = false;
  window.roomEditorLastPrimitiveId = "none";
  window.automationControl.status = "command_failed";
  return false;
}

void recordProductRoomEditorActionResult(
    ProductAppWindowState& window,
    const ProductRoomEditorActionResult& result,
    std::string_view operationOverride) {
  window.roomEditing = result.editing;
  copyRoomEditingStateToWindow(window, result.editing);
  window.roomEditorCursorReady = result.editing.ready;
  window.roomEditorCursor = result.cursor;
  window.roomEditorStatus = result.status;
  window.roomEditorReasonCode = result.reasonCode;
  // branch-gate: BG-1006
  window.roomEditorLastOperation =
      operationOverride.empty() ? result.operation : std::string(operationOverride);
  window.roomEditorLastOperationAccepted = result.operationAccepted;
  window.roomEditorLastPrimitiveId = result.primitiveId;
  clearProductRoomEditorPreview(window);

  // branch-gate: BG-1006
  if (result.status == "room_editor_command_applied") {
    window.roomEditingLastOperation = window.roomEditorLastOperation;
    window.roomEditingLastOperationStatus = "product_room_editing_edit_applied";
    window.roomEditingLastOperationReasonCode = "product_room_editing_edit_applied";
    window.roomEditingLastInputSource =
        productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Hotkey);
    window.roomEditingLastOperationAccepted = result.operationAccepted;
    window.roomEditingLastPrimitiveId = result.primitiveId;
  }
}

ProductRoomEditingStartResult startProductRoomEditAutomationFromAsciiDraft(
    ProductRoomAuthoringStartFromAsciiRequest request) {
  return startProductRoomAuthoringFromAsciiDraft(request);
}

ProductRoomEditingStartResult startProductRoomEditAutomationFromActiveRoom(
    ProductRoomAuthoringStartFromActiveRoomRequest request) {
  return startProductRoomAuthoringFromActiveRoom(request);
}

ProductRoomEditingOperationResult applyProductRoomEditAutomation(
    ProductRoomAuthoringEditCommandRequest request) {
  return applyProductRoomAuthoringEditCommand(request);
}

ProductRoomEditingOperationResult undoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request) {
  return undoProductRoomAuthoringEdit(request);
}

ProductRoomEditingOperationResult redoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request) {
  return redoProductRoomAuthoringEdit(request);
}

ProductRoomEditorCursorResult applyProductRoomEditorMoveAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  return moveProductRoomEditorCursor(state, direction);
}

ProductRoomEditorCursorResult applyProductRoomEditorToolAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorTool tool) {
  return setProductRoomEditorTool(state, tool);
}

ProductRoomEditorCursorResult applyProductRoomEditorCycleToolAutomation(
    ProductRoomEditorCursorState state) {
  return cycleProductRoomEditorTool(state);
}

ProductRoomEditorCursorResult applyProductRoomEditorWallDirectionAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  return setProductRoomEditorWallDirection(state, direction);
}

ProductRoomEditorActionResult applyProductRoomEditorPlaceAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    ProductRoomAuthoringInputSource inputSource) {
  return applyProductRoomAuthoringCursorPlace({editing, cursor, inputSource});
}

ProductRoomEditorActionResult applyProductEditorInputAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionState& actions,
    ProductRoomAuthoringInputSource inputSource) {
  return applyProductRoomEditorActions(editing, cursor, actions, inputSource);
}

ProductRoomEditorActionResult applyProductRoomEditorMousePickAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    float screenX,
    float screenY,
    ProductViewportFrameConfig viewportConfig,
    Vec3 anchorWorld) {
  ProductRoomEditorMousePickRequest request;
  request.roomEditingReady = editing.ready;
  request.cursor = cursor;
  request.screenX = screenX;
  request.screenY = screenY;
  request.viewportConfig = viewportConfig;
  request.anchorWorld = anchorWorld;
  return applyProductRoomEditorMousePick(editing, request);
}

ProductRoomEditorPlacementPreviewResult buildProductRoomEditorPreviewAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor) {
  ProductRoomEditorPlacementPreviewRequest request;
  request.roomEditingReady = editing.ready;
  request.cursor = cursor;
  request.document = &editing.authoringSnapshot.document;
  return buildProductRoomEditorPlacementPreview(request);
}

ProductRoomEditingOperationResult confirmProductRoomEditorPreviewAutomation(
    ProductRoomEditingState& editing,
    const ProductRoomEditorPlacementPreviewResult& preview) {
  return applyProductRoomEditAutomation(
      {editing, ProductRoomAuthoringInputSource::Hotkey, preview.candidateCommand});
}

ProductRoomEditorPreviewInputResult baseProductRoomEditorPreviewInputResult(
    InputAction action) {
  ProductRoomEditorPreviewInputResult result;
  result.operation = std::string(inputActionName(action));
  result.handled = isProductRoomEditorPreviewInputAction(action);
  return result;
}

bool productRoomEditorPreviewMatchesCursor(const ProductAppWindowState& window) {
  const ProductRoomEditorPlacementPreviewResult& preview =
      window.roomEditorPlacementPreview;
  const ProductRoomEditorCursorState& cursor = window.roomEditorCursor;
  return window.roomEditorPreview.active && preview.ok &&
         preview.candidateCommandReady && preview.gridX == cursor.gridX &&
         preview.gridZ == cursor.gridZ &&
         preview.storyIndex == cursor.storyIndex &&
         preview.tool == productRoomEditorToolName(cursor.selectedTool) &&
         preview.wallDirection ==
             productRoomEditorDirectionName(cursor.wallDirection);
}

void recordProductRoomEditorPreviewInputReceipt(
    ProductAppWindowState& window,
    const ProductRoomEditorPreviewInputResult& result) {
  window.roomEditorCursorReady = window.roomEditing.ready;
  window.roomEditorStatus = result.status;
  window.roomEditorReasonCode = result.reasonCode;
  window.roomEditorLastOperation = result.operation;
  window.roomEditorLastOperationAccepted = result.operationAccepted;
  window.roomEditorLastPrimitiveId = result.primitiveId;
}

ProductRoomEditorPreviewInputResult rejectProductRoomEditorPreviewInputNotReady(
    ProductAppWindowState& window,
    InputAction action) {
  ProductRoomEditorPreviewInputResult result =
      baseProductRoomEditorPreviewInputResult(action);
  result.status = "room_editor_not_ready";
  result.reasonCode = result.status;
  recordProductRoomEditorPreviewInputReceipt(window, result);
  return result;
}

ProductRoomEditorPreviewInputResult applyProductRoomEditorPreviewInputBuild(
    ProductAppWindowState& window,
    InputAction action) {
  ProductRoomEditorPreviewInputResult result =
      baseProductRoomEditorPreviewInputResult(action);
  const ProductRoomEditorPlacementPreviewResult preview =
      buildProductRoomEditorPreviewAutomation(window.roomEditing,
                                              window.roomEditorCursor);
  recordProductRoomEditorPreviewResult(window, preview);
  result.ok = preview.ok;
  result.status = preview.status;
  result.reasonCode = preview.reasonCode;
  result.operationAccepted = preview.ok;
  result.primitiveId = preview.primitiveId;
  recordProductRoomEditorPreviewInputReceipt(window, result);
  return result;
}

ProductRoomEditorPreviewInputResult applyProductRoomEditorPreviewInputConfirm(
    ProductAppWindowState& window,
    InputAction action) {
  ProductRoomEditorPreviewInputResult result =
      baseProductRoomEditorPreviewInputResult(action);
  // branch-gate: BG-1054
  if (!productRoomEditorPreviewMatchesCursor(window)) {
    markProductRoomEditorPreviewCleared(window,
                                        "room_editor_preview_confirm_missing");
    result.status = "room_editor_preview_confirm_missing";
    result.reasonCode = result.status;
    recordProductRoomEditorPreviewInputReceipt(window, result);
    return result;
  }

  ProductRoomEditingOperationResult applied =
      confirmProductRoomEditorPreviewAutomation(
          window.roomEditing, window.roomEditorPlacementPreview);
  recordProductRoomEditingOperation(window, result.operation, applied);
  result.ok = applied.accepted;
  result.operationAccepted = applied.accepted;
  result.primitiveId = "none";
  // branch-gate: BG-1054
  if (!applied.edit.primitiveId.empty()) {
    result.primitiveId = applied.edit.primitiveId;
  }
  // branch-gate: BG-1054
  if (applied.accepted) {
    result.status = "room_editor_preview_confirmed";
    result.reasonCode = result.status;
  } else {
    result.status = applied.status;
    result.reasonCode = applied.reasonCode;
  }
  recordProductRoomEditorPreviewInputReceipt(window, result);
  return result;
}

ProductRoomEditorPreviewInputResult applyProductRoomEditorPreviewInputPlace(
    ProductAppWindowState& window,
    InputAction action) {
  // branch-gate: BG-1054
  if (productRoomEditorPreviewMatchesCursor(window)) {
    return applyProductRoomEditorPreviewInputConfirm(window, action);
  }
  return applyProductRoomEditorPreviewInputBuild(window, action);
}

ProductRoomEditorPreviewInputResult applyProductRoomEditorPreviewInputCancel(
    ProductAppWindowState& window,
    InputAction action) {
  ProductRoomEditorPreviewInputResult result =
      baseProductRoomEditorPreviewInputResult(action);
  markProductRoomEditorPreviewCleared(window, "room_editor_preview_cancelled");
  result.ok = true;
  result.status = "room_editor_preview_cancelled";
  result.reasonCode = result.status;
  result.operationAccepted = true;
  recordProductRoomEditorPreviewInputReceipt(window, result);
  return result;
}

bool isProductRoomEditorPreviewInputAction(InputAction action) {
  switch (action) {  // branch-gate: BG-1054
    case InputAction::EditorPlace:
    case InputAction::EditorApply:
    case InputAction::EditorPreviewPlacement:
    case InputAction::EditorConfirmPreview:
    case InputAction::EditorCancelPreview:
      return true;
    default:
      return false;
  }
}

ProductRoomEditorPreviewInputResult applyProductRoomEditorPreviewInputAction(
    ProductAppWindowState& window,
    InputAction action) {
  ProductRoomEditorPreviewInputResult result =
      baseProductRoomEditorPreviewInputResult(action);
  // branch-gate: BG-1054
  if (!result.handled) {
    return result;
  }
  // branch-gate: BG-1054
  if (!window.roomEditing.ready) {
    return rejectProductRoomEditorPreviewInputNotReady(window, action);
  }

  switch (action) {  // branch-gate: BG-1054
    case InputAction::EditorPlace:
      return applyProductRoomEditorPreviewInputPlace(window, action);
    case InputAction::EditorApply:
      return applyProductRoomEditorPreviewInputConfirm(window, action);
    case InputAction::EditorPreviewPlacement:
      return applyProductRoomEditorPreviewInputBuild(window, action);
    case InputAction::EditorConfirmPreview:
      return applyProductRoomEditorPreviewInputConfirm(window, action);
    case InputAction::EditorCancelPreview:
      return applyProductRoomEditorPreviewInputCancel(window, action);
    default:
      return result;
  }
}

namespace {

ProductAutomationExecutionResult unhandledRoomEditingAutomation() {
  return {};
}

ProductAutomationExecutionResult failRoomEditingAutomation() {
  return {true, false};
}

ProductAutomationExecutionResult passRoomEditingAutomation(bool accepted) {
  return {true, accepted};
}

ProductAutomationExecutionResult failRoomEditorPreviewAutomation(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context,
    std::string_view reasonCode) {
  context.window.automationControl.status = "command_failed";
  markProductRoomEditorPreviewCleared(context.window, reasonCode);
  markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                        context.currentOwner(), "failed");
  return failRoomEditingAutomation();
}

struct RoomEditorMousePickValue {
  bool valid = false;
  float screenX = 0.0F;
  float screenY = 0.0F;
};

RoomEditorMousePickValue parseRoomEditorMousePickValue(std::string_view value) {
  const std::vector<std::string_view> parts = splitProductAutomationCsv(value);
  RoomEditorMousePickValue parsed;
  // branch-gate: BG-1044
  if (parts.size() != 2U) {
    return parsed;
  }
  // branch-gate: BG-1044
  if (!parseProductAutomationFloat(parts[0], parsed.screenX) ||
      !parseProductAutomationFloat(parts[1], parsed.screenY)) {
    return parsed;
  }
  parsed.valid = true;
  return parsed;
}

}  // namespace

Vec3 productRoomEditorMousePickAnchor(Session* activeSession) {
  // branch-gate: BG-1044
  if (activeSession == nullptr) {
    return {};
  }
  for (const EntityState& entity : activeSession->state().world.entities()) {
    // branch-gate: BG-1044
    if (entity.active && entity.kind == EntityKind::Player) {
      return entity.transform.position;
    }
  }
  return {};
}

ProductViewportFrameConfig productRoomEditorMousePickViewportConfig(
    const ProductAppWindowState& window) {
  ProductViewportFrameConfig config;
  config.cameraYawDegrees = window.viewport.cameraYawDegrees;
  config.cameraPitchDegrees = window.viewport.cameraPitchDegrees;
  return config;
}

namespace {

bool roomEditorReady(ProductAutomationRoomEditingContext& context,
                     const ProductAutomationCommand& command,
                     std::string_view operation) {
  // branch-gate: BG-1006
  if (context.window.roomEditing.ready) {
    return true;
  }
  rejectProductRoomEditorNotReady(context.window, operation);
  markAutomationApplied(context.window, command, operation, context.currentOwner(),
                        "failed");
  return false;
}

ProductAutomationExecutionResult applyRoomEditingOperationResult(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context,
    ProductRoomEditingOperationResult& result) {
  recordProductRoomEditingOperation(context.window, automationSpec.canonicalKey,
                                    result);
  // branch-gate: BG-1006
  markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                        context.currentOwner(),
                        result.accepted ? "applied" : "failed");
  return passRoomEditingAutomation(result.accepted);
}

ProductAutomationExecutionResult applyRoomEditorCursorResult(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context,
    const ProductRoomEditorCursorResult& result) {
  recordProductRoomEditorCursorResult(context.window, automationSpec.canonicalKey,
                                      result);
  // branch-gate: BG-1006
  if (!result.ok) {
    context.window.automationControl.status = "command_failed";
  }
  // branch-gate: BG-1006
  markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                        context.currentOwner(),
                        result.ok ? "applied" : "failed");
  return passRoomEditingAutomation(result.ok);
}

}  // namespace

ProductAutomationExecutionResult applyProductRoomEditingAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context) {
  const std::string_view value{command.value};
  bool boolValue = false;

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditStart) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }

    const ProductAsciiRoomAuthoringRequest request =
        productAsciiRoomAuthoringRequestFromDraft(context.window);
    const ProductRoomEditingStartResult started =
        startProductRoomEditAutomationFromAsciiDraft({request});
    recordProductRoomEditingStart(context.window, started);
    // branch-gate: BG-1006
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          started.ok ? "applied" : "failed");
    return passRoomEditingAutomation(started.ok);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditStartActive) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }

    const ProductRoomEditingStartResult started =
        startProductRoomEditAutomationFromActiveRoom({activeRoom(context.window)});
    recordProductRoomEditingStart(context.window, started, automationSpec.canonicalKey);
    // branch-gate: BG-1006
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          started.ok ? "applied" : "failed");
    return passRoomEditingAutomation(started.ok);
  }

  // branch-gate: BG-1006
  if (command.key == "editor.input") {
    const std::vector<std::string_view> editorInputs =
        splitProductAutomationCsv(value);
    // branch-gate: BG-1006
    if (editorInputs.empty()) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }

    InputAction lastAction = InputAction::None;
    MenuOwner lastOwner = context.currentOwner();
    bool anyApplied = false;
    for (const std::string_view token : editorInputs) {
      InputAction editorAction = InputAction::None;
      float actionValue = 1.0F;
      // branch-gate: BG-1006
      if (!parseProductRoomEditorInputAction(token, editorAction, actionValue)) {
        context.window.automationControl.status = "invalid_value";
        return failRoomEditingAutomation();
      }
      lastAction = editorAction;
      // branch-gate: BG-1006
      if (context.frontend.screen != FrontendScreen::Gameplay ||
          !context.window.gameplayActive ||
          !context.activeSessionAvailable ||
          !context.window.roomEditing.ready) {
        rejectProductRoomEditorNotReady(context.window, inputActionName(editorAction));
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              context.currentOwner(), "failed");
        return failRoomEditingAutomation();
      }

      const InputRoutingResult routed = context.routeEditorInput(editorAction);
      lastOwner = routed.owner;
      context.window.lastInputAction = routed.action;
      context.window.lastInputAccepted = routed.accepted;
      syncProductWindowInputOwnerFromActiveSurface(context.frontend,
                                                  context.window);
      // branch-gate: BG-1006
      if (!routed.accepted || routed.owner != MenuOwner::Editor) {
        context.window.automationControl.status = "owner_unavailable";
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              routed.owner, "failed");
        return failRoomEditingAutomation();
      }

      // branch-gate: BG-1054
      if (isProductRoomEditorPreviewInputAction(editorAction)) {
        const ProductRoomEditorPreviewInputResult previewResult =
            applyProductRoomEditorPreviewInputAction(context.window, editorAction);
        // branch-gate: BG-1054
        if (!previewResult.ok) {
          context.window.automationControl.status = "command_failed";
          markAutomationApplied(context.window, command, inputActionName(editorAction),
                                routed.owner, "failed");
          return failRoomEditingAutomation();
        }
        anyApplied = true;
        continue;
      }

      ActionState actions;
      recordAction(actions, editorAction, true, true, false, actionValue);
      const ProductRoomEditorActionResult result =
          applyProductEditorInputAutomation(
              context.window.roomEditing, context.window.roomEditorCursor, actions,
              ProductRoomAuthoringInputSource::Hotkey);
      recordProductRoomEditorActionResult(context.window, result);
      // branch-gate: BG-1006
      if (!result.ok) {
        context.window.automationControl.status = "command_failed";
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              routed.owner, "failed");
        return failRoomEditingAutomation();
      }
      anyApplied = true;
    }

    // branch-gate: BG-1006
    if (!anyApplied) {
      context.window.automationControl.status = "command_failed";
      markAutomationApplied(context.window, command, "editor.input", lastOwner,
                            "failed");
      return failRoomEditingAutomation();
    }
    markAutomationApplied(context.window, command, inputActionName(lastAction),
                          lastOwner, "applied");
    return passRoomEditingAutomation(true);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorMove) {
    ProductRoomEditorDirection direction = ProductRoomEditorDirection::Up;
    // branch-gate: BG-1006
    if (!parseProductRoomEditorDirection(value, direction)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorMoveAutomation(context.window.roomEditorCursor,
                                             direction));
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorTool) {
    ProductRoomEditorTool tool = ProductRoomEditorTool::Floor;
    // branch-gate: BG-1006
    if (!parseProductRoomEditorTool(value, tool)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorToolAutomation(context.window.roomEditorCursor, tool));
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorCycleTool) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorCycleToolAutomation(context.window.roomEditorCursor));
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorWallDirection) {
    ProductRoomEditorDirection direction = ProductRoomEditorDirection::Up;
    // branch-gate: BG-1006
    if (!parseProductRoomEditorDirection(value, direction)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorWallDirectionAutomation(
            context.window.roomEditorCursor, direction));
  }

  // branch-gate: BG-1044
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorMousePick) {
    const RoomEditorMousePickValue pick = parseRoomEditorMousePickValue(value);
    // branch-gate: BG-1044
    if (!pick.valid) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1044
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    const ProductRoomEditorActionResult result =
        applyProductRoomEditorMousePickAutomation(
            context.window.roomEditing,
            context.window.roomEditorCursor,
            pick.screenX,
            pick.screenY,
            productRoomEditorMousePickViewportConfig(context.window),
            productRoomEditorMousePickAnchor(context.activeSession));
    recordProductRoomEditorActionResult(context.window, result,
                                        automationSpec.canonicalKey);
    // branch-gate: BG-1044
    if (!result.ok) {
      context.window.automationControl.status = "command_failed";
    }
    // branch-gate: BG-1044
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          result.ok ? "applied" : "failed");
    return passRoomEditingAutomation(result.ok);
  }

  // branch-gate: BG-1049
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorPreview) {
    // branch-gate: BG-1049
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1049
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1049
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    const ProductRoomEditorPlacementPreviewResult result =
        buildProductRoomEditorPreviewAutomation(context.window.roomEditing,
                                                context.window.roomEditorCursor);
    recordProductRoomEditorPreviewResult(context.window, result);
    // branch-gate: BG-1049
    if (!result.ok) {
      context.window.automationControl.status = "command_failed";
    }
    // branch-gate: BG-1049
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          result.ok ? "applied" : "failed");
    return passRoomEditingAutomation(result.ok);
  }

  // branch-gate: BG-1051
  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorPreviewConfirm) {
    // branch-gate: BG-1051
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1051
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1051
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1051
    if (!context.window.roomEditorPreview.active ||
        !context.window.roomEditorPlacementPreview.ok ||
        !context.window.roomEditorPlacementPreview.candidateCommandReady) {
      return failRoomEditorPreviewAutomation(
          command, automationSpec, context, "room_editor_preview_confirm_missing");
    }

    ProductRoomEditingOperationResult result =
        confirmProductRoomEditorPreviewAutomation(
            context.window.roomEditing, context.window.roomEditorPlacementPreview);
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1051
  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorPreviewCancel) {
    // branch-gate: BG-1051
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1051
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1051
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    markProductRoomEditorPreviewCleared(context.window,
                                        "room_editor_preview_cancelled");
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(), "applied");
    return passRoomEditingAutomation(true);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorPlace) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    const ProductRoomEditorActionResult result =
        applyProductRoomEditorPlaceAutomation(
            context.window.roomEditing, context.window.roomEditorCursor,
            ProductRoomAuthoringInputSource::Hotkey);
    recordProductRoomEditorActionResult(context.window, result,
                                        automationSpec.canonicalKey);
    // branch-gate: BG-1006
    if (!result.ok) {
      context.window.automationControl.status = "command_failed";
    }
    // branch-gate: BG-1006
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          result.ok ? "applied" : "failed");
    return passRoomEditingAutomation(result.ok);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditAddFloor) {
    RoomEditCommand edit;
    // branch-gate: BG-1006
    if (!parseProductAutomationFloorCommand(value, edit)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditAddWall) {
    RoomEditCommand edit;
    // branch-gate: BG-1006
    if (!parseProductAutomationWallCommand(value, edit)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditDeleteFloor) {
    // branch-gate: BG-1006
    if (value.empty()) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    const RoomEditCommand edit = deleteFloorCommand(std::string(value));
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditDeleteWall) {
    // branch-gate: BG-1006
    if (value.empty()) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    const RoomEditCommand edit = deleteWallCommand(std::string(value));
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditUndo) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    ProductRoomEditingOperationResult result =
        undoProductRoomEditAutomation(
            {context.window.roomEditing, ProductRoomAuthoringInputSource::Script});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditRedo) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    ProductRoomEditingOperationResult result =
        redoProductRoomEditAutomation(
            {context.window.roomEditing, ProductRoomAuthoringInputSource::Script});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  return unhandledRoomEditingAutomation();
}

}  // namespace iggy3d
