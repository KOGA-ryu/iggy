#include "ProductAutomationSmokeSupport.hpp"

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

bool validAsciiPreview(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_status",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_reason_code",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_failed_stage",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_room_id",
                                 "automation_training_room") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_source_name",
                                 "automation/training_room.iggyroom.txt") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "7") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_floor_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_wall_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_marker_count",
                                 "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_static_mesh_count",
                                 "35") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_anchor_count",
                                 "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_spatial_surface_count",
                                 "55") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_asset_text_written",
                                 "true") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "ascii_room_preview_asset_text_bytes") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "false");
}

bool invalidAsciiPreview(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationCommandFailed(fields, "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_status",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_reason_code",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_failed_stage",
                                 "grid") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_room_id",
                                 "missing_spawn_room") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_source_name",
                                 "automation/missing_spawn.iggyroom.txt") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ready",
                                 "false") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "3") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "2") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_asset_text_written",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_asset_text_bytes",
                                 "0") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "false");
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int validExitCode = 77;
  iggy3d::smoke::ReceiptFields validFields;
  const bool validReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_valid",
          "ascii_room.room_id=automation_training_room\n"
          "ascii_room.source_name=automation/training_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P..N.#\\n#.+.$.#\\n#..E..#\\n#######\\n\n"
          "ascii_room.build=true\n",
          "",
          validFields,
          validExitCode);

  int invalidExitCode = 77;
  iggy3d::smoke::ReceiptFields invalidFields;
  const bool invalidReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_invalid",
          "ascii_room.room_id=missing_spawn_room\n"
          "ascii_room.source_name=automation/missing_spawn.iggyroom.txt\n"
          "ascii_room.text=...\\n...\\n\n"
          "ascii_room.build=true\n",
          "",
          invalidFields,
          invalidExitCode);

  const bool validPassed =
      validExitCode == 0 && validReceipt &&
      iggy3d::smoke::productReceipt(validFields) &&
      validAsciiPreview(validFields);
  const bool invalidPassed =
      invalidExitCode == 0 && invalidReceipt &&
      iggy3d::smoke::productReceipt(invalidFields) &&
      invalidAsciiPreview(invalidFields);

  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(validReceipt, "valid receipt parsed") &&
                  expect(validPassed, "valid ascii room preview") &&
                  expect(invalidReceipt, "invalid receipt parsed") &&
                  expect(invalidPassed, "invalid ascii room rejected");

  std::cout << "smoke=product_ascii_authoring\n";
  std::cout << "valid_preview=" << (validPassed ? "true" : "false") << "\n";
  std::cout << "invalid_preview_rejected="
            << (invalidPassed ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result="
            << (ok ? "pass" : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (ok ? "product_ascii_authoring_pass"
                   : (appAvailable ? "product_ascii_authoring_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (ok) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}
