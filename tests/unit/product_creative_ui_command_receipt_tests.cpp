#include "app/iggy3d/ReceiptBuilder.hpp"

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "render/RenderDiagnostics.hpp"

#include <cstdint>
#include <cstdlib>
#include <array>
#include <initializer_list>
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

struct ReceiptFieldExpectation {
  std::string_view key;
  std::string_view value;
  std::string_view message;
};

bool expectReceiptFields(
    const iggy3d::RenderReceipt& receipt,
    std::initializer_list<ReceiptFieldExpectation> expectations,
    std::string_view group) {
  bool ok = true;
  for (const ReceiptFieldExpectation& expectation : expectations) {
    std::string message(group);
    message.append(": ");
    message.append(expectation.message);
    ok = expectReceiptField(receipt,
                            expectation.key,
                            expectation.value,
                            message) &&
         ok;
  }
  return ok;
}

template <std::size_t Size>
bool expectReceiptFields(
    const iggy3d::RenderReceipt& receipt,
    const std::array<ReceiptFieldExpectation, Size>& expectations,
    std::string_view group) {
  bool ok = true;
  for (const ReceiptFieldExpectation& expectation : expectations) {
    std::string message(group);
    message.append(": ");
    message.append(expectation.message);
    ok = expectReceiptField(receipt,
                            expectation.key,
                            expectation.value,
                            message) &&
         ok;
  }
  return ok;
}

constexpr std::array<ReceiptFieldExpectation, 14>
    kDefaultCommandCoreReceiptFields{{
        {"creative_ui_command_requested", "false", "requested"},
        {"creative_ui_command_facade_available", "false", "facade"},
        {"creative_ui_command_input_consumed", "false", "consumed"},
        {"creative_ui_command_input_enabled", "false", "enabled"},
        {"creative_ui_command_accepted", "false", "accepted"},
        {"creative_ui_command_changed", "false", "changed"},
        {"creative_ui_command_kind", "none", "kind"},
        {"creative_ui_command_tool", "none", "tool"},
        {"creative_ui_command_object_kind", "Unknown", "object kind"},
        {"creative_ui_command_tool_before", "Select", "tool before"},
        {"creative_ui_command_tool_after", "Select", "tool after"},
        {"creative_ui_command_semantic_id", "none", "semantic"},
        {"creative_ui_command_status",
         "product_creative_ui_command_not_requested",
         "status"},
        {"creative_ui_command_reason_code",
         "product_creative_ui_command_not_requested",
         "reason"},
    }};

constexpr std::array<ReceiptFieldExpectation, 16>
    kDefaultCommandMutationReceiptFields{{
        {"creative_ui_command_mutation_requested", "false", "requested"},
        {"creative_ui_command_mutation_accepted", "false", "accepted"},
        {"creative_ui_command_mutation_changed", "false", "changed"},
        {"creative_ui_command_mutation_status", "Unknown", "status"},
        {"creative_ui_command_document_mutation_status",
         "Unknown",
         "document status"},
        {"creative_ui_command_mutation_kind", "Unknown", "kind"},
        {"creative_ui_command_mutation_target", "0", "target"},
        {"creative_ui_command_mutation_object_id", "0", "object id"},
        {"creative_ui_command_mutation_object_kind",
         "Unknown",
         "object kind"},
        {"creative_ui_command_visible_before", "false", "visible before"},
        {"creative_ui_command_visible_after", "false", "visible after"},
        {"creative_ui_command_locked_before", "false", "locked before"},
        {"creative_ui_command_locked_after", "false", "locked after"},
        {"creative_ui_command_revision_before", "0", "revision before"},
        {"creative_ui_command_revision_after", "0", "revision after"},
        {"creative_ui_command_mutation_message", "none", "message"},
    }};

constexpr std::array<ReceiptFieldExpectation, 12>
    kDefaultCommandCreateReceiptFields{{
        {"creative_ui_command_create_requested", "false", "requested"},
        {"creative_ui_command_create_accepted", "false", "accepted"},
        {"creative_ui_command_create_changed", "false", "changed"},
        {"creative_ui_command_create_status", "Unknown", "status"},
        {"creative_ui_command_create_object_id", "0", "object id"},
        {"creative_ui_command_create_object_kind", "Unknown", "object kind"},
        {"creative_ui_command_create_object_name", "none", "object name"},
        {"creative_ui_command_create_revision_before",
         "0",
         "revision before"},
        {"creative_ui_command_create_revision_after",
         "0",
         "revision after"},
        {"creative_ui_command_create_dirty_flags", "0", "dirty flags"},
        {"creative_ui_command_create_message", "none", "message"},
        {"creative_ui_command_create_reason_code", "none", "reason"},
    }};

constexpr std::array<ReceiptFieldExpectation, 13>
    kDefaultCommandDeleteReceiptFields{{
        {"creative_ui_command_delete_requested", "false", "requested"},
        {"creative_ui_command_delete_accepted", "false", "accepted"},
        {"creative_ui_command_delete_changed", "false", "changed"},
        {"creative_ui_command_delete_removed", "false", "removed"},
        {"creative_ui_command_delete_object_id", "0", "object id"},
        {"creative_ui_command_delete_object_kind", "Unknown", "object kind"},
        {"creative_ui_command_delete_object_name", "none", "object name"},
        {"creative_ui_command_delete_revision_before",
         "0",
         "revision before"},
        {"creative_ui_command_delete_revision_after",
         "0",
         "revision after"},
        {"creative_ui_command_delete_dirty_flags", "0", "dirty flags"},
        {"creative_ui_command_delete_status", "Unknown", "status"},
        {"creative_ui_command_delete_message", "none", "message"},
        {"creative_ui_command_delete_reason_code", "none", "reason"},
    }};

constexpr std::array<ReceiptFieldExpectation, 14>
    kDefaultCommandUndoReceiptFields{{
        {"creative_ui_command_undo_requested", "false", "requested"},
        {"creative_ui_command_undo_accepted", "false", "accepted"},
        {"creative_ui_command_undo_changed", "false", "changed"},
        {"creative_ui_command_undo_had_snapshot", "false", "snapshot"},
        {"creative_ui_command_undo_document_id", "0", "document"},
        {"creative_ui_command_undo_revision_before",
         "0",
         "revision before"},
        {"creative_ui_command_undo_revision_after", "0", "revision after"},
        {"creative_ui_command_undo_object_count_before",
         "0",
         "object count before"},
        {"creative_ui_command_undo_object_count_after",
         "0",
         "object count after"},
        {"creative_ui_command_undo_depth_before", "0", "depth before"},
        {"creative_ui_command_undo_depth_after", "0", "depth after"},
        {"creative_ui_command_undo_status",
         "creative_undo_not_requested",
         "status"},
        {"creative_ui_command_undo_message",
         "creative_undo_not_requested",
         "message"},
        {"creative_ui_command_undo_reason_code",
         "creative_undo_not_requested",
         "reason"},
    }};

constexpr std::array<ReceiptFieldExpectation, 13>
    kDefaultCommandRoomShellReceiptFields{{
        {"creative_ui_command_shell_requested", "false", "requested"},
        {"creative_ui_command_shell_accepted", "false", "accepted"},
        {"creative_ui_command_shell_changed", "false", "changed"},
        {"creative_ui_command_shell_room_object_id", "0", "room id"},
        {"creative_ui_command_shell_generated_object_count",
         "0",
         "generated count"},
        {"creative_ui_command_shell_removed_object_count",
         "0",
         "removed count"},
        {"creative_ui_command_shell_floor_count", "0", "floor count"},
        {"creative_ui_command_shell_wall_count", "0", "wall count"},
        {"creative_ui_command_shell_revision_before",
         "0",
         "revision before"},
        {"creative_ui_command_shell_revision_after", "0", "revision after"},
        {"creative_ui_command_shell_status",
         "creative_room_shell_not_requested",
         "status"},
        {"creative_ui_command_shell_reason_code",
         "creative_room_shell_not_requested",
         "reason"},
        {"creative_ui_command_shell_message",
         "creative_room_shell_not_requested",
         "message"},
    }};

// Manual command refresh intentionally omits cleared_active_room. The separate
// auto-refresh receipt namespace owns that key because only auto-refresh needs
// to distinguish renderable rebuilds from same-frame no-renderable clears.
constexpr std::array<ReceiptFieldExpectation, 12>
    kDefaultCommandBakedRoomRefreshReceiptFields{{
        {"creative_ui_command_baked_room_refresh_requested",
         "false",
         "requested"},
        {"creative_ui_command_baked_room_refresh_accepted",
         "false",
         "accepted"},
        {"creative_ui_command_baked_room_refresh_status",
         "product_creative_baked_room_not_requested",
         "status"},
        {"creative_ui_command_baked_room_refresh_reason_code",
         "product_creative_baked_room_not_requested",
         "reason"},
        {"creative_ui_command_baked_room_bake_measured", "false", "measured"},
        {"creative_ui_command_baked_room_bake_elapsed_microseconds",
         "0",
         "elapsed"},
        {"creative_ui_command_baked_room_baked_document_revision",
         "0",
         "document revision"},
        {"creative_ui_command_baked_room_static_mesh_count",
         "0",
         "static meshes"},
        {"creative_ui_command_baked_room_anchor_count", "0", "anchors"},
        {"creative_ui_command_baked_room_spatial_surface_count",
         "0",
         "surfaces"},
        {"creative_ui_command_baked_room_collision_ready",
         "false",
         "collision ready"},
        {"creative_ui_command_baked_room_collision_query_surface_count",
         "0",
         "query surfaces"},
    }};

bool expectDefaultCommandReceiptSchema(
    const iggy3d::RenderReceipt& receipt) {
  return expectReceiptFields(receipt,
                             kDefaultCommandCoreReceiptFields,
                             "default command core") &&
         expectReceiptFields(receipt,
                             kDefaultCommandMutationReceiptFields,
                             "default mutation") &&
         expectReceiptFields(receipt,
                             kDefaultCommandCreateReceiptFields,
                             "default create") &&
         expectReceiptFields(receipt,
                             kDefaultCommandDeleteReceiptFields,
                             "default delete") &&
         expectReceiptFields(receipt,
                             kDefaultCommandUndoReceiptFields,
                             "default undo") &&
         expectReceiptFields(receipt,
                             kDefaultCommandRoomShellReceiptFields,
                             "default room shell") &&
         expectReceiptFields(receipt,
                             kDefaultCommandBakedRoomRefreshReceiptFields,
                             "default baked room refresh");
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

iggy3d::ProductCreativeUiCommandFrameReceipt appliedRoomShellReceipt() {
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.documentForPersistence().assignId(42U));
  const cr::CreativeObjectId roomId = createRoom(facade);
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(roomId))));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt =
      commandInput("creative.row.selection.generate_room_shell");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

bool defaultWindowReceiptCarriesNotRequestedFields() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectDefaultCommandReceiptSchema(receipt);
}

bool defaultCommandReceiptRecordsSafely() {
  iggy3d::ProductAppWindowState window;
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_requested", "false", "requested"},
          {"creative_ui_command_kind", "none", "kind"},
          {"creative_ui_command_tool_before", "Select", "tool before"},
          {"creative_ui_command_tool_after", "Select", "tool after"},
          {"creative_ui_command_semantic_id", "none", "semantic"},
          {"creative_ui_command_status",
           "product_creative_ui_command_not_requested", "status"},
          {"creative_ui_command_reason_code",
           "product_creative_ui_command_not_requested", "reason"},
          {"creative_ui_command_mutation_status", "Unknown",
           "mutation status"},
          {"creative_ui_command_mutation_message", "none",
           "mutation message"},
          {"creative_ui_command_create_status", "Unknown", "create status"},
          {"creative_ui_command_create_message", "none", "create message"},
      },
      "record default command");
}

bool nullFacadeCommandReceiptRecordsFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      nullFacadeReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_requested", "true", "requested"},
          {"creative_ui_command_facade_available", "false",
           "facade unavailable"},
          {"creative_ui_command_input_consumed", "true", "consumed"},
          {"creative_ui_command_input_enabled", "true", "enabled"},
          {"creative_ui_command_kind", "none", "kind"},
          {"creative_ui_command_tool_before", "Select", "tool before"},
          {"creative_ui_command_tool_after", "Select", "tool after"},
          {"creative_ui_command_semantic_id",
           "creative.row.tools.tool_select", "semantic"},
          {"creative_ui_command_status",
           "product_creative_ui_command_facade_missing", "status"},
          {"creative_ui_command_reason_code",
           "product_creative_ui_command_facade_missing", "reason"},
      },
      "null facade command");
}

bool toolSelectNoChangeCommandReceiptRecordsFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      toolSelectNoChangeReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_requested", "true", "requested"},
          {"creative_ui_command_facade_available", "true", "facade"},
          {"creative_ui_command_input_consumed", "true", "consumed"},
          {"creative_ui_command_input_enabled", "true", "enabled"},
          {"creative_ui_command_accepted", "true", "accepted"},
          {"creative_ui_command_changed", "false", "unchanged"},
          {"creative_ui_command_kind", "set_active_tool", "kind"},
          {"creative_ui_command_tool_before", "Select", "tool before"},
          {"creative_ui_command_tool_after", "Select", "tool after"},
          {"creative_ui_command_semantic_id",
           "creative.row.tools.tool_select", "semantic"},
          {"creative_ui_command_status", "product_creative_ui_command_no_change",
           "status"},
          {"creative_ui_command_reason_code",
           "product_creative_ui_command_no_change", "reason"},
      },
      "tool select command");
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

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_kind", "toggle_selected_object_visibility",
           "kind"},
          {"creative_ui_command_status", "product_creative_ui_command_applied",
           "status"},
          {"creative_ui_command_accepted", "true", "accepted"},
          {"creative_ui_command_changed", "true", "changed"},
          {"creative_ui_command_mutation_requested", "true",
           "mutation requested"},
          {"creative_ui_command_mutation_accepted", "true",
           "mutation accepted"},
          {"creative_ui_command_mutation_changed", "true",
           "mutation changed"},
          {"creative_ui_command_mutation_status", "Applied",
           "mutation status"},
          {"creative_ui_command_document_mutation_status", "Applied",
           "document mutation status"},
          {"creative_ui_command_mutation_kind", "SetVisible",
           "mutation kind"},
          {"creative_ui_command_mutation_target", objectId,
           "mutation target"},
          {"creative_ui_command_mutation_object_id", objectId,
           "mutation object"},
          {"creative_ui_command_mutation_object_kind", "Room",
           "mutation object kind"},
          {"creative_ui_command_visible_before", "true", "visible before"},
          {"creative_ui_command_visible_after", "false", "visible after"},
          {"creative_ui_command_revision_before", revisionBefore,
           "revision before"},
          {"creative_ui_command_revision_after", revisionAfter,
           "revision after"},
          {"creative_ui_command_mutation_message",
           "document mutation applied through object mutation pipeline",
           "mutation message"},
      },
      "toggle command");
}

bool lockCommandReceiptRecordsLockedFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedLockReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_kind", "toggle_selected_object_locked", "kind"},
          {"creative_ui_command_status", "product_creative_ui_command_applied",
           "status"},
          {"creative_ui_command_mutation_kind", "SetLocked",
           "mutation kind"},
          {"creative_ui_command_locked_before", "false", "locked before"},
          {"creative_ui_command_locked_after", "true", "locked after"},
      },
      "lock command");
}

bool defaultWindowReceiptCarriesLockedFields() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_locked_before", "false", "locked before"},
          {"creative_ui_command_locked_after", "false", "locked after"},
      },
      "default locked fields");
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

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_kind", "create_object", "kind"},
          {"creative_ui_command_status", "product_creative_ui_command_applied",
           "status"},
          {"creative_ui_command_accepted", "true", "accepted"},
          {"creative_ui_command_changed", "true", "changed"},
          {"creative_ui_command_create_requested", "true", "requested"},
          {"creative_ui_command_create_accepted", "true", "receipt accepted"},
          {"creative_ui_command_create_changed", "true", "receipt changed"},
          {"creative_ui_command_create_status", "Created", "receipt status"},
          {"creative_ui_command_create_object_id", objectId, "object id"},
          {"creative_ui_command_create_object_kind", "Room", "object kind"},
          {"creative_ui_command_create_object_name", "Room", "object name"},
          {"creative_ui_command_create_revision_before", revisionBefore,
           "revision before"},
          {"creative_ui_command_create_revision_after", revisionAfter,
           "revision after"},
          {"creative_ui_command_create_dirty_flags", dirtyFlags,
           "dirty flags"},
          {"creative_ui_command_create_message", "object_created", "message"},
          {"creative_ui_command_create_reason_code", "object_created",
           "reason"},
      },
      "create room command");
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

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_kind", "delete_selected_object", "kind"},
          {"creative_ui_command_status", "product_creative_ui_command_applied",
           "status"},
          {"creative_ui_command_accepted", "true", "accepted"},
          {"creative_ui_command_changed", "true", "changed"},
          {"creative_ui_command_delete_requested", "true", "requested"},
          {"creative_ui_command_delete_accepted", "true", "receipt accepted"},
          {"creative_ui_command_delete_changed", "true", "receipt changed"},
          {"creative_ui_command_delete_removed", "true", "removed"},
          {"creative_ui_command_delete_object_id", objectId, "object id"},
          {"creative_ui_command_delete_object_kind", "Crate", "object kind"},
          {"creative_ui_command_delete_object_name", "Crate A",
           "object name"},
          {"creative_ui_command_delete_revision_before", revisionBefore,
           "revision before"},
          {"creative_ui_command_delete_revision_after", revisionAfter,
           "revision after"},
          {"creative_ui_command_delete_dirty_flags", dirtyFlags,
           "dirty flags"},
          {"creative_ui_command_delete_status", "Removed", "receipt status"},
          {"creative_ui_command_delete_message", "object_removed", "message"},
          {"creative_ui_command_delete_reason_code", "object_removed",
           "reason"},
      },
      "delete command");
}

bool undoCommandReceiptRecordsUndoFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedUndoReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);
  const std::string documentId = std::to_string(commandReceipt.undoDocumentId);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_kind", "undo_last_document_change", "kind"},
          {"creative_ui_command_status", "product_creative_ui_command_applied",
           "command status"},
          {"creative_ui_command_accepted", "true", "accepted"},
          {"creative_ui_command_changed", "true", "changed"},
          {"creative_ui_command_undo_requested", "true", "requested"},
          {"creative_ui_command_undo_accepted", "true", "receipt accepted"},
          {"creative_ui_command_undo_changed", "true", "receipt changed"},
          {"creative_ui_command_undo_had_snapshot", "true", "had snapshot"},
          {"creative_ui_command_undo_document_id", documentId, "document id"},
          {"creative_ui_command_undo_revision_before", "1",
           "revision before"},
          {"creative_ui_command_undo_revision_after", "0", "revision after"},
          {"creative_ui_command_undo_object_count_before", "1",
           "object count before"},
          {"creative_ui_command_undo_object_count_after", "0",
           "object count after"},
          {"creative_ui_command_undo_depth_before", "1", "depth before"},
          {"creative_ui_command_undo_depth_after", "0", "depth after"},
          {"creative_ui_command_undo_status", "creative_undo_applied",
           "status"},
          {"creative_ui_command_undo_message", "creative_undo_applied",
           "message"},
          {"creative_ui_command_undo_reason_code", "creative_undo_applied",
           "reason"},
      },
      "undo command");
}

bool roomShellCommandReceiptRecordsShellFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedRoomShellReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);
  const std::string roomObjectId =
      std::to_string(commandReceipt.shellRoomObjectId);

  return expectReceiptFields(
      receipt,
      {
          {"creative_ui_command_kind", "generate_selected_room_shell", "kind"},
          {"creative_ui_command_status", "product_creative_ui_command_applied",
           "command status"},
          {"creative_ui_command_accepted", "true", "accepted"},
          {"creative_ui_command_changed", "true", "changed"},
          {"creative_ui_command_shell_requested", "true", "requested"},
          {"creative_ui_command_shell_accepted", "true", "receipt accepted"},
          {"creative_ui_command_shell_changed", "true", "receipt changed"},
          {"creative_ui_command_shell_room_object_id", roomObjectId,
           "room id"},
          {"creative_ui_command_shell_generated_object_count", "5",
           "generated count"},
          {"creative_ui_command_shell_removed_object_count", "0",
           "removed count"},
          {"creative_ui_command_shell_floor_count", "1", "floor count"},
          {"creative_ui_command_shell_wall_count", "4", "wall count"},
          {"creative_ui_command_shell_revision_before", "1",
           "revision before"},
          {"creative_ui_command_shell_revision_after", "6", "revision after"},
          {"creative_ui_command_shell_status", "Generated", "status"},
          {"creative_ui_command_shell_reason_code",
           "creative_room_shell_generated", "reason"},
          {"creative_ui_command_shell_message", "creative_room_shell_generated",
           "message"},
      },
      "room shell command");
}

bool recorderPreservesNeighboringFields() {
  iggy3d::ProductAppWindowState window;
  window.frontendShell.status = "window_before";
  window.creativeAuthoring.creativeUiInput.requested = true;
  window.creativeAuthoring.creativeUiInput.consumed = true;
  window.creativeAuthoring.creativeUiInput.status = "input_before";
  window.creativeAuthoring.creativeUiInput.downstreamClickRequested = true;
  window.creativeAuthoring.creativeUiInput.downstreamClickSuppressed = true;
  window.creativeAuthoring.creativeUiInput.downstreamClickStatus = "downstream_before";
  window.creativeAuthoring.creativeViewportPickRequested = true;
  window.creativeAuthoring.creativeViewportPickStatus = "viewport_before";
  window.creativeAuthoring.creativeUiProjection.requested = true;
  window.creativeAuthoring.creativeUiProjection.status = "projection_before";
  window.frontendShell.productVulkanMenu.uiReady = true;
  window.frontendShell.productVulkanMenu.uiStatus = "vulkan_before";
  window.frontendShell.productVulkanMenu.uiSelectedAction = "resume";

  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedCreateRoomReceipt();
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(window.frontendShell.status == "window_before", "window status kept") &&
         expect(window.creativeAuthoring.creativeUiInput.requested, "input requested kept") &&
         expect(window.creativeAuthoring.creativeUiInput.consumed, "input consumed kept") &&
         expect(window.creativeAuthoring.creativeUiInput.status == "input_before",
                "input status kept") &&
         expect(window.creativeAuthoring.creativeUiInput.downstreamClickRequested,
                "downstream requested kept") &&
         expect(window.creativeAuthoring.creativeUiInput.downstreamClickSuppressed,
                "downstream suppressed kept") &&
         expect(window.creativeAuthoring.creativeUiInput.downstreamClickStatus ==
                    "downstream_before",
                "downstream status kept") &&
         expect(window.creativeAuthoring.creativeViewportPickRequested,
                "viewport requested kept") &&
         expect(window.creativeAuthoring.creativeViewportPickStatus ==
                    "viewport_before",
                "viewport status kept") &&
         expect(window.creativeAuthoring.creativeUiProjection.requested,
                "projection requested kept") &&
         expect(window.creativeAuthoring.creativeUiProjection.status == "projection_before",
                "projection status kept") &&
         expect(window.frontendShell.productVulkanMenu.uiReady, "vulkan ready kept") &&
         expect(window.frontendShell.productVulkanMenu.uiStatus == "vulkan_before",
                "vulkan status kept") &&
         expect(window.frontendShell.productVulkanMenu.uiSelectedAction == "resume",
                "vulkan action kept") &&
         expectReceiptFields(
             receipt,
             {
                 {"window_status", "window_before", "window status"},
                 {"creative_ui_input_status", "input_before", "input status"},
                 {"creative_ui_input_downstream_click_status",
                  "downstream_before", "downstream status"},
                 {"creative_viewport_pick_status", "viewport_before",
                  "viewport status"},
                 {"creative_ui_projection_status", "projection_before",
                  "projection status"},
                 {"product_vulkan_menu_ui_status", "vulkan_before",
                  "vulkan status"},
                 {"creative_ui_command_status",
                  "product_creative_ui_command_applied", "command status"},
             },
             "neighboring receipt fields");
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
                  roomShellCommandReceiptRecordsShellFields() &&
                  recorderPreservesNeighboringFields();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
