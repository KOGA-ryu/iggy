#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/view/ViewportFraming.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/input/InputAction.hpp"
#include "app/input/InputRouter.hpp"

namespace iggy3d {

struct FrontendState;
struct ProductAppWindowState;
class Session;
struct ProductRoomEditingState;
struct ProductRoomEditingStartResult;
struct ProductRoomEditingOperationResult;
struct ProductRoomEditorActionResult;
struct ProductRoomEditorCursorResult;
struct ProductRoomEditorPlacementPreviewResult;
struct ProductAsciiRoomAuthoringRequest;
struct ProductActiveRoomState;
struct ProductRoomAuthoringStartFromAsciiRequest;
struct ProductRoomAuthoringStartFromActiveRoomRequest;
struct ProductRoomAuthoringEditCommandRequest;
struct ProductRoomAuthoringUndoRedoRequest;

struct ProductRoomEditorPreviewInputResult {
  bool handled = false;
  bool ok = false;
  std::string status = "room_editor_preview_action_ignored";
  std::string reasonCode = "room_editor_preview_action_ignored";
  std::string operation = "none";
  bool operationAccepted = false;
  std::string primitiveId = "none";
};

struct ProductAutomationRoomEditingContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
  Session* activeSession = nullptr;
  bool activeSessionAvailable = false;
  std::function<MenuOwner()> currentOwner;
  std::function<InputRoutingResult(InputAction)> routeEditorInput;
};

void copyRoomEditingStateToWindow(ProductAppWindowState& window,
                                  const ProductRoomEditingState& state);

void recordProductRoomEditingStart(ProductAppWindowState& window,
                                   const ProductRoomEditingStartResult& result,
                                   std::string_view operation = "room_edit.start");

bool recordProductRoomEditingLeave(ProductAppWindowState& window,
                                   std::string_view operation = "pause_leave_editor");

void recordProductRoomEditingOperation(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditingOperationResult& result);

void recordProductRoomEditorCursorResult(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditorCursorResult& result);

bool rejectProductRoomEditorNotReady(ProductAppWindowState& window,
                                     std::string_view operation);

void recordProductRoomEditorActionResult(
    ProductAppWindowState& window,
    const ProductRoomEditorActionResult& result,
    std::string_view operationOverride = {});

void clearProductRoomEditorPreview(ProductAppWindowState& window);

void markProductRoomEditorPreviewCleared(ProductAppWindowState& window,
                                         std::string_view status);

void recordProductRoomEditorPreviewResult(
    ProductAppWindowState& window,
    const ProductRoomEditorPlacementPreviewResult& result);

Vec3 productRoomEditorMousePickAnchor(Session* activeSession);

ProductViewportFrameConfig productRoomEditorMousePickViewportConfig(
    const ProductAppWindowState& window);

ProductRoomEditorActionResult applyProductRoomEditorMousePickAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    float screenX,
    float screenY,
    ProductViewportFrameConfig viewportConfig,
    Vec3 anchorWorld);

ProductRoomEditorPlacementPreviewResult buildProductRoomEditorPreviewAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor);

ProductRoomEditingOperationResult confirmProductRoomEditorPreviewAutomation(
    ProductRoomEditingState& editing,
    const ProductRoomEditorPlacementPreviewResult& preview);

bool isProductRoomEditorPreviewInputAction(InputAction action);

ProductRoomEditorPreviewInputResult applyProductRoomEditorPreviewInputAction(
    ProductAppWindowState& window,
    InputAction action);

ProductRoomEditingStartResult startProductRoomEditAutomationFromAsciiDraft(
    ProductRoomAuthoringStartFromAsciiRequest request);

ProductRoomEditingStartResult startProductRoomEditAutomationFromActiveRoom(
    ProductRoomAuthoringStartFromActiveRoomRequest request);

ProductRoomEditingOperationResult applyProductRoomEditAutomation(
    ProductRoomAuthoringEditCommandRequest request);

ProductRoomEditingOperationResult undoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request);

ProductRoomEditingOperationResult redoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request);

ProductAutomationExecutionResult applyProductRoomEditingAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context);

}  // namespace iggy3d
