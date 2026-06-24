#include "content/PackageLoader.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
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
      std::filesystem::temp_directory_path() / "iggy3d_save_file_store_tests";
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

iggy3d::SaveFileTempValidationResult writeAndValidateTempSave(
    const std::filesystem::path& root,
    std::string_view saveId,
    std::string_view attempt,
    std::string_view encodedText) {
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, saveId, attempt);
  const iggy3d::SaveFileTempWriteResult written =
      iggy3d::writeDurableSaveTempFile({plan, std::string(encodedText)});
  return iggy3d::validateDurableSaveTempFile(written);
}

bool saveFileIdValidationMatchesStorePolicy() {
  return expect(iggy3d::isValidSaveFileId("save_001"), "save_001 valid") &&
         expect(iggy3d::isValidSaveFileId("save-001"), "save-001 valid") &&
         expect(iggy3d::isValidSaveFileId("manual_save_01"),
                "manual_save_01 valid") &&
         expect(!iggy3d::isValidSaveFileId(""), "empty invalid") &&
         expect(!iggy3d::isValidSaveFileId(" "), "whitespace invalid") &&
         expect(!iggy3d::isValidSaveFileId("save/001"), "slash invalid") &&
         expect(!iggy3d::isValidSaveFileId("save\\001"), "backslash invalid") &&
         expect(!iggy3d::isValidSaveFileId("."), "dot invalid") &&
         expect(!iggy3d::isValidSaveFileId(".."), "dotdot invalid") &&
         expect(!iggy3d::isValidSaveFileId("save 001"), "space invalid") &&
         expect(!iggy3d::isValidSaveFileId("save:001"), "punctuation invalid");
}

bool saveFilePathHelpersAreDeterministic() {
  const std::filesystem::path root = testRoot();
  const std::filesystem::path finalPath =
      iggy3d::saveFilePathForId(root, "save_001");
  const std::filesystem::path snapshotPath =
      iggy3d::saveSnapshotPathForId(root, "save_001");
  return expect(finalPath == root / "save_001.iggy3d.save",
                "final save path") &&
         expect(snapshotPath == root / "save_001.snapshot.png",
                "snapshot path");
}

bool durableWritePlanBuildsSameDirectoryPaths() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  const std::string tempFilename = plan.paths.tempPath.filename().string();
  return expect(plan.ok, "durable plan ok") &&
         expect(plan.reason == "durable_save_plan_ready",
                "durable plan reason") &&
         expect(plan.paths.root == root, "plan root") &&
         expect(plan.paths.id == "save_001", "plan id") &&
         expect(plan.paths.attemptToken == "attempt_001", "plan attempt") &&
         expect(plan.paths.finalPath == root / "save_001.iggy3d.save",
                "plan final") &&
         expect(plan.paths.tempPath ==
                    root / "save_001.iggy3d.save.tmp_attempt_001",
                "plan temp") &&
         expect(plan.paths.snapshotPath == root / "save_001.snapshot.png",
                "plan snapshot") &&
         expect(plan.paths.tempPath.parent_path() ==
                    plan.paths.finalPath.parent_path(),
                "temp same directory") &&
         expect(plan.paths.tempPath != plan.paths.finalPath,
                "temp not final") &&
         expect(!tempFilename.ends_with(".iggy3d.save"),
                "temp not normal save extension");
}

bool durableWritePlanRejectsInvalidInputs() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan invalidId =
      iggy3d::planDurableSaveFileWrite(root, "save/001", "attempt_001");
  const iggy3d::SaveFileDurableWritePlan invalidAttempt =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt 001");
  const iggy3d::SaveFileDurableWritePlan emptyAttempt =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "");
  return expect(!invalidId.ok, "invalid id rejected") &&
         expect(invalidId.reason == "durable_save_invalid_id",
                "invalid id reason") &&
         expect(invalidId.paths.finalPath.empty(), "invalid id no final") &&
         expect(!invalidAttempt.ok, "invalid attempt rejected") &&
         expect(invalidAttempt.reason == "durable_save_invalid_attempt_token",
                "invalid attempt reason") &&
         expect(!emptyAttempt.ok, "empty attempt rejected") &&
         expect(emptyAttempt.reason == "durable_save_invalid_attempt_token",
                "empty attempt reason");
}

bool durableTempPathIsNotListedAsSave() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  {
    std::ofstream temp(plan.paths.tempPath);
    temp << "not a final save";
  }
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(plan.ok, "temp list plan ok") &&
         expect(std::filesystem::exists(plan.paths.tempPath), "temp exists") &&
         expect(listed.empty(), "temp not listed");
}

bool durableTempWriteReadsBackFromDisk() {
  const std::filesystem::path root = testRoot();
  const std::string encodedText = "iggy3d.save_envelope.v1\nmetadata.id=unit\n";
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  const iggy3d::SaveFileTempWriteResult result =
      iggy3d::writeDurableSaveTempFile({plan, encodedText});
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(result.ok, "temp write ok") &&
         expect(result.reason == "durable_save_temp_written",
                "temp write reason") &&
         expect(result.paths.tempPath == plan.paths.tempPath, "temp path") &&
         expect(result.paths.finalPath == plan.paths.finalPath, "final path") &&
         expect(result.encodedBytes == encodedText.size(), "encoded bytes") &&
         expect(result.readBackBytes == encodedText.size(), "readback bytes") &&
         expect(result.rootCreated, "root created") &&
         expect(result.tempWritten, "temp written") &&
         expect(result.tempClosed, "temp closed") &&
         expect(result.tempReadBack, "temp readback") &&
         expect(result.readBackText == encodedText, "readback matches") &&
         expect(std::filesystem::exists(plan.paths.tempPath), "temp exists") &&
         expect(!std::filesystem::exists(plan.paths.finalPath),
                "final not written") &&
         expect(listed.empty(), "temp write not listed");
}

bool durableTempWriteRejectsInvalidPlan() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save/001", "attempt_001");
  const iggy3d::SaveFileTempWriteResult result =
      iggy3d::writeDurableSaveTempFile({plan, "payload"});
  return expect(!result.ok, "invalid plan temp write rejected") &&
         expect(result.reason == "durable_save_invalid_id",
                "invalid plan forwarded reason") &&
         expect(!result.tempWritten, "invalid plan not written") &&
         expect(!result.tempClosed, "invalid plan not closed") &&
         expect(!result.tempReadBack, "invalid plan not readback") &&
         expect(result.paths.tempPath.empty() ||
                    !std::filesystem::exists(result.paths.tempPath),
                "invalid plan no temp file");
}

bool durableTempWriteRejectsEmptyPayload() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  const iggy3d::SaveFileTempWriteResult result =
      iggy3d::writeDurableSaveTempFile({plan, ""});
  return expect(!result.ok, "empty payload rejected") &&
         expect(result.reason == "durable_save_empty_payload",
                "empty payload reason") &&
         expect(!result.rootCreated, "empty payload no root create") &&
         expect(!result.tempWritten, "empty payload not written") &&
         expect(!result.tempClosed, "empty payload not closed") &&
         expect(!result.tempReadBack, "empty payload not readback") &&
         expect(!std::filesystem::exists(plan.paths.tempPath),
                "empty payload no temp file");
}

bool durableTempValidationDecodesSaveFromDisk() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::SaveStateResult saved =
      iggy3d::saveSessionStateEncoded(session.state());
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  const iggy3d::SaveFileTempWriteResult written =
      iggy3d::writeDurableSaveTempFile({plan, saved.encodedSaveText});
  const iggy3d::SaveFileTempValidationResult validated =
      iggy3d::validateDurableSaveTempFile(written);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(saved.status == iggy3d::SaveLoadStatus::Ok,
                "fixture save encoded") &&
         expect(written.ok, "validation temp write ok") &&
         expect(validated.ok, "temp validation ok") &&
         expect(validated.reason == "durable_save_temp_validated",
                "temp validation reason") &&
         expect(validated.paths.tempPath == plan.paths.tempPath,
                "validation temp path") &&
         expect(validated.packageId == "iggy3d.movement_playground",
                "validation package") &&
         expect(validated.scenarioId == "movement_playground.runtime_loop",
                "validation scenario") &&
         expect(validated.savedStateHash == saved.savedStateHash,
                "validation hash") &&
         expect(!validated.savedStateHashHex.empty(), "validation hash hex") &&
         expect(validated.encodedBytes == saved.encodedSaveText.size(),
                "validation encoded bytes") &&
         expect(validated.tempRead, "validation temp read") &&
         expect(validated.tempDecoded, "validation temp decoded") &&
         expect(validated.tempValidated, "validation temp validated") &&
         expect(validated.codecStatus == iggy3d::SaveCodecStatus::Ok,
                "validation codec ok") &&
         expect(!std::filesystem::exists(plan.paths.finalPath),
                "validation final not written") &&
         expect(listed.empty(), "validation temp not listed");
}

bool durableTempValidationRejectsCorruptTempPayload() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  const iggy3d::SaveFileTempWriteResult written =
      iggy3d::writeDurableSaveTempFile({plan, "not a save envelope\n"});
  const iggy3d::SaveFileTempValidationResult validated =
      iggy3d::validateDurableSaveTempFile(written);
  return expect(written.ok, "corrupt temp write ok") &&
         expect(!validated.ok, "corrupt validation rejected") &&
         expect(validated.reason == "durable_save_temp_decode_failed",
                "corrupt validation reason") &&
         expect(validated.tempRead, "corrupt validation read") &&
         expect(!validated.tempDecoded, "corrupt validation not decoded") &&
         expect(!validated.tempValidated, "corrupt validation not validated") &&
         expect(validated.codecStatus != iggy3d::SaveCodecStatus::Ok,
                "corrupt codec status") &&
         expect(!std::filesystem::exists(plan.paths.finalPath),
                "corrupt final not written");
}

bool durableTempValidationRejectsMissingTempFile() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt_001");
  const iggy3d::SaveFileTempWriteResult written =
      iggy3d::writeDurableSaveTempFile({plan, "not a save envelope\n"});
  std::error_code error;
  std::filesystem::remove(plan.paths.tempPath, error);
  const iggy3d::SaveFileTempValidationResult validated =
      iggy3d::validateDurableSaveTempFile(written);
  return expect(written.ok, "missing temp write ok") &&
         expect(!validated.ok, "missing temp validation rejected") &&
         expect(validated.reason == "durable_save_temp_read_failed",
                "missing temp reason") &&
         expect(!validated.tempRead, "missing temp not read") &&
         expect(!validated.tempDecoded, "missing temp not decoded") &&
         expect(!validated.tempValidated, "missing temp not validated");
}

bool durableTempValidationForwardsFailedWriteReason() {
  const std::filesystem::path root = testRoot();
  const iggy3d::SaveFileDurableWritePlan plan =
      iggy3d::planDurableSaveFileWrite(root, "save_001", "attempt 001");
  const iggy3d::SaveFileTempWriteResult written =
      iggy3d::writeDurableSaveTempFile({plan, "payload"});
  const iggy3d::SaveFileTempValidationResult validated =
      iggy3d::validateDurableSaveTempFile(written);
  return expect(!written.ok, "failed write setup") &&
         expect(!validated.ok, "failed write validation rejected") &&
         expect(validated.reason == "durable_save_invalid_attempt_token",
                "failed write validation forwarded reason") &&
         expect(!validated.tempRead, "failed write validation not read") &&
         expect(!validated.tempDecoded, "failed write validation not decoded") &&
         expect(!validated.tempValidated,
                "failed write validation not validated");
}

bool durableFinalCommitValidatesNewSave() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  const iggy3d::SaveStateResult saved =
      iggy3d::saveSessionStateEncoded(session.state());
  const iggy3d::SaveFileTempValidationResult validation =
      writeAndValidateTempSave(root, "save_001", "attempt_001",
                               saved.encodedSaveText);
  const iggy3d::SaveFileFinalCommitResult committed =
      iggy3d::commitDurableSaveTempFile(validation);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(validation.ok, "new commit validation ok") &&
         expect(committed.ok, "new commit ok") &&
         expect(committed.reason == "durable_save_final_validated",
                "new commit reason") &&
         expect(!committed.previousExisted, "new commit no previous") &&
         expect(committed.previousPreserved, "new commit previous preserved") &&
         expect(committed.tempValidated, "new commit temp validated") &&
         expect(committed.committed, "new commit renamed") &&
         expect(committed.finalRead, "new commit final read") &&
         expect(committed.finalDecoded, "new commit final decoded") &&
         expect(committed.finalValidated, "new commit final validated") &&
         expect(committed.packageId == "iggy3d.movement_playground",
                "new commit package") &&
         expect(committed.scenarioId == "movement_playground.runtime_loop",
                "new commit scenario") &&
         expect(committed.savedStateHash == saved.savedStateHash,
                "new commit hash") &&
         expect(!committed.savedStateHashHex.empty(), "new commit hash hex") &&
         expect(committed.encodedBytes == saved.encodedSaveText.size(),
                "new commit encoded bytes") &&
         expect(std::filesystem::exists(validation.paths.finalPath),
                "new commit final exists") &&
         expect(!std::filesystem::exists(validation.paths.tempPath),
                "new commit temp consumed") &&
         expect(listed.size() == 1U, "new commit listed") &&
         expect(listed.front().savedStateHash == saved.savedStateHash,
                "new commit listed hash");
}

bool durableFinalCommitForwardsInvalidValidationReason() {
  const std::filesystem::path root = testRoot();
  iggy3d::SaveFileTempValidationResult validation;
  validation.reason = "durable_save_temp_decode_failed";
  validation.paths = iggy3d::planDurableSaveFileWrite(root, "save_001",
                                                      "attempt_001")
                         .paths;
  const iggy3d::SaveFileFinalCommitResult committed =
      iggy3d::commitDurableSaveTempFile(validation);
  return expect(!committed.ok, "invalid validation commit rejected") &&
         expect(committed.reason == "durable_save_temp_decode_failed",
                "invalid validation reason forwarded") &&
         expect(!committed.tempValidated, "invalid validation not validated") &&
         expect(!committed.committed, "invalid validation not committed") &&
         expect(!std::filesystem::exists(validation.paths.finalPath),
                "invalid validation no final");
}

bool durableFinalCommitReplacesExistingFinalOnThisHost() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session firstSession = makeFixtureSession();
  const iggy3d::SaveStateResult firstSaved =
      iggy3d::saveSessionStateEncoded(firstSession.state());
  const iggy3d::SaveFileTempValidationResult firstValidation =
      writeAndValidateTempSave(root, "save_001", "attempt_001",
                               firstSaved.encodedSaveText);
  const iggy3d::SaveFileFinalCommitResult firstCommit =
      iggy3d::commitDurableSaveTempFile(firstValidation);

  iggy3d::Session secondSession = makeFixtureSession();
  secondSession.mutableStateForOwnedSystems().clock.tickIndex = 1;
  const iggy3d::SaveStateResult secondSaved =
      iggy3d::saveSessionStateEncoded(secondSession.state());
  const iggy3d::SaveFileTempValidationResult secondValidation =
      writeAndValidateTempSave(root, "save_001", "attempt_002",
                               secondSaved.encodedSaveText);
  const iggy3d::SaveFileFinalCommitResult secondCommit =
      iggy3d::commitDurableSaveTempFile(secondValidation);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(firstCommit.ok, "overwrite first commit ok") &&
         expect(secondSaved.savedStateHash != firstSaved.savedStateHash,
                "overwrite second hash differs") &&
         expect(secondValidation.ok, "overwrite second validation ok") &&
         expect(secondCommit.ok, "overwrite second commit ok") &&
         expect(secondCommit.reason == "durable_save_final_validated",
                "overwrite commit reason") &&
         expect(secondCommit.previousExisted, "overwrite previous existed") &&
         expect(!secondCommit.previousPreserved,
                "overwrite previous not preserved after success") &&
         expect(secondCommit.committed, "overwrite committed") &&
         expect(secondCommit.finalValidated, "overwrite final validated") &&
         expect(secondCommit.savedStateHash == secondSaved.savedStateHash,
                "overwrite final hash") &&
         expect(std::filesystem::exists(secondValidation.paths.finalPath),
                "overwrite final exists") &&
         expect(!std::filesystem::exists(secondValidation.paths.tempPath),
                "overwrite temp consumed") &&
         expect(listed.size() == 1U, "overwrite one listed") &&
         expect(listed.front().savedStateHash == secondSaved.savedStateHash,
                "overwrite listed second hash");
}

bool writeListReadAndDeleteRoundTrips() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveFileWriteRequest request;
  request.root = root;
  request.state = &session.state();
  const iggy3d::SaveFileWriteResult written = iggy3d::writeSessionSaveFile(request);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path) : iggy3d::SaveFileReadResult{};
  const bool fileExists = written.ok && std::filesystem::exists(written.record.path);
  const bool deleted =
      written.ok && !listed.empty() && iggy3d::deleteSaveFile(listed.front().path);
  const std::vector<iggy3d::SaveFileRecord> afterDelete = iggy3d::listSaveFiles(root);
  return expect(written.ok, "save file written") &&
         expect(written.reason == "save_file_written", "write reason") &&
         expect(written.record.id == "save_001", "save id") &&
         expect(written.record.packageId == "iggy3d.movement_playground", "package id") &&
         expect(written.record.scenarioId == "movement_playground.runtime_loop",
                "scenario id") &&
         expect(written.encodedBytes > 0U, "encoded bytes") &&
         expect(fileExists, "save file exists") &&
         expect(listed.size() == 1U, "one save listed") &&
         expect(listed.front().savedStateHash == session.stateHash(), "listed hash") &&
         expect(read.ok, "save file read") &&
         expect(read.encodedText.starts_with("iggy3d.save_envelope.v1\n"), "save header") &&
         expect(read.record.savedStateHash == session.stateHash(), "read hash") &&
         expect(deleted, "save file deleted") &&
         expect(afterDelete.empty(), "saves empty after delete");
}

bool missingStateIsRejected() {
  iggy3d::SaveFileWriteRequest request;
  request.root = testRoot();
  const iggy3d::SaveFileWriteResult result = iggy3d::writeSessionSaveFile(request);
  return expect(!result.ok, "missing state rejected") &&
         expect(result.reason == "save_state_missing", "missing state reason");
}

bool idHintOverwritesExistingSave() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveFileWriteRequest first;
  first.root = root;
  first.state = &session.state();
  const iggy3d::SaveFileWriteResult firstWrite = iggy3d::writeSessionSaveFile(first);
  iggy3d::SaveFileWriteRequest second = first;
  second.idHint = "save_001";
  const iggy3d::SaveFileWriteResult secondWrite = iggy3d::writeSessionSaveFile(second);
  const std::vector<iggy3d::SaveFileRecord> listed = iggy3d::listSaveFiles(root);
  return expect(firstWrite.ok, "first write") && expect(secondWrite.ok, "second write") &&
         expect(secondWrite.record.id == "save_001", "overwrite id") &&
         expect(listed.size() == 1U, "overwrite keeps one save");
}

bool authoredRoomSectionIsWrittenToSaveFile() {
  const std::filesystem::path root = testRoot();
  iggy3d::Session session = makeFixtureSession();
  iggy3d::SaveAuthoredRoomSection authoredRoom;
  authoredRoom.present = true;
  authoredRoom.id = "editor_saved_room";
  iggy3d::SaveAuthoredRoomFloorRecord floor;
  floor.id = "edit_floor_1";
  floor.centerMeters = {1.0F, 0.0F, 1.0F};
  floor.sizeMeters = {2.0F, 0.1F, 2.0F};
  floor.semantics.materialId = "debug_floor";
  floor.semantics.walkable = true;
  floor.semantics.traversalTags = {"walkable"};
  floor.locked = true;
  floor.hidden = true;
  authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomWallRecord wall;
  wall.id = "edit_wall_1";
  wall.startMeters = {0.0F, 0.0F, 0.0F};
  wall.endMeters = {2.0F, 0.0F, 0.0F};
  wall.semantics.materialId = "debug_wall";
  wall.semantics.blocksActor = true;
  wall.semantics.blocksProjectile = true;
  wall.semantics.traversalTags = {"clamber"};
  wall.locked = true;
  wall.hidden = true;
  authoredRoom.walls.push_back(wall);

  iggy3d::SaveFileWriteRequest request;
  request.root = root;
  request.state = &session.state();
  request.authoredRoom = &authoredRoom;
  const iggy3d::SaveFileWriteResult written = iggy3d::writeSessionSaveFile(request);
  const iggy3d::SaveFileReadResult read =
      written.ok ? iggy3d::readSaveFile(written.record.path) : iggy3d::SaveFileReadResult{};
  const iggy3d::SaveDecodeResult decoded =
      read.ok ? iggy3d::decodeSaveEnvelope(read.encodedText) : iggy3d::SaveDecodeResult{};

  return expect(written.ok, "authored save file written") &&
         expect(read.ok, "authored save file read") &&
         expect(read.encodedText.find("authoredRoom.present=true\n") != std::string::npos,
                "authored room encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "authored save decoded") &&
         expect(decoded.envelope.authoredRoom.present, "authored room present") &&
         expect(decoded.envelope.authoredRoom.floors.size() == 1U, "authored floor saved") &&
         expect(decoded.envelope.authoredRoom.floors[0].locked, "authored floor locked") &&
         expect(decoded.envelope.authoredRoom.floors[0].hidden, "authored floor hidden") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.traversalTags[0] ==
                    "walkable",
                "authored floor traversal saved") &&
         expect(decoded.envelope.authoredRoom.walls.size() == 1U, "authored wall saved") &&
         expect(decoded.envelope.authoredRoom.walls[0].locked, "authored wall locked") &&
         expect(decoded.envelope.authoredRoom.walls[0].hidden, "authored wall hidden") &&
         expect(decoded.envelope.authoredRoom.walls[0].semantics.traversalTags[0] ==
                    "clamber",
                "authored wall traversal saved");
}

}  // namespace

int main() {
  const bool ok = saveFileIdValidationMatchesStorePolicy() &&
                  saveFilePathHelpersAreDeterministic() &&
                  durableWritePlanBuildsSameDirectoryPaths() &&
                  durableWritePlanRejectsInvalidInputs() &&
                  durableTempPathIsNotListedAsSave() &&
                  durableTempWriteReadsBackFromDisk() &&
                  durableTempWriteRejectsInvalidPlan() &&
                  durableTempWriteRejectsEmptyPayload() &&
                  durableTempValidationDecodesSaveFromDisk() &&
                  durableTempValidationRejectsCorruptTempPayload() &&
                  durableTempValidationRejectsMissingTempFile() &&
                  durableTempValidationForwardsFailedWriteReason() &&
                  durableFinalCommitValidatesNewSave() &&
                  durableFinalCommitForwardsInvalidValidationReason() &&
                  durableFinalCommitReplacesExistingFinalOnThisHost() &&
                  writeListReadAndDeleteRoundTrips() && missingStateIsRejected() &&
                  idHintOverwritesExistingSave() && authoredRoomSectionIsWrittenToSaveFile();
  return ok ? 0 : 1;
}
