#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include <array>
#include <cmath>
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

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) < 0.000001;
}

cr::CreativeVec3 centerOfBounds(const cr::CreativeBounds& bounds) {
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * 0.5,
      bounds.min.y + (bounds.max.y - bounds.min.y) * 0.5,
      bounds.min.z + (bounds.max.z - bounds.min.z) * 0.5,
  };
}

bool sameVec3(const cr::CreativeVec3& lhs, const cr::CreativeVec3& rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
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

cr::CreativeObjectId createObject(cr::Facade& facade,
                                  cr::CreativeObjectKind kind,
                                  std::string_view name) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string(name);
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

bool rebuildRoomCommandRequestsExternalRefreshWithoutDocumentMutation() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.tools.rebuild_room");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::RebuildRoom,
                "rebuild command kind") &&
         expect(receipt.semanticId == "creative.row.tools.rebuild_room",
                "rebuild semantic") &&
         expect(receipt.accepted, "rebuild command accepted") &&
         expect(!receipt.changed, "rebuild command unchanged") &&
         expect(receipt.status ==
                    "product_creative_ui_command_rebuild_room_requested",
                "rebuild command status") &&
         expect(receipt.reasonCode ==
                    "product_creative_ui_command_rebuild_room_requested",
                "rebuild command reason") &&
         expect(!receipt.create.document.requested, "rebuild does not create") &&
         expect(!receipt.mutation.requested, "rebuild does not mutate") &&
         expect(receipt.toolBefore == cr::Tool::Move,
                "rebuild tool before") &&
         expect(receipt.toolAfter == cr::Tool::Move,
                "rebuild tool after") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "rebuild facade tool unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "rebuild object count unchanged") &&
         expect(facade.document().revision() == revisionBefore,
                "rebuild revision unchanged");
}

bool undoCommandNoHistoryRejectsWithoutMutation() {
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  facade.reset();
  const std::uint64_t revisionBefore = facade.document().revision();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.tools.undo");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        UndoLastDocumentChange,
                "undo empty command kind") &&
         expect(receipt.semanticId == "creative.row.tools.undo",
                "undo empty semantic") &&
         expect(!receipt.accepted, "undo empty not accepted") &&
         expect(!receipt.changed, "undo empty unchanged") &&
         expect(receipt.status == "product_creative_ui_command_rejected",
                "undo empty command status") &&
         expect(receipt.undo.requested, "undo empty requested") &&
         expect(!receipt.undo.accepted, "undo empty receipt not accepted") &&
         expect(!receipt.undo.changed, "undo empty receipt unchanged") &&
         expect(!receipt.undo.hadSnapshot, "undo empty no snapshot") &&
         expect(receipt.undo.revisionBefore == revisionBefore,
                "undo empty revision before") &&
         expect(receipt.undo.revisionAfter == revisionBefore,
                "undo empty revision after") &&
         expect(receipt.undo.objectCountBefore == objectCountBefore,
                "undo empty object count before") &&
         expect(receipt.undo.objectCountAfter == objectCountBefore,
                "undo empty object count after") &&
         expect(receipt.undo.depthBefore == 0U, "undo empty depth before") &&
         expect(receipt.undo.depthAfter == 0U, "undo empty depth after") &&
         expect(receipt.undo.status == "creative_undo_empty",
                "undo empty status") &&
         expect(receipt.undo.reasonCode == "creative_undo_empty",
                "undo empty reason") &&
         expect(facade.document().revision() == revisionBefore,
                "undo empty document revision unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "undo empty document object count unchanged");
}

bool undoCommandRestoresLatestSnapshotAndClearsTransientState() {
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.documentForPersistence().assignId(42U));
  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());

  const cr::CreativeObjectId crateId =
      createObject(facade, cr::CreativeObjectKind::Crate, "Crate A");
  selectTarget(facade, crateId);
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  const std::uint64_t revisionBefore = facade.document().revision();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.tools.undo");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        UndoLastDocumentChange,
                "undo command kind") &&
         expect(receipt.accepted, "undo accepted") &&
         expect(receipt.changed, "undo changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "undo command status") &&
         expect(receipt.undo.requested, "undo requested") &&
         expect(receipt.undo.accepted, "undo receipt accepted") &&
         expect(receipt.undo.changed, "undo receipt changed") &&
         expect(receipt.undo.hadSnapshot, "undo had snapshot") &&
         expect(receipt.undo.documentId == facade.document().id(),
                "undo document id") &&
         expect(receipt.undo.revisionBefore == revisionBefore,
                "undo revision before") &&
         expect(receipt.undo.revisionAfter == 0U, "undo revision after") &&
         expect(receipt.undo.objectCountBefore == objectCountBefore,
                "undo object count before") &&
         expect(receipt.undo.objectCountAfter == 0U,
                "undo object count after") &&
         expect(receipt.undo.depthBefore == 1U, "undo depth before") &&
         expect(receipt.undo.depthAfter == 0U, "undo depth after") &&
         expect(receipt.undo.status == "creative_undo_applied",
                "undo status") &&
         expect(receipt.undo.reasonCode == "creative_undo_applied",
                "undo reason") &&
         expect(facade.findObject(crateId) == nullptr,
                "undo restored object absence") &&
         expect(facade.document().objectCount() == 0U,
                "undo restored object count") &&
         expect(facade.document().revision() == 0U,
                "undo restored revision") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "undo selection cleared") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "undo install resets tool") &&
         expect(cr::creativeUndoDepth(app.undoStack) == 0U,
                "undo stack popped");
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
  const cr::CreativeObject* room = facade.findObject(receipt.create.document.objectId);
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Room);

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateObject,
                "create command kind") &&
         expect(receipt.accepted, "create accepted") &&
         expect(receipt.changed, "create changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "create status") &&
         expect(receipt.create.document.requested, "create receipt requested") &&
         expect(receipt.create.document.accepted, "create receipt accepted") &&
         expect(receipt.create.document.changed, "create receipt changed") &&
         expect(receipt.create.document.status ==
                    cr::CreativeDocumentCreateStatus::Created,
                "create receipt status") &&
         expect(receipt.create.document.objectId != cr::kInvalidObjectId,
                "create object id") &&
         expect(receipt.create.document.objectKind == cr::CreativeObjectKind::Room,
                "create object kind") &&
         expect(receipt.create.document.objectName == "Room",
                "create object name") &&
         expect(receipt.create.document.revisionBefore == revisionBefore,
                "create revision before") &&
         expect(receipt.create.document.revisionAfter == revisionBefore + 1U,
                "create revision after") &&
         expect(receipt.create.document.creationDirtyFlags == descriptor.creationDirtyFlags,
                "create dirty flags") &&
         expect(receipt.create.document.message == "object_created",
                "create message") &&
         expect(receipt.create.document.reasonCode == "object_created",
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
         expect(first.create.document.objectId != cr::kInvalidObjectId,
                "repeat first id") &&
         expect(second.create.document.objectId != cr::kInvalidObjectId,
                "repeat second id") &&
         expect(second.create.document.objectId > first.create.document.objectId,
                "repeat ids increase") &&
         expect(first.create.document.revisionAfter == 1U,
                "repeat first revision") &&
         expect(second.create.document.revisionBefore == 1U,
                "repeat second revision before") &&
         expect(second.create.document.revisionAfter == 2U,
                "repeat second revision after") &&
         expect(second.create.placementOffsetApplied,
                "repeat second placement offset applied") &&
         expect(near(second.create.placementOffsetX, 1.0),
                "repeat second placement offset") &&
         expect(facade.document().objectCount() == 2U,
                "repeat object count") &&
         expect(facade.document().revision() == 2U,
                "repeat document revision");
}

bool createPaletteRowsRouteToDescriptorKinds() {
  bool ok = true;
  for (const iggy3d::ProductCreativeUiCommandCatalogEntry& slot :
       iggy3d::productCreativeUiCreatePalette()) {
    const cr::CreativeObjectDescriptor& descriptor =
        cr::describeObject(slot.objectKind);
    cr::CreativeAppState app;
    cr::Facade& facade = app.facade;
    facade.reset();

    const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
        routeCommand(app, slot.semanticId);
    const cr::CreativeObject* object = facade.findObject(receipt.create.document.objectId);

    ok &= expect(iggy3d::productCreativeUiCreatePaletteEntryAllowed(slot),
                 "create palette slot descriptor-allowed") &&
          expect(receipt.commandKind ==
                     iggy3d::ProductCreativeUiCommandKind::CreateObject,
                 "create palette command kind") &&
          expect(receipt.commandObjectKind == descriptor.kind,
                 "create palette command object kind") &&
          expect(receipt.create.document.objectKind == descriptor.kind,
                 "create palette receipt object kind") &&
          expect(receipt.create.document.objectName == descriptor.name,
                 "create palette receipt object name") &&
          expect(receipt.accepted && receipt.changed,
                 "create palette command applied") &&
          expect(object != nullptr && object->kind == descriptor.kind,
                 "create palette object created");
  }
  return ok;
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
         expect(receipt.mutation.requested, "toggle off mutation requested") &&
         expect(receipt.mutation.accepted, "toggle off mutation accepted") &&
         expect(receipt.mutation.changed, "toggle off mutation changed") &&
         expect(receipt.mutation.status ==
                    cr::CreativeFacadeMutationStatus::Applied,
                "toggle off mutation status") &&
         expect(receipt.mutation.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "toggle off document status") &&
         expect(receipt.mutation.mutationKind == cr::CreativeMutationKind::SetVisible,
                "toggle off mutation kind") &&
         expect(receipt.mutation.target.value == roomId,
                "toggle off mutation target") &&
         expect(receipt.mutation.objectId == roomId,
                "toggle off mutation object") &&
         expect(receipt.mutation.objectKind == cr::CreativeObjectKind::Room,
                "toggle off mutation object kind") &&
         expect(receipt.mutation.visibleBefore, "toggle off visible before") &&
         expect(!receipt.mutation.visibleAfter, "toggle off visible after") &&
         expect(!room->visible, "toggle off room hidden") &&
         expect(receipt.mutation.revisionBefore == revisionBefore,
                "toggle off revision before") &&
         expect(receipt.mutation.revisionAfter == revisionBefore + 1U,
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
         expect(!second.mutation.visibleBefore, "toggle on visible before") &&
         expect(second.mutation.visibleAfter, "toggle on visible after") &&
         expect(room->visible, "toggle on room visible") &&
         expect(second.mutation.revisionBefore == revisionBeforeSecond,
                "toggle on revision before") &&
         expect(second.mutation.revisionAfter == revisionBeforeSecond + 1U,
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
         expect(receipt.mutation.requested, "no selection mutation requested") &&
         expect(!receipt.mutation.accepted,
                "no selection mutation not accepted") &&
         expect(!receipt.mutation.changed, "no selection mutation unchanged") &&
         expect(receipt.mutation.status ==
                    cr::CreativeFacadeMutationStatus::NoSelection,
                "no selection mutation status") &&
         expect(receipt.mutation.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Unknown,
                "no selection document status") &&
         expect(receipt.mutation.mutationKind == cr::CreativeMutationKind::Unknown,
                "no selection mutation kind") &&
         expect(receipt.mutation.message == "no_selection",
                "no selection mutation message") &&
         expect(facade.document().objectCount() == 0U,
                "no selection document unchanged");
}

bool deleteSelectedObjectRemovesObjectAndClearsSelection() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId crateId =
      createObject(facade, cr::CreativeObjectKind::Crate, "Crate A");
  selectTarget(facade, crateId);
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.delete_selected");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::DeleteSelectedObject,
                "delete command kind") &&
         expect(receipt.semanticId == "creative.row.selection.delete_selected",
                "delete semantic") &&
         expect(receipt.accepted, "delete accepted") &&
         expect(receipt.changed, "delete changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "delete status") &&
         expect(receipt.reasonCode == "product_creative_ui_command_applied",
                "delete reason") &&
         expect(receipt.remove.document.requested, "delete requested") &&
         expect(receipt.remove.document.accepted, "delete receipt accepted") &&
         expect(receipt.remove.document.changed, "delete receipt changed") &&
         expect(receipt.remove.document.objectRemoved, "delete receipt removed") &&
         expect(receipt.remove.document.objectId == crateId, "delete object id") &&
         expect(receipt.remove.document.objectKind == cr::CreativeObjectKind::Crate,
                "delete object kind") &&
         expect(receipt.remove.document.objectName == "Crate A",
                "delete object name") &&
         expect(receipt.remove.document.revisionBefore == revisionBefore,
                "delete revision before") &&
         expect(receipt.remove.document.revisionAfter == revisionBefore + 1U,
                "delete revision after") &&
         expect(receipt.remove.document.removalDirtyFlags != 0U, "delete dirty flags") &&
         expect(receipt.remove.document.status == cr::CreativeDocumentRemoveStatus::Removed,
                "delete receipt status") &&
         expect(receipt.remove.document.message == "object_removed",
                "delete receipt message") &&
         expect(receipt.remove.document.reasonCode == "object_removed",
                "delete receipt reason") &&
         expect(facade.findObject(crateId) == nullptr, "delete object gone") &&
         expect(facade.document().objectCount() == objectCountBefore - 1U,
                "delete object count") &&
         expect(facade.document().revision() == revisionBefore + 1U,
                "delete document revision") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "delete selection cleared") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "delete tool preserved");
}

bool deleteSelectedObjectNoSelectionRejectsWithoutMutation() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.delete_selected");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::DeleteSelectedObject,
                "delete no selection command kind") &&
         expect(!receipt.accepted, "delete no selection not accepted") &&
         expect(!receipt.changed, "delete no selection unchanged") &&
         expect(receipt.status == "product_creative_ui_command_rejected",
                "delete no selection status") &&
         expect(receipt.remove.document.requested, "delete no selection requested") &&
         expect(!receipt.remove.document.accepted,
                "delete no selection receipt not accepted") &&
         expect(!receipt.remove.document.changed, "delete no selection not changed") &&
         expect(!receipt.remove.document.objectRemoved, "delete no selection not removed") &&
         expect(receipt.remove.document.objectId == cr::kInvalidObjectId,
                "delete no selection object id") &&
         expect(receipt.remove.document.status == cr::CreativeDocumentRemoveStatus::Unknown,
                "delete no selection native status") &&
         expect(receipt.remove.noSelection,
                "delete no selection outcome") &&
         expect(receipt.remove.document.message == "no_selection",
                "delete no selection message") &&
         expect(receipt.remove.document.reasonCode == "no_selection",
                "delete no selection reason") &&
         expect(receipt.remove.document.revisionBefore == revisionBefore,
                "delete no selection revision before") &&
         expect(receipt.remove.document.revisionAfter == revisionBefore,
                "delete no selection revision after") &&
         expect(facade.document().objectCount() == 0U,
                "delete no selection object count") &&
         expect(facade.document().revision() == revisionBefore,
                "delete no selection revision unchanged");
}

bool generateRoomShellInstallsGeneratedChildrenAtomically() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.documentForPersistence().assignId(42U));
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.generate_room_shell");

  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  bool allGeneratedHaveParent = true;
  bool allGeneratedHaveTags = true;
  bool allGeneratedHaveCenteredTransforms = true;
  for (const cr::CreativeObject& object : facade.document().objects()) {
    if (object.parentId.has_value() && object.parentId.value() == roomId) {
      floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
      wallCount += object.kind == cr::CreativeObjectKind::Wall ? 1U : 0U;
      allGeneratedHaveParent &= object.parentId.value() == roomId;
      allGeneratedHaveTags &= cr::creativeRoomShellObjectHasProvenance(
          object,
          roomId);
      allGeneratedHaveCenteredTransforms &=
          sameVec3(object.transform.position, centerOfBounds(object.bounds));
    }
  }

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        GenerateSelectedRoomShell,
                "shell command kind") &&
         expect(receipt.semanticId ==
                    "creative.row.selection.generate_room_shell",
                "shell semantic") &&
         expect(receipt.accepted, "shell accepted") &&
         expect(receipt.changed, "shell changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "shell status") &&
         expect(receipt.shell.build.requested, "shell build requested") &&
         expect(!receipt.shell.remove.requested, "shell remove unrequested") &&
         expect(receipt.shell.build.accepted, "shell receipt accepted") &&
         expect(receipt.shell.changed, "shell receipt changed") &&
         expect(receipt.shell.build.roomObjectId == roomId, "shell room id") &&
         expect(receipt.shell.build.generatedRequestCount == 5U,
                "shell generated count") &&
         expect(receipt.shell.build.floorRequestCount == 1U, "shell floor count") &&
         expect(receipt.shell.build.wallRequestCount == 4U, "shell wall count") &&
         expect(receipt.shell.revisionBefore == revisionBefore,
                "shell revision before") &&
         expect(receipt.shell.revisionAfter == revisionBefore + 5U,
                "shell revision after") &&
         expect(receipt.shell.build.status == cr::CreativeRoomShellStatus::Generated,
                "shell receipt status") &&
         expect(receipt.shell.build.reasonCode == "creative_room_shell_generated",
                "shell receipt reason") &&
         expect(facade.document().objectCount() == 6U,
                "shell object count") &&
         expect(floorCount == 1U, "one generated floor") &&
         expect(wallCount == 4U, "four generated walls") &&
         expect(allGeneratedHaveParent, "generated children parented") &&
         expect(allGeneratedHaveTags, "generated children tagged") &&
         expect(allGeneratedHaveCenteredTransforms,
                "generated children anchored at bounds centers") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "shell install clears selection");
}

bool generateRoomShellNoSelectionRejectsWithoutMutation() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.generate_room_shell");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        GenerateSelectedRoomShell,
                "shell no selection command kind") &&
         expect(!receipt.accepted, "shell no selection rejected") &&
         expect(!receipt.changed, "shell no selection unchanged") &&
         expect(receipt.shell.build.requested, "shell no selection requested") &&
         expect(!receipt.shell.remove.requested, "shell no selection remove unrequested") &&
         expect(!receipt.shell.build.accepted,
                "shell no selection receipt rejected") &&
         expect(receipt.shell.build.status == cr::CreativeRoomShellStatus::NoRoomSelected,
                "shell no selection status") &&
         expect(receipt.shell.build.reasonCode ==
                    "creative_room_shell_no_room_selected",
                "shell no selection reason") &&
         expect(facade.document().revision() == revisionBefore,
                "shell no selection revision unchanged") &&
         expect(facade.document().objectCount() == 0U,
                "shell no selection object count");
}

bool generateRoomShellNonRoomRejectsWithoutMutation() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId crateId =
      createObject(facade, cr::CreativeObjectKind::Crate, "Crate A");
  selectTarget(facade, crateId);
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.generate_room_shell");

  return expect(!receipt.accepted, "shell non-room rejected") &&
         expect(!receipt.changed, "shell non-room unchanged") &&
         expect(receipt.shell.build.roomObjectId == crateId, "shell non-room id") &&
         expect(receipt.shell.build.status == cr::CreativeRoomShellStatus::SelectedNotRoom,
                "shell non-room status") &&
         expect(receipt.shell.build.reasonCode ==
                    "creative_room_shell_selected_not_room",
                "shell non-room reason") &&
         expect(facade.document().revision() == revisionBefore,
                "shell non-room revision unchanged") &&
         expect(facade.document().objectCount() == 1U,
                "shell non-room object count");
}

bool generateRoomShellDuplicateRejectsWithoutMutation() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);

  cr::CreativeDocumentCreateRequest existing;
  existing.kind = cr::CreativeObjectKind::Floor;
  existing.name = "Existing Shell Floor";
  existing.parentId = roomId;
  existing.tags = {std::string(cr::generatedRoomShellTag()),
                   cr::sourceRoomShellTag(roomId)};
  const cr::CreativeDocumentCreateReceipt existingReceipt =
      facade.createDocumentObject(existing);
  selectTarget(facade, roomId);
  const std::uint64_t revisionBefore = facade.document().revision();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.generate_room_shell");

  return expect(existingReceipt.accepted, "shell duplicate setup") &&
         expect(!receipt.accepted, "shell duplicate rejected") &&
         expect(!receipt.changed, "shell duplicate unchanged") &&
         expect(receipt.shell.build.status == cr::CreativeRoomShellStatus::AlreadyExists,
                "shell duplicate status") &&
         expect(receipt.shell.build.reasonCode ==
                    "creative_room_shell_already_exists",
                "shell duplicate reason") &&
         expect(facade.document().revision() == revisionBefore,
                "shell duplicate revision unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "shell duplicate object count unchanged");
}

bool removeRoomShellDeletesOnlyGeneratedChildrenAtomically() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.documentForPersistence().assignId(54U));
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const iggy3d::ProductCreativeUiCommandFrameReceipt generated =
      routeCommand(app, "creative.row.selection.generate_room_shell");

  cr::CreativeDocumentCreateRequest manualChild;
  manualChild.kind = cr::CreativeObjectKind::Floor;
  manualChild.name = "Manual Room Child";
  manualChild.parentId = roomId;
  const cr::CreativeDocumentCreateReceipt manualChildReceipt =
      facade.createDocumentObject(manualChild);

  selectTarget(facade, roomId);
  const std::uint64_t revisionBefore = facade.document().revision();
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.remove_room_shell");

  const std::uint64_t generatedChildCount =
      cr::collectCreativeRoomShellChildIds(facade.document(), roomId).size();

  return expect(generated.accepted, "shell remove setup generated") &&
         expect(manualChildReceipt.accepted,
                "shell remove manual child setup") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        RemoveSelectedRoomShell,
                "shell remove command kind") &&
         expect(receipt.semanticId ==
                    "creative.row.selection.remove_room_shell",
                "shell remove semantic") &&
         expect(receipt.accepted, "shell remove accepted") &&
         expect(receipt.changed, "shell remove changed") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "shell remove status") &&
         expect(receipt.shell.remove.requested, "shell remove requested") &&
         expect(!receipt.shell.build.requested, "shell build unrequested") &&
         expect(receipt.shell.remove.accepted, "shell remove receipt accepted") &&
         expect(receipt.shell.changed, "shell remove receipt changed") &&
         expect(receipt.shell.remove.roomObjectId == roomId, "shell remove room id") &&
         expect(receipt.shell.remove.removedObjectCount == 5U,
                "shell remove count") &&
         expect(receipt.shell.remove.floorObjectCount == 1U, "shell remove floor count") &&
         expect(receipt.shell.remove.wallObjectCount == 4U, "shell remove wall count") &&
         expect(receipt.shell.revisionBefore == revisionBefore,
                "shell remove revision before") &&
         expect(receipt.shell.revisionAfter == revisionBefore + 5U,
                "shell remove revision after") &&
         expect(receipt.shell.remove.status == cr::CreativeRoomShellStatus::Removed,
                "shell remove receipt status") &&
         expect(receipt.shell.remove.reasonCode == "creative_room_shell_removed",
                "shell remove receipt reason") &&
         expect(facade.document().objectCount() == objectCountBefore - 5U,
                "shell remove object count") &&
         expect(generatedChildCount == 0U,
                "shell remove generated children gone") &&
         expect(facade.findObject(roomId) != nullptr,
                "shell remove room remains") &&
         expect(facade.findObject(manualChildReceipt.objectId) != nullptr,
                "shell remove manual child remains") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "shell remove install clears selection");
}

bool removeRoomShellWithoutGeneratedChildrenRejectsWithoutMutation() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(app, "creative.row.selection.remove_room_shell");

  return expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        RemoveSelectedRoomShell,
                "shell remove empty command kind") &&
         expect(!receipt.accepted, "shell remove empty rejected") &&
         expect(!receipt.changed, "shell remove empty unchanged") &&
         expect(receipt.shell.remove.requested, "shell remove empty requested") &&
         expect(!receipt.shell.build.requested, "shell remove empty build unrequested") &&
         expect(!receipt.shell.remove.accepted,
                "shell remove empty receipt rejected") &&
         expect(receipt.shell.remove.status == cr::CreativeRoomShellStatus::NoGeneratedShell,
                "shell remove empty status") &&
         expect(receipt.shell.remove.reasonCode ==
                    "creative_room_shell_remove_not_found",
                "shell remove empty reason") &&
         expect(receipt.shell.remove.removedObjectCount == 0U,
                "shell remove empty removed count") &&
         expect(facade.document().revision() == revisionBefore,
                "shell remove empty revision unchanged") &&
         expect(facade.document().objectCount() == 1U,
                "shell remove empty object count");
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
         expect(receipt.mutation.requested, "missing mutation requested") &&
         expect(!receipt.mutation.accepted, "missing mutation not accepted") &&
         expect(!receipt.mutation.changed, "missing mutation unchanged") &&
         expect(receipt.mutation.status ==
                    cr::CreativeFacadeMutationStatus::MissingObject,
                "missing mutation status") &&
         expect(receipt.mutation.documentStatus ==
                    cr::CreativeDocumentMutationStatus::MissingObject,
                "missing document status") &&
         expect(receipt.mutation.mutationKind == cr::CreativeMutationKind::SetVisible,
                "missing mutation kind") &&
         expect(receipt.mutation.target.value == missingTarget,
                "missing mutation target") &&
         expect(receipt.mutation.objectId == missingTarget,
                "missing mutation object id") &&
         expect(receipt.mutation.revisionBefore == revisionBefore,
                "missing revision before") &&
         expect(receipt.mutation.revisionAfter == revisionBefore,
                "missing revision after") &&
         expect(receipt.mutation.message == "missing_object",
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
         expect(receipt.mutation.requested, "lock on mutation requested") &&
         expect(receipt.mutation.accepted, "lock on mutation accepted") &&
         expect(receipt.mutation.changed, "lock on mutation changed") &&
         expect(receipt.mutation.status ==
                    cr::CreativeFacadeMutationStatus::Applied,
                "lock on mutation status") &&
         expect(receipt.mutation.mutationKind == cr::CreativeMutationKind::SetLocked,
                "lock on mutation kind") &&
         expect(!receipt.mutation.lockedBefore, "lock on locked before") &&
         expect(receipt.mutation.lockedAfter, "lock on locked after") &&
         expect(room->locked, "lock on room locked") &&
         expect(receipt.mutation.revisionBefore == revisionBefore,
                "lock on revision before") &&
         expect(receipt.mutation.revisionAfter == revisionBefore + 1U,
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

  return expect(first.changed && first.mutation.lockedAfter, "lock off setup locked") &&
         expect(room != nullptr && !room->locked, "lock off room unlocked") &&
         expect(second.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::
                        ToggleSelectedObjectLocked,
                "lock off command kind") &&
         expect(second.accepted && second.changed, "lock off changed") &&
         expect(second.mutation.lockedBefore && !second.mutation.lockedAfter,
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
         expect(receipt.mutation.requested,
                "lock no selection mutation requested") &&
         expect(receipt.mutation.status ==
                    cr::CreativeFacadeMutationStatus::NoSelection,
                "lock no selection mutation status") &&
         expect(receipt.mutation.message == "no_selection",
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

  return expect(lockReceipt.accepted && lockReceipt.mutation.lockedAfter,
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
         expect(visibilityReceipt.mutation.status ==
                    cr::CreativeFacadeMutationStatus::Rejected,
                "refuse mutation status") &&
         expect(room != nullptr && room->visible,
                "refuse visibility untouched") &&
         expect(facade.document().revision() == revisionAfterLock,
                "refuse revision untouched") &&
         expect(!visibilityReceipt.mutation.message.empty(),
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

bool catalogRowsRouteToKnownCommandsAndHandlers() {
  bool ok = true;
  for (const iggy3d::ProductCreativeUiCommandCatalogEntry& entry :
       iggy3d::productCreativeUiCommandCatalog()) {
    const iggy3d::ProductCreativeUiCommandKindMetadata* metadata =
        iggy3d::findProductCreativeUiCommandKindMetadata(entry.commandKind);
    cr::CreativeAppState app;
    const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
        routeCommand(app, entry.semanticId);
    ok &= expect(receipt.semanticId == entry.semanticId,
                 "catalog semantic routed") &&
          expect(receipt.commandKind == entry.commandKind,
                 "catalog command kind routed") &&
          expect(receipt.status !=
                     "product_creative_ui_command_unknown_semantic",
                 "catalog row known semantic") &&
          expect(receipt.status !=
                     "product_creative_ui_command_unknown_command",
                 "catalog row has handler") &&
          expect(metadata != nullptr, "catalog row has kind metadata") &&
          expect(metadata != nullptr && metadata->expectsHandler,
                 "catalog row expects handler") &&
          expect(iggy3d::productCreativeUiCommandKindHasHandler(
                     entry.commandKind),
                 "catalog command kind has handler");
  }
  return ok;
}

bool commandKindMetadataIsIndependentOfRowOrder() {
  bool ok = true;
  std::uint64_t setActiveToolRows = 0;
  std::uint64_t commandCatalogCreateRows = 0;
  for (const iggy3d::ProductCreativeUiCommandCatalogEntry& entry :
       iggy3d::productCreativeUiCommandCatalog()) {
    setActiveToolRows +=
        entry.commandKind == iggy3d::ProductCreativeUiCommandKind::SetActiveTool
            ? 1U
            : 0U;
    commandCatalogCreateRows +=
        entry.commandKind == iggy3d::ProductCreativeUiCommandKind::CreateObject
            ? 1U
            : 0U;
  }

  for (const iggy3d::ProductCreativeUiCommandKindMetadata& metadata :
       iggy3d::productCreativeUiCommandKindMetadataCatalog()) {
    ok &= expect(!metadata.receiptName.empty(),
                 "metadata receipt name present") &&
          expect(iggy3d::productCreativeUiCommandKindReceiptName(
                     metadata.commandKind) == metadata.receiptName,
                 "metadata receipt name lookup") &&
          expect(iggy3d::productCreativeUiCommandKindExpectsHandler(
                     metadata.commandKind) == metadata.expectsHandler,
                 "metadata handler expectation lookup");
    if (metadata.expectsHandler) {
      ok &= expect(iggy3d::productCreativeUiCommandKindHasHandler(
                       metadata.commandKind),
                   "metadata expected handler present");
    }
  }

  return expect(setActiveToolRows == 4U,
                "set active tool has duplicate rows") &&
         expect(commandCatalogCreateRows == 0U,
                "create object rows live outside command catalog") &&
         expect(iggy3d::productCreativeUiCreatePalette().size() == 2U,
                "create object palette row count") &&
         expect(iggy3d::productCreativeUiCommandKindReceiptName(
                    iggy3d::ProductCreativeUiCommandKind::SetActiveTool) ==
                    "set_active_tool",
                "set active tool receipt stable") &&
         expect(iggy3d::productCreativeUiCommandKindReceiptName(
                    iggy3d::ProductCreativeUiCommandKind::CreateObject) ==
                    "create_object",
                "create object receipt stable") &&
         ok;
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
  ok &= rebuildRoomCommandRequestsExternalRefreshWithoutDocumentMutation();
  ok &= undoCommandNoHistoryRejectsWithoutMutation();
  ok &= undoCommandRestoresLatestSnapshotAndClearsTransientState();
  ok &= createRoomCommandCreatesGenericRoom();
  ok &= repeatedCreateRoomCommandCreatesNewIdsAndRevisions();
  ok &= createPaletteRowsRouteToDescriptorKinds();
  ok &= selectedTargetRowTogglesRoomVisibilityOff();
  ok &= selectedTargetRowTogglesRoomVisibilityOnAgain();
  ok &= selectedTargetRowPreservesActiveTool();
  ok &= selectedTargetRowNoSelectionRejects();
  ok &= deleteSelectedObjectRemovesObjectAndClearsSelection();
  ok &= deleteSelectedObjectNoSelectionRejectsWithoutMutation();
  ok &= generateRoomShellInstallsGeneratedChildrenAtomically();
  ok &= generateRoomShellNoSelectionRejectsWithoutMutation();
  ok &= generateRoomShellNonRoomRejectsWithoutMutation();
  ok &= generateRoomShellDuplicateRejectsWithoutMutation();
  ok &= removeRoomShellDeletesOnlyGeneratedChildrenAtomically();
  ok &= removeRoomShellWithoutGeneratedChildrenRejectsWithoutMutation();
  ok &= selectedTargetRowMissingObjectRejectsAndPreservesSelection();
  ok &= selectedTargetRowIsDisplayOnlyNoop();
  ok &= inspectorLockedRowTogglesRoomLockedOn();
  ok &= inspectorLockedRowTogglesRoomLockedOffAgain();
  ok &= inspectorLockedRowNoSelectionRejects();
  ok &= lockedObjectRefusesVisibilityMutationWithReceipt();
  ok &= nonToolRowsRemainUnknownNoop();
  ok &= catalogRowsRouteToKnownCommandsAndHandlers();
  ok &= commandKindMetadataIsIndependentOfRowOrder();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
