#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"

#include <array>
#include <cstdint>
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

iggy3d::ProductCreativeUiInputFrameReceipt commandInput(
    std::string_view semanticId,
    bool consumed = true,
    bool enabled = true) {
  iggy3d::ProductCreativeUiInputFrameReceipt receipt;
  receipt.consumed = consumed;
  receipt.enabled = enabled;
  receipt.semanticId = std::string(semanticId);
  return receipt;
}

iggy3d::ProductCreativeUiCommandFrameReceipt routeCommand(
    cr::Facade& facade,
    std::string_view semanticId = "creative.row.tools.active_tool") {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt = commandInput(semanticId);
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

cr::CreativeToolInputPacket pointerPress(cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

void selectTarget(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(objectId))));
}

void inspectTarget(cr::Facade& facade, cr::Id targetId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  static_cast<void>(facade.dispatchToolInput(pointerPress(targetId)));
}

bool nullFacadeFailsClosed() {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.inputReceipt = commandInput("creative.row.tools.active_tool");
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.requested, "null requested") &&
         expect(!receipt.facadeAvailable, "null facade unavailable") &&
         expect(receipt.inputConsumed, "null consumed copied") &&
         expect(receipt.inputEnabled, "null enabled copied") &&
         expect(!receipt.accepted, "null not accepted") &&
         expect(!receipt.changed, "null unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "null command none") &&
         expect(receipt.toolBefore == cr::Tool::Select,
                "null before default select") &&
         expect(receipt.toolAfter == cr::Tool::Select,
                "null after default select") &&
         expect(receipt.semanticId == "creative.row.tools.active_tool",
                "null semantic copied") &&
         expect(receipt.status ==
                    "product_creative_ui_command_facade_missing",
                "null status") &&
         expect(receipt.reasonCode ==
                    "product_creative_ui_command_facade_missing",
                "null reason");
}

bool notConsumedInputNoops() {
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt =
      commandInput("creative.row.tools.active_tool", false, true);
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.requested, "not consumed requested") &&
         expect(receipt.facadeAvailable, "not consumed facade available") &&
         expect(!receipt.inputConsumed, "not consumed copied") &&
         expect(receipt.inputEnabled, "not consumed enabled copied") &&
         expect(!receipt.accepted, "not consumed not accepted") &&
         expect(!receipt.changed, "not consumed unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "not consumed command none") &&
         expect(receipt.toolBefore == cr::Tool::Measure,
                "not consumed before") &&
         expect(receipt.toolAfter == cr::Tool::Measure,
                "not consumed after") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "not consumed facade unchanged") &&
         expect(receipt.status ==
                    "product_creative_ui_command_not_consumed",
                "not consumed status");
}

bool consumedDisabledNoops() {
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt =
      commandInput("creative.row.tools.active_tool", true, false);
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.inputConsumed, "disabled consumed copied") &&
         expect(!receipt.inputEnabled, "disabled enabled false") &&
         expect(!receipt.accepted, "disabled not accepted") &&
         expect(!receipt.changed, "disabled unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "disabled command none") &&
         expect(receipt.toolBefore == cr::Tool::Inspect,
                "disabled before") &&
         expect(receipt.toolAfter == cr::Tool::Inspect,
                "disabled after") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "disabled facade unchanged") &&
         expect(receipt.status == "product_creative_ui_command_disabled",
                "disabled status");
}

bool consumedUnknownSemanticNoops() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.status.creative_status");

  return expect(receipt.facadeAvailable, "unknown facade available") &&
         expect(receipt.inputConsumed, "unknown consumed") &&
         expect(receipt.inputEnabled, "unknown enabled") &&
         expect(!receipt.accepted, "unknown not accepted") &&
         expect(!receipt.changed, "unknown unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "unknown command none") &&
         expect(receipt.toolBefore == cr::Tool::Select,
                "unknown before") &&
         expect(receipt.toolAfter == cr::Tool::Select, "unknown after") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "unknown facade unchanged") &&
         expect(receipt.status ==
                    "product_creative_ui_command_unknown_semantic",
                "unknown status");
}

bool activeToolCommandCyclesSelectToInspect() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade);

  return expect(receipt.accepted, "cycle accepted") &&
         expect(receipt.changed, "cycle changed") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CycleNextTool,
                "cycle command kind") &&
         expect(receipt.toolBefore == cr::Tool::Select, "cycle before") &&
         expect(receipt.toolAfter == cr::Tool::Inspect, "cycle after") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "cycle facade tool") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "cycle status");
}

bool repeatedActiveToolCommandCyclesToolOrder() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(facade);
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(facade);
  const iggy3d::ProductCreativeUiCommandFrameReceipt third =
      routeCommand(facade);

  return expect(first.toolBefore == cr::Tool::Select &&
                    first.toolAfter == cr::Tool::Inspect,
                "repeat select inspect") &&
         expect(second.toolBefore == cr::Tool::Inspect &&
                    second.toolAfter == cr::Tool::Measure,
                "repeat inspect measure") &&
         expect(third.toolBefore == cr::Tool::Measure &&
                    third.toolAfter == cr::Tool::Select,
                "repeat measure select") &&
         expect(first.changed && second.changed && third.changed,
                "repeat all changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "repeat facade select");
}

bool commandUpdatesOldStateAndDoesNotMutateDocument() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeObjectId roomId = facade.createRoom("Room");
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade);

  return expect(roomId != cr::kInvalidObjectId, "state room created") &&
         expect(receipt.changed, "state changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "state tool state inspect") &&
         expect(facade.state().tool == cr::Tool::Inspect,
                "state old state inspect") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "state document unchanged");
}

bool createRoomCommandCreatesGenericRoom() {
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  inspectTarget(facade, 77);
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.tools.create_room");
  const cr::CreativeObject* room = facade.findObject(receipt.createObjectId);
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Room);

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateRoom,
                "create command kind") &&
         expect(receipt.accepted, "create accepted") &&
         expect(receipt.changed, "create changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "create status") &&
         expect(receipt.createRequested, "create receipt requested") &&
         expect(receipt.createAccepted, "create receipt accepted") &&
         expect(receipt.createChanged, "create receipt changed") &&
         expect(receipt.createStatus ==
                    cr::CreativeDocumentCreateStatus::Created,
                "create receipt status") &&
         expect(receipt.createObjectId != cr::kInvalidObjectId,
                "create object id") &&
         expect(receipt.createObjectKind == cr::CreativeObjectKind::Room,
                "create object kind") &&
         expect(receipt.createObjectName == "Room",
                "create object name") &&
         expect(receipt.createRevisionBefore == revisionBefore,
                "create revision before") &&
         expect(receipt.createRevisionAfter == revisionBefore + 1U,
                "create revision after") &&
         expect(receipt.createDirtyFlags == descriptor.creationDirtyFlags,
                "create dirty flags") &&
         expect(receipt.createMessage == "object_created",
                "create message") &&
         expect(receipt.createReasonCode == "object_created",
                "create reason") &&
         expect(room != nullptr, "create room exists") &&
         expect(room->kind == cr::CreativeObjectKind::Room,
                "created room kind") &&
         expect(room->bounds.min.x == 0.0 && room->bounds.min.y == 0.0 &&
                    room->bounds.min.z == 0.0,
                "created room bounds min") &&
         expect(room->bounds.max.x == 10.0 && room->bounds.max.y == 4.0 &&
                    room->bounds.max.z == 10.0,
                "created room bounds max") &&
         expect(room->visible, "created room visible") &&
         expect(!room->locked, "created room unlocked") &&
         expect(facade.document().objectCount() == objectCountBefore + 1U,
                "create object count") &&
         expect(facade.document().revision() == revisionBefore + 1U,
                "create document revision") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "create tool preserved") &&
         expect(facade.state().tool == cr::Tool::Inspect,
                "create old tool preserved") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "create selection preserved") &&
         expect(facade.inspectionState().inspectedTarget.value == 77U,
                "create inspection preserved");
}

bool repeatedCreateRoomCommandCreatesNewIdsAndRevisions() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(facade, "creative.row.tools.create_room");
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(facade, "creative.row.tools.create_room");

  return expect(first.accepted && first.changed, "repeat create first") &&
         expect(second.accepted && second.changed, "repeat create second") &&
         expect(first.createObjectId != cr::kInvalidObjectId,
                "repeat first id") &&
         expect(second.createObjectId != cr::kInvalidObjectId,
                "repeat second id") &&
         expect(second.createObjectId > first.createObjectId,
                "repeat ids increase") &&
         expect(first.createRevisionAfter == 1U,
                "repeat first revision") &&
         expect(second.createRevisionBefore == 1U,
                "repeat second revision before") &&
         expect(second.createRevisionAfter == 2U,
                "repeat second revision after") &&
         expect(facade.document().objectCount() == 2U,
                "repeat object count") &&
         expect(facade.document().revision() == 2U,
                "repeat document revision");
}

bool selectedTargetRowTogglesRoomVisibilityOff() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeObjectId roomId = facade.createRoom("Room");
  selectTarget(facade, roomId);
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.selection.selected_target");
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(room != nullptr, "toggle off room exists") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "toggle off command kind") &&
         expect(receipt.accepted, "toggle off accepted") &&
         expect(receipt.changed, "toggle off changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "toggle off status") &&
         expect(receipt.mutationRequested, "toggle off mutation requested") &&
         expect(receipt.mutationAccepted, "toggle off mutation accepted") &&
         expect(receipt.mutationChanged, "toggle off mutation changed") &&
         expect(receipt.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::Applied,
                "toggle off mutation status") &&
         expect(receipt.documentMutationStatus ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "toggle off document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "toggle off mutation kind") &&
         expect(receipt.mutationTarget.value == roomId,
                "toggle off mutation target") &&
         expect(receipt.mutationObjectId == roomId,
                "toggle off mutation object") &&
         expect(receipt.mutationObjectKind == cr::CreativeObjectKind::Room,
                "toggle off mutation object kind") &&
         expect(receipt.visibleBefore, "toggle off visible before") &&
         expect(!receipt.visibleAfter, "toggle off visible after") &&
         expect(!room->visible, "toggle off room hidden") &&
         expect(receipt.revisionBefore == revisionBefore,
                "toggle off revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "toggle off revision after") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "toggle off object count unchanged") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "toggle off selection preserved");
}

bool selectedTargetRowTogglesRoomVisibilityOnAgain() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeObjectId roomId = facade.createRoom("Room");
  selectTarget(facade, roomId);

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(facade, "creative.row.selection.selected_target");
  const std::uint64_t revisionBeforeSecond = facade.document().revision();
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(facade, "creative.row.selection.selected_target");
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(first.changed, "toggle on setup changed") &&
         expect(room != nullptr, "toggle on room exists") &&
         expect(second.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "toggle on command kind") &&
         expect(second.accepted, "toggle on accepted") &&
         expect(second.changed, "toggle on changed") &&
         expect(second.status == "product_creative_ui_command_applied",
                "toggle on status") &&
         expect(!second.visibleBefore, "toggle on visible before") &&
         expect(second.visibleAfter, "toggle on visible after") &&
         expect(room->visible, "toggle on room visible") &&
         expect(second.revisionBefore == revisionBeforeSecond,
                "toggle on revision before") &&
         expect(second.revisionAfter == revisionBeforeSecond + 1U,
                "toggle on revision after") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "toggle on selection preserved");
}

bool selectedTargetRowPreservesInspectionTarget() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeObjectId roomId = facade.createRoom("Room");
  selectTarget(facade, roomId);
  inspectTarget(facade, 77);

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.selection.selected_target");

  return expect(receipt.changed, "inspection toggle changed") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "inspection selection preserved") &&
         expect(facade.inspectionState().inspectedTarget.value == 77U,
                "inspection target preserved");
}

bool selectedTargetRowNoSelectionRejects() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.selection.selected_target");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "no selection command kind") &&
         expect(!receipt.accepted, "no selection not accepted") &&
         expect(!receipt.changed, "no selection unchanged") &&
         expect(receipt.status == "product_creative_ui_command_rejected",
                "no selection status") &&
         expect(receipt.mutationRequested, "no selection mutation requested") &&
         expect(!receipt.mutationAccepted,
                "no selection mutation not accepted") &&
         expect(!receipt.mutationChanged, "no selection mutation unchanged") &&
         expect(receipt.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::NoSelection,
                "no selection mutation status") &&
         expect(receipt.documentMutationStatus ==
                    cr::CreativeDocumentMutationStatus::Unknown,
                "no selection document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Unknown,
                "no selection mutation kind") &&
         expect(receipt.mutationMessage == "no_selection",
                "no selection mutation message") &&
         expect(facade.document().objectCount() == 0U,
                "no selection document unchanged");
}

bool selectedTargetRowMissingObjectRejectsAndPreservesSelection() {
  cr::Facade facade;
  facade.reset();
  constexpr cr::Id missingTarget = 999;
  static_cast<void>(facade.dispatchToolInput(pointerPress(missingTarget)));
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.selection.selected_target");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "missing command kind") &&
         expect(!receipt.accepted, "missing not accepted") &&
         expect(!receipt.changed, "missing unchanged") &&
         expect(receipt.status == "product_creative_ui_command_rejected",
                "missing status") &&
         expect(receipt.mutationRequested, "missing mutation requested") &&
         expect(!receipt.mutationAccepted, "missing mutation not accepted") &&
         expect(!receipt.mutationChanged, "missing mutation unchanged") &&
         expect(receipt.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::MissingObject,
                "missing mutation status") &&
         expect(receipt.documentMutationStatus ==
                    cr::CreativeDocumentMutationStatus::MissingObject,
                "missing document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "missing mutation kind") &&
         expect(receipt.mutationTarget.value == missingTarget,
                "missing mutation target") &&
         expect(receipt.mutationObjectId == missingTarget,
                "missing mutation object id") &&
         expect(receipt.revisionBefore == revisionBefore,
                "missing revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "missing revision after") &&
         expect(receipt.mutationMessage == "missing_object",
                "missing mutation message") &&
         expect(facade.selectionState().selectedTarget.value == missingTarget,
                "missing selection preserved");
}

bool nonToolRowsRemainUnknownNoop() {
  constexpr std::array<std::string_view, 2> kUnknownRows = {
      "creative.row.status.creative_status",
      "creative.row.snap.snap_settings",
  };

  bool ok = true;
  for (std::string_view semanticId : kUnknownRows) {
    cr::Facade facade;
    facade.reset();
    const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
        routeCommand(facade, semanticId);
    ok &= expect(receipt.semanticId == semanticId, "unknown row semantic") &&
          expect(receipt.status ==
                     "product_creative_ui_command_unknown_semantic",
                 "unknown row status") &&
          expect(receipt.commandKind ==
                     iggy3d::ProductCreativeUiCommandKind::None,
                 "unknown row command none") &&
          expect(!receipt.accepted, "unknown row not accepted") &&
          expect(!receipt.changed, "unknown row unchanged") &&
          expect(facade.toolState().activeTool == cr::Tool::Select,
                 "unknown row facade unchanged");
  }
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= nullFacadeFailsClosed();
  ok &= notConsumedInputNoops();
  ok &= consumedDisabledNoops();
  ok &= consumedUnknownSemanticNoops();
  ok &= activeToolCommandCyclesSelectToInspect();
  ok &= repeatedActiveToolCommandCyclesToolOrder();
  ok &= commandUpdatesOldStateAndDoesNotMutateDocument();
  ok &= createRoomCommandCreatesGenericRoom();
  ok &= repeatedCreateRoomCommandCreatesNewIdsAndRevisions();
  ok &= selectedTargetRowTogglesRoomVisibilityOff();
  ok &= selectedTargetRowTogglesRoomVisibilityOnAgain();
  ok &= selectedTargetRowPreservesInspectionTarget();
  ok &= selectedTargetRowNoSelectionRejects();
  ok &= selectedTargetRowMissingObjectRejectsAndPreservesSelection();
  ok &= nonToolRowsRemainUnknownNoop();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
