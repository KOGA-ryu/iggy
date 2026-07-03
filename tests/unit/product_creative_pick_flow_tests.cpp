#include "app/iggy3d/window/CreativeInputFrame.hpp"
#include "app/iggy3d/window/CreativeUiInputFrame.hpp"
#include "app/iggy3d/window/CreativeViewportPickFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/menu/CreativeUiDrawList.hpp"
#include "app/input/ActionState.hpp"

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

cr::CreativeSpatialProjectionRequest projectionRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {4, 4, 2};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

cr::CreativeViewportPickViewport viewport() {
  return {0.0F, 0.0F, 400.0F, 400.0F};
}

cr::Facade facadeWithRoom() {
  cr::Facade facade;
  cr::CreateRoomCommand command;
  command.name = "Room";
  command.bounds.min = {1.0, 2.0, 1.0};
  command.bounds.max = {2.0, 3.0, 2.0};
  (void)facade.createRoom(command);
  return facade;
}

iggy3d::ActionState singlePressedAction(iggy3d::InputAction action) {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, action, true, true, false, 1.0F);
  return actions;
}

const cr::CreativeUiRow* findUiRow(const cr::CreativeUiModel& model,
                                   cr::CreativeUiPanelKind panel,
                                   cr::CreativeUiRowKind kind) {
  for (const cr::CreativeUiRow& row : model.rows) {
    if (row.panel == panel && row.kind == kind) {
      return &row;
    }
  }
  return nullptr;
}

const iggy3d::ProductUiPrimitive* findPrimitive(
    const iggy3d::ProductUiDrawList& list,
    std::string_view semanticId) {
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    if (primitive.semanticId == semanticId) {
      return &primitive;
    }
  }
  return nullptr;
}

iggy3d::ProductCreativeViewportPickFrameReceipt pickRoomAt(
    iggy3d::ProductAppWindowState& window,
    cr::Facade& facade,
    iggy3d::MouseClick click,
    bool suppressed = false) {
  iggy3d::ProductCreativeViewportPickFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = click;
  request.downstreamClickSuppressed = suppressed;
  request.viewport = viewport();
  request.projectionRequest = projectionRequest();
  request.z = 1;
  return iggy3d::routeProductCreativeViewportPickFrame(request);
}

iggy3d::ProductCreativeInputFrameReceipt dispatchPickedClick(
    iggy3d::ProductAppWindowState& window,
    cr::Facade& facade,
    iggy3d::MouseClick click,
    cr::TargetRef target,
    const iggy3d::ActionState* actions = nullptr) {
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.facade = &facade;
  request.actions = actions;
  request.click = click;
  request.pointerTarget = target;
  return iggy3d::processProductCreativeInputActions(request);
}

bool selectPickUpdatesFacadeAndUiRows() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;

  const iggy3d::MouseClick click = clickAt(150.0F, 250.0F);
  const iggy3d::ProductCreativeViewportPickFrameReceipt pick =
      pickRoomAt(window, facade, click);
  const iggy3d::ProductCreativeInputFrameReceipt input =
      dispatchPickedClick(window, facade, click, pick.target);
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();
  const cr::CreativeUiRow* selected = findUiRow(
      ui.model, cr::CreativeUiPanelKind::Selection,
      cr::CreativeUiRowKind::SelectedTarget);

  iggy3d::ProductCreativeUiDrawListRequest drawRequest;
  drawRequest.model = &ui.model;
  const iggy3d::ProductUiDrawList drawList =
      iggy3d::buildProductCreativeUiDrawList(drawRequest);
  const iggy3d::ProductUiPrimitive* selectedText =
      findPrimitive(drawList, "creative.row.selection.selected_target");

  return expect(pick.picked, "select pick hit") &&
         expect(pick.target.value == roomId, "select pick target") &&
         expect(input.pointerDispatched, "select input pointer dispatched") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "select facade selected") &&
         expect(facade.state().selected.value == roomId,
                "select old state selected") &&
         expect(selected != nullptr, "select ui row exists") &&
         expect(selected->target.value == roomId, "select ui row target") &&
         expect(selectedText != nullptr, "select draw text exists") &&
         expect(selectedText->text ==
                    "Selected Target: target=" + std::to_string(roomId),
                "select draw text target");
}

bool inspectPickUpdatesFacadeAndUiRows() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;

  iggy3d::ActionState actions =
      singlePressedAction(iggy3d::InputAction::EditorSelectWallTool);
  const iggy3d::MouseClick click = clickAt(150.0F, 250.0F);
  const iggy3d::ProductCreativeViewportPickFrameReceipt pick =
      pickRoomAt(window, facade, click);
  const iggy3d::ProductCreativeInputFrameReceipt input =
      dispatchPickedClick(window, facade, click, pick.target, &actions);
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();
  const cr::CreativeUiRow* inspected = findUiRow(
      ui.model, cr::CreativeUiPanelKind::Inspection,
      cr::CreativeUiRowKind::InspectedTarget);

  return expect(pick.picked, "inspect pick hit") &&
         expect(input.actionHandled, "inspect action handled") &&
         expect(input.pointerDispatched, "inspect pointer dispatched") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "inspect tool active") &&
         expect(facade.inspectionState().inspectedTarget.value == roomId,
                "inspect facade inspected") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "inspect selection untouched") &&
         expect(inspected != nullptr, "inspect ui row exists") &&
         expect(inspected->target.value == roomId, "inspect ui row target");
}

bool measurePickStoresTargetOnMeasurementRows() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;

  iggy3d::ActionState actions =
      singlePressedAction(iggy3d::InputAction::EditorPreviousTool);
  const iggy3d::MouseClick click = clickAt(150.0F, 250.0F);
  const iggy3d::ProductCreativeViewportPickFrameReceipt pick =
      pickRoomAt(window, facade, click);
  const iggy3d::ProductCreativeInputFrameReceipt input =
      dispatchPickedClick(window, facade, click, pick.target, &actions);
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();
  const cr::CreativeUiRow* start = findUiRow(
      ui.model, cr::CreativeUiPanelKind::Measurement,
      cr::CreativeUiRowKind::MeasurementStartPoint);
  const cr::CreativeUiRow* current = findUiRow(
      ui.model, cr::CreativeUiPanelKind::Measurement,
      cr::CreativeUiRowKind::MeasurementCurrentPoint);

  return expect(pick.picked, "measure pick hit") &&
         expect(input.actionHandled, "measure action handled") &&
         expect(input.pointerDispatched, "measure pointer dispatched") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "measure tool active") &&
         expect(facade.measurementState().active,
                "measure facade active") &&
         expect(facade.measurementState().startPoint.target.value == roomId,
                "measure start target") &&
         expect(start != nullptr && start->target.value == roomId,
                "measure ui start target") &&
         expect(current != nullptr && current->target.value == roomId,
                "measure ui current target");
}

bool missKeepsTargetInvalidAndSelectionUnchanged() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();

  const iggy3d::MouseClick click = clickAt(50.0F, 50.0F);
  const iggy3d::ProductCreativeViewportPickFrameReceipt pick =
      pickRoomAt(window, facade, click);
  const iggy3d::ProductCreativeInputFrameReceipt input =
      dispatchPickedClick(window, facade, click, pick.target);
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();
  const cr::CreativeUiRow* selected = findUiRow(
      ui.model, cr::CreativeUiPanelKind::Selection,
      cr::CreativeUiRowKind::SelectedTarget);

  return expect(!pick.picked, "miss not picked") &&
         expect(pick.pickStatus == cr::CreativeViewportPickStatus::Miss,
                "miss pick status") &&
         expect(pick.target.value == cr::kInvalidId, "miss target invalid") &&
         expect(input.pointerDispatched, "miss pointer still dispatched") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "miss selection invalid") &&
         expect(selected == nullptr, "miss no selected ui row");
}

bool suppressedCreativeUiClickDoesNotPickOrSelect() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();

  iggy3d::ProductCreativeUiDownstreamClickRequest downstreamRequest;
  downstreamRequest.click = clickAt(150.0F, 250.0F);
  downstreamRequest.creativeUiConsumed = true;
  const iggy3d::ProductCreativeUiDownstreamClickReceipt downstream =
      iggy3d::routeProductCreativeUiDownstreamClick(downstreamRequest);
  const iggy3d::ProductCreativeViewportPickFrameReceipt pick =
      pickRoomAt(window, facade, downstream.downstreamClick,
                 downstream.suppressed);
  const iggy3d::ProductCreativeInputFrameReceipt input =
      dispatchPickedClick(window, facade, downstream.downstreamClick,
                          pick.target);

  return expect(downstream.suppressed, "suppressed downstream") &&
         expect(!downstream.downstreamClick.clicked,
                "suppressed downstream non-click") &&
         expect(!pick.picked, "suppressed no pick") &&
         expect(pick.status ==
                    "product_creative_viewport_pick_no_click",
                "suppressed pick no click") &&
         expect(!input.pointerDispatched,
                "suppressed pointer not dispatched") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "suppressed selection invalid");
}

}  // namespace

int main() {
  const bool ok = selectPickUpdatesFacadeAndUiRows() &&
                  inspectPickUpdatesFacadeAndUiRows() &&
                  measurePickStoresTargetOnMeasurementRows() &&
                  missKeepsTargetInvalidAndSelectionUnchanged() &&
                  suppressedCreativeUiClickDoesNotPickOrSelect();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
