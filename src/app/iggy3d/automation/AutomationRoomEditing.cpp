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
  window.creativeAuthoring.roomEditorPreview.active = false;
  window.creativeAuthoring.roomEditorPlacementPreview = {};
  window.creativeAuthoring.roomEditorPreview.visible = false;
  window.creativeAuthoring.roomEditorPreview.status = "room_editor_preview_not_requested";
  window.creativeAuthoring.roomEditorPreview.reasonCode = "room_editor_preview_not_requested";
  window.creativeAuthoring.roomEditorPreview.candidateId = "none";
  window.creativeAuthoring.roomEditorPreview.tool = "floor";
  window.creativeAuthoring.roomEditorPreview.gridX = 0;
  window.creativeAuthoring.roomEditorPreview.gridZ = 0;
  window.creativeAuthoring.roomEditorPreview.beforeDrawCount = 0;
  window.creativeAuthoring.roomEditorPreview.afterDrawCount = 0;
  window.creativeAuthoring.roomEditorPreview.avoidedDrawCountDelta = 0;
  window.creativeAuthoring.roomEditorPreview.beforeTriangleCount = 0;
  window.creativeAuthoring.roomEditorPreview.afterTriangleCount = 0;
  window.creativeAuthoring.roomEditorPreview.avoidedTriangleCountDelta = 0;
  window.creativeAuthoring.roomEditorPreview.optimizedDrawDelta = 0;
  window.creativeAuthoring.roomEditorPreview.optimizedTriangleDelta = 0;
}

void markProductRoomEditorPreviewCleared(ProductAppWindowState& window,
                                         std::string_view status) {
  clearProductRoomEditorPreview(window);
  window.creativeAuthoring.roomEditorPreview.status = std::string(status);
  window.creativeAuthoring.roomEditorPreview.reasonCode = window.creativeAuthoring.roomEditorPreview.status;
}

void recordProductRoomEditorPreviewResult(
    ProductAppWindowState& window,
    const ProductRoomEditorPlacementPreviewResult& result) {
  window.creativeAuthoring.roomEditorPreview.active = true;
  window.creativeAuthoring.roomEditorPlacementPreview = result;
  window.creativeAuthoring.roomEditorPreview.visible = result.ok;
  window.creativeAuthoring.roomEditorPreview.status = result.status;
  window.creativeAuthoring.roomEditorPreview.reasonCode = result.reasonCode;
  window.creativeAuthoring.roomEditorPreview.candidateId = result.primitiveId;
  window.creativeAuthoring.roomEditorPreview.tool = result.tool;
  window.creativeAuthoring.roomEditorPreview.gridX = result.gridX;
  window.creativeAuthoring.roomEditorPreview.gridZ = result.gridZ;
  window.creativeAuthoring.roomEditorPreview.beforeDrawCount = result.before.optimizedDrawCount;
  window.creativeAuthoring.roomEditorPreview.afterDrawCount = result.after.optimizedDrawCount;
  window.creativeAuthoring.roomEditorPreview.avoidedDrawCountDelta =
      signedRoomEditorPreviewDelta(result.before.drawCountAvoided,
                                   result.after.drawCountAvoided);
  window.creativeAuthoring.roomEditorPreview.beforeTriangleCount =
      result.before.optimizedTriangleCount;
  window.creativeAuthoring.roomEditorPreview.afterTriangleCount = result.after.optimizedTriangleCount;
  window.creativeAuthoring.roomEditorPreview.avoidedTriangleCountDelta =
      signedRoomEditorPreviewDelta(result.before.triangleCountAvoided,
                                   result.after.triangleCountAvoided);
  window.creativeAuthoring.roomEditorPreview.optimizedDrawDelta = result.optimizedDrawDelta;
  window.creativeAuthoring.roomEditorPreview.optimizedTriangleDelta = result.optimizedTriangleDelta;
}

void copyRoomEditingStateToWindow(ProductAppWindowState& window,
                                  const ProductRoomEditingState& state) {
  window.creativeAuthoring.roomEditing = state;
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
  window.creativeAuthoring.roomEditingLastOperation = std::string(operation);
  window.creativeAuthoring.roomEditingLastOperationStatus = result.status;
  window.creativeAuthoring.roomEditingLastOperationReasonCode = result.reasonCode;
  window.creativeAuthoring.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
  window.creativeAuthoring.roomEditingLastOperationAccepted = result.ok;
  window.creativeAuthoring.roomEditingLastPrimitiveId = "none";
  clearProductRoomEditorPreview(window);
  copyRoomEditingStateToWindow(window, result.state);
  // branch-gate: BG-1006
  if (result.ok) {
    window.inputDevice.interactionMode = ProductInteractionMode::Creative;
    window.creativeAuthoring.roomEditorCursorReady = true;
    window.creativeAuthoring.roomEditorCursor = ProductRoomEditorCursorState{};
    window.creativeAuthoring.roomEditorStatus = "room_editor_cursor_ready";
    window.creativeAuthoring.roomEditorReasonCode = "room_editor_cursor_ready";
    window.creativeAuthoring.roomEditorLastOperation = "none";
    window.creativeAuthoring.roomEditorLastOperationAccepted = false;
    window.creativeAuthoring.roomEditorLastPrimitiveId = "none";
  }
}

bool recordProductRoomEditingLeave(const FrontendState& frontend,
                                   ProductAppWindowState& window,
                                   std::string_view operation) {
  (void)frontend;
  window.creativeAuthoring.roomEditingLastOperation = std::string(operation);
  window.creativeAuthoring.roomEditingLastPrimitiveId = "none";
  if (!window.creativeAuthoring.roomEditing.ready) {  // branch-gate: BG-1006
    window.creativeAuthoring.roomEditingLastOperationStatus = "room_editor_not_ready";
    window.creativeAuthoring.roomEditingLastOperationReasonCode = "room_editor_not_ready";
    window.creativeAuthoring.roomEditingLastInputSource =
        productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
    window.creativeAuthoring.roomEditingLastOperationAccepted = false;
    rejectProductRoomEditorNotReady(window, operation);
    return false;
  }

  ProductRoomEditingState leftState;
  leftState.status = "product_room_editing_left";
  leftState.reasonCode = leftState.status;
  copyRoomEditingStateToWindow(window, leftState);
  window.creativeAuthoring.roomEditingLastOperationStatus = "product_room_editing_left";
  window.creativeAuthoring.roomEditingLastOperationReasonCode = window.creativeAuthoring.roomEditingLastOperationStatus;
  window.creativeAuthoring.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
  window.creativeAuthoring.roomEditingLastOperationAccepted = true;
  window.inputDevice.interactionMode = ProductInteractionMode::Player;
  window.creativeAuthoring.roomEditorCursorReady = false;
  window.creativeAuthoring.roomEditorStatus = "room_editor_not_ready";
  window.creativeAuthoring.roomEditorReasonCode = "room_editor_not_ready";
  window.creativeAuthoring.roomEditorLastOperation = std::string(operation);
  window.creativeAuthoring.roomEditorLastOperationAccepted = true;
  window.creativeAuthoring.roomEditorLastPrimitiveId = "none";
  window.creativeAuthoring.roomEditorOverlay.visible = false;
  window.creativeAuthoring.roomEditorOverlay.status = "room_editor_overlay_not_ready";
  window.creativeAuthoring.roomEditorOverlay.reasonCode = window.creativeAuthoring.roomEditorOverlay.status;
  window.creativeAuthoring.roomEditorOverlay.itemCount = 0;
  window.creativeAuthoring.roomEditorHud.visible = false;
  window.creativeAuthoring.roomEditorHud.status = "room_editor_hud_not_ready";
  window.creativeAuthoring.roomEditorHud.reasonCode = window.creativeAuthoring.roomEditorHud.status;
  window.creativeAuthoring.roomEditorHud.lastOperation = std::string(operation);
  window.creativeAuthoring.roomEditorHud.lastOperationAccepted = true;
  window.creativeAuthoring.roomEditorHud.lastPrimitiveId = "none";
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
  window.creativeAuthoring.roomEditingLastOperation = std::string(operation);
  window.creativeAuthoring.roomEditingLastOperationStatus = result.status;
  window.creativeAuthoring.roomEditingLastOperationReasonCode = result.reasonCode;
  window.creativeAuthoring.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(result.inputSource);
  window.creativeAuthoring.roomEditingLastOperationAccepted = result.accepted;
  // branch-gate: BG-1006
  window.creativeAuthoring.roomEditingLastPrimitiveId =
      result.edit.primitiveId.empty() ? std::string{"none"}
                                      : result.edit.primitiveId;
  clearProductRoomEditorPreview(window);
  copyRoomEditingStateToWindow(window, result.state);
}

void recordProductRoomEditorCursorResult(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditorCursorResult& result) {
  window.creativeAuthoring.roomEditorCursorReady = window.creativeAuthoring.roomEditing.ready;
  window.creativeAuthoring.roomEditorCursor = result.state;
  window.creativeAuthoring.roomEditorStatus = result.status;
  window.creativeAuthoring.roomEditorReasonCode = result.reasonCode;
  window.creativeAuthoring.roomEditorLastOperation = std::string(operation);
  window.creativeAuthoring.roomEditorLastOperationAccepted = result.ok;
  window.creativeAuthoring.roomEditorLastPrimitiveId = "none";
  clearProductRoomEditorPreview(window);
}

bool rejectProductRoomEditorNotReady(ProductAppWindowState& window,
                                     std::string_view operation) {
  window.creativeAuthoring.roomEditorCursorReady = false;
  window.creativeAuthoring.roomEditorStatus = "room_editor_not_ready";
  window.creativeAuthoring.roomEditorReasonCode = "room_editor_not_ready";
  window.creativeAuthoring.roomEditorLastOperation = std::string(operation);
  window.creativeAuthoring.roomEditorLastOperationAccepted = false;
  window.creativeAuthoring.roomEditorLastPrimitiveId = "none";
  window.automationControl.status = "command_failed";
  return false;
}

void recordProductRoomEditorActionResult(
    ProductAppWindowState& window,
    const ProductRoomEditorActionResult& result,
    std::string_view operationOverride) {
  window.creativeAuthoring.roomEditing = result.editing;
  copyRoomEditingStateToWindow(window, result.editing);
  window.creativeAuthoring.roomEditorCursorReady = result.editing.ready;
  window.creativeAuthoring.roomEditorCursor = result.cursor;
  window.creativeAuthoring.roomEditorStatus = result.status;
  window.creativeAuthoring.roomEditorReasonCode = result.reasonCode;
  // branch-gate: BG-1006
  window.creativeAuthoring.roomEditorLastOperation =
      operationOverride.empty() ? result.operation : std::string(operationOverride);
  window.creativeAuthoring.roomEditorLastOperationAccepted = result.operationAccepted;
  window.creativeAuthoring.roomEditorLastPrimitiveId = result.primitiveId;
  clearProductRoomEditorPreview(window);

  // branch-gate: BG-1006
  if (result.status == "room_editor_command_applied") {
    window.creativeAuthoring.roomEditingLastOperation = window.creativeAuthoring.roomEditorLastOperation;
    window.creativeAuthoring.roomEditingLastOperationStatus = "product_room_editing_edit_applied";
    window.creativeAuthoring.roomEditingLastOperationReasonCode = "product_room_editing_edit_applied";
    window.creativeAuthoring.roomEditingLastInputSource =
        productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Hotkey);
    window.creativeAuthoring.roomEditingLastOperationAccepted = result.operationAccepted;
    window.creativeAuthoring.roomEditingLastPrimitiveId = result.primitiveId;
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
      window.creativeAuthoring.roomEditorPlacementPreview;
  const ProductRoomEditorCursorState& cursor = window.creativeAuthoring.roomEditorCursor;
  return window.creativeAuthoring.roomEditorPreview.active && preview.ok &&
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
  window.creativeAuthoring.roomEditorCursorReady = window.creativeAuthoring.roomEditing.ready;
  window.creativeAuthoring.roomEditorStatus = result.status;
  window.creativeAuthoring.roomEditorReasonCode = result.reasonCode;
  window.creativeAuthoring.roomEditorLastOperation = result.operation;
  window.creativeAuthoring.roomEditorLastOperationAccepted = result.operationAccepted;
  window.creativeAuthoring.roomEditorLastPrimitiveId = result.primitiveId;
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
      buildProductRoomEditorPreviewAutomation(window.creativeAuthoring.roomEditing,
                                              window.creativeAuthoring.roomEditorCursor);
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
          window.creativeAuthoring.roomEditing, window.creativeAuthoring.roomEditorPlacementPreview);
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
  if (!window.creativeAuthoring.roomEditing.ready) {
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
  if (context.window.creativeAuthoring.roomEditing.ready) {
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
          !context.window.gameplay.gameplayActive ||
          !context.activeSessionAvailable ||
          !context.window.creativeAuthoring.roomEditing.ready) {
        rejectProductRoomEditorNotReady(context.window, inputActionName(editorAction));
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              context.currentOwner(), "failed");
        return failRoomEditingAutomation();
      }

      const InputRoutingResult routed = context.routeEditorInput(editorAction);
      lastOwner = routed.owner;
      context.window.inputDevice.lastInputAction = routed.action;
      context.window.inputDevice.lastInputAccepted = routed.accepted;
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
              context.window.creativeAuthoring.roomEditing, context.window.creativeAuthoring.roomEditorCursor, actions,
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
        applyProductRoomEditorMoveAutomation(context.window.creativeAuthoring.roomEditorCursor,
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
        applyProductRoomEditorToolAutomation(context.window.creativeAuthoring.roomEditorCursor, tool));
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
        applyProductRoomEditorCycleToolAutomation(context.window.creativeAuthoring.roomEditorCursor));
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
            context.window.creativeAuthoring.roomEditorCursor, direction));
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
            context.window.creativeAuthoring.roomEditing,
            context.window.creativeAuthoring.roomEditorCursor,
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
        buildProductRoomEditorPreviewAutomation(context.window.creativeAuthoring.roomEditing,
                                                context.window.creativeAuthoring.roomEditorCursor);
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
    if (!context.window.creativeAuthoring.roomEditorPreview.active ||
        !context.window.creativeAuthoring.roomEditorPlacementPreview.ok ||
        !context.window.creativeAuthoring.roomEditorPlacementPreview.candidateCommandReady) {
      return failRoomEditorPreviewAutomation(
          command, automationSpec, context, "room_editor_preview_confirm_missing");
    }

    ProductRoomEditingOperationResult result =
        confirmProductRoomEditorPreviewAutomation(
            context.window.creativeAuthoring.roomEditing, context.window.creativeAuthoring.roomEditorPlacementPreview);
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
            context.window.creativeAuthoring.roomEditing, context.window.creativeAuthoring.roomEditorCursor,
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
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
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
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
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
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
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
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
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
            {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script});
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
            {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  return unhandledRoomEditingAutomation();
}

}  // namespace iggy3d
