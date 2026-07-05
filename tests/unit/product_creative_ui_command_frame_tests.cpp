#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
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
    cr::CreativeAppState& app,
    std::string_view semanticId = "creative.row.tools.tool_select") {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput(semanticId);
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

cr::CreativeObjectId createRoom(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  return facade.createDocumentObject(request).objectId;
}

cr::CreativeToolInputPacket pointerPress(cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

cr::CreativeToolInputPacket pointerMove(double x,
                                        double y,
                                        cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerMove;
  input.pointer.x = x;
  input.pointer.y = y;
  input.pointer.target.value = targetId;
  return input;
}

void selectTarget(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(objectId))));
}

bool nullFacadeFailsClosed() {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.inputReceipt = commandInput("creative.row.tools.tool_select");
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
         expect(receipt.semanticId == "creative.row.tools.tool_select",
                "null semantic copied") &&
         expect(receipt.status ==
                    "product_creative_ui_command_facade_missing",
                "null status") &&
         expect(receipt.reasonCode ==
                    "product_creative_ui_command_facade_missing",
                "null reason");
}

bool notConsumedInputNoops() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt =
      commandInput("creative.row.tools.tool_select", false, true);
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt =
      commandInput("creative.row.tools.tool_select", true, false);
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.inputConsumed, "disabled consumed copied") &&
         expect(!receipt.inputEnabled, "disabled enabled false") &&
         expect(!receipt.accepted, "disabled not accepted") &&
         expect(!receipt.changed, "disabled unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "disabled command none") &&
         expect(receipt.toolBefore == cr::Tool::Move,
                "disabled before") &&
         expect(receipt.toolAfter == cr::Tool::Move,
                "disabled after") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "disabled facade unchanged") &&
         expect(receipt.status == "product_creative_ui_command_disabled",
                "disabled status");
}

bool consumedUnknownSemanticNoops() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.status.creative_status");

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

bool clickingActiveToolButtonIsAcceptedNoChange() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  // Facade starts on Select; clicking the already-active Select palette row is
  // an explicit SetActiveTool command that is consumed (accepted) but makes no
  // change and is NOT an error (TD-5 / palette policy).
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.tools.tool_select");

  return expect(receipt.accepted, "active tool accepted") &&
         expect(!receipt.changed, "active tool unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::SetActiveTool,
                "active tool command kind") &&
         expect(receipt.commandTool == cr::Tool::Select,
                "active tool command tool") &&
         expect(receipt.toolBefore == cr::Tool::Select,
                "active tool before select") &&
         expect(receipt.toolAfter == cr::Tool::Select,
                "active tool after select") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "active tool facade select") &&
         expect(facade.state().tool == cr::Tool::Select,
                "active tool old state select") &&
         expect(receipt.status == "product_creative_ui_command_no_change",
                "active tool status");
}

bool clickingActiveToolButtonPreservesVisibleGhost() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.dispatchToolInput(pointerMove(1.2, 2.7, 42)));
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  // Clicking the currently-active tool button is a same-tool no-op, so the
  // setActiveTool ghost-hide hygiene does not fire and the ghost survives.
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.tools.tool_select");

  return expect(facade.ghostState().visible,
                "active tool ghost remains visible") &&
         expect(receipt.accepted, "active tool ghost accepted") &&
         expect(!receipt.changed, "active tool ghost unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::SetActiveTool,
                "active tool ghost command kind") &&
         expect(receipt.toolBefore == cr::Tool::Select,
                "active tool ghost before select") &&
         expect(receipt.toolAfter == cr::Tool::Select,
                "active tool ghost after select") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "active tool ghost facade select") &&
         expect(facade.state().tool == cr::Tool::Select,
                "active tool ghost old state select") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "active tool ghost document unchanged");
}

bool clickingActiveToolButtonPreservesActiveMeasurement() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  const cr::CreativeFacadeToolDispatchReceipt begin =
      facade.dispatchToolInput(pointerPress(0));
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  // Clicking the ACTIVE tool's own button (Measure) is a same-tool no-op, so
  // the switch-away measurement-cancel hygiene does not fire. Clicking a
  // DIFFERENT tool button would legitimately cancel it.
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.tools.tool_measure");

  return expect(begin.measurementChanged,
                "active tool measure setup began") &&
         expect(receipt.accepted, "active tool measure accepted") &&
         expect(!receipt.changed, "active tool measure unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::SetActiveTool,
                "active tool measure command kind") &&
         expect(receipt.toolBefore == cr::Tool::Measure,
                "active tool measure before") &&
         expect(receipt.toolAfter == cr::Tool::Measure,
                "active tool measure after") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "active tool measure facade tool") &&
         expect(facade.state().tool == cr::Tool::Measure,
                "active display measure old state") &&
         expect(facade.measurementState().active,
                "active display measure still active") &&
         expect(facade.measurementState().hasMeasurement,
                "active display measure still present") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "active display measure document unchanged");
}

bool createRoomCommandCreatesGenericRoom() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.create.create_room");
  const cr::CreativeObject* room = facade.findObject(receipt.createObjectId);
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Room);

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateObject,
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
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "create tool preserved") &&
         expect(facade.state().tool == cr::Tool::Move,
                "create old tool preserved") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "create selection preserved");
}

bool repeatedCreateRoomCommandCreatesNewIdsAndRevisions() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(app, "creative.row.create.create_room");
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(app, "creative.row.create.create_room");

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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.inspector_visible");
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(app, "creative.row.selection.inspector_visible");
  const std::uint64_t revisionBeforeSecond = facade.document().revision();
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(app, "creative.row.selection.inspector_visible");
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

bool selectedTargetRowPreservesActiveTool() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.inspector_visible");

  return expect(receipt.changed, "tool preserve toggle changed") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "tool preserve selection preserved") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "tool preserve active tool preserved");
}

bool selectedTargetRowNoSelectionRejects() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.inspector_visible");

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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  constexpr cr::Id missingTarget = 999;
  static_cast<void>(facade.dispatchToolInput(pointerPress(missingTarget)));
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.inspector_visible");

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

bool selectedTargetRowIsDisplayOnlyNoop() {
  // TD-4: the selected_target row lost its command-table entry, so clicking it
  // is consumed but performs NO mutation (unknown semantic noop).
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const cr::CreativeObject* before = facade.findObject(roomId);
  const bool visibleBefore = before != nullptr && before->visible;
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.selected_target");
  const cr::CreativeObject* after = facade.findObject(roomId);

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "display-only command none") &&
         expect(!receipt.accepted, "display-only not accepted") &&
         expect(!receipt.changed, "display-only unchanged") &&
         expect(receipt.status ==
                    "product_creative_ui_command_unknown_semantic",
                "display-only status") &&
         expect(after != nullptr && after->visible == visibleBefore,
                "display-only visibility untouched") &&
         expect(facade.document().revision() == revisionBefore,
                "display-only revision untouched") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "display-only selection preserved");
}

bool inspectorLockedRowTogglesRoomLockedOn() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.inspector_locked");
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(room != nullptr, "lock on room exists") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectLocked,
                "lock on command kind") &&
         expect(receipt.accepted, "lock on accepted") &&
         expect(receipt.changed, "lock on changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "lock on status") &&
         expect(receipt.mutationRequested, "lock on mutation requested") &&
         expect(receipt.mutationAccepted, "lock on mutation accepted") &&
         expect(receipt.mutationChanged, "lock on mutation changed") &&
         expect(receipt.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::Applied,
                "lock on mutation status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetLocked,
                "lock on mutation kind") &&
         expect(!receipt.lockedBefore, "lock on locked before") &&
         expect(receipt.lockedAfter, "lock on locked after") &&
         expect(room->locked, "lock on room locked") &&
         expect(receipt.revisionBefore == revisionBefore,
                "lock on revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "lock on revision after") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "lock on selection preserved");
}

bool inspectorLockedRowTogglesRoomLockedOffAgain() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(app, "creative.row.selection.inspector_locked");
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(app, "creative.row.selection.inspector_locked");
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(first.changed && first.lockedAfter, "lock off setup locked") &&
         expect(room != nullptr && !room->locked, "lock off room unlocked") &&
         expect(second.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectLocked,
                "lock off command kind") &&
         expect(second.accepted && second.changed, "lock off changed") &&
         expect(second.lockedBefore && !second.lockedAfter,
                "lock off locked fields");
}

bool inspectorLockedRowNoSelectionRejects() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.inspector_locked");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectLocked,
                "lock no selection command kind") &&
         expect(!receipt.accepted, "lock no selection not accepted") &&
         expect(!receipt.changed, "lock no selection unchanged") &&
         expect(receipt.status == "product_creative_ui_command_rejected",
                "lock no selection status") &&
         expect(receipt.mutationRequested,
                "lock no selection mutation requested") &&
         expect(receipt.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::NoSelection,
                "lock no selection mutation status") &&
         expect(receipt.mutationMessage == "no_selection",
                "lock no selection message");
}

bool lockedObjectRefusesVisibilityMutationWithReceipt() {
  // TD-3: a locked object rejects all other mutations; toggling visibility on a
  // locked selection is rejected and the message names the lock (TL-6).
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const iggy3d::ProductCreativeUiCommandFrameReceipt lockReceipt =
      routeCommand(app, "creative.row.selection.inspector_locked");
  const std::uint64_t revisionAfterLock = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt visibilityReceipt =
      routeCommand(app, "creative.row.selection.inspector_visible");
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(lockReceipt.accepted && lockReceipt.lockedAfter,
                "refuse setup locked") &&
         expect(visibilityReceipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectVisibility,
                "refuse command kind") &&
         expect(!visibilityReceipt.accepted, "refuse not accepted") &&
         expect(!visibilityReceipt.changed, "refuse unchanged") &&
         expect(visibilityReceipt.status ==
                    "product_creative_ui_command_rejected",
                "refuse status") &&
         expect(visibilityReceipt.mutationStatus ==
                    cr::CreativeFacadeMutationStatus::Rejected,
                "refuse mutation status") &&
         expect(room != nullptr && room->visible,
                "refuse visibility untouched") &&
         expect(facade.document().revision() == revisionAfterLock,
                "refuse revision untouched") &&
         expect(!visibilityReceipt.mutationMessage.empty(),
                "refuse names a reason");
}

bool nonToolRowsRemainUnknownNoop() {
  constexpr std::array<std::string_view, 2> kUnknownRows = {
      "creative.row.status.creative_status",
      "creative.row.snap.snap_settings",
  };

  bool ok = true;
  for (std::string_view semanticId : kUnknownRows) {
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    facade.reset();
    const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
        routeCommand(app, semanticId);
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
  ok &= clickingActiveToolButtonIsAcceptedNoChange();
  ok &= clickingActiveToolButtonPreservesVisibleGhost();
  ok &= clickingActiveToolButtonPreservesActiveMeasurement();
  ok &= createRoomCommandCreatesGenericRoom();
  ok &= repeatedCreateRoomCommandCreatesNewIdsAndRevisions();
  ok &= selectedTargetRowTogglesRoomVisibilityOff();
  ok &= selectedTargetRowTogglesRoomVisibilityOnAgain();
  ok &= selectedTargetRowPreservesActiveTool();
  ok &= selectedTargetRowNoSelectionRejects();
  ok &= selectedTargetRowMissingObjectRejectsAndPreservesSelection();
  ok &= selectedTargetRowIsDisplayOnlyNoop();
  ok &= inspectorLockedRowTogglesRoomLockedOn();
  ok &= inspectorLockedRowTogglesRoomLockedOffAgain();
  ok &= inspectorLockedRowNoSelectionRejects();
  ok &= lockedObjectRefusesVisibilityMutationWithReceipt();
  ok &= nonToolRowsRemainUnknownNoop();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
