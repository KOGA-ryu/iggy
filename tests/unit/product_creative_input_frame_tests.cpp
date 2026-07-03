#include "app/iggy3d/window/CreativeInputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::ProductAppWindowState creativeWindow() {
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  return window;
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

bool activeRuleUsesOnlyInteractionMode() {
  iggy3d::ProductAppWindowState window;
  const bool playerActive = iggy3d::productCreativeInputActiveForWindow(window);
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const bool creativeActive =
      iggy3d::productCreativeInputActiveForWindow(window);

  return expect(!playerActive, "player inactive") &&
         expect(creativeActive, "creative active");
}

bool nullWindowReturnsWindowMissing() {
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame({});

  return expect(!receipt.requested, "null not requested") &&
         expect(!receipt.active, "null inactive") &&
         expect(!receipt.facadeAvailable, "null no facade") &&
         expect(!receipt.accepted, "null not accepted") &&
         expect(!receipt.changed, "null unchanged") &&
         expect(receipt.status == "product_creative_input_window_missing",
                "null status") &&
         expect(receipt.reasonCode == "product_creative_input_window_missing",
                "null reason");
}

bool inactiveWindowNoopsAndDoesNotMutateFacade() {
  iggy3d::ProductAppWindowState window;
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.action = iggy3d::InputAction::EditorNextTool;
  request.click = clickAt(10.0F, 20.0F);
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(!receipt.requested, "inactive not requested") &&
         expect(!receipt.active, "inactive active false") &&
         expect(!receipt.actionHandled, "inactive action not handled") &&
         expect(!receipt.pointerDispatched, "inactive pointer not dispatched") &&
         expect(receipt.status == "product_creative_input_inactive",
                "inactive status") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "inactive tool unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "inactive document unchanged");
}

bool activeCreativeNullFacadeReportsMissing() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.action = iggy3d::InputAction::EditorNextTool;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.requested, "missing facade requested") &&
         expect(receipt.active, "missing facade active") &&
         expect(!receipt.facadeAvailable, "missing facade unavailable") &&
         expect(!receipt.actionHandled, "missing facade action untouched") &&
         expect(receipt.status == "product_creative_input_facade_missing",
                "missing facade status");
}

bool toolOrderCycles() {
  return expect(iggy3d::nextProductCreativeTool(cr::Tool::Select) ==
                    cr::Tool::Inspect,
                "next select inspect") &&
         expect(iggy3d::nextProductCreativeTool(cr::Tool::Inspect) ==
                    cr::Tool::Measure,
                "next inspect measure") &&
         expect(iggy3d::nextProductCreativeTool(cr::Tool::Measure) ==
                    cr::Tool::Select,
                "next measure select") &&
         expect(iggy3d::previousProductCreativeTool(cr::Tool::Select) ==
                    cr::Tool::Measure,
                "previous select measure") &&
         expect(iggy3d::previousProductCreativeTool(cr::Tool::Measure) ==
                    cr::Tool::Inspect,
                "previous measure inspect") &&
         expect(iggy3d::previousProductCreativeTool(cr::Tool::Inspect) ==
                    cr::Tool::Select,
                "previous inspect select");
}

bool toolActionMappingUsesFlatRows() {
  cr::Tool out = cr::Tool::Measure;
  bool ok = true;
  ok &= expect(iggy3d::productCreativeToolActionTarget(
                   iggy3d::InputAction::EditorNextTool,
                   cr::Tool::Select,
                   out) &&
                   out == cr::Tool::Inspect,
               "next action maps");
  ok &= expect(iggy3d::productCreativeToolActionTarget(
                   iggy3d::InputAction::EditorPreviousTool,
                   cr::Tool::Select,
                   out) &&
                   out == cr::Tool::Measure,
               "previous action maps");
  ok &= expect(iggy3d::productCreativeToolActionTarget(
                   iggy3d::InputAction::EditorSelect,
                   cr::Tool::Measure,
                   out) &&
                   out == cr::Tool::Select,
               "editor select maps");
  ok &= expect(iggy3d::productCreativeToolActionTarget(
                   iggy3d::InputAction::EditorSelectFloorTool,
                   cr::Tool::Inspect,
                   out) &&
                   out == cr::Tool::Select,
               "floor tool maps select");
  ok &= expect(iggy3d::productCreativeToolActionTarget(
                   iggy3d::InputAction::EditorSelectWallTool,
                   cr::Tool::Select,
                   out) &&
                   out == cr::Tool::Inspect,
               "wall tool maps inspect");
  out = cr::Tool::Measure;
  ok &= expect(!iggy3d::productCreativeToolActionTarget(
                   iggy3d::InputAction::MenuBack,
                   cr::Tool::Select,
                   out) &&
                   out == cr::Tool::Measure,
               "non-tool action rejected");
  return ok;
}

bool clickPacketMapsMouseClick() {
  const iggy3d::MouseClick click = clickAt(12.5F, 34.25F);
  const cr::CreativeToolInputPacket packet =
      iggy3d::productCreativePointerPressPacket(click);

  return expect(packet.kind == cr::CreativeToolInputKind::PointerPress,
                "packet press") &&
         expect(packet.pointer.button == cr::CreativeToolPointerButton::Primary,
                "packet primary") &&
         expect(packet.pointer.x == 12.5, "packet x") &&
         expect(packet.pointer.y == 34.25, "packet y") &&
         expect(packet.pointer.modifiers == cr::kCreativeToolModifierNone,
                "packet modifiers none") &&
         expect(packet.pointer.target.value == cr::kInvalidId,
                "packet target invalid");
}

bool editorNextToolChangesFacadeToolWithoutDocumentMutation() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.action = iggy3d::InputAction::EditorNextTool;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.requested, "tool action requested") &&
         expect(receipt.active, "tool action active") &&
         expect(receipt.facadeAvailable, "tool action facade") &&
         expect(receipt.actionHandled, "tool action handled") &&
         expect(receipt.toolChanged, "tool action changed") &&
         expect(receipt.accepted, "tool action accepted") &&
         expect(receipt.changed, "tool action receipt changed") &&
         expect(receipt.activeToolBefore == cr::Tool::Select,
                "tool before select") &&
         expect(receipt.activeToolAfter == cr::Tool::Inspect,
                "tool after inspect") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "facade active inspect") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "tool action document unchanged");
}

bool clickWithMeasureActiveBeginsMeasurement() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(48.0F, 96.5F);
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "measure pointer dispatched") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::PointerPress,
                "measure input kind press") &&
         expect(receipt.emittedIntentCount == 1U,
                "measure emitted begin intent") &&
         expect(receipt.accepted, "measure click accepted") &&
         expect(receipt.changed, "measure click changed") &&
         expect(receipt.activeToolBefore == cr::Tool::Measure,
                "measure before") &&
         expect(receipt.activeToolAfter == cr::Tool::Measure,
                "measure after") &&
         expect(facade.measurementState().active,
                "measurement active after click") &&
         expect(facade.measurementState().hasMeasurement,
                "measurement exists after click") &&
         expect(facade.measurementState().startPoint.x == 48.0,
                "measurement start x") &&
         expect(facade.measurementState().startPoint.y == 96.5,
                "measurement start y");
}

bool editorCancelPreviewCancelsActiveMeasurement() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.facade = &facade;
  beginRequest.click = clickAt(10.0F, 10.0F);
  static_cast<void>(iggy3d::processProductCreativeInputFrame(beginRequest));

  iggy3d::ProductCreativeInputFrameRequest cancelRequest;
  cancelRequest.window = &window;
  cancelRequest.facade = &facade;
  cancelRequest.action = iggy3d::InputAction::EditorCancelPreview;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(cancelRequest);

  return expect(receipt.cancelDispatched, "cancel dispatched") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::Cancel,
                "cancel input kind") &&
         expect(receipt.emittedIntentCount == 1U, "cancel emitted intent") &&
         expect(receipt.accepted, "cancel accepted") &&
         expect(receipt.changed, "cancel changed") &&
         expect(!facade.measurementState().active,
                "measurement inactive after cancel") &&
         expect(!facade.measurementState().hasMeasurement,
                "measurement cleared after cancel");
}

bool noApplicableInputReturnsNoop() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.requested, "noop requested") &&
         expect(receipt.active, "noop active") &&
         expect(receipt.facadeAvailable, "noop facade available") &&
         expect(!receipt.actionHandled, "noop action false") &&
         expect(!receipt.pointerDispatched, "noop pointer false") &&
         expect(!receipt.cancelDispatched, "noop cancel false") &&
         expect(!receipt.accepted, "noop not accepted") &&
         expect(!receipt.changed, "noop unchanged") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::Unknown,
                "noop input kind unknown") &&
         expect(receipt.emittedIntentCount == 0U, "noop emitted zero") &&
         expect(receipt.status == "product_creative_input_noop",
                "noop status");
}

}  // namespace

int main() {
  bool ok = true;
  ok &= activeRuleUsesOnlyInteractionMode();
  ok &= nullWindowReturnsWindowMissing();
  ok &= inactiveWindowNoopsAndDoesNotMutateFacade();
  ok &= activeCreativeNullFacadeReportsMissing();
  ok &= toolOrderCycles();
  ok &= toolActionMappingUsesFlatRows();
  ok &= clickPacketMapsMouseClick();
  ok &= editorNextToolChangesFacadeToolWithoutDocumentMutation();
  ok &= clickWithMeasureActiveBeginsMeasurement();
  ok &= editorCancelPreviewCancelsActiveMeasurement();
  ok &= noApplicableInputReturnsNoop();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
