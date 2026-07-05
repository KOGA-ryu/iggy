#include "app/iggy3d/window/CreativeInputFrame.hpp"
#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"
#include "app/iggy3d/window/CreativeUiInputFrame.hpp"
#include "app/iggy3d/window/CreativeViewportPickFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/menu/CreativeUiDrawList.hpp"
#include "app/input/ActionState.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
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
  window.activeCreativeSaveId = "creative_save";
  window.activeCreativeWorldId = "world_001";
  window.activeCreativeDocumentId = 42U;
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
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  request.bounds.min = {1.0, 2.0, 1.0};
  request.bounds.max = {2.0, 3.0, 2.0};
  request.hasBoundsOverride = true;
  (void)facade.createDocumentObject(request);
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

iggy3d::ProductUiDrawList drawCreativeUi(const cr::CreativeUiModel& model) {
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  return iggy3d::buildProductCreativeUiDrawList(request);
}

iggy3d::ProductCreativeUiInputFrameReceipt clickPrimitive(
    const iggy3d::ProductUiDrawList& drawList,
    const iggy3d::ProductUiPrimitive& primitive) {
  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click =
      clickAt(primitive.rect.x + 1.0F, primitive.rect.y + 1.0F);
  return iggy3d::routeProductCreativeUiInputFrame(request);
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

bool selectedTargetCommandTogglesVisibilityAndRefreshesPick() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBeforeToggle = facade.document().revision();
  const iggy3d::MouseClick viewportClick = clickAt(150.0F, 250.0F);

  const iggy3d::ProductCreativeViewportPickFrameReceipt initialPick =
      pickRoomAt(window, facade, viewportClick);
  const iggy3d::ProductCreativeInputFrameReceipt selectInput =
      dispatchPickedClick(window, facade, viewportClick, initialPick.target);
  const cr::CreativeUiBuildReceipt selectedUi = facade.buildUiModel();
  const cr::CreativeUiRow* selectedRow = findUiRow(
      selectedUi.model, cr::CreativeUiPanelKind::Selection,
      cr::CreativeUiRowKind::SelectedTarget);
  const iggy3d::ProductUiDrawList selectedDrawList =
      drawCreativeUi(selectedUi.model);
  const iggy3d::ProductUiPrimitive* selectedPrimitive =
      findPrimitive(selectedDrawList, "creative.row.selection.selected_target");

  const iggy3d::ProductCreativeUiInputFrameReceipt selectedUiInput =
      selectedPrimitive != nullptr
          ? clickPrimitive(selectedDrawList, *selectedPrimitive)
          : iggy3d::ProductCreativeUiInputFrameReceipt{};
  const iggy3d::ProductCreativeUiCommandFrameReceipt hideCommand =
      iggy3d::routeProductCreativeUiCommandFrame(
          iggy3d::ProductCreativeUiCommandFrameRequest{&facade,
                                                       selectedUiInput});
  const cr::CreativeObject* hiddenRoom = facade.findObject(roomId);
  const bool roomHiddenAfterHide =
      hiddenRoom != nullptr && !hiddenRoom->visible;
  const iggy3d::ProductCreativeViewportPickFrameReceipt hiddenPick =
      pickRoomAt(window, facade, viewportClick);
  const cr::CreativeUiBuildReceipt hiddenUi = facade.buildUiModel();
  const cr::CreativeUiRow* hiddenSelectedRow = findUiRow(
      hiddenUi.model, cr::CreativeUiPanelKind::Selection,
      cr::CreativeUiRowKind::SelectedTarget);
  const iggy3d::ProductUiDrawList hiddenDrawList =
      drawCreativeUi(hiddenUi.model);
  const iggy3d::ProductUiPrimitive* hiddenSelectedPrimitive =
      findPrimitive(hiddenDrawList, "creative.row.selection.selected_target");

  const iggy3d::ProductCreativeUiInputFrameReceipt hiddenUiInput =
      hiddenSelectedPrimitive != nullptr
          ? clickPrimitive(hiddenDrawList, *hiddenSelectedPrimitive)
          : iggy3d::ProductCreativeUiInputFrameReceipt{};
  const iggy3d::ProductCreativeUiCommandFrameReceipt showCommand =
      iggy3d::routeProductCreativeUiCommandFrame(
          iggy3d::ProductCreativeUiCommandFrameRequest{&facade,
                                                       hiddenUiInput});
  const cr::CreativeObject* visibleRoom = facade.findObject(roomId);
  const iggy3d::ProductCreativeViewportPickFrameReceipt visiblePick =
      pickRoomAt(window, facade, viewportClick);

  return expect(initialPick.picked, "visibility flow initial pick") &&
         expect(initialPick.target.value == roomId,
                "visibility flow initial target") &&
         expect(selectInput.pointerDispatched,
                "visibility flow select dispatched") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "visibility flow selected target") &&
         expect(facade.state().selected.value == roomId,
                "visibility flow old selected target") &&
         expect(selectedRow != nullptr, "visibility flow selected row") &&
         expect(selectedRow->target.value == roomId,
                "visibility flow selected row target") &&
         expect(selectedPrimitive != nullptr,
                "visibility flow selected primitive") &&
         expect(selectedPrimitive->semanticId ==
                    "creative.row.selection.selected_target",
                "visibility flow selected semantic") &&
         expect(selectedPrimitive->text ==
                    "Selected: target=" + std::to_string(roomId) +
                        " visible=true",
                "visibility flow selected visible text") &&
         expect(selectedUiInput.consumed && selectedUiInput.enabled,
                "visibility flow selected ui consumed") &&
         expect(selectedUiInput.semanticId ==
                    "creative.row.selection.selected_target",
                "visibility flow selected ui semantic") &&
         expect(hideCommand.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "visibility flow hide command kind") &&
         expect(hideCommand.accepted && hideCommand.changed,
                "visibility flow hide changed") &&
         expect(hideCommand.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::Applied,
                "visibility flow hide mutation status") &&
         expect(hideCommand.mutationKind ==
                    cr::CreativeMutationKind::SetVisible,
                "visibility flow hide mutation kind") &&
         expect(hideCommand.visibleBefore && !hideCommand.visibleAfter,
                "visibility flow hide visible fields") &&
         expect(hideCommand.revisionBefore == revisionBeforeToggle,
                "visibility flow hide revision before") &&
         expect(hideCommand.revisionAfter == revisionBeforeToggle + 1U,
                "visibility flow hide revision after") &&
         expect(roomHiddenAfterHide, "visibility flow room hidden") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "visibility flow object count unchanged") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "visibility flow selection preserved after hide") &&
         expect(hiddenPick.projectionCellCount == 0U,
                "visibility flow hidden zero cells") &&
         expect(!hiddenPick.picked, "visibility flow hidden not picked") &&
         expect(hiddenPick.target.value == cr::kInvalidId,
                "visibility flow hidden target invalid") &&
         expect(hiddenPick.status ==
                    "product_creative_viewport_pick_projection_empty",
                "visibility flow hidden pick status") &&
         expect(hiddenSelectedRow != nullptr,
                "visibility flow hidden selected row") &&
         expect(hiddenSelectedRow->target.value == roomId,
                "visibility flow hidden selected row target") &&
         expect(hiddenSelectedPrimitive != nullptr,
                "visibility flow hidden selected primitive") &&
         expect(hiddenSelectedPrimitive->text ==
                    "Selected: target=" + std::to_string(roomId) +
                        " visible=false",
                "visibility flow hidden selected text") &&
         expect(hiddenUiInput.consumed && hiddenUiInput.enabled,
                "visibility flow hidden ui consumed") &&
         expect(showCommand.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "visibility flow show command kind") &&
         expect(showCommand.accepted && showCommand.changed,
                "visibility flow show changed") &&
         expect(!showCommand.visibleBefore && showCommand.visibleAfter,
                "visibility flow show visible fields") &&
         expect(showCommand.revisionBefore == hideCommand.revisionAfter,
                "visibility flow show revision before") &&
         expect(showCommand.revisionAfter == hideCommand.revisionAfter + 1U,
                "visibility flow show revision after") &&
         expect(visibleRoom != nullptr && visibleRoom->visible,
                "visibility flow room visible again") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "visibility flow object count still unchanged") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "visibility flow selection preserved after show") &&
         expect(visiblePick.picked, "visibility flow visible picked again") &&
         expect(visiblePick.target.value == roomId,
                "visibility flow visible target again") &&
         expect(visiblePick.projectionCellCount == 1U,
                "visibility flow visible cell restored");
}

bool createRoomUiRowCommandCreatesRoomThroughFacade() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeUiBuildReceipt initialUi = facade.buildUiModel();
  const iggy3d::ProductUiDrawList initialDrawList =
      drawCreativeUi(initialUi.model);
  const iggy3d::ProductUiPrimitive* createPrimitive =
      findPrimitive(initialDrawList, "creative.row.tools.create_room");

  const iggy3d::ProductCreativeUiInputFrameReceipt createInput =
      createPrimitive != nullptr
          ? clickPrimitive(initialDrawList, *createPrimitive)
          : iggy3d::ProductCreativeUiInputFrameReceipt{};
  const iggy3d::ProductCreativeUiCommandFrameReceipt createCommand =
      iggy3d::routeProductCreativeUiCommandFrame(
          iggy3d::ProductCreativeUiCommandFrameRequest{&facade, createInput});
  const cr::CreativeObject* created =
      facade.findObject(createCommand.createObjectId);
  const cr::CreativeUiBuildReceipt rebuiltUi = facade.buildUiModel();
  const iggy3d::ProductUiDrawList rebuiltDrawList =
      drawCreativeUi(rebuiltUi.model);
  const iggy3d::ProductUiPrimitive* rebuiltCreatePrimitive =
      findPrimitive(rebuiltDrawList, "creative.row.tools.create_room");

  return expect(createPrimitive != nullptr, "create flow primitive exists") &&
         expect(createPrimitive->text == "Create Room",
                "create flow primitive text") &&
         expect(createInput.consumed && createInput.enabled,
                "create flow input consumed") &&
         expect(createInput.semanticId == "creative.row.tools.create_room",
                "create flow semantic") &&
         expect(createCommand.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateRoom,
                "create flow command kind") &&
         expect(createCommand.accepted && createCommand.changed,
                "create flow command changed") &&
         expect(createCommand.createRequested &&
                    createCommand.createAccepted &&
                    createCommand.createChanged,
                "create flow receipt changed") &&
         expect(createCommand.createStatus ==
                    cr::CreativeDocumentCreateStatus::Created,
                "create flow create status") &&
         expect(createCommand.createObjectId != cr::kInvalidObjectId,
                "create flow object id") &&
         expect(createCommand.createObjectKind == cr::CreativeObjectKind::Room,
                "create flow object kind") &&
         expect(createCommand.createObjectName == "Room",
                "create flow object name") &&
         expect(createCommand.createRevisionBefore == 0U,
                "create flow revision before") &&
         expect(createCommand.createRevisionAfter == 1U,
                "create flow revision after") &&
         expect(created != nullptr, "create flow object exists") &&
         expect(created->bounds.min.x == 0.0 && created->bounds.min.y == 0.0 &&
                    created->bounds.min.z == 0.0,
                "create flow bounds min") &&
         expect(created->bounds.max.x == 10.0 && created->bounds.max.y == 4.0 &&
                    created->bounds.max.z == 10.0,
                "create flow bounds max") &&
         expect(created->visible, "create flow visible") &&
         expect(!created->locked, "create flow unlocked") &&
         expect(facade.document().objectCount() == 1U,
                "create flow object count") &&
         expect(facade.document().revision() == 1U,
                "create flow document revision") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "create flow selection unchanged") &&
         expect(facade.inspectionState().inspectedTarget.value ==
                    cr::kInvalidId,
                "create flow inspection unchanged") &&
         expect(rebuiltUi.model.rows.size() == initialUi.model.rows.size(),
                "create flow rebuilt row count stable") &&
         expect(rebuiltCreatePrimitive != nullptr,
                "create flow rebuilt primitive") &&
         expect(rebuiltCreatePrimitive->text == "Create Room",
                "create flow rebuilt text");
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
                    "Selected: target=" + std::to_string(roomId) +
                        " visible=true",
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
  const bool ok = createRoomUiRowCommandCreatesRoomThroughFacade() &&
                  selectedTargetCommandTogglesVisibilityAndRefreshesPick() &&
                  selectPickUpdatesFacadeAndUiRows() &&
                  inspectPickUpdatesFacadeAndUiRows() &&
                  measurePickStoresTargetOnMeasurementRows() &&
                  missKeepsTargetInvalidAndSelectionUnchanged() &&
                  suppressedCreativeUiClickDoesNotPickOrSelect();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
