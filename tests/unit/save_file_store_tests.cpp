#include "content/PackageLoader.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
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
  authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomWallRecord wall;
  wall.id = "edit_wall_1";
  wall.startMeters = {0.0F, 0.0F, 0.0F};
  wall.endMeters = {2.0F, 0.0F, 0.0F};
  wall.semantics.materialId = "debug_wall";
  wall.semantics.blocksActor = true;
  wall.semantics.blocksProjectile = true;
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
         expect(decoded.envelope.authoredRoom.walls.size() == 1U, "authored wall saved");
}

}  // namespace

int main() {
  const bool ok = writeListReadAndDeleteRoundTrips() && missingStateIsRejected() &&
                  idHintOverwritesExistingSave() && authoredRoomSectionIsWrittenToSaveFile();
  return ok ? 0 : 1;
}
