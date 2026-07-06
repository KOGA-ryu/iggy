#include "app/iggy3d/ReceiptBuilder.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"

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
  }
  return condition;
}

iggy3d::RenderReceipt receiptFor(const iggy3d::ProductAppWindowState& window) {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  return iggy3d::buildProductAppReceipt(options,
                                        world,
                                        frontend,
                                        settings,
                                        window,
                                        saves);
}

bool expectReceiptField(const iggy3d::RenderReceipt& receipt,
                        std::string_view key,
                        std::string_view value,
                        std::string_view message) {
  return expect(iggy3d::hasReceiptField(receipt, key, value), message);
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

iggy3d::ProductCreativeUiCommandFrameReceipt nullFacadeReceipt() {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.inputReceipt = commandInput("creative.row.tools.tool_select");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

iggy3d::ProductCreativeUiCommandFrameReceipt toolSelectNoChangeReceipt() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput("creative.row.tools.tool_select");
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

iggy3d::ProductCreativeUiCommandFrameReceipt appliedToggleReceipt() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(roomId))));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput("creative.row.selection.inspector_visible");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

iggy3d::ProductCreativeUiCommandFrameReceipt appliedLockReceipt() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId roomId = createRoom(facade);
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(roomId))));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput("creative.row.selection.inspector_locked");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

iggy3d::ProductCreativeUiCommandFrameReceipt appliedCreateRoomReceipt() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput("creative.row.create.create_room");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

iggy3d::ProductCreativeUiCommandFrameReceipt appliedDeleteReceipt() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const cr::CreativeObjectId crateId =
      createObject(facade, cr::CreativeObjectKind::Crate, "Crate A");
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(crateId))));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput("creative.row.selection.delete_selected");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

iggy3d::ProductCreativeUiCommandFrameReceipt appliedUndoReceipt() {
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.documentForPersistence().assignId(42U));
  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  static_cast<void>(
      createObject(facade, cr::CreativeObjectKind::Crate, "Crate A"));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = commandInput("creative.row.tools.undo");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

bool defaultWindowReceiptCarriesNotRequestedFields() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "false",
                            "default requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_facade_available",
                            "false",
                            "default facade") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_consumed",
                            "false",
                            "default consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_enabled",
                            "false",
                            "default enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "false",
                            "default accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "false",
                            "default changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "none",
                            "default kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "default tool before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "default tool after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "none",
                            "default semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_not_requested",
                            "default status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_not_requested",
                            "default reason") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_requested",
                            "false",
                            "default mutation requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_accepted",
                            "false",
                            "default mutation accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_changed",
                            "false",
                            "default mutation changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_status",
                            "Unknown",
                            "default mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_document_mutation_status",
                            "Unknown",
                            "default document mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_kind",
                            "Unknown",
                            "default mutation kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_target",
                            "0",
                            "default mutation target") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_id",
                            "0",
                            "default mutation object") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_kind",
                            "Unknown",
                            "default mutation object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_before",
                            "false",
                            "default visible before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_after",
                            "false",
                            "default visible after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_before",
                            "0",
                            "default revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_after",
                            "0",
                            "default revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_message",
                            "none",
                            "default mutation message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_requested",
                            "false",
                            "default create requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_accepted",
                            "false",
                            "default create accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_changed",
                            "false",
                            "default create changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_status",
                            "Unknown",
                            "default create status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_object_id",
                            "0",
                            "default create object id") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_object_kind",
                            "Unknown",
                            "default create object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_object_name",
                            "none",
                            "default create object name") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_revision_before",
                            "0",
                            "default create revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_revision_after",
                            "0",
                            "default create revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_dirty_flags",
                            "0",
                            "default create dirty flags") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_message",
                            "none",
                            "default create message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_reason_code",
                            "none",
                            "default create reason") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_requested",
                            "false",
                            "default delete requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_accepted",
                            "false",
                            "default delete accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_changed",
                            "false",
                            "default delete changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_removed",
                            "false",
                            "default delete removed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_object_id",
                            "0",
                            "default delete object id") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_object_kind",
                            "Unknown",
                            "default delete object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_object_name",
                            "none",
                            "default delete object name") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_revision_before",
                            "0",
                            "default delete revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_revision_after",
                            "0",
                            "default delete revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_dirty_flags",
                            "0",
                            "default delete dirty flags") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_status",
                            "Unknown",
                            "default delete status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_message",
                            "none",
                            "default delete message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_reason_code",
                            "none",
                            "default delete reason") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_requested",
                            "false",
                            "default undo requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_accepted",
                            "false",
                            "default undo accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_changed",
                            "false",
                            "default undo changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_had_snapshot",
                            "false",
                            "default undo snapshot") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_document_id",
                            "0",
                            "default undo document") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_revision_before",
                            "0",
                            "default undo revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_revision_after",
                            "0",
                            "default undo revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_object_count_before",
                            "0",
                            "default undo object count before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_object_count_after",
                            "0",
                            "default undo object count after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_depth_before",
                            "0",
                            "default undo depth before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_depth_after",
                            "0",
                            "default undo depth after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_status",
                            "creative_undo_not_requested",
                            "default undo status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_message",
                            "creative_undo_not_requested",
                            "default undo message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_reason_code",
                            "creative_undo_not_requested",
                            "default undo reason");
}

bool defaultCommandReceiptRecordsSafely() {
  iggy3d::ProductAppWindowState window;
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "false",
                            "record default requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "none",
                            "record default kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "record default before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "record default after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "none",
                            "record default semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_not_requested",
                            "record default status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_not_requested",
                            "record default reason") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_status",
                            "Unknown",
                            "record default mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_message",
                            "none",
                            "record default mutation message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_status",
                            "Unknown",
                            "record default create status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_message",
                            "none",
                            "record default create message");
}

bool nullFacadeCommandReceiptRecordsFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      nullFacadeReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "true",
                            "null facade requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_facade_available",
                            "false",
                            "null facade unavailable") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_consumed",
                            "true",
                            "null facade consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_enabled",
                            "true",
                            "null facade enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "none",
                            "null facade kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "null facade before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "null facade after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "creative.row.tools.tool_select",
                            "null facade semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_facade_missing",
                            "null facade status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_facade_missing",
                            "null facade reason");
}

bool toolSelectNoChangeCommandReceiptRecordsFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      toolSelectNoChangeReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "true",
                            "tool select requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_facade_available",
                            "true",
                            "tool select facade") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_consumed",
                            "true",
                            "tool select consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_enabled",
                            "true",
                            "tool select enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "tool select accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "false",
                            "tool select unchanged") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "set_active_tool",
                            "tool select kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "tool select before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "tool select after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "creative.row.tools.tool_select",
                            "tool select semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_no_change",
                            "tool select status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_no_change",
                            "tool select reason");
}

bool toggleCommandReceiptRecordsMutationFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedToggleReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  const std::string objectId = std::to_string(commandReceipt.mutationObjectId);
  const std::string revisionBefore =
      std::to_string(commandReceipt.revisionBefore);
  const std::string revisionAfter =
      std::to_string(commandReceipt.revisionAfter);

  return expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "toggle_selected_object_visibility",
                            "toggle kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "toggle status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "toggle accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "true",
                            "toggle changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_requested",
                            "true",
                            "toggle mutation requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_accepted",
                            "true",
                            "toggle mutation accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_changed",
                            "true",
                            "toggle mutation changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_status",
                            "Applied",
                            "toggle mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_document_mutation_status",
                            "Applied",
                            "toggle document mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_kind",
                            "SetVisible",
                            "toggle mutation kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_target",
                            objectId,
                            "toggle mutation target") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_id",
                            objectId,
                            "toggle mutation object") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_kind",
                            "Room",
                            "toggle mutation object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_before",
                            "true",
                            "toggle visible before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_after",
                            "false",
                            "toggle visible after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_before",
                            revisionBefore,
                            "toggle revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_after",
                            revisionAfter,
                            "toggle revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_message",
                            "document mutation applied through object mutation pipeline",
                            "toggle mutation message");
}

bool lockCommandReceiptRecordsLockedFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedLockReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "toggle_selected_object_locked",
                            "lock kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "lock status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_kind",
                            "SetLocked",
                            "lock mutation kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_locked_before",
                            "false",
                            "lock locked before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_locked_after",
                            "true",
                            "lock locked after");
}

bool defaultWindowReceiptCarriesLockedFields() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_locked_before",
                            "false",
                            "default locked before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_locked_after",
                            "false",
                            "default locked after");
}

bool createRoomCommandReceiptRecordsCreateFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedCreateRoomReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  const std::string objectId = std::to_string(commandReceipt.createObjectId);
  const std::string revisionBefore =
      std::to_string(commandReceipt.createRevisionBefore);
  const std::string revisionAfter =
      std::to_string(commandReceipt.createRevisionAfter);
  const std::string dirtyFlags =
      std::to_string(commandReceipt.createDirtyFlags);

  return expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "create_object",
                            "create kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "create status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "create accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "true",
                            "create changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_requested",
                            "true",
                            "create requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_accepted",
                            "true",
                            "create receipt accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_changed",
                            "true",
                            "create receipt changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_status",
                            "Created",
                            "create receipt status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_object_id",
                            objectId,
                            "create object id") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_object_kind",
                            "Room",
                            "create object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_object_name",
                            "Room",
                            "create object name") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_revision_before",
                            revisionBefore,
                            "create revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_revision_after",
                            revisionAfter,
                            "create revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_dirty_flags",
                            dirtyFlags,
                            "create dirty flags") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_message",
                            "object_created",
                            "create message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_create_reason_code",
                            "object_created",
                            "create reason");
}

bool deleteCommandReceiptRecordsDeleteFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedDeleteReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  const std::string objectId = std::to_string(commandReceipt.deleteObjectId);
  const std::string revisionBefore =
      std::to_string(commandReceipt.deleteRevisionBefore);
  const std::string revisionAfter =
      std::to_string(commandReceipt.deleteRevisionAfter);
  const std::string dirtyFlags =
      std::to_string(commandReceipt.deleteDirtyFlags);

  return expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "delete_selected_object",
                            "delete kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "delete status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "delete accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "true",
                            "delete changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_requested",
                            "true",
                            "delete requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_accepted",
                            "true",
                            "delete receipt accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_changed",
                            "true",
                            "delete receipt changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_removed",
                            "true",
                            "delete removed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_object_id",
                            objectId,
                            "delete object id") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_object_kind",
                            "Crate",
                            "delete object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_object_name",
                            "Crate A",
                            "delete object name") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_revision_before",
                            revisionBefore,
                            "delete revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_revision_after",
                            revisionAfter,
                            "delete revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_dirty_flags",
                            dirtyFlags,
                            "delete dirty flags") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_status",
                            "Removed",
                            "delete receipt status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_message",
                            "object_removed",
                            "delete message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_delete_reason_code",
                            "object_removed",
                            "delete reason");
}

bool undoCommandReceiptRecordsUndoFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedUndoReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "undo_last_document_change",
                            "undo kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "undo command status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "undo accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "true",
                            "undo changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_requested",
                            "true",
                            "undo requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_accepted",
                            "true",
                            "undo receipt accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_changed",
                            "true",
                            "undo receipt changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_had_snapshot",
                            "true",
                            "undo had snapshot") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_revision_before",
                            "1",
                            "undo revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_revision_after",
                            "0",
                            "undo revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_object_count_before",
                            "1",
                            "undo object count before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_object_count_after",
                            "0",
                            "undo object count after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_depth_before",
                            "1",
                            "undo depth before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_depth_after",
                            "0",
                            "undo depth after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_status",
                            "creative_undo_applied",
                            "undo status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_message",
                            "creative_undo_applied",
                            "undo message") &&
         expectReceiptField(receipt,
                            "creative_ui_command_undo_reason_code",
                            "creative_undo_applied",
                            "undo reason");
}

bool recorderPreservesNeighboringFields() {
  iggy3d::ProductAppWindowState window;
  window.status = "window_before";
  window.creativeUiInputRequested = true;
  window.creativeUiInputConsumed = true;
  window.creativeUiInputStatus = "input_before";
  window.creativeUiInputDownstreamClickRequested = true;
  window.creativeUiInputDownstreamClickSuppressed = true;
  window.creativeUiInputDownstreamClickStatus = "downstream_before";
  window.creativeViewportPickRequested = true;
  window.creativeViewportPickStatus = "viewport_before";
  window.creativeUiProjectionRequested = true;
  window.creativeUiProjectionStatus = "projection_before";
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiStatus = "vulkan_before";
  window.productVulkanMenuUiSelectedAction = "resume";

  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedCreateRoomReceipt();
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(window.status == "window_before", "window status kept") &&
         expect(window.creativeUiInputRequested, "input requested kept") &&
         expect(window.creativeUiInputConsumed, "input consumed kept") &&
         expect(window.creativeUiInputStatus == "input_before",
                "input status kept") &&
         expect(window.creativeUiInputDownstreamClickRequested,
                "downstream requested kept") &&
         expect(window.creativeUiInputDownstreamClickSuppressed,
                "downstream suppressed kept") &&
         expect(window.creativeUiInputDownstreamClickStatus ==
                    "downstream_before",
                "downstream status kept") &&
         expect(window.creativeViewportPickRequested,
                "viewport requested kept") &&
         expect(window.creativeViewportPickStatus == "viewport_before",
                "viewport status kept") &&
         expect(window.creativeUiProjectionRequested,
                "projection requested kept") &&
         expect(window.creativeUiProjectionStatus == "projection_before",
                "projection status kept") &&
         expect(window.productVulkanMenuUiReady, "vulkan ready kept") &&
         expect(window.productVulkanMenuUiStatus == "vulkan_before",
                "vulkan status kept") &&
         expect(window.productVulkanMenuUiSelectedAction == "resume",
                "vulkan action kept") &&
         expectReceiptField(receipt,
                            "window_status",
                            "window_before",
                            "receipt window status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_input_status",
                            "input_before",
                            "receipt input status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_status",
                            "downstream_before",
                            "receipt downstream status kept") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_status",
                            "viewport_before",
                            "receipt viewport status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "projection_before",
                            "receipt projection status kept") &&
         expectReceiptField(receipt,
                            "product_vulkan_menu_ui_status",
                            "vulkan_before",
                            "receipt vulkan status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "receipt command status recorded");
}

}  // namespace

int main() {
  const bool ok = defaultWindowReceiptCarriesNotRequestedFields() &&
                  defaultCommandReceiptRecordsSafely() &&
                  nullFacadeCommandReceiptRecordsFields() &&
                  toolSelectNoChangeCommandReceiptRecordsFields() &&
                  toggleCommandReceiptRecordsMutationFields() &&
                  lockCommandReceiptRecordsLockedFields() &&
                  defaultWindowReceiptCarriesLockedFields() &&
                  createRoomCommandReceiptRecordsCreateFields() &&
                  deleteCommandReceiptRecordsDeleteFields() &&
                  undoCommandReceiptRecordsUndoFields() &&
                  recorderPreservesNeighboringFields();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
