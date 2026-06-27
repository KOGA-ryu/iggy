#include "AutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>

namespace {

bool softDeleteSave(const std::filesystem::path& binary,
                    const std::filesystem::path& saveRoot,
                    std::string_view caseName,
                    iggy3d::smoke::ReceiptFields& fields,
                    int& exitCode) {
  fields.clear();
  return iggy3d::smoke::runProductCase(
             binary,
             caseName,
             "frontend.select=load_save\nfrontend.execute=true\n"
             "save.select=save_001\nsave.delete=true\nmenu.confirm=true\n",
             iggy3d::smoke::saveRootArg(saveRoot),
             fields,
             exitCode) &&
         exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields, "save_delete_status",
                                 "product_save_soft_deleted") &&
         iggy3d::smoke::hasField(fields, "save_delete_executed", "true");
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const std::filesystem::path deleteRoot =
      iggy3d::smoke::cleanSaveRoot("save_delete");
  const bool seed = appAvailable &&
                    iggy3d::smoke::seedWorldSave(binary, "save_delete_seed",
                                                 deleteRoot, "Delete World");

  const bool loadSaveDeleteConfirmOpen =
      appAvailable && seed &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_delete_confirm_open",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.select=save_001\nsave.delete=true\n",
          iggy3d::smoke::saveRootArg(deleteRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "delete_confirm") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "delete") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "selected_save_enabled", "true") &&
      iggy3d::smoke::hasField(fields, "selected_save_status", "selected") &&
      iggy3d::smoke::hasField(fields, "save_delete_confirmation_open", "true") &&
      iggy3d::smoke::hasField(fields, "save_delete_candidate_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "save_delete_candidate_enabled", "true") &&
      iggy3d::smoke::hasField(fields, "save_delete_status", "confirm_open") &&
      iggy3d::smoke::hasField(fields, "save_delete_reason_code", "confirm_open") &&
      iggy3d::smoke::hasField(fields, "save_delete_type", "soft") &&
      iggy3d::smoke::hasField(fields, "save_delete_recoverable", "false") &&
      iggy3d::smoke::hasField(fields, "save_delete_executed", "false") &&
      std::filesystem::exists(deleteRoot / "save_001.iggy3d.save");

  fields.clear();
  const bool loadSaveDeleteCancel =
      appAvailable && seed &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_delete_cancel",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.select=save_001\nsave.delete=true\nmenu.back=true\n",
          iggy3d::smoke::saveRootArg(deleteRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "delete") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "save_delete_confirmation_open", "false") &&
      iggy3d::smoke::hasField(fields, "save_delete_candidate_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "save_delete_status", "cancelled") &&
      iggy3d::smoke::hasField(fields, "save_delete_reason_code", "cancelled") &&
      iggy3d::smoke::hasField(fields, "save_delete_type", "soft") &&
      iggy3d::smoke::hasField(fields, "save_delete_recoverable", "false") &&
      iggy3d::smoke::hasField(fields, "save_delete_executed", "false") &&
      std::filesystem::exists(deleteRoot / "save_001.iggy3d.save") &&
      !std::filesystem::exists(deleteRoot / "deleted" / "save_001.iggy3d.save");

  fields.clear();
  const bool loadSaveDeleteConfirmSoftDeleted =
      appAvailable && seed &&
      softDeleteSave(binary, deleteRoot, "load_save_delete_confirm_soft_deleted",
                     fields, exitCode) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "delete") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "selected_save_enabled", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_status", "missing") &&
      iggy3d::smoke::hasField(fields, "save_delete_confirmation_open", "false") &&
      iggy3d::smoke::hasField(fields, "save_delete_candidate_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "save_delete_reason_code",
                              "product_save_soft_deleted") &&
      iggy3d::smoke::hasField(fields, "save_delete_type", "soft") &&
      iggy3d::smoke::hasField(fields, "save_delete_recoverable", "true") &&
      !std::filesystem::exists(deleteRoot / "save_001.iggy3d.save") &&
      std::filesystem::exists(deleteRoot / "deleted" / "save_001.iggy3d.save");

  const std::filesystem::path recoverRoot =
      iggy3d::smoke::cleanSaveRoot("recover_soft_deleted");
  std::error_code recoverSetupError;
  std::filesystem::create_directories(recoverRoot / "deleted", recoverSetupError);
  recoverSetupError.clear();
  const bool recoverSetup =
      loadSaveDeleteConfirmSoftDeleted &&
      std::filesystem::copy_file(
          deleteRoot / "deleted" / "save_001.iggy3d.save",
          recoverRoot / "deleted" / "save_001.iggy3d.save",
          std::filesystem::copy_options::overwrite_existing,
          recoverSetupError) &&
      !recoverSetupError;

  fields.clear();
  const bool loadSaveRecoverSoftDeleted =
      appAvailable && recoverSetup &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_recover_soft_deleted",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.show_deleted=true\nsave.deleted_select=save_001\n"
          "save.recover=true\n",
          iggy3d::smoke::saveRootArg(recoverRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "load_save") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "selected_save_enabled", "true") &&
      iggy3d::smoke::hasField(fields, "selected_save_status", "selected") &&
      iggy3d::smoke::hasField(fields, "deleted_save_browser_open", "false") &&
      iggy3d::smoke::hasField(fields, "deleted_save_count", "0") &&
      iggy3d::smoke::hasField(fields, "deleted_compatible_save_count", "0") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_id", "none") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_enabled", "false") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_status", "empty") &&
      iggy3d::smoke::hasField(fields, "save_recover_status",
                              "product_save_recovered") &&
      iggy3d::smoke::hasField(fields, "save_recover_reason_code",
                              "product_save_recovered") &&
      iggy3d::smoke::hasField(fields, "save_recover_executed", "true") &&
      iggy3d::smoke::hasField(fields, "save_recover_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "save_recover_snapshot_missing", "true") &&
      std::filesystem::exists(recoverRoot / "save_001.iggy3d.save") &&
      !std::filesystem::exists(recoverRoot / "deleted" / "save_001.iggy3d.save");

  const std::filesystem::path snapshotRoot =
      iggy3d::smoke::cleanSaveRoot("recover_snapshot_sidecar");
  const bool snapshotSeed =
      appAvailable && iggy3d::smoke::seedWorldSave(binary, "snapshot_seed",
                                                   snapshotRoot, "Snapshot World");
  const bool snapshotSidecarReady =
      snapshotSeed &&
      iggy3d::smoke::writeTextFile(snapshotRoot / "save_001.snapshot.png",
                                   "snapshot sidecar bytes\n");
  fields.clear();
  const bool recoverSnapshotSoftDelete =
      appAvailable && snapshotSidecarReady &&
      softDeleteSave(binary, snapshotRoot, "recover_snapshot_soft_delete", fields,
                     exitCode) &&
      !std::filesystem::exists(snapshotRoot / "save_001.iggy3d.save") &&
      !std::filesystem::exists(snapshotRoot / "save_001.snapshot.png") &&
      std::filesystem::exists(snapshotRoot / "deleted" / "save_001.iggy3d.save") &&
      std::filesystem::exists(snapshotRoot / "deleted" / "save_001.snapshot.png");

  fields.clear();
  const bool loadSaveRecoverSnapshotSidecar =
      appAvailable && recoverSnapshotSoftDelete &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_recover_snapshot_sidecar",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.show_deleted=true\nsave.deleted_select=save_001\n"
          "save.recover=true\n",
          iggy3d::smoke::saveRootArg(snapshotRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "selected_save_enabled", "true") &&
      iggy3d::smoke::hasField(fields, "selected_save_status", "selected") &&
      iggy3d::smoke::hasField(fields, "save_recover_status",
                              "product_save_recovered") &&
      iggy3d::smoke::hasField(fields, "save_recover_reason_code",
                              "product_save_recovered") &&
      iggy3d::smoke::hasField(fields, "save_recover_executed", "true") &&
      iggy3d::smoke::hasField(fields, "save_recover_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "save_recover_snapshot_recovered", "true") &&
      iggy3d::smoke::hasField(fields, "save_recover_snapshot_missing", "false") &&
      std::filesystem::exists(snapshotRoot / "save_001.iggy3d.save") &&
      std::filesystem::exists(snapshotRoot / "save_001.snapshot.png") &&
      !std::filesystem::exists(snapshotRoot / "deleted" / "save_001.iggy3d.save") &&
      !std::filesystem::exists(snapshotRoot / "deleted" / "save_001.snapshot.png");

  const std::filesystem::path emptyRecoverRoot =
      iggy3d::smoke::cleanSaveRoot("recover_empty_deleted");
  const bool emptyRecoverSeed =
      appAvailable && iggy3d::smoke::seedWorldSave(binary, "recover_empty_seed",
                                                   emptyRecoverRoot, "Recover Empty");
  fields.clear();
  const bool loadSaveRecoverMissingSelection =
      appAvailable && emptyRecoverSeed &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_recover_missing_selection",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.show_deleted=true\nsave.recover=true\n",
          iggy3d::smoke::saveRootArg(emptyRecoverRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "deleted_save_browser_open", "true") &&
      iggy3d::smoke::hasField(fields, "deleted_save_count", "0") &&
      iggy3d::smoke::hasField(fields, "deleted_compatible_save_count", "0") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_id", "none") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_enabled", "false") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_status", "empty") &&
      iggy3d::smoke::hasField(fields, "save_recover_status",
                              "product_save_recover_id_missing") &&
      iggy3d::smoke::hasField(fields, "save_recover_reason_code",
                              "product_save_recover_id_missing") &&
      iggy3d::smoke::hasField(fields, "save_recover_executed", "false") &&
      iggy3d::smoke::hasField(fields, "save_recover_save_id", "none") &&
      iggy3d::smoke::automationCommandFailed(fields, "save.recover") &&
      std::filesystem::exists(emptyRecoverRoot / "save_001.iggy3d.save") &&
      !std::filesystem::exists(emptyRecoverRoot / "deleted" /
                               "save_001.iggy3d.save");

  const std::filesystem::path collisionRoot =
      iggy3d::smoke::cleanSaveRoot("recover_target_collision");
  const bool collisionSeed =
      appAvailable && iggy3d::smoke::seedWorldSave(binary, "recover_collision_seed",
                                                   collisionRoot, "Recover Collision");
  std::error_code collisionError;
  std::filesystem::create_directories(collisionRoot / "deleted", collisionError);
  collisionError.clear();
  const bool collisionSetup =
      collisionSeed &&
      std::filesystem::copy_file(
          collisionRoot / "save_001.iggy3d.save",
          collisionRoot / "deleted" / "save_001.iggy3d.save",
          std::filesystem::copy_options::overwrite_existing,
          collisionError) &&
      !collisionError;

  fields.clear();
  const bool loadSaveRecoverTargetCollision =
      appAvailable && collisionSetup &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_recover_target_collision",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.show_deleted=true\nsave.deleted_select=save_001\n"
          "save.recover=true\n",
          iggy3d::smoke::saveRootArg(collisionRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "deleted_save_browser_open", "true") &&
      iggy3d::smoke::hasField(fields, "deleted_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "deleted_compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_enabled", "true") &&
      iggy3d::smoke::hasField(fields, "deleted_selected_save_status", "selected") &&
      iggy3d::smoke::hasField(fields, "save_recover_status",
                              "recover_save_target_exists") &&
      iggy3d::smoke::hasField(fields, "save_recover_reason_code",
                              "recover_save_target_exists") &&
      iggy3d::smoke::hasField(fields, "save_recover_executed", "false") &&
      iggy3d::smoke::hasField(fields, "save_recover_save_id", "save_001") &&
      iggy3d::smoke::automationCommandFailed(fields, "save.recover") &&
      std::filesystem::exists(collisionRoot / "save_001.iggy3d.save") &&
      std::filesystem::exists(collisionRoot / "deleted" / "save_001.iggy3d.save");

  const bool passed = seed && loadSaveDeleteConfirmOpen && loadSaveDeleteCancel &&
                      loadSaveDeleteConfirmSoftDeleted &&
                      loadSaveRecoverSoftDeleted &&
                      loadSaveRecoverSnapshotSidecar &&
                      loadSaveRecoverMissingSelection &&
                      loadSaveRecoverTargetCollision;
  std::cout << "smoke=product_save_delete_recover\n";
  std::cout << "seed=" << (seed ? "true" : "false") << "\n";
  std::cout << "load_save_delete_confirm_open="
            << (loadSaveDeleteConfirmOpen ? "true" : "false") << "\n";
  std::cout << "load_save_delete_cancel="
            << (loadSaveDeleteCancel ? "true" : "false") << "\n";
  std::cout << "load_save_delete_confirm_soft_deleted="
            << (loadSaveDeleteConfirmSoftDeleted ? "true" : "false") << "\n";
  std::cout << "load_save_recover_soft_deleted="
            << (loadSaveRecoverSoftDeleted ? "true" : "false") << "\n";
  std::cout << "load_save_recover_snapshot_sidecar="
            << (loadSaveRecoverSnapshotSidecar ? "true" : "false") << "\n";
  std::cout << "load_save_recover_missing_selection="
            << (loadSaveRecoverMissingSelection ? "true" : "false") << "\n";
  std::cout << "load_save_recover_target_collision="
            << (loadSaveRecoverTargetCollision ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_save_delete_recover_pass"
                       : (appAvailable ? "product_save_delete_recover_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}
