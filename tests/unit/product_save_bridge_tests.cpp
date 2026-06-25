#include "app/iggy3d/SaveBridge.hpp"
#include "content/PackageLoader.hpp"
#include "runtime/save/SaveCodec.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_product_save_bridge_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

iggy3d::Session makeFixtureSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({"fixtures/demos/movement_playground/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request).value;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::Session makeChangedFixtureSession() {
  iggy3d::Session session = makeFixtureSession();
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  (void)session.tick();
  return session;
}

iggy3d::ProductSaveWriteRequest productSaveRequest(
    const std::filesystem::path& root,
    const iggy3d::Session& session,
    std::string_view attemptToken,
    std::string_view idHint = "") {
  iggy3d::ProductSaveWriteRequest request;
  request.saveRoot = root;
  request.saveIdHint = std::string(idHint);
  request.attemptToken = std::string(attemptToken);
  request.state = &session.state();
  request.worldId = "world_0001";
  request.worldTitle = "Training World";
  request.saveTitle = "Manual Save";
  request.saveType = "manual";
  request.createdAtUtc = "2026-06-24T00:00:00Z";
  request.savedAtUtc = "2026-06-24T01:02:03Z";
  return request;
}

iggy3d::SaveAuthoredRoomSection authoredRoomFixture() {
  iggy3d::SaveAuthoredRoomSection authoredRoom;
  authoredRoom.present = true;
  authoredRoom.id = "product_bridge_room";
  iggy3d::SaveAuthoredRoomFloorRecord floor;
  floor.id = "bridge_floor_1";
  floor.locked = true;
  floor.hidden = true;
  floor.semantics.traversalTags = {"walkable"};
  authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomMarkerRecord marker;
  marker.id = "marker_product_bridge_treasure";
  marker.tag = "treasure";
  marker.glyph = "$";
  marker.row = 2;
  marker.column = 3;
  marker.positionMeters = {2.0F, 0.05F, 3.0F};
  marker.sourceLine = 3;
  marker.sourceColumn = 4;
  authoredRoom.markers.push_back(marker);
  return authoredRoom;
}

bool productDurableSaveWritesFinalAndScans() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001"));
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path)
                 : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");

  return expect(written.ok, "product durable write ok") &&
         expect(written.status == "product_save_written", "product status") &&
         expect(written.reasonCode == "product_save_written", "product reason") &&
         expect(written.durableReason == "durable_save_file_written",
                "product durable reason") &&
         expect(written.durableWriteRequested, "product durable requested") &&
         expect(written.record.id == "save_001", "product generated id") &&
         expect(written.record.path == written.paths.finalPath,
                "product record path") &&
         expect(written.record.savedStateHash == session.stateHash(),
                "product record hash") &&
         expect(written.tempWritten, "product temp written") &&
         expect(written.tempValidated, "product temp validated") &&
         expect(written.committed, "product committed") &&
         expect(written.finalValidated, "product final validated") &&
         expect(!written.previousExisted, "product no previous") &&
         expect(written.previousPreserved, "product previous preserved") &&
         expect(written.encodedBytes > 0U, "product encoded bytes") &&
         expect(written.worldId == "world_0001", "product world proof") &&
         expect(written.worldTitle == "Training World",
                "product world title proof") &&
         expect(written.saveTitle == "Manual Save",
                "product save title proof") &&
         expect(written.saveType == "manual", "product save type proof") &&
         expect(written.createdAtUtc == "2026-06-24T00:00:00Z",
                "product created utc proof") &&
         expect(written.savedAtUtc == "2026-06-24T01:02:03Z",
                "product saved utc proof") &&
         expect(read.ok, "product final read") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "product final decoded") &&
         expect(decoded.envelope.metadata.saveId == "save_001",
                "product metadata save id") &&
         expect(decoded.envelope.metadata.saveId == written.record.id,
                "product metadata id matches record") &&
         expect(decoded.envelope.metadata.worldId == "world_0001",
                "product metadata world id") &&
         expect(decoded.envelope.metadata.worldTitle == "Training World",
                "product metadata world title") &&
         expect(decoded.envelope.metadata.saveTitle == "Manual Save",
                "product metadata save title") &&
         expect(decoded.envelope.metadata.saveType == "manual",
                "product metadata save type") &&
         expect(decoded.envelope.metadata.createdAtUtc == "2026-06-24T00:00:00Z",
                "product metadata created utc") &&
         expect(decoded.envelope.metadata.savedAtUtc == "2026-06-24T01:02:03Z",
                "product metadata saved utc") &&
         expect(std::filesystem::exists(written.paths.finalPath),
                "product final exists") &&
         expect(!std::filesystem::exists(written.paths.tempPath),
                "product temp consumed") &&
         expect(scanned.status == "save_bridge_ready", "scan status") &&
         expect(scanned.saveRoot == root, "scan root") &&
         expect(scanned.slots.slots.size() == 1U, "scan one slot") &&
         expect(scanned.slots.compatibleCount == 1U, "scan compatible") &&
         expect(scanned.slots.slots.front().id == "save_001", "scan id") &&
         expect(scanned.slots.slots.front().enabled, "scan enabled") &&
         expect(scanned.slots.slots.front().savedStateHashHex ==
                    written.record.savedStateHashHex,
                "scan hash hex");
}

bool productDurableSaveHonorsValidIdHint() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "manual_save_01"));
  return expect(written.ok, "product id hint ok") &&
         expect(written.record.id == "manual_save_01", "product id hint id") &&
         expect(written.paths.finalPath == root / "manual_save_01.iggy3d.save",
                "product id hint final");
}

bool productDurableSaveRejectsMissingState() {
  const std::filesystem::path root = testRoot();
  iggy3d::ProductSaveWriteRequest request;
  request.saveRoot = root;
  request.attemptToken = "attempt_001";
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(request);
  return expect(!written.ok, "product missing state rejected") &&
         expect(written.status == "save_state_missing",
                "product missing state status") &&
         expect(written.reasonCode == "save_state_missing",
                "product missing state reason") &&
         expect(written.durableReason == "save_state_missing",
                "product missing durable reason") &&
         expect(written.durableWriteRequested,
                "product missing durable requested") &&
         expect(!written.tempWritten, "product missing no temp") &&
         expect(!written.committed, "product missing not committed") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product missing no slots");
}

bool productDurableSaveRejectsInvalidAttemptToken() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt 001"));
  return expect(!written.ok, "product invalid attempt rejected") &&
         expect(written.status == "durable_save_invalid_attempt_token",
                "product invalid attempt status") &&
         expect(written.reasonCode == "durable_save_invalid_attempt_token",
                "product invalid attempt reason") &&
         expect(written.durableReason == "durable_save_invalid_attempt_token",
                "product invalid durable reason") &&
         expect(!written.tempWritten, "product invalid attempt no temp") &&
         expect(!written.committed, "product invalid attempt not committed") &&
         expect(written.paths.finalPath.empty(),
                "product invalid attempt no final path") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product invalid attempt no slots");
}

bool productDurableSavePersistsAuthoredRoom() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveAuthoredRoomSection authoredRoom = authoredRoomFixture();
  iggy3d::ProductSaveWriteRequest request =
      productSaveRequest(root, session, "attempt_001", "save_010");
  request.authoredRoom = &authoredRoom;
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(request);
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path)
                 : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText)
              : iggy3d::SaveDecodeResult{};
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(written.ok, "product authored write ok") &&
         expect(read.ok, "product authored read ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "product authored decoded") &&
         expect(decoded.envelope.authoredRoom.present,
                "product authored present") &&
         expect(decoded.envelope.authoredRoom.id == "product_bridge_room",
                "product authored id") &&
         expect(decoded.envelope.authoredRoom.floors.size() == 1U,
                "product authored floor") &&
         expect(decoded.envelope.authoredRoom.floors[0].locked,
                "product authored floor locked") &&
         expect(decoded.envelope.authoredRoom.floors[0].hidden,
                "product authored floor hidden") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.traversalTags[0] ==
                    "walkable",
                "product authored traversal") &&
         expect(decoded.envelope.authoredRoom.markers.size() == 1U,
                "product authored marker") &&
         expect(decoded.envelope.authoredRoom.markers[0].tag == "treasure",
                "product authored marker tag") &&
         expect(scanned.slots.slots.size() == 1U, "product authored scan slot") &&
         expect(scanned.slots.slots.front().authoredFloorCount == 1U,
                "product authored scan floor count") &&
         expect(scanned.slots.slots.front().authoredMarkerCount == 1U,
                "product authored scan marker count");
}

bool productSoftDeleteMovesSaveAndRemovesFromScan() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(written.ok, "product soft delete setup write ok") &&
         expect(deleted.ok, "product soft delete ok") &&
         expect(deleted.status == "product_save_soft_deleted",
                "product soft delete status") &&
         expect(deleted.reasonCode == "product_save_soft_deleted",
                "product soft delete reason") &&
         expect(deleted.softDeleteReason == "soft_delete_moved",
                "product soft delete runtime reason") &&
         expect(deleted.saveId == "save_001", "product soft delete id") &&
         expect(deleted.saveMoved, "product soft delete save moved") &&
         expect(deleted.snapshotMissing, "product soft delete snapshot missing") &&
         expect(!deleted.snapshotMoved, "product soft delete snapshot not moved") &&
         expect(!std::filesystem::exists(deleted.paths.activeSavePath),
                "product soft delete active gone") &&
         expect(std::filesystem::exists(deleted.paths.deletedSavePath),
                "product soft delete deleted exists") &&
         expect(scanned.slots.slots.empty(), "product soft delete not scanned");
}

bool productSoftDeleteMovesSnapshotSidecarWhenPresent() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  {
    std::ofstream snapshot(root / "save_001.snapshot.png");
    snapshot << "snapshot bytes";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  return expect(written.ok, "product soft delete snapshot setup write ok") &&
         expect(deleted.ok, "product soft delete snapshot ok") &&
         expect(deleted.snapshotMoved, "product soft delete snapshot moved") &&
         expect(!deleted.snapshotMissing,
                "product soft delete snapshot not missing") &&
         expect(!std::filesystem::exists(deleted.paths.activeSnapshotPath),
                "product soft delete active snapshot gone") &&
         expect(std::filesystem::exists(deleted.paths.deletedSnapshotPath),
                "product soft delete deleted snapshot exists");
}

bool productSoftDeleteRejectsMissingIdBeforeIo() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, ""});
  return expect(!deleted.ok, "product soft delete missing id rejected") &&
         expect(deleted.status == "product_save_delete_id_missing",
                "product soft delete missing id status") &&
         expect(deleted.reasonCode == "product_save_delete_id_missing",
                "product soft delete missing id reason") &&
         expect(deleted.softDeleteReason == "not_requested",
                "product soft delete missing id no runtime") &&
         expect(deleted.saveId == "none", "product soft delete missing id none") &&
         expect(!deleted.saveMoved, "product soft delete missing id not moved") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product soft delete missing id no scan");
}

bool productSoftDeleteRejectsInvalidId() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save/001"});
  return expect(!deleted.ok, "product soft delete invalid id rejected") &&
         expect(deleted.status == "soft_delete_invalid_id",
                "product soft delete invalid id status") &&
         expect(deleted.reasonCode == "soft_delete_invalid_id",
                "product soft delete invalid id reason") &&
         expect(deleted.softDeleteReason == "soft_delete_invalid_id",
                "product soft delete invalid runtime") &&
         expect(deleted.saveId == "save/001", "product soft delete invalid id proof") &&
         expect(deleted.paths.activeSavePath.empty(),
                "product soft delete invalid no active path");
}

bool productSoftDeleteForwardsMissingSource() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  return expect(!deleted.ok, "product soft delete missing source rejected") &&
         expect(deleted.status == "soft_delete_source_missing",
                "product soft delete missing source status") &&
         expect(deleted.reasonCode == "soft_delete_source_missing",
                "product soft delete missing source reason") &&
         expect(deleted.softDeleteReason == "soft_delete_source_missing",
                "product soft delete missing source runtime") &&
         expect(!deleted.saveMoved, "product soft delete missing source not moved") &&
         expect(!std::filesystem::exists(deleted.paths.deletedSavePath),
                "product soft delete missing source no deleted file");
}

bool productSoftDeleteRejectsExistingDeletedTarget() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::SaveFileSoftDeletePlan plan =
      iggy3d::planSoftDeleteSaveFile(root, "save_001");
  std::filesystem::create_directories(plan.paths.deletedSavePath.parent_path());
  {
    std::ofstream existing(plan.paths.deletedSavePath);
    existing << "existing deleted target";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  return expect(written.ok, "product soft delete collision setup write ok") &&
         expect(!deleted.ok, "product soft delete collision rejected") &&
         expect(deleted.status == "soft_delete_target_exists",
                "product soft delete collision status") &&
         expect(deleted.reasonCode == "soft_delete_target_exists",
                "product soft delete collision reason") &&
         expect(deleted.softDeleteReason == "soft_delete_target_exists",
                "product soft delete collision runtime") &&
         expect(deleted.targetExisted, "product soft delete collision flag") &&
         expect(!deleted.saveMoved, "product soft delete collision not moved") &&
         expect(std::filesystem::exists(plan.paths.activeSavePath),
                "product soft delete collision active preserved") &&
         expect(std::filesystem::exists(plan.paths.deletedSavePath),
                "product soft delete collision deleted preserved");
}

bool productRecoverRestoresSoftDeletedSaveAndScans() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult scanned = iggy3d::scanProductSaves(
      root, "iggy3d.movement_playground", "movement_playground.runtime_loop");
  return expect(written.ok, "product recover setup write ok") &&
         expect(deleted.ok, "product recover setup soft delete ok") &&
         expect(recovered.ok, "product recover ok") &&
         expect(recovered.status == "product_save_recovered",
                "product recover status") &&
         expect(recovered.reasonCode == "product_save_recovered",
                "product recover reason") &&
         expect(recovered.recoverReason == "recover_save_moved",
                "product recover runtime reason") &&
         expect(recovered.saveId == "save_001", "product recover id") &&
         expect(recovered.saveRecovered, "product recover save recovered") &&
         expect(recovered.snapshotMissing, "product recover snapshot missing") &&
         expect(!recovered.snapshotRecovered,
                "product recover snapshot not recovered") &&
         expect(std::filesystem::exists(recovered.paths.activeSavePath),
                "product recover active exists") &&
         expect(!std::filesystem::exists(recovered.paths.deletedSavePath),
                "product recover deleted gone") &&
         expect(scanned.slots.slots.size() == 1U, "product recover scanned") &&
         expect(scanned.slots.compatibleCount == 1U,
                "product recover compatible") &&
         expect(scanned.slots.slots.front().id == "save_001",
                "product recover scan id");
}

bool productRecoverMovesSnapshotSidecarWhenPresent() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  {
    std::ofstream snapshot(root / "save_001.snapshot.png");
    snapshot << "snapshot bytes";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  return expect(written.ok, "product recover snapshot setup write ok") &&
         expect(deleted.ok, "product recover snapshot setup delete ok") &&
         expect(recovered.ok, "product recover snapshot ok") &&
         expect(recovered.snapshotRecovered,
                "product recover snapshot recovered") &&
         expect(!recovered.snapshotMissing,
                "product recover snapshot not missing") &&
         expect(std::filesystem::exists(recovered.paths.activeSnapshotPath),
                "product recover active snapshot exists") &&
         expect(!std::filesystem::exists(recovered.paths.deletedSnapshotPath),
                "product recover deleted snapshot gone");
}

bool productRecoverRejectsMissingIdBeforeIo() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, ""});
  return expect(!recovered.ok, "product recover missing id rejected") &&
         expect(recovered.status == "product_save_recover_id_missing",
                "product recover missing id status") &&
         expect(recovered.reasonCode == "product_save_recover_id_missing",
                "product recover missing id reason") &&
         expect(recovered.recoverReason == "not_requested",
                "product recover missing id no runtime") &&
         expect(recovered.saveId == "none", "product recover missing id none") &&
         expect(!recovered.saveRecovered,
                "product recover missing id not recovered") &&
         expect(iggy3d::scanProductSaves(root, "", "").slots.slots.empty(),
                "product recover missing id no scan");
}

bool productRecoverRejectsInvalidId() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save/001"});
  return expect(!recovered.ok, "product recover invalid id rejected") &&
         expect(recovered.status == "recover_save_invalid_id",
                "product recover invalid id status") &&
         expect(recovered.reasonCode == "recover_save_invalid_id",
                "product recover invalid id reason") &&
         expect(recovered.recoverReason == "recover_save_invalid_id",
                "product recover invalid runtime") &&
         expect(recovered.saveId == "save/001",
                "product recover invalid id proof") &&
         expect(recovered.paths.activeSavePath.empty(),
                "product recover invalid no active path");
}

bool productRecoverForwardsMissingSource() {
  const std::filesystem::path root = testRoot();
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  return expect(!recovered.ok, "product recover missing source rejected") &&
         expect(recovered.status == "recover_save_source_missing",
                "product recover missing source status") &&
         expect(recovered.reasonCode == "recover_save_source_missing",
                "product recover missing source reason") &&
         expect(recovered.recoverReason == "recover_save_source_missing",
                "product recover missing source runtime") &&
         expect(!recovered.saveRecovered,
                "product recover missing source not recovered") &&
         expect(!std::filesystem::exists(recovered.paths.activeSavePath),
                "product recover missing source no active file");
}

bool productRecoverRejectsExistingActiveTarget() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::SaveFileRecoverPlan plan =
      iggy3d::planRecoverDeletedSaveFile(root, "save_001");
  std::filesystem::create_directories(plan.paths.deletedSavePath.parent_path());
  {
    std::ofstream deleted(plan.paths.deletedSavePath);
    deleted << "deleted target";
  }
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  return expect(written.ok, "product recover collision setup write ok") &&
         expect(!recovered.ok, "product recover collision rejected") &&
         expect(recovered.status == "recover_save_target_exists",
                "product recover collision status") &&
         expect(recovered.reasonCode == "recover_save_target_exists",
                "product recover collision reason") &&
         expect(recovered.recoverReason == "recover_save_target_exists",
                "product recover collision runtime") &&
         expect(recovered.targetExisted, "product recover collision flag") &&
         expect(!recovered.saveRecovered,
                "product recover collision not recovered") &&
         expect(std::filesystem::exists(plan.paths.activeSavePath),
                "product recover collision active preserved") &&
         expect(std::filesystem::exists(plan.paths.deletedSavePath),
                "product recover collision deleted preserved");
}

bool productDeletedScanShowsSoftDeletedSaveThenClearsOnRecover() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult activeAfterDelete =
      iggy3d::scanProductSaves(root, "iggy3d.movement_playground",
                               "movement_playground.runtime_loop");
  const iggy3d::ProductSaveBridgeResult deletedAfterDelete =
      iggy3d::scanDeletedProductSaves(root, "iggy3d.movement_playground",
                                      "movement_playground.runtime_loop");
  const iggy3d::ProductSaveRecoverResult recovered =
      iggy3d::recoverProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult activeAfterRecover =
      iggy3d::scanProductSaves(root, "iggy3d.movement_playground",
                               "movement_playground.runtime_loop");
  const iggy3d::ProductSaveBridgeResult deletedAfterRecover =
      iggy3d::scanDeletedProductSaves(root, "iggy3d.movement_playground",
                                      "movement_playground.runtime_loop");
  return expect(written.ok, "product deleted scan setup write ok") &&
         expect(deleted.ok, "product deleted scan setup delete ok") &&
         expect(activeAfterDelete.status == "save_bridge_ready",
                "product deleted scan active status") &&
         expect(activeAfterDelete.slots.slots.empty(),
                "product deleted scan active empty") &&
         expect(deletedAfterDelete.status == "deleted_save_bridge_ready",
                "product deleted scan status") &&
         expect(deletedAfterDelete.saveRoot == root / "deleted",
                "product deleted scan root") &&
         expect(deletedAfterDelete.slots.slots.size() == 1U,
                "product deleted scan one slot") &&
         expect(deletedAfterDelete.slots.compatibleCount == 1U,
                "product deleted scan compatible") &&
         expect(deletedAfterDelete.slots.slots.front().id == "save_001",
                "product deleted scan id") &&
         expect(deletedAfterDelete.slots.slots.front().enabled,
                "product deleted scan enabled") &&
         expect(recovered.ok, "product deleted scan recover ok") &&
         expect(deletedAfterRecover.slots.slots.empty(),
                "product deleted scan empty after recover") &&
         expect(activeAfterRecover.slots.slots.size() == 1U,
                "product active scan restored") &&
         expect(activeAfterRecover.slots.slots.front().id == "save_001",
                "product active scan restored id");
}

bool productDeletedScanUsesMovedSnapshotSidecar() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, session, "attempt_001", "save_001"));
  {
    std::ofstream snapshot(root / "save_001.snapshot.png");
    snapshot << "snapshot bytes";
  }
  const iggy3d::ProductSaveSoftDeleteResult deleted =
      iggy3d::softDeleteProductSave({root, "save_001"});
  const iggy3d::ProductSaveBridgeResult deletedScan =
      iggy3d::scanDeletedProductSaves(root, "iggy3d.movement_playground",
                                      "movement_playground.runtime_loop");
  return expect(written.ok, "product deleted snapshot setup write ok") &&
         expect(deleted.ok, "product deleted snapshot soft delete ok") &&
         expect(deleted.snapshotMoved, "product deleted snapshot moved") &&
         expect(deletedScan.slots.slots.size() == 1U,
                "product deleted snapshot scanned") &&
         expect(deletedScan.slots.slots.front().snapshotPath ==
                    root / "deleted" / "save_001.snapshot.png",
                "product deleted snapshot path") &&
         expect(deletedScan.slots.slots.front().snapshotAvailable,
                "product deleted snapshot available") &&
         expect(!deletedScan.slots.slots.front().snapshotFallback,
                "product deleted snapshot no fallback") &&
         expect(deletedScan.slots.slots.front().snapshotStatus == "available",
                "product deleted snapshot status");
}

bool productLoadSaveLoadsCompatibleSession() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, savedSession, "attempt_001", "save_001"));
  iggy3d::Session destination = makeFixtureSession();
  const std::uint64_t previousHash = destination.stateHash();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product load setup write ok") &&
         expect(loaded.ok, "product load ok") &&
         expect(loaded.status == "product_save_loaded", "product load status") &&
         expect(loaded.reasonCode == "product_save_loaded",
                "product load reason") &&
         expect(loaded.fileRead, "product load file read") &&
         expect(loaded.decoded, "product load decoded") &&
         expect(loaded.compatibilityChecked, "product load compatibility") &&
         expect(loaded.sessionLoaded, "product load session loaded") &&
         expect(loaded.record.id == "save_001", "product load record id") &&
         expect(loaded.record.path == written.record.path,
                "product load record path") &&
         expect(loaded.previousHash == previousHash,
                "product load previous hash") &&
         expect(loaded.loadedHash == written.record.savedStateHash,
                "product load loaded hash") &&
         expect(destination.stateHash() == written.record.savedStateHash,
                "product load destination hash") &&
         expect(destination.stateHash() != previousHash,
                "product load mutated destination") &&
         expect(loaded.codecStatus == iggy3d::SaveCodecStatus::Ok,
                "product load codec ok") &&
         expect(loaded.loadStatus == iggy3d::SaveLoadStatus::Ok,
                "product load status ok") &&
         expect(loaded.compatibilityStatus ==
                    iggy3d::SaveCompatibilityStatus::Compatible,
                "product load compatible") &&
         expect(loaded.sessionLoadStatus == iggy3d::SessionLoadStatus::Ok,
                "product load session status");
}

bool productLoadSaveExposesAuthoredRoomSection() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  iggy3d::SaveAuthoredRoomSection authoredRoom = authoredRoomFixture();
  iggy3d::ProductSaveWriteRequest writeRequest =
      productSaveRequest(root, savedSession, "attempt_001", "save_001");
  writeRequest.authoredRoom = &authoredRoom;
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(writeRequest);
  iggy3d::Session destination = makeFixtureSession();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product authored load setup write ok") &&
         expect(loaded.ok, "product authored load ok") &&
         expect(loaded.authoredRoomPresent,
                "product authored load room present") &&
         expect(loaded.authoredRoomId == "product_bridge_room",
                "product authored load room id") &&
         expect(loaded.authoredFloorCount == 1U,
                "product authored load floor count") &&
         expect(loaded.authoredWallCount == 0U,
                "product authored load wall count") &&
         expect(loaded.authoredMarkerCount == 1U,
                "product authored load marker count") &&
         expect(loaded.authoredRoom.present,
                "product authored load copied room present") &&
         expect(loaded.authoredRoom.floors.size() == 1U,
                "product authored load copied floor") &&
         expect(loaded.authoredRoom.markers.size() == 1U,
                "product authored load copied marker") &&
         expect(loaded.sessionLoaded,
                "product authored load session loaded");
}

bool productLoadSaveRejectsMissingSessionBeforeIo() {
  iggy3d::ProductSaveLoadRequest request;
  request.path = "/tmp/iggy3d_product_save_bridge_tests_missing_session.iggy3d.save";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product load missing session rejected") &&
         expect(loaded.status == "product_save_load_session_missing",
                "product load missing session status") &&
         expect(loaded.reasonCode == "product_save_load_session_missing",
                "product load missing session reason") &&
         expect(!loaded.fileRead, "product load missing session no read") &&
         expect(!loaded.sessionLoaded,
                "product load missing session not loaded");
}

bool productLoadSaveRejectsMissingPathBeforeIo() {
  iggy3d::Session session = makeFixtureSession();
  const std::uint64_t previousHash = session.stateHash();
  iggy3d::ProductSaveLoadRequest request;
  request.session = &session;
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product load missing path rejected") &&
         expect(loaded.status == "product_save_load_path_missing",
                "product load missing path status") &&
         expect(loaded.reasonCode == "product_save_load_path_missing",
                "product load missing path reason") &&
         expect(!loaded.fileRead, "product load missing path no read") &&
         expect(session.stateHash() == previousHash,
                "product load missing path no mutation");
}

bool productLoadSaveMissingFilePreservesSession() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const std::uint64_t previousHash = session.stateHash();
  iggy3d::ProductSaveLoadRequest request;
  request.path = root / "missing.iggy3d.save";
  request.session = &session;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product load missing file rejected") &&
         expect(loaded.status == "save_file_read_failed",
                "product load missing file status") &&
         expect(loaded.reasonCode == "save_file_read_failed",
                "product load missing file reason") &&
         expect(!loaded.fileRead, "product load missing file no read") &&
         expect(!loaded.sessionLoaded,
                "product load missing file not loaded") &&
         expect(session.stateHash() == previousHash,
                "product load missing file no mutation");
}

bool productLoadSaveRejectsIncompatiblePackage() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, savedSession, "attempt_001", "save_001"));
  iggy3d::Session destination = makeFixtureSession();
  const std::uint64_t previousHash = destination.stateHash();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "wrong.package";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product incompatible setup write ok") &&
         expect(!loaded.ok, "product incompatible rejected") &&
         expect(loaded.status == "product_save_load_compatibility_failed",
                "product incompatible status") &&
         expect(loaded.reasonCode == "product_save_load_compatibility_failed",
                "product incompatible reason") &&
         expect(loaded.fileRead, "product incompatible read") &&
         expect(loaded.decoded, "product incompatible decoded") &&
         expect(loaded.compatibilityChecked,
                "product incompatible compatibility checked") &&
         expect(loaded.compatibilityStatus ==
                    iggy3d::SaveCompatibilityStatus::PackageMismatch,
                "product incompatible package status") &&
         expect(!loaded.sessionLoaded, "product incompatible not loaded") &&
         expect(destination.stateHash() == previousHash,
                "product incompatible no mutation");
}

bool productLoadSaveRejectsIncompatibleScenario() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session savedSession = makeChangedFixtureSession();
  const iggy3d::ProductSaveWriteResult written =
      iggy3d::writeProductSessionSaveDurably(
          productSaveRequest(root, savedSession, "attempt_001", "save_001"));
  iggy3d::Session destination = makeFixtureSession();
  const std::uint64_t previousHash = destination.stateHash();

  iggy3d::ProductSaveLoadRequest request;
  request.path = written.record.path;
  request.session = &destination;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "wrong.scenario";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);

  return expect(written.ok, "product scenario setup write ok") &&
         expect(!loaded.ok, "product scenario rejected") &&
         expect(loaded.status == "product_save_load_compatibility_failed",
                "product scenario status") &&
         expect(loaded.compatibilityStatus ==
                    iggy3d::SaveCompatibilityStatus::ScenarioMismatch,
                "product scenario status enum") &&
         expect(!loaded.sessionLoaded, "product scenario not loaded") &&
         expect(destination.stateHash() == previousHash,
                "product scenario no mutation");
}

bool productLoadSaveRejectsCorruptFile() {
  const std::filesystem::path root = testRoot();
  const std::filesystem::path path = root / "corrupt.iggy3d.save";
  {
    std::ofstream output(path);
    output << "not an iggy3d save\n";
  }
  iggy3d::Session session = makeFixtureSession();
  const std::uint64_t previousHash = session.stateHash();
  iggy3d::ProductSaveLoadRequest request;
  request.path = path;
  request.session = &session;
  request.expectedPackageId = "iggy3d.movement_playground";
  request.expectedScenarioId = "movement_playground.runtime_loop";
  const iggy3d::ProductSaveLoadResult loaded =
      iggy3d::loadProductSessionSave(request);
  return expect(!loaded.ok, "product corrupt rejected") &&
         expect(loaded.status == "save_file_decode_failed",
                "product corrupt status") &&
         expect(loaded.reasonCode == "save_file_decode_failed",
                "product corrupt reason") &&
         expect(!loaded.fileRead, "product corrupt file not accepted") &&
         expect(!loaded.sessionLoaded, "product corrupt not loaded") &&
         expect(session.stateHash() == previousHash,
                "product corrupt no mutation");
}

}  // namespace

int main() {
  const bool ok = productDurableSaveWritesFinalAndScans() &&
                  productDurableSaveHonorsValidIdHint() &&
                  productDurableSaveRejectsMissingState() &&
                  productDurableSaveRejectsInvalidAttemptToken() &&
                  productDurableSavePersistsAuthoredRoom() &&
                  productSoftDeleteMovesSaveAndRemovesFromScan() &&
                  productSoftDeleteMovesSnapshotSidecarWhenPresent() &&
                  productSoftDeleteRejectsMissingIdBeforeIo() &&
                  productSoftDeleteRejectsInvalidId() &&
                  productSoftDeleteForwardsMissingSource() &&
                  productSoftDeleteRejectsExistingDeletedTarget() &&
                  productRecoverRestoresSoftDeletedSaveAndScans() &&
                  productRecoverMovesSnapshotSidecarWhenPresent() &&
                  productRecoverRejectsMissingIdBeforeIo() &&
                  productRecoverRejectsInvalidId() &&
                  productRecoverForwardsMissingSource() &&
                  productRecoverRejectsExistingActiveTarget() &&
                  productDeletedScanShowsSoftDeletedSaveThenClearsOnRecover() &&
                  productDeletedScanUsesMovedSnapshotSidecar() &&
                  productLoadSaveLoadsCompatibleSession() &&
                  productLoadSaveExposesAuthoredRoomSection() &&
                  productLoadSaveRejectsMissingSessionBeforeIo() &&
                  productLoadSaveRejectsMissingPathBeforeIo() &&
                  productLoadSaveMissingFilePreservesSession() &&
                  productLoadSaveRejectsIncompatiblePackage() &&
                  productLoadSaveRejectsIncompatibleScenario() &&
                  productLoadSaveRejectsCorruptFile();
  return ok ? 0 : 1;
}
