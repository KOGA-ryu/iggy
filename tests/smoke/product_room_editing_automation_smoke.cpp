#include "AutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool editedAsciiRoomState(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "room_edit.redo") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "room_edit.redo") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_status",
                                 "product_room_editing_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_reason_code",
                                 "product_room_editing_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_floor_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_wall_count",
                                 "9") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_active_room_loaded",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_active_room_static_mesh_count",
                                 "11") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_active_room_spatial_surface_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_authored_floor_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_authored_wall_count",
                                 "9") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_collision_surface_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_collision_walkable_surface_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_collision_actor_blocker_count",
                                 "9") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_collision_projectile_blocker_count",
                                 "9") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_undo_depth",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_redo_depth",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_last_operation",
                                 "room_edit.redo") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_last_operation_status",
                                 "product_room_editing_redo_applied") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_last_operation_reason_code",
                                 "product_room_editing_redo_applied") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_last_input_source",
                                 "script") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_last_operation_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editing_last_primitive_id",
                                 "editor_wall_1") &&
         iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
         iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_authored_floor_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_authored_wall_count",
                                 "9") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_static_mesh_count",
                                 "11") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_spatial_surface_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_query_surface_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_walkable_surface_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "false") &&
         iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
         iggy3d::smoke::hasField(fields, "window_created", "false");
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const bool receipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "room_editing_automation",
          "ascii_room.room_id=automation_edit_state_room\n"
          "ascii_room.source_name=automation/edit_state_room.iggyroom.txt\n"
          "ascii_room.text=###\\n#P#\\n###\\n\n"
          "room_edit.start=true\n"
          "room_edit.add_floor=editor_floor_1,2,-0.05,0,1,0.1,1\n"
          "room_edit.add_wall=editor_wall_1,2,0,-0.5,3,0,-0.5,0,2.5,1\n"
          "room_edit.undo=true\n"
          "room_edit.redo=true\n",
          "",
          fields,
          exitCode);

  const bool passed = exitCode == 0 && receipt &&
                      iggy3d::smoke::productReceipt(fields) &&
                      editedAsciiRoomState(fields);
  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(receipt, "receipt parsed") &&
                  expect(passed, "room editing automation state ready");

  std::cout << "smoke=product_room_editing_automation\n";
  std::cout << "room_editing_state=" << (passed ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result="
            << (ok ? "pass" : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (ok ? "product_room_editing_automation_pass"
                   : (appAvailable ? "product_room_editing_automation_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (ok) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}
